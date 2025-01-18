
#ifndef rtimvMainWindow_hpp
#define rtimvMainWindow_hpp

#include <unordered_set>

#include <QWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QPluginLoader>
#include <QDir>
#include <QMainWindow>

#include <mx/app/application.hpp>

using namespace mx::app;

#include "ui_rtimvMainWindow.h"
#include "rtimvBase.hpp"
#include "rtimvControlPanel.hpp"
#include "cubeCtrl.hpp"

#include "rtimvInterfaces.hpp"

#include "StretchBox.hpp"
#include "StretchCircle.hpp"
#include "StretchLine.hpp"

#include "rtimvStats.hpp"

#include <cstdio>

#define SCALEMODE_USER 3

#define PointerViewEnabled 0
#define PointerViewOnPress 1
#define PointerViewDisabled 2
#define PointerViewModeMax 3

#define ViewViewEnabled 0
#define ViewViewNoImage 1
#define ViewViewModeMax 2

class rtimvControlPanel;

class rtimvMainWindow : public rtimvBase, public application
{
    Q_OBJECT

  public:
    rtimvMainWindow( int argc, char **argv, QWidget *Parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );

    ~rtimvMainWindow();

    virtual void setupConfig();

    virtual void loadConfig();

  protected:
    std::string m_title{ "rtimv" };

  public:
    /// Called on initial connection to the image stream, sets matching aspect ratio.
    virtual void onConnect();

    virtual void mtxL_postSetImsize( const uniqueLockT &lock );

    virtual void post_zoomLevel();

    virtual void mtxL_postRecolor( const uniqueLockT &lock );

    virtual void mtxL_postRecolor( const sharedLockT &lock );

    virtual void mtxL_postChangeImdata( const sharedLockT &lock );

    virtual void updateFPS();

    virtual void updateAge();

    /// Update the display while not connected.
    virtual void updateNC();

    virtual void keyPressEvent( QKeyEvent * );

    /*** The control Panel ***/
  protected:
    rtimvControlPanel *imcp;

    float pointerOverZoom;

  public:
    void launchControlPanel();

    float get_act_xcen();
    float get_act_ycen();

    void setPointerOverZoom( float poz );

    /** \name Cube Control
     * @{
     */
protected:
    cubeCtrl * m_cubeCtrl {nullptr};

    public:
    void launchCubeCtrl();

    void toggleCubeCtrl();

    ///@}

    /*** Graphics stuff ***/
  protected:
    QGraphicsScene *m_qgs{ nullptr };
    QGraphicsPixmapItem *m_qpmi{ nullptr };

    float m_screenZoom;

  public:
    QGraphicsScene *get_qgs();

    /** \name North Arrow
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

    void centerNorthArrow(); ///< Center the north arrow on the current display center.

    void updateNorthArrow(); ///< Update the angle of the north arrow.

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
    void freezeRealTime();

    void reStretch();

    /*** The Main View ***/
  public:
    void change_center( bool movezoombox = true );

  private:
    void mtxL_setViewCen_impl( float x, float y, bool movezoombox );

  public:
    void mtxL_setViewCen( float x, float y, const uniqueLockT &lock, bool movezoombox = true );

    void mtxL_setViewCen( float x, float y, const sharedLockT &lock, bool movezoombox = true );

    float get_xcen()
    {
        return ui.graphicsView->xCen();
    }

    float get_ycen()
    {
        return ui.graphicsView->yCen();
    }

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
    void changeCenter()
    {
        change_center();
    }

    /// Calls changeCenter() so that the display stays centered when user resizes.
    void resizeEvent( QResizeEvent * );

    /*** pointer data***/
  protected:
    bool rightClickDragging;
    QPointF rightClickStart;
    float biasStart;
    float contrastStart;

    bool m_showToolTipCoords{ true }; ///< Flag indicating whether the mouse tool tip should be shown
    bool m_showStaticCoords{ false }; ///< Flag indicating whether the mouse tool tip should be shown

    bool m_nullMouseCoords{ true }; ///< Flag indicating whether or not the mouse coordinates have been nulled.

    void mouseMoveEvent( QMouseEvent *e );

    void nullMouseCoords();

