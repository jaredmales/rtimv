
#ifndef __imviewerform_h__
#define __imviewerform_h__

#include <QWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QTimer>
#include <QGraphicsPixmapItem>

#include "ui_imviewergui.h"
#include "imviewer.hpp"
#include "imviewerControlPanel.h"

#include "StretchBox.h"
#include "StretchCircle.h"

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

class imviewerForm : public imviewer
{
   Q_OBJECT
   
   public:
      imviewerForm( const std::vector<std::string> & shkeys, 
                    QWidget * Parent = 0, 
                    Qt::WindowFlags f = 0
                  );
   
      virtual void postSetImsize();
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
      
      bool targetVisible;
      
      QGraphicsLineItem * cenLineVert;
      QGraphicsLineItem * cenLineHorz;
      
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
   private:
      Ui::imviewerForm ui;


};

#endif //__imviewerform_h__
