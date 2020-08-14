
#ifndef rtimvMainWindow_hpp
#define rtimvMainWindow_hpp

#include <QWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QTimer>
#include <QGraphicsPixmapItem>
#include <QPluginLoader>
#include <QDir>

#include <mx/app/application.hpp>

using namespace mx::app;


#include "ui_rtimvMainWindow.h"
#include "imviewer.hpp"
#include "imviewerControlPanel.h"

#include "rtimvInterfaces.hpp"

#include "StretchBox.hpp"
#include "StretchCircle.hpp"

#include "imviewerstats.hpp"


#include <cstdio>

#define SCALEMODE_USER 3
	
#define PointerViewEnabled 0
#define PointerViewOnPress 1
#define PointerViewDisabled 2
#define PointerViewModeMax 3

#define ViewViewEnabled 0
#define ViewViewNoImage 1
#define ViewViewModeMax 2

class imviewerControlPanel;

class rtimvMainWindow : public imviewer, public application
{
   Q_OBJECT
   
   public:
      rtimvMainWindow( int argc,
                       char ** argv,
                       QWidget * Parent = 0, 
                       Qt::WindowFlags f = 0
                     );
      
      virtual void setupConfig();
      
      virtual void loadConfig();
      
      ///Called on initial connection to the image stream, sets matching aspect ratio.
      virtual void onConnect();
      
      virtual void postSetImsize();

      /// Setup the target lines
      void setTarget();
      
      virtual void post_zoomLevel();
      virtual void postChangeImdata();
      
      virtual void updateFPS();
      
      virtual void updateAge();

      virtual void keyPressEvent(QKeyEvent *);
      
      /*** The control Panel ***/
   protected:
      imviewerControlPanel *imcp;
      
      float pointerOverZoom;
      
   public:
      void launchControlPanel();
      
      float get_act_xcen();
      float get_act_ycen();
      
      void setPointerOverZoom(float poz);
      
      
   protected slots:
      //void on_buttonPanelLaunch_clicked();
   
   /*** Graphics stuff ***/
   protected:
      QGraphicsScene * qgs;
      QGraphicsPixmapItem * qpmi;
      
      QGraphicsLineItem * nup;
      QGraphicsLineItem * nup_tip;
      
      float ScreenZoom;
   public:
      QGraphicsScene * get_qgs(){return qgs;}
   
   /*** Real Time Controls ***/
   //protected slots:
   void freezeRealTime();
   void reStretch();
   //void on_darkSubCheckBox_stateChanged(int st);
      
   /*** The Main View ***/
   public:
      void change_center(bool movezoombox = true);
      void set_viewcen(float x, float y, bool movezoombox = true);
      float get_xcen(){return ui.graphicsView->xCen();}
      float get_ycen(){return ui.graphicsView->yCen();}
      
      ///Matches the viewport to the same aspect ratio as the image data, by decreasing the area.
      /** Maintains the smaller of width and height, decreasing the other to match the aspect of the image.
        *
        * Invoked by the "[" key.
        *
        * \seealso squareUp()
        */  
      void squareDown();
      
      ///Matches the viewport to the same aspect ratio as the image data, by increasing the area.
      /** Maintains the larger of width and height, increasing the other to match the aspect of the image.
        *
        * Invoked by the "]" key.
        *
        * \seealso squareDown()
        */
      void squareUp();
      
   protected slots:
      void changeCenter();
   
      ///Calls changeCenter() so that the display stays centered when user resizes.
      void resizeEvent(QResizeEvent *);
   
   /*** pointer data***/
   protected:
      bool rightClickDragging;
      QPointF rightClickStart;
      float biasStart;
      float contrastStart;
      void mouseMoveEvent(QMouseEvent *e);
      void nullMouseCoords();
      
      bool NullMouseCoords;
      
      /// Update the GUI for a change in mouse coordinates in the viewport
      /** Performs the following functions:
        * - Decides whether or not the mouse is currently in the pixmap.
        * - If it is not in the pixmap, clears the mouse coord text boxes
        * - If it is, updates the moust coord text boxes
        * - If the moust is right-clicked and dragging, updates the strech bias and contrast.
        */ 
      void updateMouseCoords();
      
