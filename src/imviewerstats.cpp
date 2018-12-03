
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

void FitThread::run()
{
   imvs->fit_thread();
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
   regionChangedFit = 0;
   work = 0;
   regionSizeChanged = 0;
   
   imdata_min = 0;
   imdata_max = 0;
   imdata_mean = 0;
   imdata_median = 0;
   statsChanged = 1;

   for(int i=0;i<7;i++) fit_params[i] = 0.;
   fit_stat = 0;
   
   dieNow = 0;
   //Start stats thread here.
   sth.imvs = this;
   sth.start();

   fth.imvs = this;
   fth.start();

   maxPlot = 0;
   peakPlot = 0;
   fwhmPlot = 0;
   xPlot = 0;
   yPlot = 0;
   ellipPlot = 0;
   //maxPlot = new rtPlotForm("Max", "Max (ADU)", 100, 1, updateTimerTimeout*5., this);
   //connect(maxPlot, SIGNAL(rejected()), this, SLOT(maxPlotClosed()));

   maxPlot = new rtPlotForm("Max", "Max (ADU)", 100, 1, updateTimerTimeout*5., this);
   connect(maxPlot, SIGNAL(rejected()), this, SLOT(maxPlotClosed()));

   peakPlot = new rtPlotForm("Peak", "Peak (ADU)", 100, 1, updateTimerTimeout*5., this);
   connect(peakPlot, SIGNAL(rejected()), this, SLOT(peakPlotClosed()));
   
/*
   fwhmPlot = new rtPlotForm("FWHM", "FWHM (pix)", 100, 1, updateTimerTimeout*5., this);
   connect(fwhmPlot, SIGNAL(rejected()), this, SLOT(fwhmPlotClosed()));
   
   xPlot = new rtPlotForm("X", "X (pix)", 100, 1, updateTimerTimeout*5., this);
   connect(xPlot, SIGNAL(rejected()), this, SLOT(xPlotClosed()));
   xPlot->plotY0 = false;
   
   yPlot = new rtPlotForm("Y", "Y (pix)", 100, 1, updateTimerTimeout*5., this);
   connect(yPlot, SIGNAL(rejected()), this, SLOT(yPlotClosed()));
   yPlot->plotY0 = false;
  */ 
   dummydark = new char[1024*1024*IMDATA_TYPE_SIZE];
   memset(dummydark, 0, 1024*1024*IMDATA_TYPE_SIZE);
   
   //for(int i=0;i<1024*1024;i++) dummydark[i] = 0;
   
   hist_sz = 10;
   hist_x = new float[hist_sz];
   hist_y = new float[hist_sz];
   for(size_t i=0;i<hist_sz;i++)
   {
      hist_x[i] = 0;
      hist_y[i] = 0;
   }
   hist_ptr = 0;
   
   connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateGUI()));
   updateTimer.start(updateTimerTimeout);
}

imviewerStats::~imviewerStats()
{
   dieNow = 1;
   sth.wait();
   fth.wait();

      
   if(maxPlot) delete maxPlot;
   if(peakPlot) delete peakPlot;
   if(fwhmPlot) delete fwhmPlot;
   if(ellipPlot) delete ellipPlot;
   if(xPlot) delete xPlot;
   if(yPlot) delete yPlot;
   
   delete dummydark;
}
   
void imviewerStats::set_imdata(void * id, float ft, void * dd)
{
   
   imdata = (char *) id;
   darkdata = (char *) dd;
   
   frame_time = ft;
   
   regionChanged = 1;
   regionChangedFit = 1;
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
   regionChangedFit = 1;
   regionSizeChanged = 1;
   
   if(nx < 100)
   {
      if(hist_x) delete hist_x;
      if(hist_y) delete hist_y;
      
      hist_sz = 100;
      hist_x = new float[hist_sz];
      hist_y = new float[hist_sz];
      for(size_t i=0;i<hist_sz;i++)
      {
         hist_x[i] = 0;
         hist_y[i] = 0;
      }
      hist_ptr = 0;
   }
   else
   {
      if(hist_x) delete hist_x;
      if(hist_y) delete hist_y;
      
      hist_sz = 10;
      hist_x = new float[hist_sz];
      hist_y = new float[hist_sz];
      for(size_t i=0;i<hist_sz;i++)
      {
         hist_x[i] = 0;
         hist_y[i] = 0;
      }
      hist_ptr = 0;
   }  
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
   
   if(maxPlot) 
   {
      maxPlot->data->add_point(get_curr_time(), imdata_max);
   }
   statsChanged = 1;
   regionChanged = 0;
   
}


struct submatrixToFit
{
   char * data;
   char * darkdata;
   size_t x0;
   size_t x1;
   size_t nx;
   size_t y0;
   size_t y1;
};

float gauss_arg(float x, float y, float * p)
{
   float c  = cos(p[6]);
   float s  = sin(p[6]);
   
   return ( pow((x-p[4]) * (c/p[2]) - (y-p[5]) * (s/p[2]),2) + pow((x-p[4]) * (s/p[3]) + (y-p[5]) * (c/p[3]),2) );

}
      
