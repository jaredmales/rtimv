/** \file rtimvGraphicsView.cpp
  * \brief Definitions for the graphics view class of RTImV
  * 
  * \author Jared R. Males (jaredmales@gmail.com)
  * 
  */

#include <QFontDatabase>

#include "rtimvGraphicsView.hpp"

#include <cmath>

//Geometries of the various text boxes.
#define SAVEWIDTH 30
#define SAVEHEIGHT 35
   
#define LOOPHEIGHT 35

#define WARNWIDTH  200
#define WARNHEIGHT  30

#define GAGEHEIGHT  35
#define COORDWIDTH 120
#define FPSWIDTH   250

#define ZOOMHEIGHT 30
#define ZOOMWIDTH 250

rtimvGraphicsView::rtimvGraphicsView(QWidget *parent): QGraphicsView(parent)
{
   setMouseTracking(true);
   setResizeAnchor(QGraphicsView::AnchorViewCenter);
   m_xCen = .5;
   m_yCen = .5;
   
   
   
   m_warningText = new QTextEdit(this);
   textEditSetup(m_warningText);
   warningFontSize(RTIMV_DEF_WARNINGFONTSIZE);
   warningFontColor(RTIMV_DEF_WARNINGFONTCOLOR);
   warningFontFamily(RTIMV_DEF_WARNINGFONTFAMILY);   
      
   m_fpsGage = new QTextEdit(this);
   textEditSetup(m_fpsGage);
   //fpsGageText("FPS: 0.000");
   
   m_textCoordX = new QTextEdit(this);
   textEditSetup(m_textCoordX);   
   textCoordX("");
      
   m_textCoordY = new QTextEdit(this);
   textEditSetup(m_textCoordY);   
   textCoordY("");
   
   m_textPixelVal = new QTextEdit(this);
   textEditSetup(m_textPixelVal);
   textPixelVal("");
   
   m_mouseCoords = new QTextEdit(this);
   textEditSetup(m_mouseCoords);
   m_mouseCoords->viewport()->setAutoFillBackground(false);
   m_mouseCoords->hide();
   m_mouseCoords->setTextBackgroundColor(QColor(0,0,0,125));

   gageFontFamily(RTIMV_DEF_GAGEFONTFAMILY);
   gageFontSize(RTIMV_DEF_GAGEFONTSIZE);
   gageFontColor(RTIMV_DEF_GAGEFONTCOLOR);
   
   m_loopText = new QTextEdit(this);
   textEditSetup(m_loopText);
   loopFontFamily(RTIMV_DEF_LOOPFONTFAMILY);
   loopFontSize(RTIMV_DEF_LOOPFONTSIZE);
   loopFontColor(RTIMV_DEF_LOOPFONTCOLOR);
 
   m_saveBox = new QTextEdit(this);
   textEditSetup(m_saveBox);
   saveBoxFontFamily(RTIMV_DEF_SAVEBOXFONTFAMILY);
   saveBoxFontSize(RTIMV_DEF_SAVEBOXFONTSIZE);
   saveBoxFontColor(RTIMV_DEF_SAVEBOXFONTCOLOR);
   m_saveBox->setText("");
   
   statusTextNo(10);
   
   
   m_zoomText = new QTextEdit(this);
   textEditSetup(m_zoomText);
   zoomFontSize(RTIMV_DEF_ZOOMFONTSIZE);
   zoomFontColor(RTIMV_DEF_ZOOMFONTCOLOR);
   zoomFontFamily(RTIMV_DEF_ZOOMFONTFAMILY);   
   connect(&m_zoomTimer, SIGNAL(timeout()), this, SLOT(zoomTimerOut()));
   zoomText("1.0x");
   
   m_userItemSize = new QTextEdit(this);
   textEditSetup(m_userItemSize);
   m_userItemSize->setGeometry(150, 150, 300, 50);
   
   QFont qf;
   qf = m_userItemSize->currentFont();
   qf.setPixelSize(14);
   m_userItemSize->setCurrentFont(qf);
   m_userItemSize->setVisible(false);
   m_userItemSize->setTextColor("lime");
   m_userItemSize->setWordWrapMode(QTextOption::NoWrap);

   m_userItemMouseCoords = new QTextEdit(this);
   textEditSetup(m_userItemMouseCoords);
   m_userItemMouseCoords->setGeometry(150, 150, 300, 50);
   m_userItemMouseCoords->setCurrentFont(qf);
   m_userItemMouseCoords->setVisible(false);
   m_userItemMouseCoords->setTextColor("lime");
   m_userItemMouseCoords->setWordWrapMode(QTextOption::NoWrap);


   m_helpText = new QTextEdit(this);
   textEditSetup(m_helpText);
   helpTextFontSize(RTIMV_DEF_HELPFONTSIZE);
   helpTextFontColor(RTIMV_DEF_HELPFONTCOLOR);
   helpTextFontFamily(RTIMV_DEF_HELPFONTFAMILY);   
   m_helpText->setVisible(false);
   
   return;
}

