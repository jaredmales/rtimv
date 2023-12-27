#ifndef StretchGraphicsItem_hpp
#define StretchGraphicsItem_hpp

#include <iostream>

#include <QTimer>
#include <QPen>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QObject>
#include <QKeyEvent>

/** \todo consider making this an old-school virtual base class.  It would be a lot cleaner and simplify code in rtimvMainWindow. 
  * However this will make it harder to make it shape agnostic.  Maybe there is a virtual interface parent class?
  */

/// A CRTP bolt-on to provide the rtimv stretch-item logic
/** 
  *
  * Required interfaces in the derived class:
  * - The timer must be connected in the constructor with the code:
  * \code
   connect(&m_cursorTimer, SIGNAL(timeout()), this, SLOT(cursorTimerOut()));
  \endcode
  * - Handler functions which must call the associated handleXXXXX function
  * of this class. For example:
  * \code
   void hoverMoveEvent(QGraphicsSceneHoverEvent * e)
   {
      handleHoverMoveEvent(e); // provided by StretchGraphicsItem
   }
   \endcode
   The other required handlers are:
   \code
   void hoverLeaveEvent(QGraphicsSceneHoverEvent * e);
   void mousePressEvent ( QGraphicsSceneMouseEvent * event );
   void mouseReleaseEvent(QGraphicsSceneMouseEvent * event );
   void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
   void keyPressEvent(QKeyEvent * ke);
   \endcode   
  *
  * - A slot for the cursor timer which calls the timeout handler.
  \code
  protected slots: 
     virtual void cursorTimerOut()
     {
        slotCursorTimerOut(); // provided by StretchGraphicsItem
     }
  \endcode
  *
  * - Signal emitters must be provided.  For example:
  \code
   void emitMoved()
   {
      emit moved(this);
   }
   \endcode
  * The remaining required emitters are:
  \code
   void emitResized();
   void emitRejectMouse();
   void emitMouseIn();
   void emitMouseOut();
   void emitRemove();
  \endcode   
  * - The signals themselves must be declared in the class header:
  \code
  signals:
   void moved(StretchCircle * s);
   void resized(StretchCircle * s);
   void rejectMouse(StretchCircle * s);
   void mouseIn(StretchCircle * s);
   void mouseOut(StretchCircle * s);
   void remove(StretchCircle * s);   
  \endcode  
  *
  * The above are all straightforward, simply interfacing the QOBJECT to the StretchGraphicsItem logic.
  * More complicated are the helper functions which must be implemented.  These are used to deal with the 
  * geometry of the implemented item, to then determine cursor state and provide item specific functionality.
  \code
   bool onHoverComputeSizing(QGraphicsSceneHoverEvent * e);
   bool onMousePressCalcGrabbed(QGraphicsSceneMouseEvent * e);
   void setGrabbedGeometry(QGraphicsSceneMouseEvent * e);
   void setMovingGeometry(QGraphicsSceneMouseEvent * e);
   void sizingCalcNewPos(QGraphicsSceneMouseEvent * e);
   void movingCalcNewPos(QGraphicsSceneMouseEvent * e);
   \endcode
  *
  * Finally you need to provide a key press handler.  The keyPressEvent will
  * handle the `delete` key if the item is removable (by emitting the remove signal)
  * but pass anything else to:
  *
   \code
   void passKeyPressEvent(QKeyEvent * ke)
   {
      QGraphicsEllipseItem::keyPressEvent(ke); //For example this passes to the Qt parent
   }
   \endcode
  */  
template<class _QGraphicsItemT>
class StretchGraphicsItem 
{
   
public:

    StretchGraphicsItem();
   
protected:
   
    /// Initialize the stretch item
    void initStretchGraphicsItem();
   
protected:
    /** \name Geometry
      * @{
      */
    
    
    ///@}
    

    /** \name Sizing and Moving State
      * @{
      */
    
    ///Is this item stretchable?  If not goes straight to grab for move.
    /** If stretchable == false, then mouse over goes straight to the hand.
      */
    bool m_stretchable {true};
    
    ///Is this item removable?  If not delete key is ignored.
    /**
      */
    bool m_removable {true};
    
    ///Tracks whether the item is being resized.
    bool m_isSizing {false};
    
    ///Describes how the item is being resized.
    enum sizingMode{szOff, szLeft, szTopl, szTop, szTopr, szRight, szBotr, szBot, szBotl, szAll};
    
    ///Tracks how the item is currently being resized.
    int m_sizing {szOff};

    ///Trackes whether the item is moving.
    bool m_isMoving {false};

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
    
    /// Set whether this object is removable.
    /** Sets m_removable.  If true, then the delete key causes the remove signal to be emitted.
      * If false, then the delete key is ignore.
      */ 
    void setRemovable(bool rm /**< [in] true if removable, false for not removable*/);
    
