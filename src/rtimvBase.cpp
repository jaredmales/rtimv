
#include "rtimvBase.hpp"

#ifdef RTIMV_MILK
#include "images/shmimImage.hpp"
#endif

#include "images/fitsImage.hpp"
#include "images/fitsDirectory.hpp"
#include "images/mzmqImage.hpp"

rtimvBase::rtimvBase( QWidget *Parent,
                      Qt::WindowFlags f) : QWidget(Parent, f)
{
}

rtimvBase::rtimvBase( const std::vector<std::string> &shkeys,
                      QWidget *Parent,
                      Qt::WindowFlags f) : QWidget(Parent, f)
{
    startup(shkeys);
}

void rtimvBase::startup(const std::vector<std::string> &shkeys)
{
    m_images.resize(4, nullptr);

    for(size_t i = 0; i < m_images.size(); ++i)
    {
        if(shkeys.size() > i)
        {
            if(shkeys[i] != "")
            {
                // safely accept several different common fits extensions
                bool isFits = false;
                if(shkeys[i].size() > 4)
                {
                    if(shkeys[i].rfind(".fit") == shkeys[i].size() - 4 ||
                        shkeys[i].rfind(".FIT") == shkeys[i].size() - 4)
                        isFits = true;
                }
                if(shkeys[i].size() > 5 && !isFits)
                {
                    if(shkeys[i].rfind(".fits") == shkeys[i].size() - 5 ||
                        shkeys[i].rfind(".FITS") == shkeys[i].size() - 5)
                        isFits = true;
                }

                bool isDirectory = false;
                if(!isFits)
                {
                    if(shkeys[i][shkeys[i].size() - 1] == '/')
                    {
                        isDirectory = true;
                    }
                }

                if(isFits)
                {
                    fitsImage *fi = new fitsImage(&m_rawMutex);
                    m_images[i] = (rtimvImage *)fi;
                }
                else if(isDirectory)
                {
                    fitsDirectory *fd = new fitsDirectory(&m_rawMutex);
                    m_images[i] = (rtimvImage *)fd;
                }
                else if(shkeys[i].find('@') != std::string::npos || shkeys[i].find(':') != std::string::npos || m_mzmqAlways == true)
                {
                    mzmqImage *mi = new mzmqImage(&m_rawMutex);

                    // change defaults
                    std::cerr << m_mzmqServer << "\n";
                    if(m_mzmqServer != "")
                        mi->imageServer(m_mzmqServer);
                    if(m_mzmqPort != 0)
                        mi->imagePort(m_mzmqPort);

                    m_images[i] = (rtimvImage *)mi;
                }
                else
                {
                    #ifdef RTIMV_MILK
                    //If we get here we try to interpret as a ImageStreamIO image
                    shmimImage *si = new shmimImage(&m_rawMutex);
                    m_images[i] = (rtimvImage *)si;
                    #else
                    qFatal("Unrecognized image key format");
                    #endif
                }

                m_images[i]->imageKey(shkeys[i]); // Set the key
            }
        }
    }

    // Turn on features if images exist:
    if(m_images[1] != nullptr)
    {
        m_subtractDark = true;
    }

    if(m_images[2] != nullptr)
    {
        m_applyMask = true;
    }

    if(m_images[3] != nullptr)
    {
        m_applySatMask = true;
    }

    connect(&m_imageTimer, SIGNAL(timeout()), this, SLOT(updateImages()));
}

bool rtimvBase::imageValid(size_t n)
{
    if(n >= m_images.size())
    {
        return false;
    }

    if(m_images[n] == nullptr)
    {
        return false;
    }

    return m_images[n]->valid();
}

void rtimvBase::setImsize(uint32_t x, uint32_t y)
{
    //Always have at least one pixel
    if(x == 0) x = 1;
    if(y == 0) y = 1;

    if(m_nx != x || m_ny != y || m_calData == 0 || m_qim == 0)
    {
        m_nx = x;
        m_ny = y;

        if(m_calData != nullptr)
        {
            delete[] m_calData;
            m_calData = nullptr;
        }  

        if(m_satData != nullptr)
        {
            delete[] m_satData;
            m_satData = nullptr;
        }

        m_calData = new float[m_nx*m_ny];
        m_satData = new uint8_t[m_nx*m_ny];

        if(m_qim != nullptr)
        {
            delete m_qim;
            m_qim = nullptr;
        }

        m_qim = new QImage(m_nx, m_ny, QImage::Format_Indexed8);

        load_colorbar(current_colorbar, false); //have to load into newly create image

        postSetImsize();

    }
}

