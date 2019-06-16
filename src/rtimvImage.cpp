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

void rtimvImage::shmimTimeout(int to)
{
   m_shmimTimeout = to;
}

void rtimvImage::timeout(int to)
{
   m_timeout = to;
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

int rtimvImage::update()
{   
   if(!m_shmimAttached)
   {
      if(age_counter > 1000/m_timeout)
      {
         age_counter = 0;
         fps_counter = 0;
         m_fpsEst = 0;
         return RTIMVIMAGE_AGEUPDATE;
      }
      else 
      {
         ++age_counter;
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
      lastCnt0 = -1;
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
      lastCnt0 = -1;
      m_timer.start(m_shmimTimeout);
      return RTIMVIMAGE_NOUPDATE;
   }
   
   cnt0 = m_image.md->cnt0;
   
   if(cnt0 != lastCnt0) //Only redraw if it's actually a new image.
   {
      m_data = ((char *) (m_image.array.raw)) + curr_image*snx*sny*m_typeSize;
      
      lastCnt0 = cnt0;
      age_counter = 0;
      
      if(fps_counter > 1000/m_timeout)
      {
         update_fps();
         fps_counter = 0;
         return RTIMVIMAGE_FPSUPDATE;
      }
      else
      {
         ++fps_counter;
         return RTIMVIMAGE_IMUPDATE;
      }
      
   }
   else
   {
      if(age_counter > 250/m_timeout)
      {
         age_counter = 0;
         fps_counter = 1000/m_timeout+1;
         return RTIMVIMAGE_AGEUPDATE;
      }
      else
      {
         ++age_counter;
         ++fps_counter;
         return RTIMVIMAGE_NOUPDATE;
      }
   }
   
   return RTIMVIMAGE_NOUPDATE; 
}

void rtimvImage::detach()
{  
   if(m_shmimAttached == 0) return;
   
   m_data = nullptr; 
   ImageStreamIO_closeIm(&m_image);
   m_shmimAttached = 0;
   lastCnt0 = -1;
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


