/** \file rtimvMaskStats_gui_test.cpp
 * \brief Check independent mask-statistics controls and readouts with real FITS images.
 */

#include <QApplication>
#include <QCheckBox>
#include <QEventLoop>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextLayout>

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

/// Verify configured startup state through the real viewer and, when applicable, its remote backend.
void checkStartup( std::vector<std::string> args, /**< [in] viewer arguments */
                   bool enabled                   /**< [in] expected mask-statistics mode */
)
{
    std::vector<char *> pointers;
    for( auto &arg : args )
        pointers.push_back( arg.data() );
    pointers.push_back( nullptr );
    rtimvMainWindow window( static_cast<int>( args.size() ), pointers.data() );
    window.show();
    require( waitUntil(
                 [&]()
                 {
                     const auto stats = window.maskStats();
                     return window.connected() && window.nx() == 32 && window.ny() == 32 &&
                            stats.m_enabled == enabled && !window.maskStatsPending() &&
                            ( !enabled || stats.m_status == rtimv::maskStatsStatus::valid );
                 } ),
             "configured mask-statistics startup state did not settle" );
    window.launchControlPanel();
    auto *check = window.findChild<QCheckBox *>( "maskStatsCheck" );
    auto *view = window.findChild<rtimvGraphicsView *>( "graphicsView" );
    require( check && check->isChecked() == enabled && window.maskStatsRequested() == enabled,
             "startup checkbox did not reflect configured mode" );
    require( view && view->maskStatsText()->isVisible() == enabled, "startup overlay visibility is incorrect" );
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
            "mask-stats-test", "--image.key=" + imagePath, "--mask.key=" + maskPath, "--mzmq.always=false" };
        const bool startupTests = qEnvironmentVariableIsSet( "RTIMV_TEST_MASK_STARTUP" );
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
            serverArgs.push_back( "--mask.stats=true" );
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
            configFile.write( "[mask]\nstats=true\n" );
            configFile.close();
            auto configuredArgs = args;
            configuredArgs.push_back( "-c" );
            configuredArgs.push_back( configPath.toStdString() );
            checkStartup( configuredArgs, true );

            auto overrideArgs = configuredArgs;
            overrideArgs.push_back( "--mask.stats=false" );
            checkStartup( overrideArgs, false );

            require( configFile.open( QIODevice::WriteOnly | QIODevice::Truncate ),
                     "could not replace startup configuration" );
            configFile.write( "[mask]\nstats=false\n" );
            configFile.close();
            checkStartup( configuredArgs, false );
            configuredArgs.push_back( "--mask.stats=true" );
            checkStartup( configuredArgs, true );
#ifdef RTIMV_GRPC
            checkStartup( args, true );
#endif
        }
        else
        {
            std::vector<char *> pointers;
            for( auto &arg : args )
                pointers.push_back( arg.data() );
            pointers.push_back( nullptr );
            rtimvMainWindow window( static_cast<int>( args.size() ), pointers.data() );
            window.resize( 640, 640 );
            window.show();
            require( waitUntil( [&]() { return window.connected() && window.nx() == 32 && window.ny() == 32; } ),
                     "viewer did not receive the FITS image" );
            auto *view = window.findChild<rtimvGraphicsView *>( "graphicsView" );
            require( view, "graphics view missing" );
            window.launchControlPanel();
            auto *check = window.findChild<QCheckBox *>( "maskStatsCheck" );
            require( check, "mask-statistics checkbox missing" );
            require( !check->isChecked() && !window.maskStatsRequested(), "mask stats should default off" );
            check->setChecked( true );
            require( waitUntil(
                         [&]()
                         {
                             return window.maskStats().m_status == rtimv::maskStatsStatus::valid &&
                                    !window.maskStatsPending();
                         } ),
                     "static image did not publish mask stats" );
            window.updateMaskStatsOverlay();
            require( view->maskStatsText()->isVisible() && view->maskStatsText()->toPlainText().contains( "median:" ),
                     "mask-statistics readout missing" );
            require( view->maskStatsText()->toPlainText().startsWith( "\nmin:" ) &&
                         view->maskStatsText()->document()->blockCount() == 5,
                     "mask heading was not replaced by one blank line" );
            const auto firstBlock = view->maskStatsText()->document()->firstBlock();
            require( firstBlock.layout()->boundingRect().height() ==
                         firstBlock.next().layout()->boundingRect().height(),
                     "blank heading line lost its original height" );
            const auto initial = window.maskStats();
            require( initial.m_count > 0 && initial.m_count < pixels.size(), "circular mask was not selected" );

            view->mouseCoords()->setPlainText( "pointer coordinates" );
            view->mouseCoords()->setGeometry( 6, 6, 200, 36 );
            view->mouseCoords()->show();
            window.updateMaskStatsOverlay();
            require( !view->maskStatsText()->geometry().intersects( view->mouseCoords()->geometry() ),
                     "pointer coordinates overlap the mask readout" );

            window.toggleStatsBox();
            require( waitUntil( [&]() { return view->statsBoxText()->isVisible(); } ), "box readout missing" );
            window.updateMaskStatsOverlay();
            require( !view->maskStatsText()->geometry().intersects( view->statsBoxText()->geometry() ),
                     "mask and box readouts overlap" );
            window.zoomLevel( 2 );
            window.resize( 420, 420 );
            QCoreApplication::processEvents();
            window.updateMaskStatsOverlay();
            require( view->viewport()->rect().contains( view->maskStatsText()->geometry() ),
                     "mask readout left viewport" );
            require( !view->maskStatsText()->geometry().intersects( view->statsBoxText()->geometry() ),
                     "readouts overlap after zoom/resize" );
            window.doHideStatsBox();
            require( view->maskStatsText()->isVisible() && window.maskStatsRequested(), "box toggle hid mask stats" );
            window.applyMask( false );
            require( waitUntil( [&]() { return !window.applyMask(); } ), "mask application toggle did not settle" );
            require( window.maskStats().m_count == initial.m_count && window.maskStats().m_mean == initial.m_mean,
                     "unfiltered mask application changed selected measurements" );
            window.resize( 640, 640 );
            window.zoomLevel( 1 );
            window.toggleStatsBox();
            QCoreApplication::processEvents();
            window.updateMaskStatsOverlay();
            const QString screenshot = qEnvironmentVariable( "RTIMV_TEST_SCREENSHOT" );
            if( !screenshot.isEmpty() )
                window.grab().save( screenshot );
            check->setChecked( false );
            require( waitUntil( [&]() { return !window.maskStats().m_enabled && !window.maskStatsPending(); } ),
                     "disable did not settle" );
            require( !view->maskStatsText()->isVisible() && view->statsBoxText()->isVisible(),
                     "disabling mask stats affected box statistics" );
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
