/** \file rtimvBase.hpp
  * \brief Declarations for the rtimvBase base class
  *
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef rtimv_rtimvBase_hpp
#define rtimv_rtimvBase_hpp

#include <cmath>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <sys/socket.h>

#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include <QSocketNotifier>

#include "rtimvImage.hpp"

#include <cstdio>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>

#include <signal.h>

#include "colorMaps.hpp"



class rtimvBase : public QWidget
{
    Q_OBJECT

public:

    /// Basic c'tor.  Does not startup the images.
    /** startup should be called with the list of keys.
      */
    rtimvBase( QWidget * Parent = nullptr, 
               Qt::WindowFlags f = Qt::WindowFlags()
             );
   
    /// Image c'tor, starts up the images.
    /** startup should not be called.
      */
    rtimvBase( const std::vector<std::string> & shkeys,  ///< [in] The shmim keys used ot access the images.
               QWidget * Parent = nullptr, 
               Qt::WindowFlags f = Qt::WindowFlags()
             );

protected:
   
    /** @name Connection - Data 
      * 
      * @{
      */

    /// The images to be displayed
    /** By index:
      * - 0 is the main image.
      * - 1 is the dark image which is (optionally) subtracted from the main image.
      * - 2 is the mask image which is (optionally) multiplied by the dark-subtracted image.  Normally a 1/0 image.
      * - 3 is the saturation mask which (optionally) denotes which pixels to turn the saturation color.
      */
    std::vector<rtimvImage *> m_images; 

    /// Flag to indicate that the milkzmq protocol should be used for all ImageStremIO images
    bool m_mzmqAlways {false};

    /// Default milkzmq server to use, if set this overrides default "localhost" in \ref mzmqImage
    std::string m_mzmqServer;
    
    /// Default milkzmq server to use, if set this overrides default "5556" in \ref mzmqImage
    int m_mzmqPort {0};

    ///@}

public:
   
    /** @name Connection 
      * 
      * @{
      */

    /// Configure the image sources and start checking for updates.
    /** 
      */
    void startup( const std::vector<std::string> & shkeys /**< [in] The keys used to access the images (see \ref m_images)*/);

    /// Function called on connection
    virtual void onConnect() {}

    /// Check if an image is currently valid.
    /** An image is valid if it was supplied on command line, and if the image itself returns true from valid().
      * 
      * \returns true if valid
      * \returns false otherwise 
      */
    bool imageValid( size_t n /**< [in] the image number */);

    ///@}


    /** @name Image Size - Data
      *    
      * @{
      */ 
protected:

    uint32_t m_nx {0}; ///< The number of pixels in the x (horizontal) direction

    uint32_t m_ny {0}; ///< The number of pixels in the y (vertical) direction

    ///@}

public: 

    /** @name Image Size
      *    
      * @{
      */ 

    /// Changes the image size, but only if necessary.
    /** The reallocates m_calData and m_qim
      * 
      */
    void setImsize( uint32_t x, ///< [in] the new x size
                    uint32_t y  ///< [in] the new y size
                  ); 
   
    /// Called after set_imsize to handle allocations for derived classes
    virtual void postSetImsize(); 
      
    ///Get the number of x pixels
    /**
      * \returns the current vvalue of m_nx
      */ 
    uint32_t nx();

    ///Get the number of y pixels
    /**
      * \returns the current vvalue of m_ny
      */
    uint32_t ny();
   
    /// @}


    /** @name Image Update - Data
      *    
      * @{
      */       

protected:
   
    QTimer m_imageTimer; ///< When this times out rtimvBase checks for a new image.
   
    int m_imageTimeout {50}; ///<The timeout for checking for a new images, ms.

    ///@}

    /** @name Image Update - Slots
      *    
      * @{
      */
protected slots:
   
    /// Function called by m_imageTimer expiration.
    /** This is what actually checks for image data updates.
      */
    void updateImages();

    ///@}

    /** @name Image Update
      *    
      * @{
      */ 

public:
   
    /// Set the image display timeout.
    /** This sets the display frame rate, e.g. a timeout of 100 msec will 
      * cause the display to update at 10 f.p.s.
      */ 
    void imageTimeout(int to /**< [in] the new image display timeout*/);

    /// Get the image display timeout.
    /** 
      * \returns the current value of m_imageTimeout 
      */
    int imageTimeout();
   
   /// @}

   /** \name Calibrated Pixel Data
     * 
     * Settings to control which calibrations are applied, and manage access to memory.
     * 
     * @{
     */ 