void rtimvBase::postSetImsize()
{
    return;
}

uint32_t rtimvBase::nx()
{
    return m_nx;
}

uint32_t rtimvBase::ny()
{
    return m_ny;
}

void rtimvBase::updateImages()
{
    static bool connected = false;

    int doupdate = RTIMVIMAGE_NOUPDATE;
    int supportUpdate = RTIMVIMAGE_NOUPDATE;

    if(m_images[0] != nullptr)
    {
        doupdate = m_images[0]->update();
    }
    
    for(size_t i = 1; i < m_images.size(); ++i)
    {
        if(m_images[i] != nullptr)
        {
            int sU = m_images[i]->update();
            if(sU > supportUpdate) //Do an update if any support image needs an update
            {
                supportUpdate = sU;  
            }
        }
    }

    if(doupdate >= RTIMVIMAGE_IMUPDATE || supportUpdate >= RTIMVIMAGE_IMUPDATE)
    {
        changeImdata(true);

        if(!connected)
        {
            onConnect();
            connected = true;
        }
    }

    if(!connected)
    {
        updateNC();
        return;
    }

    if(doupdate == RTIMVIMAGE_FPSUPDATE)
    {
        updateFPS();
    }

    if(doupdate == RTIMVIMAGE_AGEUPDATE)
    {
        updateAge();
    }
}

void rtimvBase::imageTimeout(int to)
{
    if(to == m_imageTimeout) //Don't interrupt if not needed
    {
        return;
    }

    m_imageTimer.stop();

    for(size_t i = 0; i < m_images.size(); ++i)
    {
        if(m_images[i] != nullptr)
        {
            m_images[i]->timeout(to); // just for fps calculations
        }
    }

    m_imageTimer.start(to);
}

int rtimvBase::imageTimeout()
{
    return m_imageTimeout;
}

rtimvBase::pixelF rtimvBase::rawPixel()
{
    pixelF _pixel = nullptr;

    if(m_images[0] == nullptr)
    {
        return _pixel; // no valid base image
    }

    if(m_images[0]->valid())
    {
        _pixel = &pixel_noCal; // default if there is a valid base image.
    }
    else
    {
        return _pixel; // no valid base image
    }

    if(m_subtractDark == true && m_applyMask == false)
    {
        if(m_images[1] == nullptr)
        {
            return _pixel;
        }

        if(m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny())
        {
            return _pixel;
        }

        if(m_images[0]->valid() && m_images[1]->valid())
        {
            _pixel = &pixel_subDark;
        }
    }

    if(m_subtractDark == false && m_applyMask == true)
    {
        if(m_images[2] == nullptr)
            return _pixel;

        if(m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny())
            return _pixel;

        if(m_images[0]->valid() && m_images[2]->valid())
            _pixel = &pixel_applyMask;
    }

    if(m_subtractDark == true && m_applyMask == true)
    {

        if(m_images[1] == nullptr && m_images[2] == nullptr)
            return _pixel;
        else if(m_images[2] == nullptr)
        {
            if(m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny())
                return _pixel;
            if(m_images[1]->valid())
                _pixel = &pixel_subDark;
        }
        else if(m_images[1] == nullptr)
        {
            if(m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny())
                return _pixel;
            if(m_images[2]->valid())
                _pixel = &pixel_applyMask;
        }
        else
        {
            if(m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny())
                return _pixel;
            if(m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny())
                return _pixel;
            if(m_images[1]->valid() && m_images[2]->valid())
                _pixel = &pixel_subDarkApplyMask;
        }
    }

    return _pixel;
}

float rtimvBase::pixel_noCal(rtimvBase *imv,
                             size_t idx)
{
    return imv->m_images[0]->pixel(idx);
}

float rtimvBase::pixel_subDark(rtimvBase *imv,
                               size_t idx)
{
    return imv->m_images[0]->pixel(idx) - imv->m_images[1]->pixel(idx);
}

float rtimvBase::pixel_applyMask(rtimvBase *imv,
                                 size_t idx)
{
    return imv->m_images[0]->pixel(idx) * imv->m_images[2]->pixel(idx);
}