    /// Update the GUI for a change in mouse coordinates in the viewport
    /** Performs the following functions:
     * - Decides whether or not the mouse is currently in the pixmap.
     * - If it is not in the pixmap, clears the mouse coord text boxes
     * - If it is, updates the mouse coord text boxes
     * - If the mouse is right-clicked and dragging, updates the strech bias and contrast.
     */
    void mtxL_updateMouseCoords( const sharedLockT &lock );

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

    void viewLeftPressed( QPointF mp );
    void viewLeftClicked( QPointF mp );

    void viewRightPressed( QPointF mp );
    void viewRightClicked( QPointF mp );

    void onWheelMoved( int delta );

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

    void targetXcChanged( float txc /**< [in] the new target x coordinate*/ );

    void targetYcChanged( float tyc /**< [in] the new target y coordinate*/ );

    void targetVisibleChanged( bool tv /**< [in] the new status of target visibility */ );

  public:
    /// Position the target lines
    /** This is normally called by the targetXc(float) and targetYc(float) slots
     */
    void setTarget();

    /** \name User Shapes
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

  public:
    /// Update the mouse coordinates at the center of the active user item
    void mtxTry_userItemMouseCoords( float mx, ///< [in] the scene x-coordinate of the center
                                     float my, ///< [in] the scene y-coordinate of the center
                                     float dx, ///< [in] the viewport x-coordinate of the center
                                     float dy  ///< [in] the viewport y-coordinate of the center
    );

    /// Initialize a user item upon being selected
    /** Clears the size and coordinate text fields and sets their color and visibility.  Does
     * the same for the center cross.
     */
    void userItemSelected( const QColor &color, ///< [in] the color of this user item
                           bool sizeVis,        ///< [in] whether or not the size text is visible
                           bool coordsVis,      ///< [in] whether or not the coordinate text is visible
                           bool cenVis          ///< [in] whether or not the center cross is visible
    );

    /// Position the cross marking the center of a user box or circle
    /** Sets the center coordinate and updates the cross position and pen.
     *
     */
    void userItemCross( const QPointF &pos, ///< The pos() of the item
                        const QRectF &rect, ///< The rect() of the item
                        const QPen &pen     ///< The pen() of the item
    );

    /*---- Color Box ----*/

  public:
    StretchBox *m_colorBox{ nullptr }; ///\todo make this protected, fix imcp

  public slots:

    void mtxTry_colorBoxMoved( StretchBox *sb );

    void mtxTry_colorBoxSelected( StretchBox *sb );

    void colorBoxDeselected( StretchBox *sb );

    /// Called when the color box is deleted
    void colorBoxRemove( StretchBox *sb );

    /*---- Stats Box and Display----*/

  protected:
    StretchBox *m_statsBox{ nullptr };

    rtimvStats *imStats;

  public slots:

    void doLaunchStatsBox();

    void doHideStatsBox();

    void imStatsClosed( int );

    void mtxTry_statsBoxMoved( StretchBox * );

    void mtxTry_statsBoxSelected( StretchBox * );

    void statsBoxRemove( StretchBox * );

    /*---- user boxes ----*/

  protected:
    std::unordered_set<StretchBox *> m_userBoxes;

  public:
    /// Add a user box
    void addUserBox();

    /// Update the size display for the active user box
    void mtxTry_userBoxSize( StretchBox *sb );

    /// Update the cross marking the center of the active user box
    void userBoxCross( StretchBox *sb );

    /// Update the mouse coordinates at the center of the active user box
    void mtxTry_userBoxMouseCoords( StretchBox *sb );

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
    void mtxTry_userBoxMoved( StretchBox *sb );

    /// Called when the user clicks through the user box
    void userBoxRejectMouse( StretchBox * );

    /// Called when a user box is deleted
    void userBoxRemove( StretchBox *sc );

    /// Called when a user box is selected
    void mtxTry_userBoxSelected( StretchBox *sc );

    /// Called when a user box is deselected
    void userBoxDeSelected( StretchBox *sc );

    /*---- user circles ----*/

  protected:
    std::unordered_set<StretchCircle *> m_userCircles;

  public:
    /// Add a user circle
    void addUserCircle();

    /// Update the size display for the active user box
    void userCircleSize( StretchCircle *sb );

    /// Update the cross marking the center of the circle
    void userCircleCross( StretchCircle *sc );

