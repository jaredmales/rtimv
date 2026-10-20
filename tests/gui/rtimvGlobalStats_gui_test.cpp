/** \file rtimvGlobalStats_gui_test.cpp
 * \brief Check statistics controls, viewport centering, and overlay clipping with real FITS images.
 */

#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QEnterEvent>
#include <QEventLoop>
#include <QFile>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QProcess>
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextLayout>
#include <QWheelEvent>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

#include <mx/ioutils/fits/fitsFile.hpp>

#include "rtimvMainWindow.hpp"
#include "rtimvControlPanel.hpp"

namespace
{

/// Fail with a diagnostic independently of NDEBUG.
void require( bool condition /**< [in] required condition */, const char *message /**< [in] failure description */ )
{
    if( !condition )
        throw std::runtime_error( message );
}

/// Process ordinary UI events until a bounded condition is satisfied.
template <typename Predicate>
bool waitUntil( Predicate condition /**< [in] completion condition */ )
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 8 );
    while( !condition() && std::chrono::steady_clock::now() < deadline )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
    return condition();
}

/// Viewer exposing its foundation for pixel-value regression checks.
class TestWindow : public rtimvMainWindow
{
  public:
    /// Use normal viewer construction and configuration.
    using rtimvMainWindow::rtimvMainWindow;

    /// Expose the existing notification source for observing processed pixel values.
    using RTIMV_BASE::m_foundation;

    /// Evaluate overlay opacity using a temporary image without running the image update pipeline.
    void sampleOverlay( QTextEdit *overlay, /**< [in,out] overlay whose background is updated */
                        QImage *image,      /**< [in] temporary displayed image, or null for unavailable image */
                        uint32_t width,     /**< [in] reported image width */
                        uint32_t height     /**< [in] reported image height */
    );
};

void TestWindow::sampleOverlay( QTextEdit *overlay, QImage *image, uint32_t width, uint32_t height )
{
    QImage *savedImage;
    uint32_t savedWidth;
    uint32_t savedHeight;
#ifndef RTIMV_GRPC
    std::vector<uint8_t> savedLightness;
#endif
    { // mutex scope
        uniqueLockT lock( m_calMutex );
        savedImage = m_qim;
        savedWidth = m_nx;
        savedHeight = m_ny;
        m_qim = image;
        m_nx = width;
        m_ny = height;
#ifndef RTIMV_GRPC
        savedLightness = m_lightness;
        m_lightness = { 0, 255 };
#endif
    }
    { // mutex scope
        sharedLockT lock( m_calMutex );
        mtxL_fontLuminance( overlay, lock );
    }
    { // mutex scope
        uniqueLockT lock( m_calMutex );
        m_qim = savedImage;
        m_nx = savedWidth;
        m_ny = savedHeight;
#ifndef RTIMV_GRPC
        m_lightness = std::move( savedLightness );
#endif
    }
}

/// Position a viewport overlay over a chosen floating-point scene rectangle, including extreme coordinates.
void mapOverlayToScene( rtimvGraphicsView *view, /**< [in,out] view mapping the overlay into image coordinates */
                        QTextEdit *overlay,      /**< [in] viewport overlay to map */
                        const QRectF &region     /**< [in] desired scene footprint */
)
{
    const double sx = overlay->width() / region.width();
    const double sy = overlay->height() / region.height();
    view->setTransform( QTransform( sx, 0, 0, sy, overlay->x() - region.x() * sx, overlay->y() - region.y() * sy ) );
    view->horizontalScrollBar()->setValue( 0 );
    view->verticalScrollBar()->setValue( 0 );
    const QRectF mapped = view->mapToScene( overlay->geometry() ).boundingRect();
    if( std::abs( mapped.x() - region.x() ) >= 0.001 || std::abs( mapped.y() - region.y() ) >= 0.001 )
    {
        std::cerr << "requested scene origin " << region.x() << ',' << region.y() << "; mapped " << mapped.x() << ','
                  << mapped.y() << "; transform " << view->transform().m11() << ',' << view->transform().m22() << ','
                  << view->transform().dx() << ',' << view->transform().dy() << "; scroll "
                  << view->horizontalScrollBar()->value() << ',' << view->verticalScrollBar()->value() << "; overlay "
                  << overlay->x() << ',' << overlay->y() << ',' << overlay->width() << ',' << overlay->height() << '\n';
    }
    require( std::abs( mapped.x() - region.x() ) < 0.001 && std::abs( mapped.y() - region.y() ) < 0.001,
             "test could not position the overlay in scene coordinates" );
}

