
#ifndef rtimvServer_hpp
#define rtimvServer_hpp

#include "rtimvBase.hpp"

#include <QTcpServer>
#include <mx/app/application.hpp>

using namespace mx::app;

#define RTIMV_DEBUG_BREADCRUMB




class rtimvServer : public rtimvBase, public application
{
    Q_OBJECT

  public:
    rtimvServer( int argc, char **argv, QWidget *Parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );

    ~rtimvServer();

    virtual void setupConfig();

    virtual void loadConfig();


  public:
    /// Called on initial connection to the image stream, sets matching aspect ratio.
    virtual void onConnect();

    virtual void mtxL_postSetImsize( const uniqueLockT &lock );

    virtual void post_zoomLevel();

  private:
    ///Generic implementation of postRecolor
    template<class lockT>
    void mtxL_postRecolorImpl(const lockT & lock /**<[in] a mutex lock which is locked*/);

  public:
    virtual void mtxL_postRecolor( const uniqueLockT &lock /**<[in] a unique mutex lock which is locked*/);

    virtual void mtxL_postRecolor( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/);

    virtual void mtxL_postChangeImdata( const sharedLockT &lock /**<[in] a shared mutex lock which is locked*/);

    virtual void mtxL_postSetColorBoxActive( bool usba, const sharedLockT &lock )
    {}

    QTcpServer * m_server {nullptr};

    void startServer();

    public slots:

    void newConnection();

};

template<class lockT>
void rtimvServer::mtxL_postRecolorImpl(const lockT & lock)
{
    RTIMV_DEBUG_BREADCRUMB

    assert( lock.owns_lock() );

    RTIMV_DEBUG_BREADCRUMB

    //Do the jpeg here!

    RTIMV_DEBUG_BREADCRUMB
}

#endif // rtimvServer_hpp
