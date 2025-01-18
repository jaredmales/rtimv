#include "fitsImage.hpp"

#include <iostream>
#include <mx/ioutils/fileUtils.hpp>
#include <unistd.h>

fitsImage::fitsImage( std::mutex *mut ) : rtimvImage( mut )
{
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( imageTimerout() ) );
}

int fitsImage::imageKey( const std::string &sn )
{
    m_imagePath = sn;

    if( m_imagePath.find( ".fits" ) == std::string::npos )
    {
        std::cerr << m_imagePath << " does not end in '.fits'.\n";
        return -1;
    }

    imageTimerout();

    return 0;
}

std::string fitsImage::imageKey()
{
    return m_imagePath;
}

std::string fitsImage::imageName()
{
    return mx::ioutils::pathStem( m_imagePath );
}

void fitsImage::imageTimeout( int to )
{
    m_imageTimeout = to;
}

int fitsImage::imageTimeout()
{
    return m_imageTimeout;
}

void fitsImage::timeout( int to )
{
    m_timeout = to;
}

uint32_t fitsImage::nx()
{
    return m_nx;
}

uint32_t fitsImage::ny()
{
    return m_ny;
}

uint32_t fitsImage::nz()
{
    return m_nz;
}

uint32_t fitsImage::imageNo()
{
    return m_imageNo;
}

void fitsImage::imageNo(uint32_t ino)
{
    if( ino > nz() - 1 )
    {
        m_nextImageNo = nz()-1;
    }
    else
    {
        m_nextImageNo = ino;
    }
}

void fitsImage::incImageNo()
{
    if( m_imageNo >= m_nz - 1 )
    {
        m_nextImageNo = 0;
    }
    else
    {
        m_nextImageNo = m_imageNo + 1;
    }
}

void fitsImage::decImageNo()
{
    if( m_imageNo == 0 )
    {
        m_nextImageNo = m_nz-1;
    }
    else
    {
        m_nextImageNo = m_imageNo - 1;
    }
}

void fitsImage::deltaImageNo(int32_t dino)
{
    if(abs(dino) > nz())
    {
        dino %= nz();
    }

    uint32_t absdino = abs(dino);

    if(dino < 0 && absdino > m_imageNo)
    {
        m_nextImageNo = nz() + (m_imageNo+dino);
    }
    else if(dino > 0 && absdino > (nz()-1 - m_imageNo))
    {
        m_nextImageNo = dino - (nz() - m_imageNo);
    }
    else
    {
        m_nextImageNo += dino;
    }

}

double fitsImage::imageTime()
{
    return m_lastImageTime;
}

void fitsImage::imageTimerout()
{
    m_timer.stop();
    imConnect();

    if( !m_imageFound )
    {
        m_timer.start( m_imageTimeout );
    }
}

