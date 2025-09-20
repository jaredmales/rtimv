
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
    std::string m_configFile; ///< The configuration file

    bool m_newImage {false}; ///< Flag indicating a new image is ready after the last render.

  public:

    int m_configured {0}; ///< 0 is unconfigured, 1 is configured, -1 is configuration error

    rtimvServerThread( const std::string & uri,
                       const std::string & configFile,
                       QObject *parent = nullptr
                     );

    ~rtimvServerThread();

    void configure( );

    //void run() override;

signals:
    void rendered();

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

    virtual void mtxL_postSetColorBoxActive(  bool usba, const sharedLockT &lock );

    void mtxuL_render(std::string * image);

    bool newImage();
};



#endif // rtimvServerThread_hpp
