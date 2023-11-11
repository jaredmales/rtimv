
#include "rtimvBase.hpp"

#ifdef RTIMV_MILK
#include "images/shmimImage.hpp"
#endif
#include "images/fitsImage.hpp"
#include "images/fitsDirectory.hpp"
#include "images/mzmqImage.hpp"

#if 0
rtimvBase *globalIMV;

int rtimvBase::sigsegvFd[2];

#endif


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

    for (size_t i = 0; i < m_images.size(); ++i)
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
        return false;
    if(m_images[n] == nullptr)
        return false;
    return m_images[n]->valid();
}

void rtimvBase::setImsize(uint32_t x, uint32_t y)
{
    DEBUG_TRACE_ANCHOR(rtimvBase::setImsize start)

    //Always have at least one pixel
    if(x == 0) x = 1;
    if(y == 0) y = 1;

    if(m_nx != x || m_ny != y || m_calData == 0 || m_qim == 0)
    {
        m_nx = x;
        m_ny = y;

        DEBUG_TRACE_VAL(m_nx)
        DEBUG_TRACE_VAL(m_ny)

        if(m_calData != nullptr)
        {
            DEBUG_TRACE_CRUMB

            delete[] m_calData;
            m_calData = nullptr;
        }  

        DEBUG_TRACE_CRUMB
    
        m_calData = new float[m_nx*m_ny];

        if(m_qim != nullptr)
        {
            DEBUG_TRACE_CRUMB

            delete m_qim;
            m_qim = nullptr;
        }

        DEBUG_TRACE_CRUMB

        m_qim = new QImage(m_nx, m_ny, QImage::Format_Indexed8);

        DEBUG_TRACE_CRUMB

        load_colorbar(current_colorbar, false); //have to load into newly create image

        DEBUG_TRACE_CRUMB

        postSetImsize();

        DEBUG_TRACE_CRUMB
    }

    DEBUG_TRACE_ANCHOR(rtimvBase::setImsize end)
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
    DEBUG_TRACE_ANCHOR(rtimvBase::updateImages begin)

    static bool connected = false;

    int doupdate = RTIMVIMAGE_NOUPDATE;
    int supportUpdate = RTIMVIMAGE_NOUPDATE;

    if(m_images[0] != nullptr)
    {
        DEBUG_TRACE_CRUMB
        doupdate = m_images[0]->update();
    }
    
    DEBUG_TRACE_CRUMB

    for (size_t i = 1; i < m_images.size(); ++i)
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

    DEBUG_TRACE_CRUMB

    if(doupdate >= RTIMVIMAGE_IMUPDATE || supportUpdate >= RTIMVIMAGE_IMUPDATE)
    {
        DEBUG_TRACE_CRUMB

        changeImdata(true);

        DEBUG_TRACE_CRUMB

        if(!connected)
        {
            DEBUG_TRACE_CRUMB
            onConnect();
            connected = true;
            DEBUG_TRACE_CRUMB
        }
    }

    DEBUG_TRACE_CRUMB
    if(!connected)
    {
        DEBUG_TRACE_CRUMB
        updateNC();
        DEBUG_TRACE_ANCHOR(rtimvBase::updateImages early)
        return;
    }

    if(doupdate == RTIMVIMAGE_FPSUPDATE)
    {
        DEBUG_TRACE_CRUMB
        updateFPS();
    }

    DEBUG_TRACE_CRUMB

    if(doupdate == RTIMVIMAGE_AGEUPDATE)
    {
        updateAge();
    }

    DEBUG_TRACE_ANCHOR(rtimvBase::updateImages end)
}

