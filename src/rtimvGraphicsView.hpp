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
#include <QFontMetrics>

// Defaults

#define RTIMV_DEF_LOOPFONTFAMILY "LKLUG"
#define RTIMV_DEF_LOOPFONTCOLOR "lime"
#define RTIMV_DEF_LOOPFONTSIZE ( 18 * 1.4 )

#define RTIMV_DEF_STATUSTEXTFONTFAMILY "LKLUG"
#define RTIMV_DEF_STATUSTEXTFONTCOLOR "#3DA5FF" /**"skyblue"*/
#define RTIMV_DEF_STATUSTEXTFONTSIZE ( 14 * 1.4 )

#define RTIMV_DEF_COLORBOXCOLOR "yellow"

#define RTIMV_DEF_STATSBOXCOLOR "#3DA5FF"

/// The rtimv Graphics View
class rtimvGraphicsView : public QGraphicsView
{
    Q_OBJECT

  public:
    /// Constructor
    explicit rtimvGraphicsView( QWidget *parent = 0 );

    /** \name Status Display Data
     * @{
     */
  protected:
    /// The warning text box
    QTextEdit *m_warningText;

    QString m_warningFontFamily{ "LKLUG" }; ///< Font family for the warning text box
    float m_warningFontSize{ 30 };          ///< Font size for the warning text box
    QString m_warningFontColor{ "red" };    ///< Font color for the warning text box

    /// The loop status text box
    QTextEdit *m_loopText;

    QString m_loopFontFamily; ///< Font family for the loop status box
    float m_loopFontSize;     ///< Font size for the loop status box
    QString m_loopFontColor;  ///< Font color for the loop status box

    /// The save status box
    QTextEdit *m_saveBox;

    QString m_saveBoxFontFamily{ "LKLUG" }; ///< Font family for the save box
    float m_saveBoxFontSize{ 25 };          ///< Font size for the save box
    QString m_saveBoxFontColor{ "lime" };   ///< Font color for the save box

    /// The system status grid
    std::vector<QTextEdit *> m_statusText;

    QString m_statusTextFontFamily; ///< Font family for the save box
    float m_statusTextFontSize;     ///< Font size for the save box
    QString m_statusTextFontColor;  ///< Font color for the save box

    /// The help text
    QTextEdit *m_helpText;

    QString m_helpTextFontFamily{ "" };                      ///< Font family for the help text
    float m_helpTextFontSize{ 16 };                          ///< Font size for the help text
    QString m_helpTextFontColor{ "#3DA5FF" /**"skyblue"*/ }; ///< Font color for the help text

    QTextEdit *m_fpsGage;      ///< The FPS and age gage
    QTextEdit *m_textCoordX;   ///< The x-coordinate of the mouse pointer
    QTextEdit *m_textCoordY;   ///< The y-coordinate of the mouse pointer
    QTextEdit *m_textPixelVal; /// The value of the pixel under the mouse pointer

    QTextEdit *m_mouseCoords; ///< The mouse-tip coordinate display

    QString m_gageFontFamily{ "LKLUG" };                 ///< The font family for the gages
    float m_gageFontSize{ 30 };                          ///< The font size of the gages
    QString m_gageFontColor{ "#3DA5FF" /**"skyblue"*/ }; ///< The font color of the gages

    /// The zoom level box
    QTextEdit *m_zoomText;

    QString m_zoomFontFamily{ "LKLUG" };                 ///< Font family for the zoom box
    float m_zoomFontSize{ 25 };                          ///< Font size for the zoom box
    QString m_zoomFontColor{ "#3DA5FF" /**"skyblue"*/ }; ///< Font color for the zoom color

    QTimer m_zoomTimer;        ///< When this timer expires the zoomText box is hidden
    int m_zoomTimeOut{ 2000 }; ///< The timeout length in msec for hiding the zoom box.

  protected slots:
    /// Handle the timeout signal of the zoomTimer.
    /** This just sets the zoom text to "" and stops the timer.
     */
    void zoomTimerOut();

  protected:
    QString m_userItemDefColor{ "lime" };    ///< The default line color for user items.
    QString m_userItemColor{ "lime" };       ///< The current line color for user items.
    QString m_userItemFontFamily{ "LKLUG" }; ///< Font family for the user item size and mouse coords

    QTextEdit *m_userItemSize; ///< The user item size text edit field

    float m_userItemSizeFontSize{ 22.0 }; ///< Font size for the user item size

    QTextEdit *m_userItemMouseCoords; ///< The user item mouse coordinates text edit

    float m_userItemMouseCoordsFontSize{ 22.0 }; ///< Font size for the user item mouse coordinates

    /// @}

    /** \name Status Display Access
     * @{
     */

