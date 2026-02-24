#include <QApplication>

#include "rtimvMainWindow.hpp"

int main( int argc, char *argv[] )
{
    try
    {
        QApplication app( argc, argv );

        rtimvMainWindow rtimv( argc, argv );
        rtimv.show();

        return app.exec();
    }
    catch( const std::runtime_error &e )
    {
        if( e.what() != std::string( "help" ) )
        {
            std::cerr << e.what() << '\n';
            return -1;
        }
        else
        {
            return 0;
        }
    }
}