void rtimvGraphicsView::textEditSetup( QTextEdit * te )
{
   te->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
   te->viewport()->setAutoFillBackground(false);
   te->setVisible(true);
   te->setEnabled(false);
   te->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   te->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   te->setAttribute( Qt::WA_TransparentForMouseEvents );

}


void rtimvGraphicsView::warningFontFamily(const char * ff)
{
   m_warningFontFamily = ff;
   
   QFont qf = m_warningText->currentFont();
   qf.setFamily(m_warningFontFamily);
   
   m_warningText->setCurrentFont(qf);
}

void rtimvGraphicsView::warningFontSize( float fs )
{
   m_warningFontSize = fs;
   
   QFont qf = m_warningText->currentFont();
   //qf.setPointSizeF(m_warningFontSize);
   qf.setPixelSize(m_warningFontSize);

   m_warningText->setCurrentFont(qf);
}

void rtimvGraphicsView::warningFontColor(const char * fc)
{
   m_warningFontColor = fc;
   m_warningText->setTextColor(QColor(m_warningFontColor));
}

QString rtimvGraphicsView::warningFontFamily()
{
   return m_warningFontFamily;
}

float rtimvGraphicsView::warningFontSize()
{
   return m_warningFontSize;
}

QString rtimvGraphicsView::warningFontColor()
{
   return m_warningFontColor;
}
      
void rtimvGraphicsView::warningText( const char * nt,
                                const char * fc
                              )
{
   m_warningText->setPlainText(nt);
   m_warningText->setAlignment(Qt::AlignLeft);  
   
   if(fc)
   {
      m_warningText->setTextColor(QColor(fc));
   }
   else
   {
      m_warningText->setTextColor(QColor(m_warningFontColor));
   }
}

//-------------
void rtimvGraphicsView::loopFontFamily( const char * ff )
{
   m_loopFontFamily = ff;
   
   QFont qf = m_loopText->currentFont();
   qf.setFamily(m_loopFontFamily);
   
   m_loopText->setCurrentFont(qf);
}

void rtimvGraphicsView::loopFontSize( float fs )
{
   m_loopFontSize = fs;
   
   QFont qf = m_loopText->currentFont();
   //qf.setPointSizeF(m_loopFontSize);
   qf.setPixelSize(m_loopFontSize);

   m_loopText->setCurrentFont(qf);
}

void rtimvGraphicsView::loopFontColor( const char * fc )
{
   m_loopFontColor = fc;
   m_loopText->setTextColor(QColor(m_loopFontColor));
}

QString rtimvGraphicsView::loopFontFamily()
{
   return m_loopFontFamily;
}

float rtimvGraphicsView::loopFontSize()
{
   return m_loopFontSize;
}

QString rtimvGraphicsView::loopFontColor()
{
   return m_loopFontColor;
}
      
void rtimvGraphicsView::loopText( const char * nt,
                                  const char * fc
                                )
{
   m_loopText->setPlainText(nt);
   m_loopText->setAlignment(Qt::AlignRight);  
   
   if(fc)
   {
      m_loopText->setTextColor(QColor(fc));
   }
   else
   {
      m_loopText->setTextColor(QColor(m_loopFontColor));
   }
}



//-------------

void rtimvGraphicsView::saveBoxFontFamily(const char * ff)
{
   m_saveBoxFontFamily = ff;
   
   QFont qf = m_saveBox->currentFont();
   qf.setFamily(m_saveBoxFontFamily);
   
   m_saveBox->setCurrentFont(qf);
}

void rtimvGraphicsView::saveBoxFontSize( float fs )
{
   m_saveBoxFontSize = fs;
   
   QFont qf = m_saveBox->currentFont();
   //qf.setPointSizeF(m_saveBoxFontSize);
   qf.setPixelSize(m_saveBoxFontSize);

   m_saveBox->setCurrentFont(qf);
}