float rtimvBase::pixel_subDarkApplyMask(rtimvBase *imv,
                                        size_t idx)
{
    return (imv->m_images[0]->pixel(idx) - imv->m_images[1]->pixel(idx)) * imv->m_images[2]->pixel(idx);
}

float rtimvBase::calPixel( uint32_t x,
                           uint32_t y
                         )
{
    return m_calData[y * m_nx + x];
}

uint8_t rtimvBase::satPixel( uint32_t x,
                             uint32_t y
                           )
{
    return m_satData[y * m_nx + x];
}

// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color/56678483#56678483
template <typename realT>
realT sRGBtoLinRGB(int rgb)
{
    realT V = ((realT)rgb) / 255.0;

    if(V <= 0.0405)
        return V / 12.92;

    return pow((V + 0.055) / 1.055, 2.4);
}

template <typename realT>
realT linRGBtoLuminance(realT linR,
                        realT linG,
                        realT linB)
{
    return 0.2126 * linR + 0.7152 * linG + 0.0722 * linB;
}

template <typename realT>
realT pLightness(realT lum)
{
    if(lum <= static_cast<realT>(216) / static_cast<realT>(24389))
    {
        return lum * static_cast<realT>(24389) / static_cast<realT>(27);
    }

    return pow(lum, static_cast<realT>(1) / static_cast<realT>(3)) * 116 - 16;
}

void rtimvBase::load_colorbar( int cb,
                               bool update 
                             )
{
    if(!m_qim)
    {
        return;
    }

    current_colorbar = cb;
    switch (cb)
    {
    case colorbarJet:
        m_minColor = 0;
        m_maxColor = load_colorbar_jet(m_qim);
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor("white");
        break;
    case colorbarHot:
        m_minColor = 0;
        m_maxColor = load_colorbar_hot(m_qim);
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor("cyan");
        break;
    case colorbarBone:
        m_minColor = 0;
        m_maxColor = load_colorbar_bone(m_qim);
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        warning_color = QColor("red");
        break;
    case colorbarRed:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for(int i = m_minColor; i <= m_maxColor; i++)
        {
            int c = (((float)i) / 253. * 255.) + 0.5;
            m_qim->setColor(i, qRgb(c, 0, 0));
        }
        m_qim->setColor(254, qRgb(0, 0, 0));
        m_qim->setColor(255, qRgb(0, 255, 0));
        warning_color = QColor("red");
        break;
    case colorbarGreen:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for(int i = m_minColor; i <= m_maxColor; i++)
        {
            int c = (((float)i) / 253. * 255.) + 0.5;
            m_qim->setColor(i, qRgb(0, c, 0));
        }
        m_qim->setColor(254, qRgb(0, 0, 0));
        m_qim->setColor(255, qRgb(255, 0, 0));
        warning_color = QColor("red"); 
        break;
    case colorbarBlue:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for(int i = m_minColor; i <= m_maxColor; i++)
        {
            int c = (((float)i) / 253. * 255.) + 0.5;
            m_qim->setColor(i, qRgb(0, 0, c));
        }
        m_qim->setColor(254, qRgb(0, 0, 0));
        m_qim->setColor(255, qRgb(255, 0, 0));
        warning_color = QColor("red"); 
        break;
    default:
        m_minColor = 0;
        m_maxColor = 253;
        m_maskColor = m_maxColor + 1;
        m_satColor = m_maxColor + 2;
        m_nanColor = m_maskColor;
        for(int i = m_minColor; i <= m_maxColor; i++)
        {
            int c = (((float)i) / 253. * 255.) + 0.5;
            m_qim->setColor(i, qRgb(c, c, c));
        }
        m_qim->setColor(254, qRgb(0, 0, 0));
        m_qim->setColor(255, qRgb(255, 0, 0));

        warning_color = QColor("red");
        break;
    }

    m_lightness.resize(256);

    for(int n = 0; n < 256; ++n)
    {
        m_lightness[n] = QColor(m_qim->color(n)).lightness();
    }

    if(update) 
    {
        changeImdata();
    }
}

void rtimvBase::set_cbStretch(int ct)
{
    if(ct < 0 || ct >= cbStretches_max)
    {
        ct = stretchLinear;
    }

    m_cbStretch = ct;
}

