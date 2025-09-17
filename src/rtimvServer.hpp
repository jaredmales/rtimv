
#ifndef rtimvServer_hpp
#define rtimvServer_hpp

#include "rtimvServerThread.hpp"

#include <QTcpServer>
#include <mx/app/application.hpp>

using namespace mx::app;

#define RTIMV_DEBUG_BREADCRUMB


class rtimvServer : public QTcpServer, public application
{
    Q_OBJECT

  public:
    rtimvServer( int argc, char **argv, QObject *Parent = nullptr );

    ~rtimvServer();

    virtual void setupConfig();

    virtual void loadConfig();

    QTcpServer * m_server {nullptr};

    void startServer();

    protected:

    void incomingConnection(qintptr socketDescriptor) override;

};


#endif // rtimvServer_hpp
