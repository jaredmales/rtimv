#include <QApplication>

#include "rtimvMainWindow.hpp"



int main(int argc, char *argv[])
{
   QApplication app(argc, argv);

   rtimvMainWindow imv(argc, argv);
   imv.show();

   return app.exec();
}

