
#ifndef __imviewercontrolpanel_h__
#define __imviewercontrolpanel_h__


#include "ui_imviewerControlPanel.h"
#include "rtimvMainWindow.hpp"
#include "StretchBox.hpp"

class rtimvMainWindow;

class rtimvControlPanel : public QWidget
{
   Q_OBJECT
   
   public:
      rtimvControlPanel( rtimvMainWindow *imv, 
                         Qt::WindowFlags f = Qt::WindowFlags()
                       );
   
   protected:
      rtimvMainWindow * imv;
      
      void setupMode();
      void setupCombos();
      
      void init_panel();
      
   public:
      void update_panel();
   
      /*** Zoom Controls ***/
   protected:
      bool IgnoreZoomSliderChange;
   public:
      void update_ZoomSlider();
      void update_ZoomEntry();
   
   protected slots:
      void on_ZoomSlider_valueChanged(int value);
      void on_Zoom1_clicked();
      void on_Zoom2_clicked();
      void on_Zoom4_clicked();
      void on_Zoom8_clicked();
      void on_Zoom16_clicked();
      void on_ZoomEntry_editingFinished();
      
      void on_overZoom1_clicked();
      void on_overZoom2_clicked();
      void on_overZoom4_clicked();
      
      
   public:
      /*** The zoom box view ***/
      QGraphicsScene * qgs_view;
      QGraphicsScene * qgs_empty;
      QGraphicsPixmapItem *qpmi_view;// = 0;
      QGraphicsLineItem * viewLineVert;
      QGraphicsLineItem * viewLineHorz;
      //QGraphicsRectItem * viewBox;
      StretchBox * viewBox;
      
      int ViewViewMode; //options: ViewViewEnabled ViewViewNoImage
      void set_ViewViewMode(int);
      
      void update_xycenEntry();
      void update_whEntry();
      
   protected slots:
      void on_ViewViewModecheckBox_stateChanged(int);
      void enableViewViewMode(int);
      
      void on_xcenEntry_editingFinished();
      void on_ycenEntry_editingFinished();
      
      void on_widthEntry_editingFinished();
      void on_heightEntry_editingFinished();
      
      void on_view_center_clicked();
      void on_view_ul_clicked();
      void on_view_up_clicked();
      void on_view_ur_clicked();
      void on_view_right_clicked();
      void on_view_dr_clicked();
      void on_view_down_clicked();
      void on_view_dl_clicked();
      void on_view_left_clicked();

   public:
      /*** The pointer tip view ***/
      QPointF pointerViewCen;
      int PointerViewMode; //options: PointerViewEnabled PointerViewOnPress PointerViewDisabled
      
   protected:
      bool PointerViewEmpty;
      bool PointerViewFixed;
      bool PointerViewWaiting;
      
   public:
      void updateMouseCoords(double x, double y, double v);
      void nullMouseCoords();
      void viewLeftPressed(QPointF mp);
      void viewLeftClicked(QPointF mp);
      
   public slots:
      void set_PointerViewMode(int);
      void on_pointerViewModecomboBox_activated(int);
      void on_pointerSetLocButton_clicked();
      void set_pointerViewCen(QPointF mp);
      
      
      void viewBoxMoved ( const QRectF & newcen);
      
   /*** Color Controls ***/
   protected slots:
      void on_scaleTypeCombo_activated(int);
      void on_colorbarCombo_activated(int);
      
   /* scale controls */
   protected:
      bool IgnoremindatSliderChange;
      void update_mindatSlider();
      void update_mindatEntry();
      void update_mindatRelEntry();
      
      bool IgnoremaxdatSliderChange;
      void update_maxdatSlider();
      void update_maxdatEntry();
      void update_maxdatRelEntry();
      
      bool IgnorebiasSliderChange;
      void update_biasSlider();
      void update_biasEntry();
      void update_biasRelEntry();
      
      bool IgnorecontrastSliderChange;
      void update_contrastSlider();
      void update_contrastEntry();
      void update_contrastRelEntry();

   public slots:
      void on_scaleModeCombo_activated(int index);
      
      void on_mindatSlider_valueChanged(int value);
      void on_mindatEntry_editingFinished();
      
      void on_maxdatSlider_valueChanged(int value);
      void on_maxdatEntry_editingFinished();
      
      void on_biasSlider_valueChanged(int value);
      void on_biasEntry_editingFinished();
      
      void on_contrastSlider_valueChanged(int value);
      void on_contrastEntry_editingFinished();
      

   public:
   /*** Real Time Controls ***/
   bool statsBoxButtonState;
   
   public slots:
      void on_imtimerspinBox_valueChanged(int);
      void on_statsBoxButton_clicked();

   signals:
      void launchStatsBox();
      void hideStatsBox();
      
   public:
      Ui::imviewerControlPanel ui;
      
   /*--------- Mouse coordinates display ---------------*/
public slots:
   /// Receive signal that the tool tip coords display flag has changed.
   /** Updates the button text.
     */
   void showToolTipCoordsChanged(bool sttc /**< [in] the new value of the flag*/ );

   /// Receive signal that the static coords display flag has changed.
   /** Updates the button text.
     */
   void showStaticCoordsChanged(bool /**< [in] the new value of the flag*/);

   /// Toggle the tool tip coords display flag 
   void on_toolTipCoordsButton_clicked();

   /// Toggle the static coords display flag 
   void on_staticCoordsButton_clicked();

signals:
   /// Request that the tool tip coords display flag change.
   void showToolTipCoords(bool /**< [in] the new value of the flag*/);

   /// Request that the static coords display flag change.
   void showStaticCoords(bool /**< [in] the new value of the flag*/);   


   /*--------- Target Cross ---------------*/
public slots:
   /// Notify that the x-coordinate of the target has changed
   /** This updates both the fraction and pixel values.
     */ 
   void targetXcChanged( float txc /**< [in] the new target x coordinate*/);

   /// Notify that the y-coordinate of the target has changed
   /** This updates both the fraction and pixel values.
     */
   void targetYcChanged( float tyc /**< [in] the new target y coordinate*/);

   /// Notify that the target visibility has changed.
   /** This updates the push button text.
     */
   void targetVisibleChanged( bool tv );
       

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
   void targetXc( float txc /**< [in] the new target x coordinate*/);

   /// Request to set the target cross y coordinate by fraction.
   void targetYc( float tyc /**< [in] the new target y coordinate*/);

   /// Request to set the target cross visibility
   void targetVisible( bool tv );
};


#endif //__imviewercontrolpanel_h__