void submatrix_gauss_fit(float *p, float *hx, int m, int n, void *adata)
{
   submatrixToFit * smtf = (submatrixToFit *) adata;

   float i, j;
   size_t idx_mat, idx_dat;

   idx_dat = 0;
   for(i=smtf->x0; i< smtf->x1; i++)
   {
      for(j=smtf->y0; j<smtf->y1; j++)
      {
         idx_mat = i+j*smtf->nx;
         
         hx[idx_dat] = p[0]+p[1]*exp(-.5 * gauss_arg(i, j, p) ) - (global_pixget(smtf->data,idx_mat) - global_pixget(smtf->darkdata,idx_mat));

         idx_dat++;
      }
   }
   (void)(m);
   (void)(n);
   
}


void imviewerStats::calc_fit()
{
   submatrixToFit smtf;
   float imval, darkval;
   
   if(!imdata) return;
   if(!regionChangedFit) return;

   smtf.data = imdata;
   
   if(darkdata) smtf.darkdata = darkdata;
   else smtf.darkdata = dummydark;
   
   
   smtf.nx = image_nx;
   //size_t ny = image_ny;
   smtf.x0 = region_x0;
   smtf.x1 = region_x1;
   smtf.y0 = region_y0;
   smtf.y1 = region_y1;

   
   int m = 7;
   int n = (region_x1-region_x0)*(region_y1-region_y0);
   if(regionSizeChanged || !work)
   {
      if(work) delete[] work;
      work = new float[LM_DIF_WORKSZ(m, n)];
      regionSizeChanged = 0;
   }

   size_t idx_mat;
   fit_params[0] = 0.;
   fit_params[1] = -1000000;
   int cnt = 0;
   for(float i=smtf.x0; i< smtf.x1; i++)
   {
      for(float j=smtf.y0; j<smtf.y1; j++)
      {
         cnt++;
         idx_mat = i+j*smtf.nx;
         
         imval = pixget(smtf.data,idx_mat);
         
         darkval = pixget(smtf.darkdata,idx_mat);
         
         fit_params[0] += (imval - darkval);
         
         if(imval-darkval > fit_params[1])
         {
            fit_params[1] = imval-darkval;//pixget(smtf.darkdata,idx_mat);
            fit_params[4] = i;
            fit_params[5] = j;
         }
      }
   }
   fit_params[0] /= cnt;
   
   //fit_params[0] = .5*(imdata_min + imdata_mean);
   //fit_params[1] = imdata_max - fit_params[0];
   fit_params[2] = 2.5;
   fit_params[3] = 2.5;
   //fit_params[4] = .5*(region_x1 + region_x0);
   //fit_params[5] = .5*(region_y1 + region_y0);
   fit_params[6] = 0;

   float t1= frame_time;//get_curr_time(); //get time now, so it is more accurate
   
   slevmar_dif(submatrix_gauss_fit, fit_params, 0, m,n , 1000, 0, fit_info, work, 0, (void *) &smtf);
   
                   /* O: information regarding the minimization. Set to NULL if don't care
                    * info[0]= ||e||_2 at initial p.
                    * info[1-4]=[ ||e||_2, ||J^T e||_inf,  ||Dp||_2, \mu/max[J^T J]_ii ], all computed at estimated p.
                    * info[5]= # iterations,
                    * info[6]=reason for terminating: 1 - stopped by small gradient J^T e
                    *                                 2 - stopped by small Dp
                    *                                 3 - stopped by itmax
                    *                                 4 - singular matrix. Restart from current p with increased \mu
                    *                                 5 - no further error reduction is possible. Restart with increased mu
                    *                                 6 - stopped by small ||e||_2
                    *                                 7 - stopped by invalid (i.e. NaN or Inf) "func" values; a user error
                    * info[7]= # function evaluations
                    * info[8]= # Jacobian evaluations
                    * info[9]= # linear systems solved, i.e. # attempts for reducing error
                    */

   fit_stat = (fit_info[6] < 3);

   fit_FWHM =  fabs(1.17741*(fit_params[2] + fit_params[3]));
   fit_FWHM_min = fabs(2.*1.17741*std::min(fit_params[2], fit_params[3]));
   fit_FWHM_max = fabs(2.*1.17741*std::max(fit_params[2], fit_params[3]));

   fit_x = fit_params[4] - .5*(region_x1 + region_x0);
   fit_y = fit_params[5] - .5*(region_y1 + region_y0);
   
   hist_x[hist_ptr] =  fit_x;
   hist_y[hist_ptr] = fit_y;
   hist_ptr++;
   if(hist_ptr >= hist_sz) hist_ptr=0;
   
   float meanx =0, meany = 0;
   for(size_t i=0;i<hist_sz;i++)
   {
      meanx += hist_x[i];
      meany += hist_y[i];
   }
   meanx /= hist_sz;
   meany /= hist_sz;
   
   stdx = 0; 
   stdy = 0;
   for(size_t i=0;i<hist_sz;i++)
   {
      stdx += pow(hist_x[i]-meanx,2);
      stdy += pow(hist_y[i]-meany,2);
   }
   stdx /= hist_sz;
   stdy /= hist_sz;
   
   stdx = sqrt(stdx);
   stdy = sqrt(stdy);
   
   fit_Ellipt = (fit_FWHM_max - fit_FWHM_min)/(fit_FWHM_max);

   if(peakPlot) peakPlot->data->add_point(t1, fit_params[1]);
   if(fwhmPlot) fwhmPlot->data->add_point(t1, fit_FWHM);
   if(ellipPlot) ellipPlot->data->add_point(t1, fit_Ellipt);
   if(xPlot) xPlot->data->add_point(t1, fit_x);
   if(yPlot) yPlot->data->add_point(t1, fit_y);
   
   regionChangedFit = 0;
   fitChanged = 1;
 
}


