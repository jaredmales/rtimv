#include <QCoreApplication>

#include "rtimvServer.hpp"

int main( int argc, char *argv[] )
{
    QCoreApplication app( argc, argv );

    std::cerr << "1\n";

    rtimvServer rtimv( argc, argv );

    std::cerr << "2\n";

    return app.exec();
}
