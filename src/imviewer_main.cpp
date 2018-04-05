#include <QApplication>

#include "imviewerform.h"

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

#if RT_SYSTEM == RT_SYSTEM_VISAO
set_global_error_report(&error_report);
set_global_log_info(&log_info);
#endif

int main(int argc, char *argv[])
{
   //int data_type;
   QApplication app(argc, argv);

#if RT_SYSTEM == RT_SYSTEM_VISAO
   key_t shmem_key = 5000;
   data_type = IMV_SHORT;
   
   if(argc > 1)
   {
      shmem_key = atoi(argv[1]);
   }

   
#endif

#if RT_SYSTEM == RT_SYSTEM_SCEXAO
   std::string shmem_key = "test";
   //data_type = 1;//IMV_SHORT;
   
   if(argc > 1)
   {
      shmem_key = argv[1];
   }
   else
   {
      std::cerr << "Must provide a SCExAO shared memory file\n";
      exit(0);
   }

   
#endif
   
   imviewerForm imv(shmem_key);
   
   imv.show();
   
   return app.exec();
}
