
#include "imviewer.hpp"

imviewer::imviewer( const std::vector<std::string> & shkeys, 
                    QWidget * Parent, 
                    Qt::WindowFlags f
                  ) : QWidget(Parent, f)
{
   m_images.resize(shkeys.size());
   
   for(size_t i=0; i< m_images.size(); ++i)
   {
      m_images[i] = new rtimvImage;
      m_images[i]->m_shmimName=shkeys[i]; //Set the key
      m_images[i]->m_timer.start(m_images[i]->m_shmimTimeout); //and set timers.
   }
   
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(_timerout()));
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

void imviewer::timeout(int to)
{
   m_timer.stop();
   
   for(size_t i=0; i<m_images.size();++i)
   {
      m_images[i]->timeout(to); //just for fps calculations
   }
   
   m_timer.start(to);
}

void imviewer::_timerout()
{
   int doupdate = m_images[0]->update();
   
   for(size_t i=1;i<m_images.size(); ++i) 
   {
      m_images[i]->update();
   }
   
   if(doupdate >= RTIMVIMAGE_IMUPDATE) 
   {
      changeImdata(true);
   }
   
   if(doupdate == RTIMVIMAGE_FPSUPDATE) 
   {
      updateFPS();
   }
   
   if(doupdate == RTIMVIMAGE_AGEUPDATE) 
   {
      updateAge();
   }
   
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


      float (*pixel)(imviewer*,size_t) = &pixel_noCal;
   
   if(m_subtractDark == true && m_applyMask == false && m_images.size()>1)
   {
      pixel = &imviewer::pixel_subDark;
   }
   
   if(m_subtractDark == false && m_applyMask == true && m_images.size()>2)
   {
      pixel = &imviewer::pixel_applyMask;
   }
   
   if(m_subtractDark == true && m_applyMask == true && m_images.size()>1)
   {
      if(m_images.size()==2) pixel = &pixel_subDark;
      else pixel = &pixel_subDarkApplyMask;
   }
   
      idx = userBox_j0*m_nx + userBox_i0;
      
      imval = pixel(this, idx); //m_imData[idx];

      userBox_min = imval;
      userBox_max = imval;
      for(int i = userBox_i0; i < userBox_i1; i++)
      {
         for(int j = userBox_j0; j < userBox_j1; j++)
         {
            idx = j*m_nx + i;
            imval = pixel(this, idx);// m_imData[idx];

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

float imviewer::pixel_noCal( imviewer * imv,
                             size_t idx
                           )
{
   return imv->m_images[0]->pixel(idx);
}

float imviewer::pixel_subDark( imviewer * imv,
                             size_t idx
                           )
{
   return imv->m_images[0]->pixel(idx) - imv->m_images[1]->pixel(idx);
}

float imviewer::pixel_applyMask( imviewer * imv,
                                 size_t idx
                               )
{
   return imv->m_images[0]->pixel(idx) * imv->m_images[2]->pixel(idx);
}

float imviewer::pixel_subDarkApplyMask( imviewer * imv,
                                        size_t idx
                                      )
{
   return (imv->m_images[0]->pixel(idx) - imv->m_images[1]->pixel(idx))*imv->m_images[2]->pixel(idx);
}

void imviewer::changeImdata(bool newdata)
{
   float tmp_min;
   float tmp_max;

   int idx;
   float imval;

   if(!m_images[0]->m_data) return;

   float (*pixel)(imviewer*, size_t) = &pixel_noCal;
   
   if(m_subtractDark == true && m_applyMask == false && m_images.size()>1)
   {
      pixel = &pixel_subDark;
   }
   
   if(m_subtractDark == false && m_applyMask == true && m_images.size()>2)
   {
      pixel = &pixel_applyMask;
   }
   
   if(m_subtractDark == true && m_applyMask == true && m_images.size()>1)
   {
      if(m_images.size()==1) pixel = &pixel_subDark;
      else pixel = &pixel_subDarkApplyMask;
   }
   
   if(m_images[0]->m_nx != m_nx || m_images[0]->m_ny != m_ny) 
   {
      setImsize(m_images[0]->m_nx, m_images[0]->m_ny);
      
      //Need to set these at the beginning
      imdat_min = pixel(this,0);
      imdat_max = pixel(this,0);
      for(uint32_t i = 0; i < m_ny; ++i)
      {
         for(uint32_t j=0;j < m_nx; ++j)
         {
            if(pixel(this, i*m_nx + j) > imdat_max) imdat_max = pixel(this, i*m_nx + j);
            if(pixel(this, i*m_nx + j) < imdat_min) imdat_min = pixel(this, i*m_nx + j) ;

         }
      }
      set_mindat(imdat_min);
      set_maxdat(imdat_max);
   }
   
   amChangingimdata = true;

   if(!newdata)
   {
      changeImdataRecolorOnly();
   }
   else
   {
      //Update statistics
      imval = pixel(this, 0);//m_imData[0];
      tmp_min = imval;
      tmp_max = imval;
      saturated = 0;

      if(userBoxActive)
      {
         idx = userBox_j0*m_nx + userBox_i0;
         imval = pixel(this, idx); //m_imData[idx];
         userBox_min = imval;
         userBox_max = imval;
      }

      for(uint32_t i = 0; i < m_ny; ++i)
      {
         for(uint32_t j=0;j < m_nx; ++j)
         {
            idx = i*m_nx + j;
            imval = pixel(this, idx); //m_imData[idx];
            
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

    }
    qpm.convertFromImage(*qim, Qt::AutoColor | Qt::ThresholdDither);

    postChangeImdata();
    amChangingimdata = false;
}

void imviewer::changeImdataRecolorOnly()
{
   float (*pixel)(imviewer*, size_t) = &pixel_noCal;
   
   if(m_subtractDark == true && m_applyMask == false && m_images.size()>1)
   {
      pixel = &pixel_subDark;
   }
   
   if(m_subtractDark == false && m_applyMask == true  && m_images.size()>2)
   {
      pixel = &pixel_applyMask;
   }
   
   if(m_subtractDark == true && m_applyMask == true  && m_images.size()>1)
   {
      if(m_images.size() == 2) pixel = &pixel_subDark;
      else pixel = &pixel_subDarkApplyMask;
   }
   
   for(uint32_t i = 0; i < m_ny; ++i)
   {
      for(uint32_t j=0;j <m_nx; ++j)
      {
         qim->setPixel(j, m_ny-i-1, (int)calcPixval( pixel(this, i*m_nx + j) ));
      }
   }
}


float imviewer::calcPixval(float d)
{
   float pixval;
   static float a = 1000;
   static float log10_a = log10(a);

   pixval = (d - mindat)/((float)(maxdat-mindat));
   if(pixval < 0) pixval = 0;
   
   switch(colorbar_type)
   {
      case typelog:
         pixval = log10(pixval*a+1)/log10_a; 
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


void imviewer::set_mindat(float md)
{
   mindat = md;
}

void imviewer::set_maxdat(float md)
{
   maxdat = md;
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
            maxcol = 256;
            //qim->setNumColors(256);
            for(int i=mincol; i<maxcol; i++) qim->setColor(i, qRgb(i,0,0));
            warning_color = QColor("lime");
            break;
         case colorbarGreen:
            mincol = 0;
            maxcol = 256;
            //qim->setNumColors(256);
            for(int i=mincol; i<maxcol; i++) qim->setColor(i, qRgb(0,i,0));
            warning_color = QColor("magenta");
            break;
         case colorbarBlue:
            mincol = 0;
            maxcol = 256;
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
            maxcol = 256;
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
      m_timer.stop();
   }
   else
   {
      m_timer.start(m_timeout);
   }
}

void imviewer::updateFPS()
{
   return;
}

void imviewer::updateAge()
{
   return;
}