int rtimvBase::get_cbStretch()
{
    return m_cbStretch;
}

void rtimvBase::mindat(float md)
{
    m_mindat = md;
}

float rtimvBase::mindat()
{
    return m_mindat;
}

void rtimvBase::maxdat(float md)
{
    m_maxdat = md;
}

float rtimvBase::maxdat()
{
    return m_maxdat;
}

void rtimvBase::bias(float b)
{
    float cont = contrast();

    mindat(b - 0.5 * cont);
    maxdat(b + 0.5 * cont);
}

float rtimvBase::bias()
{
    return 0.5 * (m_maxdat + m_mindat);
}

void rtimvBase::bias_rel(float br)
{
    float cont = contrast();

    mindat(imdat_min + br * (imdat_max - imdat_min) - 0.5 * cont);
    maxdat(imdat_min + br * (imdat_max - imdat_min) + 0.5 * cont);
}

float rtimvBase::bias_rel()
{
    return 0.5 * (m_maxdat + m_mindat) / (m_maxdat - m_mindat);
}

void rtimvBase::contrast(float c)
{
    float b = bias();
    mindat(b - 0.5 * c);
    maxdat(b + 0.5 * c);
}

float rtimvBase::contrast()
{
    return m_maxdat - m_mindat;
}

float rtimvBase::contrast_rel()
{
    return (imdat_max - imdat_min) / (m_maxdat - m_mindat);
}

void rtimvBase::contrast_rel(float cr)
{
    float b = bias();
    mindat(b - .5 * (imdat_max - imdat_min) / cr);
    maxdat(b + .5 * (imdat_max - imdat_min) / cr);
}

int calcPixIndex_linear(float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = (pixval - mindat) / ((float)(maxdat - mindat));
    if(pixval < 0)
        return 0;

    // Clamp it to <= 1
    if(pixval > 1.)
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * (maxcol - mincol) + 0.5;
}

int calcPixIndex_log(float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
    static float a = 1000;
    static float log10_a = log10(a);

    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = (pixval - mindat) / ((float)(maxdat - mindat));
    if(pixval < 0)
        return 0;

    pixval = log10(pixval * a + 1) / log10_a;

    // Clamp it to <= 1
    if(pixval > 1.)
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * (maxcol - mincol) + 0.5;
}

int calcPixIndex_pow(float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
    static float a = 1000;

    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = (pixval - mindat) / ((float)(maxdat - mindat));
    if(pixval < 0)
        return 0;

    pixval = (pow(a, pixval)) / a;

    // Clamp it to <= 1
    if(pixval > 1.)
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * (maxcol - mincol) + 0.5;
}

int calcPixIndex_sqrt(float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = (pixval - mindat) / ((float)(maxdat - mindat));
    if(pixval < 0)
        return 0;

    pixval = sqrt(pixval);

    // Clamp it to <= 1
    if(pixval > 1.)
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * (maxcol - mincol) + 0.5;
}

int calcPixIndex_square(float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
    // We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
    pixval = (pixval - mindat) / ((float)(maxdat - mindat));
    if(pixval < 0)
        return 0;

    pixval = pixval * pixval;

    // Clamp it to <= 1
    if(pixval > 1.)
        pixval = 1.;

    // And finally put it in the color bar index range
    return pixval * (maxcol - mincol) + 0.5;
}

