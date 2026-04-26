/** \file rtimvServer.cpp
 * \brief Definitions for the rtimvServer class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include "rtimvServer.hpp"
#include "rtimvLog.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace
{
std::string image0OrUnknown( rtimvServerThread *imageTh )
{
    if( imageTh == nullptr )
    {
        return "unknown";
    }

    std::string im0 = imageTh->imageName( 0 );
    if( im0.empty() )
    {
        return "unknown";
    }

    return im0;
}

std::string boolString( bool value )
{
    if( value )
    {
        return "true";
    }

    return "false";
}

std::string quotedOrUnset( const std::string &value )
{
    if( value.empty() )
    {
        return "<unset>";
    }

    return "'" + value + "'";
}

void appendUniqueConfigSources( std::vector<std::string> &loadedFiles, const std::vector<std::string> &sources )
{
    for( const auto &source : sources )
    {
        if( source.empty() || source == "command line" )
        {
            continue;
        }

        if( std::find( loadedFiles.begin(), loadedFiles.end(), source ) == loadedFiles.end() )
        {
            loadedFiles.push_back( source );
        }
    }
}

std::vector<std::string> loadedConfigFiles( const mx::app::appConfigurator &config )
{
    std::vector<std::string> loadedFiles;

    for( const auto &[name, target] : config.m_targets )
    {
        static_cast<void>( name );
        appendUniqueConfigSources( loadedFiles, target.sources );
    }

    for( const auto &[name, target] : config.m_unusedConfigs )
    {
        static_cast<void>( name );
        appendUniqueConfigSources( loadedFiles, target.sources );
    }

    return loadedFiles;
}

std::string joinConfigFiles( const std::vector<std::string> &files )
{
    if( files.empty() )
    {
        return "<none>";
    }

    std::string joined;

    for( size_t n = 0; n < files.size(); ++n )
    {
        if( n > 0 )
        {
            joined += ", ";
        }

        joined += files[n];
    }

    return joined;
}

std::string configValueSource( const mx::app::appConfigurator &config, const std::string &name )
{
    auto targetIt = config.m_targets.find( name );
    if( targetIt == config.m_targets.end() || !targetIt->second.set || targetIt->second.sources.empty() )
    {
        return "default";
    }

    return targetIt->second.sources.back();
}

std::string summarizeBaseConfigOverrides( const rtimvBase::startupConfig &settings,
                                          const mx::app::appConfigurator &config )
{
    std::vector<std::string> parts;

    auto addValue = [&]( const std::string &name, const std::string &value )
    { parts.push_back( std::format( "{}={} ({})", name, value, configValueSource( config, name ) ) ); };

    if( !settings.m_imageKeys[0].empty() )
    {
        addValue( "image.key", settings.m_imageKeys[0] );
    }

    if( !settings.m_imageKeys[1].empty() )
    {
        addValue( "dark.key", settings.m_imageKeys[1] );
    }

    if( !settings.m_imageKeys[2].empty() )
    {
        addValue( "mask.key", settings.m_imageKeys[2] );
    }

    if( !settings.m_imageKeys[3].empty() )
    {
        addValue( "satMask.key", settings.m_imageKeys[3] );
    }

    if( settings.m_updateTimeoutSet )
    {
        const auto updateTimeoutTarget = config.m_targets.find( "update.timeout" );
        const bool updateTimeoutExplicit =
            updateTimeoutTarget != config.m_targets.end() && updateTimeoutTarget->second.set;
        const std::string timeoutSource = updateTimeoutExplicit ? configValueSource( config, "update.timeout" )
                                                                : configValueSource( config, "update.fps" );
        parts.push_back( std::format( "update.timeout={} ({})", settings.m_updateTimeout, timeoutSource ) );
    }

    if( settings.m_updateCubeFPSSet )
    {
        addValue( "update.cubeFPS", std::to_string( settings.m_updateCubeFPS ) );
    }

    if( settings.m_colorbarSet )
    {
        addValue( "colorbar", settings.m_colorbarName );
    }

    if( settings.m_autoscaleSet )
    {
        addValue( "autoscale", boolString( settings.m_autoscale ) );
    }

    if( settings.m_darkSubSet )
    {
        addValue( "darksub", boolString( settings.m_darkSub ) );
    }

    if( settings.m_satLevelSet )
    {
        addValue( "satLevel", std::to_string( settings.m_satLevel ) );
    }

    if( settings.m_maskSatSet )
    {
        addValue( "masksat", boolString( settings.m_maskSat ) );
    }

    if( settings.m_mzmqAlwaysSet )
    {
        addValue( "mzmq.always", boolString( settings.m_mzmqAlways ) );
    }

    if( settings.m_mzmqServerSet )
    {
        addValue( "mzmq.server", settings.m_mzmqServer );
    }

    if( settings.m_mzmqPortSet )
    {
        addValue( "mzmq.port", std::to_string( settings.m_mzmqPort ) );
    }

    if( parts.empty() )
    {
        return "<none>";
    }

    std::string summary;

    for( size_t n = 0; n < parts.size(); ++n )
    {
        if( n > 0 )
        {
            summary += ", ";
        }

        summary += parts[n];
    }

    return summary;
}

struct rpcActivityGuard
{
    rtimvServerThread *m_imageTh{ nullptr };

    rpcActivityGuard() = default;

    explicit rpcActivityGuard( rtimvServerThread *imageTh ) : m_imageTh( imageTh )
    {
    }

    ~rpcActivityGuard()
    {
        if( m_imageTh )
        {
            m_imageTh->rpcEnd();
        }
    }
};
} // namespace

// The boilerplate preparation for responding to an rpc
#define PREPARE_RPC_REACTOR                                                                                            \
    ServerUnaryReactor *reactor = context->DefaultReactor();                                                           \
                                                                                                                       \
    /* We need a shared lock b/c we access the thread */                                                               \
    sharedLockT slock( m_clientMutex );                                                                                \
                                                                                                                       \
    auto clientIt = m_clients.find( context->peer() );                                                                 \
    if( clientIt == m_clients.end() )                                                                                  \
    {                                                                                                                  \
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );                                \
        return reactor;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    rtimvServerThread *imageTh = clientIt->second;                                                                     \
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
    }                                                                                                                  \
                                                                                                                       \
    imageTh->rpcBegin();                                                                                               \
    rpcActivityGuard rpcGuard( imageTh );                                                                              \
                                                                                                                       \
    slock.unlock();

