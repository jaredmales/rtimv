
#include "imviewer.hpp"

imviewer * globalIMV;

int imviewer::sigsegvFd[2];

imviewer::imviewer( QWidget * Parent, 
                    Qt::WindowFlags f
                  ) : QWidget(Parent, f)
{
}

imviewer::imviewer( const std::vector<std::string> & shkeys, 
                    QWidget * Parent, 
                    Qt::WindowFlags f
                  ) : QWidget(Parent, f)
{
   startup(shkeys);
}

void imviewer::startup( const std::vector<std::string> & shkeys )
{
   m_images.resize(4, nullptr);
   
   for(size_t i=0; i< m_images.size(); ++i)
   {
      if(shkeys.size() > i)
      {
         if(shkeys[i] != "")
         {
            m_images[i] = new rtimvImage;
            m_images[i]->shmimName(shkeys[i]); // Set the key
            m_images[i]->shmimTimerout(); // And start checking for the image
         }
      }
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

   if(m_nx !=x  || m_ny !=y  || m_qim == 0)
   {
      if(x!=0 && y!=0)
      {
         m_nx = x;
         m_ny = y;

         if(m_qim) delete m_qim;

         m_qim = new QImage(m_nx, m_ny, QImage::Format_Indexed8);

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
   
   int doupdate = RTIMVIMAGE_NOUPDATE;
   
   if(m_images[0] != nullptr) doupdate = m_images[0]->update();
   
   for(size_t i=1;i<m_images.size(); ++i) 
   {
      if(m_images[i] != nullptr) m_images[i]->update();
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
   
   if(!connected) return;
   
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
      if(m_images[i] != nullptr) m_images[i]->timeout(to); //just for fps calculations
   }
   
   m_timer.start(to);
}

imviewer::pixelF imviewer::pixel()
{
   pixelF _pixel = nullptr;
   
   if(m_images[0] == nullptr) return _pixel; //no valid base image
   
   if(m_images[0]->valid()) _pixel =  &pixel_noCal; //default if there is a valid base image.
   else return _pixel; //no valid base image
   
   if(m_subtractDark == true)// && m_applyMask == false)
   {
      if(m_images[1] == nullptr) return _pixel;
      
      if(m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny()) return _pixel;
      
      if(m_images[0]->valid() && m_images[1]->valid()) _pixel = &pixel_subDark;
   }
   
   if(m_subtractDark == false && m_applyMask == true)
   {
      if(m_images[2] == nullptr) return _pixel;
      
      if(m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny()) return _pixel;
      
      if(m_images[0]->valid() && m_images[2]->valid()) _pixel = &pixel_applyMask;
   }
   
   if(m_subtractDark == true && m_applyMask == true)
   {
      
      if( m_images[1] == nullptr && m_images[2] == nullptr) return _pixel;
      else if( m_images[2] == nullptr )
      {
         if(m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny()) return _pixel;
         if(m_images[1]->valid()) _pixel = &pixel_subDark;
      } 
      else if(m_images[1] == nullptr)
      {
         if(m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny()) return _pixel;
         if(m_images[2]->valid()) _pixel = &pixel_applyMask;
      }
      else
      {
         if(m_images[1]->nx() != m_images[0]->nx() || m_images[1]->ny() != m_images[0]->ny()) return _pixel;
         if(m_images[2]->nx() != m_images[0]->nx() || m_images[2]->ny() != m_images[0]->ny()) return _pixel;
         if(m_images[1]->valid() &&  m_images[2]->valid()) _pixel = &pixel_subDarkApplyMask;
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

// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color/56678483#56678483
template<typename realT>
realT sRGBtoLinRGB( int rgb )
{
   realT V = ((realT) rgb)/255.0;
   
   if( V <= 0.0405 ) return V/12.92;
   
   return pow( (V+0.055)/1.055, 2.4);
}

template<typename realT>
realT linRGBtoLuminance( realT linR,
                         realT linG,
                         realT linB
                       )
{
   return  0.2126*linR + 0.7152*linG + 0.0722*linB;
}

template<typename realT>
realT pLightness( realT lum )
{
   if(lum <= static_cast<realT>(216)/static_cast<realT>(24389))
   {
      return lum*static_cast<realT>(24389)/static_cast<realT>(27);
   }
   
   return pow(lum, static_cast<realT>(1)/static_cast<realT>(3))*116 - 16;
      
}

void imviewer::load_colorbar(int cb)
{
   if(current_colorbar != cb && m_qim)
   {
      current_colorbar = cb;
      switch(cb)
      {
         case colorbarJet:
            m_mincol = 0;
            m_maxcol = load_colorbar_jet(m_qim);
            m_maskcol = m_maxcol + 1;
            m_satcol = m_maxcol + 2;
            warning_color = QColor("white");
            break;
         case colorbarHot:
            m_mincol = 0;
            m_maxcol = load_colorbar_hot(m_qim);
            m_maskcol = m_maxcol + 1;
            m_satcol = m_maxcol + 2;
            warning_color = QColor("cyan");
            break;
         case colorbarBone:
            m_mincol = 0;
            m_maxcol = load_colorbar_bone(m_qim);
            m_maskcol = m_maxcol + 1;
            m_satcol = m_maxcol + 2;
            warning_color = QColor("lime");
            break;
         default:
            m_mincol = 0;
            m_maxcol = 253;
            m_maskcol = m_maxcol + 1;
            m_satcol = m_maxcol + 2;
            for(int i=m_mincol; i <= m_maxcol; i++) 
            {
               int c = (( (float) i) / 253. * 255.) + 0.5;
               m_qim->setColor(i, qRgb(c,c,c));
            }
            m_qim->setColor(254, qRgb(0,0,0));
            m_qim->setColor(255, qRgb(255,0,0));
            
            warning_color = QColor("lime");
            break;
      }
      
      
      m_lightness.resize(256);
      
      for(int n=0;n<256;++n)
      {
         //QRgb rgb = m_qim->color(n);
         
         //m_lightness[n] = pLightness(linRGBtoLuminance( sRGBtoLinRGB<double>(qRed(rgb)), sRGBtoLinRGB<double>(qGreen(rgb)), sRGBtoLinRGB<double>(qBlue(rgb))));
         m_lightness[n] = QColor(m_qim->color(n)).lightness();
      }
      
      changeImdata();
   }
}

void imviewer::set_cbStretch(int ct)
{
   if(ct < 0 || ct >= cbStretches_max)
   {
      ct = stretchLinear;
   }

   m_cbStretch = ct;

}

int imviewer::get_cbStretch()
{
   return m_cbStretch;
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


int calcPixIndex_linear( float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
   //We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
   pixval = (pixval - mindat)/((float)(maxdat-mindat));
   if(pixval < 0) return 0;
   
   //Clamp it to <= 1
   if(pixval > 1.) pixval = 1.;

   //And finally put it in the color bar index range
   return pixval*(maxcol-mincol) + 0.5;
}

int calcPixIndex_log( float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
   static float a = 1000;
   static float log10_a = log10(a);
   
   //We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
   pixval = (pixval - mindat)/((float)(maxdat-mindat));
   if(pixval < 0) return 0;
   
   pixval = log10(pixval*a+1)/log10_a; 
   
   //Clamp it to <= 1
   if(pixval > 1.) pixval = 1.;

   //And finally put it in the color bar index range
   return pixval*(maxcol-mincol) + 0.5;
}

int calcPixIndex_pow( float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
   static float a = 1000;
   
   //We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
   pixval = (pixval - mindat)/((float)(maxdat-mindat));
   if(pixval < 0) return 0;
   
   pixval = (pow(a, pixval))/a;
   
   //Clamp it to <= 1
   if(pixval > 1.) pixval = 1.;

   //And finally put it in the color bar index range
   return pixval*(maxcol-mincol) + 0.5;
}

int calcPixIndex_sqrt( float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
   //We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
   pixval = (pixval - mindat)/((float)(maxdat-mindat));
   if(pixval < 0) return 0;
   
   pixval = sqrt(pixval);
   
   //Clamp it to <= 1
   if(pixval > 1.) pixval = 1.;

   //And finally put it in the color bar index range
   return pixval*(maxcol-mincol) + 0.5;
}

int calcPixIndex_square( float pixval, float mindat, float maxdat, int mincol, int maxcol)
{
   //We first produce a value nominally between 0 and 1, though depending on the range it could be > 1.
   pixval = (pixval - mindat)/((float)(maxdat-mindat));
   if(pixval < 0) return 0;
   
   pixval = pixval*pixval;
   
   //Clamp it to <= 1
   if(pixval > 1.) pixval = 1.;

   //And finally put it in the color bar index range
   return pixval*(maxcol-mincol) + 0.5;
}

void imviewer::changeImdata(bool newdata)
{
   float tmp_min;
   float tmp_max;

   int idx;
   float imval;

   if(m_images[0] == nullptr) return;
   if(!m_images[0]->valid()) return;

   //Get the pixel calculating function
   float (*_pixel)(imviewer*, size_t) = pixel();
   
   //Get the color index calculating function
   int (*_index)(float,float,float,int,int);
   switch(m_cbStretch)
   {
      case stretchLog:
         _index = calcPixIndex_log;
         break;
      case stretchPow:
         _index = calcPixIndex_pow;
         break;
      case stretchSqrt:
         _index = calcPixIndex_sqrt;
         break;
      case stretchSquare:
         _index = calcPixIndex_square;
         break;
      default:
         _index = calcPixIndex_linear;
   }
   
   if(m_images[0]->nx() != m_nx || m_images[0]->ny() != m_ny || m_autoScale) 
   {
      setImsize(m_images[0]->nx(), m_images[0]->ny());
      
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
               m_qim->setPixel(j, m_ny-i-1, 0);
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
               m_qim->setPixel(j, m_ny-i-1, _index(imval,m_mindat, m_maxdat, m_mincol, m_maxcol));
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

      if(colorBoxActive)
      {
         idx = colorBox_j0*m_nx + colorBox_i0;
         imval = _pixel(this, idx); //m_imData[idx];
         colorBox_min = imval;
         colorBox_max = imval;
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
      
               if(colorBoxActive)
               {
                  if(i>=colorBox_i0 && i<colorBox_i1 && j>=colorBox_j0 && j < colorBox_j1)
                  {
                     if(imval < colorBox_min) colorBox_min = imval;
                     if(imval > colorBox_max) colorBox_max = imval;
                  }
               }
      
               m_qim->setPixel(j, m_ny-i-1, 0);
      
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
      
               if(colorBoxActive)
               {
                  if(i>=colorBox_i0 && i<colorBox_i1 && j>=colorBox_j0 && j < colorBox_j1)
                  {
                     if(imval < colorBox_min) colorBox_min = imval;
                     if(imval > colorBox_max) colorBox_max = imval;
                  }
               }
      
               m_qim->setPixel(j, m_ny-i-1, _index(imval,m_mindat, m_maxdat, m_mincol, m_maxcol));
      
            }
         }
      }
   
      imdat_max = tmp_max;
      imdat_min = tmp_min;
      
    }
    
   if(m_applyMask && m_images[2] != nullptr)
   {
      if(m_images[2]->nx() == m_images[0]->nx() || m_images[2]->ny() == m_images[0]->ny())
      {
         for(uint32_t i = 0; i < m_ny; ++i)
         {
            for(uint32_t j=0;j < m_nx; ++j)
            {
               idx = i*m_nx + j;
               if( m_images[2]->pixel(idx) == 0 ) m_qim->setPixel(j, m_ny-i-1, m_maskcol);
            }
         }
      }
   }

   if(m_applySatMask && m_images[3] != nullptr)
   {
      if(m_images[3]->nx() == m_images[0]->nx() || m_images[3]->ny() == m_images[0]->ny())
      {
         for(uint32_t i = 0; i < m_ny; ++i)
         {
            for(uint32_t j=0;j < m_nx; ++j)
            {
               idx = i*m_nx + j;
               if( m_images[3]->pixel(idx) == 1 ) m_qim->setPixel(j, m_ny-i-1, m_satcol);
            }
         }
      }
   }
   
    m_qpm.convertFromImage(*m_qim, Qt::AutoColor | Qt::ThresholdDither);

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

      if(colorBox_i0 > colorBox_i1)
      {
         idx = colorBox_i0;
         colorBox_i0 = colorBox_i1;
         colorBox_i1 = idx;
      }

      if(colorBox_i0 < 0) colorBox_i0 = 0;
      if(colorBox_i0 >= (int64_t) m_nx) colorBox_i0 = (int64_t) m_nx-(colorBox_i1-colorBox_i0);

      if(colorBox_i1 <= 0) colorBox_i1 = 0 + (colorBox_i1-colorBox_i0);
      if(colorBox_i1 > (int64_t) m_nx) colorBox_i1 = (int64_t)m_nx-1;

      if(colorBox_j0 > colorBox_j1)
      {
         idx = colorBox_j0;
         colorBox_j0 = colorBox_j1;
         colorBox_j1 = idx;
      }

      if(colorBox_j0 < 0) colorBox_j0 = 0;
      if(colorBox_j0 >= (int64_t) m_nx) colorBox_j0 = (int64_t)m_ny-(colorBox_j1-colorBox_j0);

      if(colorBox_j1 <= 0) colorBox_j1 = 0 + (colorBox_j1-colorBox_j0);
      if(colorBox_j1 > (int64_t) m_ny) colorBox_j1 = (int64_t)m_ny-1;

      pixelF _pixel = pixel();
   
      idx = colorBox_j0*m_nx + colorBox_i0;
      
      imval = _pixel(this, idx); //m_imData[idx];

      colorBox_min = imval;
      colorBox_max = imval;
      for(int i = colorBox_i0; i < colorBox_i1; i++)
      {
         for(int j = colorBox_j0; j < colorBox_j1; j++)
         {
            idx = j*m_nx + i;
            imval = _pixel(this, idx);// m_imData[idx];

            if(imval < colorBox_min) colorBox_min = imval;
            if(imval > colorBox_max) colorBox_max = imval;
         }
      }

      mindat(colorBox_min);
      maxdat(colorBox_max);
      colorBoxActive = usba;
      set_colorbar_mode(minmaxbox);
      changeImdata(false);
      return;
   }
   colorBoxActive = usba;

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
      if(m_images[i] != nullptr) m_images[i]->detach();
   }
   
   snSegv->setEnabled(true);
}
