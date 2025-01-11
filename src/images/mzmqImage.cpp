#include "mzmqImage.hpp"

#include <iostream>
#include <mx/sys/timeUtils.hpp>
#include <xrif/xrif.h>

//The milkzmq messager format:
/*
 *  0-127    image stream name
 *  128      data type code (uint8_t)
 *  129-141  size 0 (uint32_t)

 */
constexpr size_t headerSize = 256; ///< total size in bytes of the header,  gives room to grow!

constexpr size_t nameSize = 128; ///< The size of the name field.
constexpr size_t typeOffset = nameSize;                             ///< Start of data type field
constexpr size_t size0Offset = typeOffset+sizeof(uint8_t);     ///< start of size0 field
constexpr size_t size1Offset = size0Offset + sizeof(uint32_t); ///< start of size1 field
constexpr size_t cnt0Offset = size1Offset + sizeof(uint32_t);
constexpr size_t tv_secOffset = cnt0Offset + sizeof(uint64_t);
constexpr size_t tv_nsecOffset = tv_secOffset + sizeof(uint64_t);
constexpr size_t xrifDifferenceOffset = tv_nsecOffset + sizeof(uint64_t); ///< The XRIF encoding differencing method.  Generally must be PIXEL.
constexpr size_t xrifReorderOffset = xrifDifferenceOffset + sizeof(int16_t);                  ///< The XRIF encoding reordering method.
constexpr size_t xrifCompressOffset = xrifReorderOffset + sizeof(int16_t);                 ///< The XRIF encoding compression method.
constexpr size_t xrifSizeOffset =  xrifCompressOffset + sizeof(int16_t);                    ///< The size of the compressed data.

constexpr size_t endOfHeader = xrifSizeOffset + sizeof(uint16_t);         ///< The current end of the header.
constexpr size_t imageOffset = headerSize;

static_assert(endOfHeader <= imageOffset, "Header fields sum to larger than reserved headerSize");


mzmqImage::mzmqImage(std::mutex * mut) : rtimvImage(mut)
{
   m_ZMQ_context = new zmq::context_t(1);

}

mzmqImage::~mzmqImage()
{
   m_timeToDie = true;

   if(m_ZMQ_context) delete m_ZMQ_context;
}

int mzmqImage::imageKey( const std::string & sn )
{
   m_imageKey = sn;

   size_t ats = sn.find('@');

   if(ats != std::string::npos)
   {
      if(ats == 0)
      {
         std::cerr << "No image name in mzmq image key: " << sn << "\n";
         return -1;
      }

      m_imageName = sn.substr(0, ats);

      size_t pcol = sn.find(':', ats);

      if(pcol != std::string::npos)
      {
         if(pcol > ats+1) m_server = sn.substr(ats+1, pcol-ats-1);

         if(sn.size() > pcol+1) m_port = std::stoi(sn.substr(pcol+1, sn.size()-pcol-1));
      }
      else
      {
         if(sn.size() > ats+1) m_server = sn.substr(ats+1, sn.size()-ats-1);
      }
   }
   else
   {
      size_t pcol = sn.find(':');

      if(pcol == 0)
      {
         std::cerr << "No image name in mzmq image key: " << sn << "\n";
         return -1;
      }

      if(pcol == std::string::npos)
      {
         m_imageName = sn;
      }
      else
      {
         m_imageName = sn.substr(0,pcol);
         if(sn.size() > pcol+1) m_port = std::stoi(sn.substr(pcol+1, sn.size()-pcol-1));
      }
   }

   //start thread here.

   if(imageThreadStart() != 0)
   {
      exit(0);
   }

   return 0;
}

std::string mzmqImage::imageKey()
{
   return m_imageName + "@" + m_server + ":" + std::to_string(m_port);
}

std::string mzmqImage::imageName()
{
   return m_imageName;
}

void mzmqImage::imageServer(const std::string & server)
{
   m_server = server;
}

void mzmqImage::imagePort(int port)
{
   m_port = port;
}

void mzmqImage::timeout(int to)
{
   m_timeout = to;
}

uint32_t mzmqImage::nx()
{
   return m_nx;
}

uint32_t mzmqImage::ny()
{
   return m_ny;
}

uint32_t mzmqImage::nz()
{
   return 1;
}

uint32_t mzmqImage::imageNo()
{
   return 0;
}

double mzmqImage::imageTime()
{
   return m_imageTime;
}

void mzmqImage::internal_imageThreadStart( mzmqImage * mi )
{
   mi->imageThreadExec();
}

int mzmqImage::imageThreadStart()
{
   try
   {
      m_thread = std::thread( internal_imageThreadStart, this);
   }
   catch( const std::exception & e )
   {
      std::cerr << std::string("exception in image thread startup: ") + e.what() << " " <<  __FILE__ << " " <<  __LINE__ << "\n";
      return -1;
   }
   catch( ... )
   {
      std::cerr << "unknown exception in image thread startup " <<  __FILE__ << " " <<  __LINE__ << "\n";
      return -1;
   }

   if(!m_thread.joinable())
   {
      std::cerr << "image thread did not start " << __FILE__ << " " << __LINE__ << "\n";
      return -1;
   }

   return 0;
}

