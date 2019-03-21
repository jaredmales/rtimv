#include <QApplication>

#include "imviewerform.hpp"

#if 0
struct rtimvAppConfig : public mx::app::application
{
   std::string m_imShmimKey;
   int m_imShmimTimeout {1000};
   int m_imTimeout {100};
   
   std::string m_darkShmimKey;
   int m_darkShmimTimeout {1000};
   int m_darkTimeout {100};
   
   std::string m_refShmimKey;
   int m_refShmimTimeout {1000};
   int m_refTimeout {100};
   
   std::string m_flatShmimKey;
   int m_flatShmimTimeout {1000};
   int m_flatTimeout {100};
   
   std::string m_maskShmimKey;
   int m_maskShmimTimeout {1000};
   int m_maskTimeout {100};

   virtual void setupConfig()
   {
      config.add("image.shmim_name", "", "image.shmim_name", argType::Required, "image", "shmim_name", false, "string", "The shared memory image file name for the image.");
      config.add("image.shmim_timeout", "", "image.shmim_timeout", argType::Required, "image", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the image.  Default is 1000 msec.");
      config.add("image.timeout", "", "image.timeout", argType::Required, "image", "timeout", false, "int", "The timeout for checking for a new image.  Default is 100 msec (10 f.p.s.).");
      
      config.add("dark.shmim_name", "", "dark.shmim_name", argType::Required, "dark", "shmim_name", false, "string", "The shared memory image file name for the dark.");
      config.add("dark.shmim_timeout", "", "dark.shmim_timeout", argType::Required, "dark", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the dark.  Default is 1000 msec.");
      config.add("dark.timeout", "", "dark.timeout", argType::Required, "dark", "timeout", false, "int", "The timeout for checking for a new dark.  Default is 100 msec (10 f.p.s.).");
      
      config.add("reference.shmim_name", "", "reference.shmim_name", argType::Required, "reference", "shmim_name", false, "string", "The shared memory image file name for the reference.");
      config.add("reference.shmim_timeout", "", "reference.shmim_timeout", argType::Required, "reference", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the reference.  Default is 1000 msec.");
      config.add("reference.timeout", "", "reference.timeout", argType::Required, "reference", "timeout", false, "int", "The timeout for checking for a new reference.  Default is 100 msec (10 f.p.s.).");
      
      config.add("flat.shmim_name", "", "flat.shmim_name", argType::Required, "flat", "shmim_name", false, "string", "The shared memory image file name for the flat.");
      config.add("flat.shmim_timeout", "", "flat.shmim_timeout", argType::Required, "flat", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the flat.  Default is 1000 msec.");
      config.add("flat.timeout", "", "flat.timeout", argType::Required, "flat", "timeout", false, "int", "The timeout for checking for a new flat.  Default is 100 msec (10 f.p.s.).");
      
      config.add("mask.shmim_name", "", "mask.shmim_name", argType::Required, "mask", "shmim_name", false, "string", "The shared memory image file name for the mask.");
      config.add("mask.shmim_timeout", "", "mask.shmim_timeout", argType::Required, "mask", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the mask.  Default is 1000 msec.");
      config.add("mask.timeout", "", "mask.timeout", argType::Required, "mask", "timeout", false, "int", "The timeout for checking for a new mask.  Default is 100 msec (10 f.p.s.).");
   }

   virtual void loadConfig()
   {
      config(m_imShmimKey, "image.shmim_name");
      config(m_imShmimTimeout, "image.shmim_timeout");
      config(m_imTimeout, "image.timeout");
      
      config(m_darkShmimKey, "dark.shmim_name");
      config(m_darkShmimTimeout, "dark.shmim_timeout");
      config(m_darkTimeout, "dark.timeout");
      
      config(m_referenceShmimKey, "reference.shmim_name");
      config(m_referenceShmimTimeout, "reference.shmim_timeout");
      config(m_referenceTimeout, "reference.timeout");
      
      config(m_flatShmimKey, "flat.shmim_name");
      config(m_flatShmimTimeout, "flat.shmim_timeout");
      config(m_flatTimeout, "flat.timeout");
      
      config(m_darkShmimKey, "dark.shmim_name");
      config(m_darkShmimTimeout, "dark.shmim_timeout");
      config(m_darkTimeout, "dark.timeout");
      
   }

   virtual int execute()
   {
      return 0;
   }
}
#endif

int main(int argc, char *argv[])
{
   //int data_type;
   QApplication app(argc, argv);

   std::string shmem_key, dark_key, mask_key;

   std::vector<std::string> keys;
   if(argc > 1)
   {
      keys.push_back(argv[1]);
   }
   else
   {
      std::cerr << "Must provide a SCExAO shared memory file\n";
      exit(0);
   }

   if(argc > 2)
   {
      keys.push_back(argv[2]);
   }

   if(argc > 3)
   {
      keys.push_back(argv[3]);
   }
   
   imviewerForm imv(keys);

   imv.show();

   return app.exec();
}