void rtimvBase::imageTimeout(int to)
{
    if(to == m_imageTimeout) //Don't interrupt if not needed
    {
        return;
    }

    m_imageTimer.stop();

    for (size_t i = 0; i < m_images.size(); ++i)
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
    DEBUG_TRACE_ANCHOR(rtimvBase::load_colorbar start)

    if(!m_qim)
    {
        DEBUG_TRACE_ANCHOR(rtimvBase::load_colorbar early-null)
    }

    if(m_qim)
    {
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
            warning_color = QColor("lime");
            break;
        default:
            m_minColor = 0;
            m_maxColor = 253;
            m_maskColor = m_maxColor + 1;
            m_satColor = m_maxColor + 2;
            m_nanColor = m_maskColor;
            for (int i = m_minColor; i <= m_maxColor; i++)
            {
                int c = (((float)i) / 253. * 255.) + 0.5;
                m_qim->setColor(i, qRgb(c, c, c));
            }
            m_qim->setColor(254, qRgb(0, 0, 0));
            m_qim->setColor(255, qRgb(255, 0, 0));

            warning_color = QColor("lime");
            break;
        }

        m_lightness.resize(256);

        for (int n = 0; n < 256; ++n)
        {
            m_lightness[n] = QColor(m_qim->color(n)).lightness();
        }

        if(update) 
        {
            changeImdata();
        }
    }

    DEBUG_TRACE_ANCHOR(rtimvBase::load_colorbar end)
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
    DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata start)

    float tmp_min;
    float tmp_max;

    int idx;
    float imval;

    if(m_images[0] == nullptr)
    {
        DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata early-null)
        return;
    }

    DEBUG_TRACE_CRUMB

    if(!m_images[0]->valid())
    {
        DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata early-not-valid)
        return;
    }

    DEBUG_TRACE_CRUMB

    bool resized = false;

    //Here we realize we need to resize
    if(m_images[0]->nx() != m_nx || m_images[0]->ny() != m_ny || !m_qim)
    {
        amChangingimdata = true;

        DEBUG_TRACE_CRUMB

        //Need to lock a mutex here
        setImsize(m_images[0]->nx(), m_images[0]->ny());

        DEBUG_TRACE_CRUMB

        resized = true;
    }

    DEBUG_TRACE_CRUMB

    if(resized || newdata) //need to copy new data to m_calData
    {
        std::unique_lock<std::mutex> lock(m_rawMutex); //, std::try_to_lock);
        if(!lock.owns_lock())
        {        
            DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata early-no-lock) 
            return;
        }

        DEBUG_TRACE_CRUMB

        // Get the pixel calculating function
        float (*_pixel)(rtimvBase *, size_t) = rawPixel();

        if(_pixel == nullptr)
        {
            DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata early-null_pixel)
            return;
        }

        DEBUG_TRACE_CRUMB

        if(m_nx != m_images[0]->nx() || m_ny != m_images[0]->ny())
        {
            DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata early-size-mismatch)
            return;
        }

        DEBUG_TRACE_CRUMB

        for(uint64_t n=0; n < m_nx*m_ny; ++n)
        {
            m_calData[n] = _pixel(this, n);
        }
    }

    DEBUG_TRACE_CRUMB

    if(resized || newdata || m_autoScale)
    {
        DEBUG_TRACE_CRUMB

        // Need to set these at the beginning
        imdat_min = std::numeric_limits<float>::max();
        imdat_max = -std::numeric_limits<float>::max();

        for (uint32_t i = 0; i < m_ny; ++i)
        {
            for (uint32_t j = 0; j < m_nx; ++j)
            {
                imval = calPixel(j,i);  

                if(!std::isfinite(imval))
                {
                    continue;
                }

                if(imval > imdat_max)
                {
                    imdat_max = calPixel(j,i);//_pixel(this, i * m_nx + j);
                }

                if(imval < imdat_min)
                {
                    imdat_min = calPixel(j,i);//_pixel(this, i * m_nx + j);
                }
            }
        }

        if(!std::isfinite(imdat_max) || !std::isfinite(imdat_min))
        {
            // It should be impossible for them to be infinite by themselves unless it's all NaNs.
            imdat_max = 0;
            imdat_min = 0;
        }

        mindat(imdat_min);
        maxdat(imdat_max);
    }

    DEBUG_TRACE_CRUMB

    if(!m_qim) 
    {
        amChangingimdata = false;
        return;
    }

    DEBUG_TRACE_CRUMB

    amChangingimdata = true;

    /* Here is where we color the pixmap*/

    DEBUG_TRACE_CRUMB

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

    DEBUG_TRACE_CRUMB

    if(!newdata && !resized) //This is just a recolor
    {
        DEBUG_TRACE_CRUMB
        if(m_mindat == m_maxdat) //Constant
        {
            for (uint32_t i = 0; i < m_ny; ++i)
            {
                for (uint32_t j = 0; j < m_nx; ++j)
                {
                    m_qim->setPixel(j, m_ny - i - 1, 0);
                }
            }
        }
        else
        {
            for (uint32_t i = 0; i < m_ny; ++i)
            {
                for (uint32_t j = 0; j < m_nx; ++j)
                {
                    //idx = i * m_nx + j;
                    imval = calPixel(j,i);//  _pixel(this, idx);

                    if(!std::isfinite(imval))
                    {
                        m_qim->setPixel(j, m_ny - i - 1, m_nanColor);
                        continue;
                    }
                    m_qim->setPixel(j, m_ny - i - 1, _index(imval, m_mindat, m_maxdat, m_minColor, m_maxColor));
                }
            }
        }
    }
    else
    {
        DEBUG_TRACE_CRUMB

        // Update statistics
        tmp_min = std::numeric_limits<float>::max();
        tmp_max = -std::numeric_limits<float>::max();
        saturated = 0;

        if(colorBoxActive)
        {
            colorBox_min = std::numeric_limits<float>::max();
            colorBox_max = -std::numeric_limits<float>::max();
        }

        DEBUG_TRACE_CRUMB

        if(m_mindat == m_maxdat)
        {
            DEBUG_TRACE_CRUMB

            for (uint32_t i = 0; i < m_ny; ++i)
            {
                for (uint32_t j = 0; j < m_nx; ++j)
                {
                    //idx = i * m_nx + j;
                    imval = calPixel(j,i); //_pixel(this, idx); // m_imData[idx];

                    if(!std::isfinite(imval))
                    {
                        m_qim->setPixel(j, m_ny - i - 1, m_nanColor);
                        continue;
                    }

                    if(imval > tmp_max)
                        tmp_max = imval;
                    if(imval < tmp_min)
                        tmp_min = imval;

                    if(imval >= sat_level)
                        saturated++;

                    if(colorBoxActive)
                    {
                        if(i >= colorBox_i0 && i < colorBox_i1 && j >= colorBox_j0 && j < colorBox_j1)
                        {
                            if(imval < colorBox_min)
                                colorBox_min = imval;
                            if(imval > colorBox_max)
                                colorBox_max = imval;
                        }
                    }
                    m_qim->setPixel(j, m_ny - i - 1, 0);
                }
            }
        }
        else
        {
            DEBUG_TRACE_CRUMB

            for (uint32_t i = 0; i < m_ny; ++i)
            {
                for (uint32_t j = 0; j < m_nx; ++j)
                {
                    imval = calPixel(j,i);

                    if(!std::isfinite(imval))
                    {
                        m_qim->setPixel(j, m_ny - i - 1, m_nanColor);
                        continue;
                    }

                    if(imval > tmp_max)
                        tmp_max = imval;
                    if(imval < tmp_min)
                        tmp_min = imval;

                    if(imval >= sat_level)
                        saturated++;

                    if(colorBoxActive)
                    {
                        if(i >= colorBox_i0 && i < colorBox_i1 && j >= colorBox_j0 && j < colorBox_j1)
                        {
                            if(imval < colorBox_min)
                                colorBox_min = imval;
                            if(imval > colorBox_max)
                                colorBox_max = imval;
                        }
                    }

                    int idxVal =  _index(imval, m_mindat, m_maxdat, m_minColor, m_maxColor);
                    m_qim->setPixel(j, m_ny - i - 1, idxVal);
                }
            }
        }

        imdat_max = tmp_max;
        imdat_min = tmp_min;
    }

    if(m_applyMask && m_images[2] != nullptr)
    {
        if(m_images[2]->nx() == m_images[0]->nx() || m_images[2]->ny() == m_images[0]->ny())
        {
            for (uint32_t i = 0; i < m_ny; ++i)
            {
                for (uint32_t j = 0; j < m_nx; ++j)
                {
                    idx = i * m_nx + j;
                    if(m_images[2]->pixel(idx) == 0)
                        m_qim->setPixel(j, m_ny - i - 1, m_maskColor);
                }
            }
        }
    }

    if(m_applySatMask && m_images[3] != nullptr)
    {
        if(m_images[3]->nx() == m_images[0]->nx() || m_images[3]->ny() == m_images[0]->ny())
        {
            for (uint32_t i = 0; i < m_ny; ++i)
            {
                for (uint32_t j = 0; j < m_nx; ++j)
                {
                    idx = i * m_nx + j;
                    if(m_images[3]->pixel(idx) == 1)
                        m_qim->setPixel(j, m_ny - i - 1, m_satColor);
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
    amChangingimdata = false;

    DEBUG_TRACE_ANCHOR(rtimvBase::changeImdata end)

    

} //void rtimvBase::changeImdata(bool newdata)

void rtimvBase::postChangeImdata()
{
    return;
}

void rtimvBase::zoomLevel(float zl)
{
    DEBUG_TRACE_ANCHOR(rtimvBase::zoomLevel begin)

    DEBUG_TRACE_VAL(zl)

    if(zl < m_zoomLevelMin)
    {
        zl = m_zoomLevelMin;
    }

    if(zl > m_zoomLevelMax)
    {
        zl = m_zoomLevelMax;
    }

    m_zoomLevel = zl;

    DEBUG_TRACE_VAL(m_zoomLevel)

    post_zoomLevel();

    DEBUG_TRACE_ANCHOR(rtimvBase::zoomLevel end)
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
            colorBox_i0 = 0;
        if(colorBox_i0 >= (int64_t)m_nx)
            colorBox_i0 = (int64_t)m_nx - (colorBox_i1 - colorBox_i0);

        if(colorBox_i1 <= 0)
            colorBox_i1 = 0 + (colorBox_i1 - colorBox_i0);
        if(colorBox_i1 > (int64_t)m_nx)
            colorBox_i1 = (int64_t)m_nx - 1;

        if(colorBox_j0 > colorBox_j1)
        {
            idx = colorBox_j0;
            colorBox_j0 = colorBox_j1;
            colorBox_j1 = idx;
        }

        if(colorBox_j0 < 0)
            colorBox_j0 = 0;
        if(colorBox_j0 >= (int64_t)m_nx)
            colorBox_j0 = (int64_t)m_ny - (colorBox_j1 - colorBox_j0);

        if(colorBox_j1 <= 0)
            colorBox_j1 = 0 + (colorBox_j1 - colorBox_j0);
        if(colorBox_j1 > (int64_t)m_ny)
            colorBox_j1 = (int64_t)m_ny - 1;

        /*{//mutex scope
        std::unique_lock<std::mutex> lock(m_rawMutex, std::try_to_lock);
        if(!lock.owns_lock())
        {        
            return;
        }*/
 
        //pixelF _pixel = pixel();

        colorBox_min = std::numeric_limits<float>::max();
        colorBox_max = -std::numeric_limits<float>::max();
        for (int i = colorBox_i0; i < colorBox_i1; i++)
        {
            for (int j = colorBox_j0; j < colorBox_j1; j++)
            {
                //idx = j * m_nx + i;
                imval = calPixel(j,i); //_pixel(this, idx); // m_imData[idx];

                if(!std::isfinite(imval))
                    continue;

                if(imval < colorBox_min)
                    colorBox_min = imval;
                if(imval > colorBox_max)
                    colorBox_max = imval;
            }
        }
        //} //release mutex here.

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

