#include <QApplication>

#include "imviewerform.hpp"

void error_report(const char* er, const char* file, int lno)
{
   //Logger::get()->log(Logger::LOG_LEV_ERROR, "%s.  File: %s  Line: %i", er, file, lno);
   std::cerr << "imviewer: " << er << " File: " << file << " Line: " << lno << std::endl;
}


void log_info(const char* li) 
{
   //Logger::get()->log(Logger::LOG_LEV_INFO, "%s.", li);
   std::cout << li << "\n";
}


int main(int argc, char *argv[])
{
   //int data_type;
   QApplication app(argc, argv);

   std::string shmem_key;
   
   if(argc > 1)
   {
      shmem_key = argv[1];
   }
   else
   {
      std::cerr << "Must provide a SCExAO shared memory file\n";
      exit(0);
   }
   
   imviewerForm imv(shmem_key);
   
   imv.show();
   
   return app.exec();
}
