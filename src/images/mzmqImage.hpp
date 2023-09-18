/** \file mzmqImage.hpp
  * \brief Declarations for the mzmqImage management class
  *
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef rtimv_mzmqImage_hpp
#define rtimv_mzmqImage_hpp

#include <thread>

#ifndef ZMQ_BUILD_DRAFT_API
#define ZMQ_BUILD_DRAFT_API
#endif

#define ZMQ_CPP11
#include <zmq.hpp>

#include "../rtimvImage.hpp"
#include "pixaccess.hpp"

/// rtimvImage stream to a milkzmq protocol image stream
struct mzmqImage : public rtimvImage
{

   Q_OBJECT

protected:
   std::string m_imageKey; ///< The network specifications of the stream containing the image data.  Has the form `image@host:port`.

   std::string m_imageName;            ///< The name of the image.
   std::string m_server {"localhost"}; ///< The image server name or address.
   int m_port {5556};                  ///< The port on the server.

   int m_timeout {100}; ///< The image display timeout, should be set from the managing process.  Only used for F.P.S. calculations.
   
   uint32_t m_nx {0}; ///< Size of the image in pixels in the x direction (horizontal on screen)
   
   uint32_t m_ny {0}; ///< Size of the image in pixels in the y direction (vertical on screen)
   
   size_t m_typeSize; ///< The size, in bytes, of the image data type
 
   char * m_data {nullptr}; ///< Pointer to the image data
 
   bool m_imageAttached {false}; ///< Flag denoting whether or not we are attached to the stream.

   uint64_t m_cnt0 {0}; ///< Current frame number
   
   /// Previous frame number, stored to detect new image
   uint64_t m_lastCnt0 {std::numeric_limits<uint64_t>::max()}; //make huge so we don't skip the 0 image
   
   /// Counts how many times the FPS has been updated
   int m_fps_counter {0};

   /// Counts how many times the Age has been updated
   int m_age_counter {0};
   
   zmq::context_t * m_ZMQ_context {nullptr}; ///< The ZeroMQ context, allocated on construction.
   
   bool m_timeToDie {false}; ///< Flag to signal the imageThreadExec main loop to exit and clean up.
   
   std::thread m_thread; ///< The thread for the milkzmq client connection
   
public:
   
   ///Default c'tor
   mzmqImage();

   ///Destructor, cleans up the zmq context and allocate memory.
   ~mzmqImage();
   
   ///Set the image key to a shared memory image name 
   /** If this contains the string ".fits" then it is treated as a FITS file and loaded as a static image.  Otherwise
     * it is treated as an ImageStreamIO image stream, and added to the path such as `/milk/shm/<m_shmimName>.im.shm`.
     * 
     * \returns 0 on success
     * \returns -1 on error
     */ 
   int imageKey(const std::string & sn /**< [in] the new shared memory image name*/);
   
   /// Get the current shared memory key.
   std::string imageKey();
   
   /// Get the current shared memory name, same as key.
   std::string imageName();
   
   /// Set the default server address or name.
   void imageServer(const std::string & server /**< [in] the new default server */);
   
   /// Set the default server port.
   void imagePort(int port /**< [in] the new default port */);

private:
   
   /// Thread starter, called by imageThreadStart on thread construction.  Calls imageThreadExec.
   static void internal_imageThreadStart( mzmqImage * mi /**< [in] a pointer to an mzmqImage class */);

public:
   
   /// Initializes the image client thread and starts it.
   /**
     * \returns 0 on success
     * \returns -1 on error
     */ 
   int imageThreadStart();

protected:
   
   /// The image thread function, conducts the milkzmq client business logic.
   /**
     * Runs until m_timeToDie is true.
     *  
     */
   void imageThreadExec();

public:
   /// Set the managing processes display timeout, which is only used for F.P.S. calculations
   void timeout(int to /**< [in] the new timeout in milliseconds */);
   
   /// Get the image dimension in the x direction.
   /** Units are pixels.
     *
     * \returns the current value of m_nx;
     */ 
   uint32_t nx();
   
   /// Get the image dimension in the y direction.
   /** Units are pixels.
     *
     * \returns the current value of m_ny;
     */
   uint32_t ny();
   
   /// Get the image acquisition time 
   /** Gets the acquisition time converted to double, giving time since the epoch.
     * 
     * \returns the time the current image was acquired.
     */
   double imageTime();
   
public:
   
   ///Function called by timer expiration.  Points to latest image and updates the FPS.
   int update();

   /// Detach / disconnect from the image source and clean up.  
   /** Primarily used in the event of a SIGSEGV or SIGBUS.
     * This instance will be ready to connect again, and begin monitoring the source for new data.
     */ 
   void detach();
   
   /// Returns `true` if this instance is attached to its stream and has valid data.  `false` otherwise.
   bool valid();
   
protected:
   float (*pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

public:
   
   /// Get the value at pixel n as a float.
   /** The linear index n is determined by n = y * nx() + x where (x,y) is the pixel coordinate.
     *
     * \returns the value of the pixel 
     */   
   float pixel(size_t n);
   
protected:

    /*** Real time frames per second ***/

   double m_imageTime {0}; ///< The current image time.
   
   double m_fpsTime0 {0}; ///< The reference time for calculate f.p.s.
   
   uint64_t m_fpsFrame0 {0}; ///< The reference frame number for calculaiting f.p.s.
   
   float m_fpsEst {0}; ///< The current f.p.s. estimate.
   
   void update_fps(); ///< Update the f.p.s. estimate from the current timestamp and frame numbers.

public:
   
   /// Get the latest estimate of FPS for this image.
   /**
     * \returns the latest FPS estimate.
     */ 
   float fpsEst();
   
};


#endif //rtimv_mzmqImage_hpp
