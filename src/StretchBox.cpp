
#include "StretchBox.hpp"
#include <iostream>

StretchBox::StretchBox(QGraphicsItem * parent) : QGraphicsRectItem(parent)
{
   initStretchBox();
   return;
}
      
StretchBox::StretchBox(const QRectF & rect, QGraphicsItem * parent) : QGraphicsRectItem(rect, parent)
{
   initStretchBox();
   return;
}
      
StretchBox::StretchBox(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent) : QGraphicsRectItem(x,y,width,height,parent)
{
   xoff = x;
   yoff = y;
   
   initStretchBox();
   return;
}

void StretchBox::initStretchBox()
{
   setFlag(QGraphicsItem::ItemIsSelectable, false);
   setAcceptHoverEvents(true);

   edgeTol = 5;
   
   sizing = szOff;
   isSizing = false;   
   isMoving = false;
   grabbing = false;
   
   setStretchable(true);
   
   //setAltSelected(false);
   
   setCursorStatus (0);
   cursorTimeout = 750;
   
   cursorTimer.setSingleShot(true);
   connect(&cursorTimer, SIGNAL(timeout()), this, SLOT(cursorTimerOut()));
  
}

void StretchBox::hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{
   double rx, ry;
   
   rx = e->lastPos().x() - rect().x();
   ry = e->lastPos().y() - rect().y();

   //QPointF np = mapFromScene(QPointF(0, 0));
   //QPointF np2 = mapFromScene(QPointF(edgeTol, edgeTol));
   
   if(rx > edgeTol && rx < rect().width()-edgeTol && ry > edgeTol && ry < rect().height() -edgeTol)
   { 
      //Mouse hover outside the edge band
      setCursorStatus(0);
      return;
   }

   
   //Upper Left Corner
   if(rx > -edgeTol && rx < edgeTol && ry > -edgeTol && ry < edgeTol)
   {
      sizing = szTopl;
   }
   else if(rx > -edgeTol && rx < edgeTol && ry > rect().height()-edgeTol && ry < rect().height()+edgeTol)
   {
      sizing = szBotl; //Bottom Left
   }
   else if(rx > rect().width()-edgeTol && rx < rect().width()+edgeTol && ry > -edgeTol && ry < edgeTol)
   {
      sizing = szTopr;
   }
   else if(rx > rect().width()-edgeTol && rx < rect().width()+edgeTol && ry > rect().height()-edgeTol && ry < rect().height()+edgeTol)
   {
      sizing = szBotr;
   }
   else if(rx > -edgeTol && rx < edgeTol && ry > -edgeTol && ry < rect().height()+edgeTol)
   {
      sizing = szLeft;
   }
   else  if(rx > rect().width()-edgeTol && rx < rect().width()+edgeTol && ry > -edgeTol && ry < rect().height()+edgeTol)
   {
      sizing = szRight;
   }
   else if(rx > -edgeTol && rx < rect().width()+edgeTol && ry > -edgeTol && ry < edgeTol)
   {
      sizing = szTop;
   }
   else if(rx > -edgeTol && rx < rect().width()+edgeTol && ry > rect().height()-edgeTol && ry < rect().height()+edgeTol)
   {
      sizing = szBot;
   }

   //next:
   
   if(cursorStatus == 0)
   {
      setCursorStatus(1);
      return;
   }
   
   if(cursorStatus == 2 ) 
   {
      setCursorStatus(2);
      return;
   }
   
  

}

void StretchBox::hoverLeaveEvent(QGraphicsSceneHoverEvent * e)
{
   setCursorStatus(0);
   (void)(e);
}
   
void StretchBox::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
   if(grabbing || !stretchable)
   {
      double rx, ry;
      rx = e->pos().x() - rect().x();
      ry = e->pos().y() - rect().y();
      
      if(rx > edgeTol && rx < rect().width()-edgeTol && ry > edgeTol && ry < rect().height() -edgeTol)
      { 
         //Mouse pressed outside the edge band
         setCursorStatus(0);
         return;
      }
      else
      {
         //Mouse pressed inside the edge band while in Open Hand
         setCursorStatus(3);
         x0 = x();
         y0 = y();
         mx0 = e->scenePos().x();
         my0 = e->scenePos().y();
         isMoving=true;
         isSizing = false;
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
      mx0 = e->scenePos().x();
      my0 = e->scenePos().y();
   }
   else
   {
      emit rejectMouse();
   }
   
   (void)(e);
}

