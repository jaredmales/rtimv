/** \file rtimvClientBase_lock_order_test.cpp
 * \brief Regression coverage for request/error-callback connection lock ordering.
 */

#include <QCoreApplication>

#include <array>
#include <chrono>
#include <iostream>
#include <thread>

#include "rtimvClientBase.hpp"

class LockOrderClient : public rtimvClientBase
{
  public:
    /// Describe an async operation's callback and the state it must release before reporting a connection error.
    struct CallbackCase
    {
        /// Name included in test failures.
        const char *m_name;

        /// Production callback exercised by this case.
        void ( rtimvClientBase::*m_callback )( grpc::Status );

        /// Pending flag owned by the callback.
        bool rtimvClientBase::*m_pending;

        /// Request context transferred to the callback on completion.
        grpc::ClientContext *rtimvClientBase::*m_context;
    };

    /// Get the callbacks whose error handling must not invert the request lock order.
    static const std::array<CallbackCase, 6> &callbackCases();

    /// Prepare a pending request without requiring a live gRPC transport.
    void prepare( const CallbackCase &testCase /**< [in] operation to prepare */ );

    /// Hold the connection mutex to simulate competing connection-state work.
    uniqueLockT holdConnection();

    /// Read whether the callback has released its pending state without blocking on the async mutex.
    bool pendingCleared( const CallbackCase &testCase /**< [in] operation whose pending state is inspected */ );

    /// Complete the selected operation with a transport error.
    void failRequest( const CallbackCase &testCase, /**< [in] operation to complete */
                      grpc::StatusCode code         /**< [in] simulated transport error code */
    );

    /// Read the connection state after the callback finishes.
    bool connectedForTest();

  protected:
    /// Test stub: no GUI colormode work is needed.
    void mtxL_postColormode( rtimv::colormode mode,  /**< [in] requested colormode */
                             const sharedLockT &lock /**< [in] caller's image lock */
                             ) override;

    /// Test stub: no GUI recolor work is needed.
    void mtxL_postRecolor( const uniqueLockT &lock /**< [in] caller's exclusive image lock */ ) override;

    /// Test stub: no GUI recolor work is needed.
    void mtxL_postRecolor( const sharedLockT &lock /**< [in] caller's shared image lock */ ) override;

    /// Test stub: no GUI image-data work is needed.
    void mtxL_postChangeImdata( const sharedLockT &lock /**< [in] caller's image lock */ ) override;

    /// Test stub: no GUI zoom work is needed.
    void post_zoomLevel() override;
};

const std::array<LockOrderClient::CallbackCase, 6> &LockOrderClient::callbackCases()
{
    static const std::array<CallbackCase, 6> cases{ CallbackCase{ "GetPixel",
                                                                  &LockOrderClient::GetPixel_callback,
                                                                  &LockOrderClient::m_getPixelPending,
                                                                  &LockOrderClient::m_GetPixelContext },
                                                    CallbackCase{ "ColorBox",
                                                                  &LockOrderClient::ColorBox_callback,
                                                                  &LockOrderClient::m_colorBoxPending,
                                                                  &LockOrderClient::m_ColorBoxContext },
                                                    CallbackCase{ "StatsBox",
                                                                  &LockOrderClient::StatsBox_callback,
                                                                  &LockOrderClient::m_statsBoxPending,
                                                                  &LockOrderClient::m_StatsBoxContext },
                                                    CallbackCase{ "SetColorstretch",
                                                                  &LockOrderClient::SetColorstretch_callback,
                                                                  &LockOrderClient::m_setColorstretchPending,
                                                                  &LockOrderClient::m_SetColorstretchContext },
                                                    CallbackCase{ "SetMinScale",
                                                                  &LockOrderClient::SetMinScale_callback,
                                                                  &LockOrderClient::m_setMinScalePending,
                                                                  &LockOrderClient::m_SetMinScaleContext },
                                                    CallbackCase{ "SetMaxScale",
                                                                  &LockOrderClient::SetMaxScale_callback,
                                                                  &LockOrderClient::m_setMaxScalePending,
                                                                  &LockOrderClient::m_SetMaxScaleContext } };
    return cases;
}

void LockOrderClient::prepare( const CallbackCase &testCase )
{
    uniqueLockT connLock( m_connectedMutex );
    std::lock_guard<std::mutex> asyncLock( m_asyncRpcMutex );
    m_connected = true;
    this->*testCase.m_pending = true;
    this->*testCase.m_context = new grpc::ClientContext;
}

rtimvClientBase::uniqueLockT LockOrderClient::holdConnection()
{
    return uniqueLockT( m_connectedMutex );
}

bool LockOrderClient::pendingCleared( const CallbackCase &testCase )
{
    std::unique_lock<std::mutex> asyncLock( m_asyncRpcMutex, std::try_to_lock );
    return asyncLock.owns_lock() && !( this->*testCase.m_pending ) && this->*testCase.m_context == nullptr;
}

void LockOrderClient::failRequest( const CallbackCase &testCase, grpc::StatusCode code )
{
    ( this->*testCase.m_callback )( grpc::Status( code, "simulated disconnected tunnel" ) );
}

bool LockOrderClient::connectedForTest()
{
    sharedLockT connLock( m_connectedMutex );
    return m_connected;
}

void LockOrderClient::mtxL_postColormode( rtimv::colormode mode, const sharedLockT &lock )
{
    static_cast<void>( mode );
    static_cast<void>( lock );
}

void LockOrderClient::mtxL_postRecolor( const uniqueLockT &lock )
{
    static_cast<void>( lock );
}

void LockOrderClient::mtxL_postRecolor( const sharedLockT &lock )
{
    static_cast<void>( lock );
}

void LockOrderClient::mtxL_postChangeImdata( const sharedLockT &lock )
{
    static_cast<void>( lock );
}

void LockOrderClient::post_zoomLevel()
{
}

/// Run each error callback while the connection mutex remains held by another thread.
int main( int argc,   /**< [in] command-line argument count */
          char **argv /**< [in] command-line arguments */
)
{
    QCoreApplication app( argc, argv );
    static_cast<void>( app );

    for( auto code : { grpc::StatusCode::UNAVAILABLE, grpc::StatusCode::DEADLINE_EXCEEDED } )
    {
        for( const auto &testCase : LockOrderClient::callbackCases() )
        {
            LockOrderClient client;
            client.prepare( testCase );
            auto connLock = client.holdConnection();
            std::thread callbackThread( [&]() { client.failRequest( testCase, code ); } );

            // Completion of pending-state cleanup proves the callback ran and released its async mutex.
            // The connection mutex stays held throughout, so reporting the error cannot finish first.
            bool pendingCleared = false;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
            do
            {
                pendingCleared = client.pendingCleared( testCase );
                if( !pendingCleared )
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            } while( !pendingCleared && std::chrono::steady_clock::now() < deadline );

            // Always release the setup lock and join, including when testing the original defective implementation.
            connLock.unlock();
            callbackThread.join();

            if( !pendingCleared )
            {
                std::cerr << testCase.m_name << " retained async state while waiting for the connection mutex\n";
                return 1;
            }

            if( client.connectedForTest() != ( code == grpc::StatusCode::DEADLINE_EXCEEDED ) )
            {
                std::cerr << testCase.m_name << " changed the transport error's connection-state behavior\n";
                return 1;
            }
        }
    }

    return 0;
}
