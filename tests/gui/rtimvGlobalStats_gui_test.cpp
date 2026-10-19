/** \file rtimvGlobalStats_gui_test.cpp
 * \brief Check independent global-statistics controls and readouts with real FITS images.
 */

#include <QApplication>
#include <QCheckBox>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextLayout>

#include <algorithm>
#include <chrono>
#include <iostream>
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
};

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
        if( startupTests )
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
