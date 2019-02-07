/** \file graphicsview.cpp
  * \brief Definitions for the graphics view class of RTImV
  * 
  * \author Jared R. Males (jaredmales@gmail.com)
  * 
  */

#include "graphicsview.hpp"


//Geometries of the various text boxes.
#define SAVEWIDTH 30
#define SAVEHEIGHT 35
   
#define LOOPHEIGHT 35

#define WARNWIDTH  200
#define WARNHEIGHT  30

#define GAGEHEIGHT  35
#define COORDWIDTH 100
#define FPSWIDTH   150

#define ZOOMHEIGHT 30
#define ZOOMWIDTH 150

graphicsview::graphicsview(QWidget *parent): QGraphicsView(parent)
{
   setMouseTracking(true);
   
   m_xCen = .5;
   m_yCen = .5;
   
   m_warningText = new QTextEdit(this);
   textEditSetup(m_warningText);
   warningFontSize(RTIMV_DEF_WARNINGFONTSIZE);
   warningFontColor(RTIMV_DEF_WARNINGFONTCOLOR);
   warningFontFamily(RTIMV_DEF_WARNINGFONTFAMILY);   
      
   m_fpsGage = new QTextEdit(this);
   textEditSetup(m_fpsGage);
   fpsGageText("FPS: 0.000");
   
   m_textCoordX = new QTextEdit(this);
   textEditSetup(m_textCoordX);   
   textCoordX("");
      
   m_textCoordY = new QTextEdit(this);
   textEditSetup(m_textCoordY);   
   textCoordY("");
   
   m_textPixelVal = new QTextEdit(this);
   textEditSetup(m_textPixelVal);
   textPixelVal("");
   
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
   
   m_zoomText = new QTextEdit(this);
   textEditSetup(m_zoomText);
   zoomFontSize(RTIMV_DEF_ZOOMFONTSIZE);
   zoomFontColor(RTIMV_DEF_ZOOMFONTCOLOR);
   zoomFontFamily(RTIMV_DEF_ZOOMFONTFAMILY);   
   connect(&m_zoomTimer, SIGNAL(timeout()), this, SLOT(zoomTimerOut()));
   zoomText("1.0x");
   
   coords = new QTextEdit(this);
   textEditSetup(coords);
   coords->setGeometry(150, 150, 100, 50);
   
   QFont qf;
   qf = coords->currentFont();
   qf.setPointSize(14);
   coords->setCurrentFont(qf);
   coords->setVisible(false);
   coords->setTextColor("lime");
   
   
   
   return;
}

void graphicsview::textEditSetup( QTextEdit * te )
{
   te->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
   te->viewport()->setAutoFillBackground(false);
   te->setVisible(true);
   te->setEnabled(false);
   te->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   te->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   te->setAttribute( Qt::WA_TransparentForMouseEvents );

}


void graphicsview::warningFontFamily(const char * ff)
{
   m_warningFontFamily = ff;
   
   QFont qf = m_warningText->currentFont();
   qf.setFamily(m_warningFontFamily);
   
   m_warningText->setCurrentFont(qf);
}

void graphicsview::warningFontSize( float fs )
{
   m_warningFontSize = fs;
   
   QFont qf = m_warningText->currentFont();
   qf.setPointSizeF(m_warningFontSize);
   
   m_warningText->setCurrentFont(qf);
}

void graphicsview::warningFontColor(const char * fc)
{
   m_warningFontColor = fc;
   m_warningText->setTextColor(QColor(m_warningFontColor));
}

QString graphicsview::warningFontFamily()
{
   return m_warningFontFamily;
}

float graphicsview::warningFontSize()
{
   return m_warningFontSize;
}

QString graphicsview::warningFontColor()
{
   return m_warningFontColor;
}
      
