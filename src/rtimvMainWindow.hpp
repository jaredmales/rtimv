
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
                    QWidget * Parent = nullptr, 
                    Qt::WindowFlags f = Qt::WindowFlags()
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

/*** Graphics stuff ***/
protected:
   QGraphicsScene * m_qgs {nullptr};
   QGraphicsPixmapItem * m_qpmi {nullptr};



   float ScreenZoom;

public:
   QGraphicsScene * get_qgs();

/** \name North Arrow
  * The north arrow is toggled on/off with the `n` key.  The angle of the north arrow returned by northAngle() is determined by
  * the dictionary key "rtimv.north.angle", which can be set from a dictionary plugin or using the slot
  * northAngleRaw(float).  This is multiplied by m_northAngleScale and then m_northAngleOffset is added to the result to produce
  * a counter clockwise rotation angle in degrees.
  * 
  * The north arrow can be disabled in configuration.  If "north.enabled=false" is set, the north arrow will not be displayed.
  * 
  * Relevant configuration key=value pairs are
  * - `north.enabled=true/false`: Whether or not to enable the north arrow. Default is true.
  * - `north.offset=float`: Offset in degrees c.c.w. to apply to the north angle. Default is 0.
  * - `north.scale=float`: Scaling factor to apply to north angle to convert to degrees c.c.w. on the image.  Default is -1.
  * @{ 
  */
protected:   
   
   QGraphicsLineItem * m_northArrow; ///< The north arrow line.
   
   QGraphicsLineItem * m_northArrowTip; ///< The north arrow tip line.
   
   bool m_northArrowEnabled {true}; ///< Flag controlling whether or not the north arrow is enabled.
   
   float m_northAngleScale {-1}; ///< The scale to multiply the value contained in "rtimv.north.angle" to convert to degrees c.c.w.
   
   float m_northAngleOffset {0}; ///< The offset to apply to the scaled north angle in degrees c.c.w.

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
   void northAngleRaw(float north /**< [in] the new value of the raw north angle*/);

  ///@}

   /*** Real Time Controls ***/
public:
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

   bool m_showToolTipCoords {true}; ///< Flag indicating whether the mouse tool tip should be shown
   bool m_showStaticCoords {false}; ///< Flag indicating whether the mouse tool tip should be shown

   bool m_nullMouseCoords {true}; ///< Flag indicating whether or not the mouse coordinates have been nulled.

   void mouseMoveEvent(QMouseEvent *e);

   void nullMouseCoords();

   /// Update the GUI for a change in mouse coordinates in the viewport
   /** Performs the following functions:
     * - Decides whether or not the mouse is currently in the pixmap.
     * - If it is not in the pixmap, clears the mouse coord text boxes
     * - If it is, updates the mouse coord text boxes
     * - If the mouse is right-clicked and dragging, updates the strech bias and contrast.
     */ 
   void updateMouseCoords();

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
   void showToolTipCoords(bool sttc /**< [in] The new value of the flag */);
      
   /// Receive signal to show or hide static coordinates
   void showStaticCoords(bool ssc /**< [in] The new value of the flag */);

signals:
   /// Notify that the tool tip coordinate display flag has changed.
   void showToolTipCoordsChanged(bool sttc /**< [in] The new value of the flag */);

   /// Notify that the static coordinate display flag has changed.
   void showStaticCoordsChanged(bool sttc /**< [in] The new value of the flag */);

public slots:
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
      QGraphicsEllipseItem * m_lineHead;
      QGraphicsLineItem * m_objCenV;
      QGraphicsLineItem * m_objCenH;

      bool m_userItemSelected {false};
         
   public:
      
      /// Add a user box 
      void addUserBox();
      
      /// Add a user circle 
      void addUserCircle();
      
      /// Add a user line 
      void addUserLine();
      
      

   /*---- Target Cross ----*/