/// Verify clipped opacity sampling and center-entry behavior through the actual local or remote GUI.
void checkViewportBounds( std::vector<std::string> args /**< [in] viewer arguments */ )
{
    std::vector<char *> pointers;
    for( auto &arg : args )
        pointers.push_back( arg.data() );
    pointers.push_back( nullptr );
    TestWindow window( static_cast<int>( args.size() ), pointers.data() );
    window.resize( 640, 640 );
    window.show();
    require( waitUntil( [&]() { return window.connected() && window.nx() == 32 && window.ny() == 32; } ),
             "viewer did not receive the viewport-test image" );
    auto *view = window.findChild<rtimvGraphicsView *>( "graphicsView" );
    require( view, "graphics view missing" );
    const QTransform savedTransform = view->transform();
    const QRectF savedSceneRect = view->sceneRect();
    view->setSceneRect( QRectF( -1e12, -1e12, 2e12, 2e12 ) );

    QTextEdit overlay( view );
    overlay.setGeometry( 0, 0, 20, 20 );
    overlay.setPlainText( "test" );
    overlay.show();
    QImage image( 4, 4, QImage::Format_Indexed8 );
    image.setColorTable( { qRgb( 0, 0, 0 ), qRgb( 255, 255, 255 ) } );
    image.fill( 1 );
    const QColor unchanged( 17, 34, 51, 77 );
    const QColor opaque( 0, 0, 0, 150 );
    const QColor transparent( 0, 0, 0, 0 );
    const auto checkBackground = [&]( const QRectF &region, const QColor &expected, uint32_t width, uint32_t height )
    {
        mapOverlayToScene( view, &overlay, region );
        overlay.setTextBackgroundColor( unchanged );
        window.sampleOverlay( &overlay, &image, width, height );
        require( overlay.textBackgroundColor() == expected, "overlay sampled the wrong image rectangle" );
    };

    // Entirely off-image overlays must not sample a fallback image rectangle or replace their existing background.
    for( const QRectF &region : { QRectF( -4, -4, 2, 2 ),
                                  QRectF( -3.58e9, -3.58e9, 2, 2 ),
                                  QRectF( 3.58e9, 3.58e9, 2, 2 ),
                                  QRectF( 6, 1, 2, 2 ),
                                  QRectF( 1, 6, 2, 2 ) } )
    {
        checkBackground( region, unchanged, 4, 4 );
    }

    checkBackground( QRectF( -1.5, -1.5, 2, 2 ), opaque, 4, 4 );
    checkBackground( QRectF( 3.5, 3.5, 2, 2 ), opaque, 4, 4 );
    checkBackground( QRectF( 0.1, 0.1, 0.5, 0.5 ), opaque, 1, 1 );
    checkBackground( QRectF( -1, -1, 8, 8 ), unchanged, 0, 4 );
    checkBackground( QRectF( -1, -1, 8, 8 ), unchanged, 4, 0 );
    checkBackground( QRectF( 6, 6, 2, 2 ), unchanged, 32, 32 );

    // Clipping a left-edge region must include only its dark edge pixel and exclude the bright rest of the image.
    image.fill( 1 );
    image.setPixel( 0, 0, 0 );
    checkBackground( QRectF( -1.5, -1.5, 2, 2 ), transparent, 4, 4 );
    checkBackground( QRectF( 1.1, 1.1, 0.5, 0.5 ), opaque, 4, 4 );

    mapOverlayToScene( view, &overlay, QRectF( 0, 0, 2, 2 ) );
    overlay.setTextBackgroundColor( unchanged );
    overlay.hide();
    window.sampleOverlay( &overlay, &image, 4, 4 );
    require( overlay.textBackgroundColor() == unchanged, "hidden overlay background was changed" );
    overlay.show();
    overlay.clear();
    overlay.setTextBackgroundColor( unchanged );
    window.sampleOverlay( &overlay, &image, 4, 4 );
    require( overlay.textBackgroundColor() == unchanged, "empty overlay background was changed" );
    overlay.setPlainText( "test" );
    overlay.setTextBackgroundColor( unchanged );
    window.sampleOverlay( &overlay, nullptr, 4, 4 );
    require( overlay.textBackgroundColor() == unchanged, "unavailable image changed overlay background" );
    QImage emptyImage;
    window.sampleOverlay( &overlay, &emptyImage, 4, 4 );
    require( overlay.textBackgroundColor() == unchanged, "null image changed overlay background" );
    window.sampleOverlay( nullptr, &image, 4, 4 );
    overlay.hide();

    view->setSceneRect( savedSceneRect );
    view->setTransform( savedTransform );
    window.launchControlPanel();
    auto *xEntry = window.findChild<QLineEdit *>( "xcenEntry" );
    auto *yEntry = window.findChild<QLineEdit *>( "ycenEntry" );
    require( xEntry && yEntry, "view-center entries missing" );
    const auto checkCenter = [&]( float x, float y )
    {
        require( std::abs( window.get_xcen() - x ) < 0.001 && std::abs( window.get_ycen() - y ) < 0.001,
                 "viewport center moved to the wrong scene coordinates" );
    };
    const auto edit = [&]( QLineEdit *entry, const QString &value )
    {
        entry->setText( value );
        require( QMetaObject::invokeMethod( entry, "editingFinished", Qt::DirectConnection ),
                 "could not invoke center entry editingFinished" );
    };
    window.mtxUL_setViewCen( 0.25, 0.625 );
    edit( xEntry, "12" );
    checkCenter( 12, 20 );
    edit( yEntry, "10" );
    checkCenter( 12, 10 );
    for( const QString &invalid : { QString( "invalid" ), QString( "nan" ), QString( "inf" ) } )
    {
        edit( xEntry, invalid );
        edit( yEntry, invalid );
        checkCenter( 12, 10 );
    }
    window.mtxUL_setViewCen( std::numeric_limits<float>::infinity(), 0.5 );
    window.mtxUL_setViewCen( 0.5, std::numeric_limits<float>::quiet_NaN() );
    checkCenter( 12, 10 );
    window.mtxUL_setViewCen( 2.5, -1.5 );
    checkCenter( 80, -48 );
    view->setTransform( QTransform::fromTranslate( 3.58e9, 3.58e9 ) );
    window.mtxUL_setViewCen( 0.5, 0.5 );
    checkCenter( 16, 16 );
    view->setTransform( savedTransform );
    window.mtxUL_setViewCen( 0.5, 0.5 );
}

