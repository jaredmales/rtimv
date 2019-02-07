
#ifndef rtimv_imviewerstats_hpp
#define rtimv_imviewerstats_hpp


#include <cstdio>
#include <unistd.h>


#include <QDialog>
#include <QThread>

#include "ui_imviewerStats.h"

#include "imviewer.hpp"

class imviewerStats;

///Thread class to start the stats thread.
class StatsThread : public QThread
{
   public:
      void run();
      imviewerStats *imvs;
};

/// Class to manage calculating statistics in the designated image region 
class imviewerStats : public QDialog
{
   Q_OBJECT
   
   public:
      imviewerStats(float (*pg)(void *, size_t), size_t typeSize, QWidget * Parent = 0, Qt::WindowFlags f = 0);
      ~imviewerStats();
      
   protected:
      int statsPause;

      QTimer updateTimer; ///< When this times out the GUI is updated.
      int updateTimerTimeout;
      
      char * imdata;
      char * darkdata;
      char * dummydark;

      int IMDATA_TYPE;
      size_t IMDATA_TYPE_SIZE;
      float (*pixget)(void *, size_t);
            
      float frame_time;
      size_t image_nx;
      size_t image_ny;
      size_t region_x0;
      size_t region_x1;
      size_t region_y0;
      size_t region_y1;

      float imdata_min;
      float imdata_max;
      float imdata_mean;
      float imdata_median;

      int regionChanged;
      int regionSizeChanged;
      int regionChangedFit;
      int statsChanged;
      int dieNow;

      StatsThread sth;
      
   public:
      void set_imdata(void * id, float ft, void * dd = 0);
      void set_imdata(void * id, float ft, size_t _nx, size_t _ny, size_t x0, size_t x1, size_t y0, size_t y1, void * dd = 0);

      void stats_thread();
      void calc_stats();

      
   protected slots:
      void updateGUI();
      
   private:
      Ui::statsform ui;

};

#endif //rtimv_imviewerstats_hpp
