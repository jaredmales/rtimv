/** \file rtimvClientBase_reconnect_test.cpp
 * \brief Exercise connection RPC responsiveness, cancellation, and retry against a local gRPC service.
 */

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#include "rtimvClientBase.hpp"

namespace
{

/// Fail with a useful diagnostic even when the test is compiled with NDEBUG.
void require( bool condition,     /**< [in] condition required for the test to pass */
              const char *message /**< [in] failure diagnostic */
)
{
    if( !condition )
    {
        throw std::runtime_error( message );
    }
}

/// Process Qt events until a condition is satisfied or a bounded wait expires.
template <typename Predicate>
bool waitUntil( Predicate condition, /**< [in] condition checked on the Qt thread */
                int timeoutMs = 1500 /**< [in] maximum wait in milliseconds */
)
{
    if( condition() )
    {
        return true;
    }

    QEventLoop loop;
    QTimer poll;
    QTimer timeout;
    bool satisfied = false;
    QObject::connect( &poll,
                      &QTimer::timeout,
                      &loop,
                      [&]()
                      {
                          if( condition() )
                          {
                              satisfied = true;
                              loop.quit();
                          }
                      } );
    QObject::connect( &timeout, &QTimer::timeout, &loop, &QEventLoop::quit );
    poll.start( 5 );
    timeout.start( timeoutMs );
    const auto start = std::chrono::steady_clock::now();
    loop.exec();
    return satisfied && std::chrono::steady_clock::now() - start < std::chrono::milliseconds( timeoutMs + 100 );
}

/// Verify that an ordinary Qt quit event runs while the server is withholding replies.
void checkQtResponsiveness()
{
    QEventLoop loop;
    QTimer::singleShot( 100, &loop, &QEventLoop::quit );
    const auto start = std::chrono::steady_clock::now();
    loop.exec();
    require( std::chrono::steady_clock::now() - start < std::chrono::seconds( 1 ),
             "Qt quit event was blocked by a connection RPC" );
}

/// Minimal client adapter that leaves connection behavior intact and disables unrelated image polling.
class TestClient : public rtimvClientBase
{
  public:
    /// Expose the cached image name for configuration-result checks.
    using rtimvClientBase::imageName;

    /// Configure the client for the local test service.
    explicit TestClient( int port /**< [in] local service port */ );

    /// Complete any deliberately withheld image callbacks before base teardown.
    ~TestClient();

    /// Mark an established session disconnected to exercise reconnect without reconfiguration.
    void disconnectForTest();

    /// Start another image-name refresh to test request deduplication.
    void refreshNamesForTest();

    /// Read the client identifier applied by the Qt configuration completion.
    std::string clientIdForTest() const;

    /// Check whether a completed connection RPC is waiting for its queued Qt delivery.
    bool connectionCompletionQueuedForTest();

    /// Enable image polling while retaining callbacks for generation-order regression coverage.
    void deferImagesForTest();

    /// Get the number of image callbacks deliberately retained by the test.
    size_t deferredImageCountForTest() const;

    /// Complete the oldest retained image request with an unavailable-server error.
    void failOldestImageForTest();

  protected:
    /// Retain the dispatched image request until the test explicitly completes it.
    void dispatchImagePleaseAsync( uint64_t requestId,                      /**< [in] dispatched request identifier */
                                   std::shared_ptr<imageRequestState> state /**< [in] request ownership */
                                   ) override;

    /// Ignore GUI colormode work in the test adapter.
    void mtxL_postColormode( rtimv::colormode mode,  /**< [in] requested colormode */
                             const sharedLockT &lock /**< [in] held image lock */
                             ) override;

    /// Ignore GUI recolor work in the test adapter.
    void mtxL_postRecolor( const uniqueLockT &lock /**< [in] held exclusive image lock */ ) override;

    /// Ignore GUI recolor work in the test adapter.
    void mtxL_postRecolor( const sharedLockT &lock /**< [in] held shared image lock */ ) override;

    /// Ignore GUI image updates in the test adapter.
    void mtxL_postChangeImdata( const sharedLockT &lock /**< [in] held image lock */ ) override;