/// Deliver a real mouse-button event through the graphics viewport's event dispatcher.
void sendMouseButton( rtimvGraphicsView *view,        /**< [in,out] view receiving the mouse event */
                      QEvent::Type type,              /**< [in] press or release event type */
                      const QPoint &position,         /**< [in] viewport position */
                      Qt::MouseButton button,         /**< [in] button being pressed or released */
                      Qt::KeyboardModifiers modifiers /**< [in] keyboard modifiers held for this event */
)
{
    QMouseEvent event( type,
                       QPointF( position ),
                       QPointF( view->viewport()->mapToGlobal( position ) ),
                       button,
                       type == QEvent::MouseButtonPress ? Qt::MouseButtons( button ) : Qt::NoButton,
                       modifiers );
    QApplication::sendEvent( view->viewport(), &event );
}

/// Deliver a key through the focused widget so ordinary Qt propagation reaches the viewer.
void sendViewerKey( int key,                         /**< [in] Qt key code */
                    Qt::KeyboardModifiers modifiers, /**< [in] keyboard modifiers held during the key press */
                    const QString &text              /**< [in] text produced by the key */
)
{
    require( QApplication::focusWidget(), "viewer has no keyboard focus" );
    QKeyEvent press( QEvent::KeyPress, key, modifiers, text );
    QApplication::sendEvent( QApplication::focusWidget(), &press );
    QKeyEvent release( QEvent::KeyRelease, key, modifiers, text );
    QApplication::sendEvent( QApplication::focusWidget(), &release );
}

