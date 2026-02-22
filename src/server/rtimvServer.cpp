/** \file rtimvServer.cpp
 * \brief Definitions for the rtimvServer class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include "rtimvServer.hpp"

// The boilerplate preparation for responding to an rpc
#define PREPARE_RPC_REACTOR                                                                                            \
    ServerUnaryReactor *reactor = context->DefaultReactor();                                                           \
                                                                                                                       \
    /* We need a shared lock b/c we access the thread */                                                               \
    sharedLockT slock( m_clientMutex );                                                                                \
                                                                                                                       \
    if( m_clients.count( context->peer() ) == 0 )                                                                      \
    {                                                                                                                  \
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );                                \
        return reactor;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    rtimvServerThread *imageTh = m_clients[context->peer()];                                                           \
                                                                                                                       \
    if( imageTh == nullptr ) /* Something has gone wrong. Here we expect the client to reconnect */                    \
    {                                                                                                                  \
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );                                     \
        return reactor;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    imageTh->lastRequest( -1 ); /* sets to now */                                                                      \
                                                                                                                       \
    if( imageTh->asleep() )                                                                                            \
    {                                                                                                                  \
        imageTh->emit_awaken();                                                                                        \
    }

rtimvServer::rtimvServer( int argc, char **argv, QObject *Parent ) : QObject( Parent )
{
    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.

    setup( argc, argv );

    if( doHelp )
    {
        help();
        exit( 0 );
    }

    connect( this, SIGNAL( gotConfigure( const configSpec * ) ), this, SLOT( doConfigure( const configSpec * ) ) );

    m_serverThread = QThread::create( [this]() { startServer(); } );
    if( m_serverThread )
    {
        m_serverThread->start();
    }
}

rtimvServer::~rtimvServer()
{
}

void rtimvServer::setupConfig()
{
}

void rtimvServer::loadConfig()
{
}

void rtimvServer::startServer()
{
    int port = 7000;

    std::string server_address = std::format( "0.0.0.0:{}", port );

    grpc::EnableDefaultHealthCheckService( true );

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort( server_address, grpc::InsecureServerCredentials() );

    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService( this );

    // Finally assemble the server.
    std::unique_ptr<Server> server( builder.BuildAndStart() );
    std::cout << "Server listening on " << server_address << '\n';

    while( 1 )
    {
        // We need a shared lock b/c we're iterating and accessing the threads
        sharedLockT slock( m_clientMutex );

        auto client = m_clients.begin();
        while( client != m_clients.end() )
        {
            auto imageTh = client->second;

            if( imageTh == nullptr )
            {
                std::cerr << "got nullptr\n";

                // get key, give up shared mutex, get exclusive lock, then delete using key lookup.

                std::string key = client->first;

                slock.unlock();

                uniqueLockT ulock( m_clientMutex );

                client = m_clients.find( key );
                m_clients.erase( client );

                break; // give up mutex and start loop over
            }
            else
            {
                double slr = imageTh->sinceLastRequest();

                if( slr > m_clientDisconnect )
                {
                    // get key,
                    std::string ckey = client->first; // save this b/c it's going away

                    // give up shared mutex,
                    slock.unlock();
                    // so we can get exclusive lock,
                    uniqueLockT ulock( m_clientMutex );
                    // Now reset client in case something changed curing the re-lock
                    client = m_clients.find( ckey );

                    imageTh = client->second;

                    imageTh->quit();

                    if( !imageTh->wait( 1000 ) )
                    {
                        std::cerr << "QThread for client " << ckey << " didn't quit.  Terminating.\n";
                        imageTh->terminate();
                    }

                    imageTh->deleteLater();
                    m_clients.erase( client );

                    std::cerr << "Client " << ckey << " disconnected.\n";

                    break; // give up mutex and start loop over
                }
                else if( slr > m_clientSleep )
                {
                    if( !imageTh->asleep() )
                    {
                        imageTh->emit_gotosleep();
                        std::cerr << "Client " << client->first << " put to sleep\n";
                    }
                }
            }

            ++client;
        }

        if( slock.owns_lock() )
        {
            slock.unlock();
        }

        mx::sys::sleep( 1 );
    }
}

