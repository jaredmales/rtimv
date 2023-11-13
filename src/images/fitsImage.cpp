#include "fitsImage.hpp"

#include <iostream>
#include <mx/ioutils/fileUtils.hpp>
#include <unistd.h>
#include <sys/inotify.h>

fitsImage::fitsImage(std::mutex * mut) : rtimvImage(mut)
{
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(imageTimerout()));

   m_notifyfd = inotify_init1(IN_NONBLOCK);
   if(m_notifyfd < 0)
   {
      perror("rtimv: fitsImage: error intializing inotify");
      std::cerr << "Will not be able to watch file for changes\n";
   }
}

int fitsImage::imageKey( const std::string & sn )
{
   m_imagePath = sn;
   
   if( m_imagePath.find(".fits") == std::string::npos )
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
   return mx::ioutils::pathStem(m_imagePath);
}

void fitsImage::imageTimeout(int to)
{
   m_imageTimeout = to;
}

int fitsImage::imageTimeout()
{
   return m_imageTimeout;
}

void fitsImage::timeout(int to)
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
   
double fitsImage::imageTime()
{
   return m_lastImageTime;
}
   
void fitsImage::imageTimerout()
{
   m_timer.stop();
   imConnect();

   if(!m_imageFound)
   {
      m_timer.start(m_imageTimeout);
   }
}

int fitsImage::readImage()
{
    ///The cfitsio data structure
    fitsfile * fptr {nullptr};
   
    int fstatus = 0;

    fits_open_file(&fptr, m_imagePath.c_str(), READONLY, &fstatus);

    if (fstatus)
    {
        //we try again 100 ms later in case this was a rewrite of the existing file
        mx::sys::milliSleep(100);
        fstatus = 0;
        fits_open_file(&fptr, m_imagePath.c_str(), READONLY, &fstatus);
      
        if(fstatus)
        {
            if(!m_reported) std::cerr << "rtimv: " << m_imagePath << " not found.\n";
            m_reported = true;
            return -1;
        }
   }
   m_reported = false;

   ///The dimensions of the image (1D, 2D, 3D etc)
   int naxis;

   fits_get_img_dim(fptr, &naxis, &fstatus);
   if (fstatus)
   {
      if(!m_reported) std::cerr << "rtimv: error getting number of axes in file " << m_imagePath << "\n";
      m_reported = true;

      fstatus = 0;
      fits_close_file(fptr, &fstatus);

      return -1;
   }
   m_reported = false;

   long * naxes = new long[naxis];

   fits_get_img_size(fptr, naxis, naxes, &fstatus);
   if (fstatus)
   {
      if(!m_reported) std::cerr << "rtimv: error getting dimensions in file " << m_imagePath << "\n";
      m_reported = true;

      fstatus = 0;
      fits_close_file(fptr, &fstatus);

      delete naxes;
      return -1;
   }
   m_reported = false;

   //resize the array if needed, which could be a reformat
   if(m_data == nullptr || m_nx*m_ny != naxes[0]*naxes[1])
   {
      //If here and m_data is not null, then it is already allocated with new.
      if(m_data) delete[] m_data;
      m_data = new char[naxes[0]*naxes[1]*sizeof(float)];
   }

   //always set in case of a reformat
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

   delete naxes;

   int anynul;
   
   fits_read_subset(fptr, TFLOAT, fpix, lpix, inc, 0,
                                     (void *) m_data, &anynul, &fstatus);
   
   if (fstatus)
   {
      if(!m_reported) std::cerr << "rtimv: error reading data from " << m_imagePath << "\n";
      m_reported = true;

      fstatus = 0;
      fits_close_file(fptr, &fstatus);
      
      return -1;
   }
   m_reported = false;

   this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();
   
   fits_close_file(fptr, &fstatus);

   if (fstatus)
   {
      if(!m_reported) std::cerr << "rtimv: error closing file " << m_imagePath << "\n";
      m_reported = true;

      fstatus = 0;
      fits_close_file(fptr, &fstatus);

      return -1;
   }
   m_reported = false;

   return 0;
}

void fitsImage::imConnect()
{
   m_imageFound = 0;
   m_imageUpdated = false;
   
   if(readImage() < 0)
   {
      return;
   }

   m_imageFound = 1;
   
   if(m_notifyfd < 0) 
   {
      emit connected();
      return;
   }
   
   m_notifywd = inotify_add_watch(m_notifyfd, m_imagePath.c_str(), IN_CLOSE_WRITE);

   if(m_notifywd < 0)
   {
      perror("rtimv: fitsImage: error adding inotify watch");
   }

   emit connected();
}

int fitsImage::update()
{   
   if(!m_imageFound) return RTIMVIMAGE_NOUPDATE;

   //First time through after connect
   if(!m_imageUpdated)  
   {
      m_imageUpdated = true;
         
      return RTIMVIMAGE_IMUPDATE;
   }

   //Read inotify to see if it changed.
   
   ssize_t len;
   
   if(m_notifyfd < 0 ) return RTIMVIMAGE_NOUPDATE;

   len = read(m_notifyfd, m_notify_buf, sizeof(m_notify_buf));
   if (len == -1 && errno != EAGAIN) 
   {
      perror("rtimv: fitsImage: error reading inotify");
      detach();
      return RTIMVIMAGE_NOUPDATE;
   }

   if (len <= 0)
   {
      return RTIMVIMAGE_NOUPDATE;
   }
   else
   {
      if(readImage() < 0)
      {
         detach();
         return RTIMVIMAGE_NOUPDATE;
      };

      m_notifywd = inotify_add_watch(m_notifyfd, m_imagePath.c_str(), IN_CLOSE_WRITE);

      return RTIMVIMAGE_IMUPDATE;
   }
}

void fitsImage::detach()
{  
   if(m_data)
   {
      delete[] m_data;
      m_data = 0;
   }
      
   m_imageFound = 0;
      
   //Start checking for the file
   m_timer.start(m_imageTimeout);

   return;
}

bool fitsImage::valid()
{
   if(m_imageFound && m_data) return true;
   
   return false;
}

void fitsImage::update_fps()
{

}

float fitsImage::pixel(size_t n)
{
   return pixget(m_data, n);
}

std::vector<std::string> fitsImage::info()
{
    std::vector<std::string> info = rtimvImage::info();
    info.push_back(std::string( imageName().size()+1, ' ') + imageKey());

    return info;
}
