
#include "imviewer.hpp"

imviewer::imviewer(imviewer_shmt shkey, QWidget * Parent, Qt::WindowFlags f) : QWidget(Parent, f)
{
   pixget = getPixPointer<1>();
   type_size = imageStructDataType<1>::size;

   shmem_key = shkey;

   connect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
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
      if(userBox_i0 >= (int64_t) m_nx) userBox_i0 = (int64_t) m_nx-(userBox_i1-userBox_i0);

      if(userBox_i1 <= 0) userBox_i1 = 0 + (userBox_i1-userBox_i0);
      if(userBox_i1 > (int64_t) m_nx) userBox_i1 = (int64_t)m_nx-1;

      if(userBox_j0 > userBox_j1)
      {
         idx = userBox_j0;
         userBox_j0 = userBox_j1;
         userBox_j1 = idx;
      }

      if(userBox_j0 < 0) userBox_j0 = 0;
      if(userBox_j0 >= (int64_t) m_nx) userBox_j0 = (int64_t)m_ny-(userBox_j1-userBox_j0);

      if(userBox_j1 <= 0) userBox_j1 = 0 + (userBox_j1-userBox_j0);
      if(userBox_j1 > (int64_t) m_ny) userBox_j1 = (int64_t)m_ny-1;


      idx = userBox_j0*m_nx + userBox_i0;
      
      imval = pixget(m_imdata, idx);

      userBox_min = imval;
      userBox_max = imval;
      for(int i = userBox_i0; i < userBox_i1; i++)
      {
         for(int j = userBox_j0; j < userBox_j1; j++)
         {
            idx = j*m_nx + i;
            imval = pixget(m_imdata, idx);

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

void imviewer::allocImdata(uint32_t x, uint32_t y)
{
   if(m_imdata && localImdata) delete[] m_imdata;

   setImsize(x, y);

   m_imdata = new char[m_nx*m_ny*type_size];

   localImdata = true;
   changeImdata(true);
}

void imviewer::setImsize(uint32_t x, uint32_t y)
{
   int cb;

   if(m_nx !=x  || m_ny !=y  || qim == 0)
   {
      if(x!=0 && y!=0)
      {
         m_nx = x;
         m_ny = y;

         if(qim) delete qim;

         qim = new QImage(m_nx, m_ny, QImage::Format_Indexed8);

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

   if(!m_imdata) return;

   amChangingimdata = true;

   if(!newdata)
   {
      changeImdataRecolorOnly();
   }
   else
   {
      //Update statistics
      imval = pixget(m_imdata,0);
      tmp_min = imval;
      tmp_max = imval;
      saturated = 0;

      if(userBoxActive)
      {
         idx = userBox_j0*m_nx + userBox_i0;
         imval = pixget(m_imdata, idx);
         userBox_min = imval;
         userBox_max = imval;
      }

      for(uint32_t i = 0; i < m_ny; ++i)
      {
         for(uint32_t j=0;j < m_nx; ++j)
         {
            idx = i*m_nx + j;
            imval = pixget(m_imdata, idx);

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

            qim->setPixel(j, m_ny-i-1, (int)calcPixval(imval));

         }
      }

      imdat_max = tmp_max;
      imdat_min = tmp_min;

      mindat_rel = (mindat - imdat_min)/(imdat_max-imdat_min);
      maxdat_rel = (maxdat - imdat_min)/(imdat_max-imdat_min);
    }
    //qpm = QPixmap::fromImage(*qim, Qt::AutoColor | Qt::ThresholdDither);
    qpm.convertFromImage(*qim, Qt::AutoColor | Qt::ThresholdDither);

    postChangeImdata();
    amChangingimdata = false;
}

void imviewer::changeImdataRecolorOnly()
{
   for(uint32_t i = 0; i < m_ny; ++i)
   {
      for(uint32_t j=0;j <m_nx; ++j)
      {
         qim->setPixel(j, m_ny-i-1, (int)calcPixval( pixget(m_imdata,i*m_nx + j) ));
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
//          idx = userBox_i0*m_ny + userBox_j0;
//          imval = pixget(imdata, idx);
//          darkval = pixget(dark_sim.imdata, idx);
//          imv_m_darkv = imval-darkval;
// 
//          userBox_min = imv_m_darkv;
//          userBox_max = imv_m_darkv;
//       }
// 
//       for(int i = 0; i < m_ny; i++)
//       {
//          for(int j=0;j < m_nx; j++)
//          {
//             idx = i*m_ny + j;
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
//             qim->setPixel(j, m_ny-i-1, (int)calcPixval(imv_m_darkv));
// 
//          }
//       }
// 
//       imdat_max = tmp_max;//max(*mean_acc);
//       imdat_min = tmp_min;// min(*mean_acc);
//       //imdat_mean = tmp_mean/(m_nx*m_ny);//mean(*mean_acc);
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
//    for(int i = 0; i < m_ny; i++)
//    {
//       for(int j=0;j <m_nx; j++)
//       {
//          idx = i*m_ny + j;
//          qim->setPixel(j, m_ny-i-1, (int)calcPixval(pixget(imdata,idx)-pixget(dark_sim.imdata, idx)));
//       }
//    }
//   #endif
}

float imviewer::calcPixval(float d)
{
   float pixval;
   static float a = 1000;
   static float log10_a = log10(a);

   //pixval = d - mindatsc;
   //if(pixval < 0) pixval = 0;

   //if(pixval > maxdatsc - mindatsc) pixval = maxdatsc-mindatsc;

   //if(maxdatsc > mindatsc) pixval = pixval/((float)(maxdatsc-mindatsc));
   //else pixval = .5;

   pixval = (d - mindatsc)/((float)(maxdatsc-mindatsc));
   if(pixval < 0) pixval = 0;
   
   switch(colorbar_type)
   {
      case typelog:
         pixval = log10(pixval*a+1)/log10_a; //log10(a*pixval+1.)/log10_a;
         
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
   if(m_imdata && localImdata) delete[] m_imdata;
   
   m_imdata = (char *)imd;

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
//    if(colorbar_type == typelinear)
//    {
      mindatsc = mindat;
//    }
//    if(colorbar_type == typelog)
//    {
//       mindatsc = 0;
// 
//    }
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

   set_mindat(mindat);
   set_maxdat(maxdat);

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
      m_imtimer.stop();
      if(m_imdata)
      {
         if(tmpim) free(tmpim);
         tmpim = (char *) malloc(m_nx*m_ny*type_size); //new IMDATA_TYPE[m_nx*m_ny];
         
         memcpy(tmpim, m_imdata, m_nx*m_ny*type_size);
         m_imdata = tmpim;
      }
   }
   else
   {
      m_imtimer.start(m_imageTimeout);

      if(shmem_attached) timerout();
      else shmem_timerout();

      if(tmpim) free(tmpim);//delete[] tmpim;
      tmpim = 0;
   }
}

void imviewer::set_imageTimeout(int to)
{
   m_imageTimeout = to;
   m_imtimer.start(m_imageTimeout);
}

void imviewer::_shmem_timerout()
{
   m_imtimer.stop();
   shmem_timerout();

   if(shmem_attached)
   {
      disconnect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
      connect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_timerout()));
      m_imtimer.start(m_imageTimeout);
   }
   else
   {
      m_imtimer.start(m_shmemTimeout);
   }
}


void imviewer::shmem_timerout()
{

   if( ImageStreamIO_openIm(&image, shmem_key.c_str()) != 0)
   {
      shmem_attached = 0;
      return; //comeback on next timeout
   }
   
   if(image.md[0].sem <= 1)  //Creation not complete yet (believe it or not this happens0
   {
      ImageStreamIO_closeIm(&image);
      shmem_attached = 0;
      return; //We just need to wait for the server process to finish startup, so come back on next timeout
   }
   
   shmem_attached = 1;
   
   type_size = ImageStreamIO_typesize(image.md[0].datatype);
         
   setImsize(image.md[0].size[0], image.md[0].size[1]);
         
   switch(image.md[0].datatype)
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
   int64_t curr_image;
   uint64_t cnt0;
   
   static uint64_t lastCnt0 = -1; //make huge so we don't skip the 0 image
   
   uint32_t snx, sny;

   static int fps_counter = 0;
   static int age_counter = 0;
   
   if(image.md[0].sem <= 0) //Indicates that the server has cleaned up.
   {
      ImageStreamIO_closeIm(&image);
      shmem_attached = 0;
      lastCnt0 = -1;
      disconnect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_timerout()));
      connect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
      
      return;
   }
      
      
   if(image.md[0].size[2] > 0)
   {
      curr_image = image.md[0].cnt1 - 1;
      if(curr_image < 0) curr_image = image.md[0].size[2] - 1;
   }
   else curr_image = 0;

   
   
   snx = image.md[0].size[0];
   sny = image.md[0].size[1];
   
   if( snx != m_nx || sny != m_ny ) //Something else changed!
   {
      ImageStreamIO_closeIm(&image);
      shmem_attached = 0;
      lastCnt0 = -1;
      disconnect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_timerout()));
      connect(&m_imtimer, SIGNAL(timeout()), this, SLOT(_shmem_timerout()));
      
      return;
   }
   
   cnt0 = image.md[0].cnt0;
   if(cnt0 != lastCnt0) //Only redraw if it's actually a new image.
   {
      point_imdata(snx,sny, (void *) (image.array.SI8 + curr_image*snx*sny*type_size));
   
   
      lastCnt0 = cnt0;
   
      if(fps_counter > 1000/m_imageTimeout)
      {
         update_fps();
         fps_counter = 0;
         age_counter = 0;
      }
      else
      {
         ++fps_counter;
      }
   }
   else
   {
      if(age_counter > 1000/m_imageTimeout)
      {
         update_age();
         age_counter = 0;
         fps_counter = 0;
         m_fpsEst = 0;
      }
      else
      {
         ++age_counter;
      }
   }
      
      


}

void imviewer::update_fps()
{
   double dftime;

   m_fpsTime = image.md[0].atime.tv_sec + ((double) image.md[0].atime.tv_nsec)/1e9;
   if(m_fpsTime0 == 0)
   {
      m_fpsTime0 = m_fpsTime;
      m_fpsFrame0 = image.md[0].cnt0;
   }
   
   if(m_fpsTime != m_fpsTime0)
   {
      dftime = m_fpsTime - m_fpsTime0;

      if(dftime < 1e-9) return;

      
      m_fpsEst = (float)((image.md[0].cnt0 - m_fpsFrame0))/dftime;   
      
      //std::cerr << dftime << " " << m_fpsEst << "\n";
      
      m_fpsTime0 = m_fpsTime;
      m_fpsFrame0 = image.md[0].cnt0;
   }

   update_age();
}

void imviewer::update_age()
{
   return;
}