void rtimvBase::changeImdata(bool newdata)
{
    if(m_amChangingimdata && !newdata) //this means we're already in this function!
    {
        return;
    }

    if(!imageValid(0))
    {
        return;
    }

    m_amChangingimdata = true;

    bool resized = false;

    //Here we realize we need to resize
    if(m_images[0]->nx() != m_nx || m_images[0]->ny() != m_ny || !m_qim)
    {
        std::unique_lock<std::mutex> lock(m_calMutex);
        setImsize(m_images[0]->nx(), m_images[0]->ny());

        resized = true;
    }

    //If it's new data we copy it to m_calData
    if(resized || newdata)
    {
        std::unique_lock<std::mutex> lock(m_rawMutex);
        
        // Get the pixel calculating function
        float (*_pixel)(rtimvBase *, size_t) = rawPixel();

        if(_pixel == nullptr)
        {
            m_amChangingimdata = false;

            return;
        }

        if(m_nx != m_images[0]->nx() || m_ny != m_images[0]->ny())
        {
            m_amChangingimdata = false;
            return;
        }

        for(uint64_t n=0; n < m_nx*m_ny; ++n)
        {
            //Check for saturation
            if(pixel_noCal(this,n) >= m_satLevel)
            {
                m_satData[n] = 1;
                ++m_saturated;
            }
            else
            {
                m_satData[n] = 0;
            }

            //Fill in calibrated value
            m_calData[n] = _pixel(this, n);
        }

        //Now check the sat image itself
        if(imageValid(3))
        {
            if(m_nx == m_images[3]->nx() && m_ny == m_images[3]->ny())
            {
                for(uint64_t n=0; n < m_nx*m_ny; ++n)
                {
                    if(m_images[3]->pixel(n) > 0)
                    {
                        m_satData[n] = 1;
                    }
                    //don't set 0 b/c it would override m_satLevel 
                }
            }
        }
    }

    if(resized || newdata || m_autoScale)
    {
        imdat_min = std::numeric_limits<float>::max();
        imdat_max = -std::numeric_limits<float>::max();

        if(colorBoxActive)
        {
            colorBox_min = std::numeric_limits<float>::max();
            colorBox_max = -std::numeric_limits<float>::max();
        }

        float imval;

        for(uint32_t j = 0; j < m_ny; ++j)
        {
            for(uint32_t i = 0; i < m_nx; ++i)
            {
                imval = calPixel(i,j);  

                if(!std::isfinite(imval))
                {
                    continue;
                }

                if(imval > imdat_max)
                {
                    imdat_max = imval;
                }

                if(imval < imdat_min)
                {
                    imdat_min = imval;
                }

                if(colorBoxActive)
                {
                    if(i >= colorBox_i0 && i < colorBox_i1 && j >= colorBox_j0 && j < colorBox_j1)
                    {
                        if(imval < colorBox_min)
                        {
                            colorBox_min = imval;
                        }
                        if(imval > colorBox_max)
                        {
                            colorBox_max = imval;
                        }
                    }
                }
            }
        }

        if(!std::isfinite(imdat_max) || !std::isfinite(imdat_min))
        {
            // It should be impossible for them to be infinite by themselves unless it's all NaNs.
            imdat_max = 0;
            imdat_min = 0;
        }

        if(colorBoxActive)
        {
            if(!std::isfinite(colorBox_max) || !std::isfinite(colorBox_min))
            {
                // It should be impossible for them to be infinite by themselves unless it's all NaNs in the box.
                colorBox_max = 0;
                colorBox_min = 0;
            }

            mindat(colorBox_min);
            maxdat(colorBox_max);
        }
        else
        {
            mindat(imdat_min);
            maxdat(imdat_max);
        }
    }

    if(!m_qim) 
    {
        m_amChangingimdata = false;
        return;
    }

    

    /* Here is where we color the pixmap*/

    // Get the color index calculating function
    int (*_index)(float, float, float, int, int);
    switch (m_cbStretch)
    {
        case stretchLog:
            _index = calcPixIndex_log;
            break;
        case stretchPow:
            _index = calcPixIndex_pow;
            break;
        case stretchSqrt:
            _index = calcPixIndex_sqrt;
            break;
        case stretchSquare:
            _index = calcPixIndex_square;
            break;
        default:
            _index = calcPixIndex_linear;
    }

    if(m_mindat == m_maxdat)
    {
        float imval;
        for(uint32_t i = 0; i < m_ny; ++i)
        {
            for(uint32_t j = 0; j < m_nx; ++j)
            {
                imval = calPixel(j,i);
                if(!std::isfinite(imval))
                {
                    m_qim->setPixel(j, m_ny - i - 1, m_nanColor);
                    continue;
                }

                m_qim->setPixel(j, m_ny - i - 1, 0);
            }
        }
    }
    else
    {
        float imval;
        for(uint32_t i = 0; i < m_ny; ++i)
        {
            for(uint32_t j = 0; j < m_nx; ++j)
            {
                imval = calPixel(j,i);

                if(!std::isfinite(imval))
                {
                    m_qim->setPixel(j, m_ny - i - 1, m_nanColor);
                    continue;
                }
                                
                int idxVal =  _index(imval, m_mindat, m_maxdat, m_minColor, m_maxColor);
                m_qim->setPixel(j, m_ny - i - 1, idxVal);
            }
        }
    }
    
    if(m_applySatMask)
    {
        for(uint32_t j = 0; j < m_ny; ++j)
        {
            for(uint32_t i = 0; i < m_nx; ++i)
            {
                if(satPixel(i,j) == 1)
                {
                    m_qim->setPixel(i, m_ny - j - 1, m_satColor);
                }
            }
        }
    }

    m_qpm.convertFromImage(*m_qim, Qt::AutoColor | Qt::ThresholdDither);

    if(resized)
    {
        //Always switch to zoom 1 after a resize occurs
        zoomLevel(1);
    }

    postChangeImdata();

    m_amChangingimdata = false;

} //void rtimvBase::changeImdata(bool newdata)