  public:
    /// Setup a text box
    void textEditSetup( QTextEdit *te /**< [in/out] the text box to setup */ );

    /// Set the Warning font size
    void warningFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Get the current warning font family
    /**
     * \returns the warning font family
     */
    QString warningFontFamily();

    /// Set the Warning font size
    void warningFontSize( float fs /**< [in] The new font size */ );

    /// Get the current warning font size
    /**
     * \returns the warning font size
     */
    float warningFontSize();

    /// Set the warning font color
    void warningFontColor( const QString &fc /**< [in] The new font color */ );

    /// Get the current warning font color
    /**
     * \returns the warning font color
     */
    QString warningFontColor();

    /// Set the warning text
    /** If a font color is supplied the warning text is set to that color.  Otherwise (fc == 0), then _warningFontColor
     * is used.
     */
    void warningText( const char *nt /**< [in] the new warning text */ );

    /// Set the Loop font size
    void loopFontFamily( const char *ff /**< [in] The new font family */ );

    /// Set the Loop font size
    void loopFontSize( float fs /**< [in] The new font size */ );

    /// Set the loop font color
    void loopFontColor( const char *fc /**< [in] The new font color */ );

    /// Get the current loop font family
    /**
     * \returns the loop font family
     */
    QString loopFontFamily();

    /// Get the current loop font size
    /**
     * \returns the loop font size
     */
    float loopFontSize();

    /// Get the current loop font color
    /**
     * \returns the loop font color
     */
    QString loopFontColor();

    /// Set the loop text
    /** If a font color is supplied the loop text is set to that color.  Otherwise (fc == 0), then _loopFontColor is
     * used.
     */
    void loopText( const char *nt,    ///< [in] the new loop text
                   const char *fc = 0 ///< [in] [optional] color for the loop text
    );

    /// Get a pointer to the save box text edit
    /**
     * \returns a the value of m_saveBox
     */
    QTextEdit *saveBox();

    /// Set the SaveBox font size
    void saveBoxFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Get the current saveBox font family
    /**
     * \returns the saveBox font family
     */
    QString saveBoxFontFamily();

    /// Set the SaveBox font size
    void saveBoxFontSize( float fs /**< [in] The new font size */ );

    /// Get the current saveBox font size
    /**
     * \returns the saveBox font size
     */
    float saveBoxFontSize();

    /// Set the saveBox font color
    void saveBoxFontColor( const QString &fc /**< [in] The new font color */ );

    /// Get the current saveBox font color
    /**
     * \returns the saveBox font color
     */
    QString saveBoxFontColor();

    /// Set the saveBox text
    /** If a font color is supplied the saveBox text is set to that color.  Otherwise (fc == 0), then _saveBoxFontColor
     * is used.
     */
    void saveBoxText( const char *nt,    ///< [in] the new saveBox text
                      const char *fc = 0 ///< [in] [optional] color for the saveBox text
    );

    /// Set the number of status text fields
    /** This resizes m_statusText, first deleting any existing text boxes.
     * Does not currently handle what to do if these are currently being used, so if,
     * say, there are competing extensions this could lead to segfaults.
     */
    void statusTextNo( size_t no /**< [in] the new number of status text fields*/ );

    /// Get the number of status text fields
    /**
     * \returns m_statusText.size(), the number of available fields in the status text grid
     */
    size_t statusTextNo();

    /// Get a pointer to a status text edit
    /**
     * \returns a the value of m_statusText[n]
     */
    QTextEdit *statusText( size_t n );

    /// Set the statusText font size
    void statusTextFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Get the current statusText font family
    /**
     * \returns the statusText font family
     */
    QString statusTextFontFamily();

    /// Set the statusText font size
    void statusTextFontSize( float fs /**< [in] The new font size */ );

    /// Get the current statusText font size
    /**
     * \returns the statusText font size
     */
    float statusTextFontSize();

    /// Set the statusText font color
    void statusTextFontColor( const QString &fc /**< [in] The new font color */ );

    /// Get the current statusText font color
    /**
     * \returns the statusText font color
     */
    QString statusTextFontColor();

    /// Set the statusText text
    /** If a font color is supplied the statusText text is set to that color.  Otherwise (fc == 0), then
     * _statusTextFontColor is used.
     */
    void statusTextText( size_t fieldNo, ///< [in] the field number (0- statusTextNo()-1)
                         const char *nt  ///< [in] the new statusText text
    );

    /// Get a pointer to the help text edit
    /**
     * \returns a the value of m_helpText
     */
    QTextEdit *helpText();

    /// Set the SaveText font size
    void helpTextFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Get the current helpText font family
    /**
     * \returns the helpText font family
     */
    QString helpTextFontFamily();

