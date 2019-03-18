/** \file imviewer.hpp
  * \brief Declarations for the imviewer base class
  *
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  */


#ifndef rtimv_imviewer_hpp
#define rtimv_imviewer_hpp

#include <cmath>
#include <iostream>
#include <vector>
#include <sys/time.h>

#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "pixaccess.h"


//Still needed since we haven't scrubbed all the legacy VisAO code yet...
#define RT_SYSTEM_VISAO 0
#define RT_SYSTEM_SCEXAO 1
#define RT_SYSTEM 1



#include <cstdio>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>

#include <ImageStreamIO.h>

#include "colorMaps.hpp"
#include "ImageStruct.hpp"

typedef std::string imviewer_shmt;


class imviewer : public QWidget
{
   Q_OBJECT

public:

   imviewer(imviewer_shmt shkey, QWidget * Parent = 0, Qt::WindowFlags f = 0);

   /*** Image Data ***/
protected:
   uint32_t m_nx {0}; ///< The number of pixels in the x (horizontal) direction
   uint32_t m_ny {0}; ///< The number of pixels in the y (vertical) direction

   //----------- The image -----------
   float * m_imData {nullptr}; ///< Pointer to the image data

   imviewer_shmt imShmimKey; ///< The path to the shared memory buffer containing the image data.

   IMAGE image; ///< A real-time image structure which contains the image data and meta-data.

