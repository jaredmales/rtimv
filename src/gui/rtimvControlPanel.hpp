/** \file rtimvControlPanel.hpp
 * \brief Declarations for the rtimvControlPanel GUI class.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimv_rtimvControlPanel_hpp
#define rtimv_rtimvControlPanel_hpp

#include <QWidget>

#include "rtimvMainWindow.hpp"
#include "StretchBox.hpp"
#include "ui_rtimvControlPanel.h"

class rtimvMainWindow;

class rtimvControlPanel : public QWidget
{
    Q_OBJECT

  private:
    /// Private default constructor is disabled for normal use.
    rtimvControlPanel();

  public:
    /// Construct the control panel and bind it to the main window model.
    rtimvControlPanel( rtimvMainWindow *imv /**< [in] the main window/model*/,
                       Qt::WindowFlags f = Qt::WindowFlags() /**< [in] Qt window flags*/ );

  protected:
    /// Pointer to the main window/model this control panel drives.
    rtimvMainWindow *m_imv;

    /// Initialize internal default mode state.
    void setupMode();

    /// Populate combo boxes with their available options.
    void setupCombos();

    /// Initialize all UI fields from current model state.
    void init_panel();

  public:
    /// Refresh all UI controls from the current model values.
    void update_panel();

    /*** Zoom Controls ***/
  public:
    /// Synchronize zoom slider position from model zoom.
    void update_ZoomSlider();

    /// Synchronize zoom entry text from model zoom.
    void update_ZoomEntry();

  protected slots:
    /// Handle zoom slider changes and apply zoom to the model.
    void on_ZoomSlider_valueChanged( int value /**< [in] new slider value*/ );

    /// Set zoom level to 1x.
    void on_Zoom1_clicked();

    /// Set zoom level to 2x.
    void on_Zoom2_clicked();

    /// Set zoom level to 4x.
    void on_Zoom4_clicked();

    /// Set zoom level to 8x.
    void on_Zoom8_clicked();

    /// Set zoom level to 16x.
    void on_Zoom16_clicked();

    /// Apply zoom value entered in the zoom text field.
    void on_ZoomEntry_editingFinished();

    /// Set pointer over-zoom factor to 1x.
    void on_overZoom1_clicked();

    /// Set pointer over-zoom factor to 2x.
    void on_overZoom2_clicked();

    /// Set pointer over-zoom factor to 4x.
    void on_overZoom4_clicked();

  public:
    /*** The zoom box view ***/
    /// Scene containing the overview image and selection graphics.
    QGraphicsScene *m_qgsView;

    /// Empty scene shown when overview/pointer views are disabled.
    QGraphicsScene *m_qgsEmpty;

    /// Pixmap item for the overview image.
    QGraphicsPixmapItem *m_qpmiView; // = 0;

    /// Vertical center guideline in overview mode.
    QGraphicsLineItem *m_viewLineVert;

    /// Horizontal center guideline in overview mode.
    QGraphicsLineItem *m_viewLineHorz;

    /// Movable viewport rectangle in the overview scene.
    StretchBox *m_viewBox;

    /// Current overview mode (ViewViewEnabled/ViewViewNoImage).
    int m_viewViewMode; // options: ViewViewEnabled ViewViewNoImage

    /// Set the view-overview display mode.
    void set_ViewViewMode( int vvm /**< [in] requested view-overview mode*/ );

    /// Update overview center coordinate entries.
    void update_xycenEntry();

    /// Update overview width/height entries.
    void update_whEntry();

  protected slots:
    /// Handle checkbox state changes for overview mode.
    void on_ViewViewModecheckBox_stateChanged( int state /**< [in] new checkbox state*/ );

    /// Enable or disable overview mode from UI state.
    void enableViewViewMode( int state /**< [in] non-zero enables overview mode*/ );

    /// Apply edited x-center value.
    void on_xcenEntry_editingFinished();

    /// Apply edited y-center value.
    void on_ycenEntry_editingFinished();

    /// Apply edited viewport width.
    void on_widthEntry_editingFinished();

    /// Apply edited viewport height.
    void on_heightEntry_editingFinished();

    /// Center viewport on full-frame center.
    void on_view_center_clicked();

    /// Move viewport up-left.
    void on_view_ul_clicked();

    /// Move viewport up.
    void on_view_up_clicked();

    /// Move viewport up-right.
    void on_view_ur_clicked();

    /// Move viewport right.
    void on_view_right_clicked();

    /// Move viewport down-right.
    void on_view_dr_clicked();

    /// Move viewport down.
    void on_view_down_clicked();

    /// Move viewport down-left.
    void on_view_dl_clicked();

    /// Move viewport left.
    void on_view_left_clicked();

  public:
    /*** The pointer tip view ***/
    /// Fixed pointer-view center in scene coordinates.
    QPointF m_pointerViewCen;

    /// Current pointer-view mode state.
    int m_pointerViewMode; // options: PointerViewEnabled PointerViewOnPress PointerViewDisabled

  protected:
    /// True when pointer view is displaying the empty scene.
    bool m_pointerViewEmpty;

    /// True when pointer view location is user-fixed.
    bool m_pointerViewFixed;

    /// True while waiting for a click to set fixed pointer location.
    bool m_pointerViewWaiting;

  public:
    /// Update mouse coordinate and value displays.
    void updateMouseCoords( double x /**< [in] x coordinate in image pixels*/,
                            double y /**< [in] y coordinate in image pixels*/,
                            double v /**< [in] pixel value at x,y*/ );

    /// Clear mouse coordinate and value displays.
    void nullMouseCoords();

    /// Handle left-button press in the main view.
    void viewLeftPressed( QPointF mp /**< [in] mouse position in scene coordinates*/ );

    /// Handle left-button click/release in the main view.
    void viewLeftClicked( QPointF mp /**< [in] mouse position in scene coordinates*/ );

  public slots:
    /// Set pointer-view mode policy.
    void set_PointerViewMode( int pvm /**< [in] pointer view mode*/ );

    /// Handle pointer-view mode combo selection.
    void on_pointerViewModecomboBox_activated( int pvm /**< [in] selected pointer view mode*/ );

    /// Toggle pointer location locking workflow.
    void on_pointerSetLocButton_clicked();

    /// Set the pointer-view center location.
    void set_pointerViewCen( QPointF mp /**< [in] new pointer-view center in scene coordinates*/ );

    /// Update model center after overview box movement.
    void viewBoxMoved( const QRectF &newcen /**< [in] updated overview selection rectangle*/ );

  protected slots:
    /// Handle stretch type combo selection.
    void on_scaleTypeCombo_activated( int ct /**< [in] selected stretch type index*/ );

    /// Handle colorbar combo selection.
    void on_colorbarCombo_activated( int cb /**< [in] selected colorbar index*/ );

    /* scale controls */
  protected:
    /// Synchronize min-data slider from model state.
    void update_mindatSlider();

    /// Synchronize min-data absolute entry from model state.
    void update_mindatEntry();

    /// Synchronize min-data relative entry from model state.
    void update_mindatRelEntry();

    /// Synchronize max-data slider from model state.
    void update_maxdatSlider();

    /// Synchronize max-data absolute entry from model state.
    void update_maxdatEntry();

    /// Synchronize max-data relative entry from model state.
    void update_maxdatRelEntry();

    /// Synchronize bias slider from model state.
    void update_biasSlider();

    /// Synchronize bias absolute entry from model state.
    void update_biasEntry();

    /// Synchronize bias relative entry from model state.
    void update_biasRelEntry();

    /// Synchronize contrast slider from model state.
    void update_contrastSlider();

    /// Synchronize contrast absolute entry from model state.
    void update_contrastEntry();

    /// Synchronize contrast relative entry from model state.
    void update_contrastRelEntry();

    /// Synchronize JPEG quality slider from model state.
    void update_qualitySlider();

    /// Synchronize JPEG quality entry from model state.
    void update_qualityEntry();

    /// Synchronize high-pass filter controls from model state.
    void update_hpFilter();

    /// Synchronize low-pass filter controls from model state.
    void update_lpFilter();

  public slots:
    /// Handle color mode combo selection.
    void on_scaleModeCombo_activated( int index /**< [in] selected color mode index*/ );

    /// Handle minimum data slider changes.
    void on_mindatSlider_valueChanged( int value /**< [in] new minimum slider value*/ );

    /// Handle minimum data entry edits.
    void on_mindatEntry_editingFinished();

    /// Handle maximum data slider changes.
    void on_maxdatSlider_valueChanged( int value /**< [in] new maximum slider value*/ );

    /// Handle maximum data entry edits.
    void on_maxdatEntry_editingFinished();

    /// Handle bias slider changes.
    void on_biasSlider_valueChanged( int value /**< [in] new bias slider value*/ );

    /// Handle bias entry edits.
    void on_biasEntry_editingFinished();

    /// Handle contrast slider changes.
    void on_contrastSlider_valueChanged( int value /**< [in] new contrast slider value*/ );

    /// Handle contrast entry edits.
    void on_contrastEntry_editingFinished();

    /// Handle high-pass filter type changes.
    void on_hpFilterCombo_activated( int index /**< [in] selected high-pass filter type*/ );

    /// Handle high-pass filter enable state changes.
    void on_hpApplyCheck_stateChanged( int state /**< [in] checked state*/ );

    /// Handle high-pass width slider changes.
    void on_hpFWSlider_valueChanged( int value /**< [in] slider position*/ );

    /// Handle high-pass width entry edits.
    void on_hpFWEntry_editingFinished();

    /// Handle low-pass filter type changes.
    void on_lpFilterCombo_activated( int index /**< [in] selected low-pass filter type*/ );

    /// Handle low-pass filter enable state changes.
    void on_lpApplyCheck_stateChanged( int state /**< [in] checked state*/ );

    /// Handle low-pass width slider changes.
    void on_lpFWSlider_valueChanged( int value /**< [in] slider position*/ );

    /// Handle low-pass width entry edits.
    void on_lpFWEntry_editingFinished();

  public:
    /*** Real Time Controls ***/
    /// Tracks whether the stats box is currently shown.
    bool m_statsBoxButtonState;

  public slots:
    /// Handle image timer update interval changes.
    void on_imtimerspinBox_valueChanged( int timeout /**< [in] new image timeout value*/ );

    /// Handle JPEG quality slider changes.
    void on_qualitySlider_valueChanged( int value /**< [in] new quality value*/ );

    /// Handle JPEG quality entry edits.
    void on_qualityEntry_editingFinished();

    /// Toggle stats box visibility.
    void on_statsBoxButton_clicked();

  signals:
    /// Request stats box creation/display.
    void launchStatsBox();

    /// Request stats box hide/close.
    void hideStatsBox();

  public:
    /// Generated Qt UI object for this panel.
    Ui::rtimvControlPanel m_ui;

    /*--------- Mouse coordinates display ---------------*/
  public slots:
    /// Receive signal that the tool tip coords display flag has changed.
    /** Updates the button text.
     */
    void showToolTipCoordsChanged( bool sttc /**< [in] the new value of the flag*/ );

    /// Receive signal that the static coords display flag has changed.
    /** Updates the button text.
     */
    void showStaticCoordsChanged( bool ssc /**< [in] the new value of the flag*/ );

    /// Toggle the tool tip coords display flag
    void on_toolTipCoordsButton_clicked();

    /// Toggle the static coords display flag
    void on_staticCoordsButton_clicked();

  signals:
    /// Request that the tool tip coords display flag change.
    void showToolTipCoords( bool sttc /**< [in] the new value of the flag*/ );

    /// Request that the static coords display flag change.
    void showStaticCoords( bool ssc /**< [in] the new value of the flag*/ );

    /*--------- Target Cross ---------------*/
  public slots:
    /// Notify that the x-coordinate of the target has changed
    /** This updates both the fraction and pixel values.
     */
    void targetXcChanged( float txc /**< [in] the new target x coordinate*/ );

    /// Notify that the y-coordinate of the target has changed
    /** This updates both the fraction and pixel values.
     */
    void targetYcChanged( float tyc /**< [in] the new target y coordinate*/ );

    /// Notify that the target visibility has changed.
    /** This updates the push button text.
     */
    void targetVisibleChanged( bool tv /**< [in] the new target visibility state*/ );

    /// Toggle the target cross visibility when button pressed.
    void on_buttonTargetCross_clicked();

    /// Set the target cross x coordinate by pixel value when enter is pressed.
    void on_lineEditTargetPixelX_returnPressed();

    /// Set the target cross y coordinate by pixel value when enter is pressed.
    void on_lineEditTargetPixelY_returnPressed();

    /// Set the target cross x coordinate by fraction when enter is pressed.
    void on_lineEditTargetFractionX_returnPressed();

    /// Set the target cross y coordinate by fraction when enter is pressed.
    void on_lineEditTargetFractionY_returnPressed();

  signals:

    /// Request to set the target cross x coordinate by fraction.
    void targetXc( float txc /**< [in] the new target x coordinate*/ );

    /// Request to set the target cross y coordinate by fraction.
    void targetYc( float tyc /**< [in] the new target y coordinate*/ );

    /// Request to set the target cross visibility
    void targetVisible( bool tv /**< [in] the new target visibility state*/ );
};

#endif // rtimv_rtimvControlPanel_hpp