    /// Set the SaveText font size
    void helpTextFontSize( float fs /**< [in] The new font size */ );

    /// Get the current helpText font size
    /**
     * \returns the helpText font size
     */
    float helpTextFontSize();

    /// Set the helpText font color
    void helpTextFontColor( const QString &fc /**< [in] The new font color */ );

    /// Get the current helpText font color
    /**
     * \returns the helpText font color
     */
    QString helpTextFontColor();

    /// Set the helpText text
    /** If a font color is supplied the helpText text is set to that color.  Otherwise (fc == 0), then
     * _helpTextFontColor is used.
     */
    void helpTextText( const char *nt /**< [in] the new helpText text */ );

    /// Get a pointer to the fpsGage text edit
    /**
     * \returns a the value of m_fpsGage
     */
    QTextEdit *fpsGage();

    /// Get a pointer to the textCoordX text edit
    /**
     * \returns a the value of m_textCoordX
     */
    QTextEdit *textCoordX();

    /// Get a pointer to the textCoordY text edit
    /**
     * \returns a the value of m_textCoordY
     */
    QTextEdit *textCoordY();

    /// Get a pointer to the textPixelVal text edit
    /**
     * \returns a the value of m_textPixelVal
     */
    QTextEdit *textPixelVal();

    /// Get a pointer to the mouseCoords text edit
    /**
     * \returns a the value of m_mouseCoords
     */
    QTextEdit *mouseCoords();

    /// Set the gage font family
    void gageFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Set the gage font size
    void gageFontSize( float fs /**< [in] The new font size */ );

    /// Set the gage font color
    void gageFontColor( const QString &fc /**< [in] The new font color */ );

    /// Get the current gage font family
    /**
     * \returns the gage font family
     */
    QString gageFontFamily();

    /// Get the current gage font size
    /**
     * \returns the gage font size
     */
    float gageFontSize();

    /// Get the current gage font color
    /**
     * \returns the gage font color
     */
    QString gageFontColor();

    /// Set the FPS Gage text.
    void fpsGageText( const char *nt /**< [in] The new FPS gage text */ );

    /// Set the FPS Gage text by value.
    void fpsGageText( float nt,        ///< [in] The new FPS value
                      bool age = false ///< [in] [optional] if true then Age: is used instead of FPS:
    );

    /// Set the X Gage text.
    void textCoordX( const char *nt /**< [in] The new X gage text */ );

    /// Set the X Gage text by value.
    void textCoordX( float nv /**< [in] The new X value */ );

    /// Set the Y Gage text.
    void textCoordY( const char *nt /**< [in] The new Y gage text */ );

    /// Set the Y Gage text by value.
    void textCoordY( float nv /**< [in] The new Y value */ );

    /// Set the Value Gage text.
    void textPixelVal( const char *nt /**< [in] The new Value gage text */ );

    /// Set the Value Gage text by value.
    void textPixelVal( float nv /**< [in] The new value */ );

    /// Update and show the mouse tooltip
    void showMouseToolTip( const std::string &valStr, ///< [in] String holding the pixel value text
                           const std::string &posStr, ///< [in] String holding the pixel position text
                           const QPoint &pt           ///< [in] The current position of the pointer
    );

    /// Hide the mouse tooltip.
    void hideMouseToolTip();

    QTextEdit *zoomText();

    /// Set the Zoom font size
    void zoomFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Set the Zoom font size
    void zoomFontSize( float fs /**< [in] The new font size */ );

    /// Set the zoom font color
    void zoomFontColor( const QString &fc /**< [in] The new font color */ );

    /// Get the current zoom font color
    /**
     * \returns the zoom font color
     */
    QString zoomFontColor();

    /// Get the current zoom font family
    /**
     * \returns the zoom font family
     */
    QString zoomFontFamily();

    /// Get the current zoom font size
    /**
     * \returns the zoom font size
     */
    float zoomFontSize();

    /// Set the zoom text
    /** If a font color is supplied the zoom text is set to that color.  Otherwise (fc == 0), then _zoomFontColor is
     * used.
     */
    void zoomText( const char *nt /**< [in] the new zoom text */ );

    /// Set the User Item default line color
    /** This does not change the color of the displayed items
     *
     */
    void userItemDefColor( const QString &fc /**< [in] The new line color */ );

    /// Get the current User Item default line color
    /**
     * \returns the User Item default line color
     */
    QString userItemDefColor();

    /// Set the User Item line color
    /** This changes the color of the displayed items
     *
     */
    void userItemColor( const QString &fc /**< [in] The new line color */ );

    /// Get the current User Item line color
    /**
     * \returns the User Item line color
     */
    QString userItemColor();

