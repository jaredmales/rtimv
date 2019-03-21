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

#include "rtimvImage.hpp"

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

class imviewer : public QWidget
{
   Q_OBJECT

public:

   imviewer( const std::vector<std::string> & shkeys, 
             QWidget * Parent = 0, 
             Qt::WindowFlags f = 0
           );

   /*** Image Data ***/
protected:
   uint32_t m_nx {0}; ///< The number of pixels in the x (horizontal) direction
   uint32_t m_ny {0}; ///< The number of pixels in the y (vertical) direction

   std::vector<rtimvImage *> m_images;
      
    /***** Image Data Management *****/

   
   QTimer m_timer; ///< When this times out imviewer checks for a new image.
   
   int m_timeout {100}; ///<The timeout for checking for a new images, ms.

public:
   /// Set the image display timeout.
   void timeout(int);
      
protected slots:
   
   void _timerout();
   
public:   
   //memory management
   
   void setImsize(uint32_t x, uint32_t y); ///Changes the image size, but only if necessary.
   
   virtual void postSetImsize(); ///<to call after set_imsize to handle allocations for derived classes
      
   ///Get the number of x pixels
   float getNx(){return m_nx;}

   ///Get the number of y pixels
   float getNy(){return m_ny;}
   
protected:
   bool amChangingimdata;
   
public:

   virtual void postChangeImdata(); ///<to call after change imdata does its work.
          
   
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
      float sat_level {1e30};
      int saturated {0};

   /*** Color Map ***/
   protected:
      float mindat;
      
      float maxdat;
      
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

      virtual void updateFPS();///<Called whenever the displayed image updates its FPS.
      virtual void updateAge();///<Called whenever the displayed image updates its Age.

};

#endif //rtimv_imviewer_hpp
