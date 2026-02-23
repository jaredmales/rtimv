#include <QCoreApplication>
#include <iostream>

#include "rtimvServer.hpp"

int main( int argc, char *argv[] )
{
    std::cout.setf( std::ios::unitbuf );

    QCoreApplication app( argc, argv );

    rtimvServer rtimv( argc, argv );

    return app.exec();
}
