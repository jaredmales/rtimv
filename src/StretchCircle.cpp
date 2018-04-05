
#include "StretchCircle.h"
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
   xoff = x;
   yoff = y;
   
   initStretchCircle();
   return;
}

void StretchCircle::initStretchCircle()
{
   setFlag(QGraphicsItem::ItemIsSelectable, false);
   setAcceptHoverEvents(true);

   edgeTol = 5;
   
   sizing = szOff;
   isSizing = false;   
   isMoving = false;
   grabbing = false;
   
   setStretchable(true);
      
   setCursorStatus (0);
   cursorTimeout = 750;
   
   cursorTimer.setSingleShot(true);
   connect(&cursorTimer, SIGNAL(timeout()), this, SLOT(cursorTimerOut()));
   
   
}

void StretchCircle::hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{   
   double xcen, ycen, rad;
   
   xcen = rect().x() + .5*rect().width();
   ycen = rect().y() + .5*rect().height();
   
   rad = sqrt(pow(e->pos().x() - xcen,2) +  pow(e->pos().y() - ycen,2));
   cursorAngle = atan2(e->pos().y() - ycen, e->pos().x() - xcen) * 180./3.14159;
   if(cursorAngle < 0) cursorAngle += 360.;
   
   double drad = fabs(rad - 0.5*rect().width());

   QPointF np = mapFromScene(QPointF(0, 0));
   QPointF np2 = mapFromScene(QPointF(edgeTol, edgeTol));
  
   if(drad > edgeTol)
   {
      emit mouseOut();
      setCursorStatus(0);
      return;
   }
   
   sizing = szOn;
   emit mouseIn();
   
   if(cursorStatus == 0)
   {
      setCursorStatus(1);
   }
   
   if(cursorStatus == 2)
   {
      setCursorStatus(2);
   }
   
}

void StretchCircle::hoverLeaveEvent(QGraphicsSceneHoverEvent * e)
{  
   setCursorStatus(0);
   emit mouseOut();
}

void StretchCircle::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
   if(grabbing || !stretchable)
   {
      double xcen, ycen, rad;
   
      xcen = rect().x() + .5*rect().width();
      ycen = rect().y() + .5*rect().height();
   
      rad = sqrt(pow(e->pos().x() - xcen,2) +  pow(e->pos().y() - ycen,2));
      cursorAngle = atan2(e->pos().y() - ycen, e->pos().x() - xcen) * 180./3.14159;
      if(cursorAngle < 0) cursorAngle += 360.;  
      
      double drad = fabs(rad - 0.5*rect().width());
   
      if(drad > edgeTol)
      {
         setCursorStatus(0);
         return;
      }
      else
      {
         setCursorStatus(3);
         x0 = x();
         y0 = y();
         mx0 = e->scenePos().x();
         my0 = e->scenePos().y();
         isMoving=true;
         isSizing=false;
         sizing=szOff;
         return;
      }
   }
   else if(sizing)
   {
      grabbing = false;
      x0 = rect().x();
      y0 = rect().y();
      w0 = rect().width();
      h0 = rect().height();
     
      xc0 = rect().x() + 0.5*rect().width();
      yc0 = rect().y() + 0.5*rect().height();
      
      mx0 = e->scenePos().x();
      my0 = e->scenePos().y();
      
      drad0 = (sqrt(pow(e->pos().x() - xc0, 2) + pow(e->pos().y() - yc0,2)));
   }
   else
   {
      emit rejectMouse();
   }
   
}

void StretchCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
   if(sizing)
   {
      if(isSizing)
      {
         sizing = szOff;
         isSizing = false;
         emit(moved(rect()));
         return;
      }
      else
      {
         sizing = szOff;
         setCursorStatus(1);
         return;
      }
   }
   
   if(isMoving)
   {
      isMoving = false;
      emit(moved(rect()));
      setCursorStatus(1);
      return;
   }
   setCursorStatus(0);
}

void StretchCircle::mouseMoveEvent ( QGraphicsSceneMouseEvent * e )
{
   double drad, newx, newy, neww, newh;
    
   if(sizing && !isMoving)
   {
      isSizing = true;
      
      double rad = sqrt(pow(e->pos().x() - xc0, 2) + pow(e->pos().y() - yc0,2));
      drad = (rad - drad0)*4.;
      
      newx = x0 - .25*drad;
      newy = y0 - .25*drad;
      neww = w0 + 0.5*drad;
      newh = neww;
      
      setRect(newx, newy, neww, newh);

      emit(resized(0.5*neww));
      
   }
   else if(isMoving)
   {
      isSizing = false;
      newx = x0 + (e->scenePos().x() - mx0);
      newy = y0 + (e->scenePos().y() - my0);
      setPos(newx, newy);
      emit(moved(rect()));
   }
}

void StretchCircle::setCursorStatus(int cs)
{
   if(cs == 0)
   {
      cursorTimer.stop();
      grabbing = false;
      isMoving = false;
      sizing = szOff;
      setCursor(QCursor(Qt::ArrowCursor));
      cursorStatus = 0;
      return;
   }
   
   if(cs == 1)
   {
      if(grabbing) return;
      setCursor(QCursor(Qt::OpenHandCursor));
      grabbing=true;
      cursorTimer.start(cursorTimeout);
      cursorStatus = 1;
      return;
   }
   
   if(cs == 2)
   {
      grabbing = false;
      sizing = szOn;
      if( cursorAngle <= 20 || cursorAngle > 340 ||  (cursorAngle > 160 && cursorAngle <= 200)) setCursor(QCursor(Qt::SizeHorCursor));
      if( (cursorAngle > 20 && cursorAngle <= 70) || (cursorAngle > 200 && cursorAngle <= 250)) setCursor(QCursor(Qt::SizeFDiagCursor));
      if( (cursorAngle >70 && cursorAngle <= 110) || (cursorAngle > 250 && cursorAngle <= 290))  setCursor(QCursor(Qt::SizeVerCursor));
      if( (cursorAngle > 110 && cursorAngle <= 160) || (cursorAngle > 290 && cursorAngle <= 340)) setCursor(QCursor(Qt::SizeBDiagCursor));   
      cursorStatus = 2;
      return;
   }
 
   if(cs == 3)
   {
      cursorTimer.stop();
      setCursor(QCursor(Qt::ClosedHandCursor));
      isMoving == true;
      grabbing = false;
      cursorStatus = 3;
      return;
   }
}       

void StretchCircle::cursorTimerOut()
{
   setCursorStatus(2);
}


void StretchCircle::setStretchable(bool is)
{
   stretchable = is;
}
