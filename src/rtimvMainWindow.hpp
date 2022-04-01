
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


#include <mx/app/application.hpp>

using namespace mx::app;


#include "ui_rtimvMainWindow.h"
#include "rtimvBase.hpp"
#include "rtimvControlPanel.hpp"

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
   rtimvMainWindow( int argc,
                    char ** argv,
                    QWidget * Parent = 0, 
                    Qt::WindowFlags f = 0
                  );
      
   ~rtimvMainWindow();
   
public:
   virtual void setupConfig();
      
   virtual void loadConfig();
      
protected:
   std::string m_title {"rtimv"};

   
public:
   
   ///Called on initial connection to the image stream, sets matching aspect ratio.
   virtual void onConnect();
   
   virtual void postSetImsize();

   /// Setup the target lines
   void setTarget();
   
   virtual void post_zoomLevel();
   
   virtual void postChangeImdata();
   
   virtual void updateFPS();
   
   virtual void updateAge();

   ///Update the display while not connected.
   virtual void updateNC();

   virtual void keyPressEvent(QKeyEvent *);
   
   /*** The control Panel ***/
protected:
   rtimvControlPanel *imcp;
   
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
   QGraphicsScene * m_qgs {nullptr};
   QGraphicsPixmapItem * m_qpmi {nullptr};
   
   QGraphicsLineItem * nup;
   QGraphicsLineItem * nup_tip;
   
   float ScreenZoom;
public:
   QGraphicsScene * get_qgs(){return m_qgs;}

   /*** Real Time Controls ***/
   
   void freezeRealTime();

   void reStretch();
   
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
      
      bool m_nullMouseCoords {true}; ///< Flag indicating whether or not the mouse coordinates have been nulled.
      
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
      
      StretchBox* m_colorBox;
      
      StretchBox* m_statsBox {nullptr};

      std::unordered_set<StretchBox *> m_userBoxes;
      std::unordered_set<StretchCircle *> m_userCircles;
      std::unordered_set<StretchLine *> m_userLines;
      
      rtimvStats * imStats;

      void launchImStats();
      
   protected:
      float m_targetXc {0.5}; ///< The x-coordinate of the target, 0<= x <= 1
      float m_targetYc {0.5}; ///< The y-coordinate of the target, 0<= y <= 1
      
      bool m_targetVisible;  ///< Flag controlling whether the target is visible
      
      QGraphicsLineItem * m_cenLineVert; ///< The vertical line component of the target
      QGraphicsLineItem * m_cenLineHorz; ///< The horizontal line component of the target
   
      
   public:
      
      /// Add a user box 
      void addUserBox();
      
      /// Add a user circle 
      void addUserCircle();
      
      /// Add a user line 
      void addUserLine();
      
      QGraphicsEllipseItem * m_lineHead;
      QGraphicsLineItem * m_objCenV;
      QGraphicsLineItem * m_objCenH;
      
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
   
   /// Add a StretchBox to user boxes
   /** This addes the StretchBox to the user boxes, and connects
     * its rejectMouse signal to the userBoxRejectMouse slot so that 
     * the mouse though management works.  The remove signal is connected to the 
     * userBoxRemove slog.  None of the other signals are connected.
     * 
     * This can be connected to a signal from a plugin to register a new
     * StretchBox for mouse management.
     * 
     */
   void addStretchBox(StretchBox * sb /**< [in] The new StretchBox to add*/);
   
   /// Add a StretchCircle to the user circle
   /** This addes the StretchCircle to the user boxes, and connects
     * its rejectMouse signal to the userCircleRejectMouse slot so that 
     * the mouse though management works.  The remove signal is connected to the 
     * userCircleRemove slog.  None of the other signals are connected.
     * 
     * This can be connected to a signal from a plugin to register a new
     * StretchCircle for mouse management.
     * 
     */
   void addStretchCircle(StretchCircle * sc /**< [in] The new StretchCircle to add*/);
   
   /// Add a StretchLine to the user lines
   /** This addes the StretchLine to the user boxes, and connects
     * its rejectMouse signal to the userLineRejectMouse slot so that 
     * the mouse though management works.  The remove signal is connected to the 
     * userLineRemove slog.  None of the other signals are connected.
     * 
     * This can be connected to a signal from a plugin to register a new
     * StretchLine for mouse management.
     * 
     */
   void addStretchLine(StretchLine * sl) /**< [in] The new StretchLine to add*/;
   
   void doLaunchStatsBox();
   void doHideStatsBox();
   void imStatsClosed(int);

   void statsBoxMoved(StretchBox *);

   void colorBoxMoved(StretchBox *);

   void userBoxResized(StretchBox * sb);
   void userBoxMoved(StretchBox * sb);
   void userBoxMouseIn(StretchBox * sb);
   void userBoxMouseOut(StretchBox * sb);
   void userBoxRejectMouse(StretchBox *);
   void userBoxRemove(StretchBox * sc);
   
   void userCircleResized(StretchCircle * sc);
   void userCircleMoved(StretchCircle * sc);
   void userCircleMouseIn(StretchCircle * sc);
   void userCircleMouseOut(StretchCircle * sc);
   void userCircleRejectMouse(StretchCircle * sc);
   void userCircleRemove(StretchCircle * sc);

   
   void userLineResized(StretchLine * sl);
   void userLineMoved(StretchLine * sl);
   void userLineMouseIn(StretchLine * sl);
   void userLineMouseOut(StretchLine * sl);
   void userLineRejectMouse(StretchLine * sl);
   void userLineRemove(StretchLine * sl);
         
   public:
      virtual void post_setUserBoxActive(bool usba);
         
   protected:
      bool m_showLoopStat {false};
      bool m_showSaveStat {false};
      bool m_showFPSGage {true};
      
      
   public:
      
      void setAutoScale( bool as );
      
      void toggleAutoScale();
      
      void center();
      
      void showFPSGage( bool sfg );
      
      void toggleFPSGage();
      
      void toggleColorBox();
      
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
      
      std::string generateHelp();
      
      void toggleHelp();
      
      int fontLuminance(QTextEdit* qte);
      
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
      
      int loadPlugin(QObject *plugin);
      
      std::vector<rtimvOverlayInterface *> m_overlays;
      
   ///@}
      
   private:
      Ui::rtimvMainWindow ui;

   

};

#endif //rtimvMainWindow_hpp