    /// Ignore GUI zoom work in the test adapter.
    void post_zoomLevel() override;

  private:
    /// Image requests deliberately kept alive while a new connection generation starts.
    std::vector<std::pair<uint64_t, std::shared_ptr<imageRequestState>>> m_deferredImages;
};

TestClient::TestClient( int port )
{
    m_foundation->m_connectionTimer.stop();
    QObject::disconnect( m_foundation, SIGNAL( ImageNeeded() ), m_foundation, SLOT( ImagePlease() ) );
    m_server = "127.0.0.1";
    m_port = port;
    setupConfig();
    config.nonOptions.push_back( "requested-image" );
    loadConfig();
}

TestClient::~TestClient()
{
    m_shuttingDown = true;
    for( const auto &[requestId, state] : m_deferredImages )
    {
        ImagePlease_callback( requestId, state, grpc::Status( grpc::StatusCode::CANCELLED, "test teardown" ) );
    }
}

void TestClient::disconnectForTest()
{
    uniqueLockT lock( m_connectedMutex );
    m_connected = false;
}

void TestClient::refreshNamesForTest()
{
    updateImageNamesFromServer();
}

std::string TestClient::clientIdForTest() const
{
    return m_clientId;
}

bool TestClient::connectionCompletionQueuedForTest()
{
    std::lock_guard<std::mutex> lock( m_connectionRequestMutex );
    return m_pendingConnectionRequest && !m_connectionRpcPending;
}

void TestClient::deferImagesForTest()
{
    QObject::connect( m_foundation, SIGNAL( ImageNeeded() ), m_foundation, SLOT( ImagePlease() ) );
}

size_t TestClient::deferredImageCountForTest() const
{
    return m_deferredImages.size();
}

void TestClient::failOldestImageForTest()
{
    require( !m_deferredImages.empty(), "no retained image callback to complete" );
    auto [requestId, state] = m_deferredImages.front();
    m_deferredImages.erase( m_deferredImages.begin() );
    ImagePlease_callback( requestId, state, grpc::Status( grpc::StatusCode::UNAVAILABLE, "old connection failed" ) );
}

void TestClient::dispatchImagePleaseAsync( uint64_t requestId, std::shared_ptr<imageRequestState> state )
{
    m_deferredImages.emplace_back( requestId, std::move( state ) );
}

void TestClient::mtxL_postColormode( rtimv::colormode mode, const sharedLockT &lock )
{
    static_cast<void>( mode );
    static_cast<void>( lock );
}

void TestClient::mtxL_postRecolor( const uniqueLockT &lock )
{
    static_cast<void>( lock );
}

void TestClient::mtxL_postRecolor( const sharedLockT &lock )
{
    static_cast<void>( lock );
}

void TestClient::mtxL_postChangeImdata( const sharedLockT &lock )
{
    static_cast<void>( lock );
}

void TestClient::post_zoomLevel()
{
}

/// Controllable service that withholds selected replies until release, cancellation, or deadline expiry.
class TestService : public remote_rtimv::rtimv::Service
{
  public:
    /// Whether the server already has an image session.
    std::atomic<bool> m_configured{ false };

    /// Whether unconfigured GetImageName probes should remain pending.
    std::atomic<bool> m_stallProbe{ false };

    /// Whether Configure should remain pending.
    std::atomic<bool> m_stallConfiguration{ false };

    /// Whether configured image-name lookups should remain pending.
    std::atomic<bool> m_stallNames{ false };

    /// Configuration failures still to return before accepting configuration.
    std::atomic<int> m_configurationFailures{ 0 };

    /// Number of Configure calls observed by the server.
    std::atomic<int> m_configurationCalls{ 0 };

    /// Number of name requests observed for each image slot, including reconnect probes.
    std::array<std::atomic<int>, 4> m_nameCalls{};

    /// Number of stalled calls canceled by the client or its deadline.
    std::atomic<int> m_cancelledCalls{ 0 };

    /// Whether any connection RPC omitted its deadline.
    std::atomic<bool> m_missingDeadline{ false };