void StretchBox::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
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
         sizing=szOff;
         setCursorStatus(1);
         return;
      }
   }
   
   if(isMoving)
   {
      isMoving = false;
      //setAltSelected(true);
      emit(moved(rect()));
      setCursorStatus(1);
      return;
   }
     
   setCursorStatus(0);
   
   (void)(event);
}

void StretchBox::mouseMoveEvent ( QGraphicsSceneMouseEvent * e )
{
   double newx, newy, neww, newh;
   
   if(sizing && !isMoving)
   {
      isSizing = true;
      if(sizing == szLeft)
      {
         newx = x0 + (e->scenePos().x() - mx0);
         newy = y0;
         neww = w0 - (e->scenePos().x() - mx0);
         newh = h0; 
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szTopl)
      {
         newx = x0 + (e->scenePos().x() - mx0);
         newy = y0 + (e->scenePos().y() - my0);
         neww = w0 - (e->scenePos().x() - mx0);
         newh = h0 - (e->scenePos().y() - my0); 
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szTop)
      {
         newx = x0;
         newy = y0 + (e->scenePos().y() - my0);
         neww = w0;
         newh = h0 - (e->scenePos().y() - my0); 
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szTopr)
      {
         newx = x0;
         newy = y0 + (e->scenePos().y() - my0);
         neww = w0 + (e->scenePos().x() - mx0);
         newh = h0 - (e->scenePos().y() - my0); 
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szRight)
      {
         newx = x0;
         newy = y0;
         neww = w0 + (e->scenePos().x() - mx0);
         newh = h0; 
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szBotr)
      {
         newx = x0;
         newy = y0;
         neww = w0 + (e->scenePos().x() - mx0);
         newh = h0 + (e->scenePos().y() - my0);
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szBot)
      {
         newx = x0;
         newy = y0;
         neww = w0;
         newh = h0 + (e->scenePos().y() - my0);
         setRect(newx, newy, neww, newh);
         return;
      }
      if(sizing == szBotl)
      {
         newx = x0 + (e->scenePos().x() - mx0);
         newy = y0;
         neww = w0 - (e->scenePos().x() - mx0);
         newh = h0 + (e->scenePos().y() - my0);
         setRect(newx, newy, neww, newh);
         return;
      }
   }
   else if(isMoving)
   {
      isSizing = false;
      newx = x0 + (e->scenePos().x() - mx0);
      newy = y0 + (e->scenePos().y() - my0);
      setPos(newx, newy);
   }
}

void StretchBox::setCursorStatus(int cs)
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
      if(sizing == szTopl)  setCursor(QCursor(Qt::SizeFDiagCursor));
      if(sizing == szBotl)  setCursor(QCursor(Qt::SizeBDiagCursor));
      if(sizing == szTopr)  setCursor(QCursor(Qt::SizeBDiagCursor));
      if(sizing == szBotr)  setCursor(QCursor(Qt::SizeFDiagCursor));
      if(sizing == szLeft)  setCursor(QCursor(Qt::SizeHorCursor));
      if(sizing == szRight) setCursor(QCursor(Qt::SizeHorCursor));
      if(sizing == szTop)   setCursor(QCursor(Qt::SizeVerCursor));
      if(sizing == szBot)   setCursor(QCursor(Qt::SizeVerCursor));
      cursorStatus = 2;
      return;
   }
 
   if(cs == 3)
   {
      cursorTimer.stop();
      setCursor(QCursor(Qt::ClosedHandCursor));
      isMoving = true;
      grabbing = false;
      cursorStatus = 3;
   }
}       

void StretchBox::cursorTimerOut()
{
   setCursorStatus(2);
}

void StretchBox::setStretchable(bool is)
{
   stretchable = is;
}

void StretchBox::setCursorTimeout(int cto)
{
   cursorTimeout = cto;
}

int StretchBox::getCursorTimeout()
{
   return cursorTimeout;
}