void rtimvGraphicsView::saveBoxFontColor(const char * fc)
{
   m_saveBoxFontColor = fc;
   m_saveBox->setTextColor(QColor(m_saveBoxFontColor));
}

QString rtimvGraphicsView::saveBoxFontFamily()
{
   return m_saveBoxFontFamily;
}

float rtimvGraphicsView::saveBoxFontSize()
{
   return m_saveBoxFontSize;
}

QString rtimvGraphicsView::saveBoxFontColor()
{
   return m_saveBoxFontColor;
}
      
void rtimvGraphicsView::saveBoxText( const char * nt,
                                   const char * fc
                                 )
{
   m_saveBox->setPlainText(nt);
   m_saveBox->setAlignment(Qt::AlignLeft);  
   
   if(fc)
   {
      m_saveBox->setTextColor(QColor(fc));
   }
   else
   {
      m_saveBox->setTextColor(QColor(m_saveBoxFontColor));
   }
   
}


//---------------------------------------------------------
// Status Text Array
//--------------------------------------------------------
void rtimvGraphicsView::statusTextNo(size_t no)
{
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      m_statusText[n]->deleteLater();
   }
   
   m_statusText.clear();
   
   m_statusText.resize(no);
   
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      m_statusText[n] =  new QTextEdit(this);
      textEditSetup(m_statusText[n]);
   }
   
   statusTextFontFamily(RTIMV_DEF_STATUSTEXTFONTFAMILY);
   statusTextFontSize(RTIMV_DEF_STATUSTEXTFONTSIZE);
   statusTextFontColor(RTIMV_DEF_STATUSTEXTFONTCOLOR);
   
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      statusTextText(n, "");
   }
}

size_t rtimvGraphicsView::statusTextNo()
{
   return m_statusText.size();
}


void rtimvGraphicsView::statusTextFontFamily( const char * ff /* [in] The new font family */ )
{
   m_statusTextFontFamily = ff;
   
   
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      QFont qf = m_statusText[n]->currentFont();
      qf.setFamily(m_statusTextFontFamily);
      m_statusText[n]->setCurrentFont(qf);
   }
}

void rtimvGraphicsView::statusTextFontSize( float fs /* [in] The new font size */ )
{
   m_statusTextFontSize = fs;
   
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      QFont qf = m_statusText[n]->currentFont();
      //qf.setPointSizeF(m_statusTextFontSize);
      qf.setPixelSize(m_statusTextFontSize);

      m_statusText[n]->setCurrentFont(qf);
   }
}


void rtimvGraphicsView::statusTextFontColor( const char * fc /* [in] The new font color */ )
{
   m_statusTextFontColor = fc;
   
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      m_statusText[n]->setTextColor(QColor(m_statusTextFontColor));
   }
}

QString rtimvGraphicsView::statusTextFontFamily()
{
   return m_statusTextFontFamily;
}

float rtimvGraphicsView::statusTextFontSize()
{
   return m_statusTextFontSize;
}

QString rtimvGraphicsView::statusTextFontColor()
{
   return m_statusTextFontColor;
}


void rtimvGraphicsView::statusTextText( size_t n,        ///< [in] the field number (0- statusTextNo()-1)
                                        const char * nt, ///< [in] the new statusText text
                                        const char * fc  ///< [in] [optional] color for the statusText text
                                      )
{
   if(n > m_statusText.size()-1)
   {
      std::cerr << "statusTextText: n too large for current vector size\n";
      return;
   }
   
   m_statusText[n]->setPlainText(nt);
   m_statusText[n]->setAlignment(Qt::AlignRight);  
   m_statusText[n]->setWordWrapMode(QTextOption::NoWrap);

   if(fc)
   {
      m_statusText[n]->setTextColor(QColor(fc));
   }
   else
   {
      m_statusText[n]->setTextColor(QColor(m_statusTextFontColor));
   }

   QFontMetrics fm(m_statusText[n]->currentFont());
   QSize textSize = fm.size(0, nt);
   m_statusText[n]->setGeometry(width()-(textSize.width()+5),LOOPHEIGHT+GAGEHEIGHT*n, textSize.width()+5, GAGEHEIGHT);

}

//-------------

