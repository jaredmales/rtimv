/** \file rtimvClientBase_async_race_test.cpp
 * \brief Regression tests for ImagePlease async callback races in rtimvClientBase
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include <QCoreApplication>

#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>

#include "rtimvClientBase.hpp"

class TestClientBase : public rtimvClientBase
{
  public:
    /// Force the connected flag for test setup.
    void setConnectedForTest( bool connected /**< [in] desired test connected state */ )
    {
        uniqueLockT lock( m_connectedMutex );
        m_connected = connected;
    }

    /// Read the connected flag under the normal client lock.
    bool connectedForTest()
    {
        sharedLockT lock( m_connectedMutex );
        return m_connected;
    }

    /// Get the number of ImagePlease RPCs currently tracked as in flight.
    size_t imageRequestCountForTest()
    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        return m_inflightImageRequests.size();
    }

    /// Force the image pipeline state for pipelined shutdown regression coverage.
    void setImagePipelineForTest( size_t window, /**< [in] desired in-flight window */
                                  bool primed    /**< [in] true to allow full-window dispatch immediately */
    )
    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        m_targetImageWindow = window;
        m_imagePipelinePrimed = primed;
    }

  protected:
    /// Test stub: no GUI colormode work needed.
    void mtxL_postColormode( rtimv::colormode, const sharedLockT & ) override
    {
    }

    /// Test stub: no GUI recolor work needed.
    void mtxL_postRecolor( const uniqueLockT & ) override
    {
    }

    /// Test stub: no GUI recolor work needed.
    void mtxL_postRecolor( const sharedLockT & ) override
    {
    }

    /// Test stub: no GUI image-data work needed.
    void mtxL_postChangeImdata( const sharedLockT & ) override
    {
    }

    /// Test stub: no GUI zoom work needed.
    void post_zoomLevel() override
    {
    }
};

class ImmediateFailClient : public TestClientBase
{
  protected:
    /// Complete the queued ImagePlease RPC immediately with a transport failure.
    void dispatchImagePleaseAsync( uint64_t requestId,                      /**< [in] local request identifier */
                                   std::shared_ptr<imageRequestState> state /**< [in] request state */
                                   ) override
    {
        std::thread callbackThread(
            [this, requestId, state]()
            {
                this->ImagePlease_callback(
                    requestId, state, grpc::Status( grpc::StatusCode::UNAVAILABLE, "simulated unavailable server" ) );
            } );
        callbackThread.join();
    }
};

class DelayedFailClient : public TestClientBase
{
  protected:
    /// Complete the queued ImagePlease RPC after a short delay with a transport failure.
    void dispatchImagePleaseAsync( uint64_t requestId,                      /**< [in] local request identifier */
                                   std::shared_ptr<imageRequestState> state /**< [in] request state */
                                   ) override
    {
        std::thread callbackThread(
            [this, requestId, state]()
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
                this->ImagePlease_callback(
                    requestId, state, grpc::Status( grpc::StatusCode::UNAVAILABLE, "simulated unavailable server" ) );
            } );
        callbackThread.detach();
    }
};

class NoImageRetryClient : public TestClientBase
{
  protected:
    /// Complete the queued ImagePlease RPC with a NO_IMAGE response to exercise retry scheduling during teardown.
    void dispatchImagePleaseAsync( uint64_t requestId,                      /**< [in] local request identifier */
                                   std::shared_ptr<imageRequestState> state /**< [in] request state */
                                   ) override
    {
        std::thread callbackThread(
            [this, requestId, state]()
            {
                state->m_reply.set_status( remote_rtimv::IMAGE_STATUS_NO_IMAGE );
                this->ImagePlease_callback( requestId, state, grpc::Status::OK );
            } );
        callbackThread.join();
    }
};

int main( int argc, char **argv )
{
    QCoreApplication app( argc, argv );
    static_cast<void>( app );

    // Regression: callback can run immediately after dispatch.
    {
        ImmediateFailClient client;
        client.setConnectedForTest( true );

        client.ImagePlease();

        assert( client.imageRequestCountForTest() == 0 );
        assert( !client.connectedForTest() );
    }

    // Regression: shutdown while ImagePlease is pending should not hang.
    {
        auto *client = new DelayedFailClient;
        client->setConnectedForTest( true );
        client->ImagePlease();

        std::atomic<bool> deleted{ false };
        std::thread deleteThread(
            [client, &deleted]()
            {
                delete client;
                deleted = true;
            } );

        for( int i = 0; i < 200 && !deleted.load(); ++i )
        {
            QCoreApplication::processEvents();
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }

        assert( deleted.load() );
        deleteThread.join();
    }

    // Regression: shutdown while a pipelined image window is in flight should not hang.
    {
        auto *client = new DelayedFailClient;
        client->setConnectedForTest( true );
        client->setImagePipelineForTest( 8, true );
        client->ImagePlease();

        std::atomic<bool> deleted{ false };
        std::thread deleteThread(
            [client, &deleted]()
            {
                delete client;
                deleted = true;
            } );

        for( int i = 0; i < 200 && !deleted.load(); ++i )
        {
            QCoreApplication::processEvents();
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }

        assert( deleted.load() );
        deleteThread.join();
    }

    // Regression: a queued no-image retry should be canceled safely during shutdown.
    {
        auto *client = new NoImageRetryClient;
        client->setConnectedForTest( true );
        client->ImagePlease();

        delete client;

        std::this_thread::sleep_for( std::chrono::milliseconds( 2200 ) );
        QCoreApplication::processEvents();
    }

    return 0;
}
