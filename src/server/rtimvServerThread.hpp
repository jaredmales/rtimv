
#ifndef rtimvServerThread_hpp
#define rtimvServerThread_hpp

#include "rtimvBase.hpp"

#include <QThread>
#include <QByteArray>

#include <mx/app/application.hpp>

using namespace mx::app;

#define RTIMV_DEBUG_BREADCRUMB

class rtimvServerThread : public QThread, public rtimvBase
{
    Q_OBJECT

  protected:
    std::string m_uri; ///< The URI for this thread as a server client

    std::shared_ptr<std::vector<std::string>> m_argv; /**< The command line argv.  This is set in the constructor, and
                                                      destroyed after configure is called.*/

    bool m_newImage{ false }; ///< Flag indicating a new image is ready after the last render.

    int m_quality{ 50 }; ///< The JPEG quality factor (0-100).  Default is 50.

    double m_lastRequest{ 0 }; ///< The time of the last request for an image

    bool m_asleep{ false };

    int m_configured{ 0 }; ///< 0 is unconfigured, 1 is configured, -1 is configuration error

  public:
    rtimvServerThread( const std::string &uri,                         /**< [in] client uri */
                       std::shared_ptr<std::vector<std::string>> argv, /**< [in] The argv vector. */
                       QObject *parent = nullptr                       /**< [in] [opt] the parent of this thread */
    );

    ~rtimvServerThread();

    void configure();

    int configured();

  signals:
    void rendered();

  public:
    /// Called on initial connection to the image stream, sets matching aspect ratio.
    virtual void onConnect();

    virtual void mtxL_postSetImsize( const uniqueLockT &lock );

    virtual void post_zoomLevel();

  public:

    void mtxUL_recolor();

    virtual void mtxL_postRecolor( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/ );

    virtual void mtxL_postRecolor( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    virtual void mtxL_postChangeImdata( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/ );

    virtual void mtxL_postColormode( rtimv::colormode m, const sharedLockT &lock );

    void mtxuL_render( std::string *image );

    bool newImage();

    int quality();

    void quality( int q );

    double lastRequest();

    void lastRequest( double lr );

    double sinceLastRequest();

    bool asleep();

  signals:

    void gotosleep();

    void awaken();

  public:
    void emit_gotosleep();

    void emit_awaken();

  public slots:

    void sleep();

    void wakeup();
};

#endif // rtimvServerThread_hpp
