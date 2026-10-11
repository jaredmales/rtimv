
/** \file rtimvMainWindow.hpp
 * \brief Declaration of the rtimvMainWindow class.
 *
 *
 */

#ifndef rtimvMainWindow_hpp_inc
#define rtimvMainWindow_hpp_inc

#include <cstdio>
#include <cmath>
#include <functional>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include <QWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QPluginLoader>
#include <QDir>
#include <QMainWindow>
#include <QPixmap>

// This file defines the base class, and must also define the RTIMV_BASE class
// Example:
// -DRTIMV_BASE_INCLUDE="rtimvBase.hpp"
//
// where "rtimvBase.hpp" includes:
// #define RTIMV_BASE rtimvBase
//
#include RTIMV_BASE_INCLUDE

#include "ui_rtimvMainWindow.h"

#include "rtimvControlPanel.hpp"
#include "cubeCtrl.hpp"

#include "rtimvInterfaces.hpp"
#include "rtimvCommandInterface.hpp"

#include "StretchBox.hpp"
#include "StretchCircle.hpp"
#include "StretchLine.hpp"

#include "rtimvStats.hpp"

#define SCALEMODE_USER 3

#define PointerViewEnabled 0
#define PointerViewOnPress 1
#define PointerViewDisabled 2
#define PointerViewModeMax 3

#define ViewViewEnabled 0
#define ViewViewNoImage 1
#define ViewViewModeMax 2

#ifdef RTIMV_GRPC
    #define RTIMV_MOUSE_PIXEL_TIMEOUT_DEFAULT 35
    #define RTIMV_COLOR_BOX_TIMEOUT_DEFAULT 75
    #define RTIMV_STATS_BOX_TIMEOUT_DEFAULT 75
#else
    #define RTIMV_MOUSE_PIXEL_TIMEOUT_DEFAULT 5
    #define RTIMV_COLOR_BOX_TIMEOUT_DEFAULT 5
    #define RTIMV_STATS_BOX_TIMEOUT_DEFAULT 5
#endif

class rtimvControlPanel;

// #define RTIMV_DEBUG_BREADCRUMB std::cerr << __FILE__ << " " << __LINE__ << "\n";
#define RTIMV_DEBUG_BREADCRUMB

/// Image viewer, graphics scene owner, and optional plugin host.
class rtimvMainWindow : public QWidget, public RTIMV_BASE
{
    Q_OBJECT

  public:
    /// Construct and configure the image viewer and its plugin host.
    rtimvMainWindow( int argc /**< [in] Number of command-line arguments. */ , char **argv /**< [in] Command-line arguments. */ , QWidget *Parent = nullptr /**< [in] Optional owning Qt widget. */ , Qt::WindowFlags f = Qt::WindowFlags() /**< [in] Qt window flags. */ );

    /// Release viewer resources and plugin ownership.
    ~rtimvMainWindow();

    /// Register the viewer configuration options.
    virtual void setupConfig();

    /// Load configured display and plugin settings.
    virtual void loadConfig();

  public slots:
    /// Show a normal transient message using the existing status box.
    void showStatusMessage(const QString &message /**< [in] Short user-facing message. */);

    /// Return the existing new-user-box placement in image scene coordinates.
    QRectF newUserBoxRect();

    /// Return actual displayed image dimensions for overlay geometry validation.
    QSize overlayImageSize() const;

    /// Add a plugin box with the same interactive presentation as a user box.
    void addInteractiveBox(StretchBox *box /**< [in] Scene-owned box supplied by a plugin. */);

  protected:
    rtimvCommandRouter *m_commands{nullptr}; ///< Owned optional plugin command router.

    /// Weak references to overlay boxes sharing user-box scale and pointer presentation.
    QVector<QPointer<StretchBox>> m_overlayBoxes;


    /// Configured viewer title.
    std::string m_title{ "rtimv" };

  public:
    /// Called on initial connection to the image stream, sets matching aspect ratio.
    virtual void onConnect();

    /// Rebuild image-dependent display geometry while the image mutex is held.
    virtual void mtxL_postSetImsize( const uniqueLockT &lock /**< [in] Image mutex lock already held by the caller. */ );

    /// Update view transforms and screen-scaled overlay geometry after zoom changes.
    virtual void post_zoomLevel();

  private:
    /// Generic implementation of postRecolor
    template <class lockT>
    /// Refresh image presentation with the supplied image lock held.
    void mtxL_postRecolorImpl( const lockT &lock /**<[in] a mutex lock which is locked*/ );

  public:
    /// Refresh the recolored image with the supplied image lock held.
    virtual void mtxL_postRecolor( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/ );