    /// Return a configured name, request configuration, or hold the reply for cancellation coverage.
    grpc::Status GetImageName( grpc::ServerContext *context,                  /**< [in] server-side RPC context */
                               const remote_rtimv::ImageNameRequest *request, /**< [in] requested image slot */
                               remote_rtimv::ImageNameResponse *response      /**< [out] configured image name */
                               ) override;

    /// Accept configuration, inject a failure, or hold the reply for cancellation coverage.
    grpc::Status Configure( grpc::ServerContext *context,        /**< [in] server-side RPC context */
                            const remote_rtimv::Config *request, /**< [in] client configuration */
                            remote_rtimv::ConfigResult *response /**< [out] accepted session identifier */
                            ) override;

    /// Keep an established test session alive if a retry timer races the successful completion.
    grpc::Status Ping( grpc::ServerContext *context,             /**< [in] server-side RPC context */
                       const remote_rtimv::PingRequest *request, /**< [in] liveness request */
                       remote_rtimv::PingResponse *response      /**< [out] liveness response */
                       ) override;

  private:
    /// Wait for the selected stall to end while honoring client cancellation.
    bool waitForRelease( grpc::ServerContext *context,  /**< [in] context used to observe cancellation */
                         const std::atomic<bool> &stall /**< [in] selected server stall flag */
    );
};

bool TestService::waitForRelease( grpc::ServerContext *context, const std::atomic<bool> &stall )
{
    if( context->deadline() == std::chrono::system_clock::time_point::max() )
    {
        m_missingDeadline = true;
    }
    while( stall.load() )
    {
        if( context->IsCancelled() )
        {
            ++m_cancelledCalls;
            return false;
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    }
    return true;
}

grpc::Status TestService::GetImageName( grpc::ServerContext *context,
                                        const remote_rtimv::ImageNameRequest *request,
                                        remote_rtimv::ImageNameResponse *response )
{
    const auto index = request->image();
    if( index >= m_nameCalls.size() )
    {
        return grpc::Status( grpc::StatusCode::INVALID_ARGUMENT, "invalid image slot" );
    }
    ++m_nameCalls[index];
    if( !waitForRelease( context, m_configured ? m_stallNames : m_stallProbe ) )
    {
        return grpc::Status( grpc::StatusCode::CANCELLED, "test name request canceled" );
    }
    if( !m_configured )
    {
        return grpc::Status( grpc::StatusCode::FAILED_PRECONDITION, "test session needs configuration" );
    }
    response->set_valid( true );
    response->set_name( "server-image-" + std::to_string( index ) );
    return grpc::Status::OK;
}

grpc::Status TestService::Configure( grpc::ServerContext *context,
                                     const remote_rtimv::Config *request,
                                     remote_rtimv::ConfigResult *response )
{
    static_cast<void>( request );
    ++m_configurationCalls;
    if( !waitForRelease( context, m_stallConfiguration ) )
    {
        return grpc::Status( grpc::StatusCode::CANCELLED, "test configuration canceled" );
    }
    if( m_configurationFailures.load() > 0 )
    {
        --m_configurationFailures;
        return grpc::Status( grpc::StatusCode::UNAVAILABLE, "test configuration failure" );
    }
    m_configured = true;
    response->set_client_id( "test-client" );
    return grpc::Status::OK;
}

grpc::Status TestService::Ping( grpc::ServerContext *context,
                                const remote_rtimv::PingRequest *request,
                                remote_rtimv::PingResponse *response )
{
    static_cast<void>( context );
    static_cast<void>( request );
    static_cast<void>( response );
    return grpc::Status::OK;
}

/// Own a local service and guarantee that any withheld server calls are canceled at teardown.
class TestServer
{
  public:
    /// Service controls and observations shared with the test thread.
    TestService m_service;

    /// Ephemeral loopback port selected by gRPC.
    int m_port{ 0 };

    /// Start the local test service.
    TestServer();

    /// Cancel outstanding calls and stop the local service.
    ~TestServer();

  private:
    /// Server ownership retained until all client test objects have been destroyed.
    std::unique_ptr<grpc::Server> m_server;
};

TestServer::TestServer()
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort( "127.0.0.1:0", grpc::InsecureServerCredentials(), &m_port );
    builder.RegisterService( &m_service );
    m_server = builder.BuildAndStart();
    require( m_server != nullptr && m_port != 0, "could not start local test service" );
}

TestServer::~TestServer()
{
    m_server->Shutdown( std::chrono::system_clock::now() );
}

/// Delete on the Qt thread after its event loop has stopped, then drain any stale queued events.
void checkShutdown( std::unique_ptr<TestClient> &client /**< [in,out] client to destroy */ )
{
    const auto start = std::chrono::steady_clock::now();
    client.reset();
    require( std::chrono::steady_clock::now() - start < std::chrono::seconds( 1 ),
             "shutdown waited for a connection deadline or Qt completion" );
    QCoreApplication::processEvents();
}

/// Verify prompt quit and cancellation while reconnect is waiting for its initial probe.
void testStalledProbe()
{
    TestServer server;
    server.m_service.m_stallProbe = true;
    auto client = std::make_unique<TestClient>( server.m_port );
    QTimer::singleShot( 0, [&]() { client->reconnect(); } );
    require( waitUntil( [&]() { return server.m_service.m_nameCalls[0] == 1; } ), "probe did not start promptly" );
    for( int n = 0; n < 20; ++n )
    {
        client->reconnect();
    }
    checkQtResponsiveness();
    require( server.m_service.m_nameCalls[0] == 1, "repeated reconnect dispatched overlapping probes" );
    require( !server.m_service.m_missingDeadline, "reconnect probe omitted its deadline" );
    checkShutdown( client );
    require( waitUntil( [&]() { return server.m_service.m_cancelledCalls == 1; } ), "shutdown did not cancel probe" );
}

/// Verify prompt quit and cancellation after the probe advances to configuration.
void testStalledConfiguration()
{
    TestServer server;
    server.m_service.m_stallConfiguration = true;
    auto client = std::make_unique<TestClient>( server.m_port );
    QTimer::singleShot( 0, [&]() { client->reconnect(); } );
    require( waitUntil( [&]() { return server.m_service.m_configurationCalls == 1; } ),
             "configuration did not start promptly" );
    for( int n = 0; n < 20; ++n )
    {
        client->reconnect();
        client->Configure();
    }
    checkQtResponsiveness();
    require( server.m_service.m_configurationCalls == 1, "repeated connection attempts overlapped configuration" );
    require( server.m_service.m_nameCalls[0] == 1, "reconnect dispatched a probe while configuration was pending" );
    require( !server.m_service.m_missingDeadline, "configuration omitted its deadline" );
    checkShutdown( client );
    require( waitUntil( [&]() { return server.m_service.m_cancelledCalls == 1; } ),
             "shutdown did not cancel configuration" );
}

/// Verify that successful configuration does not block on the subsequent name refresh.
void testStalledImageNames()
{
    TestServer server;
    server.m_service.m_stallNames = true;
    auto client = std::make_unique<TestClient>( server.m_port );
    QTimer::singleShot( 0, [&]() { client->reconnect(); } );
    require( waitUntil( [&]() { return server.m_service.m_configured && server.m_service.m_nameCalls[0] == 2; } ),
             "post-configuration name refresh did not start promptly" );
    for( int n = 0; n < 20; ++n )
    {
        client->refreshNamesForTest();
    }
    checkQtResponsiveness();
    require( server.m_service.m_nameCalls[0] == 2, "main-image refresh requests were not deduplicated" );
    require( server.m_service.m_nameCalls[1] == 0, "name refresh did not retain a single outstanding request" );
    require( !server.m_service.m_missingDeadline, "image-name refresh omitted its deadline" );
    checkShutdown( client );
    require( waitUntil( [&]() { return server.m_service.m_cancelledCalls == 1; } ),
             "shutdown did not cancel image-name refresh" );
}

/// Verify shutdown after transport completion without delivering the queued Qt handler first.
void testUndeliveredCompletion()
{
    TestServer server;
    server.m_service.m_configured = true;
    auto client = std::make_unique<TestClient>( server.m_port );
    client->reconnect();
    const auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds( 1 );
    while( !client->connectionCompletionQueuedForTest() && std::chrono::steady_clock::now() < timeout )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    }
    require( client->connectionCompletionQueuedForTest(), "transport callback did not finish without Qt events" );
    for( int n = 0; n < 20; ++n )
    {
        client->reconnect();
    }
    require( server.m_service.m_nameCalls[0] == 1, "reconnect replaced a completion awaiting Qt delivery" );
    checkShutdown( client );
}