   protected slots:
      
      /// Receive signal that the viewport mouse coordinates have changed.
      void changeMouseCoords();
      
      void viewLeftPressed(QPointF mp);
      void viewLeftClicked(QPointF mp);
      
      void viewRightPressed(QPointF mp);
      void viewRightClicked(QPointF mp);
      
      void onWheelMoved(int delta);
      
   public:
      
      StretchBox* userBox;
      
      StretchBox* statsBox;

      StretchBox* guideBox;
      
      StretchCircle * userCircle;
      
      imviewerStats * imStats;

      void launchImStats();
      
   protected:
      float m_targetXc {0.5}; ///< The x-coordinate of the target, 0<= x <= 1
      float m_targetYc {0.5}; ///< The y-coordinate of the target, 0<= y <= 1
      
      bool m_targetVisible;  ///< Flag controlling whether the target is visible
      
      QGraphicsLineItem * m_cenLineVert; ///< The vertical line component of the target
      QGraphicsLineItem * m_cenLineHorz; ///< The horizontal line component of the target
   
      
   public:
      
      /// Set the x-coordinate of the target 
      /** The coordinate is set as a fraction of the image, 0 <= targetXc <= 1.0
        * It will be clamped to this range by this function..
        * 
        * \returns 0 on success
        */ 
      int targetXc( float txc /**< [in] the new target x coordinate*/);
      
      /// Get the x-coordinate of the target
      /** 
        * \returns the current value of m_targetXc
        */ 
      float targetXc();
      
      /// Set the y-coordinate of the target 
      /** The coordinate is set as a fraction of the image, 0 <= targetYc <= 1.0
        * It will be clamped to this range by this function..
        * 
        * \returns 0 on success
        */
      int targetYc( float tyc /**< [in] the new target y coordinate*/);
      
      /// Get the y-coordinate of the target
      /** 
        * \returns the current value of m_targetYc
        */
      float targetYc();
      
   protected slots:
      void doLaunchStatsBox();
      void doHideStatsBox();
      void imStatsClosed(int);

      void statsBoxMoved(const QRectF & newr);
      void statsBoxRejectMouse();

      void userBoxMoved(const QRectF &newr);
      void userBoxRejectMouse();
      
      void guideBoxMoved(const QRectF &newr);
      void guideBoxRejectMouse();
 
      void userCircleResized(const float &rad);
      void userCircleMoved(const QRectF &newr);
      void userCircleMouseIn();
      void userCircleMouseOut();
      void userCircleRejectMouse();
      
      
   public:
      virtual void post_setUserBoxActive(bool usba);
         
   protected:
      bool m_showLoopStat {false};
      bool m_showSaveStat {false};
      bool m_showFPSGage {true};
      
      
   public:
      
      int setAutoScale( bool as );
      
      int toggleAutoScale();
      
      int center();
      
      int showFPSGage( bool sfg );
      
      int toggleFPSGage();
      
      int toggleUserBox();
      
      int toggleStatsBox();
      
      int setDarkSub( bool ds );
      
      int toggleDarkSub();
      
      int setApplyMask( bool am );
      
      int toggleApplyMask();
      
      int setApplySatMask( bool as );
      
      int toggleApplySatMask();
      
      int toggleLogLinear();
      
      int toggleTarget();
      
      int fontLuminance();
      
   /** \name Data Dictionary
     *
     * A map of key=binary-blog pairs to be used and managed by plugins.
     * 
     * @{
     */
   
      ///The data dictionary.
      dictionaryT m_dictionary;
   
   ///@}
   
   /** \name Plugins
     *
     *@{
     */ 
   protected:
      QStringList m_pluginFileNames;
      
      void loadPlugin(QObject *plugin);
      
      std::vector<rtimvOverlayInterface *> m_overlays;
      
   ///@}
      
   private:
      Ui::rtimvMainWindow ui;

   

};

#endif //rtimvMainWindow_hpp