    /// Refresh the recolored image with the supplied image lock held.
    virtual void mtxL_postRecolor( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    /// Refresh pixel-dependent display state while the image mutex is held.
    virtual void mtxL_postChangeImdata( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    /// Refresh the displayed frame rate.
    virtual void updateFPS();

    /// Refresh the displayed image age.
    virtual void updateAge();

    /// Update the display while not connected.
    virtual void updateNC();

    /// Dispatch keyboard input to display controls and enabled overlays.
    virtual void keyPressEvent( QKeyEvent * ke /**< [in] Keyboard event to dispatch. */ );

    /*** The control Panel ***/
  protected:
    /// Optional image-control panel owned by this window.
    rtimvControlPanel *imcp;

    /// Control-panel magnification while the pointer is over the image.
    float pointerOverZoom;

  public:
    /// Create or reveal the image control panel.
    void launchControlPanel();

    /// Return the current horizontal center in fractional image coordinates.
    float get_act_xcen();
    /// Return the current vertical center in fractional image coordinates.
    float get_act_ycen();

    /// Set the pointer-centered magnification used by the control-panel view.
    void setPointerOverZoom( float poz /**< [in] Magnification while the pointer is over the control-panel view. */ );

    /** \name Cube Control - Data
     * @{
     */
  protected:
    /// Optional image-cube controller owned by this window.
    cubeCtrl *m_cubeCtrl{ nullptr };

    ///@}

    /** \name Cube Control
     * @{
     */
  public:
    /// Create or reveal the image-cube controller.
    void launchCubeCtrl();

    /// Toggle image-cube controller visibility.
    void toggleCubeCtrl();

    ///@}

    /*** Graphics stuff ***/
  protected:
    QPixmap m_qpm; ///< A QT pixmap, used to prep the QImage for display.

    /// Owned graphics scene containing the image and overlay items.
    QGraphicsScene *m_qgs{ nullptr };
    /// Scene-owned item presenting the current image pixmap.
    QGraphicsPixmapItem *m_qpmi{ nullptr };

    /// Scale from image pixels to the fitted viewport at unit zoom.
    float m_screenZoom;

  public:
    /// Get a pointer to the pixmap
    /**
     * \returns a pointer m_qpm
     */
    QPixmap * getPixmap();

    /// Get a pointer to the graphics scene
    /**
     * \returns a pointer m_qgs
     */
    QGraphicsScene *get_qgs();

    /** \name North Arrow - Data
     * The north arrow is toggled on/off with the `n` key.  The angle of the north arrow returned by northAngle() is
     * determined by the dictionary key "rtimv.north.angle", which can be set from a dictionary plugin or using the slot
     * northAngleRaw(float).  This is multiplied by m_northAngleScale and then m_northAngleOffset is added to the result
     * to produce a counter clockwise rotation angle in degrees.
     *
     * The north arrow can be disabled in configuration.  If "north.enabled=false" is set, the north arrow will not be
     * displayed.
     *
     * Relevant configuration key=value pairs are
     * - `north.enabled=true/false`: Whether or not to enable the north arrow. Default is true.
     * - `north.offset=float`: Offset in degrees c.c.w. to apply to the north angle. Default is 0.
     * - `north.scale=float`: Scaling factor to apply to north angle to convert to degrees c.c.w. on the image.  Default
     * is -1.
     * @{
     */
  protected:
    QGraphicsLineItem *m_northArrow; ///< The north arrow line.

    QGraphicsLineItem *m_northArrowTip; ///< The north arrow tip line.

    bool m_northArrowEnabled{ true }; ///< Flag controlling whether or not the north arrow is enabled.

    float m_northAngleScale{ -1 }; /**< The scale to multiply the value contained in "rtimv.north.angle"
                                        to convert to degrees c.c.w.*/

    float m_northAngleOffset{ 0 }; ///< The offset to apply to the scaled north angle in degrees c.c.w.

    /// Center the north arrow on the visible image.
    void centerNorthArrow(); ///< Center the north arrow on the current display center.

    /// Refresh the north arrow orientation from the dictionary.
    void updateNorthArrow(); ///< Update the angle of the north arrow.

    ///@}

    /** \name North Arrow
     * @{
     */
  public:
    /// Get the angle of North on the image display
    /** Uses "rtimv.north.angle" in the dictionary to fine the current `north` for the target, then calculates:
     *   north = -1*(m_northAngleOffset + m_northAngleScale*north)
     * and returns the result.
     *
     * \returns the angle of north on the image display if "rtimv.north.angle" is set
     * \returns 0 otherwise
     */
    float northAngle();

  public slots:

    /// Set the value of the north angle in the dictionary, for use by northAngle()
    /** Sets the value of "rtimv.north.angle" in the dictionary.
     *
     */
    void northAngleRaw( float north /**< [in] the new value of the raw north angle*/ );

    ///@}

    /*** Real Time Controls ***/
  public slots:
    /// Toggle live image updates.
    void freezeRealTime();

    /// Reset the display stretch using the current image.
    void reStretch();

    /*** The Main View ***/
  public:
    /// Apply the current center to the viewport and optional zoom box.
    void change_center( bool movezoombox = true /**< [in] Whether to update the control-panel zoom box. */ );

  private:
    /// Apply fractional viewport coordinates with the image lock already held.
    void mtxL_setViewCen_impl( float x /**< [in] Horizontal coordinate. */ , float y /**< [in] Vertical coordinate. */ , bool movezoombox /**< [in] Whether to update the control-panel zoom box. */ );

  public:
    /// Lock m_calMutex, then update the viewport center.
    void mtxUL_setViewCen( float x /**< [in] new x center in fractional image coordinates*/,
                           float y /**< [in] new y center in fractional image coordinates*/,
                           bool movezoombox = true /**< [in] whether to move the zoom box graphic*/ );

    /// Change the viewport center while the supplied image lock is held.
    void mtxL_setViewCen( float x /**< [in] Horizontal coordinate. */ , float y /**< [in] Vertical coordinate. */ , const uniqueLockT &lock /**< [in] Image mutex lock already held by the caller. */ , bool movezoombox = true /**< [in] Whether to update the control-panel zoom box. */ );

    /// Change the viewport center while the supplied image lock is held.
    void mtxL_setViewCen( float x /**< [in] Horizontal coordinate. */ , float y /**< [in] Vertical coordinate. */ , const sharedLockT &lock /**< [in] Image mutex lock already held by the caller. */ , bool movezoombox = true /**< [in] Whether to update the control-panel zoom box. */ );

    /// Return the horizontal viewport center in scene coordinates.
    float get_xcen();

    /// Return the vertical viewport center in scene coordinates.
    float get_ycen();

    /// Matches the viewport to the same aspect ratio as the image data, by decreasing the area.
    /** Maintains the smaller of width and height, decreasing the other to match the aspect of the image.
     *
     * Invoked by the "[" key.
     *
     * \seealso squareUp()
     */
    void squareDown();

    /// Matches the viewport to the same aspect ratio as the image data, by increasing the area.
    /** Maintains the larger of width and height, increasing the other to match the aspect of the image.
     *
     * Invoked by the "]" key.
     *
     * \seealso squareDown()
     */
    void squareUp();

  protected slots:
    /// Refresh the viewport center after a Qt geometry change.
    void changeCenter();

    /// Calls changeCenter() so that the display stays centered when user resizes.
    void resizeEvent( QResizeEvent * e /**< [in] Qt event to process. */ );

    /*** pointer data***/
  protected:
    /// True while a right-button drag adjusts bias and contrast.
    bool rightClickDragging;
    /// Pointer origin of the active right-button drag.
    QPointF rightClickStart;
    /// Display bias captured at the start of the drag.
    float biasStart;
    /// Display contrast captured at the start of the drag.
    float contrastStart;

    bool m_showToolTipCoords{ true }; ///< Flag indicating whether the mouse tool tip should be shown
    bool m_showStaticCoords{ false }; ///< Flag indicating whether the mouse tool tip should be shown

    bool m_nullMouseCoords{ true }; ///< Flag indicating whether or not the mouse coordinates have been nulled.

    /// Update local rectangle geometry during a mouse edit.
    void mouseMoveEvent( QMouseEvent *e /**< [in] Qt event to process. */ );

    /// Hide stale pointer coordinates when no image position is available.
    void nullMouseCoords();

    /// Update the GUI for a change in mouse coordinates in the viewport
    /** Performs the following functions:
     * - Decides whether or not the mouse is currently in the pixmap.
     * - If it is not in the pixmap, clears the mouse coord text boxes
     * - If it is, updates the mouse coord text boxes
     * - If the mouse is right-clicked and dragging, updates the strech bias and contrast.
     */
    void mtxL_updateMouseCoords( const sharedLockT &lock /**< [in] Image mutex lock already held by the caller. */ ,
                                 bool requestPixel = true /**< [in] whether to issue a pixel-value request */ );

    QTimer m_mousePixelTimer;                                     ///< Debounce timer for mouse pixel requests.
    int m_mousePixelTimeout{ RTIMV_MOUSE_PIXEL_TIMEOUT_DEFAULT }; ///< Debounce timeout for mouse pixel requests, ms.
    bool m_mousePixelRequestPending{ false }; ///< True when a mouse pixel request should be dispatched.
    uint32_t m_mousePixelRequestX{ 0 };       ///< Pending mouse pixel request x coordinate.
    uint32_t m_mousePixelRequestY{ 0 };       ///< Pending mouse pixel request y coordinate.

    bool m_mousePixelCacheValid{ false }; ///< True when cached mouse pixel value is valid.
    uint32_t m_mousePixelCacheX{ 0 };     ///< Cached mouse pixel x coordinate.
    uint32_t m_mousePixelCacheY{ 0 };     ///< Cached mouse pixel y coordinate.
    float m_mousePixelCacheValue{ 0 };    ///< Cached mouse pixel value.

  public:
    /// Get the value of the flag controlling whether tool tip coordinates are shown
    /** \returns the current value of m_showToolTipCoords
     */
    bool showToolTipCoords();

    /// Get the value of the flag controlling whether static coordinates are shown
    /** \returns the current value of m_showStaticCoords
     */
    bool showStaticCoords();

  public slots:

    /// Receive signal to show or hide tool tip coordinates
    void showToolTipCoords( bool sttc /**< [in] The new value of the flag */ );

    /// Receive signal to show or hide static coordinates
    void showStaticCoords( bool ssc /**< [in] The new value of the flag */ );

  signals:
    /// Notify that the tool tip coordinate display flag has changed.
    void showToolTipCoordsChanged( bool sttc /**< [in] The new value of the flag */ );

    /// Notify that the static coordinate display flag has changed.
    void showStaticCoordsChanged( bool sttc /**< [in] The new value of the flag */ );

  public slots:
    /// Receive signal that the viewport mouse coordinates have changed.
    void changeMouseCoords();

    /// Update mouse coordinate display from cached pixel data without issuing a request.
    void updateMouseCoordsFromCache();

    /// Dispatch the debounced mouse pixel request.
    void dispatchMousePixelRequest();

    /// Handle a pixel-value update from the backend.
    void pixelValueUpdated( uint32_t x , /**< [in] x coordinate */
                            uint32_t y,  /**< [in] y coordinate */
                            float value, /**< [in] calibrated pixel value */
                            bool valid   /**< [in] true when value is valid */
    );

    /// Begin a left-button interaction in the image view.
    void viewLeftPressed( QPointF mp /**< [in] Pointer position in image scene coordinates. */ );
    /// Complete a left-button interaction in the image view.
    void viewLeftClicked( QPointF mp /**< [in] Pointer position in image scene coordinates. */ );

    /// Capture the starting bias and contrast for a right-button drag.
    void viewRightPressed( QPointF mp /**< [in] Pointer position in image scene coordinates. */ );
    /// Complete a right-button bias/contrast interaction.
    void viewRightClicked( QPointF mp /**< [in] Pointer position in image scene coordinates. */ );

    /// Adjust image zoom for a mouse-wheel step.
    void onWheelMoved( int delta /**< [in] Mouse-wheel angle delta. */ );

  public:
    /// @}

    /*---- Target Cross ----*/
  protected:
    float m_targetXc{ 0.5 }; ///< The x-coordinate of the target, 0<= x <= 1
    float m_targetYc{ 0.5 }; ///< The y-coordinate of the target, 0<= y <= 1

    bool m_targetVisible; ///< Flag controlling whether the target is visible

    QGraphicsLineItem *m_cenLineVert; ///< The vertical line component of the target
    QGraphicsLineItem *m_cenLineHorz; ///< The horizontal line component of the target

  public:
    /// Get the x-coordinate of the target
    /**
     * \returns the current value of m_targetXc
     */
    float targetXc();

    /// Get the y-coordinate of the target
    /**
     * \returns the current value of m_targetYc
     */
    float targetYc();

    /// Get whether or not the target is currently visible
    /**
     * \returns the current value of m_targetVisible.
     */
    bool targetVisible();

  public slots:
    /// Set the x-coordinate of the target
    /** The coordinate is set as a fraction of the image, 0 <= targetXc <= 1.0
     * It will be clamped to this range by this function..
     */
    void targetXc( float txc /**< [in] the new target x coordinate*/ );

    /// Set the y-coordinate of the target
    /** The coordinate is set as a fraction of the image, 0 <= targetYc <= 1.0
     * It will be clamped to this range by this function..
     */
    void targetYc( float tyc /**< [in] the new target y coordinate*/ );

    /// Set whether or not the target is visible
    void targetVisible( bool tv /**< [in] the new status of target visibility */ );

  signals:

    /// Notify controls that the horizontal target position changed.
    void targetXcChanged( float txc /**< [in] the new target x coordinate*/ );

    /// Notify controls that the vertical target position changed.
    void targetYcChanged( float tyc /**< [in] the new target y coordinate*/ );

    /// Notify controls that target visibility changed.
    void targetVisibleChanged( bool tv /**< [in] the new status of target visibility */ );

  public:
    /// Position the target lines
    /** This is normally called by the targetXc(float) and targetYc(float) slots
     */
    void setTarget();

    /** \name User Shapes - Data
     * @{
     */
  protected:
    /// Flag indicating that one of the user items is selected
    /** This causes its stats to be updated with the image
     */
    bool m_userItemSelected{ false };

    float m_userItemXCen{ 0 }; ///< Center of active user item in scene coordinates
    float m_userItemYCen{ 0 }; ///< Center of active user item in scene coordinates

    float m_userItemMouseViewX{ 0 }; ///< Center of active user item in viewport coordinates
    float m_userItemMouseViewY{ 0 }; ///< Center of active user item in viewport coordinates

    bool m_offsetItemMouseCoordsX{
        false }; ///< Flag to indicate that the x mouse coordinates should be offset to avoid the item.
    bool m_offsetItemMouseCoordsY{
        false }; ///< Flag to indicate that the y mouse coordinates should be offset to avoid the item.

    float m_userItemLineWidth{ 2 }; /**< The width of lines in user items in screen pixels.  Default is 2. */

    float m_userItemEdgeTol{ 5.5 }; /**< The tolerance in screen pixels for the mouse to be on the edge of
                                         a user item.  For closed shapes this applies only to the inside.
                                         Default is 5.5 */

    QGraphicsEllipseItem *m_lineHead; ///< Origin marker for user lines

    float m_userLineHeadRad{ 10 }; /**< The radius of the circle marking the head of a user line, in screen pixels.
                                       Default is 50. */

    QGraphicsLineItem *m_objCenV; ///< Center marker for user boxes and circles, the vertical part

    QGraphicsLineItem *m_objCenH; ///< Center marker for user boxes and circles, the horizontal part

    float m_userItemCrossWidthFract{ 0.1 }; /**< The half-width of the center cross, relative to the smallest
                                                 dimension of the item. Default is 0.1. */

    float m_userItemCrossWidthMin{ 5 }; /**< The minimum half-width of the center cross, in screen pixels.
                                             Default is 5. */

    float m_warningBorderWidth{ 5 }; /**< The width of the warning border in screen pixels.  Default is 5. */

    ///@}

    /** \name User Shapes
     * @{
     */
  public:
    /// Update the mouse coordinates at the center of the active user item
    void mtxTry_userItemMouseCoords( float mx, /**< [in] the scene x-coordinate of the center */
                                     float my, /**< [in] the scene y-coordinate of the center */
                                     float dx, /**< [in] the viewport x-coordinate of the center */
                                     float dy  /**< [in] the viewport y-coordinate of the center */
    );

    /// Initialize a user item upon being selected
    /** Clears the size and coordinate text fields and sets their color and visibility.  Does
     * the same for the center cross.
     */
    void userItemSelected( const QColor &color, /**< [in] the color of this user item */
                           bool sizeVis,        /**< [in] whether or not the size text is visible */
                           bool coordsVis,      /**< [in] whether or not the coordinate text is visible */
                           bool cenVis          /**< [in] whether or not the center cross is visible */
    );

    /// Position the cross marking the center of a user box or circle
    /** Sets the center coordinate and updates the cross position and pen.
     *
     */
    void userItemCross( const QPointF &pos, /**< [in] The pos() of the item */
                        const QRectF &rect, /**< [in] The rect() of the item */
                        const QPen &pen     /**< [in] The pen() of the item */
    );

    /*---- Color Box ----*/

  public:
    StretchBox *m_colorBox{ nullptr }; ///\todo make this protected, fix imcp

  protected:
    QTimer m_colorBoxTimer;                                   ///< Debounce timer for color-box requests.
    int m_colorBoxTimeout{ RTIMV_COLOR_BOX_TIMEOUT_DEFAULT }; ///< Debounce timeout for color-box requests, ms.
    bool m_colorBoxRequestPending{ false };                   ///< True when color-box request should be dispatched.

    bool m_colorBoxCacheValid{ false }; ///< True when cached color-box min/max are valid.
    int64_t m_colorBoxCache_i0{ 0 };    ///< Cached color-box upper-left x coordinate.
    int64_t m_colorBoxCache_i1{ 0 };    ///< Cached color-box lower-right x coordinate.
    int64_t m_colorBoxCache_j0{ 0 };    ///< Cached color-box upper-left y coordinate.
    int64_t m_colorBoxCache_j1{ 0 };    ///< Cached color-box lower-right y coordinate.
    float m_colorBoxCacheMin{ 0 };      ///< Cached color-box minimum value.
    float m_colorBoxCacheMax{ 0 };      ///< Cached color-box maximum value.

    /// Update the on-image color-box min/max text.
    void mtxTry_updateColorBoxText( StretchBox *sb , /**< [in] color box to annotate */
                                    bool hasValues, /**< [in] true when min/max values are available */
                                    float minVal,   /**< [in] minimum value */
                                    float maxVal    /**< [in] maximum value */
    );

  public slots:

    /// Queue the changed color-scaling region without blocking the GUI.
    void mtxTry_colorBoxMoved( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Dispatch the debounced color-box request.
    void dispatchColorBoxRequest();

    /// Handle a color-box update from the backend.
    void colorBoxUpdated( int64_t i0, /**< [in] upper-left x coordinate */
                          int64_t i1, /**< [in] lower-right x coordinate */
                          int64_t j0, /**< [in] upper-left y coordinate */
                          int64_t j1, /**< [in] lower-right y coordinate */
                          float min,  /**< [in] minimum value */
                          float max,  /**< [in] maximum value */
                          bool valid  /**< [in] true when min/max are valid */
    );

    /// Show the selected color box and its coordinate readout.
    void mtxTry_colorBoxSelected( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Hide the color-box selection readouts.
    void colorBoxDeselected( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Called when the color box is deleted
    void colorBoxRemove( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /*---- Stats Box and Display----*/

  protected:
    /// Scene-owned rectangle delimiting the statistics region.
    StretchBox *m_statsBox{ nullptr };

    /// Display modes for the stats-box results.
    enum statsDisplayMode
    {
        statsDisplayOverlay = 0, ///< Render stats as an overlay near the stats box.
        statsDisplayDialog = 1   ///< Render stats in the standalone dialog window.
    };

    rtimvStats *imStats{ nullptr }; ///< Optional dialog displaying stats-box values.

    /// Current presentation mode for stats-box values.
    int m_statsDisplayMode{ statsDisplayOverlay };

    /// True to open the stats box in dialog mode during startup.
    bool m_statsShowDialogOnStartup{ false };

    QTimer m_statsBoxTimer;                                   ///< Debounce timer for stats-box requests.
    int m_statsBoxTimeout{ RTIMV_STATS_BOX_TIMEOUT_DEFAULT }; ///< Debounce timeout for stats-box requests, ms.
    bool m_statsBoxRequestPending{ false };                   ///< True when stats-box request should be dispatched.

    /// Format one stats-box value for display.
    std::string formatStatsValue( float value /**< [in] value to format */ ) const;

    /// Update the stats-box overlay placement and contents.
    void mtxTry_updateStatsBoxOverlay( StretchBox *sb , /**< [in] stats box to annotate */
                                       bool valid,     /**< [in] true when stats values are valid */
                                       float minVal,   /**< [in] minimum value */
                                       float maxVal,   /**< [in] maximum value */
                                       float meanVal,  /**< [in] mean value */
                                       float medianVal /**< [in] median value */
    );

    /// Show or hide the stats-box overlay based on current state.
    void updateStatsBoxOverlayVisibility();

  public slots:

    /// Set the active stats display mode.
    void statsDisplayMode( int mode /**< [in] requested stats display mode */ );

    /// Create or reveal the image statistics box.
    void doLaunchStatsBox();

    /// Hide the image statistics box.
    void doHideStatsBox();

    /// Handle closure of the image statistics window.
    void imStatsClosed( int status /**< [in] Statistics dialog result code. */ );

    /// Queue changed statistics geometry without blocking the GUI.
    void mtxTry_statsBoxMoved( StretchBox * sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Dispatch the debounced stats-box request.
    void dispatchStatsBoxRequest();

    /// Update the stats-box presentation after a backend refresh.
    void statsBoxUpdated( int64_t i0,   /**< [in] upper-left x coordinate */
                          int64_t i1,   /**< [in] lower-right x coordinate */
                          int64_t j0,   /**< [in] upper-left y coordinate */
                          int64_t j1,   /**< [in] lower-right y coordinate */
                          float min,    /**< [in] minimum value */
                          float max,    /**< [in] maximum value */
                          float mean,   /**< [in] mean value */
                          float median, /**< [in] median value */
                          bool valid    /**< [in] true when stats are valid */
    );

    /// Show the selected statistics region and its readouts.
    void mtxTry_statsBoxSelected( StretchBox * sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Remove the statistics box and clear its display state.
    void statsBoxRemove( StretchBox * sb /**< [in] Rectangle or circle whose presentation is updated. */ );

  public:
    /// Get the active stats display mode.
    int statsDisplayMode() const;

    /// Get whether the stats box widget is currently visible.
    bool statsBoxVisible() const;

    /*---- user boxes ----*/

  protected:
    /// Scene-owned user rectangles; removed when image dimensions change.
    std::unordered_set<StretchBox *> m_userBoxes;

  public:
    /// Add a user box
    void addUserBox();

    /// Update the size display for the active user box
    void mtxTry_userBoxSize( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Update the cross marking the center of the active user box
    void userBoxCross( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Update the mouse coordinates at the center of the active user box
    void mtxTry_userBoxMouseCoords( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

  public slots:

    /// Add a StretchBox to user boxes
    /** This adds the StretchBox to the user boxes, and connects
     * its rejectMouse signal to the userBoxRejectMouse slot so that
     * the mouse though management works.  The remove signal is connected to the
     * userBoxRemove slot.  None of the other signals are connected.
     *
     * This can be connected to a signal from a plugin to register a new
     * StretchBox for mouse management.
     *
     */
    void addStretchBox( StretchBox *sb /**< [in] The new StretchBox to add*/ );

    /// Called whenever the user box display shuold be updated
    /** This includes when moved, resized, and when the mouse enters the box.
     */
    void mtxTry_userBoxMoved( StretchBox *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Called when the user clicks through the user box
    void userBoxRejectMouse( StretchBox * sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Called when a user box is deleted
    void userBoxRemove( StretchBox *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Called when a user box is selected
    void mtxTry_userBoxSelected( StretchBox *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Called when a user box is deselected
    void userBoxDeSelected( StretchBox *sc /**< [in] Scene item whose presentation is updated. */ );

    /*---- user circles ----*/

  protected:
    /// Scene-owned user circles; removed when image dimensions change.
    std::unordered_set<StretchCircle *> m_userCircles;

  public:
    /// Add a user circle
    void addUserCircle();

    /// Update the size display for the active user box
    void userCircleSize( StretchCircle *sb /**< [in] Rectangle or circle whose presentation is updated. */ );

    /// Update the cross marking the center of the circle
    void userCircleCross( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Update he mouse coordinates of the user circle
    void mtxTry_userCircleMouseCoords( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

  public slots:
    /// Add a StretchCircle to the user circle
    /** This addes the StretchCircle to the user boxes, and connects
     * its rejectMouse signal to the userCircleRejectMouse slot so that
     * the mouse though management works.  The remove signal is connected to the
     * userCircleRemove slot.  None of the other signals are connected.
     *
     * This can be connected to a signal from a plugin to register a new
     * StretchCircle for mouse management.
     *
     */
    void addStretchCircle( StretchCircle *sc /**< [in] The new StretchCircle to add*/ );

    /// Called whenever the user circle display shuold be updated
    /** This includes when moved, resized, and when the mouse enters the circle.
     */
    void userCircleMoved( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Called when the user clicks through the user circle
    void userCircleRejectMouse( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Called when a user circle is deleted
    void userCircleRemove( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Called when a user circle is selected
    void userCircleSelected( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

    /// Called when a user circle is deselected
    void userCircleDeSelected( StretchCircle *sc /**< [in] Scene item whose presentation is updated. */ );

    /*---- user lines ----*/

  protected:
    /// Scene-owned user lines; removed when image dimensions change.
    std::unordered_set<StretchLine *> m_userLines;

  public:
    /// Add a user line
    void addUserLine();

    /// Update the size display for the active user line
    void mtxTry_userLineSize( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /// Update the circle marking the head of the line
    void userLineHead( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /// Update the mouse coordinates of the user line
    void mtxTry_userLineMouseCoords( StretchLine *sl /**< [in] The StretchLine to update*/ );

  public slots:

    /// Add a StretchLine to the user lines
    /** This adds the StretchLine to the user lines, and connects
     * its rejectMouse signal to the userLineRejectMouse slot so that
     * the mouse though management works.  The remove signal is connected to the
     * userLineRemove slot.  None of the other signals are connected.
     *
     * This can be connected to a signal from a plugin to register a new
     * StretchLine for mouse management.
     *
     */
    void addStretchLine( StretchLine *sl /**< [in] The new StretchLine to add*/ );

    /// Called whenever the user line display shuold be updated
    /** This includes when moved, resized, and when the mouse enters the circle.
     */
    void mtxTry_userLineMoved( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /// Called when the user clicks through the user line
    void userLineRejectMouse( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /// Called when a user line is deleted
    void userLineRemove( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /// Called when a user line is selected
    void mtxTry_userLineSelected( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /// Called when a user line is selected
    void userLineDeSelected( StretchLine *sl /**< [in] The StretchLine to update*/ );

    /** Set the saving indicator
     * Possible stats are:
     * - rtimv::savingState::off     images are not being saved
     * - rtimv::savingState::waiting the next image will be saved
     * - rtimv::savingState::on      images are being saved
     */
    /// Update the camera stream saving-state indicator.
    void savingState( rtimv::savingState ss /**< [in] Camera stream saving state. */ );

  public:
    /// Refresh color-box visibility after changing the color mode.
    virtual void mtxL_postColormode( rtimv::colormode m /**< [in] New display color mode. */ , const sharedLockT &lock /**< [in] Image mutex lock already held by the caller. */ );

  protected:
    /// Whether to display loop-state telemetry.
    bool m_showLoopStat{ false };
    /// Whether to display stream-saving telemetry.
    bool m_showSaveStat{ false };
    /// Whether to display the frame-rate gauge.
    bool m_showFPSGage{ true };

  public slots:

    /// Set the autoScale flag
    /** When m_autoScale is true, the color scale is adjusted for each image.
     *
     */
    void autoScale( bool as /**< [in] the new value of the autoScale flag. */ );

  signals:

    /// Notify that the autoScale flag has changed.
    void autoScaleUpdated( bool enabled /**< [in] the new value of the autoScale flag. */ );

  public:
    /// Change the value of the autoScale flag.
    void toggleAutoScale();

    /// Lock the image and recenter the viewport.
    void mtxUL_center();

    /// Set frame-rate gauge visibility.
    void showFPSGage( bool sfg /**< [in] Whether to show the frame-rate gauge. */ );

    /// Toggle the frame-rate gauge.
    void toggleFPSGage();

    /// Toggle the color-scaling region.
    void toggleColorBox();
    /// Enable the color-scaling region.
    void toggleColorBoxOn();
    /// Disable the color-scaling region.
    void toggleColorBoxOff();

    /// Toggle the statistics region.
    void toggleStatsBox();

    /// Toggle north-arrow visibility.
    void toggleNorthArrow();

    /// Set dark-frame subtraction.
    void setDarkSub( bool ds /**< [in] Whether to subtract the dark frame. */ );

    /// Toggle dark-frame subtraction.
    void toggleDarkSub();

    /// Set image-mask application.
    void setApplyMask( bool am /**< [in] Whether to apply the image mask. */ );

    /// Toggle image-mask application.
    void toggleApplyMask();

    /// Set saturation-mask application.
    void setApplySatMask( bool as /**< [in] Whether to apply the saturation mask. */ );

    /// Toggle saturation-mask application.
    void toggleApplySatMask();

    /// Set modulation-transfer-function display state with user feedback.
    void setApplyMTF( bool amtf /**< [in] true enables MTF display */ );

    /// Toggle modulation-transfer-function display state.
    void toggleApplyMTF();

    /// Toggle the configured image filtering.
    void toggleFilter();

    /// Switch between logarithmic and linear display stretch.
    void toggleLogLinear();

    /// Toggle the target marker.
    void toggleTarget();

    /// Show the transient JPEG quality status text.
    void showQualityMessage( int q /**< [in] JPEG quality value to display */ );

    /** \name Text Overlays - Data
     *
     * Shared state for the full-screen text overlays toggled by shortcut keys.
     *
     * @{
     */
  protected:
    /// Describes one registered text overlay entry.
    struct textOverlayEntry
    {
        /// Short title used in the built-in help listing.
        std::string m_title;

        /// Generates the current overlay text on demand.
        std::function<std::string()> m_textProvider;

        /// Generates one line of overlay text on demand, or empty to request a full refresh.
        std::function<std::string( size_t )> m_lineProvider;

        /// True when this entry is one of the built-in overlays.
        bool m_builtin{ false };

        /// Source label used in log messages.
        std::string m_source;
    };

    /// Registry of overlay shortcuts to text providers.
    std::map<char, textOverlayEntry> m_textOverlays;

    /// Active text overlay shortcut, or `'\0'` when hidden.
    char m_activeTextOverlayKey{ '\0' };

    /// Cached lines for the currently visible shared text overlay.
    std::vector<std::string> m_activeTextOverlayLines;

    /// Split overlay text into line cache entries.
    std::vector<std::string> splitTextOverlayLines( const std::string &text /**< [in] Overlay text to split. */ ) const;

    /// Join cached overlay lines into the text shown by the shared widget.
    std::string joinTextOverlayLines( const std::vector<std::string> &lines /**< [in] Overlay lines to join. */ ) const;

    ///@}

    /** \name Text Overlays
     *
     * Helpers for registering and toggling shared text overlays.
     *
     * @{
     */
  public:
    /// Register a toggleable text overlay shortcut.
    bool registerTextOverlay(
        char key,                                          /**< [in] Shortcut key used to toggle the overlay. */
        const std::string &title,                          /**< [in] Short title shown in the help listing. */
        std::function<std::string()> provider,             /**< [in] Generator for the current overlay text. */
        std::function<std::string( size_t )> lineProvider, /**< [in] Optional generator for one overlay line. */
        const std::string &source                          /**< [in] Source label for collision log messages. */
    );

    /// Check if a shortcut has a registered text overlay.
    bool hasTextOverlay( char key /**< [in] Shortcut key to look up. */ ) const;

  protected:
    /// Hide the shared text overlay widget.
    void hideTextOverlay();

    /// Show the shared text overlay widget for a registered shortcut.
    void showTextOverlay( char key /**< [in] Shortcut key identifying the overlay to show. */ );

    /// Refresh the currently active shared text overlay.
    void refreshActiveTextOverlay();

    /// Refresh one line of the currently active shared text overlay.
    void refreshActiveTextOverlayLine( size_t line /**< [in] Zero-based line index to refresh. */ );

    /// Refresh the shared text overlay widget for a registered shortcut.
    void refreshTextOverlay( char key /**< [in] Shortcut key identifying the overlay to refresh. */ );

    /// Refresh one line of the shared text overlay widget for a registered shortcut.
    void refreshTextOverlayLine( char key,   /**< [in] Shortcut key identifying the overlay to refresh. */
                                 size_t line /**< [in] Zero-based line index to refresh. */
    );

    /// Toggle the shared text overlay widget for a registered shortcut.
    void toggleTextOverlay( char key /**< [in] Shortcut key identifying the overlay to toggle. */ );

    ///@}

  public:
    /// Generate the built-in keyboard help overlay.
    std::string generateHelp();

    /// Toggle the built-in keyboard help overlay.
    void toggleHelp();

    /// Generate the built-in image and plugin info overlay.
    std::string generateInfo();

    /// Toggle the built-in image and plugin info overlay.
    void toggleInfo();

  public slots:
    /// Request a full shared text overlay refresh for a registered shortcut.
    void textOverlayRefreshRequested( char key /**< [in] Shortcut key identifying the overlay to refresh. */ );

    /// Request a single-line shared text overlay refresh for a registered shortcut.
    void textOverlayLineRefreshRequested( char key,  /**< [in] Shortcut key identifying the overlay to refresh. */
                                          int lineNo /**< [in] Zero-based line index to refresh. */
    );

    /** \name Border Colors - Data
     * @{
     */

  protected:
    /// The current warning level for border colors.
    rtimv::warningLevel m_borderWarningLevel{ rtimv::warningLevel::normal };

    StretchBox *m_borderBox{ nullptr }; ///< The border overlay

    ///@}

    /** \name Border Colors
     * @{
     */
  public slots:

    /// Set the warning level for the border color
    /** The levels are defined in the enum rtimv::warningLevel
     *
     */
    void borderWarningLevel( rtimv::warningLevel lvl /**< [in] New border warning level. */ );

    /// Set the border box geometry based on current view and zoom
    /** Called every zoom change and if border is toggled
     *
     */
    void setBorderBox();

    ///@}

    /** \name Font Luminance - Data
     *
     * Tools to change the opacity of the overlay font background color based on the
     * luminance of the background pixels.
     *
     * @{
     */
  protected:
    /// The power for averaging pixel luminance.
    /** The average is formed as pow( sum(pow(pixel-luminance, m_lumPwr), 1/m_lumPwr).  This
     * being a large number causes a few very bright pixels to bias the average.
     */
    double m_lumPwr{ 12 };

    /// The luminance threshold below which no background transparency is set.
    /** Above this, but below m_lumMax, linear interpolation is used to set the opacity.
     *
     * Range: 0-255;
     */
    double m_lumThresh{ 100 };

    /// The maximum luminance, at which point the opacity is set to m_opacityMax.
    /** Below this, but above m_lumThresh, linear interpolation is used to set the opacity.
     *
     * Range: 0-255;
     */
    double m_lumMax{ 175 };

    /// The maximum opacity which will be set.
    /** This is the opacity set when at or above m_lumMax.
     * Between m_lumThresh and m_lumMax, the opacity is interpolated between 0 and this value.
     *
     * Range: 0-255;
     */
    double m_opacityMax{ 150 };

    ///@}

    /** \name Font Luminance
     * @{
     */
  public:
    /// Sets the background transparency of a QTextEdit based on the average luminance of the background pixels.
    /** Uses a power-quadrature average (see m_lumPwr) and sets the background opacity based on interpolation with
     *  the values of m_lumThresh, m_lumMax, and m_opacityMax.
     *
     * \note Call this version with the mutex already locked
     */
    void mtxL_fontLuminance( QTextEdit *qte , /**< [in/out] the QTextEdit to modify */
        const sharedLockT &lock,
        bool print = false /**< [in] [out] if true the average luminance is printed.  Used for debugging. */
    );

    /// Sets the background transparency of a QTextEdit based on the average luminance of the background pixels.
    /** Uses a power-quadrature average (see m_lumPwr) and sets the background opacity based on interpolation with
     * the values of m_lumThresh, m_lumMax, and m_opacityMax.
     *
     * \note Call this version with the mutex unlocked
     *
     */
    void mtxTry_fontLuminance( QTextEdit *qte , /**< [in/out] the QTextEdit to modify */
        bool print = false /**< [in] [out] if true the average luminance is printed.  Used for debugging. */
    );

    /// Update the background opacity for all the text fields.  Called periodically.
    /**
     * \note Call this version with the mutex unlocked
     *
     */
    void mtxTry_fontLuminance();

    ///@}

    /** \name Data Dictionary
     *
     * A map of key=binary-blob pairs to be used and managed by plugins.
     *
     * @{
     */

    /// The data dictionary.
    dictionaryT m_dictionary;

    ///@}

    /** \name Plugins
     *
     *@{
     */
  protected:
    /// Plugin paths discovered during startup.
    QStringList m_pluginFileNames;

    /// Attach supported plugin interfaces and register any optional command provider.
    int loadPlugin( QObject *plugin /**< [in] Plugin QObject to inspect and attach. */ );

    /// Attached plugin interfaces whose lifetime is managed by Qt plugin loading.
    std::vector<rtimvInterface *> m_plugins;

    /// Attached overlay interfaces used for periodic updates and keyboard dispatch.
    std::vector<rtimvOverlayInterface *> m_overlays;

    ///@}

  private:
    /// Qt-generated controls owned by the main window.
    Ui::rtimvMainWindow ui;

  public:
    /// Filter events for various members
    /** The following events are filtered:
     *  - QGraphicsScene * m_qgs KeyPress is stopped if it's an arrow or page key.  This is instead passed directly to
     * keyPressEvent.
     *
     * This filter is registered with objects upon their construction.
     * :
     * \returns true if processing of the event should stop
     * \returns false otherwise
     *
     */
    bool eventFilter( QObject *obj, /**< [in] the object processing the event */
                      QEvent *event /**< [in] the event */
    );
};

#endif // rtimvMainWindow_hpp_inc