protected:
   float m_targetXc {0.5}; ///< The x-coordinate of the target, 0<= x <= 1
   float m_targetYc {0.5}; ///< The y-coordinate of the target, 0<= y <= 1
      
   bool m_targetVisible;  ///< Flag controlling whether the target is visible
      
   QGraphicsLineItem * m_cenLineVert; ///< The vertical line component of the target
   QGraphicsLineItem * m_cenLineHorz; ///< The horizontal line component of the target

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
   void targetXc( float txc /**< [in] the new target x coordinate*/);

   /// Set the y-coordinate of the target 
   /** The coordinate is set as a fraction of the image, 0 <= targetYc <= 1.0
     * It will be clamped to this range by this function..
     */
   void targetYc( float tyc /**< [in] the new target y coordinate*/);

   /// Set whether or not the target is visible   
   void targetVisible( bool tv /**< [in] the new status of target visibility */ );

signals:

   void targetXcChanged(float txc /**< [in] the new target x coordinate*/);

   void targetYcChanged(float tyc /**< [in] the new target y coordinate*/);

   void targetVisibleChanged(bool tv /**< [in] the new status of target visibility */);

public:
   /// Position the target lines
   /** This is normally called by the targetXc(float) and targetYc(float) slots
     */
   void setTarget();


   /** \name User Shapes
     * @{
     */
 
   void userItemMouseCoords( float mx,
                             float my
                           );

   void userBoxItemSize( StretchBox * sb );
   void userBoxItemCoords( StretchBox * sb );

   void userCircleItemSize( StretchCircle * sb );
   void userCircleItemCoords( StretchCircle * sc );

   float m_userItemXCen {0}; ///< Center of active user item in scene coordinates
   float m_userItemYCen {0}; ///< Center of active user item in scene coordinates
   float m_userItemMouseViewX {0}; ///< Center of active user item in viewport coordinates
   float m_userItemMouseViewY {0}; ///< Center of active user item in viewport coordinates
   

   void userBoxItemMouseCoords( StretchBox * sb );

   void userCircleItemMouseCoords( StretchCircle * sc );

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
   void userBoxSelected(StretchBox * sc);
   void userBoxDeSelected(StretchBox * sc);

   void userCircleResized(StretchCircle * sc);
   void userCircleMoved(StretchCircle * sc);
   void userCircleMouseIn(StretchCircle * sc);
   void userCircleMouseOut(StretchCircle * sc);
   void userCircleRejectMouse(StretchCircle * sc);
   void userCircleRemove(StretchCircle * sc);
   void userCircleSelected(StretchCircle * sc);
   void userCircleDeSelected(StretchCircle * sc);
   
   void userLineResized(StretchLine * sl);
   void userLineMoved(StretchLine * sl);
   void userLineMouseIn(StretchLine * sl);
   void userLineMouseOut(StretchLine * sl);
   void userLineRejectMouse(StretchLine * sl);
   void userLineRemove(StretchLine * sl);
   void userLineSelected(StretchLine * sl);
   void userLineDeSelected(StretchLine * sl);

   void savingState(bool ss);
   
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
      double m_lumPwr {12};
      
      /// The luminance threshold below which no background transparency is set.
      /** Above this, but below m_lumMax, linear interpolation is used to set the opacity.
        * 
        * Range: 0-255;
        */
      double m_lumThresh {100};

      /// The maximum luminance, at which point the opacity is set to m_opacityMax.
      /** Below this, but above m_lumThresh, linear interpolation is used to set the opacity.
        * 
        * Range: 0-255;
        */ 
      double m_lumMax {175};

      /// The maximum opacity which will be set.
      /** This is the opacity set when at or above m_lumMax.
        * Between m_lumThresh and m_lumMax, the opacity is interpolated between 0 and this value.
        * 
        * Range: 0-255;
        */
      double m_opacityMax {150};
   
   public:

      /// Sets the background transparency of a QTextEdit based on the average luminance of the background pixels.
      /** Uses a power-quadrature average (see m_lumPwr) and sets the background opacity based on interpolation with
        * the values of m_lumThresh, m_lumMax, and m_opacityMax.
        */
      void fontLuminance( QTextEdit* qte,    ///< [in/out] the QTextEdit to modify
                          bool print = false ///< [in] [out] if true the average luminance is printed.  Used for debugging.
                        );
       
      /// Update the background opacity for all the text fields.  Called periodically.
      void fontLuminance();
      
   ///@}

   /** \name Data Dictionary
     *
     * A map of key=binary-blob pairs to be used and managed by plugins.
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
