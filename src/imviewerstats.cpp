
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

imviewerStats::imviewerStats(float (*pg)(void *, size_t), size_t typeSize, QWidget * Parent, Qt::WindowFlags f) : QDialog(Parent, f)
{
   ui.setupUi(this);
   
   statsPause = 20;
   updateTimerTimeout = 50;
      
   pixget = pg;
   global_pixget = pixget;
   IMDATA_TYPE_SIZE = typeSize;
   
   imdata = 0;
   image_nx = 0;
   image_ny = 0;
   region_x0 = 0;
   region_x1 = 0;
   region_y0 = 0;
   region_y1 = 0;

   regionChanged = 0;
   regionSizeChanged = 0;
   
   imdata_min = 0;
   imdata_max = 0;
   imdata_mean = 0;
   imdata_median = 0;
   statsChanged = 1;

   
   dieNow = 0;
   //Start stats thread here.
   sth.imvs = this;
   sth.start();

   dummydark = new char[1024*1024*IMDATA_TYPE_SIZE];
   memset(dummydark, 0, 1024*1024*IMDATA_TYPE_SIZE);
   
   
   connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateGUI()));
   updateTimer.start(updateTimerTimeout);
}

imviewerStats::~imviewerStats()
{
   dieNow = 1;
   sth.wait();
   
   delete dummydark;
}
   
void imviewerStats::set_imdata(void * id, float ft, void * dd)
{
   
   imdata = (char *) id;
   darkdata = (char *) dd;
   
   frame_time = ft;
   
   regionChanged = 1;
}


void imviewerStats::set_imdata(void * id, float ft, size_t nx, size_t ny, size_t x0, size_t x1, size_t y0, size_t y1, void * dd)
{
   
   imdata = (char *) id;
   darkdata = (char *) dd;
   
   frame_time = ft;
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
   size_t idx;
   float imval, darkval;
   
   char * id = imdata;
   char * dd;
   
   if(darkdata) dd = darkdata;
   else dd = dummydark;
   
   size_t nx = image_nx;
   size_t x0 = region_x0;
   size_t x1 = region_x1;
   size_t y0 = region_y0;
   size_t y1 = region_y1;
   
      
   if(!id) return;
   if(!regionChanged) return;
   
   float tmp_min, tmp_max;
   float tmp_mean = 0;
   
   idx = y0*nx + x0;
   imval = pixget(id, idx);
   darkval = pixget(dd, idx);
   
   tmp_min = imval - darkval;//id[y0*nx + x0] - dd[y0*nx + x0];
   tmp_max = imval - darkval;//id[y0*nx + x0] - dd[y0*nx + x0];
   
   for(size_t i=x0;i<x1;i++)
   {
      for(size_t j=y0;j<y1;j++)
      {
         idx = j*nx + i;
         imval = pixget(id, idx);
         darkval = pixget(dd, idx);
   
         tmp_mean += imval-darkval;//id[idx] - dd[y0*nx + x0];
         if(imval-darkval < tmp_min) tmp_min = imval-darkval;
         if(imval-darkval > tmp_max) tmp_max = imval-darkval;
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