/// Verify that retained images from an older connection cannot block or invalidate a new image pipeline.
void testRetainedImageFromPreviousConnection()
{
    TestServer server;
    server.m_service.m_configured = true;
    auto client = std::make_unique<TestClient>( server.m_port );
    client->deferImagesForTest();
    client->reconnect();
    require( waitUntil( [&]() { return client->connected() && client->deferredImageCountForTest() == 1; } ),
             "initial connection did not start its image pipeline" );
    client->disconnectForTest();
    client->reconnect();
    require( waitUntil( [&]() { return client->connected() && client->deferredImageCountForTest() == 2; } ),
             "previous-generation image request blocked the reconnected image pipeline" );
    client->failOldestImageForTest();
    require( client->connected(), "previous-generation image failure disconnected the new connection" );
    checkShutdown( client );
}

/// Verify that a probe deadline releases the attempt so reconnect can later succeed.
void testProbeDeadlineRecovery()
{
    TestServer server;
    server.m_service.m_stallProbe = true;
    auto client = std::make_unique<TestClient>( server.m_port );
    client->reconnect();
    require( waitUntil( [&]() { return server.m_service.m_cancelledCalls == 1; }, 4000 ),
             "stalled reconnect probe did not expire" );
    require( !client->connected(), "failed probe marked the client connected" );
    server.m_service.m_stallProbe = false;
    server.m_service.m_configured = true;
    QTimer retries;
    QObject::connect( &retries, &QTimer::timeout, [&]() { client->reconnect(); } );
    retries.start( 10 );
    require( waitUntil( [&]() { return client->connected(); } ), "reconnect did not recover after probe deadline" );
    retries.stop();
    require( server.m_service.m_configurationCalls == 0, "existing server session was unnecessarily reconfigured" );
    checkShutdown( client );
}

