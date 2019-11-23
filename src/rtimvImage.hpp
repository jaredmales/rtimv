/** \file rtimvImage.hpp
  * \brief Declarations for the rtimvImage shared memory management class
  *
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef rtimv_rtimvImage_hpp
#define rtimv_rtimvImage_hpp

#include <iostream>

#include <QObject>

#include <QTimer>

#include <fitsio.h>
#include <ImageStreamIO.h>

#include "pixaccess.h"

#define RTIMVIMAGE_NOUPDATE (0)
#define RTIMVIMAGE_AGEUPDATE (1)
#define RTIMVIMAGE_IMUPDATE (2)
#define RTIMVIMAGE_FPSUPDATE (3)

struct rtimvImage : public QObject
{
   Q_OBJECT
 
public:
   
protected:
   std::string m_shmimName; ///< The path to the shared memory buffer containing the image data.

   bool m_isStatic {false}; ///< Flag set if the image is static, e.g. if it's loaded from a FITS file

   bool m_staticUpdated {false}; ///< Flag used so that a static images is updated once and only once.
   
   int m_shmimTimeout {1000}; ///<The timeout for checking for shared memory file existence.
   
   int m_timeout {100}; ///< The image display timeout, should be set from the managing process.  Only used for F.P.S. calculations.
   
public:
   IMAGE m_image; ///< A real-time image structure which contains the image data and meta-data.

   uint32_t m_nx {0}; ///< Size of the image in pixels in the x direction (horizontal on screen)
   
   uint32_t m_ny {0}; ///< Size of the image in pixels in the y direction (vertical on screen)
   
   size_t m_typeSize; ///< The size, in bytes, of the image data type
 
   char * m_data {nullptr}; ///< Pointer to the image data
 
   QTimer m_timer; ///< When this times out imviewer checks for a new image.
   
   
   
   bool m_shmimAttached {false}; ///< Flag denoting whether or not the shared memory is attached.

   uint64_t lastCnt0 {std::numeric_limits<uint64_t>::max()}; //make huge so we don't skip the 0 image
   
   int fps_counter {0};
   int age_counter {0};
   
public:
   
   ///Default c'tor
   rtimvImage();

   ///Set the shared memory image name 
   /** If this contains the string ".fits" then it is treated as a FITS file and loaded as a static image.  Otherwise
     * it is treated as an ImageStreamIO image stream, and added to the path such as `/milk/shm/<m_shmimName>.im.shm`.
     * 
     * \returns 0 on success
     * \returns -1 on error
     */ 
   int shmimName(const std::string & sn /**< [in] the new share memory image name*/);
   
   /// Get the current share memory name
   std::string shmimName();
   
   /// Get the current value of the the m_isStatic flag.
   bool isStatic();
   
   /// Set the shared-memory timeout
   void shmimTimeout(int st /**< [in] the new timeout in milliseconds */);
   
   /// Get the shared-memory timeout
   int shmimTimeout();
   
   /// Set the managing processes display timeout, which is only used for F.P.S. calculations
   void timeout(int to /**< [in] the new timeout in milliseconds */);
   
public slots:
   void shmimTimerout();
   
signals:
   void connected();
   
public:
   
   virtual void imConnect();

   void imConnectStatic();
   
   ///Function called by timer expiration.  Points to latest image and updates the FPS.
   int update();

   void detach();
   
   /// Returns `true` if this instance is attached to its stream and has valid data.  `false` otherwise.
   bool valid();
   
public:

    /*** Real time frames per second ***/

   double m_fpsTime {0}; ///< The current image time.
   
   double m_fpsTime0 {0}; ///< The reference time for calculate f.p.s.
   
   uint64_t m_fpsFrame0 {0}; ///< The reference frame number for calculaiting f.p.s.
   
   float m_fpsEst {0}; ///< The current f.p.s. estimate.

   void update_fps(); ///< Update the f.p.s. estimate from the current timestamp and frame numbers.

   float (*pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

public:
   float pixel(size_t n);
};


#endif //rtimv_rtimvImage_hpp
