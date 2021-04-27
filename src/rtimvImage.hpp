/** \file rtimvImage.hpp
  * \brief Declarations for the rtimvImage virtual interface
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

/// Base class for rtimv images 
/** Defines the interface to an image stream to be displayed by rtimv.
  *
  */ 
struct rtimvImage : public QObject
{
   
    
public:
   
   /// Set the image key
   /** This function sets the image key, and must begin the connection and reading process.
     * 
     * \returns 0 on success
     * \returns -1 on error
     */ 
   virtual int imageKey(const std::string & sn /**< [in] the new share memory image name*/) = 0;
   
   /// Get the current image key.
   /**
     * \returns the image key.
     */ 
   virtual std::string imageKey() = 0;
   
   /// Set the managing processes display timeout, which is only used for F.P.S. calculations
   virtual void timeout(int to /**< [in] the new timeout in milliseconds */) = 0;
   
   /// Get the image dimension in the x direction.
   /** Units are pixels.
     *
     * \returns the current x dimension
     */ 
   virtual uint32_t nx() = 0;
   
   /// Get the image dimension in the y direction.
   /** Units are pixels.
     *
     * \returns the current y dimension;
     */
   virtual uint32_t ny() = 0;
   
   /// Get the image acquisition time 
   /** Gets the acquisition time converted to double, giving time since the epoch.
     * 
     * \returns the time the current image was acquired.
     */
   virtual double imageTime() = 0;
   
   /// Function called at intervals to check for updated image data.
   /**
     * \returns RTIMVIMAGE_NOUPDATE if there no updates
     * \returns RTIMVIMAGE_AGEUPDATE if the image age has been updated but the data has not.
     * \returns RTIMVIMAGE_IMUPDATE if the image data has updated
     * \returns RTIMVIMAGE_FPSUPDATE if there is an F.P.S. update along with an image data update
     */
   virtual int update() = 0;

   /// Detach / disconnect from the image source and clean up.  
   /**
     * This instance must be ready to connect again, and begin monitoring the source for new data.
     */ 
   virtual void detach() = 0;
   
   /// Check whether image is connected and has valid data
   /**
     * \returns `true` if this instance is attached to its source and has valid data.  `false` otherwise.
     */
   virtual bool valid() = 0;
   
   /// Get the value at pixel n as a float.
   /** The linear index n is determined by n = y * nx() + x where (x,y) is the pixel coordinate.
     *
     * \returns the value of the pixel 
     */   
   virtual float pixel(size_t n) = 0;
   
   /// Get the latest estimate of FPS for this image.
   /**
     * \returns the latest FPS estimate.
     */ 
   virtual float fpsEst() = 0;

};


#endif //rtimv_rtimvImage_hpp