/// Check the reported middle-click, zoom, Ctrl+C, and zoom sequence with an off-center mouse anchor.
void checkMouseZoomSequence( TestWindow &window,     /**< [in,out] actual local or client viewer */
                             rtimvGraphicsView *view /**< [in,out] viewer graphics surface */
)
{
    window.activateWindow();
    view->setFocus( Qt::OtherFocusReason );
    QCoreApplication::processEvents();
    require( QApplication::focusWidget() == view, "graphics view did not receive keyboard focus" );
    require( view->transformationAnchor() == QGraphicsView::AnchorUnderMouse,
             "mouse-centering test did not exercise the mouse zoom anchor" );
    window.zoomLevel( 1 );
    window.mtxUL_setViewCen( 0.5, 0.5 );
    window.borderWarningLevel( rtimv::warningLevel::warning );
    const QPoint click( view->viewport()->width() * 3 / 8, view->viewport()->height() * 5 / 8 );
    const QPointF clickedScene = view->mapToScene( click );
    const auto checkCenter = [&]( const QPointF &expected, const char *phase )
    {
        window.setBorderBox();
        QCoreApplication::processEvents();
        const QPointF saved( window.get_xcen(), window.get_ycen() );
        const QPointF displayError =
            QPointF( view->mapFromScene( expected ) ) - QRectF( view->viewport()->rect() ).center();
        if( std::abs( saved.x() - expected.x() ) >= 0.001 || std::abs( saved.y() - expected.y() ) >= 0.001 ||
            std::abs( displayError.x() ) > 1.0 || std::abs( displayError.y() ) > 1.0 )
        {
            std::cerr << "mouse/zoom sequence failed: " << phase << " expected=" << expected.x() << ',' << expected.y()
                      << " saved=" << saved.x() << ',' << saved.y() << " displayError=" << displayError.x() << ','
                      << displayError.y() << '\n';
            require( false, "zoom changed the saved or visible image center" );
        }
    };
    sendMouseButton( view, QEvent::MouseButtonPress, click, Qt::MiddleButton, Qt::NoModifier );
    sendMouseButton( view, QEvent::MouseButtonRelease, click, Qt::MiddleButton, Qt::NoModifier );
    checkCenter( clickedScene, "middle click" );

    const QPoint cursor( view->viewport()->width() * 7 / 8, view->viewport()->height() / 8 );
    const QPoint globalCursor = view->viewport()->mapToGlobal( cursor );
    QCursor::setPos( globalCursor );
    QEnterEvent enter( QPointF( cursor ), QPointF( view->mapFromGlobal( globalCursor ) ), QPointF( globalCursor ) );
    QApplication::sendEvent( view->viewport(), &enter );
    QMouseEvent move(
        QEvent::MouseMove, QPointF( cursor ), QPointF( globalCursor ), Qt::NoButton, Qt::NoButton, Qt::NoModifier );
    QApplication::sendEvent( view->viewport(), &move );
    const auto wheelZoom = [&]()
    {
        const float previousZoom = window.zoomLevel();
        QWheelEvent wheel( QPointF( cursor ),
                           QPointF( globalCursor ),
                           QPoint(),
                           QPoint( 0, 120 ),
                           Qt::NoButton,
                           Qt::NoModifier,
                           Qt::NoScrollPhase,
                           false );
        QApplication::sendEvent( view->viewport(), &wheel );
        require( std::abs( window.zoomLevel() - previousZoom * 1.02f ) < 0.0001,
                 "mouse wheel did not reach the viewer zoom handler" );
    };
    wheelZoom();
    checkCenter( clickedScene, "wheel after middle click" );
    sendViewerKey( Qt::Key_2, Qt::NoModifier, "2" );
    require( window.zoomLevel() == 2, "numeric zoom key did not reach the viewer" );
    checkCenter( clickedScene, "numeric zoom after middle click" );
    sendViewerKey( Qt::Key_C, Qt::ControlModifier, "c" );
    const QPointF imageCenter( window.nx() / 2.0, window.ny() / 2.0 );
    checkCenter( imageCenter, "Ctrl+C" );
    wheelZoom();
    checkCenter( imageCenter, "wheel after Ctrl+C" );
    sendViewerKey( Qt::Key_4, Qt::NoModifier, "4" );
    require( window.zoomLevel() == 4, "second numeric zoom key did not reach the viewer" );
    checkCenter( imageCenter, "numeric zoom after Ctrl+C" );
    window.borderWarningLevel( rtimv::warningLevel::normal );
}