protected:   

    /// Whether or not the dark image is subtracted, default is false.
    bool m_subtractDark {false};

    /// Whether or not the mask is applied, default is false.
    bool m_applyMask {false};
   
    /// Whether or not the saturation mask is applied, default is false.  
    /** Note this only controls whether the pixels are colored m_satColor.  It does
      * not change the values returned by rawPixel().
      */  
    bool m_applySatMask {false};

    float * m_calData {nullptr};
    uint8_t * m_satData {nullptr};

    ///Mutex for locking access to raw pixels
    /** This is used by rtimvImage derived classes to protect
      * deletion and recreation of the m_data array they manage.
      * 
      */
    std::mutex m_rawMutex;

    ///Mutex for locking access to calibrated pixels
    std::mutex m_calMutex;
    
    ///@}

   /** \name Calibrated Pixel Access
     * 
     * Functions to manage which calibrations are applied and provide acccess to calibrated pixels.
     * 
     * Calibrations include dark subtraction, reference subtraction, flat field, mask, and low and high pass filtering.
     * Note: only dark subtraction and masking are currently implemented.
     * 
     * The pixelF function pointer is used so that only a single `if-else` tree needs to be evaluated before 
     * iterating over all pixels.  The rawPixel() function returns a pointer to the static pixel_* function appropriate
     * for the current calibration settings.
     * 
     * @{
     */ 
public:
    /// The fuction pointer type for accessing pixels with calibrations applied.
    typedef float (*pixelF)(rtimvBase*, size_t);

    /// Returns a pointer to the static pixel value calculation function for the current calibration configuration
    /** Calibration configuration includes the value of m_subtractDark, m_applyMask.
      *
      * \returns a pointer to one of pixel_noCal, pixel_subDark, pixel_applyMask, pixel_subDarkApplyMask.
      */
    pixelF rawPixel();
  
    /// Access pixels with no calibrations applied.
    /** 
      * \returns the value of pixel idx
      */ 
    static float pixel_noCal( rtimvBase * imv, ///< [in] the rtimvBase instance to access
                              size_t idx      ///< [in] the linear pixel number to access
                            );
   
    /// Access pixels with dark subtraction applied.
    /** 
      * \returns the value of pixel idx after dark subtraction
      */
    static float pixel_subDark( rtimvBase * imv, ///< [in] the rtimvBase instance to access 
                                size_t idx      ///< [in] the linear pixel number to access
                              );
   
    /// Access pixels with the mask applied.
    /** 
      * \returns the value of pixel idx after applying the mask
      */
    static float pixel_applyMask( rtimvBase * imv, ///< [in] the rtimvBase instance to access
                                  size_t idx      ///< [in] the linear pixel number to access
                                );
   
    /// Access pixels with dark subtraction and masking applied.
    /** 
      * \returns the value of pixel idx after subtracting the dark and applying the mask
      */
     static float pixel_subDarkApplyMask( rtimvBase * imv, ///< [in] the rtimvBase instance to access 
                                          size_t idx      ///< [in] the linear pixel number to access
                                        );
   
    float calPixel( uint32_t x,
                    uint32_t y
                  );

    uint8_t satPixel( uint32_t x,
                      uint32_t y
                    );

    ///@}
   
    /** @name Colorbar Selection
      * 
      * @{
      */

public:
    typedef int (*pixelIndexF)(float);
   
   enum en_cbStretches{ stretchLinear,  ///< The pixel values are scaled linearly to between m_mindat and m_maxdat 
                        stretchLog,     ///< The pixel values are scaled logarithmically between m_mindat and m_maxdat 
                        stretchPow,     ///< the pixel values are scaled as \f$ 1000^p/1000 \f$ between m_mindat and m_maxdat
                        stretchSqrt,    ///< the pixel values are scaled as \f$ \sqrt(p) \f$ between m_mindat and m_maxdat
                        stretchSquare,  ///< the pixel values are scaled as \f$ p^2 \f$ between m_mindat and m_maxdat
                        cbStretches_max
                      };
   
protected:
   int m_minColor {0}; ///< The minimum index to use for the color table.
   
   int m_maxColor {253}; ///< The maximum index to use for the color table.
   
   int m_maskColor {254}; ///< The index in the color table to use for the mask color.
   
   int m_satColor {255}; ///< The index in the color table to use for the saturation color.

   int m_nanColor {254}; ///< The index in the color table to use for nans and infinities.

   int colorbar_mode {minmaxglobal};
   int m_cbStretch {stretchLinear};

   int current_colorbar {colorbarBone};

   QColor warning_color;
   
   