rtimvServer::rtimvServer( int argc, char **argv, QObject *Parent ) : QObject( Parent )
{
    if( argv != nullptr && argv[0] != nullptr )
    {
        m_calledName = std::filesystem::path( argv[0] ).filename().string();
    }

    m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; // Tells mx::application to look for this env var.
    config.m_sources = true;

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
    rtimvBase::setupBaseConfig( config, false );

    config.add( "server.port",
                "p",
                "server.port",
                mx::app::argType::Required,
                "server",
                "port",
                false,
                "int",
                "Port the grpc server listens on." );

    config.add( "server.address",
                "",
                "server.address",
                mx::app::argType::Required,
                "server",
                "address",
                false,
                "string",
                "Host/interface address the grpc server listens on." );

    config.add( "image.timeout",
                "",
                "image.timeout",
                mx::app::argType::Required,
                "image",
                "timeout",
                false,
                "real",
                "Time to wait for a new image to be ready before timing out, in s.  Default is 1 s." );

    config.add( "image.sleep",
                "",
                "image.sleep",
                mx::app::argType::Required,
                "image",
                "sleep",
                false,
                "int",
                "Time to sleep while waiting for a new image, in ms.  Default is 100 ms." );

    config.add( "client.sleep",
                "",
                "client.sleep",
                mx::app::argType::Required,
                "client",
                "sleep",
                false,
                "real",
                "Time in seconds after which a thread with no requests will be put to sleep.  Default is 10 s." );

    config.add( "client.disconnect",
                "",
                "client.disconnect",
                mx::app::argType::Required,
                "client",
                "disconnect",
                false,
                "real",
                "Time in seconds after which a thread with no requests will be disconnected. Default is 120 s." );

    config.add( "quality",
                "",
                "quality",
                mx::app::argType::Required,
                "",
                "quality",
                false,
                "int",
                "Default JPEG quality for served images when no per-image quality is configured." );
}

