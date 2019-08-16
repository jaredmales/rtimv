
#include "imviewer.hpp"

imviewer * globalIMV;

int imviewer::sigsegvFd[2];

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
   
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(timerout()));
   
   //Install signal handling
   
   if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigsegvFd))
       qFatal("Couldn't create TERM socketpair");
   
   snSegv = new QSocketNotifier(sigsegvFd[1], QSocketNotifier::Read, this);
   connect(snSegv, SIGNAL(activated(int)), this, SLOT(handleSigSegv()));
    
   globalIMV = this;
   
   struct sigaction act;
   sigset_t set;

   act.sa_sigaction = &imviewer::st_handleSigSegv;
   act.sa_flags = SA_SIGINFO;
   sigemptyset(&set);
   act.sa_mask = set;

   errno = 0;
   if( sigaction(SIGBUS, &act, 0) < 0 )
   {
      perror("rtimv: error installing SIGBUS handler");
   }
   
   if( sigaction(SIGSEGV, &act, 0) < 0 )
   {
      perror("rtimv: error installing SIGSEGV handler");
   }
   
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

uint32_t imviewer::nx()
{
   return m_nx;
}

uint32_t imviewer::ny()
{
   return m_ny;
}

void imviewer::timerout()
{
   static bool connected = false;
   
   int doupdate = m_images[0]->update();
   
   for(size_t i=1;i<m_images.size(); ++i) 
   {
      m_images[i]->update();
   }
   
   if(doupdate >= RTIMVIMAGE_IMUPDATE) 
   {
      changeImdata(true);
      
      if(!connected)
      {
         onConnect();
         connected = true;
      }
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

void imviewer::timeout(int to)
{
   m_timer.stop();
   
   for(size_t i=0; i<m_images.size();++i)
   {
      m_images[i]->timeout(to); //just for fps calculations
   }
   
   m_timer.start(to);
}

imviewer::pixelF imviewer::pixel()
{
   pixelF _pixel = nullptr;
   
   if(m_images[0]->valid()) _pixel =  &pixel_noCal; //default if there is a valid base image.
   
   if(m_subtractDark == true && m_applyMask == false && m_images.size()>1)
   {
      if(m_images[0]->valid() && m_images[1]->valid()) _pixel = &pixel_subDark;
   }
   
   if(m_subtractDark == false && m_applyMask == true && m_images.size()>2)
   {
      if(m_images[0]->valid() && m_images[2]->valid()) _pixel = &pixel_applyMask;
   }
   
   if(m_subtractDark == true && m_applyMask == true && m_images.size()>1)
   {
      if(m_images.size()==1)
      {
         if(m_images[0]->valid() && m_images[1]->valid()) _pixel = &pixel_subDark;
      }      
      else
      {
         if(m_images[0]->valid() && m_images[1]->valid() &&  m_images[2]->valid()) _pixel = &pixel_subDarkApplyMask;
      }
   }
   
   return _pixel;
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

}

void imviewer::mindat(float md)
{
   m_mindat = md;
}

float imviewer::mindat()
{
   return m_mindat;
}
      
      
void imviewer::maxdat(float md)
{
   m_maxdat = md;
}

float imviewer::maxdat()
{
   return m_maxdat;
}

void imviewer::bias(float b)
{
   float cont = contrast();

   mindat(b - 0.5*cont);
   maxdat(b + 0.5*cont);
}

float imviewer::bias()
{
   return 0.5*(m_maxdat+m_mindat);
}

void imviewer::bias_rel(float br)
{
   float cont = contrast();

   mindat(imdat_min + br*(imdat_max-imdat_min) - 0.5*cont);
   maxdat(imdat_min + br*(imdat_max-imdat_min) + 0.5*cont);
}

float imviewer::bias_rel()
{
   return 0.5*(m_maxdat+m_mindat)/(m_maxdat-m_mindat);
}

void imviewer::contrast(float c)
{
   float b = bias();
   mindat(b - 0.5*c);
   maxdat(b + 0.5*c);
}

float imviewer::contrast()
{
   return m_maxdat-m_mindat;
}
   
float imviewer::contrast_rel()
{
   return (imdat_max-imdat_min)/(m_maxdat-m_mindat);
}

void imviewer::contrast_rel(float cr)
{
   float b = bias();
   mindat(b - .5*(imdat_max-imdat_min)/cr);
   maxdat(b + .5*(imdat_max-imdat_min)/cr);
}

int imviewer::calcPixIndex(float d)
{
   float pixval;
   static float a = 1000;
   static float log10_a = log10(a);

   pixval = (d - m_mindat)/((float)(m_maxdat-m_mindat));
   if(pixval < 0) pixval = 0;
   
   switch(colorbar_type)
   {
      case typelog:
         pixval = log10(pixval*a+1)/log10_a; 
         break;
      case typepow:
         pixval = (pow(a, pixval))/a;
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

   return pixval*(maxcol-1-mincol);
}

void imviewer::changeImdata(bool newdata)
{
   float tmp_min;
   float tmp_max;

   int idx;
   float imval;

   if(!m_images[0]->valid()) return;

   float (*_pixel)(imviewer*, size_t) = pixel();
   
   if(m_images[0]->m_nx != m_nx || m_images[0]->m_ny != m_ny || m_autoScale) 
   {
      setImsize(m_images[0]->m_nx, m_images[0]->m_ny);
      
      //Need to set these at the beginning
      imdat_min = _pixel(this,0);
      imdat_max = _pixel(this,0);
      for(uint32_t i = 0; i < m_ny; ++i)
      {
         for(uint32_t j=0;j < m_nx; ++j)
         {
            if(_pixel(this, i*m_nx + j) > imdat_max) imdat_max = _pixel(this, i*m_nx + j);
            if(_pixel(this, i*m_nx + j) < imdat_min) imdat_min = _pixel(this, i*m_nx + j) ;

         }
      }
      mindat(imdat_min);
      maxdat(imdat_max);
   }
   
   amChangingimdata = true;

   if(!newdata)
   {
      if( m_mindat == m_maxdat )
      {
         for(uint32_t i = 0; i < m_ny; ++i)
         {
            for(uint32_t j=0;j < m_nx; ++j)
            {
               qim->setPixel(j, m_ny-i-1, 0);
            }
         }
      }
      else
      {
         for(uint32_t i = 0; i < m_ny; ++i)
         {
            for(uint32_t j=0;j <m_nx; ++j)
            {
               idx = i*m_nx + j;
               imval = _pixel(this, idx);
               qim->setPixel(j, m_ny-i-1, calcPixIndex(imval));
            }
         }
      }
   }
   else
   {
      //Update statistics
      imval = _pixel(this, 0);//m_imData[0];
      tmp_min = imval;
      tmp_max = imval;
      saturated = 0;

      if(userBoxActive)
      {
         idx = userBox_j0*m_nx + userBox_i0;
         imval = _pixel(this, idx); //m_imData[idx];
         userBox_min = imval;
         userBox_max = imval;
      }

      if( m_mindat == m_maxdat )
      {
         for(uint32_t i = 0; i < m_ny; ++i)
         {
            for(uint32_t j=0;j < m_nx; ++j)
            {
               idx = i*m_nx + j;
               imval = _pixel(this, idx); //m_imData[idx];
               
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
      
               qim->setPixel(j, m_ny-i-1, 0);
      
            }
         }
      }
      else
      {
         for(uint32_t i = 0; i < m_ny; ++i)
         {
            for(uint32_t j=0;j < m_nx; ++j)
            {
               idx = i*m_nx + j;
               imval = _pixel(this, idx); //m_imData[idx];
               
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
      
               qim->setPixel(j, m_ny-i-1, calcPixIndex(imval));
      
            }
         }
      }
   
      imdat_max = tmp_max;
      imdat_min = tmp_min;
      
    }
    qpm.convertFromImage(*qim, Qt::AutoColor | Qt::ThresholdDither);

    postChangeImdata();
    amChangingimdata = false;
}

void imviewer::postChangeImdata()
{
   return;
}

void imviewer::zoomLevel(float zl)
{
   if(zl < m_zoomLevelMin) zl = m_zoomLevelMin;
   if(zl > m_zoomLevelMax) zl = m_zoomLevelMax;

   m_zoomLevel = zl;

   post_zoomLevel();
}

void imviewer::post_zoomLevel()
{
   return;
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

      pixelF _pixel = pixel();
   
      idx = userBox_j0*m_nx + userBox_i0;
      
      imval = _pixel(this, idx); //m_imData[idx];

      userBox_min = imval;
      userBox_max = imval;
      for(int i = userBox_i0; i < userBox_i1; i++)
      {
         for(int j = userBox_j0; j < userBox_j1; j++)
         {
            idx = j*m_nx + i;
            imval = _pixel(this, idx);// m_imData[idx];

            if(imval < userBox_min) userBox_min = imval;
            if(imval > userBox_max) userBox_max = imval;
         }
      }

      mindat(userBox_min);
      maxdat(userBox_max);
      userBoxActive = usba;
      set_colorbar_mode(minmaxbox);
      changeImdata(false);
      return;
   }
   userBoxActive = usba;

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

void imviewer::st_handleSigSegv( int signum,
                                 siginfo_t *siginf,
                                 void *ucont
                               )
{
   static_cast<void>(signum);
   static_cast<void>(siginf);
   static_cast<void>(ucont);
   
    char a = 1;
    int rv = ::write(sigsegvFd[0], &a, sizeof(a));
    
    static_cast<void>(rv);
}

void imviewer::handleSigSegv()
{
   snSegv->setEnabled(false);
   
   char tmp;
   int rv = ::read(sigsegvFd[1], &tmp, sizeof(tmp));
   static_cast<void>(rv); 
   
   std::cerr << "\n\n****** sigbus/sigterm ******\n" << amChangingimdata << "\n" << std::endl;
     
   for(size_t i=1;i<m_images.size(); ++i) 
   {
      m_images[i]->detach();
   }
   
   snSegv->setEnabled(true);
}
