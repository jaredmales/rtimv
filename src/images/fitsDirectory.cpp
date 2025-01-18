#include "fitsDirectory.hpp"

#include <iostream>
#include <mx/ioutils/fileUtils.hpp>
#include <unistd.h>

fitsDirectory::fitsDirectory( std::mutex *mut ) : rtimvImage( mut )
{
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( imageTimerout() ) );
}

int fitsDirectory::imageKey( const std::string &sn )
{
    m_dirPath = sn;

    imageTimerout();

    return 0;
}

std::string fitsDirectory::imageKey()
{
    return m_dirPath;
}

std::string fitsDirectory::imageName()
{
    /*if( m_fileList.size() == 0 )
    {
        return "";
    }

    return mx::ioutils::pathStem( m_fileList[m_imageNo] );*/

    return m_dirPath + "*.fits";
}

void fitsDirectory::imageTimeout( int to )
{
    m_imageTimeout = to;
}

int fitsDirectory::imageTimeout()
{
    return m_imageTimeout;
}

void fitsDirectory::timeout( int to )
{
    m_timeout = to;
}

uint32_t fitsDirectory::nx()
{
    return m_nx;
}

uint32_t fitsDirectory::ny()
{
    return m_ny;
}

uint32_t fitsDirectory::nz()
{
    return m_fileList.size();
}

uint32_t fitsDirectory::imageNo()
{
    return m_imageNo;
}

void fitsDirectory::imageNo(uint32_t ino)
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

void fitsDirectory::incImageNo()
{
    if( m_imageNo >= nz() - 1 )
    {
        m_nextImageNo = 0;
    }
    else
    {
        m_nextImageNo = m_imageNo + 1;
    }
}

void fitsDirectory::decImageNo()
{
    if( m_imageNo == 0 )
    {
        m_nextImageNo = nz() - 1;
    }
    else
    {
        m_nextImageNo = m_imageNo - 1;
    }
}

void fitsDirectory::deltaImageNo(int32_t dino)
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


double fitsDirectory::imageTime()
{
    return m_lastImageTime;
}

void fitsDirectory::imageTimerout()
{
    m_timer.stop();
    imConnect();

    if( !m_imageFound )
    {
        m_timer.start( m_imageTimeout );
    }
}

int fitsDirectory::readImage( size_t imno )
{
    if( imno >= m_fileList.size() )
        return -1;

    /// The cfitsio data structure
    fitsfile *fptr{ nullptr };

    int fstatus = 0;

    fits_open_file( &fptr, m_fileList[imno].c_str(), READONLY, &fstatus );

    if( fstatus )
    {
        if( !m_reported )
            std::cerr << "rtimv: " << m_fileList[imno] << " not found.\n";
        m_reported = true;
        return -1;
    }
    m_reported = false;

    /// The dimensions of the image (1D, 2D, 3D etc)
    int naxis;

    fits_get_img_dim( fptr, &naxis, &fstatus );
    if( fstatus )
    {
        if( !m_reported )
            std::cerr << "rtimv: error getting number of axes in file " << m_fileList[imno] << "\n";
        m_reported = true;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        return -1;
    }
    m_reported = false;

    long *naxes = new long[naxis];

    fits_get_img_size( fptr, naxis, naxes, &fstatus );
    if( fstatus )
    {
        if( !m_reported )
            std::cerr << "rtimv: error getting dimensions in file " << m_fileList[imno] << "\n";
        m_reported = true;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        delete[] naxes;
        return -1;
    }
    m_reported = false;

    // resize the array if needed, which could be a reformat
    if( m_data == nullptr || m_nx * m_ny != naxes[0] * naxes[1] )
    {
        // If here and m_data is not null, then it is already allocated with new.
        if( m_data )
            delete[] m_data;
        m_data = new char[naxes[0] * naxes[1] * sizeof( float )];
    }

    // always set in case of a reformat
    m_nx = naxes[0];
    m_ny = naxes[1];

    long fpix[3];
    long lpix[3];
    long inc[3];

    fpix[0] = 1;
    fpix[1] = 1;
    fpix[2] = 1;

    lpix[0] = naxes[0];
    lpix[1] = naxes[1];
    lpix[2] = 1;

    inc[0] = 1;
    inc[1] = 1;
    inc[2] = 1;

    delete[] naxes;

    int anynul;

    fits_read_subset( fptr, TFLOAT, fpix, lpix, inc, 0, (void *)m_data, &anynul, &fstatus );

    if( fstatus )
    {
        if( !m_reported )
            std::cerr << "rtimv: error reading data from " << m_fileList[imno] << "\n";
        m_reported = true;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        return -1;
    }
    m_reported = false;

    this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();

    fits_close_file( fptr, &fstatus );

    if( fstatus )
    {
        if( !m_reported )
            std::cerr << "rtimv: error closing file " << m_fileList[imno] << "\n";
        m_reported = true;

        fstatus = 0;
        fits_close_file( fptr, &fstatus );

        return -1;
    }
    m_reported = false;

    return 0;
}

void fitsDirectory::imConnect()
{
    m_imageFound = 0;
    m_imageUpdated = false;

    m_fileList = mx::ioutils::getFileNames( m_dirPath, "", "", ".fits" );

    if( m_fileList.size() == 0 )
    {
        return;
    }

    m_imageNo = 1;
    m_nextImageNo = 0;

    m_imageFound = 1;

    m_lastWriteTime = std::filesystem::last_write_time( m_dirPath );

    emit connected();
}

int fitsDirectory::update()
{
    if( !m_imageFound )
        return RTIMVIMAGE_NOUPDATE;

    // Check file time to see if it changed.
    if( m_lastWriteTime != std::filesystem::last_write_time( m_dirPath ) )
    {
        // Handle change in directory, but this could be a lot smarter than just starting over
        detach();
        return RTIMVIMAGE_NOUPDATE;
    }

    // No change in directory, just keep playing.

    if( m_fileList.size() == 1 )
    {
        return RTIMVIMAGE_NOUPDATE;
    }
    if( m_nextImageNo != m_imageNo )
    {
        m_imageNo = m_nextImageNo;
        if( readImage( m_imageNo ) < 0 )
        {
            // this likely means we need to re-read the directory list
            detach();
            return RTIMVIMAGE_NOUPDATE;
        }

        return RTIMVIMAGE_IMUPDATE;
    }

    return RTIMVIMAGE_IMUPDATE;
}

void fitsDirectory::detach()
{
    if( m_data )
    {
        delete[] m_data;
        m_data = 0;
    }

    m_imageFound = 0;

    // Start checking for the file
    m_timer.start( m_imageTimeout );

    return;
}

bool fitsDirectory::valid()
{
    if( m_imageFound && m_data )
        return true;

    return false;
}

void fitsDirectory::update_fps()
{
}

float fitsDirectory::pixel( size_t n )
{
    return pixget( m_data, n );
}

std::vector<std::string> fitsDirectory::info()
{
    std::vector<std::string> info = rtimvImage::info();
    info.push_back( std::string("path: ") + imageKey() );

    return info;
}

