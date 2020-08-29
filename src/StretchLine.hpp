
#ifndef __StretchLine_h__
#define __StretchLine_h__


#include <QGraphicsEllipseItem>


#include "StretchGraphicsItem.hpp"

class StretchLine : public QObject, public QGraphicsLineItem, public StretchGraphicsItem<StretchLine>
{
   Q_OBJECT

   friend class StretchGraphicsItem<StretchLine>;
   
public:

   StretchLine(QGraphicsItem * parent = 0);
   StretchLine(qreal xs, qreal ys, qreal xe, qreal ye, QGraphicsItem * parent = 0 );
   
private:
   /// Initialize the stretch circle
   /** This connects m_cursorTimer (from StretchGraphicsItem) to the required cursorTimerOut slot.
     *
     */ 
   void initStretchLine(); 
      
protected:
      
   /** \name Geometry
     * @{
     */
   float m_start_x {0};
   float m_start_y {0};
     
   float m_end_x {0}; 
   float m_end_y {0};

   float m_mv_x0 {0};
   float m_mv_y0 {0};
   
   float m_mv_ang0 {0};

public:

   /// Get the current length of the line 
   /**
     * \returns the length of the line
     */ 
   float length();
   
   /// Get the current angle of the line 
   /**
     * \returns the angle of the line
     */
   float angle();
   
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
   
signals:
   void moved(StretchLine * s);
   void resized(StretchLine * s);
   void rejectMouse(StretchLine * s);
   void mouseIn(StretchLine * s);
   void mouseOut(StretchLine * s);
   void remove(StretchLine * s);   
   
   ///@}
   

};

#endif //__StretchLine_h__