/// Verify saved and visible mouse centering, including release handling and an open control panel.
void checkMouseCentering( std::vector<std::string> args /**< [in] viewer arguments */ )
{
    std::vector<char *> pointers;
    for( auto &arg : args )
        pointers.push_back( arg.data() );
    pointers.push_back( nullptr );
    TestWindow window( static_cast<int>( args.size() ), pointers.data() );
    window.resize( 640, 640 );
    window.show();
    require(
        waitUntil(
            [&]()
            { return window.connected() && window.nx() == 32 && window.ny() == 32 && !window.getPixmap()->isNull(); } ),
        "viewer did not render the mouse-centering test image" );
    auto *view = window.findChild<rtimvGraphicsView *>( "graphicsView" );
    require( view, "graphics view missing" );
    int centerChanges = 0;
    QObject receiver;
    QObject::connect( view, &rtimvGraphicsView::centerChanged, &receiver, [&]() { ++centerChanges; } );
    int failures = 0;

    for( bool controlPanel : { false, true } )
    {
        if( controlPanel )
        {
            window.launchControlPanel();
            QCoreApplication::processEvents();
        }
        for( float zoom : { 1.0f, 2.0f, 4.0f } )
        {
            for( Qt::MouseButton button : { Qt::MiddleButton, Qt::LeftButton } )
            {
                for( bool nearEdge : { false, true } )
                {
                    window.zoomLevel( zoom );
                    const float initialCenter = nearEdge ? 0.5f / zoom : 0.5f;
                    window.mtxUL_setViewCen( initialCenter, initialCenter );
                    QCoreApplication::processEvents();
                    const QPoint position =
                        nearEdge ? QPoint( view->viewport()->width() / 16, view->viewport()->height() / 16 )
                                 : QPoint( view->viewport()->width() * 3 / 8, view->viewport()->height() * 5 / 8 );
                    const QPointF clickedScene = view->mapToScene( position );
                    const Qt::KeyboardModifiers modifiers =
                        button == Qt::LeftButton ? Qt::ControlModifier : Qt::NoModifier;
                    centerChanges = 0;

                    const auto checkCentered = [&]( const char *phase )
                    {
                        const QPointF savedCenter( window.get_xcen(), window.get_ycen() );
                        const QPointF visiblePosition = view->mapFromScene( clickedScene );
                        const QPointF viewportCenter = QRectF( view->viewport()->rect() ).center();
                        const QPointF displayError = visiblePosition - viewportCenter;
                        if( std::abs( savedCenter.x() - clickedScene.x() ) >= 0.001 ||
                            std::abs( savedCenter.y() - clickedScene.y() ) >= 0.001 ||
                            std::abs( displayError.x() ) > 1.0 || std::abs( displayError.y() ) > 1.0 ||
                            centerChanges != 1 )
                        {
                            std::cerr << "mouse centering failed: zoom=" << zoom << " panel=" << controlPanel
                                      << " edge=" << nearEdge
                                      << " button=" << ( button == Qt::MiddleButton ? "middle" : "ctrl-left" )
                                      << " phase=" << phase << " clickedScene=" << clickedScene.x() << ','
                                      << clickedScene.y() << " savedCenter=" << savedCenter.x() << ','
                                      << savedCenter.y() << " displayError=" << displayError.x() << ','
                                      << displayError.y() << " centerSignals=" << centerChanges << '\n';
                            ++failures;
                        }
                    };

                    sendMouseButton( view, QEvent::MouseButtonPress, position, button, modifiers );
                    checkCentered( "press" );
                    sendMouseButton( view, QEvent::MouseButtonRelease, position, button, modifiers );
                    checkCentered( "release" );

                    if( controlPanel && zoom == 2.0f && button == Qt::MiddleButton && !nearEdge )
                    {
                        window.resize( 700, 420 );
                        QCoreApplication::processEvents();
                        checkCentered( "rectangular resize" );
                        window.zoomLevel( 4 );
                        QCoreApplication::processEvents();
                        checkCentered( "zoom after resize" );
                        window.resize( 640, 640 );
                        QCoreApplication::processEvents();
                    }

                    const QPointF savedCenter( window.get_xcen(), window.get_ycen() );
                    const QPointF savedPosition = view->mapToScene( position );
                    centerChanges = 0;
                    sendMouseButton( view, QEvent::MouseButtonPress, position, Qt::LeftButton, Qt::NoModifier );
                    sendMouseButton( view, QEvent::MouseButtonRelease, position, Qt::LeftButton, Qt::NoModifier );
                    require( centerChanges == 0 && QPointF( window.get_xcen(), window.get_ycen() ) == savedCenter &&
                                 view->mapToScene( position ) == savedPosition,
                             "ordinary left click changed the viewport center" );
                }
            }
        }
        checkMouseZoomSequence( window, view );
    }
    require( failures == 0, "mouse clicks did not center the clicked image point exactly once" );
}

/// Check known full-image or masked values for the 32x32 gradient fixture.
void checkValues( const rtimv::globalStatsState &stats, /**< [in] measured global statistics */
                  bool applyMask,                       /**< [in] whether image processing applies the mask */
                  bool useMask                          /**< [in] whether statistical selection uses the mask */
)
{
    require( stats.m_status == rtimv::globalStatsStatus::valid && stats.m_useMask == useMask,
             "global statistics have the wrong selection policy" );
    if( useMask )
    {
        require( stats.m_count == 305 && stats.m_min == 18 && stats.m_max == 46 && stats.m_mean == 32 &&
                     stats.m_median == 32,
                 "incorrect mask-selected statistics" );
    }
    else
    {
        require( stats.m_count == 1024 && stats.m_min == 0 && stats.m_max == ( applyMask ? 46 : 62 ) &&
                     stats.m_mean == ( applyMask ? 9.53125f : 31 ) && stats.m_median == ( applyMask ? 0 : 31 ),
                 "incorrect whole-image statistics" );
    }
}

