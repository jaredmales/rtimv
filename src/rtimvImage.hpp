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
   
   ///Set the shared memory image name 
   /** If this contains the string ".fits" then it is treated as a FITS file and loaded as a static image.  Otherwise
     * it is treated as an ImageStreamIO image stream, and added to the path such as `/milk/shm/<m_shmimName>.im.shm`.
     * 
     * \returns 0 on success
     * \returns -1 on error
     */ 
   virtual int shmimName(const std::string & sn /**< [in] the new share memory image name*/) = 0;
   
   /// Get the current share memory name
   virtual std::string shmimName() = 0;
   
   /// Set the managing processes display timeout, which is only used for F.P.S. calculations
   virtual void timeout(int to /**< [in] the new timeout in milliseconds */) = 0;
   
   /// Get the image dimension in the x direction.
   /** Units are pixels.
     *
     * \returns the current value of m_nx;
     */ 
   virtual uint32_t nx() = 0;
   
   /// Get the image dimension in the y direction.
   /** Units are pixels.
     *
     * \returns the current value of m_ny;
     */
   virtual uint32_t ny() = 0;
   
   /// Get the image acquisition time 
   /** Gets the acquisition time converted to double, giving time since the epoch.
     * 
     * \returns the time the current image was acquired.
     */
   virtual double imageTime() = 0;
   
//public slots:
 //  virtual void shmimTimerout() = 0;
   
public:
   
   
   ///Function called by timer expiration.  Points to latest image and updates the FPS.
   virtual int update() = 0;

   virtual void detach() = 0;
   
   /// Returns `true` if this instance is attached to its stream and has valid data.  `false` otherwise.
   virtual bool valid() = 0;
   
   virtual float fpsEst() = 0;
   
   virtual float pixel(size_t n) = 0;
};


#endif //rtimv_rtimvImage_hpp
