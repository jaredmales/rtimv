#include "fitsImage.hpp"

#include <iostream>
#include <mx/ioutils/fileUtils.hpp>

fitsImage::fitsImage()
{
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(imageTimerout()));
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

void fitsImage::imConnect()
{
   
   
   m_imageFound = 0;
   m_imageUpdated = false;
   
   ///The cfitsio data structure
   fitsfile * fptr {nullptr};
   
   int fstatus = 0;

   fits_open_file(&fptr, m_imagePath.c_str(), READONLY, &fstatus);

   if (fstatus)
   {
      std::cerr << "Could not open " << m_imagePath << "\n";
      return;
   }
   
   ///The dimensions of the image (1D, 2D, 3D etc)
   int naxis;

   fits_get_img_dim(fptr, &naxis, &fstatus);
   if (fstatus)
   {
      std::cerr << "Error getting number of axes in file " << m_imagePath << "\n";
      return;
   }

   long * naxes = new long[naxis];

   fits_get_img_size(fptr, naxis, naxes, &fstatus);
   if (fstatus)
   {
      std::cerr << "Error getting dimensions in file " << m_imagePath << "\n";
      return;
   }

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
      
   long fpix[2];
   long lpix[2];
   long inc[2];
   
   fpix[0] = 1;
   fpix[1] = 1;
   
   lpix[0] = naxes[0];
   lpix[1] = naxes[1];
   
   inc[0] = 1;
   inc[1] = 1;
   
   int anynul;
   
   fits_read_subset(fptr, TFLOAT, fpix, lpix, inc, 0,
                                     (void *) m_data, &anynul, &fstatus);
   
   this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();
   m_imageFound = 1;
   
   
   fits_close_file(fptr, &fstatus);

   if (fstatus)
   {
      std::cerr << "Error closing file " << m_imagePath << "\n";
      return;
   }
   
   emit connected();
}

int fitsImage::update()
{   
   if(!m_imageFound) return RTIMVIMAGE_NOUPDATE;
      
   if(!m_imageUpdated)  
   {
      m_imageUpdated = true;
         
      return RTIMVIMAGE_IMUPDATE;
   }
      
   return RTIMVIMAGE_NOUPDATE;   
}

void fitsImage::detach()
{  
   if(m_imageFound == 0) return;
   
   if(m_data)
   {
      delete[] m_data;
      m_data = 0;
   }
      
   m_imageFound = 0;
      
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