/// Verify configuration failure retry, cached names, and reconnection to an existing server session.
void testConfigurationRecovery()
{
    TestServer server;
    server.m_service.m_configurationFailures = 1;
    auto client = std::make_unique<TestClient>( server.m_port );
    QTimer retries;
    QObject::connect( &retries, &QTimer::timeout, [&]() { client->reconnect(); } );
    retries.start( 10 );
    require( waitUntil( [&]() { return client->connected() && client->imageName( 3 ) == "server-image-3"; } ),
             "configuration did not retry and refresh names after failure" );
    retries.stop();
    require( server.m_service.m_configurationCalls == 2, "unexpected number of configuration attempts" );
    require( client->clientIdForTest() == "test-client", "configuration did not apply the client identifier" );
    require( client->imageName( 0 ) == "requested-image", "server refresh replaced an explicit image name" );
    require( client->imageName( 1 ) == "server-image-1", "server refresh did not populate an empty image name" );
    client->disconnectForTest();
    client->reconnect();
    require( waitUntil( [&]() { return client->connected(); } ), "existing session failed to reconnect" );
    require( server.m_service.m_configurationCalls == 2, "reconnect repeated server configuration" );
    checkShutdown( client );
}

} // namespace

/// Run local-service regressions under a real Qt event dispatcher.
int main( int argc,   /**< [in] command-line argument count */
          char **argv /**< [in] command-line arguments */
)
{
    QCoreApplication app( argc, argv );
    static_cast<void>( app );
    try
    {
        testStalledProbe();
        testStalledConfiguration();
        testStalledImageNames();
        testUndeliveredCompletion();
        testRetainedImageFromPreviousConnection();
        testProbeDeadlineRecovery();
        testConfigurationRecovery();
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
