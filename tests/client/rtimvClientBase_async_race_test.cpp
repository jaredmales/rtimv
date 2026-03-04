#include <QCoreApplication>

#include <cassert>
#include <atomic>
#include <chrono>
#include <thread>

#include "rtimvClientBase.hpp"

class TestClientBase : public rtimvClientBase
{
  public:
    void setConnectedForTest( bool connected )
    {
        uniqueLockT lock( m_connectedMutex );
        m_connected = connected;
    }

    bool connectedForTest()
    {
        sharedLockT lock( m_connectedMutex );
        return m_connected;
    }

    bool imagePendingForTest()
    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        return m_imageRequestPending;
    }

    bool imageContextAllocatedForTest()
    {
        std::lock_guard<std::mutex> lock( m_imageRequestMutex );
        return m_ImagePleaseContext != nullptr;
    }

  protected:
    void mtxL_postColormode( rtimv::colormode, const sharedLockT & ) override {}
    void mtxL_postRecolor( const uniqueLockT & ) override {}
    void mtxL_postRecolor( const sharedLockT & ) override {}
    void mtxL_postChangeImdata( const sharedLockT & ) override {}
    void post_zoomLevel() override {}
};

class ImmediateFailClient : public TestClientBase
{
  protected:
    void dispatchImagePleaseAsync() override
    {
        std::thread callbackThread( [this]() {
            this->ImagePlease_callback( grpc::Status( grpc::StatusCode::UNAVAILABLE, "simulated unavailable server" ) );
        } );
        callbackThread.join();
    }
};

class DelayedFailClient : public TestClientBase
{
  protected:
    void dispatchImagePleaseAsync() override
    {
        std::thread callbackThread( [this]() {
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            this->ImagePlease_callback( grpc::Status( grpc::StatusCode::UNAVAILABLE, "simulated unavailable server" ) );
        } );
        callbackThread.detach();
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

        assert( !client.imagePendingForTest() );
        assert( !client.imageContextAllocatedForTest() );
        assert( !client.connectedForTest() );
    }

    // Regression: shutdown while ImagePlease is pending should not hang.
    {
        auto *client = new DelayedFailClient;
        client->setConnectedForTest( true );
        client->ImagePlease();

        std::atomic<bool> deleted{ false };
        std::thread deleteThread( [client, &deleted]() {
            delete client;
            deleted = true;
        } );

        for( int i = 0; i < 200 && !deleted.load(); ++i )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }

        assert( deleted.load() );
        deleteThread.join();
    }

    return 0;
}
