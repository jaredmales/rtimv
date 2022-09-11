
#include "StretchBox.hpp"
#include <cmath>
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
   initStretchBox();
   return;
}

void StretchBox::initStretchBox()
{   
   connect(&m_cursorTimer, SIGNAL(timeout()), this, SLOT(cursorTimerOut()));
}

bool StretchBox::maintainCenter()
{
   return m_maintainCenter;
}
   
void StretchBox::setMaintainCenter(bool mc)
{
   m_maintainCenter = mc;
}
   
void StretchBox::hoverMoveEvent(QGraphicsSceneHoverEvent * e)
{
   handleHoverMoveEvent(e);
}

void StretchBox::hoverLeaveEvent(QGraphicsSceneHoverEvent * e)
{
   handleHoverLeaveEvent(e);
}

void StretchBox::mousePressEvent (QGraphicsSceneMouseEvent * e)
{
   handleMousePressEvent(e);
}

void StretchBox::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
   handleMouseReleaseEvent(e);
}

void StretchBox::mouseMoveEvent(QGraphicsSceneMouseEvent * e)
{
   handleMouseMoveEvent(e);
}

void StretchBox::keyPressEvent(QKeyEvent * ke)
{
   handleKeyPressEvent(ke);
}

void StretchBox::passKeyPressEvent(QKeyEvent * ke)
{
   QGraphicsRectItem::keyPressEvent(ke);
}

void StretchBox::cursorTimerOut()
{
   slotCursorTimerOut();
}

bool StretchBox::onHoverComputeSizing(QGraphicsSceneHoverEvent * e)
{
   double rx, ry;
   
   rx = e->lastPos().x() - rect().x();
   ry = e->lastPos().y() - rect().y();

   if(rx > m_edgeTol && rx < rect().width()-m_edgeTol && ry > m_edgeTol && ry < rect().height() -m_edgeTol)
   { 
      return false;
   }

   //Upper Left Corner
   if(rx > -m_edgeTol && rx < m_edgeTol && ry > -m_edgeTol && ry < m_edgeTol)
   {
      m_sizing = szTopl;
   }
   else if(rx > -m_edgeTol && rx < m_edgeTol && ry > rect().height()-m_edgeTol && ry < rect().height()+m_edgeTol)
   {
      m_sizing = szBotl; //Bottom Left
   }
   else if(rx > rect().width()-m_edgeTol && rx < rect().width()+m_edgeTol && ry > -m_edgeTol && ry < m_edgeTol)
   {
      m_sizing = szTopr;
   }
   else if(rx > rect().width()-m_edgeTol && rx < rect().width()+m_edgeTol && ry > rect().height()-m_edgeTol && ry < rect().height()+m_edgeTol)
   {
      m_sizing = szBotr;
   }
   else if(rx > -m_edgeTol && rx < m_edgeTol && ry > -m_edgeTol && ry < rect().height()+m_edgeTol)
   {
      m_sizing = szLeft;
   }
   else  if(rx > rect().width()-m_edgeTol && rx < rect().width()+m_edgeTol && ry > -m_edgeTol && ry < rect().height()+m_edgeTol)
   {
      m_sizing = szRight;
   }
   else if(rx > -m_edgeTol && rx < rect().width()+m_edgeTol && ry > -m_edgeTol && ry < m_edgeTol)
   {
      m_sizing = szTop;
   }
   else if(rx > -m_edgeTol && rx < rect().width()+m_edgeTol && ry > rect().height()-m_edgeTol && ry < rect().height()+m_edgeTol)
   {
      m_sizing = szBot;
   }
   
   return true;
}

bool StretchBox::onMousePressCalcGrabbed(QGraphicsSceneMouseEvent * e)
{
   double rx, ry;
   rx = e->pos().x() - rect().x();
   ry = e->pos().y() - rect().y();
   
   if(rx > m_edgeTol && rx < rect().width()-m_edgeTol && ry > m_edgeTol && ry < rect().height() - m_edgeTol)
   { 
      return false;
   }
   else
   {
      return true;
   }
}
 
void StretchBox::setGrabbedGeometry(QGraphicsSceneMouseEvent * e)
{
   m_ul_x = x();
   m_ul_y = y();
   m_mv_x0 = e->scenePos().x();
   m_mv_y0 = e->scenePos().y();
   
}

