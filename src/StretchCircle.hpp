
#ifndef __StretchCircle_h__
#define __StretchCircle_h__


#include <QGraphicsEllipseItem>


#include "StretchGraphicsItem.hpp"

class StretchCircle : public QObject, public QGraphicsEllipseItem, public StretchGraphicsItem<StretchCircle>
{
   Q_OBJECT

   friend class StretchGraphicsItem<StretchCircle>;
   
public:

   StretchCircle(QGraphicsItem * parent = 0);
   StretchCircle(const QRectF & rect, QGraphicsItem * parent = 0 );
   StretchCircle(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0 );
   
private:
   /// Initialize the stretch circle
   /** This connects m_cursorTimer (from StretchGraphicsItem) to the required cursorTimerOut slot.
     *
     */ 
   void initStretchCircle(); 
      
protected:
      
   /** \name Geometry
     * @{
     */
   
   float m_cen_x;  ///< The x-coordinate of the center of the bounding box
   float m_cen_y;  ///< The x-coordinate of the center of the bounding box
   float m_drad0; ///< The radius of the mouse position at the start of a re-size
   
public:
   
   /// Get the current radius of the circle 
   /**
     * \returns the radius
     */ 
   float radius();
   
   ///@}
   
      

protected:
   
   /** \name Event Handlers
     * Each of these calls the associated handleXXXX from StretchGraphicsItem
     * 
     * @{
     */
   void hoverMoveEvent(QGraphicsSceneHoverEvent * e);
   
   void hoverLeaveEvent(QGraphicsSceneHoverEvent * e);
      
   void mousePressEvent ( QGraphicsSceneMouseEvent * event );
   
   void mouseReleaseEvent(QGraphicsSceneMouseEvent * event );
      
   void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

   void keyPressEvent(QKeyEvent * ke);

   ///@}

   /** \name StretchGraphicsItem Interface
     *
     * @{
     */ 

   bool onHoverComputeSizing(QGraphicsSceneHoverEvent * e);

   bool onMousePressCalcGrabbed(QGraphicsSceneMouseEvent * e);

   void setGrabbedGeometry(QGraphicsSceneMouseEvent * e);

   void setMovingGeometry(QGraphicsSceneMouseEvent * e);

   QRectF sizingCalcNewRect(QGraphicsSceneMouseEvent * e);

   QPointF movingCalcNewPos(QGraphicsSceneMouseEvent * e);

   void passKeyPressEvent(QKeyEvent * ke);

protected slots:

   /// When the cursor timer times-out, the cursor is changed to the double-arrow
   virtual void cursorTimerOut();
   
   ///@}
   
   /** \name Signals
     * The emitXXXX are required by StretchGraphicsItem, and each
     * emits the associated signal.
     * @{
     */
   void emitMoved();
   void emitResized();
   void emitRejectMouse();
   void emitMouseIn();
   void emitMouseOut();
   void emitRemove();
   
signals:
   void moved(StretchCircle * s);
   void resized(StretchCircle * s);
   void rejectMouse(StretchCircle * s);
   void mouseIn(StretchCircle * s);
   void mouseOut(StretchCircle * s);
   void remove(StretchCircle * s);   
   
   ///@}
   

};

#endif //__StretchCircle_h__