    /// Set the User Item font family
    void userItemFontFamily( const QString &ff /**< [in] The new font family */ );

    /// Get the current User Item  font family
    /**
     * \returns the User Item font family
     */
    QString userItemFontFamily();

    /// Get a pointer to the User Item Size text edit.
    /**
     * \returns m_userItemSize
     */
    QTextEdit *userItemSize();

    /// Set the User Item Size font size
    void userItemSizeFontSize( float fs /**< [in] The new font size */ );

    /// Get the current User Item Size font size
    /**
     * \returns the User Item Size font size
     */
    float userItemSizeFontSize();

    /// Set the User Item Size text
    /**
     */
    void userItemSizeText( const char *nt /**< [in] the new User Item Size text */ );

    /// Set the User Item Size text and move it
    /**
     */
    void userItemSizeText( const char *nt,   ///< [in] the new User Item Size text
                           const QPoint &pos ///< [in] the position of the User Item Size text
    );

    /// Set the User Item Size text and resize and move it
    /**
     */
    void userItemSizeText( const char *nt,  ///< [in] the new User Item Size text
                           const QRect &pos ///< [in] the position of the User Item Size text
    );

    /// Get a pointer to the User Item Mouse Coords text edit.
    /**
     * \returns m_userItemMouseCoords
     */
    QTextEdit *userItemMouseCoords();

    /// Set the User Item Mouse Coords font size
    void userItemMouseCoordsFontSize( float fs /**< [in] The new font size */ );

    /// Get the current User Item Mouse Coords font size
    /**
     * \returns the User Item Mouse Coords font size
     */
    float userItemMouseCoordsFontSize();

    /// Set the User Item Mouse Coords text
    /**
     */
    void userItemMouseCoordsText( const char *nt /**< [in] the new User Item Mouse Coords text */ );

    /// Set the User Item Mouse Coords text and move it
    /**
     */
    void userItemMouseCoordsText( const char *nt,   ///< [in] the new User Item Mouse Coords text
                                  const QPoint &pos ///< [in] the position of the User Item Mouse Coords text
    );

    /// Set the User Item Mouse Coords text and resize and move it
    /**
     */
    void userItemMouseCoordsText( const char *nt,  ///< [in] the new User Item Mouse Coords text
                                  const QRect &pos ///< [in] the position of the User Item Mouse Coords text
    );

    ///@}

    /** \name Mouse Interaction
     *
     * A middle click sets the center coordinate, which is used to center the scene in the viewport.
     *
     * Viewport coordinates of the mouse are reported any time it moves.
     *
     * @{
     */

    //************************************
    // Mouse interactions
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

    /// Get the requested x center
    /**
     * \returns m_xCen
     */
    float xCen();

    /// Get the requested y center
    /**
     * \returns m_yCen
     */
    float yCen();

  protected:
    float m_mouseViewX; ///< The current x-coordinate of the mouse in viewport coords
    float m_mouseViewY; ///< The current y-coordinate of the mouse in viewport coords

  public:
    /// Get the current mouse x coordinate
    /**
     * \returns the current mouse x coordinate
     */
    float mouseViewX();

    /// Get the current mouse y coordinate
    /**
     * \returns the current mouse y coordinate
     */
    float mouseViewY();

  protected:
    float m_screenZoom; ///< The current screen zoom factor, the ratio of screen pixels to physical pixels when
                        ///< ZoomLevel==1.

  public:
    /// Set the current screen zoom
    void screenZoom( float sz /**< [in] the new screen zoom*/ );

    /// Get the current screen zoom
    /**
     * \returns the current screen zoom
     */
    float screenZoom();

  signals:
    void centerChanged();
    void mouseCoordsChanged();
    void leftPressed( QPointF mp );
    void leftClicked( QPointF mp );
    void rightPressed( QPointF mp );
    void rightClicked( QPointF mp );
    void wheelMoved( int delta );

  protected slots:
    void resizeEvent( QResizeEvent * );

  protected:
    bool viewportEvent( QEvent *e )
    {
        /*if(e->type() == QEvent::TouchBegin || e->type() == QEvent::TouchUpdate || e->type() == QEvent::TouchEnd)
        {
          std::cerr << "touch event\n";
        }*/

        return QGraphicsView::viewportEvent( e );
    }

    void mouseMoveEvent( QMouseEvent *e );
    void leaveEvent( QEvent *e );

    void mousePressEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );
    void mouseDoubleClickEvent( QMouseEvent *e );

    void wheelEvent( QWheelEvent *e );

    ///@}

  public:
    void setStyleSheet( const QString &qss )
    {

        QWidget::setStyleSheet( qss );
    }
};

#endif // rtimv_rtimvGraphicsView_hpp
