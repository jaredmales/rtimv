/** \file rtimvGraphicsView.hpp
  * \brief Declarations for the graphics view class of rtimv
  * 
  * \author Jared R. Males (jaredmales@gmail.com)
  * 
  */

#ifndef rtimv_rtimvGraphicsView_hpp
#define rtimv_rtimvGraphicsView_hpp

#include <iostream>

#include <QGraphicsView>
#include <QTimer>
#include <QScrollBar>
#include <QMouseEvent>
#include <QTextEdit>


//Defaults
#define RTIMV_DEF_WARNINGFONTFAMILY "LKLUG"
#define RTIMV_DEF_WARNINGFONTCOLOR "lightgreen"
#define RTIMV_DEF_WARNINGFONTSIZE 22

#define RTIMV_DEF_SAVEBOXFONTFAMILY "LKLUG"
#define RTIMV_DEF_SAVEBOXFONTCOLOR "lime"
#define RTIMV_DEF_SAVEBOXFONTSIZE 18

#define RTIMV_DEF_LOOPFONTFAMILY "LKLUG"
#define RTIMV_DEF_LOOPFONTCOLOR "lime"
#define RTIMV_DEF_LOOPFONTSIZE 18

#define RTIMV_DEF_GAGEFONTFAMILY "LKLUG"
#define RTIMV_DEF_GAGEFONTCOLOR "skyblue"
#define RTIMV_DEF_GAGEFONTSIZE 14

#define RTIMV_DEF_ZOOMFONTFAMILY "LKLUG"
#define RTIMV_DEF_ZOOMFONTCOLOR "skyblue"
#define RTIMV_DEF_ZOOMFONTSIZE 18
#define RTIMV_DEF_ZOOMTIMEOUT 2000

#define RTIMV_DEF_STATUSTEXTFONTFAMILY "LKLUG"
#define RTIMV_DEF_STATUSTEXTFONTCOLOR "skyblue"
#define RTIMV_DEF_STATUSTEXTFONTSIZE 14

///The rtimv Graphics View
class rtimvGraphicsView : public QGraphicsView
{
   Q_OBJECT
   
   public:
      ///Constructor
      explicit rtimvGraphicsView(QWidget *parent = 0);
      
   /** \name Status Displays
     * @{
     */
      
   protected:     
      ///Setup a text box
      void textEditSetup( QTextEdit * te /**< [in/out] the text box to setup */);
      
      ///The warning text box 
      QTextEdit * m_warningText;
      
      QString m_warningFontFamily; ///< Font family for the warning text box
      float m_warningFontSize; ///< Font size for the warning text box
      QString m_warningFontColor; ///< Font color for the warning text box
   
   public:
      ///Set the Warning font size
      void warningFontFamily( const char * ff /**< [in] The new font family */ );
      
      ///Set the Warning font size
      void warningFontSize( float fs /**< [in] The new font size */ );

      ///Set the warning font color
      void warningFontColor( const char * fc /**< [in] The new font color */ );
      
      ///Get the current warning font family
      /**
        * \returns the warning font family
        */ 
      QString warningFontFamily();
      
      ///Get the current warning font size
      /**
        * \returns the warning font size
        */
      float warningFontSize();
      
      ///Get the current warning font color
      /**
        * \returns the warning font color
        */
      QString warningFontColor();
      
      ///Set the warning text
      /** If a font color is supplied the warning text is set to that color.  Otherwise (fc == 0), then _warningFontColor is used.
        */
      void warningText( const char * nt, ///< [in] the new warning text
                        const char * fc=0  ///< [in] [optional] color for the warning text
                      );
      
      
   protected:
      ///The loop status text box
      QTextEdit * m_loopText;
      
      QString m_loopFontFamily; ///< Font family for the loop status box
      float m_loopFontSize; ///< Font size for the loop status box
      QString m_loopFontColor; ///< Font color for the loop status box
   
   public:
      ///Set the Loop font size
      void loopFontFamily( const char * ff /**< [in] The new font family */ );
      
      ///Set the Loop font size
      void loopFontSize( float fs /**< [in] The new font size */ );

      ///Set the loop font color
      void loopFontColor( const char * fc /**< [in] The new font color */);
      
      ///Get the current loop font family
      /**
        * \returns the loop font family
        */ 
      QString loopFontFamily();
      
      ///Get the current loop font size
      /**
        * \returns the loop font size
        */
      float loopFontSize();
      
      ///Get the current loop font color
      /**
        * \returns the loop font color
        */
      QString loopFontColor();
      
      ///Set the loop text
      /** If a font color is supplied the loop text is set to that color.  Otherwise (fc == 0), then _loopFontColor is used.
        */
      void loopText( const char * nt, ///< [in] the new loop text
                     const char * fc=0  ///< [in] [optional] color for the loop text
                   );
      
   protected:   
      ///The save status box
      QTextEdit * m_saveBox;
      
      QString m_saveBoxFontFamily; ///< Font family for the save box
      float m_saveBoxFontSize; ///< Font size for the save box
      QString m_saveBoxFontColor; ///< Font color for the save box
   