ServerUnaryReactor *rtimvServer::Configure( CallbackServerContext *context,
                                            const remote_rtimv::Config *config,
                                            remote_rtimv::ConfigResult *result )
{
    // We don't lock client mutex here b/c we are just checking count and don't access the values.

    if( m_clients.count( context->peer() ) > 0 )
    {
        std::cerr << "Attempt to reconfigure " << context->peer() << '\n';

        result->set_result( -2 );
    }
    else
    {
        configSpec *cspec = new configSpec( context->peer(), config );
        emit gotConfigure( cspec );

        // Now we wait for configuration to finish

        while( m_clients.count( context->peer() ) == 0 )
        {
            sleep( 1 );
        }

        // now we need a shared lock b/c we access the thread
        sharedLockT lock( m_clientMutex );

        // Now check that it wasn't deleted while we waited for lock:
        if( m_clients.count( context->peer() ) == 0 )
        {
            // thread wires crossed during configure

            ServerUnaryReactor *reactor = context->DefaultReactor();
            reactor->Finish( grpc::Status( grpc::INTERNAL, "sorry" ) );

            return reactor;
        }

        while( m_clients[context->peer()]->configured() == 0 )
        {
            sleep( 1 );
        }

        if( m_clients[context->peer()]->configured() == 1 )
        {
            result->set_result( 0 );
        }
        else
        {
            result->set_result( -1 );
        }
    }

    ServerUnaryReactor *reactor = context->DefaultReactor();
    reactor->Finish( Status::OK );

    return reactor;
}