void rtimvGraphicsView::helpTextFontFamily(const char * ff)
{
    ///\todo need to refactor this since we want to use fixed font all the time
    
    static_cast<void>(ff);
   //m_helpTextFontFamily = ff;
   
   //QFont qf = m_helpText->currentFont();
   //qf.setFamily(m_helpTextFontFamily);
   
   m_helpText->setCurrentFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void rtimvGraphicsView::helpTextFontSize( float fs )
{
   m_helpTextFontSize = fs;
   
   QFont qf = m_helpText->currentFont();
   //qf.setPointSizeF(m_helpTextFontSize);
   qf.setPixelSize(m_helpTextFontSize);

   m_helpText->setCurrentFont(qf);
}

void rtimvGraphicsView::helpTextFontColor(const char * fc)
{
   m_helpTextFontColor = fc;
   m_helpText->setTextColor(QColor(m_helpTextFontColor));
}

QString rtimvGraphicsView::helpTextFontFamily()
{
   return m_helpTextFontFamily;
}

float rtimvGraphicsView::helpTextFontSize()
{
   return m_helpTextFontSize;
}

QString rtimvGraphicsView::helpTextFontColor()
{
   return m_helpTextFontColor;
}
      
void rtimvGraphicsView::helpTextText( const char * nt,
                                   const char * fc
                                 )
{
   m_helpText->setPlainText(nt);
   m_helpText->setAlignment(Qt::AlignLeft);  
   
   if(fc)
   {
      m_helpText->setTextColor(QColor(fc));
   }
   else
   {
      m_helpText->setTextColor(QColor(m_helpTextFontColor));
   }
   
}

//-------------
const QTextEdit * rtimvGraphicsView::fpsGage()
{
   return m_fpsGage;
}

void rtimvGraphicsView::gageFontFamily(const char * ff)
{
   m_gageFontFamily = ff;
   
   QFont qf = m_fpsGage->currentFont();
   qf.setFamily(m_gageFontFamily);
   
   m_fpsGage->setCurrentFont(qf);
   m_textCoordX->setCurrentFont(qf);
   m_textCoordY->setCurrentFont(qf);
   m_textPixelVal->setCurrentFont(qf);
   m_mouseCoords->setCurrentFont(qf);
   
}

void rtimvGraphicsView::gageFontSize( float fs )
{
   m_gageFontSize = fs;
   
   QFont qf = m_fpsGage->currentFont();
   //qf.setPointSizeF(fs);
   qf.setPixelSize(fs);

   m_fpsGage->setCurrentFont(qf);
   m_textCoordX->setCurrentFont(qf);
   m_textCoordY->setCurrentFont(qf);
   m_textPixelVal->setCurrentFont(qf);
   m_mouseCoords->setCurrentFont(qf);
}

void rtimvGraphicsView::gageFontColor(const char * fc)
{
   m_gageFontColor = fc;
   m_fpsGage->setTextColor(QColor(m_gageFontColor));
   m_textCoordX->setTextColor(QColor(m_gageFontColor));
   m_textCoordY->setTextColor(QColor(m_gageFontColor));
   m_textPixelVal->setTextColor(QColor(m_gageFontColor));
   m_mouseCoords->setTextColor(QColor(m_gageFontColor));
}

QString rtimvGraphicsView::gageFontFamily()
{
   return m_gageFontFamily;
}

float rtimvGraphicsView::gageFontSize()
{
   return m_gageFontSize;
}

QString rtimvGraphicsView::gageFontColor()
{
   return m_gageFontColor;
}
      
void rtimvGraphicsView::fpsGageText( const char * nt )
{
   m_fpsGage->setPlainText(nt);
   m_fpsGage->setAlignment(Qt::AlignRight);  
}

void rtimvGraphicsView::fpsGageText( float nv,
                                bool age
                              )
{
   char strtmp[32];
   if(age)
   {
      snprintf(strtmp, 32, "Age: %.3f", nv);
   }
   else
   {
      snprintf(strtmp, 32, "FPS: %.3f", nv);
   }
   //std::cerr << strtmp << "\n";
   fpsGageText(strtmp);
}

void rtimvGraphicsView::textCoordX( const char * nt )
{
   m_textCoordX->setPlainText(nt);
   m_textCoordX->setAlignment(Qt::AlignLeft);  
}      

void rtimvGraphicsView::textCoordX( float nv )
{
   char strtmp[24];
   snprintf(strtmp, sizeof(strtmp), "X: %0.1f", nv);
   textCoordX(strtmp);   
}

void rtimvGraphicsView::textCoordY( const char * nt )
{
   m_textCoordY->setPlainText(nt);
   m_textCoordY->setAlignment(Qt::AlignLeft);  
}            

void rtimvGraphicsView::textCoordY( float nv )
{
   char strtmp[24];
   snprintf(strtmp, sizeof(strtmp), "Y: %0.1f", nv);
   textCoordY(strtmp);   
}

void rtimvGraphicsView::textPixelVal( const char * nt )
{
   m_textPixelVal->setPlainText(nt);
   m_textPixelVal->setAlignment(Qt::AlignLeft);  
}      

void rtimvGraphicsView::showMouseToolTip( const std::string & valStr, 
                                          const std::string & posStr,
                                          const QPoint & pt
                                        )
{
   std::string str;

   m_mouseCoords->setWordWrapMode(QTextOption::NoWrap);
   
   int x = pt.x();
   int y = pt.y();

   //First construct the text in correct order for y position
   if(y > this->height()*0.5) 
   {
      std::string sp;
      if(x > this->width()*0.5)
      {
         //Right justify the value string if needed
         if(posStr.size() > valStr.size())
         {
            sp.append(posStr.size() - valStr.size(), ' ');
         }
      }
      str = posStr + "\n" + sp + valStr;
   }
   else
   {
      str = valStr + "\n" + posStr;
   }
   
   m_mouseCoords->setPlainText(str.c_str());

   QFontMetrics fm(m_mouseCoords->currentFont());
   QSize textSize = fm. size(0, str.c_str());
   m_mouseCoords->resize(textSize.width()+5, textSize.height());

   if(x > this->width()*0.5) 
   {
      x -= m_mouseCoords->width();
      if(x < 0) x = 0;
      m_mouseCoords->setAlignment(Qt::AlignRight);
   }
   else
   {
      if(y < this->height()*0.5) x+=10; //have to move past the arrow
      m_mouseCoords->setAlignment(Qt::AlignLeft);
   }

   if(y > this->height()*0.5) 
   {
      y-= m_mouseCoords->height();
   }
   
   m_mouseCoords->move(QPoint(x,y));
   m_mouseCoords->show();
   
}

void rtimvGraphicsView::hideMouseToolTip()
{
   m_mouseCoords->hide();
}

void rtimvGraphicsView::textPixelVal( float nv )
{
   //1.00e+5
   //10000.0
   //1000.0
   //100.0
   //10.0
   //1.0
   //0.10
   //0.010
   //0.0010
   //0.00010
   //1.0e-3
   char strtmp[24];

   float anv = fabs(nv);
   if(anv == 0)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: 0");
   }
   else if(anv >= 100000)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.2e", nv);
   }
   else if(anv >= 1.0)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.1f", nv);
   } 
   else if(anv >= 0.1)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.2f", nv);
   }
   else if(anv >= 0.01)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.3f", nv);
   }
   else if(anv >= 0.001)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.4f", nv);
   }
   else if(anv >= 0.0001)
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.5f", nv);
   }
   else
   {
      snprintf(strtmp, sizeof(strtmp), "Z: %0.1e", nv);
   }

   textPixelVal(strtmp);   
}