void graphicsview::warningText( const char * nt,
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
void graphicsview::loopFontFamily( const char * ff )
{
   m_loopFontFamily = ff;
   
   QFont qf = m_loopText->currentFont();
   qf.setFamily(m_loopFontFamily);
   
   m_loopText->setCurrentFont(qf);
}

void graphicsview::loopFontSize( float fs )
{
   m_loopFontSize = fs;
   
   QFont qf = m_loopText->currentFont();
   qf.setPointSizeF(m_loopFontSize);
   
   m_loopText->setCurrentFont(qf);
}

void graphicsview::loopFontColor( const char * fc )
{
   m_loopFontColor = fc;
   m_loopText->setTextColor(QColor(m_loopFontColor));
}

QString graphicsview::loopFontFamily()
{
   return m_loopFontFamily;
}

float graphicsview::loopFontSize()
{
   return m_loopFontSize;
}

QString graphicsview::loopFontColor()
{
   return m_loopFontColor;
}
      
void graphicsview::loopText( const char * nt,
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

void graphicsview::saveBoxFontFamily(const char * ff)
{
   m_saveBoxFontFamily = ff;
   
   QFont qf = m_saveBox->currentFont();
   qf.setFamily(m_saveBoxFontFamily);
   
   m_saveBox->setCurrentFont(qf);
}

void graphicsview::saveBoxFontSize( float fs )
{
   m_saveBoxFontSize = fs;
   
   QFont qf = m_saveBox->currentFont();
   qf.setPointSizeF(m_saveBoxFontSize);
   
   m_saveBox->setCurrentFont(qf);
}

void graphicsview::saveBoxFontColor(const char * fc)
{
   m_saveBoxFontColor = fc;
   m_saveBox->setTextColor(QColor(m_saveBoxFontColor));
}

QString graphicsview::saveBoxFontFamily()
{
   return m_saveBoxFontFamily;
}

float graphicsview::saveBoxFontSize()
{
   return m_saveBoxFontSize;
}

QString graphicsview::saveBoxFontColor()
{
   return m_saveBoxFontColor;
}
      
void graphicsview::saveBoxText( const char * nt,
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



//-------------
void graphicsview::gageFontFamily(const char * ff)
{
   m_gageFontFamily = ff;
   
   QFont qf = m_fpsGage->currentFont();
   qf.setFamily(m_gageFontFamily);
   
   m_fpsGage->setCurrentFont(qf);
   m_textCoordX->setCurrentFont(qf);
   m_textCoordY->setCurrentFont(qf);
   m_textPixelVal->setCurrentFont(qf);
   
}

void graphicsview::gageFontSize( float fs )
{
   m_gageFontSize = fs;
   
   QFont qf = m_fpsGage->currentFont();
   qf.setPointSizeF(fs);
   
   m_fpsGage->setCurrentFont(qf);
   m_textCoordX->setCurrentFont(qf);
   m_textCoordY->setCurrentFont(qf);
   m_textPixelVal->setCurrentFont(qf);
}

void graphicsview::gageFontColor(const char * fc)
{
   m_gageFontColor = fc;
   m_fpsGage->setTextColor(QColor(m_gageFontColor));
   m_textCoordX->setTextColor(QColor(m_gageFontColor));
   m_textCoordY->setTextColor(QColor(m_gageFontColor));
   m_textPixelVal->setTextColor(QColor(m_gageFontColor));
   
}

QString graphicsview::gageFontFamily()
{
   return m_gageFontFamily;
}

float graphicsview::gageFontSize()
{
   return m_gageFontSize;
}

QString graphicsview::gageFontColor()
{
   return m_gageFontColor;
}
      
void graphicsview::fpsGageText( const char * nt )
{
   m_fpsGage->setPlainText(nt);
   m_fpsGage->setAlignment(Qt::AlignRight);  
}

void graphicsview::fpsGageText( float nv,
                                bool age
                              )
{
   char strtmp[16];
   if(age)
   {
      snprintf(strtmp, 16, "Age: %.3f", nv);
   }
   else
   {
      snprintf(strtmp, 16, "FPS: %.3f", nv);
   }
   fpsGageText(strtmp);
}

void graphicsview::textCoordX( const char * nt )
{
   m_textCoordX->setPlainText(nt);
   m_textCoordX->setAlignment(Qt::AlignLeft);  
}      

void graphicsview::textCoordX( float nv )
{
   char strtmp[16];
   snprintf(strtmp, 16, "X: %.1f", nv);
   textCoordX(strtmp);   
}

void graphicsview::textCoordY( const char * nt )
{
   m_textCoordY->setPlainText(nt);
   m_textCoordY->setAlignment(Qt::AlignLeft);  
}            

void graphicsview::textCoordY( float nv )
{
   char strtmp[16];
   snprintf(strtmp, 16, "Y: %.1f", nv);
   textCoordY(strtmp);   
}

void graphicsview::textPixelVal( const char * nt )
{
   m_textPixelVal->setPlainText(nt);
   m_textPixelVal->setAlignment(Qt::AlignLeft);  
}      

void graphicsview::textPixelVal( float nv )
{
   char strtmp[24];
   snprintf(strtmp, 24, "Val: %.1f", nv);
   textPixelVal(strtmp);   
}

//----------------

void graphicsview::zoomFontFamily( const char * ff )
{
   m_zoomFontFamily = ff;
   
   QFont qf = m_zoomText->currentFont();
   qf.setFamily(m_zoomFontFamily);
   
   m_zoomText->setCurrentFont(qf);
}

void graphicsview::zoomFontSize( float fs )
{
   m_zoomFontSize = fs;
   
   QFont qf = m_zoomText->currentFont();
   qf.setPointSizeF(m_zoomFontSize);
   
   m_zoomText->setCurrentFont(qf);
}

void graphicsview::zoomFontColor( const char * fc )
{
   m_zoomFontColor = fc;
   m_zoomText->setTextColor(QColor(m_zoomFontColor));
}

QString graphicsview::zoomFontFamily()
{
   return m_zoomFontFamily;
}

float graphicsview::zoomFontSize()
{
   return m_zoomFontSize;
}

QString graphicsview::zoomFontColor()
{
   return m_zoomFontColor;
}
    
void graphicsview::zoomText( const char * nt,
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

void graphicsview::zoomTimerOut()
{
   m_zoomText->setText("");
   m_zoomTimer.stop();
}
      
void graphicsview::centerOn( qreal x, 
                             qreal y 
                           )
{
   m_xCen = x;
   m_yCen = y;
   
   QGraphicsView::centerOn(x,y);
}
      
void graphicsview::mapCenterToScene( float xc,
                                     float yc
                                   )
{
   QPointF p = mapToScene(xc, yc);
   m_xCen = p.x();
   m_yCen = p.y();
}

float graphicsview::xCen()
{
   return m_xCen;
}


float graphicsview::yCen()
{
   return m_yCen;
}

float graphicsview::mouseViewX()
{
   return m_mouseViewX;
}
      
float graphicsview::mouseViewY()
{
   return m_mouseViewY;
}

void graphicsview::zoomLevel( float zl )
{
   m_zoomLevel = zl;
}

float graphicsview::zoomLevel()
{
   return m_zoomLevel;
}
      
void graphicsview::screenZoom( float sz )
{
   m_screenZoom = sz;
}

float graphicsview::screenZoom()
{
   return m_screenZoom;
}
      
void graphicsview::resizeEvent(QResizeEvent *)
{
   m_saveBox->setGeometry(1, 1, SAVEWIDTH, SAVEHEIGHT);
   
   m_loopText->setGeometry(SAVEWIDTH, 1, width()-SAVEWIDTH, LOOPHEIGHT);
      
   m_warningText->setGeometry(0, height()-GAGEHEIGHT-WARNHEIGHT, WARNWIDTH, WARNHEIGHT);

   m_fpsGage->setGeometry(width()-FPSWIDTH, height()-GAGEHEIGHT, FPSWIDTH, GAGEHEIGHT);
   m_textCoordX->setGeometry(0, height()-GAGEHEIGHT, COORDWIDTH, GAGEHEIGHT);
   m_textCoordY->setGeometry(0+COORDWIDTH, height()-GAGEHEIGHT, COORDWIDTH, GAGEHEIGHT);
   m_textPixelVal->setGeometry(0+COORDWIDTH+COORDWIDTH, height()-GAGEHEIGHT, COORDWIDTH+50, GAGEHEIGHT);   
   
   m_zoomText->setGeometry(width()-ZOOMWIDTH, height()-GAGEHEIGHT-ZOOMHEIGHT, ZOOMWIDTH, ZOOMHEIGHT);
}

void graphicsview::mouseMoveEvent(QMouseEvent *e)
{
   m_mouseViewX = e->pos().x();
   m_mouseViewY = e->pos().y();
   
   emit mouseCoordsChanged();
      
   QGraphicsView::mouseMoveEvent(e);
}

void graphicsview::leaveEvent(QEvent *)
{
   m_mouseViewX = -1;
   m_mouseViewY = -1;
   emit mouseCoordsChanged();
}

void graphicsview::mousePressEvent(QMouseEvent *e)
{
   if(e->button() == Qt::LeftButton)
   {
      emit leftPressed(e->pos());//mp);
      QGraphicsView::mousePressEvent(e);
   }
   
   if(e->button() == Qt::RightButton)
   {
      emit rightPressed(e->pos());//mp);
   }
   else QGraphicsView::mousePressEvent(e);
}

void graphicsview::mouseReleaseEvent(QMouseEvent *e)
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

void graphicsview::mouseDoubleClickEvent(QMouseEvent *e)
{
   (void)(e);
}

void graphicsview::wheelEvent(QWheelEvent *e)
{
   emit wheelMoved(e->delta());
}


