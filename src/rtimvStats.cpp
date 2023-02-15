
#include "rtimvStats.hpp"

double get_curr_time()
{
   struct timespec tsp;
   clock_gettime(CLOCK_REALTIME, &tsp);
   
   return ((double)tsp.tv_sec) + ((double)tsp.tv_nsec)/1e9;
}

void StatsThread::run()
{
   m_imvs->statsThread();
}

//float (*global_pixget)(void *, size_t);

rtimvStats::rtimvStats( rtimvBase * imv,
                        QWidget * Parent, 
                        Qt::WindowFlags f) : QDialog(Parent, f)
{
   ui.setupUi(this);
   
   m_imv = imv;

   m_statsPause = 20;
   m_updateTimerTimeout = 50;
      
   //Start stats thread here.
   m_statsThread.m_imvs = this;
   m_statsThread.start();
   
   connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(updateGUI()));
   m_updateTimer.start(m_updateTimerTimeout);
}

rtimvStats::~rtimvStats()
{
   m_dieNow = 1;
   m_statsThread.wait();
}
   
void rtimvStats::setImdata()
{
   m_regionChanged = 1;
}

void rtimvStats::setImdata(size_t nx, size_t ny, size_t x0, size_t x1, size_t y0, size_t y1)
{

   if(x1 <= x0 || x0 > nx || x1 > nx || y1 <= y0 || y0 > ny || y1 > ny ) 
   {
      m_regionChanged = 0;
      return; //Bad data.
   }

   std::unique_lock<std::mutex> lock(m_dataMutex);

   m_nx = nx;
   m_ny = ny;
   m_x0 = x0;
   m_x1 = x1;
   m_y0 = y0;
   m_y1 = y1;

   //avoid comparison warning.  we checked if these would be > 0 above.
   int dx = x1-x0;
   int dy = y1-y0;
   if(dx != m_imdata.rows() || dy != m_imdata.cols())
   {
      m_imdata.resize( dx, dy);
   }

   m_regionChanged = 1;
}

void rtimvStats::statsThread()
{
   while(!m_dieNow)
   {
      usleep(m_statsPause*1000);
      calcStats();
   }
}

void rtimvStats::calcStats()
{
 
   if(!m_regionChanged) return;

   if(m_imv == nullptr)
   {
      m_regionChanged = 0;
      return; //no data.
   }

   //mutex lock here (trylock, exit and wait if can't get it)
   std::unique_lock<std::mutex> lock(m_dataMutex, std::try_to_lock);
   if(!lock.owns_lock()) return;

   float (*_pixel)(rtimvBase*, size_t) = m_imv->pixel();

   if(_pixel == nullptr)
   {
      m_regionChanged = 0;
      return; //invalid data
   }

   //copy data so we can be safe from changes to image memory
   for(size_t i=m_x0; i<m_x1; ++i)
   {
      for(size_t j=m_y0; j<m_y1; ++j)
      {
         size_t idx = j*m_nx + i;
         if(m_imv->imageValid(0) == false) //Check every time to be as robust as possible.
         {
            m_regionChanged = 0;
            return;
         }
         m_imdata(i-m_x0, j-m_y0) = _pixel(m_imv, idx);
      }
   }

   float tmp_min = m_imdata(0,0);
   float tmp_max = tmp_min;
   float tmp_mean = 0;
         
   for(int c = 0; c < m_imdata.cols(); ++c)
   {
      for(int r = 0; r < m_imdata.rows(); ++r)
      {
         float imval = m_imdata(r,c);
   
         tmp_mean += imval;
         if(imval < tmp_min) tmp_min = imval;
         if(imval > tmp_max) tmp_max = imval;
      }
   }
   
   m_dataMin = tmp_min;
   m_dataMax = tmp_max;
   m_dataMean = tmp_mean / (m_imdata.rows()*m_imdata.cols());
   
   m_statsChanged = 1;
   m_regionChanged = 0;
   
}


   
void rtimvStats::updateGUI()
{
   char txt[50];

   if(m_statsChanged)
   {
      snprintf(txt, 50, "%0.1f", (float) m_dataMin);
      ui.dataMin->setText(txt);

      snprintf(txt, 50, "%0.1f", (float) m_dataMax);
      ui.dataMax->setText(txt);

      snprintf(txt, 50, "%0.1f", (float) m_dataMean);
      ui.dataMean->setText(txt);

//       snprintf(txt, 50, "%0.1f", (float) m_datamedian);
//       ui.dataMedian->setText(txt);
      
      m_statsChanged = 0;
   }


}   