//----------------

void rtimvGraphicsView::zoomFontFamily( const char * ff )
{
   m_zoomFontFamily = ff;
   
   QFont qf = m_zoomText->currentFont();
   qf.setFamily(m_zoomFontFamily);
   
   m_zoomText->setCurrentFont(qf);
}

void rtimvGraphicsView::zoomFontSize( float fs )
{
   m_zoomFontSize = fs;
   
   QFont qf = m_zoomText->currentFont();
   //qf.setPointSizeF(m_zoomFontSize);
   qf.setPixelSize(m_zoomFontSize);

   m_zoomText->setCurrentFont(qf);
}

void rtimvGraphicsView::zoomFontColor( const char * fc )
{
   m_zoomFontColor = fc;
   m_zoomText->setTextColor(QColor(m_zoomFontColor));
}

QString rtimvGraphicsView::zoomFontFamily()
{
   return m_zoomFontFamily;
}

float rtimvGraphicsView::zoomFontSize()
{
   return m_zoomFontSize;
}

QString rtimvGraphicsView::zoomFontColor()
{
   return m_zoomFontColor;
}
    
void rtimvGraphicsView::zoomText( const char * nt,
                             const char * fc
                           )
{
   m_zoomText->setPlainText(nt);
   m_zoomText->setAlignment(Qt::AlignRight);  
   
   if(fc)
   {
      m_zoomText->setTextColor(QColor(fc));
   }
   else
   {
      m_zoomText->setTextColor(QColor(m_zoomFontColor));
   }
   
   m_zoomTimer.start( m_zoomTimeOut );
}

void rtimvGraphicsView::zoomTimerOut()
{
   m_zoomText->setText("");
   m_zoomTimer.stop();
}
      