ServerUnaryReactor *rtimvServer::SetColorbar( CallbackServerContext *context,
                                              const remote_rtimv::ColorbarRequest *request,
                                              remote_rtimv::ColorbarResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    rtimv::colorbar cb = rtimv::grpc2colorbar( request->colorbar() );

    if( cb != static_cast<rtimv::colorbar>( -1 ) )
    {
        imageTh->mtxUL_load_colorbar( cb, request->update() );
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid color bar" ) );
        return reactor;
    }

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetColormode( CallbackServerContext *context,
                                               const remote_rtimv::ColormodeRequest *request,
                                               remote_rtimv::ColormodeResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    rtimv::colormode cm = rtimv::grpc2colormode( request->colormode() );

    if( cm != static_cast<rtimv::colormode>( -1 ) )
    {
        imageTh->mtxUL_colormode( cm );

        reactor->Finish( Status::OK );
        return reactor;
    }
    else
    {
        imageTh->mtxUL_colormode( rtimv::colormode::minmaxglobal );
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid color mode" ) );
        return reactor;
    }
}

ServerUnaryReactor *rtimvServer::SetColorstretch( CallbackServerContext *context,
                                                  const remote_rtimv::ColorstretchRequest *request,
                                                  remote_rtimv::ColorstretchResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    rtimv::stretch cs = rtimv::grpc2stretch( request->colorstretch() );

    if( cs != static_cast<rtimv::stretch>( -1 ) )
    {
        imageTh->stretch( cs );
        reactor->Finish( Status::OK );
        return reactor;
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid color stretch" ) );
        return reactor;
    }
}

ServerUnaryReactor *rtimvServer::SetMinScale( CallbackServerContext *context,
                                              const remote_rtimv::ScaleRequest *request,
                                              remote_rtimv::ScaleResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->minScaleData( request->value() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetMaxScale( CallbackServerContext *context,
                                              const remote_rtimv::ScaleRequest *request,
                                              remote_rtimv::ScaleResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->maxScaleData( request->value() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetImageTimeout( CallbackServerContext *context,
                                                  const remote_rtimv::ImageTimeoutRequest *request,
                                                  remote_rtimv::ImageTimeoutResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->imageTimeout( request->timeout() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetQuality( CallbackServerContext *context,
                                             const remote_rtimv::QualityRequest *request,
                                             remote_rtimv::QualityResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->quality( request->quality() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::Restretch( CallbackServerContext *context,
                                            const remote_rtimv::RestretchRequest *request,
                                            remote_rtimv::RestretchResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( request );
    static_cast<void>( reply );

    imageTh->mtxUL_reStretch();

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetAutoscale( CallbackServerContext *context,
                                               const remote_rtimv::AutoscaleRequest *request,
                                               remote_rtimv::AutoscaleResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->mtxUL_autoScale( request->autoscale() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetSubDark( CallbackServerContext *context,
                                             const remote_rtimv::SubDarkRequest *request,
                                             remote_rtimv::SubDarkResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->subtractDark( request->subtract_dark() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetApplyMask( CallbackServerContext *context,
                                               const remote_rtimv::ApplyMaskRequest *request,
                                               remote_rtimv::ApplyMaskResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->applyMask( request->apply_mask() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetApplySatMask( CallbackServerContext *context,
                                                  const remote_rtimv::ApplySatMaskRequest *request,
                                                  remote_rtimv::ApplySatMaskResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->applySatMask( request->apply_sat_mask() );

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetHPFilter( CallbackServerContext *context,
                                              const remote_rtimv::HPFilterRequest *request,
                                              remote_rtimv::HPFilterResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    rtimv::hpFilter filter = rtimv::grpc2hpFilter( request->hp_filter() );

    if( filter != static_cast<rtimv::hpFilter>( -1 ) )
    {
        imageTh->hpFilter( filter );
        reactor->Finish( Status::OK );
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid HP filter" ) );
    }

    return reactor;
}

ServerUnaryReactor *rtimvServer::SetHPFW( CallbackServerContext *context,
                                          const remote_rtimv::FilterWidthRequest *request,
                                          remote_rtimv::FilterWidthResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->hpfFW( request->width() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetApplyHPFilter( CallbackServerContext *context,
                                                   const remote_rtimv::ApplyFilterRequest *request,
                                                   remote_rtimv::ApplyFilterResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->applyHPFilter( request->apply_filter() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetLPFilter( CallbackServerContext *context,
                                              const remote_rtimv::LPFilterRequest *request,
                                              remote_rtimv::LPFilterResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    rtimv::lpFilter filter = rtimv::grpc2lpFilter( request->lp_filter() );

    if( filter != static_cast<rtimv::lpFilter>( -1 ) )
    {
        imageTh->lpFilter( filter );
        reactor->Finish( Status::OK );
    }
    else
    {
        reactor->Finish( grpc::Status( grpc::INVALID_ARGUMENT, "invalid LP filter" ) );
    }

    return reactor;
}

ServerUnaryReactor *rtimvServer::SetLPFW( CallbackServerContext *context,
                                          const remote_rtimv::FilterWidthRequest *request,
                                          remote_rtimv::FilterWidthResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->lpfFW( request->width() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::SetApplyLPFilter( CallbackServerContext *context,
                                                   const remote_rtimv::ApplyFilterRequest *request,
                                                   remote_rtimv::ApplyFilterResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    imageTh->applyLPFilter( request->apply_filter() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::ImagePlease( CallbackServerContext *context,
                                              const remote_rtimv::ImageRequest *request,
                                              remote_rtimv::Image *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( request );

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_status( remote_rtimv::IMAGE_STATUS_NO_IMAGE );
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        reactor->Finish( Status::OK );
        return reactor;
    }

    int maxWaits;

    if( m_waitSleep <= 0 ) // prevent an infinite busy
    {
        maxWaits = 1;
    }
    else
    {
        maxWaits = m_waitTimeout / ( m_waitSleep / 1000. ) + 1;
    }

    int nwaits = 0;
    while( imageTh->newImage() == false && nwaits < maxWaits )
    {
        mx::sys::milliSleep( m_waitSleep );
        ++nwaits;
    }

    auto populateImageState = [imageTh, reply]()
    {
        reply->set_nx( imageTh->nx() );
        reply->set_ny( imageTh->ny() );
        reply->set_nz( imageTh->nz() );
        reply->set_no( imageTh->imageNo( 0 ) );

        reply->set_atime( imageTh->imageTime() );
        reply->set_fps( imageTh->fpsEst() );

        reply->set_saturated( imageTh->saturated() );

        reply->set_min_image_data( imageTh->minImageData() );
        reply->set_max_image_data( imageTh->maxImageData() );
        reply->set_min_scale_data( imageTh->minScaleData() );
        reply->set_max_scale_data( imageTh->maxScaleData() );

        reply->set_colorbar( rtimv::colorbar2grpc( imageTh->colorbar() ) );

        reply->set_colormode( rtimv::colormode2grpc( imageTh->colormode() ) );

        reply->set_colorstretch( rtimv::stretch2grpc( imageTh->stretch() ) );

        reply->set_autoscale( imageTh->autoScale() );

        reply->set_subtract_dark( imageTh->subtractDark() );
        reply->set_apply_mask( imageTh->applyMask() );
        reply->set_apply_sat_mask( imageTh->applySatMask() );

        reply->set_image_timeout( imageTh->imageTimeout() );
        reply->set_cube_dir( imageTh->cubeDir() );
        reply->set_quality( imageTh->quality() );

        reply->set_hp_filter( rtimv::hpFilter2grpc( imageTh->hpFilter() ) );
        reply->set_hpf_fw( imageTh->hpfFW() );
        reply->set_apply_hp_filter( imageTh->applyHPFilter() );

        reply->set_lp_filter( rtimv::lpFilter2grpc( imageTh->lpFilter() ) );
        reply->set_lpf_fw( imageTh->lpfFW() );
        reply->set_apply_lp_filter( imageTh->applyLPFilter() );

        reply->set_stats_box( imageTh->statsBox() );
        reply->set_stats_box_i0( imageTh->statsBox_i0() );
        reply->set_stats_box_i1( imageTh->statsBox_i1() );
        reply->set_stats_box_j0( imageTh->statsBox_j0() );
        reply->set_stats_box_j1( imageTh->statsBox_j1() );
        reply->set_stats_box_min( imageTh->statsBox_min() );
        reply->set_stats_box_max( imageTh->statsBox_max() );
        reply->set_stats_box_mean( imageTh->statsBox_mean() );
        reply->set_stats_box_median( imageTh->statsBox_median() );

        reply->set_color_box( imageTh->colormode() == rtimv::colormode::minmaxbox );
        reply->set_color_box_i0( imageTh->colorBox_i0() );
        reply->set_color_box_i1( imageTh->colorBox_i1() );
        reply->set_color_box_j0( imageTh->colorBox_j0() );
        reply->set_color_box_j1( imageTh->colorBox_j1() );
        reply->set_color_box_min( imageTh->colorBox_min() );
        reply->set_color_box_max( imageTh->colorBox_max() );
    };

    if( imageTh->newImage() == false )
    {
        reply->set_status( remote_rtimv::IMAGE_STATUS_TIMEOUT );
        populateImageState();
        std::string *im = new std::string;
        reply->set_allocated_image( im );

        imageTh->lastRequest( -1 ); // sets to now, again b/c of wait

        reactor->Finish( Status::OK );
        return reactor;
    }

    // Now we can render the latest image and send it
    std::string *im = new std::string;

    imageTh->mtxuL_render( im );

    imageTh->lastRequest( -1 ); // sets to now, again b/c of wait

    reply->set_status( remote_rtimv::IMAGE_STATUS_VALID );
    populateImageState();

    reply->set_allocated_image( im );

    reactor->Finish( Status::OK );

    return reactor;
}

ServerUnaryReactor *rtimvServer::UpdateCube( CallbackServerContext *context,
                                             const remote_rtimv::UpdateCubeRequest *request,
                                             remote_rtimv::UpdateCubeResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( request );
    static_cast<void>( reply );

    if( !imageTh->connected() )
    {
        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->updateCube();

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::CubeDir( CallbackServerContext *context,
                                          const remote_rtimv::CubeDirRequest *request,
                                          remote_rtimv::CubeDirResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    if( !imageTh->connected() )
    {
        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->cubeDir( request->dir() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::CubeFrame( CallbackServerContext *context,
                                            const remote_rtimv::CubeFrameRequest *request,
                                            remote_rtimv::CubeFrameResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    if( !imageTh->connected() )
    {
        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->cubeFrame( request->frame() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::CubeFrameDelta( CallbackServerContext *context,
                                                 const remote_rtimv::CubeFrameDeltaRequest *request,
                                                 remote_rtimv::CubeFrameDeltaResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( reply );

    if( !imageTh->connected() )
    {
        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->cubeFrameDelta( request->delta() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *
rtimvServer::GetPixel( CallbackServerContext *context, const remote_rtimv::Coord *request, remote_rtimv::Pixel *reply )
{
    PREPARE_RPC_REACTOR

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_valid( false );
        reply->set_value( 0 );
        reactor->Finish( Status::OK );
        return reactor;
    }

    uint32_t x = request->x();
    uint32_t y = request->y();

    if( x > imageTh->nx() - 1 || y > imageTh->ny() - 1 )
    {
        reply->set_valid( false );
        reply->set_value( 0 );
        reactor->Finish( Status::OK ); // maybe send something else?
        return reactor;
    }

    float val = imageTh->calPixel( x, y );

    reply->set_valid( true );
    reply->set_value( val );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::GetImageName( CallbackServerContext *context,
                                               const remote_rtimv::ImageNameRequest *request,
                                               remote_rtimv::ImageNameResponse *reply )
{
    PREPARE_RPC_REACTOR

    uint32_t n = request->image();
    if( n >= 4 )
    {
        reply->set_valid( false );
        reply->set_name( "" );
        reactor->Finish( Status::OK );
        return reactor;
    }

    reply->set_valid( true );
    reply->set_name( imageTh->imageName( n ) );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::GetImageNo( CallbackServerContext *context,
                                             const remote_rtimv::ImageNoRequest *request,
                                             remote_rtimv::ImageNoResponse *reply )
{
    PREPARE_RPC_REACTOR

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_valid( false );
        reply->set_no( 0 );
        reactor->Finish( Status::OK );
        return reactor;
    }

    uint32_t n = request->image();
    if( n >= imageTh->nz() )
    {
        reply->set_valid( false );
        reply->set_no( 0 );
        reactor->Finish( Status::OK );
        return reactor;
    }

    reply->set_valid( true );
    reply->set_no( imageTh->imageNo( n ) );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::ColorBox( CallbackServerContext *context,
                                           const remote_rtimv::Box *request,
                                           remote_rtimv::MinvalMaxval *reply )
{
    PREPARE_RPC_REACTOR

    // Check if image has been found
    if( !imageTh->connected() )
    {
        reply->set_valid( false );
        reply->set_min( 0 );
        reply->set_max( 0 );
        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->colorBox_i0( request->upper_left().x() );
    imageTh->colorBox_j0( request->upper_left().y() );

    imageTh->colorBox_i1( request->lower_right().x() );
    imageTh->colorBox_j1( request->lower_right().y() );

    imageTh->mtxUL_colormode( rtimv::colormode::minmaxbox );

    reply->set_valid( true );
    reply->set_min( imageTh->colorBox_min() );
    reply->set_max( imageTh->colorBox_max() );

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::Recolor( CallbackServerContext *context,
                                          const remote_rtimv::RecolorRequest *request,
                                          remote_rtimv::RecolorResponse *reply )
{
    PREPARE_RPC_REACTOR
    static_cast<void>( request );
    static_cast<void>( reply );

    imageTh->mtxUL_recolor();

    reactor->Finish( Status::OK ); // maybe send something else?
    return reactor;
}

ServerUnaryReactor *rtimvServer::StatsBox( CallbackServerContext *context,
                                           const remote_rtimv::Box *request,
                                           remote_rtimv::StatsValues *reply )
{
    PREPARE_RPC_REACTOR

    if( !imageTh->connected() )
    {
        reply->set_valid( false );
        reactor->Finish( Status::OK );
        return reactor;
    }

    if( request->lower_right().x() <= request->upper_left().x() ||
        request->lower_right().y() <= request->upper_left().y() )
    {
        imageTh->statsBox( false );
        reply->set_valid( true );
        reply->set_i0( imageTh->statsBox_i0() );
        reply->set_i1( imageTh->statsBox_i1() );
        reply->set_j0( imageTh->statsBox_j0() );
        reply->set_j1( imageTh->statsBox_j1() );
        reply->set_min( imageTh->statsBox_min() );
        reply->set_max( imageTh->statsBox_max() );
        reply->set_mean( imageTh->statsBox_mean() );
        reply->set_median( imageTh->statsBox_median() );

        reactor->Finish( Status::OK );
        return reactor;
    }

    imageTh->statsBox_i0( request->upper_left().x() );
    imageTh->statsBox_j0( request->upper_left().y() );
    imageTh->statsBox_i1( request->lower_right().x() );
    imageTh->statsBox_j1( request->lower_right().y() );
    imageTh->statsBox( true );
    imageTh->mtxUL_calcStatsBox();

    reply->set_valid( true );
    reply->set_i0( imageTh->statsBox_i0() );
    reply->set_i1( imageTh->statsBox_i1() );
    reply->set_j0( imageTh->statsBox_j0() );
    reply->set_j1( imageTh->statsBox_j1() );
    reply->set_min( imageTh->statsBox_min() );
    reply->set_max( imageTh->statsBox_max() );
    reply->set_mean( imageTh->statsBox_mean() );
    reply->set_median( imageTh->statsBox_median() );

    reactor->Finish( Status::OK );
    return reactor;
}

void rtimvServer::doConfigure( const configSpec *cspec )
{
    std::string uri = cspec->m_uri;

    std::shared_ptr<std::vector<std::string>> argv = std::make_shared<std::vector<std::string>>();

    argv->push_back( "rst" );

    if( cspec->m_config.file() != "" )
    {
        argv->push_back( "-c" );
        argv->push_back( cspec->m_config.file() );
    }

    if( cspec->m_config.image_key() != "" )
    {
        argv->push_back( "--image.key" );
        argv->push_back( cspec->m_config.image_key() );
    }

    if( cspec->m_config.dark_key() != "" )
    {
        argv->push_back( "--dark.key" );
        argv->push_back( cspec->m_config.dark_key() );
    }

    if( cspec->m_config.mask_key() != "" )
    {
        argv->push_back( "--mask.key" );
        argv->push_back( cspec->m_config.mask_key() );
    }

    if( cspec->m_config.sat_mask_key() != "" )
    {
        argv->push_back( "--satMask.key" );
        argv->push_back( cspec->m_config.sat_mask_key() );
    }

    if( cspec->m_config.update_timeout_set() )
    {
        argv->push_back( "--update.timeout" );
        argv->push_back( std::to_string( cspec->m_config.update_timeout() ) );
    }

    if( cspec->m_config.update_cube_fps_set() )
    {
        argv->push_back( "--update.cubeFPS" );
        argv->push_back( std::to_string( cspec->m_config.update_cube_fps() ) );
    }

    if( cspec->m_config.autoscale_set() )
    {
        if( cspec->m_config.autoscale() )
        {
            argv->push_back( "--autoscale" );
        }
    }

    if( cspec->m_config.darksub_set() )
    {
        if( cspec->m_config.darksub() )
        {
            argv->push_back( "--darksub" );
        }
    }

    if( cspec->m_config.satlevel_set() )
    {
        argv->push_back( "--satLevel" );
        argv->push_back( std::to_string( cspec->m_config.satlevel() ) );
    }

    if( cspec->m_config.mask_sat_set() )
    {
        if( cspec->m_config.mask_sat() )
        {
            argv->push_back( "--masksat" );
        }
    }

    if( cspec->m_config.mzmq_always_set() )
    {
        if( cspec->m_config.mzmq_always() )
        {
            argv->push_back( "--mzmq.always" );
        }
    }

    if( cspec->m_config.mzmq_server() != "" )
    {
        argv->push_back( "--mzmq.server" );
        argv->push_back( cspec->m_config.mzmq_server() );
    }

    if( cspec->m_config.mzmq_port_set() )
    {
        argv->push_back( "--mzmq.port" );
        argv->push_back( std::to_string( cspec->m_config.mzmq_port() ) );
    }

    delete cspec;

    { // mutex scope

        // Get a unique lock on clients to insert
        uniqueLockT ulock( m_clientMutex );

        if( m_clients.count( uri ) > 0 )
        {
            std::cerr << "error: client " << uri << " already configured\n";
            return;
        }

        rtimvServerThread *imageTh = new rtimvServerThread( uri, argv ); // takes ownership of argv

        m_clients.insert( clientT( uri, imageTh ) );

        ///\todo check errors on insert!
    }

    // Now switch to a shared lock for thread access
    sharedLockT slock( m_clientMutex );

    rtimvServerThread *imageTh = m_clients[uri]; // previous imageTh inside mutex scope

    imageTh->configure();

    if( imageTh->configured() == 1 )
    {
        imageTh->start();
        imageTh->lastRequest( -1 ); // sets to now

        std::cerr << "Client " << uri << " configured\n";
    }
    else
    {
        std::cerr << "Client " << uri << " configuration failed\n";
        ///\todo should we delete here?
    }
}
