/** \file StretchBox.hpp
 * \brief Movable, resizable scene rectangle with optional user-edit notifications.
 */
#ifndef __StretchBox_hpp__
#define __StretchBox_hpp__

#include <QGraphicsRectItem>

#include "StretchGraphicsItem.hpp"

/// Scene rectangle sharing user-shape movement, resizing, and selection behavior.
class StretchBox : public QObject, public QGraphicsRectItem, public StretchGraphicsItem<StretchBox>
{
    Q_OBJECT

    friend class StretchGraphicsItem<StretchBox>;

  public:
    typedef QRectF coordSpecT;

    /// Construct an editable rectangle with optional scene ownership.
    StretchBox( QGraphicsItem *parent = 0 /**< [in] Optional owning graphics item. */ );

    /// Construct an editable rectangle with optional scene ownership.
    StretchBox( const QRectF &rect /**< [in] Initial rectangle in scene coordinates. */,
                QGraphicsItem *parent = 0 /**< [in] Optional owning graphics item. */ );

    /// Construct an editable rectangle with optional scene ownership.
    StretchBox( qreal x /**< [in] Horizontal coordinate. */,
                qreal y /**< [in] Vertical coordinate. */,
                qreal width /**< [in] Rectangle width in scene pixels. */,
                qreal height /**< [in] Rectangle height in scene pixels. */,
                QGraphicsItem *parent = 0 /**< [in] Optional owning graphics item. */ );

  private:
    /// Initialize the stretch rectangle
    /** This connects m_cursorTimer (from StretchGraphicsItem) to the required cursorTimerOut slot.
     *
     */
    void initStretchBox();

    /** \name Geometry - Data
     * @{
     */
  protected:
    bool m_maintainCenter{
        false }; ///< Sets whether the center coordinate is maintained (true) when stretching, or if it moves (false)

    float m_ul_x{ 0 };   ///< The x-coordinate of the upper left corner of the bounding box
    float m_ul_y{ 0 };   ///< The y-coordinate of the upper left corner of the bounding box
    float m_width{ 0 };  ///< The width of the bounding box
    float m_height{ 0 }; ///< The height of the bounding box
    float m_mv_x0{ 0 };  ///< The x-coordinate of the mouse at the start of a re-size
    float m_mv_y0{ 0 };  ///< The y-coordinate of the mouse at the start of a re-size

    ///@}

    /** \name Geometry
     * @{
     */
  public:
    /// Get whether or not the center is maintained when stretching.
    /** \returns the current value of m_maintainCenter
     */
    bool maintainCenter();

    /// Set whether to maintain the center when stretching.
    void
    setMaintainCenter( bool mc /**< [in] Set m_maintainCenter to true to maintain the center, false to move it.*/ );

    ///@}
  protected:
    /** \name Event Handlers
     * Each of these calls the associated handleXXXX from StretchGraphicsItem
     *
     * @{
     */
    /// Update hover selection and sizing intent for the rectangle.
    void hoverMoveEvent( QGraphicsSceneHoverEvent *e /**< [in] Qt event to process. */ );

    /// Clear hover selection when the pointer leaves the rectangle.
    void hoverLeaveEvent( QGraphicsSceneHoverEvent *e /**< [in] Qt event to process. */ );

    /// Begin a user move or resize and emit its start notification.
    void mousePressEvent( QGraphicsSceneMouseEvent *event /**< [in] Qt event to process. */ );

    /// Complete a user move or resize and emit one completion notification.
    void mouseReleaseEvent( QGraphicsSceneMouseEvent *event /**< [in] Qt event to process. */ );

    /// Update local rectangle geometry during a mouse edit.
    void mouseMoveEvent( QGraphicsSceneMouseEvent *event /**< [in] Qt event to process. */ );

    /// Process removal keys and forward other input to the scene item.
    void keyPressEvent( QKeyEvent *ke /**< [in] Keyboard event to dispatch. */ );

    ///@}

    /** \name StretchGraphicsItem Interface
     *
     * @{
     */

    /// Select a rectangle edge or corner from the hover position.
    bool onHoverComputeSizing( QGraphicsSceneHoverEvent *e /**< [in] Qt event to process. */ );

    /// Return whether the press falls within the draggable border.
    bool onMousePressCalcGrabbed( QGraphicsSceneMouseEvent *e /**< [in] Qt event to process. */ );

    /// Capture rectangle position at the start of movement.
    void setGrabbedGeometry( QGraphicsSceneMouseEvent *e /**< [in] Qt event to process. */ );

    /// Capture rectangle edges at the start of resizing.
    void setMovingGeometry( QGraphicsSceneMouseEvent *e /**< [in] Qt event to process. */ );

    /// Resize the captured rectangle from the current pointer position.
    void sizingCalcNewPos( QGraphicsSceneMouseEvent *e /**< [in] Qt event to process. */ );

    /// Move the captured rectangle from the current pointer position.
    void movingCalcNewPos( QGraphicsSceneMouseEvent *e /**< [in] Qt event to process. */ );

    /// Forward unhandled keys to the graphics item.
    void passKeyPressEvent( QKeyEvent *ke /**< [in] Keyboard event to dispatch. */ );

    /// Clear the cursor state when the rectangle loses focus.
    void focusOutEvent( QFocusEvent *e /**< [in] Qt event to process. */ );

  protected slots:

    /// When the cursor timer times-out, the cursor is changed to the double-arrow
    virtual void cursorTimerOut();

    /// When the selection timer times-out, the item is selected without clicking
    virtual void selectionTimerOut();

    ///@}

  public:
    /// Remove this box from the list
    void remove();

    /// Cancel unsent mouse editing without removing the box.
    void cancelEdit();

    /** \name Signals
     * The emitXXXX are required by StretchGraphicsItem, and each
     * emits the associated signal.
     * @{
     */
    /// Emit the moved rectangle notification.
    void emitMoved();

    /// Emit the resized rectangle notification.
    void emitResized();

    /// Emit the rejectMouse rectangle notification.
    void emitRejectMouse();

    /// Emit the mouseIn rectangle notification.
    void emitMouseIn();

    /// Emit the mouseOut rectangle notification.
    void emitMouseOut();

    /// Emit the remove rectangle notification.
    void emitRemove();

    /// Emit the selected rectangle notification.
    void emitSelected();

    /// Emit the deSelected rectangle notification.
    void emitDeSelected();

  signals:
    /// A user has begun moving or resizing the box.
    void editStarted( StretchBox *box /**< [in] Edited box. */ );

    /// A user has released the box after moving or resizing it.
    void editFinished( StretchBox *box /**< [in] Edited box. */ );

    /// Report that rectangle geometry moved.
    void moved( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Report that rectangle dimensions changed.
    void resized( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Yield pointer interaction to other scene items.
    void rejectMouse( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Report that the pointer entered the active rectangle border.
    void mouseIn( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Report that the pointer left the active rectangle border.
    void mouseOut( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Request removal of the rectangle from its owning scene.
    void remove( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Report selection of the rectangle.
    void selected( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    /// Report deselection of the rectangle.
    void deSelected( StretchBox *s /**< [in] Rectangle sending this notification. */ );

    ///@}
};

#endif //__StretchBox_h__