    /// Get whether this object is removable.
    bool removable();
    
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

    QTimer m_selectionTimer; ///< This is started on mouse in, and if mouse stays in for long enough it sets selected.

    /// Set the cursor status, including changing state and displayed cursor.
    void setCursorStatus(int cs);
   
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
   
protected:
    /// When the cursor timer times-out, the cursor is changed to the double-arrow
    void slotCursorTimerOut();

    void slotSelectionTimerOut();

    ///@}

protected:
   
    /** \name Event Handlers
      * @{
      */
    void handleHoverMoveEvent(QGraphicsSceneHoverEvent * e);

    void handleHoverLeaveEvent(QGraphicsSceneHoverEvent * e);

    void handleMousePressEvent(QGraphicsSceneMouseEvent * e);
    
    void handleMouseReleaseEvent(QGraphicsSceneMouseEvent * e );

    void handleMouseMoveEvent(QGraphicsSceneMouseEvent * e);

    void handleKeyPressEvent(QKeyEvent * ke);

    void handleFocusOutEvent(QFocusEvent * e);

    ///@}

   
public:
   
    /// Get Color
    /** Gets the pen color.
      * 
      * \returns the current pen color.
      */ 
    QColor penColor();

    /// Set Color
    void setPenColor(const QColor & newcol /**< [in] the new pen color*/);
    
    /// Get the width
    /** Gets the pen width.
      *
      * \returns the current pen width
      */ 
    float penWidth();
    
    /// Set the width
    /** Sets the pen width 
      */
    void setPenWidth( float newwidth /**< [in] the new pen width*/);
   
private:
   
    _QGraphicsItemT * derived()
    {
       return static_cast<_QGraphicsItemT *>(this);
    }

};

template<class QGraphicsItemT>
StretchGraphicsItem<QGraphicsItemT>::StretchGraphicsItem() 
{
   initStretchGraphicsItem();
   return;
}
   
template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::initStretchGraphicsItem()
{   
    derived()->setFlag(QGraphicsItem::ItemIsFocusable, true);
    derived()->setFlag(QGraphicsItem::ItemIsSelectable, true);
    derived()->setAcceptHoverEvents(true);
   
    setCursorStatus (cursorOff);
   
    m_cursorTimer.setSingleShot(true);
    m_selectionTimer.setSingleShot(true);
        
}   

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::setStretchable(bool ss)
{
   m_stretchable = ss;
}