void rtimvServer::loadConfig()
{
    rtimvBase::loadBaseConfig( m_baseConfigDefaults, config );

    config( m_port, "server.port" );
    config( m_serverAddress, "server.address" );
    config( m_waitTimeout, "image.timeout" );
    config( m_waitSleep, "image.sleep" );
    config( m_clientSleep, "client.sleep" );
    config( m_clientDisconnect, "client.disconnect" );
    config( m_qualityDefault, "quality" );
    m_qualityDefault = std::clamp( m_qualityDefault, 0, 100 );

    if( m_baseConfigDefaults.m_logAppNameSet )
    {
        m_logAppName = m_baseConfigDefaults.m_logAppName;
    }

    rtimv::logContext serverCtx;
    serverCtx.calledName = m_calledName;
    serverCtx.includeAppName = m_logAppName;

    std::string configPathEnvValue;
    if( !m_configPathCLBase_env.empty() )
    {
        const char *envValue = std::getenv( m_configPathCLBase_env.c_str() );
        if( envValue != nullptr )
        {
            configPathEnvValue = envValue;
        }
    }

    const std::vector<std::string> configFiles = loadedConfigFiles( config );

    std::string logAppNameSource = configValueSource( config, "log.appname" );
    if( config.isSet( "no-log-appname" ) )
    {
        bool noLogAppName = false;
        config( noLogAppName, "no-log-appname" );
        if( noLogAppName )
        {
            logAppNameSource = configValueSource( config, "no-log-appname" );
        }
    }

    std::cout << rtimv::formatLogMessage(
                     serverCtx,
                     std::format( "config env {}={}", m_configPathCLBase_env, quotedOrUnset( configPathEnvValue ) ) )
              << '\n';

    std::cout << rtimv::formatLogMessage( serverCtx,
                                          std::format( "config paths: global={} user={} local={} command_line_base={} "
                                                       "command_line={}",
                                                       quotedOrUnset( m_configPathGlobal ),
                                                       quotedOrUnset( m_configPathUser ),
                                                       quotedOrUnset( m_configPathLocal ),
                                                       quotedOrUnset( m_configPathCLBase ),
                                                       quotedOrUnset( m_configPathCL ) ) )
              << '\n';

    std::cout << rtimv::formatLogMessage( serverCtx,
                                          std::format( "loaded config files: {}", joinConfigFiles( configFiles ) ) )
              << '\n';

    std::cout << rtimv::formatLogMessage(
                     serverCtx,
                     std::format( "effective config: server.address={} ({}) server.port={} ({}) image.timeout={} ({}) "
                                  "image.sleep={} ({})",
                                  m_serverAddress,
                                  configValueSource( config, "server.address" ),
                                  m_port,
                                  configValueSource( config, "server.port" ),
                                  m_waitTimeout,
                                  configValueSource( config, "image.timeout" ),
                                  m_waitSleep,
                                  configValueSource( config, "image.sleep" ) ) )
              << '\n';

    std::cout << rtimv::formatLogMessage(
                     serverCtx,
                     std::format( "effective config: client.sleep={} ({}) client.disconnect={} ({}) quality={} ({}) "
                                  "log.appname={} ({})",
                                  m_clientSleep,
                                  configValueSource( config, "client.sleep" ),
                                  m_clientDisconnect,
                                  configValueSource( config, "client.disconnect" ),
                                  m_qualityDefault,
                                  configValueSource( config, "quality" ),
                                  boolString( m_logAppName ),
                                  logAppNameSource ) )
              << '\n';

    std::cout << rtimv::formatLogMessage( serverCtx,
                                          std::format( "rtimvBase default overrides: {}",
                                                       summarizeBaseConfigOverrides( m_baseConfigDefaults, config ) ) )
              << '\n';
}