void mzmqImage::imageThreadExec()
{
   std::string srvstr = "tcp://" + m_server + ":" + std::to_string(m_port);

   std::cerr << "Beginning receive at " + srvstr + " for " + m_imageName << "\n";


   xrif_typecode_t new_atype, atype=0;
   size_t new_typesize, typesize;
   xrif_dimension_t new_nx, nx =0;
   xrif_dimension_t new_ny, ny =0;

   /* Initialize xrif
    */
   xrif_error_t xe;
   xrif_t xrif;
   xe = xrif_new(&xrif);

   if(xe != XRIF_NOERROR)
   {
      std::cerr << "Failure allocating xrif handle for " + m_imageName << "\n";
      exit(0);
   }

   //Outer loop, which will periodically refresh the subscription if needed.
   while(!m_timeToDie)
   {
      zmq::socket_t subscriber (*m_ZMQ_context, ZMQ_CLIENT);

      #if (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
      subscriber.set(zmq::sockopt::rcvtimeo, 1000);
      subscriber.set(zmq::sockopt::linger, 0);
      #else
      subscriber.setsockopt(ZMQ_RCVTIMEO, 1000);
      subscriber.setsockopt(ZMQ_LINGER, 0);
      #endif

      subscriber.connect(srvstr);

      zmq::message_t request(m_imageName.data(), m_imageName.size());

      #if (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
      subscriber.send(request, zmq::send_flags::none);
      #else
      subscriber.send(request);
      #endif
      bool reconnect = false;


      bool first = true;
      bool connected = false;

      #ifdef MZMQ_FPS_MONITORING
      int Nrecvd = 100;
      double t0 = 0, t1;
      #endif

      while(!m_timeToDie && !reconnect) //Inner loop waits for each new image and processes it as it comes in.
      {
         zmq::message_t msg;

         #if (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 7, 0))
         zmq::recv_result_t recvd;
         #elif (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
         zmq::detail::recv_result_t recvd;
         #else
         size_t recvd;
         #endif

         try
         {
            #if (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
               recvd = subscriber.recv(msg);
            #else
               recvd = subscriber.recv(&msg);
            #endif
         }
         catch(...)
         {
            if(m_timeToDie) break; //This will true be if signaled during shutdown
            //otherwise, this is an error
            throw;
         }

         #if (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
         if(!recvd)
         {
            if(zmq_errno() == EAGAIN) //If we timed out, just re-send the request
            {
               request.rebuild(m_imageName.data(), m_imageName.size());
               subscriber.send(request, zmq::send_flags::none);
               continue;
            }

            if(connected) std::cerr << "Disconnected from " + imageKey() << "\n";
            connected = false;
            reconnect = true;
            break;
         }
         #else
         if(recvd == 0)
         {
            if(zmq_errno() == EAGAIN) //If we timed out, just re-send the request
            {
               request.rebuild(m_imageName.data(), m_imageName.size());
               subscriber.send(request);
               continue;
            }

            if(connected) std::cerr << "Disconnected from " + imageKey() << "\n";
            connected = false;
            reconnect = true;
            break;
         }
         #endif

         if(first)
         {
            std::cerr << "Connected to " + imageKey() << "\n";
            connected = true;
            first = false;
         }

         if(msg.size() <= headerSize) //If we don't get enough data, we reconnect to the server.
         {
            mx::sys::sleep(1); //Give server time to finish its shutdown.
            reconnect= true;
            continue;
         }

         char * raw_image= (char *) msg.data();

         new_atype = *( (uint8_t *) (raw_image + typeOffset) );
         new_typesize = xrif_typesize(new_atype);
         new_nx = *( (uint32_t *) (raw_image + size0Offset));
         new_ny = *( (uint32_t *) (raw_image + size1Offset));

         if( nx != new_nx || ny != new_ny || atype != new_atype || !m_imageAttached)
         {
            std::lock_guard<std::mutex> guard(*m_accessMutex);

            if(m_data) //This can't be a call to detach due to mutex
            {
               m_imageAttached = false;
               delete m_data;
               m_data = nullptr;
            }

            m_nx = new_nx;
            m_ny = new_ny;
            m_data = new char[new_nx*new_ny*new_typesize];

            switch(new_atype)
            {
               case XRIF_TYPECODE_UINT8:
                  this->pixget = getPixPointer<IMAGESTRUCT_UINT8>();
                  break;
               case XRIF_TYPECODE_INT8:
                  this->pixget = getPixPointer<IMAGESTRUCT_INT8>();
                  break;
               case XRIF_TYPECODE_UINT16:
                  this->pixget = getPixPointer<XRIF_TYPECODE_UINT16>();
                  break;
               case XRIF_TYPECODE_INT16:
                  this->pixget = getPixPointer<XRIF_TYPECODE_INT16>();
                  break;
               case XRIF_TYPECODE_UINT32:
                  this->pixget = getPixPointer<XRIF_TYPECODE_UINT32>();
                  break;
               case XRIF_TYPECODE_INT32:
                  this->pixget = getPixPointer<XRIF_TYPECODE_INT32>();
                  break;
               case XRIF_TYPECODE_UINT64:
                  this->pixget = getPixPointer<XRIF_TYPECODE_UINT64>();
                  break;
               case XRIF_TYPECODE_INT64:
                  this->pixget = getPixPointer<XRIF_TYPECODE_INT64>();
                  break;
               case XRIF_TYPECODE_FLOAT:
                  this->pixget = getPixPointer<XRIF_TYPECODE_FLOAT>();
                  break;
               case XRIF_TYPECODE_DOUBLE:
                  this->pixget = getPixPointer<XRIF_TYPECODE_DOUBLE>();
                  break;
               default:
                  std::cerr << "Unknown or unsupported data type\n";
                  exit(0);
            }

            xe = xrif_set_size(xrif, new_nx, new_ny, 1, 1, new_atype);
            xrif_set_difference_method(xrif, *((int16_t *) (raw_image + xrifDifferenceOffset)));
            xrif_set_reorder_method(xrif, *((int16_t *) (raw_image + xrifReorderOffset)));
            xrif_set_compress_method(xrif, *((int16_t *) (raw_image+ xrifCompressOffset)));

            xe = xrif_allocate(xrif);

            m_imageAttached = true;
         }

         atype = new_atype;
         typesize = new_typesize;
         nx = new_nx;
         ny = new_ny;


         m_cnt0 = *( (uint64_t *) (raw_image + cnt0Offset));
         uint64_t tv_sec = *( (uint64_t *) (raw_image + tv_secOffset));
         uint64_t tv_nsec = *( (uint64_t *) (raw_image + tv_nsecOffset));

         m_imageTime = tv_sec + ((double) tv_nsec)/1e9;

         xrif->compressed_size =  *((uint32_t *) (raw_image + xrifSizeOffset));

         memcpy(xrif->raw_buffer, raw_image + imageOffset, xrif->compressed_size);
         xe = xrif_decode(xrif);

         if(!m_imageAttached || m_timeToDie) continue; //Check that detach wasn't called.
         memcpy(m_data, xrif->raw_buffer, nx*ny*typesize);

         #ifdef MZMQ_FPS_MONITORING
         if(Nrecvd >= 10)
         {
            Nrecvd = 0;
            t0 = get_curr_time();
         }
         else ++Nrecvd;

         if(Nrecvd >= 10)
         {
            t1 = get_curr_time() - t0;
            std::cerr << m_imageName << " averaging " << Nrecvd/t1 << " FPS received.\n";
         }
         #endif

         //Here is where we can add client-specefic rate control!

         if(!m_imageAttached || m_timeToDie) continue; //Check that detach wasn't called.

         request.rebuild(m_imageName.data(), m_imageName.size());

         #if (CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 1))
         subscriber.send(request, zmq::send_flags::dontwait);
         #else
         subscriber.send(request, ZMQ_DONTWAIT);
         #endif

      } // inner loop (image processing)

      subscriber.close(); //close so that unsent messages are dropped.

      detach();

      atype=0;
      nx =0;
      ny =0;

      first = true;
      connected = false;

      #ifdef MZMQ_FPS_MONITORING
      Nrecvd = 100;
      t0 = 0;
      #endif

      std::cerr << "Disconnected from " + imageKey() << "\n";

   }// outer loop (checking stale connections)

   detach();

   xrif_delete(xrif);

} // mzmqImage::imageThreadExec()

