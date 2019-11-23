#include <QApplication>

#include "imviewerform.hpp"


#if 1
#include <mx/app/application.hpp>

using namespace mx::app;

struct rtimvAppConfig : public mx::app::application
{
   std::string m_imShmimKey;
   int m_imShmimTimeout {1000};
   int m_imTimeout {100};
   
   std::string m_darkShmimKey;
   int m_darkShmimTimeout {1000};
   int m_darkTimeout {100};
   
   std::string m_flatShmimKey;
   int m_flatShmimTimeout {1000};
   int m_flatTimeout {100};
   
   std::string m_maskShmimKey;
   int m_maskShmimTimeout {1000};
   int m_maskTimeout {100};

   std::string m_satMaskShmimKey;
   int m_satMaskShmimTimeout {1000};
   int m_satMaskTimeout {100};
   
   std::vector<std::string> m_keys;
   
   bool m_autoscale {false};
   bool m_nofpsgage {false};

   virtual void setupConfig()
   {
      config.add("image.shmim_name", "", "image.shmim_name", argType::Required, "image", "shmim_name", false, "string", "The shared memory image file name for the image.");
      config.add("image.shmim_timeout", "", "image.shmim_timeout", argType::Required, "image", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the image.  Default is 1000 msec.");
      config.add("image.timeout", "", "image.timeout", argType::Required, "image", "timeout", false, "int", "The timeout for checking for a new image.  Default is 100 msec (10 f.p.s.).");
      
      config.add("dark.shmim_name", "", "dark.shmim_name", argType::Required, "dark", "shmim_name", false, "string", "The shared memory image file name for the dark.");
      config.add("dark.shmim_timeout", "", "dark.shmim_timeout", argType::Required, "dark", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the dark.  Default is 1000 msec.");
      config.add("dark.timeout", "", "dark.timeout", argType::Required, "dark", "timeout", false, "int", "The timeout for checking for a new dark.  Default is 100 msec (10 f.p.s.).");
      
      config.add("mask.shmim_name", "", "mask.shmim_name", argType::Required, "mask", "shmim_name", false, "string", "The shared memory image file name for the mask.");
      config.add("mask.shmim_timeout", "", "mask.shmim_timeout", argType::Required, "mask", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the mask.  Default is 1000 msec.");
      config.add("mask.timeout", "", "mask.timeout", argType::Required, "mask", "timeout", false, "int", "The timeout for checking for a new mask.  Default is 100 msec (10 f.p.s.).");

      config.add("satMask.shmim_name", "", "satMask.shmim_name", argType::Required, "satMask", "shmim_name", false, "string", "The shared memory image file name for the saturation mask.");
      config.add("satMask.shmim_timeout", "", "satMask.shmim_timeout", argType::Required, "satMask", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the saturation mask.  Default is 1000 msec.");
      config.add("satMask.timeout", "", "satMask.timeout", argType::Required, "satMask", "timeout", false, "int", "The timeout for checking for a new saturation mask.  Default is 100 msec (10 f.p.s.).");

      
      
      config.add("autoscale", "", "autoscale", argType::True, "", "autoscale", false, "bool", "Set to turn autoscaling on at startup");
      config.add("nofpsgage", "", "nofpsgage", argType::True, "", "nofpsgage", false, "bool", "Set to turn the fps gage off at startup");
      
   }

   virtual void loadConfig()
   {
      config(m_imShmimKey, "image.shmim_name");
      config(m_imShmimTimeout, "image.shmim_timeout");
      config(m_imTimeout, "image.timeout");
      
      config(m_darkShmimKey, "dark.shmim_name");
      config(m_darkShmimTimeout, "dark.shmim_timeout");
      config(m_darkTimeout, "dark.timeout");
      
      config(m_maskShmimKey, "mask.shmim_name");
      config(m_maskShmimTimeout, "mask.shmim_timeout");
      config(m_maskTimeout, "mask.timeout");
      
      config(m_satMaskShmimKey, "satMask.shmim_name");
      config(m_satMaskShmimTimeout, "satMask.shmim_timeout");
      config(m_satMaskTimeout, "satMask.timeout");
      
      m_keys.resize(4);
      
      if(m_imShmimKey != "") m_keys[0] = m_imShmimKey;
      
      if(m_darkShmimKey != "") m_keys[1] = m_darkShmimKey;
      
      if(m_maskShmimKey != "") m_keys[2] = m_maskShmimKey;
      
      if(m_satMaskShmimKey != "") m_keys[3] = m_satMaskShmimKey;
      
      
      
      if(config.nonOptions.size() > 0) m_keys[0] = config.nonOptions[0];

      if(m_keys[0] == "")
      {
         std::cerr << "Must provide a SCExAO shared memory file\n";
         exit(0);
      }

      if(config.nonOptions.size() > 1) m_keys[1] = config.nonOptions[1];

      if(config.nonOptions.size() > 2) m_keys[2] = config.nonOptions[2];
      
      if(config.nonOptions.size() > 3) m_keys[3] = config.nonOptions[3];

      config(m_autoscale, "autoscale");
      config(m_nofpsgage, "nofpsgage");
   }

   virtual int execute()
   {
      return 0;
   }
};

int main(int argc, char *argv[])
{
   //int data_type;
   QApplication app(argc, argv);

   rtimvAppConfig rtimv;
   rtimv.main(argc,argv);

   imviewerForm imv(rtimv.m_keys);

   
   if(rtimv.m_autoscale)
   {
      imv.setAutoScale(true);
   }
   
   
   if(rtimv.m_nofpsgage)
   {
      imv.showFPSGage(false);
   }
   
   

   
   
   imv.show();

   return app.exec();
}

#else

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

#endif
