/** \file shmimImage.hpp
  * \brief Declarations for the shmimImage shared memory management class
  *
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef rtimv_shmimImage_hpp
#define rtimv_shmimImage_hpp

#include <ImageStreamIO.h>

#include "../rtimvImage.hpp"

/// rtimvImage interface to a milk shared memory stream on the local computer.
struct shmimImage : public rtimvImage
{
   
   Q_OBJECT

   
protected:
   std::string m_shmimName; ///< The path to the shared memory buffer containing the image data.

   int m_shmimTimeout {1000}; ///<The timeout for checking for shared memory file existence.
   
   int m_timeout {100}; ///< The image display timeout, should be set from the managing process.  Only used for F.P.S. calculations.
   
   IMAGE m_image; ///< A real-time image structure which contains the image data and meta-data.

   uint32_t m_nx {0}; ///< Size of the image in pixels in the x direction (horizontal on screen)
   
   uint32_t m_ny {0}; ///< Size of the image in pixels in the y direction (vertical on screen)
   
   size_t m_typeSize; ///< The size, in bytes, of the image data type
 
   char * m_data {nullptr}; ///< Pointer to the image data
 
   QTimer m_timer; ///< When this times out imviewer checks for a new image.
   
   bool m_shmimAttached {false}; ///< Flag denoting whether or not the shared memory is attached.

   /// Previous frame number, stored to detect new image
   uint64_t m_lastCnt0 {std::numeric_limits<uint64_t>::max()}; //make huge so we don't skip the 0 image
   
   /// Counts how many times the FPS has been updated
   int m_fps_counter {0};
   
   /// Counts how many times the Age has been updated
   int m_age_counter {0};
   
public:
   
   ///Default c'tor
   shmimImage();

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
   
   /// Set the shared-memory timeout
   void shmimTimeout(int st /**< [in] the new timeout in milliseconds */);
   
   /// Get the shared-memory timeout
   int shmimTimeout();
   
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
   
public slots:
   
   /// Called on m_timer timeout signal. Calls imConnect.
   /** The timer is stopped and imConnect is called.
     * If imConnect is not successful, then the timer is restarted.
     */ 
   void shmimTimerout();
      
public:
   
   /// Connect to the stream.
   /** If successful, m_shmimAttached will be set to true and the image
     * data will begin updating on next call to update(). 
     */
   void imConnect();

   /// Function called by timer expiration.  Points to latest image and updates the FPS.
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


#endif //rtimv_shmimImage_hpp
