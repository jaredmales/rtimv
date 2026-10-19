/** \file rtimvClientBase_maskStats_test.cpp
 * \brief Test authoritative mask-statistics state and asynchronous mode requests.
 */

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>

#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

#include "rtimvClientBase.hpp"

namespace
{

/// Require a condition independently of NDEBUG.
void require( bool condition /**< [in] required condition */, const char *message /**< [in] failure description */ )
{
    if( !condition )
        throw std::runtime_error( message );
}

/// Process owner-checked Qt completion slots until a bounded condition is met.
template <typename Predicate>
bool waitUntil( Predicate condition /**< [in] completion condition */, int timeoutMs = 1500 /**< [in] wait bound */ )
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeoutMs );
    while( !condition() && std::chrono::steady_clock::now() < deadline )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 5 );
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    }
    return condition();
}

/// Client with normal image-state application and a real asynchronous setter transport.
class TestClient : public rtimvClientBase
{
  public:
    /// Connect directly to the local test service without unrelated image polling.
    explicit TestClient( int port /**< [in] local service port */ );

    /// Deliver a reply through the normal serial-ordered ImageReceived path.
    void deliver( remote_rtimv::Image reply /**< [in] image/state payload */ );

    /// Start a new connection generation while retaining user intent.
    void newGeneration();

    /// Simulate loss of the transport connection.
    void disconnectForTest();

    /// Check transport completion without processing its queued Qt slot.
    bool completionQueued();