   public:
      ///Set the SaveBox font size
      void saveBoxFontFamily( const char * ff /**< [in] The new font family */ );
      
      ///Set the SaveBox font size
      void saveBoxFontSize( float fs /**< [in] The new font size */ );

      ///Set the saveBox font color
      void saveBoxFontColor( const char * fc /**< [in] The new font color */ );
      
      ///Get the current saveBox font family
      /**
        * \returns the saveBox font family
        */ 
      QString saveBoxFontFamily();
      
      ///Get the current saveBox font size
      /**
        * \returns the saveBox font size
        */
      float saveBoxFontSize();
      
      ///Get the current saveBox font color
      /**
        * \returns the saveBox font color
        */
      QString saveBoxFontColor();
      
      ///Set the saveBox text
      /** If a font color is supplied the saveBox text is set to that color.  Otherwise (fc == 0), then _saveBoxFontColor is used.
        */
      void saveBoxText( const char * nt, ///< [in] the new saveBox text
                        const char * fc=0  ///< [in] [optional] color for the saveBox text
                      );
   
   protected:
      ///The system status grid
      std::vector<QTextEdit *> m_statusText;
      QString m_statusTextFontFamily;
      float m_statusTextFontSize;
      QString m_statusTextFontColor;
      
   public:
      /// Set the number of status text fields
      /** This resizes m_statusText, first deleting any existing text boxes.
        * Does not currently handle what to do if these are currently being used, so if,
        * say, there are competing extensions this could lead to segfaults.
        */
      void statusTextNo(size_t no /**< [in] the new number of status text fields*/);
      
      /// Get the number of status text fields
      /**
        * \returns m_statusText.size(), the number of available fields in the status text grid
        */ 
      size_t statusTextNo();
      
      ///Set the statusText font size
      void statusTextFontFamily( const char * ff /**< [in] The new font family */ );
      
      ///Set the statusText font size
      void statusTextFontSize( float fs /**< [in] The new font size */ );

      ///Set the statusText font color
      void statusTextFontColor( const char * fc /**< [in] The new font color */ );
      
      ///Get the current statusText font family
      /**
        * \returns the statusText font family
        */ 
      QString statusTextFontFamily();
      
      ///Get the current statusText font size
      /**
        * \returns the statusText font size
        */
      float statusTextFontSize();
      
      ///Get the current statusText font color
      /**
        * \returns the statusText font color
        */
      QString statusTextFontColor();
      
      ///Set the statusText text
      /** If a font color is supplied the statusText text is set to that color.  Otherwise (fc == 0), then _statusTextFontColor is used.
        */
      void statusTextText( size_t fieldNo,    ///< [in] the field number (0- statusTextNo()-1)
                           const char * nt,   ///< [in] the new statusText text
                           const char * fc=0  ///< [in] [optional] color for the statusText text
                         );
      
      
      ///@}
      
   /** \name The Gages 
    * @{
    */
   
   public:
      QTextEdit * m_fpsGage; ///< The FPS and age gage
      QTextEdit * m_textCoordX; ///< The x-coordinate of the mouse pointer
      QTextEdit * m_textCoordY; ///< The y-coordinate of the mouse pointer
      QTextEdit * m_textPixelVal; /// The value of the pixel under the mouse pointer
      
      QString m_gageFontFamily; ///< The font family for the gages
      float m_gageFontSize; ///< The font size of the gages
      QString m_gageFontColor; ///< The font color of the gages
      
   public:
      
      const QTextEdit * fpsGage();
      
      ///Set the gage font family
      void gageFontFamily( const char * ff /**< [in] The new font family */ );
      
      ///Set the gage font size
      void gageFontSize( float fs /**< [in] The new font size */ );

      ///Set the gage font color
      void gageFontColor( const char * fc /**< [in] The new font color */ );
      
      ///Get the current gage font family
      /**
        * \returns the gage font family
        */ 
      QString gageFontFamily();
      
      ///Get the current gage font size
      /**
        * \returns the gage font size
        */
      float gageFontSize();
      
      ///Get the current gage font color
      /**
        * \returns the gage font color
        */
      QString gageFontColor();
      
      ///Set the FPS Gage text.
      void fpsGageText( const char * nt /**< [in] The new FPS gage text */ );
      
      ///Set the FPS Gage text by value.
      void fpsGageText( float nt, ///< [in] The new FPS value  
                        bool age = false ///< [in] [optional] if true then Age: is used instead of FPS:
                      );
      
      ///Set the X Gage text.
      void textCoordX( const char * nt /**< [in] The new X gage text */ );
      
      ///Set the X Gage text by value.
      void textCoordX( float nv /**< [in] The new X value */ );
      
      ///Set the Y Gage text.
      void textCoordY( const char * nt /**< [in] The new Y gage text */ );
      
      ///Set the Y Gage text by value.
      void textCoordY( float nv /**< [in] The new Y value */ );
      
      ///Set the Value Gage text.
      void textPixelVal( const char * nt /**< [in] The new Value gage text */ );

