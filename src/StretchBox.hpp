
#ifndef __StretchBox_h__
#define __StretchBox_h__


#include <QGraphicsRectItem>


#include "StretchGraphicsItem.hpp"

class StretchBox : public QObject, public QGraphicsRectItem, public StretchGraphicsItem<StretchBox>
{
   Q_OBJECT

   friend class StretchGraphicsItem<StretchBox>;
   
public:

   typedef QRectF coordSpecT;
   
   StretchBox(QGraphicsItem * parent = 0);
   StretchBox(const QRectF & rect, QGraphicsItem * parent = 0 );
   StretchBox(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0 );
   
private:
   /// Initialize the stretch circle
   /** This connects m_cursorTimer (from StretchGraphicsItem) to the required cursorTimerOut slot.
     *
     */ 
   void initStretchBox(); 
      
   /** \name Geometry 
     * @{
     */ 
protected:
   bool m_maintainCenter {false}; ///< Sets whether the center coordinate is maintained (true) when stretching, or if it moves (false)
   
   float m_ul_x {0}; ///< The x-coordinate of the upper left corner of the bounding box
   float m_ul_y {0}; ///< The y-coordinate of the upper left corner of the bounding box
   float m_width {0};  ///< The width of the bounding box
   float m_height {0}; ///< The height of the bounding box
   float m_mv_x0 {0}; ///< The x-coordinate of the mouse at the start of a re-size
   float m_mv_y0 {0}; ///< The y-coordinate of the mouse at the start of a re-size

   
public:
   
   /// Get whether or not the center is maintained when stretching.
   /** \returns the current value of m_maintainCenter
     */
   bool maintainCenter();
   
   /// Set whether to maintain the center when stretching.
   void setMaintainCenter(bool mc /**< [in] Set m_maintainCenter to true to maintain the center, false to move it.*/);
   
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

   void sizingCalcNewPos(QGraphicsSceneMouseEvent * e);

   void movingCalcNewPos(QGraphicsSceneMouseEvent * e);

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
   void emitSelected();
   void emitDeSelected();
   
signals:
   void moved(StretchBox * s);
   void resized(StretchBox * s);
   void rejectMouse(StretchBox * s);
   void mouseIn(StretchBox * s);
   void mouseOut(StretchBox * s);
   void remove(StretchBox * s);   
   void selected(StretchBox * s);
   void deSelected(StretchBox * s);

   ///@}
   

};

#endif //__StretchBox_h__