  protected:
    /// Ignore GUI work in the transport test.
    void mtxL_postSetImsize( const uniqueLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the transport test.
    void mtxL_postColormode( rtimv::colormode mode /**< [in] requested mode */,
                             const sharedLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the transport test.
    void mtxL_postRecolor( const uniqueLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the transport test.
    void mtxL_postRecolor( const sharedLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the transport test.
    void mtxL_postChangeImdata( const sharedLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the transport test.
    void post_zoomLevel() override;
};

TestClient::TestClient( int port )
{
    m_foundation->m_connectionTimer.stop();
    QObject::disconnect( m_foundation, SIGNAL( ImageNeeded() ), m_foundation, SLOT( ImagePlease() ) );
    m_server = "127.0.0.1";
    m_port = port;
    setupConfig();
    config.nonOptions.push_back( "test-image" );
    loadConfig();
    m_connected = true;
}

void TestClient::deliver( remote_rtimv::Image reply )
{
    { // mutex scope
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        m_completedImageReplies.push_back( std::move( reply ) );
    }
    ImageReceived();
}

void TestClient::newGeneration()
{
    { // mutex scope
        uniqueLockT lock( m_connectedMutex );
        ++m_connections;
    }
    finishConnection();
}

void TestClient::disconnectForTest()
{
    uniqueLockT lock( m_connectedMutex );
    m_connected = false;
}

bool TestClient::completionQueued()
{
    std::lock_guard<std::mutex> lock( m_asyncRpcMutex );
    return m_pendingMaskStats && !m_maskStatsRpcPending;
}

void TestClient::mtxL_postSetImsize( const uniqueLockT &lock )
{
    static_cast<void>( lock );
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

/// Service that can delay setters and observe their ordering/cancellation.
class TestService : public remote_rtimv::rtimv::Service
{
  public:
    /// Whether setters should wait before applying the requested state.
    std::atomic<bool> m_hold{ false };

    /// Whether the setter should report an unsupported RPC.
    std::atomic<bool> m_unsupported{ false };

    /// Last applied mode on the server.
    std::atomic<bool> m_enabled{ false };

    /// Count of received setters.
    std::atomic<int> m_calls{ 0 };

    /// Count of finished setters.
    std::atomic<int> m_finished{ 0 };

    /// Count of cancellations observed by the server.
    std::atomic<int> m_cancelled{ 0 };

    /// Whether a request was missing its deadline.
    std::atomic<bool> m_missingDeadline{ false };

    /// Apply or withhold a mode request.
    grpc::Status SetMaskStats( grpc::ServerContext *context /**< [in] request context */,
                               const remote_rtimv::MaskStatsRequest *request /**< [in] requested mode */,
                               remote_rtimv::MaskStatsResponse *reply /**< [out] empty response */
                               ) override;
};

grpc::Status TestService::SetMaskStats( grpc::ServerContext *context,
                                        const remote_rtimv::MaskStatsRequest *request,
                                        remote_rtimv::MaskStatsResponse *reply )
{
    static_cast<void>( reply );
    ++m_calls;
    if( context->deadline() == std::chrono::system_clock::time_point::max() )
        m_missingDeadline = true;
    while( m_hold && !context->IsCancelled() )
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    if( context->IsCancelled() )
    {
        ++m_cancelled;
        return grpc::Status( grpc::StatusCode::CANCELLED, "cancelled" );
    }
    ++m_finished;
    if( m_unsupported.exchange( false ) )
        return grpc::Status( grpc::StatusCode::UNIMPLEMENTED, "old server" );
    m_enabled = request->enabled();
    return grpc::Status::OK;
}

/// Build a metadata-only Image reply with deterministic selected-pixel statistics.
remote_rtimv::Image makeImage( uint64_t serial /**< [in] monotonic response serial */,
                               bool enabled /**< [in] authoritative mode */ )
{
    remote_rtimv::Image reply;
    reply.set_status( remote_rtimv::IMAGE_STATUS_VALID );
    reply.set_response_serial( serial );
    reply.set_nx( 2 );
    reply.set_ny( 2 );
    reply.set_nz( 1 );
    reply.set_image_timeout( 50 );
    auto *stats = reply.mutable_mask_stats();
    stats->set_enabled( enabled );
    stats->set_status( enabled ? remote_rtimv::MASK_STATS_VALID : remote_rtimv::MASK_STATS_UNAVAILABLE );
    if( enabled )
    {
        stats->set_count( 2 );
        stats->set_min( 1 );
        stats->set_max( 3 );
        stats->set_mean( 2 );
        stats->set_median( 2 );
    }
    return reply;
}

/// Verify passive state updates, compatibility, and serial ordering without JPEG payloads.
void testState( int port /**< [in] service port */, TestService &service /**< [in] service observations */ )
{
    TestClient client( port );
    client.deliver( makeImage( 2, true ) );
    const auto stats = client.maskStats();
    require( stats.m_enabled && stats.m_count == 2 && stats.m_mean == 2 && stats.m_median == 2,
             "metadata-only Image did not apply mask stats" );
    client.deliver( makeImage( 1, false ) );
    require( client.maskStats().m_enabled, "older Image overwrote mask stats" );
    client.MaskStatsUnavailable( 0 );
    require( client.maskStats().m_status == rtimv::maskStatsStatus::noImage && client.maskStats().m_count == 0,
             "image-unavailable reply retained measurements" );
    client.newGeneration();
    client.deliver( makeImage( 2, true ) );
    client.MaskStatsUnavailable( 0 );
    require( client.maskStats().m_count == 2, "old connection invalidated a new result" );
    auto invalid = makeImage( 3, true );
    invalid.mutable_mask_stats()->set_status( remote_rtimv::MASK_STATS_NO_MASK );
    client.deliver( invalid );
    require( client.maskStats().m_status == rtimv::maskStatsStatus::noMask && client.maskStats().m_count == 0,
             "invalid reply retained measurement values" );
    auto oldServer = makeImage( 4, false );
    oldServer.clear_mask_stats();
    client.deliver( oldServer );
    require( client.maskStats().m_status == rtimv::maskStatsStatus::unsupported, "old server was not detected" );
    client.mtxUL_maskStats( true );
    require( service.m_calls == 0, "unsupported server received mode RPC" );
    client.disconnectForTest();
    require( client.maskStats().m_count == 0 && client.maskStats().m_status == rtimv::maskStatsStatus::noImage,
             "disconnected client exposed measurements" );
}

/// Verify serialized last-intent-wins toggles and server-authoritative state.
void testRequests( int port /**< [in] service port */, TestService &service /**< [in,out] service controls */ )
{
    TestClient client( port );
    client.deliver( makeImage( 1, false ) );
    service.m_hold = true;
    client.mtxUL_maskStats( true );
    require( waitUntil( [&]() { return service.m_calls == 1; } ), "setter did not reach server" );
    require( !client.maskStats().m_enabled && client.maskStatsRequested() && client.maskStatsPending(),
             "local intent replaced authoritative cache" );
    client.mtxUL_maskStats( false );
    client.mtxUL_maskStats( true );
    client.mtxUL_maskStats( false );
    require( service.m_calls == 1, "concurrent setters were not coalesced" );
    service.m_hold = false;
    require( waitUntil( [&]() { return service.m_finished == 2 && !client.maskStatsPending(); } ),
             "latest mode was not dispatched after completion" );
    require( !service.m_enabled && service.m_calls == 2 && !service.m_missingDeadline, "wrong final mode or deadline" );

    client.mtxUL_maskStats( true );
    require( waitUntil( [&]() { return service.m_finished == 3; } ), "enable request was not sent" );
    require( !client.maskStats().m_enabled, "RPC acknowledgement overwrote Image cache" );
    client.deliver( makeImage( 2, true ) );
    require( waitUntil( [&]() { return !client.maskStatsPending(); } ), "Image did not confirm mode" );
    require( client.maskStats().m_count == 2, "Image statistics were lost" );

    client.newGeneration();
    require( client.maskStats().m_count == 0 && client.maskStatsRequested(),
             "reconnect did not clear results/preserve intent" );
    client.deliver( makeImage( 1, false ) );
    require( waitUntil( [&]() { return service.m_finished == 4; } ), "reconnect did not restore requested mode" );
    client.deliver( makeImage( 2, true ) );
    require( waitUntil( [&]() { return !client.maskStatsPending(); } ), "reconnect result did not settle" );
}

/// Verify an unsupported RPC is remembered and stale failures cannot poison a new connection.
void testUnsupportedAndStale( int port /**< [in] service port */,
                              TestService &service /**< [in,out] service controls */ )
{
    {
        TestClient client( port );
        client.deliver( makeImage( 1, false ) );
        const int calls = service.m_calls;
        service.m_unsupported = true;
        client.mtxUL_maskStats( true );
        require( waitUntil( [&]() { return client.maskStats().m_status == rtimv::maskStatsStatus::unsupported; } ),
                 "unsupported RPC was not handled" );
        client.deliver( makeImage( 2, false ) );
        require( client.maskStats().m_status == rtimv::maskStatsStatus::unsupported && !client.maskStatsPending(),
                 "unsupported RPC caused a permanent pending state" );
        require( service.m_calls == calls + 1, "unsupported setter was retried" );
        client.mtxUL_maskStats( false );
        require( !client.maskStatsRequested(), "unsupported overlay could not be disabled" );
    }
    {
        TestClient client( port );
        client.deliver( makeImage( 1, false ) );
        service.m_hold = true;
        const int calls = service.m_calls;
        client.mtxUL_maskStats( true );
        require( waitUntil( [&]() { return service.m_calls > calls; } ), "old-generation request was not sent" );
        client.newGeneration();
        client.deliver( makeImage( 1, false ) );
        service.m_unsupported = true;
        service.m_hold = false;
        require( waitUntil( [&]() { return service.m_calls == calls + 2 && service.m_enabled; } ),
                 "old callback prevented the new-generation request" );
        client.deliver( makeImage( 2, true ) );
        require( waitUntil( [&]() { return !client.maskStatsPending(); } ) && client.connected() &&
                     client.maskStats().m_status == rtimv::maskStatsStatus::valid,
                 "stale callback damaged new-generation state" );
    }
}

/// Verify an expired setter clears measurements and preserves intent for reconnect.
void testDeadline( int port /**< [in] service port */, TestService &service /**< [in,out] service controls */ )
{
    TestClient client( port );
    client.deliver( makeImage( 1, false ) );
    service.m_hold = true;
    client.mtxUL_maskStats( true );
    require( waitUntil( [&]() { return !client.connected(); }, 3500 ), "setter deadline did not complete" );
    require( client.maskStatsRequested() && client.maskStats().m_count == 0,
             "deadline lost intent or retained results" );
    service.m_hold = false;
}

/// Verify shutdown cancels transport and does not require queued Qt completion processing.
void testShutdown( int port /**< [in] service port */, TestService &service /**< [in,out] service controls */ )
{
    service.m_hold = true;
    const int calls = service.m_calls;
    auto client = std::make_unique<TestClient>( port );
    client->deliver( makeImage( 1, false ) );
    client->mtxUL_maskStats( true );
    require( waitUntil( [&]() { return service.m_calls > calls; } ), "stalled request was not dispatched" );
    const auto start = std::chrono::steady_clock::now();
    client.reset();
    require( std::chrono::steady_clock::now() - start < std::chrono::milliseconds( 1500 ),
             "shutdown waited for deadline" );
    require( waitUntil( [&]() { return service.m_cancelled > 0; } ), "shutdown did not cancel transport" );
    service.m_hold = false;

    client = std::make_unique<TestClient>( port );
    client->deliver( makeImage( 1, false ) );
    client->mtxUL_maskStats( true );
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
    while( !client->completionQueued() && std::chrono::steady_clock::now() < deadline )
        std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
    require( client->completionQueued(), "could not retain queued completion" );
    client.reset();
    QCoreApplication::processEvents();
}

} // namespace

/// Run transport regressions against a loopback gRPC service.
int main( int argc /**< [in] argument count */, char **argv /**< [in] arguments */ )
{
    QCoreApplication app( argc, argv );
    TestService service;
    int port = 0;
    grpc::ServerBuilder builder;
    builder.AddListeningPort( "127.0.0.1:0", grpc::InsecureServerCredentials(), &port );
    builder.RegisterService( &service );
    auto server = builder.BuildAndStart();
    try
    {
        require( server && port, "could not start test service" );
        testState( port, service );
        testRequests( port, service );
        testUnsupportedAndStale( port, service );
        testDeadline( port, service );
        testShutdown( port, service );
    }
    catch( const std::exception &error )
    {
        service.m_hold = false;
        if( server )
            server->Shutdown( std::chrono::system_clock::now() );
        std::cerr << error.what() << '\n';
        return 1;
    }
    server->Shutdown( std::chrono::system_clock::now() );
    return 0;
}
