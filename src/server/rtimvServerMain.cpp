#include <QCoreApplication>

#include "rtimvServer.hpp"

int main( int argc, char *argv[] )
{
    QCoreApplication app( argc, argv );

    rtimvServer rtimv( argc, argv );

    return app.exec();
}
