#include "fitsDirectory.hpp"

#include <iostream>
#include <mx/ioutils/fileUtils.hpp>

#include <sys/inotify.h>

fitsDirectory::fitsDirectory()
{
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(imageTimerout()));

   m_notifyfd = inotify_init1(IN_NONBLOCK);
   if(m_notifyfd < 0)
   {
      perror("rtimv: fitsDirectory: error intializing inotify");
      std::cerr << "Will not be able to watch file for changes\n";
   }
}

int fitsDirectory::imageKey( const std::string & sn )
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
   if(m_fileList.size() == 0)
   {
      return "";
   }

   return mx::ioutils::pathStem(m_fileList[m_currImage]);
}

void fitsDirectory::imageTimeout(int to)
{
   m_imageTimeout = to;
}

int fitsDirectory::imageTimeout()
{
   return m_imageTimeout;
}

void fitsDirectory::timeout(int to)
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
   
double fitsDirectory::imageTime()
{
   return m_lastImageTime;
}
   
void fitsDirectory::imageTimerout()
{
   m_timer.stop();
   imConnect();

   if(!m_imageFound)
   {
      m_timer.start(m_imageTimeout);
   }
}

int fitsDirectory::readImage(size_t imno)
{
   if(imno >= m_fileList.size()) return -1;

   ///The cfitsio data structure
   fitsfile * fptr {nullptr};
   
   int fstatus = 0;

   fits_open_file(&fptr, m_fileList[imno].c_str(), READONLY, &fstatus);

   if (fstatus)
   {
      if(!m_reported) std::cerr << "rtimv: " << m_fileList[imno] << " not found.\n";
      m_reported = true;
      return -1;
   }
   m_reported = false;

   ///The dimensions of the image (1D, 2D, 3D etc)
   int naxis;

   fits_get_img_dim(fptr, &naxis, &fstatus);
   if (fstatus)
   {
      if(!m_reported) std::cerr << "rtimv: error getting number of axes in file " << m_fileList[imno] << "\n";
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
      if(!m_reported) std::cerr << "rtimv: error getting dimensions in file " << m_fileList[imno] << "\n";
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
      if(!m_reported) std::cerr << "rtimv: error reading data from " << m_fileList[imno] << "\n";
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
      if(!m_reported) std::cerr << "rtimv: error closing file " << m_fileList[imno] << "\n";
      m_reported = true;

      fstatus = 0;
      fits_close_file(fptr, &fstatus);

      return -1;
   }
   m_reported = false;

   return 0;
}

void fitsDirectory::imConnect()
{
   m_imageFound = 0;
   m_imageUpdated = false;
   
   m_fileList = mx::ioutils::getFileNames(m_dirPath, "", "", ".fits");

   if(m_fileList.size() == 0)
   {
      return;
   }

   m_currImage = 0;

   m_imageFound = 1;
   
   if(m_notifyfd < 0) 
   {
      emit connected();
      return;
   }
   
   m_notifywd = inotify_add_watch(m_notifyfd, m_dirPath.c_str(), IN_CREATE | IN_DELETE );

   if(m_notifywd < 0)
   {
      perror("rtimv: fitsDirectory: error adding inotify watch");
   }

   emit connected();
}

int fitsDirectory::update()
{   
   if(!m_imageFound) return RTIMVIMAGE_NOUPDATE;


   //Read inotify to see if it changed.

   ssize_t len;

   if(m_notifyfd >= 0 )
   {
      //Check if the directory changed
      len = read(m_notifyfd, m_notify_buf, sizeof(m_notify_buf));
      if (len == -1 && errno != EAGAIN)
      {
         perror("rtimv: fitsDirectory: error reading inotify");
         detach();
         return RTIMVIMAGE_NOUPDATE;
      }

      if (len > 0)
      {
         //Handle change in directory, but this could be a lot smarter than just starting over
         detach();
         return RTIMVIMAGE_NOUPDATE;
      }
   }
   //No change in directory, just keep playing.

   if(m_fileList.size() == 1) return RTIMVIMAGE_NOUPDATE;

   if(readImage(m_currImage) < 0)
   {
      //this likely means we need to re-read the directory list
      detach();
      return RTIMVIMAGE_NOUPDATE;
   }


   ++m_currImage;
   if(m_currImage >= m_fileList.size()) m_currImage =0;

   return RTIMVIMAGE_IMUPDATE;

}

void fitsDirectory::detach()
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

bool fitsDirectory::valid()
{
   if(m_imageFound && m_data) return true;
   
   return false;
}

void fitsDirectory::update_fps()
{

}

float fitsDirectory::pixel(size_t n)
{
   return pixget(m_data, n);
}


