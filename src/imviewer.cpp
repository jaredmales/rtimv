
#include "imviewer.hpp"

imviewer::imviewer(imviewer_shmt shkey, QWidget * Parent, Qt::WindowFlags f) : QWidget(Parent, f)
{
   pixget = getPixPointer<1>();
   type_size = imageStructDataType<1>::size;

   shmem_key = shkey;

   connect(&imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
}


void imviewer::setUserBoxActive(bool usba)
{
   if(usba)
   {
      int idx;
      float imval;

      if(userBox_i0 > userBox_i1)
      {
         idx = userBox_i0;
         userBox_i0 = userBox_i1;
         userBox_i1 = idx;
      }

      if(userBox_i0 < 0) userBox_i0 = 0;
      if(userBox_i0 >= nx) userBox_i0 = nx-(userBox_i1-userBox_i0);

      if(userBox_i1 <= 0) userBox_i1 = 0 + (userBox_i1-userBox_i0);
      if(userBox_i1 > nx) userBox_i1 = nx-1;

      if(userBox_j0 > userBox_j1)
      {
         idx = userBox_j0;
         userBox_j0 = userBox_j1;
         userBox_j1 = idx;
      }

      if(userBox_j0 < 0) userBox_j0 = 0;
      if(userBox_j0 >= nx) userBox_j0 = ny-(userBox_j1-userBox_j0);

      if(userBox_j1 <= 0) userBox_j1 = 0 + (userBox_j1-userBox_j0);
      if(userBox_j1 > ny) userBox_j1 = ny-1;


      idx = userBox_i0*ny + userBox_j0;
      imval = pixget(imdata, idx);

      userBox_min = imval;
      userBox_max = imval;
      for(int i = userBox_i0; i < userBox_i1; i++)
      {
         for(int j = userBox_j0; j < userBox_j1; j++)
         {
            idx = i*ny + j;
            imval = pixget(imdata, idx);

            if(imval < userBox_min) userBox_min = imval;
            if(imval > userBox_max) userBox_max = imval;
         }
      }

      set_mindat(userBox_min);
      set_maxdat(userBox_max);
      userBoxActive = usba;
      set_colorbar_mode(minmaxbox);
      changeImdata(false);
      return;
   }
   userBoxActive = usba;

}

void imviewer::allocImdata(int x, int y)
{
   if(imdata && localImdata) delete[] imdata;

   setImsize(x, y);

   imdata = new char[nx*ny*type_size];

   localImdata = true;
   changeImdata(true);
}

void imviewer::setImsize(int x, int y)
{
   int cb;

   if(nx !=x  || ny !=y  || qim == 0)
   {
      if(x!=0 && y!=0)
      {
         nx = x;
         ny = y;

         if(qim) delete qim;

         qim = new QImage(nx, ny, QImage::Format_Indexed8);

         cb = current_colorbar; //force a reload.
         current_colorbar = -1;

         load_colorbar(cb);

         postSetImsize();
      }
   }
}

void imviewer::postSetImsize()
{
   return;
}

void imviewer::changeImdata(bool newdata)
{
   float tmp_min;
   float tmp_max;

   int idx;
   float imval;

   if(!imdata) return;

   amChangingimdata = true;

   if(!newdata)
   {
      changeImdataRecolorOnly();
   }
   else
   {
      //Update statistics
      imval = pixget(imdata,0);
      tmp_min = imval;
      tmp_max = imval;
      saturated = 0;

      if(userBoxActive)
      {
         idx = userBox_i0*ny + userBox_j0;
         imval = pixget(imdata, idx);
         userBox_min = imval;
         userBox_max = imval;
      }

      for(int i = 0; i < ny; i++)
      {
         for(int j=0;j < nx; j++)
         {
            idx = i*nx + j;
            imval = pixget(imdata, idx);

            if(imval > tmp_max) tmp_max = imval;
            if(imval < tmp_min) tmp_min = imval;

            if(imval >= sat_level) saturated++;

            if(userBoxActive)
            {
               if(i>=userBox_i0 && i<userBox_i1 && j>=userBox_j0 && j < userBox_j1)
               {
                  if(imval < userBox_min) userBox_min = imval;
                  if(imval > userBox_max) userBox_max = imval;
               }
            }

            qim->setPixel(j, ny-i-1, (int)calcPixval(imval));

         }
      }

      imdat_max = tmp_max;
      imdat_min = tmp_min;

      mindat_rel = (mindat - imdat_min)/(imdat_max-imdat_min);
      maxdat_rel = (maxdat - imdat_min)/(imdat_max-imdat_min);
    }
    qpm = QPixmap::fromImage(*qim,Qt::ThresholdDither);

    postChangeImdata();
    amChangingimdata = false;
}

void imviewer::changeImdataRecolorOnly()
{
   for(int i = 0; i < ny; i++)
   {
      for(int j=0;j <nx; j++)
      {
         qim->setPixel(j, ny-i-1, (int)calcPixval( pixget(imdata,j*ny + i) ));
      }
   }
}

void imviewer::changeImdata_applyDark(bool newdata)
{
#if RT_SYSTEM == RT_SYSTEM_VISAO
//    //float pixval;
//    float tmp_min;
//    float tmp_max;
//    //float tmp_mean;
// 
//    int idx;
//    float imval, darkval, imv_m_darkv;
// 
//    if(!imdata) return;
// 
//    amChangingimdata = true;
// 
//    if(!newdata)
//    {
//       changeImdataRecolorOnly_applyDark();
//    }
//    else
//    {
//       //Update statistics
//       imval = pixget(imdata, 0);
//       darkval = pixget(dark_sim.imdata, 0);
//       imv_m_darkv = imval-darkval;
// 
//       tmp_min = imv_m_darkv;
//       tmp_max = imv_m_darkv;
//       saturated = 0;
// 
//       //tmp_mean = 0;
// 
// 
//       if(userBoxActive)
//       {
//          idx = userBox_i0*ny + userBox_j0;
//          imval = pixget(imdata, idx);
//          darkval = pixget(dark_sim.imdata, idx);
//          imv_m_darkv = imval-darkval;
// 
//          userBox_min = imv_m_darkv;
//          userBox_max = imv_m_darkv;
//       }
// 
//       for(int i = 0; i < ny; i++)
//       {
//          for(int j=0;j < nx; j++)
//          {
//             idx = i*ny + j;
//             imval = pixget(imdata, idx);
//             darkval = pixget(dark_sim.imdata, idx);
//             imv_m_darkv = imval-darkval;
// 
//             if(imv_m_darkv > tmp_max) tmp_max = imv_m_darkv;
//             if(imv_m_darkv < tmp_min) tmp_min = imv_m_darkv;
// 
//             if(imval >= sat_level) saturated++;
// 
//             if(userBoxActive)
//             {
//                if(i>=userBox_i0 && i<userBox_i1 && j>=userBox_j0 && j < userBox_j1)
//                {
//                   if(imv_m_darkv < userBox_min) userBox_min = imv_m_darkv;
//                   if(imv_m_darkv > userBox_max) userBox_max = imv_m_darkv;
//                }
//             }
// 
//             qim->setPixel(j, ny-i-1, (int)calcPixval(imv_m_darkv));
// 
//          }
//       }
// 
//       imdat_max = tmp_max;//max(*mean_acc);
//       imdat_min = tmp_min;// min(*mean_acc);
//       //imdat_mean = tmp_mean/(nx*ny);//mean(*mean_acc);
// 
//       mindat_rel = (mindat - imdat_min)/(imdat_max-imdat_min);
//       maxdat_rel = (maxdat - imdat_min)/(imdat_max-imdat_min);
// 
// 
//    }
//    qpm = QPixmap::fromImage(*qim,Qt::ThresholdDither);
// 
//    postChangeImdata();
//    amChangingimdata = false;

#else
   (void)(newdata);
#endif
}

void imviewer::changeImdataRecolorOnly_applyDark()
{
//    #if RT_SYSTEM == RT_SYSTEM_VISAO
//    int idx;
// 
//    for(int i = 0; i < ny; i++)
//    {
//       for(int j=0;j <nx; j++)
//       {
//          idx = i*ny + j;
//          qim->setPixel(j, ny-i-1, (int)calcPixval(pixget(imdata,idx)-pixget(dark_sim.imdata, idx)));
//       }
//    }
//   #endif
}

float imviewer::calcPixval(float d)
{
   float pixval;
   float a = 1000;
   static float log10_a = log10(a);

   pixval = d - mindatsc;
   if(pixval < 0) pixval = 0;

   if(pixval > maxdatsc - mindatsc) pixval = maxdatsc-mindatsc;

   if(maxdatsc > mindatsc) pixval = pixval/((float)(maxdatsc-mindatsc));
   else pixval = .5;

   switch(colorbar_type)
   {
      case typelog:
         pixval = log10(a*pixval+1.)/log10_a;
         break;
      case typepow:
         pixval = (pow(a, pixval) - 1.)/a;
         break;
      case typesqrt:
         pixval = sqrt(pixval);
         break;
      case typesquare:
         pixval = pixval*pixval;
         break;
      default:
         break;
   }

   if(pixval > 1.) pixval = 1.;
   if(pixval < 0.) pixval = 0.;

   return pixval*((float)(maxcol-1-mincol));

}

void imviewer::postChangeImdata()
{
   return;
}

void imviewer::point_imdata(void * imd)
{
   if(imdata && localImdata) delete[] imdata;
   imdata = (char *)imd;

   localImdata = false;
   changeImdata(true);
}

void imviewer::point_imdata(int x, int y, void * imd)
{
   setImsize(x,y);
   point_imdata(imd);
}

void imviewer::set_mindat(float md)
{
   mindat = md;
   if(colorbar_type == typelinear)
   {
      mindatsc = mindat;
   }
   if(colorbar_type == typelog)
   {
      mindatsc = 0;

   }
}

void imviewer::set_maxdat(float md)
{
   maxdat = md;
   if(colorbar_type == typelinear)
   {
      maxdatsc = maxdat;
   }
   if(colorbar_type == typelog)
   {
      maxdatsc = maxdat;
   }
}

void imviewer::set_bias(float b)
{
   float cont = get_contrast();

   set_mindat(b - 0.5*cont);
   set_maxdat(b + 0.5*cont);
}

void imviewer::set_bias_rel(float br)
{
   float cont = get_contrast();

   set_mindat(imdat_min + br*(imdat_max-imdat_min) - 0.5*cont);
   set_maxdat(imdat_min + br*(imdat_max-imdat_min) + 0.5*cont);
}

void imviewer::set_contrast(float c)
{
   float b = get_bias();
   set_mindat(b - 0.5*c);
   set_maxdat(b + 0.5*c);
}

void imviewer::set_contrast_rel(float cr)
{
   float b = get_bias();
   set_mindat(b - .5*(imdat_max-imdat_min)/cr);
   set_maxdat(b + .5*(imdat_max-imdat_min)/cr);
}




void imviewer::load_colorbar(int cb)
{
   if(current_colorbar != cb && qim)
   {
      current_colorbar = cb;
      switch(cb)
      {
         case colorbarRed:
            mincol = 0;
            maxcol = 255;
            //qim->setNumColors(256);
            for(int i=mincol; i<maxcol; i++) qim->setColor(i, qRgb(i,0,0));
            warning_color = QColor("lime");
            break;
         case colorbarGreen:
            mincol = 0;
            maxcol = 255;
            //qim->setNumColors(256);
            for(int i=mincol; i<maxcol; i++) qim->setColor(i, qRgb(0,i,0));
            warning_color = QColor("magenta");
            break;
         case colorbarBlue:
            mincol = 0;
            maxcol = 255;
            //qim->setNumColors(256);
            for(int i=mincol; i<maxcol; i++) qim->setColor(i, qRgb(0,0,i));
            warning_color = QColor("yellow");
            break;
         case colorbarJet:

            mincol = 0;
            maxcol = load_colorbar_jet(qim);
            warning_color = QColor("white");
            break;
         case colorbarHot:
            mincol = 0;
            maxcol = load_colorbar_hot(qim);
            warning_color = QColor("cyan");
            break;
         case colorbarBone:
            mincol = 0;
            maxcol = load_colorbar_bone(qim);
            warning_color = QColor("lime");
            break;
         default:
            mincol = 0;
            maxcol = 255;
            //qim->setNumColors(256);
            for(int i=mincol; i<maxcol; i++) qim->setColor(i, qRgb(i,i,i));
            warning_color = QColor("lime");
            break;
      }
      changeImdata();
   }
}

void imviewer::set_colorbar_type(int ct)
{
   if(ct < 0 || ct >= colorbar_types_max)
   {
      ct = typelinear;
   }

   colorbar_type = ct;

   //now update mindatsc and maxdatasc
   set_mindat(mindat);
   set_maxdat(maxdat);
   /*      if(colorbar_type == typelinear)
    *       {
    *               mindatsc = mindat;
    *               maxdatsc = maxdat;
    }

    if(colorbar_type == typelog)
    {
       mindatsc = log10(mindat);
       maxdatsc = log10(maxdat);
    }*/
}

void imviewer::set_ZoomLevel(float zl)
{
   if(zl < ZoomLevel_min) zl = ZoomLevel_min;
   if(zl > ZoomLevel_max) zl = ZoomLevel_max;

   ZoomLevel = zl;

   post_set_ZoomLevel();
}

void imviewer::post_set_ZoomLevel()
{
   return;
}

void imviewer::set_RealTimeEnabled(int rte)
{
   RealTimeEnabled = (rte != 0);
}

void imviewer::set_RealTimeStopped(int rts)
{
   RealTimeStopped = (rts != 0);

   if(RealTimeStopped)
   {
      imtimer.stop();
      if(imdata)
      {
         if(tmpim) free(tmpim);
         tmpim = (char *) malloc(nx*ny*type_size); //new IMDATA_TYPE[nx*ny];
         //for(int i=0; i< nx*ny; i++) tmpim[i] = imdata[i];
         memcpy(tmpim, imdata, nx*ny*type_size);
         imdata = tmpim;
      }
   }
   else
   {
      imtimer.start(imtimer_timeout);

      if(shmem_attached) timerout();
      else shmem_timerout();

      if(tmpim) free(tmpim);//delete[] tmpim;
      tmpim = 0;
   }
}

void imviewer::set_imtimer_timeout(int to)
{
   imtimer_timeout = to;
   imtimer.start(imtimer_timeout);
}

void imviewer::_shmem_timerout()
{
   shmem_timerout();

   if(shmem_attached)
   {
      disconnect(&imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
      connect(&imtimer, SIGNAL(timeout()), this, SLOT(_timerout()));
   }
}


void imviewer::shmem_timerout()
{

   if( ImageStreamIO_openIm(&image, shmem_key.c_str()) != 0)
   {
      shmem_attached = 0;
      sleep(1);
      return; //comeback on next timeout
   }
   
   if(image.md[0].sem <= semaphoreNumber)  //Creation not complete yet (believe it or not this happens0
   {
      ImageStreamIO_closeIm(&image);
      shmem_attached = 0;
      sleep(1);
      return; //We just need to wait for the server process to finish startup, so come back on next timeout
   }
   
   shmem_attached = 1;
   
   sem = image.semptr[semaphoreNumber];
   type_size = ImageStreamIO_typesize(image.md[0].atype);
         
   setImsize(image.md[0].size[0], image.md[0].size[1]);
         
   switch(image.md[0].atype)
   {
      case IMAGESTRUCT_UINT8:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT8>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_UINT8>::max;
         break;
      case IMAGESTRUCT_INT8:
         this->pixget = getPixPointer<IMAGESTRUCT_INT8>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_INT8>::max;
         break;
      case IMAGESTRUCT_UINT16:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT16>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_UINT16>::max;
         break;
      case IMAGESTRUCT_INT16:
         this->pixget = getPixPointer<IMAGESTRUCT_INT16>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_INT16>::max;
         break;
      case IMAGESTRUCT_UINT32:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT32>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_UINT32>::max;
         break;
      case IMAGESTRUCT_INT32:
         this->pixget = getPixPointer<IMAGESTRUCT_INT32>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_INT32>::max;
         break;
      case IMAGESTRUCT_UINT64:
         this->pixget = getPixPointer<IMAGESTRUCT_UINT64>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_UINT64>::max;
         break;
      case IMAGESTRUCT_INT64:
         this->pixget = getPixPointer<IMAGESTRUCT_INT64>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_INT64>::max;
         break;
      case IMAGESTRUCT_FLOAT:
         this->pixget = getPixPointer<IMAGESTRUCT_FLOAT>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_FLOAT>::max;
         break;
      case IMAGESTRUCT_DOUBLE:
         this->pixget = getPixPointer<IMAGESTRUCT_DOUBLE>();
         sat_level = (float) imageStructDataType<IMAGESTRUCT_DOUBLE>::max;
         break;
      default:
         std::cerr << "Unknown or unsupported data type\n";
         exit(0);
   }

}

void imviewer::_timerout()
{
   timerout();
}

void imviewer::timerout()
{
   int curr_image;
   size_t snx, sny;

   if(sem_trywait(sem) == 0)
   {
      if(image.md[0].size[2] > 0)
      {
         curr_image = image.md[0].cnt1 - 1;
         if(curr_image < 0) curr_image = image.md[0].size[2] - 1;
      }
      else curr_image = 0;

      update_fps(false);

      snx = image.md[0].size[0];
      sny = image.md[0].size[1];
      point_imdata(snx,sny, (void *) (image.array.SI8 + curr_image*snx*sny*type_size));
   }
   else 
   {
      if(image.md[0].sem <= 0) //Indicates that the server has cleaned up.
      {
         ImageStreamIO_closeIm(&image);
         shmem_attached = 0;
         disconnect(&imtimer, SIGNAL(timeout()), this, SLOT(_timerout()));
         connect(&imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
        
         i_fps = -1;
      }
      
      update_fps(true);
   }

}

void imviewer::update_fps(bool NoAdvance)
{
   struct timeval tvtmp;
   double dftime, timetmp;
   static unsigned n_noadvance = 0;

   if(i_fps < 0)
   {
      fps_hist.clear();
      i_fps = 0;
      fps_time0 = 0.;
      fps_ave = 0.0;
      n_noadvance = 0;
   }
   gettimeofday(&tvtmp, 0);
   timetmp = (double) tvtmp.tv_sec + ((double)tvtmp.tv_usec)/1e6;

   if(fps_time0 > 0)
   {
      dftime = timetmp - fps_time0;

      //insert time
      if(fps_hist.size() < n_ave_fps)
      {
         fps_hist.push_back(dftime);
         i_fps = fps_hist.size()-1;
      }
      else
      {
         fps_hist[i_fps] = dftime;
         ++i_fps;
         if((unsigned) i_fps >= n_ave_fps) i_fps = 0;
      }

      //calc fps_ave
      fps_sum = 0.;
      for(unsigned i=0; i<fps_hist.size(); i++) fps_sum += fps_hist[i];
      fps_ave = 1./(fps_sum/fps_hist.size());

      if(NoAdvance)
      {

         --i_fps;
         if(fps_hist.size() < n_ave_fps) fps_hist.pop_back();
         if(i_fps < 0) i_fps = n_ave_fps-1;

         ++n_noadvance;
      }
      else
      {
         n_noadvance = 0;
      }
   }

   if(!NoAdvance) fps_time0 =  timetmp;

   update_age();
}

void imviewer::update_age()
{
   return;
}
