
#ifndef __StretchCircle_h__
#define __StretchCircle_h__


#include <QGraphicsEllipseItem>


#include "StretchGraphicsItem.hpp"

class StretchCircle : public QObject, public QGraphicsEllipseItem, public StretchGraphicsItem<StretchCircle>
{
   Q_OBJECT

   friend class StretchGraphicsItem<StretchCircle>;
   
public:

   typedef QRectF coordSpecT;
   
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
   float m_ul_x {0}; ///< The x-coordinate of the upper left corner of the bounding box
   float m_ul_y {0}; ///< The y-coordinate of the upper left corner of the bounding box
   float m_width {0};  ///< The width of the bounding box
   float m_height {0}; ///< The height of the bounding box
   float m_mv_x0 {0}; ///< The x-coordinate of the mouse at the start of a re-size
   float m_mv_y0 {0}; ///< The y-coordinate of the mouse at the start of a re-size

   float m_cen_x {0};  ///< The x-coordinate of the center of the bounding box
   float m_cen_y {0};  ///< The x-coordinate of the center of the bounding box
   float m_drad0 {0}; ///< The radius of the mouse position at the start of a re-size
   
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

   void focusOutEvent(QFocusEvent * e);

   ///@}

   /** \name StretchGraphicsItem Interface
     *
     * @{
     */ 

   bool onHoverComputeSizing(QGraphicsSceneHoverEvent * e);

   bool onMousePressCalcGrabbed(QGraphicsSceneMouseEvent * e);

   void setGrabbedGeometry(QGraphicsSceneMouseEvent * e);

   void setMovingGeometry(QGraphicsSceneMouseEvent * e);

   void sizingCalcNewPos(QGraphicsSceneMouseEvent * e);

   void movingCalcNewPos(QGraphicsSceneMouseEvent * e);

   void passKeyPressEvent(QKeyEvent * ke);

protected slots:

   /// When the cursor timer times-out, the cursor is changed to the double-arrow
   virtual void cursorTimerOut();
   
   /// When the selection timer times-out, the item is selected without clicking
   virtual void selectionTimerOut();

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
   void emitSelected();
   void emitDeSelected();

signals:
   void moved(StretchCircle * s);
   void resized(StretchCircle * s);
   void rejectMouse(StretchCircle * s);
   void mouseIn(StretchCircle * s);
   void mouseOut(StretchCircle * s);
   void remove(StretchCircle * s);   
   void selected(StretchCircle * s);
   void deSelected(StretchCircle * s);


   ///@}
   

};

#endif //__StretchCircle_h__