template<class QGraphicsItemT>
bool StretchGraphicsItem<QGraphicsItemT>::stretchable()
{
   return m_stretchable;
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::setRemovable(bool rm)
{
   m_removable = rm;
}

template<class QGraphicsItemT>
bool StretchGraphicsItem<QGraphicsItemT>::removable()
{
   return m_removable;
}
   
template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::setCursorStatus(int cs)
{
    if(cs == cursorOff)
    {
        m_cursorTimer.stop();
        m_grabbing = false;
        m_isMoving = false;
        m_sizing = szOff;
        derived()->setCursor(QCursor(Qt::ArrowCursor));
        m_cursorStatus = cursorOff;
        return;
    }
    
    if(cs == cursorGrabbing)
    {
        if(m_grabbing) return;
        derived()->setCursor(QCursor(Qt::OpenHandCursor));
        m_grabbing = true;
        m_cursorTimer.start(m_cursorTimeout);
        m_cursorStatus = cursorGrabbing;
        return;
    }
    
    if(cs == cursorSizing)
    {
        m_grabbing = false;

        if(m_sizing == szTopl)  derived()->setCursor(QCursor(Qt::SizeFDiagCursor));
        if(m_sizing == szBotl)  derived()->setCursor(QCursor(Qt::SizeBDiagCursor));
        if(m_sizing == szTopr)  derived()->setCursor(QCursor(Qt::SizeBDiagCursor));
        if(m_sizing == szBotr)  derived()->setCursor(QCursor(Qt::SizeFDiagCursor));
        if(m_sizing == szLeft)  derived()->setCursor(QCursor(Qt::SizeHorCursor));
        if(m_sizing == szRight) derived()->setCursor(QCursor(Qt::SizeHorCursor));
        if(m_sizing == szTop)   derived()->setCursor(QCursor(Qt::SizeVerCursor));
        if(m_sizing == szBot)   derived()->setCursor(QCursor(Qt::SizeVerCursor));
        if(m_sizing == szAll)   derived()->setCursor(QCursor(Qt::SizeAllCursor));
        m_cursorStatus = cursorSizing;

        return;
    }
    
    if(cs == cursorGrabbed)
    {
        m_cursorTimer.stop();
        derived()->setCursor(QCursor(Qt::ClosedHandCursor));
        m_isMoving = true;
        m_grabbing = false;
        m_cursorStatus = cursorGrabbed;
    
        return;
    }
}

template<class QGraphicsItemT>
int StretchGraphicsItem<QGraphicsItemT>::edgeTol()
{ 
    return m_edgeTol;
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::setEdgeTol(int et)
{ 
    m_edgeTol = et;
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::slotCursorTimerOut()
{
    setCursorStatus(cursorSizing);
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::slotSelectionTimerOut()
{
    derived()->setFocus();
    derived()->setSelected(true);
    derived()->emitSelected();
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleHoverMoveEvent(QGraphicsSceneHoverEvent * e)
{   
    m_selectionTimer.stop();

    if(!m_stretchable)
    {
        derived()->emitRejectMouse();
        return;
    }
   
    bool inTol = derived()->onHoverComputeSizing(e);
   
    if(!inTol)
    {
        setCursorStatus(cursorOff);
        derived()->clearFocus();
        derived()->setSelected(false);
        derived()->emitMouseOut();
        return; 
    }
   
    derived()->emitMouseIn();
    m_selectionTimer.start(m_cursorTimeout);

    if(m_cursorStatus == cursorOff)
    {
        setCursorStatus(cursorGrabbing);
    }
   
    if(m_cursorStatus == cursorSizing)
    {
        setCursorStatus(cursorSizing);
    }
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleHoverLeaveEvent(QGraphicsSceneHoverEvent * e)
{  
    static_cast<void>(e);

    m_selectionTimer.stop();

    if(!m_stretchable)
    {
        derived()->emitRejectMouse();
        return;
    }
   
    setCursorStatus(cursorOff);
    derived()->clearFocus();
    derived()->setSelected(false);
    derived()->emitDeSelected();
    derived()->emitMouseOut();
}
   
template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleMousePressEvent(QGraphicsSceneMouseEvent * e)
{
    m_selectionTimer.stop();

    if(!m_stretchable)
    {
        derived()->emitRejectMouse();
        return;
    }
   
    derived()->setSelected(true);
    derived()->emitSelected();
    if(m_grabbing || !m_stretchable)
    {
        bool onGrab = derived()->onMousePressCalcGrabbed(e);
      
        if(onGrab == false)
        {
            setCursorStatus(cursorOff);
            return;
        }
        else
        {
            setCursorStatus(cursorGrabbed);
            derived()->setGrabbedGeometry(e);
            m_isMoving=true;
            m_isSizing=false;
            m_sizing=szOff;
            return;
        }
    }
    else if(m_sizing)
    {
        m_grabbing = false;
        derived()->setMovingGeometry(e);
    }
    else
    {
        derived()->emitRejectMouse();
    }
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleMouseReleaseEvent(QGraphicsSceneMouseEvent * event )
{
    m_selectionTimer.stop();

    if(!m_stretchable)
    {
        derived()->emitRejectMouse();
        return;
    }
   
    static_cast<void>(event);
    if(m_sizing)
    {
        if(m_isSizing)
        {
            m_sizing = szOff;
            m_isSizing = false;
            derived()->emitMoved();
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
        derived()->emitMoved();
        setCursorStatus(cursorGrabbing);
        return;
    }
    setCursorStatus(cursorOff);
}
   
template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleMouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if(!m_stretchable)
    {
        derived()->emitRejectMouse();
        return;
    }
    
    if(m_sizing && !m_isMoving)
    {
        m_isSizing = true;

        derived()->sizingCalcNewPos(event);

        derived()->emitResized();

    }
    else if(m_isMoving)
    {
        m_isSizing = false;
        derived()->movingCalcNewPos(event);
        derived()->emitMoved();
    }
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleKeyPressEvent(QKeyEvent * ke)
{
    if(ke->key() == Qt::Key_Delete && m_removable)
    {
        derived()->emitRemove();
        return;
    }

    derived()->passKeyPressEvent(ke);
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::handleFocusOutEvent(QFocusEvent * e)
{
    static_cast<void>(e);
    setCursorStatus(cursorOff);
}

template<class QGraphicsItemT>
QColor StretchGraphicsItem<QGraphicsItemT>::penColor()
{
    return derived()->pen().color();
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::setPenColor(const QColor & newcol)
{
    QPen p = derived()->pen();
    p.setColor(newcol);
    derived()->setPen(p);
}

template<class QGraphicsItemT>
float StretchGraphicsItem<QGraphicsItemT>::penWidth()
{
    return derived()->pen().widthF();
}

template<class QGraphicsItemT>
void StretchGraphicsItem<QGraphicsItemT>::setPenWidth( float newwidth)
{
    QPen p = derived()->pen();
    p.setWidthF(newwidth);
    derived()->setPen(p);
}
   
#endif //StretchGraphicsItem_hpp