void rtimvGraphicsView::centerOn( qreal x, 
                                  qreal y 
                                )
{
   m_xCen = x;
   m_yCen = y;
   
   QGraphicsView::centerOn(x,y);
}
      
void rtimvGraphicsView::mapCenterToScene( float xc,
                                          float yc
                                       )  
{
   QPointF p = mapToScene(xc, yc);
   m_xCen = p.x();
   m_yCen = p.y();
}

float rtimvGraphicsView::xCen()
{
   return m_xCen;
}


float rtimvGraphicsView::yCen()
{
   return m_yCen;
}

float rtimvGraphicsView::mouseViewX()
{
   return m_mouseViewX;
}
      
float rtimvGraphicsView::mouseViewY()
{
   return m_mouseViewY;
}

void rtimvGraphicsView::zoomLevel( float zl )
{
   m_zoomLevel = zl;
}

float rtimvGraphicsView::zoomLevel()
{
   return m_zoomLevel;
}
      
void rtimvGraphicsView::screenZoom( float sz )
{
   m_screenZoom = sz;
}

float rtimvGraphicsView::screenZoom()
{
   return m_screenZoom;
}
      
void rtimvGraphicsView::resizeEvent(QResizeEvent *)
{
   m_helpText->setGeometry(0,0,width(), height());
   
   m_saveBox->setGeometry(1, 1, SAVEWIDTH, SAVEHEIGHT);
   
   m_loopText->setGeometry(SAVEWIDTH, 1, width()-SAVEWIDTH, LOOPHEIGHT);
      
   for(size_t n=0; n<m_statusText.size(); ++n)
   {
      m_statusText[n]->setGeometry(1,LOOPHEIGHT+GAGEHEIGHT*n, width(), GAGEHEIGHT);
   }
   
   m_warningText->setGeometry(0, height()-GAGEHEIGHT-WARNHEIGHT, WARNWIDTH, WARNHEIGHT);

   m_fpsGage->setGeometry(width()-FPSWIDTH, height()-GAGEHEIGHT, FPSWIDTH, GAGEHEIGHT);
   m_textCoordX->setGeometry(0, height()-GAGEHEIGHT, COORDWIDTH, GAGEHEIGHT);
   m_textCoordY->setGeometry(0+COORDWIDTH, height()-GAGEHEIGHT, COORDWIDTH, GAGEHEIGHT);
   m_textPixelVal->setGeometry(0+COORDWIDTH+COORDWIDTH, height()-GAGEHEIGHT, COORDWIDTH+50, GAGEHEIGHT);   
   
   m_zoomText->setGeometry(width()-ZOOMWIDTH, height()-GAGEHEIGHT-ZOOMHEIGHT, ZOOMWIDTH, ZOOMHEIGHT);
}

void rtimvGraphicsView::mouseMoveEvent(QMouseEvent *e)
{
   m_mouseViewX = e->pos().x();
   m_mouseViewY = e->pos().y();
   
   emit mouseCoordsChanged();
      
   QGraphicsView::mouseMoveEvent(e);
}

void rtimvGraphicsView::leaveEvent(QEvent *)
{
   m_mouseViewX = -1;
   m_mouseViewY = -1;
   emit mouseCoordsChanged();
}

void rtimvGraphicsView::mousePressEvent(QMouseEvent *e)
{
   if(e->button() == Qt::LeftButton)
   {
      emit leftPressed(e->pos());//mp);
      QGraphicsView::mousePressEvent(e);
      return;
   }
   
   if(e->button() == Qt::RightButton)
   {
      emit rightPressed(e->pos());//mp);
   }
   else QGraphicsView::mousePressEvent(e);
}

void rtimvGraphicsView::mouseReleaseEvent(QMouseEvent *e)
{
   if(e->button()  == Qt::MidButton)
   {
      mapCenterToScene(e->pos().x(), e->pos().y());
      emit centerChanged();
   }
   
   if(e->button() == Qt::LeftButton)
   {
      emit leftClicked(e->pos());//mp);
      QGraphicsView::mouseReleaseEvent(e);
   }
   
   if(e->button() == Qt::RightButton)
   {
      emit rightClicked(e->pos());//mp);
   }
}

void rtimvGraphicsView::mouseDoubleClickEvent(QMouseEvent *e)
{
   (void)(e);
}

void rtimvGraphicsView::wheelEvent(QWheelEvent *e)
{
   emit wheelMoved(e->angleDelta().y());
}