/// Verify configured startup state through the real viewer and, when applicable, its remote backend.
void checkStartup( std::vector<std::string> args, /**< [in] viewer arguments */
                   bool enabled,                  /**< [in] expected global-statistics mode */
                   bool applyMask,                /**< [in] expected ordinary masking mode */
                   bool useMask                   /**< [in] expected statistical mask-selection policy */
)
{
    std::vector<char *> pointers;
    for( auto &arg : args )
        pointers.push_back( arg.data() );
    pointers.push_back( nullptr );
    TestWindow window( static_cast<int>( args.size() ), pointers.data() );
    window.show();
    require( waitUntil(
                 [&]()
                 {
                     const auto stats = window.globalStats();
                     return window.connected() && window.nx() == 32 && window.ny() == 32 &&
                            stats.m_enabled == enabled && stats.m_useMask == useMask &&
                            window.applyMask() == applyMask && !window.globalStatsPending() &&
                            ( !enabled || stats.m_status == rtimv::globalStatsStatus::valid );
                 } ),
             "configured global-statistics startup state did not settle" );
    window.launchControlPanel();
    auto *check = window.findChild<QCheckBox *>( "globalStatsCheck" );
    auto *maskCheck = window.findChild<QCheckBox *>( "globalStatsMaskCheck" );
    auto *view = window.findChild<rtimvGraphicsView *>( "graphicsView" );
    require( check && check->isChecked() == enabled && window.globalStatsRequested() == enabled,
             "startup checkbox did not reflect configured mode" );
    require( maskCheck && maskCheck->isChecked() == useMask && window.globalStatsUseMaskRequested() == useMask,
             "startup mask checkbox did not reflect configured selection" );
    require( view && view->globalStatsText()->isVisible() == enabled, "startup overlay visibility is incorrect" );

    if( enabled )
    {
        checkValues( window.globalStats(), applyMask, useMask );
    }

    bool pixelReceived = false;
    float pixel = -1;
    QObject receiver;
    QObject::connect( window.m_foundation,
                      &rtimvBaseObject::pixelValueUpdated,
                      &receiver,
                      [&]( uint32_t x, uint32_t y, float value, bool valid )
                      {
                          if( x == 31 && y == 0 && valid )
                          {
                              pixel = value;
                              pixelReceived = true;
                          }
                      } );
    window.requestPixelValue( 31, 0 );
    require( waitUntil( [&]() { return pixelReceived; } ), "could not read a pixel outside the mask" );
    require( pixel == ( applyMask ? 0 : 31 ), "configured masking did not affect the processed pixels correctly" );
}

} // namespace

