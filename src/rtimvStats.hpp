
#ifndef rtimv_rtimvStats_hpp
#define rtimv_rtimvStats_hpp


#include <cstdio>
#include <unistd.h>


#include <QDialog>
#include <QThread>

#include "ui_imviewerStats.h"

#include "rtimvBase.hpp"

class rtimvStats;

///Thread class to start the stats thread.
class StatsThread : public QThread
{
   public:
      void run();
      rtimvStats *imvs;
};

/// Class to manage calculating statistics in the designated image region 
class rtimvStats : public QDialog
{
   Q_OBJECT
   
   public:
      rtimvStats( rtimvBase * imv, 
                  QWidget * Parent = nullptr, 
                  Qt::WindowFlags f = Qt::WindowFlags()
                );

      
      ~rtimvStats();
      
   protected:
      
      rtimvBase * m_imv {nullptr};
      
      int statsPause {20};

      QTimer updateTimer; ///< When this times out the GUI is updated.
      int updateTimerTimeout {50};
      
      size_t image_nx {0};
      size_t image_ny {0};
      size_t region_x0 {0};
      size_t region_x1 {0};
      size_t region_y0 {0};
      size_t region_y1 {0};

      float imdata_min {0};
      float imdata_max {0};
      float imdata_mean {0};
      float imdata_median {0};

      int regionChanged {0};
      int regionSizeChanged {0};
      int regionChangedFit {0};
      int statsChanged {0};
      int dieNow {0};

      StatsThread sth;
      
   public:
      void set_imdata();
      void set_imdata(size_t _nx, size_t _ny, size_t x0, size_t x1, size_t y0, size_t y1);

      void stats_thread();
      void calc_stats();

      
   protected slots:
      void updateGUI();
      
   private:
      Ui::statsform ui;

};

#endif //rtimv_rtimvStats_hpp
