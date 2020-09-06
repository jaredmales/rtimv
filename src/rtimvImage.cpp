#include "rtimvImage.hpp"


errno_t isio_err_to_ignore = 0;
errno_t new_printError( const char *file, const char *func, int line, errno_t code, char *errmessage )
{
   if(code == isio_err_to_ignore) return IMAGESTREAMIO_SUCCESS;
   
   std::cerr << "ImageStreamIO Error:\n\tFile: " << file << "\n\tLine: " << line << "\n\tFunc: " << func << "\n\tMsg:  " << errmessage << std::endl; 
   return IMAGESTREAMIO_SUCCESS;
}

rtimvImage::rtimvImage()
{
   ImageStreamIO_set_printError(new_printError);
   
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(shmimTimerout()));
}

int rtimvImage::shmimName( const std::string & sn )
{
   m_shmimName = sn;
   
   if( m_shmimName.find(".fits") != std::string::npos )
   {
      m_isStatic = true;
   }
   
   return 0;
}

std::string rtimvImage::shmimName()
{
   return m_shmimName;
}

bool rtimvImage::isStatic()
{
   return m_isStatic;
}
   
void rtimvImage::shmimTimeout(int to)
{
   m_shmimTimeout = to;
}

int rtimvImage::shmimTimeout()
{
   return m_shmimTimeout;
}

void rtimvImage::timeout(int to)
{
   m_timeout = to;
}

uint32_t rtimvImage::nx()
{ 
   return m_nx; 
}
   
uint32_t rtimvImage::ny()
{ 
   return m_ny;
}
   
double rtimvImage::imageTime()
{
   return m_image.md->atime.tv_sec + ((double) m_image.md->atime.tv_nsec)/1e9;
}
   
void rtimvImage::shmimTimerout()
{
   m_timer.stop();
   imConnect();

   if(!m_shmimAttached)
   {
      m_timer.start(m_shmimTimeout);
   }
}

void rtimvImage::imConnect()
{
   if(m_isStatic)
   {
      imConnectStatic();
      return;
   }
   
   isio_err_to_ignore = IMAGESTREAMIO_FILEOPEN;
   if( ImageStreamIO_openIm(&m_image, m_shmimName.c_str()) != 0)
   {
      m_data = nullptr;
      m_shmimAttached = 0;
      isio_err_to_ignore = 0;
      return; //comeback on next timeout
   }
   isio_err_to_ignore = 0;
   
   if(m_image.md->sem <= 1)  //Creation not complete yet (believe it or not this happens!)
   {
      ImageStreamIO_closeIm(&m_image);
      m_data = nullptr;
      m_shmimAttached = 0;
      return; //We just need to wait for the server process to finish startup, so come back on next timeout
   }
   
   m_nx = m_image.md->size[0];
   m_ny = m_image.md->size[1];
      
   m_typeSize = ImageStreamIO_typesize(m_image.md->datatype);
         
   switch(m_image.md->datatype)
   {
      case IMAGESTRUCT_UINT8:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT8>();
         break;
      case IMAGESTRUCT_INT8:
         this->pixget = getPixPointer<IMAGESTRUCT_INT8>();
         break;
      case IMAGESTRUCT_UINT16:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT16>();
         break;
      case IMAGESTRUCT_INT16:
         this->pixget = getPixPointer<IMAGESTRUCT_INT16>();
         break;
      case IMAGESTRUCT_UINT32:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT32>();
         break;
      case IMAGESTRUCT_INT32:
         this->pixget = getPixPointer<IMAGESTRUCT_INT32>();
         break;
      case IMAGESTRUCT_UINT64:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT64>();
         break;
      case IMAGESTRUCT_INT64:
         this->pixget = getPixPointer<IMAGESTRUCT_INT64>();
         break;
      case IMAGESTRUCT_FLOAT:
         this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();
         break;
      case IMAGESTRUCT_DOUBLE:
         this->pixget = getPixPointer<IMAGESTRUCT_DOUBLE>();
         break;
      default:
         std::cerr << "Unknown or unsupported data type\n";
         exit(0);
   }
   
   m_shmimAttached = 1;

   emit connected();
   
}

void rtimvImage::imConnectStatic()
{
   if(!m_isStatic)
   {
      std::cerr << "Attempting to connect to static file, but m_isStatic is false\n";
      return;
   }
   
   //If here and m_data is not null, then it is already allocated with new.
   if(m_data) delete[] m_data;
   
   m_data = nullptr;
   m_shmimAttached = 0;
   m_staticUpdated = false;
   
   ///The cfitsio data structure
   fitsfile * fptr {nullptr};
   
   int fstatus = 0;

   fits_open_file(&fptr, m_shmimName.c_str(), READONLY, &fstatus);

   if (fstatus)
   {
      std::cerr << "Could not open " << m_shmimName << "\n";
      return;
   }
   
   ///The dimensions of the image (1D, 2D, 3D etc)
   int naxis;

   fits_get_img_dim(fptr, &naxis, &fstatus);
   if (fstatus)
   {
      std::cerr << "Error getting number of axes in file " << m_shmimName << "\n";
      return ;
   }

   long * naxes = new long[naxis];

   fits_get_img_size(fptr, naxis, naxes, &fstatus);
   if (fstatus)
   {
      std::cerr << "Error getting dimensions in file " << m_shmimName << "\n";
      return;
   }
   
   m_nx = naxes[0];
   m_ny = naxes[1];
      
   m_data = new char[naxes[0]*naxes[1]*sizeof(float)];
   
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
   
   m_typeSize = ImageStreamIO_typesize(IMAGESTRUCT_FLOAT);
   this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();
   m_shmimAttached = 1;
   
   
   fits_close_file(fptr, &fstatus);

   if (fstatus)
   {
      std::cerr << "Error closing file " << m_shmimName << "\n";
      return;
   }
   
   emit connected();
}