int mzmqImage::update()
{
   if(!m_data)
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

   if(m_cnt0 != m_lastCnt0) //Only redraw if it's actually a new image.
   {
      m_lastCnt0 = m_cnt0;
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

void mzmqImage::detach()
{
    std::lock_guard<std::mutex> guard(*m_accessMutex);

    m_imageAttached = false;
    if(m_data) delete m_data;
    m_data = nullptr;

    m_lastCnt0 = -1;
}

bool mzmqImage::valid()
{
    if(m_data && m_imageAttached)
    {
        return true;
    }

    return false;

}

float mzmqImage::pixel(size_t n)
{
   return pixget(m_data, n);
}

void mzmqImage::update_fps()
{
   double dftime;

   if(m_fpsTime0 == 0)
   {
      m_fpsTime0 = m_imageTime;
      m_fpsFrame0 = m_cnt0;
   }

   if(m_imageTime != m_fpsTime0)
   {
      dftime = m_imageTime - m_fpsTime0;

      if(dftime < 1e-9) return;

      m_fpsEst = (float)((m_cnt0 - m_fpsFrame0))/dftime;

      m_fpsTime0 = m_imageTime;
      m_fpsFrame0 = m_cnt0;
   }
}

float mzmqImage::fpsEst()
{
   return m_fpsEst;
}

std::vector<std::string> mzmqImage::info()
{
    std::vector<std::string> info = rtimvImage::info();
    info.push_back(std::string( imageName().size()+1, ' ') + "mzmq://" + m_server + ":" + std::to_string(m_port));

    return info;
}