/// Exercise the real GUI with local or remote processing.
int main( int argc /**< [in] argument count */, char **argv /**< [in] arguments */ )
{
    QApplication app( argc, argv );
    QTemporaryDir directory;
    QProcess serverProcess;
    int result = 0;
    try
    {
        require( directory.isValid(), "could not create FITS fixture directory" );
        std::vector<float> pixels( 32 * 32 );
        std::vector<float> mask( pixels.size(), 0 );
        for( int y = 0; y < 32; ++y )
        {
            for( int x = 0; x < 32; ++x )
            {
                pixels[y * 32 + x] = x + y;
                mask[y * 32 + x] = ( x - 16 ) * ( x - 16 ) + ( y - 16 ) * ( y - 16 ) < 100;
            }
        }
        const auto imagePath = directory.filePath( "image.fits" ).toStdString();
        const auto maskPath = directory.filePath( "mask.fits" ).toStdString();
        mx::fits::fitsFile<float> fits;
        fits.write( imagePath, pixels.data(), 32, 32, 1 );
        fits.write( maskPath, mask.data(), 32, 32, 1 );
        std::vector<std::string> args{
            "global-stats-test", "--image.key=" + imagePath, "--mask.key=" + maskPath, "--mzmq.always=false" };
        const bool startupTests = qEnvironmentVariableIsSet( "RTIMV_TEST_GLOBAL_STARTUP" );
        const bool viewportTests = qEnvironmentVariableIsSet( "RTIMV_TEST_VIEWPORT_BOUNDS" );
        const bool mouseCenterTests = qEnvironmentVariableIsSet( "RTIMV_TEST_MOUSE_CENTER" );
#ifdef RTIMV_GRPC
        require( argc > 1, "server executable path missing" );
        int port = 0;
        grpc::ServerBuilder builder;
        remote_rtimv::rtimv::Service temporaryService;
        builder.AddListeningPort( "127.0.0.1:0", grpc::InsecureServerCredentials(), &port );
        builder.RegisterService( &temporaryService );
        auto reservation = builder.BuildAndStart();
        require( reservation && port, "could not reserve test port" );
        reservation->Shutdown();
        reservation.reset();
        QStringList serverArgs{ "--server.address=127.0.0.1", "--server.port=" + QString::number( port ) };
        if( startupTests )
        {
            serverArgs.push_back( "--stats.global=true" );
            serverArgs.push_back( "--stats.mask=true" );
            serverArgs.push_back( "--mask=false" );
        }
        serverProcess.start( argv[1], serverArgs );
        require( serverProcess.waitForStarted(), "could not start rtimvServer" );
        args.push_back( "--server=127.0.0.1" );
        args.push_back( "--port=" + std::to_string( port ) );
#endif
        if( mouseCenterTests )
        {
            checkMouseCentering( args );
        }
        else if( viewportTests )
        {
            checkViewportBounds( args );
        }
        else if( startupTests )
        {
            const QString configPath = directory.filePath( "startup.conf" );
            QFile configFile( configPath );
            require( configFile.open( QIODevice::WriteOnly ), "could not create startup configuration" );
            configFile.write( "mask=false\n[stats]\nglobal=true\n" );
            configFile.close();
            auto configuredArgs = args;
            configuredArgs.push_back( "-c" );
            configuredArgs.push_back( configPath.toStdString() );
            checkStartup( configuredArgs, true, false, true );

            auto overrideArgs = configuredArgs;
            overrideArgs.push_back( "--stats.global=false" );
            overrideArgs.push_back( "--mask=true" );
            overrideArgs.push_back( "--stats.mask=false" );
            checkStartup( overrideArgs, false, true, false );

            require( configFile.open( QIODevice::WriteOnly | QIODevice::Truncate ),
                     "could not replace startup configuration" );
            configFile.write( "mask=true\n[stats]\nglobal=false\nmask=false\n" );
            configFile.close();
            checkStartup( configuredArgs, false, true, false );
            configuredArgs.push_back( "--stats.global=true" );
            configuredArgs.push_back( "--mask=false" );
            configuredArgs.push_back( "--stats.mask=true" );
            checkStartup( configuredArgs, true, false, true );
#ifdef RTIMV_GRPC
            checkStartup( args, true, false, true );
            auto policyOverride = args;
            policyOverride.push_back( "--stats.mask=false" );
            checkStartup( policyOverride, true, false, false );
            auto enabledOverride = args;
            enabledOverride.push_back( "--stats.global=false" );
            checkStartup( enabledOverride, false, false, true );
#endif
        }
        else
        {
            auto unmaskedArgs = args;
            unmaskedArgs.push_back( "--mask=false" );
            unmaskedArgs.push_back( "--stats.global=true" );
            unmaskedArgs.push_back( "--stats.mask=false" );
            checkStartup( unmaskedArgs, true, false, false );
            auto noMaskArgs = unmaskedArgs;
            noMaskArgs.erase( std::remove( noMaskArgs.begin(), noMaskArgs.end(), "--mask.key=" + maskPath ),
                              noMaskArgs.end() );
            checkStartup( noMaskArgs, true, false, false );

            std::vector<char *> pointers;
            for( auto &arg : args )
                pointers.push_back( arg.data() );
            pointers.push_back( nullptr );
            rtimvMainWindow window( static_cast<int>( args.size() ), pointers.data() );
            window.resize( 640, 640 );
            window.show();
            require( waitUntil( [&]() { return window.connected() && window.nx() == 32 && window.ny() == 32; } ),
                     "viewer did not receive the FITS image" );
            require( window.applyMask(), "supplying a mask should enable masking by default" );
            auto *view = window.findChild<rtimvGraphicsView *>( "graphicsView" );
            require( view, "graphics view missing" );
            window.launchControlPanel();
            auto *check = window.findChild<QCheckBox *>( "globalStatsCheck" );
            auto *maskCheck = window.findChild<QCheckBox *>( "globalStatsMaskCheck" );
            require( check, "global-statistics checkbox missing" );
            require( maskCheck && maskCheck->isChecked(), "mask selection should default on" );
            require( !check->isChecked() && !window.globalStatsRequested(), "global stats should default off" );
            check->setChecked( true );
            require( waitUntil(
                         [&]()
                         {
                             return window.globalStats().m_status == rtimv::globalStatsStatus::valid &&
                                    !window.globalStatsPending();
                         } ),
                     "static image did not publish global stats" );
            checkValues( window.globalStats(), true, true );
            maskCheck->setChecked( false );
            require( waitUntil( [&]() { return !window.globalStats().m_useMask && !window.globalStatsPending(); } ),
                     "Use mask did not restore whole-image statistics" );
            checkValues( window.globalStats(), true, false );
            maskCheck->setChecked( true );
            require( waitUntil( [&]() { return window.globalStats().m_useMask && !window.globalStatsPending(); } ),
                     "mask selection did not settle" );
            window.updateGlobalStatsOverlay();
            require( view->globalStatsText()->isVisible() &&
                         view->globalStatsText()->toPlainText().contains( "median:" ),
                     "global-statistics readout missing" );
            require( view->globalStatsText()->toPlainText().startsWith( "\nmin:" ) &&
                         view->globalStatsText()->document()->blockCount() == 5,
                     "readout heading was not replaced by one blank line" );
            const auto firstBlock = view->globalStatsText()->document()->firstBlock();
            require( firstBlock.layout()->boundingRect().height() ==
                         firstBlock.next().layout()->boundingRect().height(),
                     "blank heading line lost its original height" );
            const auto initial = window.globalStats();
            require( initial.m_count > 0 && initial.m_count < pixels.size(), "circular mask was not selected" );

            view->mouseCoords()->setPlainText( "pointer coordinates" );
            view->mouseCoords()->setGeometry( 6, 6, 200, 36 );
            view->mouseCoords()->show();
            window.updateGlobalStatsOverlay();
            require( !view->globalStatsText()->geometry().intersects( view->mouseCoords()->geometry() ),
                     "pointer coordinates overlap the global readout" );

            window.toggleStatsBox();
            require( waitUntil( [&]() { return view->statsBoxText()->isVisible(); } ), "box readout missing" );
            window.updateGlobalStatsOverlay();
            require( !view->globalStatsText()->geometry().intersects( view->statsBoxText()->geometry() ),
                     "global and box readouts overlap" );
            window.zoomLevel( 2 );
            window.resize( 420, 420 );
            QCoreApplication::processEvents();
            window.updateGlobalStatsOverlay();
            require( view->viewport()->rect().contains( view->globalStatsText()->geometry() ),
                     "global readout left viewport" );
            require( !view->globalStatsText()->geometry().intersects( view->statsBoxText()->geometry() ),
                     "readouts overlap after zoom/resize" );
            window.doHideStatsBox();
            require( view->globalStatsText()->isVisible() && window.globalStatsRequested(),
                     "box toggle hid global stats" );
            window.applyMask( false );
            require( waitUntil( [&]() { return !window.applyMask(); } ), "mask application toggle did not settle" );
            require( window.globalStats().m_count == initial.m_count && window.globalStats().m_mean == initial.m_mean,
                     "unfiltered mask application changed selected measurements" );
            window.resize( 640, 640 );
            window.zoomLevel( 1 );
            window.toggleStatsBox();
            QCoreApplication::processEvents();
            window.updateGlobalStatsOverlay();
            const QString screenshot = qEnvironmentVariable( "RTIMV_TEST_SCREENSHOT" );
            if( !screenshot.isEmpty() )
            {
                window.grab().save( screenshot );
                auto *panel = window.findChild<rtimvControlPanel *>();
                panel->findChild<QTabWidget *>( "tabWidget" )
                    ->setCurrentWidget( panel->findChild<QWidget *>( "tabSetup" ) );
                panel->grab().save( screenshot + "-controls.png" );
            }
            check->setChecked( false );
            require( waitUntil( [&]() { return !window.globalStats().m_enabled && !window.globalStatsPending(); } ),
                     "disable did not settle" );
            require( !view->globalStatsText()->isVisible() && view->statsBoxText()->isVisible(),
                     "disabling global stats affected box statistics" );
            maskCheck->setChecked( false );
            require( waitUntil( [&]() { return !window.globalStats().m_useMask && !window.globalStatsPending(); } ) &&
                         !window.globalStatsRequested() && !view->globalStatsText()->isVisible(),
                     "changing selection enabled the disabled readout" );
        }
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    if( serverProcess.state() != QProcess::NotRunning )
    {
        serverProcess.terminate();
        if( !serverProcess.waitForFinished( 1000 ) )
        {
            serverProcess.kill();
            serverProcess.waitForFinished( 1000 );
        }
        if( result )
            std::cerr << serverProcess.readAllStandardError().toStdString();
    }
    return result;
}