void rtimvServer::startServer()
{
    std::string server_address = std::format( "{}:{}", m_serverAddress, m_port );

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
    rtimv::logContext serverCtx;
    serverCtx.calledName = m_calledName;
    serverCtx.includeAppName = m_logAppName;
    serverCtx.image0 = "";
    std::cout << rtimv::formatLogMessage( serverCtx, std::format( "listening on {}", server_address ) ) << '\n';

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
                std::string ckey = client->first;
                std::cerr << rtimv::formatServerLogMessage(
                                 m_calledName, m_logAppName, ckey, "unknown", "null client thread entry" )
                          << '\n';

                // get key, give up shared mutex, get exclusive lock, then delete using key lookup.

                slock.unlock();

                uniqueLockT ulock( m_clientMutex );

                auto staleClient = m_clients.find( ckey );
                if( staleClient != m_clients.end() )
                {
                    m_clients.erase( staleClient );
                }

                break; // give up mutex and start loop over
            }
            else
            {
                imageTh->prunePendingImageRequests( m_waitTimeout );

                double slr = imageTh->sinceLastRequest();
                size_t pendingRequests = imageTh->pendingImageRequests();

                if( slr > m_clientDisconnect )
                {
                    if( imageTh->rpcActive() > 0 || pendingRequests > 0 )
                    {
                        ++client;
                        continue;
                    }

                    // get key,
                    std::string ckey = client->first; // save this b/c it's going away
                    std::string image0 = image0OrUnknown( imageTh );

                    // give up shared mutex,
                    slock.unlock();
                    // erase under exclusive lock, then do thread shutdown without holding the client lock.
                    rtimvServerThread *staleThread = nullptr;
                    {
                        uniqueLockT ulock( m_clientMutex );
                        auto staleClient = m_clients.find( ckey );
                        if( staleClient != m_clients.end() )
                        {
                            staleThread = staleClient->second;
                            m_clients.erase( staleClient );
                        }
                    }

                    if( staleThread != nullptr )
                    {
                        staleThread->quit();

                        if( !staleThread->wait( 1000 ) )
                        {
                            std::cerr << rtimv::formatServerLogMessage( m_calledName,
                                                                        m_logAppName,
                                                                        ckey,
                                                                        image0,
                                                                        "QThread didn't quit, terminating" )
                                      << '\n';
                            staleThread->terminate();
                        }

                        staleThread->deleteLater();
                    }

                    std::cout << rtimv::formatServerLogMessage(
                                     m_calledName, m_logAppName, ckey, image0, "disconnected" )
                              << '\n';

                    break; // give up mutex and start loop over
                }
                else if( slr > m_clientSleep )
                {
                    if( pendingRequests == 0 && !imageTh->asleep() )
                    {
                        imageTh->emit_gotosleep();
                        std::cout << rtimv::formatServerLogMessage( m_calledName,
                                                                    m_logAppName,
                                                                    client->first,
                                                                    image0OrUnknown( imageTh ),
                                                                    "put to sleep" )
                                  << '\n';
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
    const std::string peer = context->peer();
    const std::string reqImage0 = config ? config->image_key() : "";
    result->set_client_id( peer );

    { // mutex scope
        sharedLockT lock( m_clientMutex );
        if( m_clients.find( peer ) != m_clients.end() )
        {
            std::cerr << rtimv::formatServerLogMessage(
                             m_calledName, m_logAppName, peer, reqImage0, "attempt to reconfigure" )
                      << '\n';
            result->set_result( -2 );
        }
    }

    if( result->result() != -2 )
    {
        configSpec *cspec = new configSpec( peer, config );
        emit gotConfigure( cspec );

        // Now we wait for configuration to finish
        bool seenClient = false;
        while( true )
        {
            int configured = 0;
            bool haveClient = false;
            {
                sharedLockT lock( m_clientMutex );
                auto clientIt = m_clients.find( peer );
                if( clientIt != m_clients.end() )
                {
                    haveClient = true;
                    seenClient = true;

                    if( clientIt->second == nullptr )
                    {
                        std::cerr << rtimv::formatServerLogMessage( m_calledName,
                                                                    m_logAppName,
                                                                    peer,
                                                                    reqImage0,
                                                                    "null thread during configure wait" )
                                  << '\n';
                        configured = -1;
                    }
                    else
                    {
                        configured = clientIt->second->configured();
                    }
                }
            }

            if( !haveClient && seenClient )
            {
                ServerUnaryReactor *reactor = context->DefaultReactor();
                reactor->Finish( grpc::Status( grpc::INTERNAL, "client disappeared during configure" ) );
                return reactor;
            }

            if( configured != 0 )
            {
                result->set_result( configured == 1 ? 0 : -1 );
                break;
            }

            mx::sys::milliSleep( 50 );
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

    imageTh->setImageTimeout( request->timeout() );

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

ServerUnaryReactor *rtimvServer::Ping( CallbackServerContext *context,
                                       const remote_rtimv::PingRequest *request,
                                       remote_rtimv::PingResponse *reply )
{
    static_cast<void>( request );
    static_cast<void>( reply );

    ServerUnaryReactor *reactor = context->DefaultReactor();

    sharedLockT slock( m_clientMutex );

    auto clientIt = m_clients.find( context->peer() );
    if( clientIt == m_clients.end() )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = clientIt->second;
    if( imageTh == nullptr )
    {
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    reactor->Finish( Status::OK );
    return reactor;
}

ServerUnaryReactor *rtimvServer::ImagePlease( CallbackServerContext *context,
                                              const remote_rtimv::ImageRequest *request,
                                              remote_rtimv::Image *reply )
{
    static_cast<void>( request );

    sharedLockT slock( m_clientMutex );

    auto clientIt = m_clients.find( context->peer() );
    if( clientIt == m_clients.end() )
    {
        ServerUnaryReactor *reactor = context->DefaultReactor();
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "not configured" ) );
        return reactor;
    }

    rtimvServerThread *imageTh = clientIt->second;
    if( imageTh == nullptr )
    {
        ServerUnaryReactor *reactor = context->DefaultReactor();
        reactor->Finish( grpc::Status( grpc::FAILED_PRECONDITION, "reconnect" ) );
        return reactor;
    }

    imageTh->lastRequest( -1 );

    if( imageTh->asleep() )
    {
        imageTh->emit_awaken();
    }

    ServerUnaryReactor *reactor = imageTh->newImagePleaseReactor( reply );

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

ServerUnaryReactor *rtimvServer::GetInfo( CallbackServerContext *context,
                                          const remote_rtimv::InfoRequest *request,
                                          remote_rtimv::InfoResponse *reply )
{
    PREPARE_RPC_REACTOR

    uint32_t n = request->image();
    std::vector<std::string> info = imageTh->info( n );
    if( info.size() == 0 )
    {
        reply->set_valid( false );
    }
    else
    {
        reply->set_valid( true );
        for( size_t i = 0; i < info.size(); ++i )
        {
            reply->add_info( info[i] );
        }
    }

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
    std::string reqImage0 = cspec->m_config.image_key();

    std::shared_ptr<std::vector<std::string>> argv = std::make_shared<std::vector<std::string>>();

    argv->push_back( "rst" );

    if( cspec->m_config.file() != "" )
    {
        argv->push_back( "-c" );
        argv->push_back( cspec->m_config.file() );
    }

    rtimvBase::appendBaseConfigArgs( *argv, m_baseConfigDefaults );

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

    if( cspec->m_config.colorbar_set() )
    {
        rtimv::colorbar cb = rtimv::grpc2colorbar( cspec->m_config.colorbar() );

        if( cb != static_cast<rtimv::colorbar>( -1 ) )
        {
            argv->push_back( "--colorbar" );
            argv->push_back( std::string( rtimv::colorbarName( cb ) ) );
        }
    }

    if( cspec->m_config.autoscale_set() )
    {
        argv->push_back( cspec->m_config.autoscale() ? "--autoscale" : "--autoscale=false" );
    }

    if( cspec->m_config.darksub_set() )
    {
        argv->push_back( cspec->m_config.darksub() ? "--darksub" : "--darksub=false" );
    }

    if( cspec->m_config.satlevel_set() )
    {
        argv->push_back( "--satLevel" );
        argv->push_back( std::to_string( cspec->m_config.satlevel() ) );
    }

    if( cspec->m_config.mask_sat_set() )
    {
        argv->push_back( cspec->m_config.mask_sat() ? "--masksat" : "--masksat=false" );
    }

    if( cspec->m_config.mzmq_always_set() )
    {
        argv->push_back( cspec->m_config.mzmq_always() ? "--mzmq.always" : "--mzmq.always=false" );
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

    if( cspec->m_config.quality_set() )
    {
        argv->push_back( "--quality" );
        argv->push_back( std::to_string( cspec->m_config.quality() ) );
    }

    delete cspec;

    { // mutex scope

        // Get a unique lock on clients to insert
        uniqueLockT ulock( m_clientMutex );

        if( m_clients.count( uri ) > 0 )
        {
            std::cerr << rtimv::formatServerLogMessage(
                             m_calledName, m_logAppName, uri, reqImage0, "already configured" )
                      << '\n';
            return;
        }

        rtimvServerThread *imageTh = new rtimvServerThread( uri,
                                                            argv,
                                                            m_qualityDefault,
                                                            m_calledName,
                                                            m_logAppName ); // takes ownership of argv

        m_clients.insert( clientT( uri, imageTh ) );

        ///\todo check errors on insert!
    }

    rtimvServerThread *imageTh = nullptr;
    {
        sharedLockT slock( m_clientMutex );
        auto clientIt = m_clients.find( uri );
        if( clientIt != m_clients.end() )
        {
            imageTh = clientIt->second;
        }
    }
    if( imageTh == nullptr )
    {
        std::cerr << rtimv::formatServerLogMessage(
                         m_calledName, m_logAppName, uri, reqImage0, "missing thread after insert" )
                  << '\n';
        return;
    }

    imageTh->configure();

    if( imageTh->configured() == 1 )
    {
        imageTh->start();
        imageTh->lastRequest( -1 ); // sets to now

        std::cout << rtimv::formatServerLogMessage(
                         m_calledName, m_logAppName, uri, image0OrUnknown( imageTh ), "configured" )
                  << '\n';
    }
    else
    {
        std::cerr << rtimv::formatServerLogMessage(
                         m_calledName, m_logAppName, uri, image0OrUnknown( imageTh ), "configuration failed" )
                  << '\n';
        ///\todo should we delete here?
    }
}
