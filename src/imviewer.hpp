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
      int nx {0}; ///< The number of pixels in the x (horizontal) direction
      int ny {0}; ///< The number of pixels in the y (vertical) direction
      
      float frame_time {0}; ///<The timestamp of the current frame
      
      char * imdata {nullptr}; ///< Pointer to the image data
      
      char * tmpim {nullptr}; ///<  A temporary image storage
      
      bool localImdata {false}; ///< flag to determine if imdata is locally allocated

      imviewer_shmt shmem_key; ///< The path to the shared memory buffer containing the image data.

      IMAGE image; ///< A real-time image structure which contains the image data and meta-data.
      
      int semaphoreNumber {0}; ///< Number of the semaphore to monitor for new image data.  This determines the filename.
      
      sem_t * sem {nullptr}; ///< The semaphore to monitor for new image data
      
      float (*pixget)(void *, size_t) {nullptr}; ///< Pointer to a function to extract the image data as a float.
     
      size_t type_size; ///< The size, in bytes, of the image data type
      
      QImage * qim {nullptr}; ///<A QT image, used to store the color-map encoded data
      QPixmap qpm; ///<A QT pixmap, used to prep the QImage for display.
      
      /** @name A User Defined Region
       */
      //@{
      int userBoxActive {0};
      
      int userBox_i0;
      int userBox_i1;
      int userBox_j0;
      int userBox_j1;
      
      int guideBox_i0;
      int guideBox_i1;
      int guideBox_j0;
      int guideBox_j1;
      
      float userBox_max;
      float userBox_min;

      
   public:
      int getUserBoxActive(){ return userBoxActive; }
      void setUserBoxActive(bool usba);
      //virtual void post_setUserBoxActive(){ return; }
      //@}
      
   public:
      ///Get the number of x pixels
      float getNx(){return nx;}

      ///Get the number of y pixels
      float getNy(){return ny;}

      ///Get the QPixMap pointer
      QPixmap * getPixmap(){return &qpm;}
      
      void allocImdata(int x, int y);
      void setImsize(int x, int y); ///Changes the image size, but only if necessary.
      virtual void postSetImsize(); ///<to call after set_imsize to handle allocations for derived classes
      
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
      bool amChangingimdata;
   public:
      
      virtual void postChangeImdata(); ///<to call after change imdata does its work.
      
      void point_imdata(void * imd); ///<Points imdata at a new array, no copying is done.
      void point_imdata(int x, int y, void * imd); ///<Points imdata at a new array, changing imsize if necessary, no copying is done.
      
      char * get_imdata(){return imdata;} ///<Returns the imdata pointer.
      
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
      
   /***** Real Time *****/
   protected:
      bool RealTimeEnabled {true}; ///<Controls whether imviewer is using real-time data.
      bool RealTimeStopped {false}; ///<Set when user temporarily freezes real-time data viewing.
      
      
      
      
      QTimer imtimer; ///< When this times out imviewer checks for a new image.
      int imtimer_timeout {20}; ///<The timeout for checking for a new images, ms.
      int shmem_attached {false}; ///< Flag denoting whether or not the shared memory is attached.
      
      int curr_saved {0};
      int last_saved {-1};
      
   /*** Real Time Controls ***/
   public:
      void set_RealTimeEnabled(int);
      void set_RealTimeStopped(int);
      void set_RealTimeProtocol(int);
      void set_imtimer_timeout(int);
      
   protected slots:
      void _shmem_timerout();
      void _timerout();

   protected:
      virtual void shmem_timerout();
      
      ///Function called by timer expiration.  Checks semaphore and updates the FPS.
      virtual void timerout();
      
   /*** Real time frames per second ***/
   protected:
      int i_fps {-1};
      unsigned n_ave_fps {20};
      float fps_sum;
      float fps_ave;
      double fps_time0;
      std::vector<float> fps_hist;
      void update_fps(bool NoAdvance = false);

      virtual void update_age();///<Called every timeout no matter what, to update the image age.
      
};

#endif //rtimv_imviewer_hpp



