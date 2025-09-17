
#ifndef rtimvServerThread_hpp
#define rtimvServerThread_hpp

#include "rtimvBase.hpp"

#include <QThread>
#include <QTcpSocket>
#include <mx/app/application.hpp>

using namespace mx::app;

#define RTIMV_DEBUG_BREADCRUMB


class rtimvServerThread : public QThread, public rtimvBase
{
    Q_OBJECT

  protected:

    QTcpSocket * m_tcpSocket {nullptr};

  public:
    rtimvServerThread( int sockDescrip, const std::string & configFile, QObject *parent = nullptr );

    ~rtimvServerThread();

    void run() override;

signals:
    void error(QTcpSocket::SocketError socketError);

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

    virtual void mtxL_postSetColorBoxActive(  bool usba, const sharedLockT &lock )
    {
        static_cast<void>(usba);
        static_cast<void>(lock);
    }


};



#endif // rtimvServerThread_hpp