void StretchBox::setMovingGeometry(QGraphicsSceneMouseEvent * e)
{
   m_ul_x = rect().x();
   m_ul_y = rect().y();
   m_width = rect().width();
   m_height = rect().height();
   m_mv_x0 = e->scenePos().x();
   m_mv_y0 = e->scenePos().y();
   
   return;
}

void StretchBox::sizingCalcNewPos(QGraphicsSceneMouseEvent * e)
{
   double newx, newy, neww, newh, dw, dh;
   
   dw = (e->scenePos().x() - m_mv_x0);
   dh = (e->scenePos().y() - m_mv_y0);

   if(m_sizing == szLeft)
   {
      newx = m_ul_x + dw;
      if(m_maintainCenter)
      {
         dw *= 2;
      }

      newy = m_ul_y;
      neww = m_width - dw; 
      newh = m_height; 
   }
   else if(m_sizing == szTopl)
   {
      newx = m_ul_x + dw;
      newy = m_ul_y + dh;

      if(m_maintainCenter)
      {
         dw *= 2;
         dh *= 2;
      }
      
      neww = m_width - dw;
      newh = m_height - dh;    
   }
   else if(m_sizing == szTop)
   {
      newx = m_ul_x;
      newy = m_ul_y + dh;
      if(m_maintainCenter)
      {
         dh *= 2;
      }

      neww = m_width;
      newh = m_height - dh; 
   }
   else if(m_sizing == szTopr)
   {
      newx = m_ul_x;
      newy = m_ul_y + dh;
      if(m_maintainCenter)
      {
         newx -= dw;

         dw *= 2;
         dh *= 2;
      }

      neww = m_width + dw;
      newh = m_height - dh; 
   }
   else if(m_sizing == szRight)
   {
      newx = m_ul_x;
      newy = m_ul_y;

      if(m_maintainCenter)
      {
         newx -= dw;
         dw *= 2;
      }

      neww = m_width + dw;
      newh = m_height; 
   }
   else if(m_sizing == szBotr)
   {
      newx = m_ul_x;
      newy = m_ul_y;
      if(m_maintainCenter)
      {
         newx -= (e->scenePos().x() - m_mv_x0);
         newy -= (e->scenePos().y() - m_mv_y0);
         dw *= 2;
         dh *= 2;
      }
      neww = m_width + dw;
      newh = m_height + dh;
   }
   else if(m_sizing == szBot)
   {
      newx = m_ul_x;
      newy = m_ul_y;
      if(m_maintainCenter)
      {
         newy -= dh;
         dh *= 2;
      }
      neww = m_width;
      newh = m_height + dh;
   }
   else if(m_sizing == szBotl)
   {
      newx = m_ul_x + dw;
      newy = m_ul_y;
      if(m_maintainCenter)
      {
         newy -= dh;
         
         dw *= 2;
         dh *= 2;
      }
      
      neww = m_width - dw;
      newh = m_height + dh;
   }
   else 
   {
      std::cerr << "StretchBox::sizingCalcNewPos (" << __FILE__ << " " << __LINE__ << "): m_sizing has invalid value.\n";
      return;
   }
      
   setRect(newx, newy, neww, newh);
}

void StretchBox::movingCalcNewPos( QGraphicsSceneMouseEvent * e )
{
   float newx = m_ul_x + (e->scenePos().x() - m_mv_x0);
   float newy = m_ul_y + (e->scenePos().y() - m_mv_y0);
 
   setPos(newx, newy);
}

void StretchBox::emitMoved()
{
   emit moved(this);
}

void StretchBox::emitResized()
{
   emit resized(this);
}

void StretchBox::emitRejectMouse()
{
   emit rejectMouse(this);
}

void StretchBox::emitMouseIn()
{
   emit mouseIn(this);
}

void StretchBox::emitMouseOut()
{
   emit mouseOut(this);
}

void StretchBox::emitRemove()
{
   emit remove(this);
}

void StretchBox::emitSelected()
{
   emit selected(this);
}

void StretchBox::emitDeSelected()
{
   emit deSelected(this);
}
