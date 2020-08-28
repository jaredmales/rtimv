
#include "StretchCircle.hpp"
#include <iostream>
#include <cmath>

StretchCircle::StretchCircle(QGraphicsItem * parent) : QGraphicsEllipseItem(parent)
{
   initStretchCircle();
   return;
}
      
StretchCircle::StretchCircle(const QRectF & rect, QGraphicsItem * parent) : QGraphicsEllipseItem(rect, parent)
{
   initStretchCircle();
   return;
}
      
StretchCircle::StretchCircle(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent) : QGraphicsEllipseItem(x,y,width,height,parent)
{
   initStretchCircle();
   return;
}

void StretchCircle::initStretchCircle()
{   
   setFlag(QGraphicsItem::ItemIsFocusable, true);
   setFlag(QGraphicsItem::ItemIsSelectable, true);
   setAcceptHoverEvents(true);
   
   setCursorStatus (cursorOff);
   
   m_cursorTimer.setSingleShot(true);
   connect(&m_cursorTimer, SIGNAL(timeout()), this, SLOT(cursorTimerOut()));
   
}

void StretchCircle::setStretchable(bool is)
{
   m_stretchable = is;
}

bool StretchCircle::stretchable()
{
   return m_stretchable;
}

float StretchCircle::radius()
{
   return 0.5*rect().width();
}

void StretchCircle::setCursorStatus(int cs) //same
{
   if(cs == cursorOff)
   {
      m_cursorTimer.stop();
      m_grabbing = false;
      m_isMoving = false;
      m_sizing = szOff;
      setCursor(QCursor(Qt::ArrowCursor));
      m_cursorStatus = cursorOff;
      return;
   }
   
   if(cs == cursorGrabbing)
   {
      if(m_grabbing) return;
      setCursor(QCursor(Qt::OpenHandCursor));
      m_grabbing = true;
      m_cursorTimer.start(m_cursorTimeout);
      m_cursorStatus = cursorGrabbing;
      return;
   }
   
   if(cs == cursorSizing)
   {
      m_grabbing = false;

      if(m_sizing == szTopl)  setCursor(QCursor(Qt::SizeFDiagCursor));
      if(m_sizing == szBotl)  setCursor(QCursor(Qt::SizeBDiagCursor));
      if(m_sizing == szTopr)  setCursor(QCursor(Qt::SizeBDiagCursor));
      if(m_sizing == szBotr)  setCursor(QCursor(Qt::SizeFDiagCursor));
      if(m_sizing == szLeft)  setCursor(QCursor(Qt::SizeHorCursor));
      if(m_sizing == szRight) setCursor(QCursor(Qt::SizeHorCursor));
      if(m_sizing == szTop)   setCursor(QCursor(Qt::SizeVerCursor));
      if(m_sizing == szBot)   setCursor(QCursor(Qt::SizeVerCursor));
      m_cursorStatus = cursorSizing;
      
      return;
   }
 
   if(cs == cursorGrabbed)
   {
      m_cursorTimer.stop();
      setCursor(QCursor(Qt::ClosedHandCursor));
      m_isMoving = true;
      m_grabbing = false;
      m_cursorStatus = cursorGrabbed;
      
      return;
   }
}       

int StretchCircle::edgeTol()
{ 
   return m_edgeTol;
}
   
void StretchCircle::setEdgeTol(int et)
{ 
   m_edgeTol = et;
}

void StretchCircle::cursorTimerOut()
{
   setCursorStatus(cursorSizing);
}

void StretchCircle::hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{   
   //bool onHoverComputeSizing(QGraphicsSceneHoverEvent * e)
   //{
   double xcen, ycen, rad;
   
   xcen = rect().x() + .5*rect().width();
   ycen = rect().y() + .5*rect().height();
   
   rad = sqrt(pow(e->pos().x() - xcen,2) +  pow(e->pos().y() - ycen,2));
   
   double drad = fabs(rad - 0.5*rect().width());

   if(drad > m_edgeTol)
   {
      //return false
      setCursorStatus(cursorOff);
      clearFocus();
      setSelected(false);
      emit mouseOut(this);
      return; 
   }
   
   m_cursorAngle = atan2(e->pos().y() - ycen, e->pos().x() - xcen) * 180./3.14159;
   if(m_cursorAngle < 0) m_cursorAngle += 360.;
   
   if( m_cursorAngle <= 20 || m_cursorAngle > 340 ||  (m_cursorAngle > 160 && m_cursorAngle <= 200)) m_sizing = szLeft;
   if( (m_cursorAngle > 20 && m_cursorAngle <= 70) || (m_cursorAngle > 200 && m_cursorAngle <= 250)) m_sizing = szBotr;
   if( (m_cursorAngle >70 && m_cursorAngle <= 110) || (m_cursorAngle > 250 && m_cursorAngle <= 290))  m_sizing = szTop;
   if( (m_cursorAngle > 110 && m_cursorAngle <= 160) || (m_cursorAngle > 290 && m_cursorAngle <= 340)) m_sizing = szTopr; 

   //return true
   //}
   emit mouseIn(this);
   
   if(m_cursorStatus == cursorOff)
   {
      setCursorStatus(cursorGrabbing);
   }
   
   if(m_cursorStatus == cursorSizing)
   {
      setCursorStatus(cursorSizing);
   }
}