      ///Set the Value Gage text by value.
      void textPixelVal( float nv /**< [in] The new value */ );
      
      
   protected:
      ///The zoom level box
      QTextEdit * m_zoomText;
      
      QString m_zoomFontFamily; ///< Font family for the zoom box
      float m_zoomFontSize; ///< Font size for the zoom box
      QString m_zoomFontColor; ///< Font color for the zoom color
   
      QTimer m_zoomTimer; ///< When this timer expires the zoomText box is hidden
      int    m_zoomTimeOut { RTIMV_DEF_ZOOMTIMEOUT}; ///<The timeout length in msec for hiding the zoom box.
      
   public:
      ///Set the Zoom font size
      void zoomFontFamily(const char * ff /**< [in] The new font family */ );
      
      ///Set the Zoom font size
      void zoomFontSize( float fs /**< [in] The new font size */ );

      ///Set the zoom font color
      void zoomFontColor( const char * fc /**< [in] The new font color */ );
      
      ///Get the current zoom font color
      /**
        * \returns the zoom font color
        */
      QString zoomFontColor();
      
      ///Get the current zoom font family
      /**
        * \returns the zoom font family
        */ 
      QString zoomFontFamily();
      
      ///Get the current zoom font size
      /**
        * \returns the zoom font size
        */
      float zoomFontSize();
      
      ///Set the zoom text
      /** If a font color is supplied the zoom text is set to that color.  Otherwise (fc == 0), then _zoomFontColor is used.
        */
      void zoomText( const char * nt, ///< [in] the new zoom text
                     const char * fc=0  ///< [in] [optional] color for the zoom text
                   );
      
   protected slots:
      ///Handle the timeout signal of the zoomTimer.
      /** This just sets the zoom text to "" and stops the timer.
        */
      void zoomTimerOut();
      
   public:
      QTextEdit * coords;

   /** \name Mouse Interaction 
     * 
     * A middle click sets the center coordinate, which is used to center the scene in the viewport.
     * 
     * Viewport coordinates of the mouse are reported any time it moves.
     * 
     * @{
     */

      //************************************
      //Mouse interactions
   protected:
      
      float m_xCen; ///< The requested x-coordinate of the  center of the current view, in fractions of the image width
      float m_yCen; ///< The requested y-coordinate of the  center of the current view, in fractions of the image height
   
   ///@}
   
   public:
   
      /// Center the view port on the given scene coordinate
      /** Overloads QGraphicsView::centerOn so we can capture this coordinate
        * for later use.  After setting m_xCen=x and m_yCen = y, just calls
        * QGraphicsView::centerOn(x,y).
        */ 
      void centerOn( qreal x, ///< [in] The scene x coordinate on which to center
                     qreal y  ///< [in] The scene y coordinate on which to center
                   );
      
      /// Set the scene-coordinate center given viewport coordinates.
      /** 
        */ 
      void mapCenterToScene( float xc, ///< [in] the viewport x-coorodinate on which to center
                             float yc  ///< [in] the viewport x-coorodinate on which to center 
                           );
      
      ///Get the requested x center
      /**
        * \returns m_xCen
        */ 
      float xCen();
      
      ///Get the requested y center
      /**
        * \returns m_yCen
        */
      float yCen();
            
   protected:
      float m_mouseViewX; ///< The current x-coordinate of the mouse in viewport coords
      float m_mouseViewY; ///< The current y-coordinate of the mouse in viewport coords
   
   public:
      
      ///Get the current mouse x coordinate
      /**
        * \returns the current mouse x coordinate
        */ 
      float mouseViewX();
      
      ///Get the current mouse y coordinate
      /**
        * \returns the current mouse y coordinate
        */       
      float mouseViewY();
    
   protected:
      float m_zoomLevel; ///< The current zoom level
      float m_screenZoom; ///< The current screen zoom factor, the ratio of screen pixels to physical pixels when ZoomLevel==1.
      
   public:
      ///Set the current zoom level
      void zoomLevel( float zl /**< [in] the new zoom level*/ );
      
      ///Get the current zoom level
      /**
        * \returns the current zoom level.
        */ 
      float zoomLevel();
      
      ///Set the current screen zoom
      void screenZoom( float sz /**< [in] the new screen zoom*/ );
      
      ///Get the current screen zoom
      /** 
        * \returns the current screen zoom
        */ 
      float screenZoom();
      
      
   signals:
      void centerChanged();
      void mouseCoordsChanged();
      void leftPressed(QPointF mp);
      void leftClicked(QPointF mp);
      void rightPressed(QPointF mp);
      void rightClicked(QPointF mp);
      void wheelMoved(int delta);
   
   protected slots:
      void resizeEvent(QResizeEvent *);
      
   protected:
      void mouseMoveEvent(QMouseEvent *e);
      void leaveEvent(QEvent * e);
      
      void mousePressEvent(QMouseEvent *e);
      void mouseReleaseEvent(QMouseEvent *e);
      void mouseDoubleClickEvent(QMouseEvent *e);
      
      void wheelEvent(QWheelEvent *e);

   ///@}
      
};

#endif //rtimv_rtimvGraphicsView_hpp

