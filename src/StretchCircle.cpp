
#include "StretchCircle.hpp"
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
   connect(&m_cursorTimer, SIGNAL(timeout()), this, SLOT(cursorTimerOut()));
   connect(&m_selectionTimer, SIGNAL(timeout()), this, SLOT(selectionTimerOut()));
}

float StretchCircle::radius()
{
   return 0.5*rect().width();
}

void StretchCircle::hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{
   handleHoverMoveEvent(e);
}

void StretchCircle::hoverLeaveEvent(QGraphicsSceneHoverEvent * e)
{
   handleHoverLeaveEvent(e);
}

void StretchCircle::mousePressEvent (QGraphicsSceneMouseEvent * e)
{
   handleMousePressEvent(e);
}

void StretchCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
   handleMouseReleaseEvent(e);
}

void StretchCircle::mouseMoveEvent(QGraphicsSceneMouseEvent * e)
{
   handleMouseMoveEvent(e);
}

void StretchCircle::keyPressEvent(QKeyEvent * ke)
{
   handleKeyPressEvent(ke);
}

void StretchCircle::focusOutEvent(QFocusEvent * e)
{
    handleFocusOutEvent(e);
}

void StretchCircle::passKeyPressEvent(QKeyEvent * ke)
{
   QGraphicsEllipseItem::keyPressEvent(ke);
}

void StretchCircle::cursorTimerOut()
{
   slotCursorTimerOut();
}

void StretchCircle::selectionTimerOut()
{
   slotSelectionTimerOut();
}

bool StretchCircle::onHoverComputeSizing(QGraphicsSceneHoverEvent * e)
{

   double xcen, ycen, rad;
   
   xcen = rect().x() + .5*rect().width();
   ycen = rect().y() + .5*rect().height();
   
   rad = sqrt(pow(e->pos().x() - xcen,2) +  pow(e->pos().y() - ycen,2));
   
   double drad = fabs(rad - 0.5*rect().width());

   if(drad > m_edgeTol)
   {
      return false;
   }
   
   m_cursorAngle = atan2(e->pos().y() - ycen, e->pos().x() - xcen) * 180./3.14159;
   if(m_cursorAngle < 0) m_cursorAngle += 360.;
   
   if( m_cursorAngle <= 20 || m_cursorAngle > 340 ||  (m_cursorAngle > 160 && m_cursorAngle <= 200)) m_sizing = szLeft;
   if( (m_cursorAngle > 20 && m_cursorAngle <= 70) || (m_cursorAngle > 200 && m_cursorAngle <= 250)) m_sizing = szBotr;
   if( (m_cursorAngle >70 && m_cursorAngle <= 110) || (m_cursorAngle > 250 && m_cursorAngle <= 290))  m_sizing = szTop;
   if( (m_cursorAngle > 110 && m_cursorAngle <= 160) || (m_cursorAngle > 290 && m_cursorAngle <= 340)) m_sizing = szTopr; 

   return true;
}

bool StretchCircle::onMousePressCalcGrabbed(QGraphicsSceneMouseEvent * e)
{
   double xcen, ycen, rad;

   xcen = rect().x() + .5*rect().width();
   ycen = rect().y() + .5*rect().height();

   rad = sqrt(pow(e->pos().x() - xcen,2) +  pow(e->pos().y() - ycen,2));
   m_cursorAngle = atan2(e->pos().y() - ycen, e->pos().x() - xcen) * 180./3.14159;
   if(m_cursorAngle < 0) m_cursorAngle += 360.;  
   
   double drad = fabs(rad - 0.5*rect().width());

   if(drad > m_edgeTol) 
   {
      return false;
   }
   else
   {
      return true;
   }
}
 
void StretchCircle::setGrabbedGeometry(QGraphicsSceneMouseEvent * e)
{
   m_ul_x = x();
   m_ul_y = y();
   m_mv_x0 = e->scenePos().x();
   m_mv_y0 = e->scenePos().y();
}

void StretchCircle::setMovingGeometry(QGraphicsSceneMouseEvent * e)
{
   m_ul_x = rect().x();
   m_ul_y = rect().y();
   m_width = rect().width();
   m_height = rect().height();
   m_mv_x0 = e->scenePos().x();
   m_mv_y0 = e->scenePos().y();
      
   m_cen_x = rect().x() + 0.5*rect().width();
   m_cen_y = rect().y() + 0.5*rect().height();
   
   m_drad0 = (sqrt(pow(e->pos().x() - m_cen_x, 2) + pow(e->pos().y() - m_cen_y,2)));   
}

void StretchCircle::sizingCalcNewPos(QGraphicsSceneMouseEvent * e)
{
   float drad, newx, newy, neww, newh;
    
   float rad = sqrt(pow(e->pos().x() - m_cen_x, 2) + pow(e->pos().y() - m_cen_y,2));
   drad = (rad - m_drad0)*4.;
   
   newx = m_ul_x - .25*drad;
   newy = m_ul_y - .25*drad;
   neww = m_width + 0.5*drad;
   newh = neww;
   
   setRect(newx, newy, neww, newh);
}

void StretchCircle::movingCalcNewPos( QGraphicsSceneMouseEvent * e )
{
   float newx = m_ul_x + (e->scenePos().x() - m_mv_x0);
   float newy = m_ul_y + (e->scenePos().y() - m_mv_y0);
 
   setPos(newx, newy);
}

void StretchCircle::emitMoved()
{
   emit moved(this);
}

void StretchCircle::emitResized()
{
   emit resized(this);
}

void StretchCircle::emitRejectMouse()
{
   emit rejectMouse(this);
}

void StretchCircle::emitMouseIn()
{
   emit mouseIn(this);
}

void StretchCircle::emitMouseOut()
{
   emit mouseOut(this);
}

void StretchCircle::emitRemove()
{
   emit remove(this);
}

void StretchCircle::emitSelected()
{
   emit selected(this);
}

void StretchCircle::emitDeSelected()
{
   emit deSelected(this);
}