void rtimvBase::postChangeImdata()
{
    return;
}

void rtimvBase::zoomLevel(float zl)
{
    if(zl < m_zoomLevelMin)
    {
        zl = m_zoomLevelMin;
    }

    if(zl > m_zoomLevelMax)
    {
        zl = m_zoomLevelMax;
    }

    m_zoomLevel = zl;

    post_zoomLevel();
}

void rtimvBase::post_zoomLevel()
{
    return;
}

void rtimvBase::setUserBoxActive(bool usba)
{
    if(usba)
    {
        int idx;
        float imval;

        if(colorBox_i0 > colorBox_i1)
        {
            idx = colorBox_i0;
            colorBox_i0 = colorBox_i1;
            colorBox_i1 = idx;
        }

        if(colorBox_i0 < 0)
        {
            colorBox_i0 = 0;
        }
        else if(colorBox_i0 >= (int64_t) m_nx)
        {
            colorBox_i0 = (int64_t)m_nx - (colorBox_i1 - colorBox_i0);
        }

        if(colorBox_i1 <= 0)
        {
            colorBox_i1 = 0 + (colorBox_i1 - colorBox_i0);
        }
        
        if(colorBox_i1 > (int64_t )m_nx)
        {
            colorBox_i1 = (int64_t)m_nx - 1;
        }

        if(colorBox_j0 > colorBox_j1)
        {
            idx = colorBox_j0;
            colorBox_j0 = colorBox_j1;
            colorBox_j1 = idx;
        }

        if(colorBox_j0 < 0)
        {
            colorBox_j0 = 0;
        }
        else if(colorBox_j0 >= (int64_t) m_nx)
        {
            colorBox_j0 = (int64_t) m_ny - (colorBox_j1 - colorBox_j0);
        }

        if(colorBox_j1 <= 0)
        {
            colorBox_j1 = 0 + (colorBox_j1 - colorBox_j0);
        }

        if(colorBox_j1 > (int64_t) m_ny)
        {
            colorBox_j1 = (int64_t) m_ny - 1;
        }

        colorBox_min = std::numeric_limits<float>::max();
        colorBox_max = -std::numeric_limits<float>::max();
        
        for(int i = colorBox_i0; i < colorBox_i1; i++)
        {
            for(int j = colorBox_j0; j < colorBox_j1; j++)
            {
                imval = calPixel(i,j);

                if(!std::isfinite(imval))
                {
                    continue;
                }

                if(imval < colorBox_min)
                    colorBox_min = imval;
                if(imval > colorBox_max)
                    colorBox_max = imval;
            }
        }

        if(colorBox_min == std::numeric_limits<float>::max() && colorBox_max == -std::numeric_limits<float>::max()) // If all nans
        {
            colorBox_min = 0;
            colorBox_max = 0;
        }

        mindat(colorBox_min);
        maxdat(colorBox_max);

        colorBoxActive = usba;
        
        set_colorbar_mode(minmaxbox);
        
        changeImdata(false);
        
        return;
    }

    colorBoxActive = usba;
}

void rtimvBase::set_RealTimeEnabled(int rte)
{
    RealTimeEnabled = (rte != 0);
}

void rtimvBase::set_RealTimeStopped(int rts)
{
    RealTimeStopped = (rts != 0);

    if(RealTimeStopped)
    {
        m_imageTimer.stop();
    }
    else
    {
        m_imageTimer.start(m_imageTimeout);
    }
}

void rtimvBase::updateFPS()
{
    return;
}

void rtimvBase::updateAge()
{
    return;
}

void rtimvBase::updateNC()
{
    return;
}