public:

   enum colorbars{colorbarGrey, colorbarJet, colorbarHot, colorbarBone, colorbarRed, colorbarGreen, colorbarBlue, colorbarMax};
   
   void load_colorbar( int cb,
                       bool update = true
                     );
   
   int get_current_colorbar(){return current_colorbar;}

   enum colorbar_modes{minmaxglobal, minmaxview, minmaxbox, user, colorbar_modes_max};
   void set_colorbar_mode(int mode){ colorbar_mode = mode;}
   int get_colorbar_mode(){return colorbar_mode;}

   
   void set_cbStretch(int);
   int get_cbStretch();

   pixelIndexF pixelIndex();
   
   static int pixelIndex_linear(float d);
   
   static int pixelIndex_log(float d);
   
   static int pixelIndex_pow(float d);
   
   static int pixelIndex_sqrt(float d);
   
   static int pixelIndex_square(float d);
   
   ///@}
   
   /** @name Colorbar Scale Control
     * 
     * @{
     */

protected:
   /*** Color Map ***/
   float m_mindat;  ///< The minimum data value used for scaling
      
   float m_maxdat; ///< The maximum data valuse used for scaling

   bool m_autoScale {false};

public:

   
   void mindat(float md);
      
   float mindat();
      
   void maxdat(float md);
      
   float maxdat();
   
   void bias(float b);

   float bias();
   
   void bias_rel(float b);

   float bias_rel();
   
   void contrast(float c);

   float contrast();
   
   void contrast_rel(float cr);
   
   float contrast_rel();

   
   ///@}
   
   /** @name Image Filtering
     * 
     * @{
     */

      float * m_lowPassFiltered {nullptr};
      
      bool m_applyLPFilter;
      
      int m_lpFilterType;
   
   ///@} -- filtering
   
   //****** The display *************
protected:
      
      
   QImage * m_qim {nullptr}; ///<A QT image, used to store the color-map encoded data

   std::vector<double> m_lightness; ///< The perceived lightness values of the colormap RGB values
   
   QPixmap m_qpm; ///<A QT pixmap, used to prep the QImage for display.

    ///Flag indicating that changeImdata(bool) is currently executing
    bool m_amChangingimdata {false};
   
public:
      
    ///Get the QPixMap pointer
    QPixmap * getPixmap(){return &m_qpm;}

    ///Updates the QImage and basic statistics after a new image.
    /** \param newdata determines whether statistics are calculated (true) or not (false).
      */
    void changeImdata(bool newdata = false);

    virtual void postChangeImdata(); ///<to call after change imdata does its work.

protected:
    float m_satLevel {1e30};
    uint32_t m_saturated {0};


   /* Image Stats */
   protected:
      float imdat_min;
      float imdat_max;

   public:
      float get_imdat_min(){return imdat_min;}
      float get_imdat_max(){return imdat_max;}

   /*** Abstract Zoom ***/
   protected:
      float m_zoomLevel {1};
      float m_zoomLevelMin {1};
      float m_zoomLevelMax {64};

   public:
      float zoomLevel(){return m_zoomLevel;}
      float zoomLevelMin(){return m_zoomLevelMin;}
      float zoomLevelMax(){return m_zoomLevelMax;}

      //void set_ZoomLevel(int zlint);
      void zoomLevel(float zl);
      virtual void post_zoomLevel();

  
   /** @name A User Defined Region
     *
     * @{
     */
protected:
   int colorBoxActive {0};

   //ImageStreamIO images are sized in uint32_t, so we need these big enough for signed comparisons without wraparound
   int64_t colorBox_i0;
   int64_t colorBox_i1;
   int64_t colorBox_j0;
   int64_t colorBox_j1;

   int64_t guideBox_i0;
   int64_t guideBox_i1;
   int64_t guideBox_j0;
   int64_t guideBox_j1;

   float colorBox_max;
   float colorBox_min;

public:
   int getUserBoxActive(){ return colorBoxActive; }
   
   void setUserBoxActive(bool usba);
   
   ///@}


   /*** Real Time Controls ***/   
protected:
      bool RealTimeEnabled {true}; ///<Controls whether rtimvBase is using real-time data.
      bool RealTimeStopped {false}; ///<Set when user temporarily freezes real-time data viewing.

public:
      void set_RealTimeEnabled(int);
      void set_RealTimeStopped(int);

      virtual void updateFPS();///<Called whenever the displayed image updates its FPS.
      virtual void updateAge();///<Called whenever the displayed image updates its Age.
      virtual void updateNC(); ///<Update the display while not connected.

      
   
};

#endif //rtimv_rtimvBase_hpp
