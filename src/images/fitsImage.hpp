/** \file fitsImage.hpp
  * \brief Declarations for the rtimvImage FITS management class fitsImage
  *
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef rtimv_fitsImage_hpp
#define rtimv_fitsImage_hpp

#include <fitsio.h>
#include <filesystem>

#include "../rtimvImage.hpp"

#include "pixaccess.hpp"

/// FITS file interface
/** This image class reads a FITS file from the path provided in the image key.  The file is monitored for updates
  * and automatically reloaded.
  */ 
struct fitsImage : public rtimvImage
{
   
   Q_OBJECT

protected:
   std::string m_imagePath; ///< The path to the fits image containing the image data.
   
   bool m_reported {false}; ///< Switch to provide reporting errors only once.

   std::filesystem::file_time_type m_lastWriteTime; ///< The last write time of the file.  Used for tracking changes.
   
   int m_imageTimeout {1000}; ///<The timeout for checking for shared memory file existence.
   
   int m_timeout {100}; ///< The image display timeout, should be set from the managing process.  Only used for F.P.S. calculations.
   
   uint32_t m_nx {0}; ///< Size of the image in pixels in the x direction (horizontal on screen)
   
   uint32_t m_ny {0}; ///< Size of the image in pixels in the y direction (vertical on screen)
   
   char * m_data {nullptr}; ///< Pointer to the image data
 
   QTimer m_timer; ///< When this times out we check for a new image.
   
   bool m_imageFound {false}; ///< Flag denoting whether or not the image exists at the path.
   bool m_imageUpdated {false}; ///< Flag denoting whether the image update has been reported on a call to update().
   
   int m_fps_counter {0};
   int m_age_counter {0};
   
public:
   
   fitsImage() = delete;
   
   ///Only c'tor
   fitsImage(std::mutex * mut);

   /// Set the image key to a path to a fits file.
   /**  
     * \returns 0 on success
     * \returns -1 on error
     */ 
   int imageKey(const std::string & sn /**< [in] the new shared memory image name*/);
   
   /// Get the curent image key, the full path the file.
   std::string imageKey();
   
   /// Get the image name derived from the key.
   /**
     * The name will be the file name stripped of the path and extension
     *
     * \returns the image name
     */ 
   std::string imageName();
   
   /// Set the image-checking timeout
   void imageTimeout(int st /**< [in] the new timeout in milliseconds */);
   
   /// Get the shared-memory timeout
   int imageTimeout();
   
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
   void imageTimerout();
   
signals:
   void connected();
   
public:
   
   /// Open the FITS file and read the image.
   /**
     * \returns 0 on success
     * \returns -1 on error
     */ 
   int readImage();

   /// Attempt to open the FITS file and enter the connected state
   /**
     */  
   void imConnect();
   
   /// Function called by timer expiration.  Points to latest image and updates the FPS.
   int update();

   /// Cleanup image memory and enter the disconnected state.
   void detach();
   
   /// Returns `true` if this instance is attached to its stream and has valid data.  `false` otherwise.
   bool valid();

protected:   

   /*** Real time frames per second ***/

   double m_lastImageTime {0};
   
   double m_fpsTime {0}; ///< The current image time.
   
   double m_fpsTime0 {0}; ///< The reference time for calculate f.p.s.
   
   uint64_t m_fpsFrame0 {0}; ///< The reference frame number for calculaiting f.p.s.
   
   float m_fpsEst {0}; ///< The current f.p.s. estimate.

public:


   float fpsEst()
   {
      return m_fpsEst;
   }
   
   void update_fps(); ///< Update the f.p.s. estimate from the current timestamp and frame numbers.

   float (*pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

public:
   float pixel(size_t n);

   virtual std::vector<std::string> info();
};


#endif //rtimv_fitsImage_hpp
