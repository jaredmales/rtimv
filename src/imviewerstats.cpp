
#include "imviewerstats.hpp"

double get_curr_time()
{
   struct timespec tsp;
   clock_gettime(CLOCK_REALTIME, &tsp);
   
   return ((double)tsp.tv_sec) + ((double)tsp.tv_nsec)/1e9;
}

void StatsThread::run()
{
   imvs->stats_thread();
}

float (*global_pixget)(void *, size_t);

imviewerStats::imviewerStats( imviewer * imv,
                              QWidget * Parent, 
                              Qt::WindowFlags f) : QDialog(Parent, f)
{
   ui.setupUi(this);
   
   m_imv = imv;
   
   statsPause = 20;
   updateTimerTimeout = 50;
      
   //Start stats thread here.
   sth.imvs = this;
   sth.start();
   
   connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateGUI()));
   updateTimer.start(updateTimerTimeout);
}

imviewerStats::~imviewerStats()
{
   dieNow = 1;
   sth.wait();
   
}
   
void imviewerStats::set_imdata()
{
   regionChanged = 1;
}

void imviewerStats::set_imdata(size_t nx, size_t ny, size_t x0, size_t x1, size_t y0, size_t y1)
{
   image_nx = nx;
   image_ny = ny;
   region_x0 = x0;
   region_x1 = x1;
   region_y0 = y0;
   region_y1 = y1;
   
   regionChanged = 1;
   regionSizeChanged = 1;
}

void imviewerStats::stats_thread()
{
   while(!dieNow)
   {
      usleep(statsPause*1000);
      calc_stats();
   }
}

void imviewerStats::calc_stats()
{
   if(!m_imv) return;
   
   
   size_t idx;
   
   size_t nx = image_nx;
   size_t x0 = region_x0;
   size_t x1 = region_x1;
   size_t y0 = region_y0;
   size_t y1 = region_y1;
   
   if(!regionChanged) return;

   float (*_pixel)(imviewer*, size_t) = m_imv->pixel();

   float tmp_min, tmp_max;
   float tmp_mean = 0;
   
   idx = y0*nx + x0;
   float imval = _pixel(m_imv, idx);
   
   tmp_min = imval;//id[y0*nx + x0] - dd[y0*nx + x0];
   tmp_max = imval;//id[y0*nx + x0] - dd[y0*nx + x0];
   
   for(size_t i=x0;i<x1;i++)
   {
      for(size_t j=y0;j<y1;j++)
      {
         idx = j*nx + i;
         imval = _pixel(m_imv, idx);
   
         tmp_mean += imval;
         if(imval < tmp_min) tmp_min = imval;
         if(imval > tmp_max) tmp_max = imval;
      }
   }
   
   imdata_min = tmp_min;
   imdata_max = tmp_max;
   imdata_mean = tmp_mean / ((x1-x0)*(y1-y0));
   
   statsChanged = 1;
   regionChanged = 0;
   
}


   
void imviewerStats::updateGUI()
{
   char txt[50];

   if(statsChanged)
   {
      snprintf(txt, 50, "%0.1f", (float) imdata_min);
      ui.dataMin->setText(txt);

      snprintf(txt, 50, "%0.1f", (float) imdata_max);
      ui.dataMax->setText(txt);

      snprintf(txt, 50, "%0.1f", (float) imdata_mean);
      ui.dataMean->setText(txt);

//       snprintf(txt, 50, "%0.1f", (float) imdata_median);
//       ui.dataMedian->setText(txt);
      
      statsChanged = 0;
   }


}   

