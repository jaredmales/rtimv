
#ifndef __StretchCircle_h__
#define __StretchCircle_h__


#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QObject>
#include <QTimer>
#include <QTextEdit>
#include <QKeyEvent>

#include "StretchGraphicsItem.hpp"

class StretchCircle : public QObject, public QGraphicsEllipseItem
{
   Q_OBJECT

public:

   StretchCircle(QGraphicsItem * parent = 0);
   StretchCircle(const QRectF & rect, QGraphicsItem * parent = 0 );
   StretchCircle(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0 );
   
private:
   void initStretchCircle(); ///< Initialize the stretch circle
      
protected:
      
   /** \name Sizing and Moving State
     * @{
     */
   
   ///Tracks whether the circle is being resized.
   bool m_isSizing {false};
   
   ///Describes how the circle is being resized.
   enum sizingMode{szOff, szLeft, szTopl, szTop, szTopr, szRight, szBotr, szBot, szBotl};
   
   ///Tracks how the circle is currently being resized.
   int m_sizing {szOff};
      
   ///Trackes whether the circle is moving.
   bool m_isMoving {false};

   ///Is this circle stretchable?  If not goes straight to grab for move.
   /** If stretchable == false, then mouse over goes straight to the hand.
     */
   bool m_stretchable {true};
   
public:

   /// Set whether this object is stretchable
   /** Sets m_stretchable
     */
   void setStretchable(bool ss /**< [in] true for stretchable, false for not stretchable*/);
   
   /// Get whether or not the object is strechable.
   /** Returns the value of m_stretchable.
     * \returns true if the object is stretchable
     * \returns false if the object is not stretchable
     */
   bool stretchable();
   
   ///@}

   /** \name Geometry
     * @{
     */
   
   float m_ul_x; ///< The x-coordinate of the upper left corner of the bounding box
   float m_ul_y; ///< The y-coordinate of the upper left corner of the bounding box
   float m_width;  ///< The width of the bounding box
   float m_height; ///< The height of the bounding box
   float m_cen_x;  ///< The x-coordinate of the center of the bounding box
   float m_cen_y;  ///< The x-coordinate of the center of the bounding box
   
   float m_mv_x0; ///< The x-coordinate of the mouse at the start of a re-size
   float m_mv_y0; ///< The y-coordinate of the mouse at the start of a re-size
   float m_drad0; ///< The radius of the mouse position at the start of a re-size
   
public:
   
   /// Get the current radius of the circle 
   /**
     * \returns the radius
     */ 
   float radius();
   
   ///@}
   
      
   /** \name Cursor
     *@{
     */
   
   int m_edgeTol {5}; ///< Tolerance in pixels for selection, within this distance this object accepts the mouse.
   
   ///Possible cursor status values.
   enum cursorStatus{cursorOff, cursorGrabbing, cursorSizing, cursorGrabbed};
      
   int m_cursorStatus; ///< The cursor status.
   
   float m_cursorAngle; ///< The angle of the cursor position, for determining which double-arrow cursor to apply.
   
   bool m_grabbing {false}; ///< Whether or not the cursor is in grabbing (open-hand) mode (true) or has grabbed (false).
   
   int m_cursorTimeout {750}; ///< The time out to change to double arrow from open-hand (in milliseconds)
      
   QTimer m_cursorTimer; ///< When this times out change cursor from open-hand to double-arrow

   /// Set the cursor status, including changing state and displayed cursor.
   void setCursorStatus(int cs /**< [in] the new cursor status, one of the cursorStatus values*/ );
   
public:
   
   /// Get the edge tolerance
   /**
     * \returns the edge tolerance in pixels, the current value of m_edgeTol
     */ 
   int edgeTol();
   
   /// Set the edge tolerance
   /** 
     */
   void setEdgeTol(int et);
    
protected slots:

   /// When the cursor timer times-out, the cursor is changed to the double-arrow
   virtual void cursorTimerOut();

   ///@}

protected:
   
   /** \name Event Handlers
     * @{
     */
   void hoverMoveEvent(QGraphicsSceneHoverEvent * e);
   void hoverLeaveEvent(QGraphicsSceneHoverEvent * e);
      
   void mousePressEvent ( QGraphicsSceneMouseEvent * event );
   void mouseReleaseEvent(QGraphicsSceneMouseEvent * event );
      
   void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

   void keyPressEvent(QKeyEvent * ke);

   ///@}

   /** \name Signals
     * @{
     */
signals:
   void moved(StretchCircle * s);
   void resized(StretchCircle * s);
   void rejectMouse(StretchCircle * s);
   void mouseIn(StretchCircle * s);
   void mouseOut(StretchCircle * s);
   void remove(StretchCircle * s);   
   
   ///@}
   
public:
   
   /// Get Color
   /** Gets the pen color.
     * 
     * \returns the current pen color.
     */ 
   QColor penColor();
   
   /// Set Color
   void penColor(const QColor & newcol /**< [in] the new pen color*/);
   
   /// Get the width
   /** Gets the pen width.
     *
     * \returns the current pen width
     */ 
   float penWidth();
   
   /// Set the width
   /** Sets the pen width 
     */
   void penWidth( float newwidth /**< [in] the new pen width*/);
   
   
   
        
   

};

#endif //__StretchCircle_h__

