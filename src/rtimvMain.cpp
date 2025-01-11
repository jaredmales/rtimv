#include <QApplication>

#include "rtimvMainWindow.hpp"

int main( int argc, char *argv[] )
{
    QApplication app( argc, argv );

    rtimvMainWindow rtimv( argc, argv );
    rtimv.show();

    return app.exec();
}