void imviewerStats::fit_thread()
{
   while(!dieNow)
   {
      usleep(statsPause*1000);
      calc_fit();
   }
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

   if(fitChanged)
   {
      snprintf(txt, 50, "%0.1f", (float) fit_params[1]);
      ui.dataPeak->setText(txt);
      
      snprintf(txt, 50, "%0.2f", (float) fit_FWHM);
      ui.dataFWHM->setText(txt);
      
      snprintf(txt, 50, "%0.2f", (float) fit_x);
      ui.dataX->setText(txt);
      
      snprintf(txt, 50, "%0.2f", (float) stdx);
      ui.dataX_rms->setText(txt);
      
      snprintf(txt, 50, "%0.2f", (float) fit_y);
      ui.dataY->setText(txt);
      
      snprintf(txt, 50, "%0.2f", (float) stdy);
      ui.dataY_rms->setText(txt);
      
      snprintf(txt, 50, "%0.2f", (float) fit_Ellipt);
      ui.dataEllipt->setText(txt);

      if(fabs(fit_Ellipt) < 0.03) ui.dataAngle->setText("-");
      else
      {
         snprintf(txt, 50, "%0.1f", (float) fit_params[6]*180./3.14159);
         ui.dataAngle->setText(txt);
      }
      snprintf(txt, 50, "%0.1f", (float) fit_params[0]);
      ui.dataBG->setText(txt);
      
      fitChanged = 0;
   }  
}   

void imviewerStats::on_maxPlotButton_clicked()
{
   if(!maxPlot)
   {
      maxPlot = new rtPlotForm("Max", "Max (ADU)", 100, 1, updateTimerTimeout*5., this);
      connect(maxPlot, SIGNAL(rejected()), this, SLOT(maxPlotClosed()));
   }
   maxPlot->show();
}

void imviewerStats::maxPlotClosed()
{
   //if(maxPlot) delete maxPlot;
   //maxPlot = 0;
}

void imviewerStats::on_peakPlotButton_clicked()
{
   if(!peakPlot)
   {
      peakPlot = new rtPlotForm("Peak", "Peak (ADU)", 100, 1, updateTimerTimeout*5., this);
      connect(peakPlot, SIGNAL(rejected()), this, SLOT(peakPlotClosed()));
   }
   peakPlot->show();
}

void imviewerStats::peakPlotClosed()
{
   //if(peakPlot) delete peakPlot;
   //peakPlot = 0;
}

void imviewerStats::on_fwhmPlotButton_clicked()
{
   if(!fwhmPlot)
   {
      fwhmPlot = new rtPlotForm("FWHM", "FWHM (pix)", 100, 1, updateTimerTimeout*5., this);
      connect(fwhmPlot, SIGNAL(rejected()), this, SLOT(fwhmPlotClosed()));
   }
   fwhmPlot->show();
}

void imviewerStats::fwhmPlotClosed()
{
   //if(fwhmPlot) delete fwhmPlot;
   //fwhmPlot = 0;
}

void imviewerStats::on_ellipPlotButton_clicked()
{
   if(!ellipPlot)
   {
      ellipPlot = new rtPlotForm("Ellipticity", "Ellip.", 100, 1, updateTimerTimeout*5., this);
      connect(ellipPlot, SIGNAL(rejected()), this, SLOT(ellipPlotClosed()));
   }
   ellipPlot->show();
}

void imviewerStats::on_xPlotButton_clicked()
{
   if(!xPlot)
   {
      xPlot = new rtPlotForm("X", "X (pix)", 100, 1, updateTimerTimeout*5., this);
      connect(xPlot, SIGNAL(rejected()), this, SLOT(xPlotClosed()));
      xPlot->plotY0 = false;
   }
   xPlot->show();
}

void imviewerStats::xPlotClosed()
{
   //if(xPlot) delete xPlot;
   //xPlot = 0;
}

void imviewerStats::on_yPlotButton_clicked()
{
   if(!yPlot)
   {
      yPlot = new rtPlotForm("Y", "Y (pix)", 100, 1, updateTimerTimeout*5., this);
      connect(yPlot, SIGNAL(rejected()), this, SLOT(yPlotClosed()));
      yPlot->plotY0 = false;
   }
   yPlot->show();
}

void imviewerStats::yPlotClosed()
{
   //if(yPlot) delete yPlot;
   //yPlot = 0;
}