   float (*pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

   size_t type_size; ///< The size, in bytes, of the image data type

   //----------- The dark -----------
   
   float * m_darkData {nullptr}; ///< Pointer to the image data

   imviewer_shmt darkShmimKey; ///< The path to the shared memory buffer containing the image data.

   IMAGE darkImage; ///< A real-time image structure which contains the image data and meta-data.

   float (*dark_pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

   size_t dark_type_size; ///< The size, in bytes, of the image data type
   
   //----------- The reference -----------
   
   float * m_refData {nullptr}; ///< Pointer to the image data

   imviewer_shmt refShmimKey; ///< The path to the shared memory buffer containing the image data.

   IMAGE refImage; ///< A real-time image structure which contains the image data and meta-data.

   float (*ref_pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

   size_t ref_type_size; ///< The size, in bytes, of the image data type
   
   //----------- The flat -----------
   
   float * m_flatData {nullptr}; ///< Pointer to the image data

   imviewer_shmt flatShmimKey; ///< The path to the shared memory buffer containing the image data.

   IMAGE flatImage; ///< A real-time image structure which contains the image data and meta-data.

   float (*flat_pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.

   size_t flat_type_size; ///< The size, in bytes, of the image data type
   
    /***** Image Data Management *****/
public:
   //memory management
   
   void allocImdata(uint32_t x, uint32_t y);
   
   void setImsize(uint32_t x, uint32_t y); ///Changes the image size, but only if necessary.
   
   virtual void postSetImsize(); ///<to call after set_imsize to handle allocations for derived classes
      
   ///Get the number of x pixels
   float getNx(){return m_nx;}

   ///Get the number of y pixels
   float getNy(){return m_ny;}
   
protected:
      bool amChangingimdata;
   
public:
      ///< Sets imdata based on the new array, resizing if necessary.
      void setImdata( void * imd ); 

      virtual void postChangeImdata(); ///<to call after change imdata does its work.
       
      //float * get_imdata(){return m_imData;} ///<Returns the imdata pointer.

      
protected:
   QTimer m_imTimer; ///< When this times out imviewer checks for a new image.
   int m_imShmimTimeout {1000}; ///<The timeout for checking for shared memory file existence.
   int m_imTimeout {100}; ///<The timeout for checking for a new images, ms.
   
   int m_imShmimAttached {false}; ///< Flag denoting whether or not the shared memory is attached.

public:

   void imShmimTimeout(int);
   void imTimeout(int);

protected slots:
   void _imShmim_timerout();
   void _imTimerout();

protected:
   virtual void imShmim_timerout();

   ///Function called by timer expiration.  Displays latest image and updates the FPS.
   virtual void imUpdate();
    
   /***** Dark Data Updates *****/
public:
   //memory management
   void allocDarkData(uint32_t x, uint32_t y);
   
   ///< Sets imdata based on the new array, resizing if necessary.
   void setDarkData( void * imd ); 

protected:
   QTimer m_darkTimer; ///< When this times out imviewer checks for a new image.
   int m_darkShmimTimeout {1000}; ///<The timeout for checking for shared memory file existence.
   int m_darkTimeout {100}; ///<The timeout for checking for a new dark image, ms.
   
   int m_darkShmimAttached {false}; ///< Flag denoting whether or not the shared memory is attached.

public:

   void darkShmimTimeout(int);
   void darkTimeout(int);

protected slots:
   void _darkShmim_timerout();
   void _darkTimerout();

protected:
   virtual void darkShmim_timerout();

   ///Function called by timer expiration.  Displays latest image and updates the FPS.
   virtual void darkUpdate();
   
   
   //****** The display *************
protected:
      
      
      QImage * qim {nullptr}; ///<A QT image, used to store the color-map encoded data
      
      QPixmap qpm; ///<A QT pixmap, used to prep the QImage for display.

      /** @name A User Defined Region
       */
      //@{
      int userBoxActive {0};

      //ImageStreamIO images are sized in uint32_t, so we need these big enough for signed comparisons without wraparound
      int64_t userBox_i0;
      int64_t userBox_i1;
      int64_t userBox_j0;
      int64_t userBox_j1;

      int64_t guideBox_i0;
      int64_t guideBox_i1;
      int64_t guideBox_j0;
      int64_t guideBox_j1;

      float userBox_max;
      float userBox_min;


   public:
      int getUserBoxActive(){ return userBoxActive; }
      void setUserBoxActive(bool usba);
      //virtual void post_setUserBoxActive(){ return; }
      //@}

   public:
      
      

      ///Get the QPixMap pointer
      QPixmap * getPixmap(){return &qpm;}


      ///Updates the QImage and the statistics after a new image.
      /** \param newdata determines whether statistics are calculated (true) or not (false).
       */
      void changeImdata(bool newdata = false);

      void changeImdataRecolorOnly();///<Only update the QImage with a new color mapping, does not recalc any statistics.

      void changeImdata_applyDark(bool newdata = false);

      void changeImdataRecolorOnly_applyDark();///<Only update the QImage with a new color mapping, does not recalc any statistics.

      float calcPixval(float d);///<Actually calculates the color mapped value of each pixel from 0 to 255.

      bool applyDark {false};
      bool applyDarkChanged {true};


   protected:
      float sat_level {0};
      int saturated {0};

   /*** Color Map ***/
   protected:
      float mindat;
      float mindatsc;
      float mindat_rel;
      float maxdat;
      float maxdatsc;
      float maxdat_rel;

   public:
      void set_mindat(float md);
      float get_mindat(){return mindat;}
      void set_maxdat(float md);
      float get_maxdat(){return maxdat;}

      void set_bias(float b);
      void set_bias_rel(float b);
      float get_bias(){return 0.5*(maxdat+mindat);}
      float get_bias_rel(){return 0.5*(maxdat+mindat)/(maxdat-mindat);}

      void set_contrast(float c);
      void set_contrast_rel(float cr);
      float get_contrast(){return maxdat-mindat;}
      float get_contrast_rel(){return (imdat_max-imdat_min)/(maxdat-mindat);}

   protected:
      int mincol {0};
      int maxcol {256};

      bool abs_fixed {true};
      bool rel_fixed {false};

      int colorbar_mode {minmaxglobal};
      int colorbar_type {typelinear};

      int current_colorbar {colorbarBone};

      QColor warning_color;

   public:
      bool get_abs_fixed(){return abs_fixed;}
      bool get_rel_fixed(){return rel_fixed;}
      void set_abs_fixed()
      {
         abs_fixed = true;
         rel_fixed = false;
      }
      void set_rel_fixed()
      {
         rel_fixed = true;
         abs_fixed = false;
      }

      enum colorbars{colorbarGrey, colorbarJet, colorbarHot, colorbarBone, colorbarRed, colorbarGreen, colorbarBlue, colorbarMax};
      void load_colorbar(int);
      int get_current_colorbar(){return current_colorbar;}

      enum colorbar_modes{minmaxglobal, minmaxview, minmaxbox, user, colorbar_modes_max};
      void set_colorbar_mode(int mode){ colorbar_mode = mode;}
      int get_colorbar_mode(){return colorbar_mode;}

      enum colorbar_types{typelinear, typelog, typepow, typesqrt, typesquare, colorbar_types_max};
      void set_colorbar_type(int);
      int get_colorbar_type(){return colorbar_type;}

   /* Image Stats */
   protected:
      float imdat_min;
      float imdat_max;

   public:
      float get_imdat_min(){return imdat_min;}
      float get_imdat_max(){return imdat_max;}

   /*** Abstract Zoom ***/
   protected:
      float ZoomLevel {1};
      float ZoomLevel_min {0.125};
      float ZoomLevel_max {64};

   public:
      float get_ZoomLevel(){return ZoomLevel;}
      float get_ZoomLevel_min(){return ZoomLevel_min;}
      float get_ZoomLevel_max(){return ZoomLevel_max;}

      //void set_ZoomLevel(int zlint);
      void set_ZoomLevel(float zl);
      virtual void post_set_ZoomLevel();

  


   /*** Real Time Controls ***/   
   protected:
      bool RealTimeEnabled {true}; ///<Controls whether imviewer is using real-time data.
      bool RealTimeStopped {false}; ///<Set when user temporarily freezes real-time data viewing.

   public:
      void set_RealTimeEnabled(int);
      void set_RealTimeStopped(int);
      //void set_RealTimeProtocol(int);

   /*** Real time frames per second ***/
   protected:
      double m_fpsTime {0}; ///< The current image time.
      double m_fpsTime0 {0}; ///< The reference time for calculate f.p.s.
      
      uint64_t m_fpsFrame0 {0}; ///< The reference frame number for calculaiting f.p.s.
      
      float m_fpsEst {0}; ///< The current f.p.s. estimate.

      void update_fps(); ///< Update the current f.p.s. estimate from the current timestamp and frame numbers.

      virtual void update_age();///<Called every timeout no matter what, to update the image age.

};

#endif //rtimv_imviewer_hpp