void StretchCircle::hoverLeaveEvent(QGraphicsSceneHoverEvent * e)
{  
   static_cast<void>(e);
   
   setCursorStatus(cursorOff);
   clearFocus();
   setSelected(false);
   emit mouseOut(this);
}

void StretchCircle::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
   setSelected(true);
   if(m_grabbing || !m_stretchable)
   {
      //onMousePressCalcGrab
      //{
      double xcen, ycen, rad;
   
      xcen = rect().x() + .5*rect().width();
      ycen = rect().y() + .5*rect().height();
   
      rad = sqrt(pow(e->pos().x() - xcen,2) +  pow(e->pos().y() - ycen,2));
      m_cursorAngle = atan2(e->pos().y() - ycen, e->pos().x() - xcen) * 180./3.14159;
      if(m_cursorAngle < 0) m_cursorAngle += 360.;  
      
      double drad = fabs(rad - 0.5*rect().width());
   
      //}
      if(drad > m_edgeTol) //if true
      {
         setCursorStatus(cursorOff);
         return;
      }
      else
      {
         setCursorStatus(cursorGrabbed);
         m_ul_x = x();
         m_ul_y = y();
         m_mv_x0 = e->scenePos().x();
         m_mv_y0 = e->scenePos().y();
         m_isMoving=true;
         m_isSizing=false;
         m_sizing=szOff;
         return;
      }
   }
   else if(m_sizing)
   {
      m_grabbing = false;
      m_ul_x = rect().x();
      m_ul_y = rect().y();
      m_width = rect().width();
      m_height = rect().height();
     
      m_cen_x = rect().x() + 0.5*rect().width();
      m_cen_y = rect().y() + 0.5*rect().height();
      
      m_mv_x0 = e->scenePos().x();
      m_mv_y0 = e->scenePos().y();
      
      m_drad0 = (sqrt(pow(e->pos().x() - m_cen_x, 2) + pow(e->pos().y() - m_cen_y,2)));
   }
   else
   {
      emit rejectMouse(this);
   }
   
}

void StretchCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
   static_cast<void>(event);
   if(m_sizing)
   {
      if(m_isSizing)
      {
         m_sizing = szOff;
         m_isSizing = false;
         emit moved(this);
         return;
      }
      else
      {
         m_sizing = szOff;
         setCursorStatus(cursorGrabbing);
         return;
      }
   }
   
   if(m_isMoving)
   {
      m_isMoving = false;
      emit moved(this);
      setCursorStatus(cursorGrabbing);
      return;
   }
   setCursorStatus(cursorOff);
}

void StretchCircle::mouseMoveEvent ( QGraphicsSceneMouseEvent * e )
{
   double drad, newx, newy, neww, newh;
    
   if(m_sizing && !m_isMoving)
   {
      //sizingCalcNewRect
      m_isSizing = true;
      
      double rad = sqrt(pow(e->pos().x() - m_cen_x, 2) + pow(e->pos().y() - m_cen_y,2));
      drad = (rad - m_drad0)*4.;
      
      newx = m_ul_x - .25*drad;
      newy = m_ul_y - .25*drad;
      neww = m_width + 0.5*drad;
      newh = neww;
      
      setRect(newx, newy, neww, newh);

      emit resized(this);
      
   }
   else if(m_isMoving)
   {
      m_isSizing = false;
      newx = m_ul_x + (e->scenePos().x() - m_mv_x0);
      newy = m_ul_y + (e->scenePos().y() - m_mv_y0);
      setPos(newx, newy);
      emit moved(this);
   }
}

void StretchCircle::keyPressEvent(QKeyEvent * ke)
{
   if(ke->key() == Qt::Key_Delete)
   {
      emit remove(this);
      return;
   }
   
   QGraphicsEllipseItem::keyPressEvent(ke);
}




QColor StretchCircle::penColor()
{
   return pen().color();
}

void StretchCircle::penColor(const QColor & newcol)
{
   QPen p = pen();
   p.setColor(newcol);
   setPen(p);
}

float StretchCircle::penWidth()
{
   return pen().widthF();
}

void StretchCircle::penWidth(float newwidth)
{
   QPen p = pen();
   p.setWidthF(newwidth);
   setPen(p);
}