    /// Update he mouse coordinates of the user circle
    void mtxTry_userCircleMouseCoords( StretchCircle *sc );

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
    void userCircleMoved( StretchCircle *sc );

    /// Called when the user clicks through the user circle
    void userCircleRejectMouse( StretchCircle *sc );

    /// Called when a user circle is deleted
    void userCircleRemove( StretchCircle *sc );

    /// Called when a user circle is selected
    void userCircleSelected( StretchCircle *sc );

    /// Called when a user circle is deselected
    void userCircleDeSelected( StretchCircle *sc );

    /*---- user lines ----*/

  protected:
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
    void savingState( rtimv::savingState ss );

  public:
    virtual void mtxL_postSetColorBoxActive( bool usba, const sharedLockT &lock );

  protected:
    bool m_showLoopStat{ false };
    bool m_showSaveStat{ false };
    bool m_showFPSGage{ true };

  public slots:

    /// Set the autoScale flag
    /** When m_autoScale is true, the color scale is adjusted for each image.
     *
     */
    void autoScale( bool as /**< [in] the new value of the autoScale flag. */);

  signals:

    /// Notify that the autoScale flag has changed.
    void autoScaleUpdated(bool /**< [in] the new value of the autoScale flag. */);

  public:

    ///Change the value of the autoScale flag.
    void toggleAutoScale();

    void mtxUL_center();

    void showFPSGage( bool sfg );

    void toggleFPSGage();

    void toggleColorBox();
    void toggleColorBoxOn();
    void toggleColorBoxOff();

    void toggleStatsBox();

    void toggleNorthArrow();

    void setDarkSub( bool ds );

    void toggleDarkSub();

    void setApplyMask( bool am );

    void toggleApplyMask();

    void setApplySatMask( bool as );

    void toggleApplySatMask();

    void toggleLogLinear();

    void toggleTarget();

    bool m_helpVisible{ false };
    std::string generateHelp();

    void toggleHelp();

    bool m_infoVisible{ false };

    std::string generateInfo();

    void toggleInfo();

    /** \name Border Colors
     * @{
     */

  protected:
    /// The current warning level for border colors.
    rtimv::warningLevel m_borderWarningLevel{ rtimv::warningLevel::normal };

    StretchBox *m_borderBox{ nullptr }; ///< The border overlay

  public slots:

    /// Set the warning level for the border color
    /** The levels are defined in the enum rtimv::warningLevel
     *
     */
    void borderWarningLevel( rtimv::warningLevel lvl );

    /// Set the border box geometry based on current view and zoom
    /** Called every zoom change and if border is toggled
     *
     */
    void setBorderBox();

    ///@}

    /** \name Font Luminance
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

  public:
    /// Sets the background transparency of a QTextEdit based on the average luminance of the background pixels.
    /** Uses a power-quadrature average (see m_lumPwr) and sets the background opacity based on interpolation with
     *  the values of m_lumThresh, m_lumMax, and m_opacityMax.
     *
     * \note Call this version with the mutex already locked
     */
    void mtxL_fontLuminance(
        QTextEdit *qte, ///< [in/out] the QTextEdit to modify
        const sharedLockT &lock,
        bool print = false ///< [in] [out] if true the average luminance is printed.  Used for debugging.
    );

    /// Sets the background transparency of a QTextEdit based on the average luminance of the background pixels.
    /** Uses a power-quadrature average (see m_lumPwr) and sets the background opacity based on interpolation with
     * the values of m_lumThresh, m_lumMax, and m_opacityMax.
     *
     * \note Call this version with the mutex unlocked
     *
     */
    void mtxTry_fontLuminance(
        QTextEdit *qte,    ///< [in/out] the QTextEdit to modify
        bool print = false ///< [in] [out] if true the average luminance is printed.  Used for debugging.
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
    QStringList m_pluginFileNames;

    int loadPlugin( QObject *plugin );

    std::vector<rtimvInterface *> m_plugins;

    std::vector<rtimvOverlayInterface *> m_overlays;

    ///@}

  private:
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
    bool eventFilter( QObject *obj, ///< [in] the object processing the event
                      QEvent *event ///< [in] the event
    );



};

#endif // rtimvMainWindow_hpp
