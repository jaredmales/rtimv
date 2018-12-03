
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
      imviewerForm(imviewer_shmt shkey, QWidget * Parent = 0, Qt::WindowFlags f = 0);
   
      virtual void postSetImsize();
      virtual void post_set_ZoomLevel();
      virtual void postChangeImdata();
      //virtual void stale_fps();
      virtual void update_age();

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
      void update_MouseCoords();
      
   protected slots:
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
   private:
      Ui::imviewerForm ui;


};

#endif //__imviewerform_h__