int fitsImage::readImage()
{
    /// The cfitsio data structure
    fitsfile *fptr{ nullptr };

    int fstatus = 0;

    fits_open_file( &fptr, m_imagePath.c_str(), READONLY, &fstatus );

    if( fstatus )
    {
        // we try again 100 ms later in case this was a rewrite of the existing file
        mx::sys::milliSleep( 100 );
        fstatus = 0;
        fits_open_file( &fptr, m_imagePath.c_str(), READONLY, &fstatus );

        if( fstatus )
        {
            if( m_reported == m_reportThresh )
                std::cerr << "rtimv: " << m_imagePath << " not found.\n";
            ++m_reported;
            return -1;
        }
    }

    /// The dimensions of the image (1D, 2D, 3D etc)
    int naxis;

    fits_get_img_dim( fptr, &naxis, &fstatus );
    if( fstatus )
    {
        if( m_reported == m_reportThresh )
            std::cerr << "rtimv: error getting number of axes in file " << m_imagePath << "\n";
        ++m_reported;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        return -1;
    }

    long *naxes = new long[naxis];

    fits_get_img_size( fptr, naxis, naxes, &fstatus );
    if( fstatus )
    {
        if( m_reported == m_reportThresh )
            std::cerr << "rtimv: error getting dimensions in file " << m_imagePath << "\n";
        ++m_reported;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        delete[] naxes;
        return -1;
    }

    long nz;
    if( naxis > 2 )
    {
        nz = naxes[2];

        if( nz < 1 )
        {
            nz = 1;
        }
    }
    else
    {
        nz = 1;
    }

    // resize the array if needed, which could be a reformat
    if( m_data == nullptr || m_nx * m_ny * m_nz != naxes[0] * naxes[1] * nz )
    {
        // If here and m_data is not null, then it is already allocated with new.
        if( m_data )
            delete[] m_data;
        m_data = new char[naxes[0] * naxes[1] * nz * sizeof( float )];
        m_currData = m_data;
    }

    // always set in case of a reformat
    m_nx = naxes[0];
    m_ny = naxes[1];
    m_nz = nz;

    long fpix[3];
    long lpix[3];
    long inc[3];

    fpix[0] = 1;
    fpix[1] = 1;
    fpix[2] = 1;

    lpix[0] = naxes[0];
    lpix[1] = naxes[1];
    lpix[2] = nz;

    inc[0] = 1;
    inc[1] = 1;
    inc[2] = 1;

    delete[] naxes;

    int anynul;

    fits_read_subset( fptr, TFLOAT, fpix, lpix, inc, 0, (void *)m_data, &anynul, &fstatus );

    if( fstatus )
    {
        if( m_reported == m_reportThresh )
            std::cerr << "rtimv: error reading data from " << m_imagePath << "\n";
        ++m_reported;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        return -1;
    }

    this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();

    fits_close_file( fptr, &fstatus );

    if( fstatus )
    {
        if( m_reported == m_reportThresh )
            std::cerr << "rtimv: error closing file " << m_imagePath << "\n";
        ++m_reported;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        return -1;
    }

    if( m_reported >= m_reportThresh )
    {
        std::cerr << "rtimv: " << m_imagePath << " found and read.\n";
    }

    m_reported = 0;

    return 0;
}

void fitsImage::imConnect()
{
    m_imageFound = 0;
    m_imageUpdated = false;

    if( readImage() < 0 )
    {
        return;
    }

    m_imageFound = 1;

    m_lastWriteTime = std::filesystem::last_write_time( m_imagePath );

    emit connected();
}

int fitsImage::update()
{
    if( !m_imageFound )
        return RTIMVIMAGE_NOUPDATE;

    // First time through after connect
    if( !m_imageUpdated )
    {
        m_imageUpdated = true;

        return RTIMVIMAGE_IMUPDATE;
    }

    if( m_lastWriteTime != std::filesystem::last_write_time( m_imagePath ) )
    {
        if( readImage() < 0 )
        {
            detach();
            return RTIMVIMAGE_NOUPDATE;
        };

        m_lastWriteTime = std::filesystem::last_write_time( m_imagePath );
        return RTIMVIMAGE_IMUPDATE;
    }

    if( m_nextImageNo != m_imageNo )
    {
        m_imageNo = m_nextImageNo;
        m_currData = m_data + ( m_imageNo * m_nx * m_ny * sizeof( float ) );
        return RTIMVIMAGE_IMUPDATE;
    }

    return RTIMVIMAGE_NOUPDATE;
}

void fitsImage::detach()
{
    if( m_data )
    {
        delete[] m_data;
        m_data = nullptr;
        m_currData = nullptr;
    }

    m_imageFound = 0;

    // Start checking for the file
    m_timer.start( m_imageTimeout );

    return;
}

bool fitsImage::valid()
{
    if( m_imageFound && m_data )
        return true;

    return false;
}

void fitsImage::update_fps()
{
}

float fitsImage::pixel( size_t n )
{
    return pixget( m_currData, n );
}

std::vector<std::string> fitsImage::info()
{
    std::vector<std::string> info = rtimvImage::info();
    info.push_back( std::string("path: ") + imageKey() );

    return info;
}