int rtimvImage::update()
{   
   if( m_isStatic ) 
   {
      if(!m_shmimAttached) return RTIMVIMAGE_NOUPDATE;
      
      if(!m_staticUpdated)  
      {
         m_staticUpdated = true;
         
         return RTIMVIMAGE_IMUPDATE;
      }
      
      return RTIMVIMAGE_NOUPDATE;
   }
   
   if(!m_shmimAttached)
   {
      if(m_age_counter > 1000/m_timeout)
      {
         m_age_counter = 0;
         m_fps_counter = 0;
         m_fpsEst = 0;
         return RTIMVIMAGE_AGEUPDATE;
      }
      else 
      {
         ++m_age_counter;
         return RTIMVIMAGE_NOUPDATE;
      }
   }
   
   
   int64_t curr_image;
   uint64_t cnt0;
      
   uint32_t snx, sny;

   
   if(m_image.md->sem <= 0) //Indicates that the server has cleaned up.
   {
      m_data = nullptr;
      ImageStreamIO_closeIm(&m_image);
      m_shmimAttached = 0;
      m_lastCnt0 = -1;
      m_timer.start(m_shmimTimeout);
      
      return RTIMVIMAGE_NOUPDATE;
   }
   
   if(m_image.md->size[2] > 0)
   {
      curr_image = m_image.md->cnt1;
      if(curr_image < 0) curr_image = m_image.md->size[2] - 1;
   }
   else curr_image = 0;

   
   snx = m_image.md->size[0];
   sny = m_image.md->size[1];
   
   if( snx != m_nx || sny != m_ny ) //Something else changed!
   {
      m_data = nullptr;
      ImageStreamIO_closeIm(&m_image);
      m_shmimAttached = 0;
      m_lastCnt0 = -1;
      m_timer.start(m_shmimTimeout);
      return RTIMVIMAGE_NOUPDATE;
   }
   
   cnt0 = m_image.md->cnt0;
   
   if(cnt0 != m_lastCnt0) //Only redraw if it's actually a new image.
   {
      m_data = ((char *) (m_image.array.raw)) + curr_image*snx*sny*m_typeSize;
      
      m_lastCnt0 = cnt0;
      m_age_counter = 0;
      
      if(m_fps_counter > 1000/m_timeout)
      {
         update_fps();
         m_fps_counter = 0;
         return RTIMVIMAGE_FPSUPDATE;
      }
      else
      {
         ++m_fps_counter;
         return RTIMVIMAGE_IMUPDATE;
      }
      
   }
   else
   {
      if(m_age_counter > 250/m_timeout)
      {
         m_age_counter = 0;
         m_fps_counter = 1000/m_timeout+1;
         return RTIMVIMAGE_AGEUPDATE;
      }
      else
      {
         ++m_age_counter;
         ++m_fps_counter;
         return RTIMVIMAGE_NOUPDATE;
      }
   }
   
   return RTIMVIMAGE_NOUPDATE; 
}

void rtimvImage::detach()
{  
   if(m_shmimAttached == 0) return;
   
   if(m_isStatic)
   {
      delete[] m_data;
      m_data = 0;
      m_shmimAttached = 0;
      
      return;
   }
      
      
   m_data = nullptr; 
   ImageStreamIO_closeIm(&m_image);
   m_shmimAttached = 0;
   m_lastCnt0 = -1;
   m_timer.start(m_shmimTimeout);
}

bool rtimvImage::valid()
{
   if(m_shmimAttached && m_data) return true;
   
   return false;
}

void rtimvImage::update_fps()
{
   double dftime;

   m_fpsTime = m_image.md->atime.tv_sec + ((double) m_image.md->atime.tv_nsec)/1e9;
   
   if(m_fpsTime0 == 0)
   {
      m_fpsTime0 = m_fpsTime;
      m_fpsFrame0 = m_image.md->cnt0;
   }
   
   if(m_fpsTime != m_fpsTime0)
   {
      dftime = m_fpsTime - m_fpsTime0;

      if(dftime < 1e-9) return;

      m_fpsEst = (float)((m_image.md->cnt0 - m_fpsFrame0))/dftime;   
      
      m_fpsTime0 = m_fpsTime;
      m_fpsFrame0 = m_image.md->cnt0;
   }

}


float rtimvImage::pixel(size_t n)
{
   return pixget(m_data, n);
}


