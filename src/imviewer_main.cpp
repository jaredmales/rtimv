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

   float m_targetXc {0.5};
   float m_targetYc {0.5};
   
   virtual void setupConfig()
   {
      config.add("image.shmim_name", "", "image.shmim_name", argType::Required, "image", "shmim_name", false, "string", "The shared memory image file name for the image, or a FITS file path.");
      config.add("image.shmim_timeout", "", "image.shmim_timeout", argType::Required, "image", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the image.  Default is 1000 msec.");
      config.add("image.timeout", "", "image.timeout", argType::Required, "image", "timeout", false, "int", "The timeout for checking for a new image.  Default is 100 msec (10 f.p.s.).");
      
      config.add("dark.shmim_name", "", "dark.shmim_name", argType::Required, "dark", "shmim_name", false, "string", "The shared memory image file name for the dark, or a FITS image path.");
      config.add("dark.shmim_timeout", "", "dark.shmim_timeout", argType::Required, "dark", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the dark.  Default is 1000 msec.");
      config.add("dark.timeout", "", "dark.timeout", argType::Required, "dark", "timeout", false, "int", "The timeout for checking for a new dark.  Default is 100 msec (10 f.p.s.).");
      
      config.add("mask.shmim_name", "", "mask.shmim_name", argType::Required, "mask", "shmim_name", false, "string", "The shared memory image file name for the mask, or a FITS image path.");
      config.add("mask.shmim_timeout", "", "mask.shmim_timeout", argType::Required, "mask", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the mask.  Default is 1000 msec.");
      config.add("mask.timeout", "", "mask.timeout", argType::Required, "mask", "timeout", false, "int", "The timeout for checking for a new mask.  Default is 100 msec (10 f.p.s.).");

      config.add("satMask.shmim_name", "", "satMask.shmim_name", argType::Required, "satMask", "shmim_name", false, "string", "The shared memory image file name for the saturation , or a FITS image path.");
      config.add("satMask.shmim_timeout", "", "satMask.shmim_timeout", argType::Required, "satMask", "shmim_timeout", false, "int", "The timeout for checking for the shared memory image for the saturation mask.  Default is 1000 msec.");
      config.add("satMask.timeout", "", "satMask.timeout", argType::Required, "satMask", "timeout", false, "int", "The timeout for checking for a new saturation mask.  Default is 100 msec (10 f.p.s.).");

      
      
      config.add("autoscale", "", "autoscale", argType::True, "", "autoscale", false, "bool", "Set to turn autoscaling on at startup");
      config.add("nofpsgage", "", "nofpsgage", argType::True, "", "nofpsgage", false, "bool", "Set to turn the fps gage off at startup");
      config.add("targetXc", "", "targetXc", argType::Required, "", "targetXc", false, "float", "The fractional x-coordinate of the target, 0<= x <=1");
      config.add("targetYc", "", "targetYc", argType::Required, "", "targetXc", false, "float", "The fractional y-coordinate of the target, 0<= y <=1");
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
      
      

      //The command line always overrides the config
      if(config.nonOptions.size() > 0) m_keys[0] = config.nonOptions[0];

      if(config.nonOptions.size() > 1) m_keys[1] = config.nonOptions[1];

      if(config.nonOptions.size() > 2) m_keys[2] = config.nonOptions[2];
      
      if(config.nonOptions.size() > 3) m_keys[3] = config.nonOptions[3];

      config(m_autoscale, "autoscale");
      config(m_nofpsgage, "nofpsgage");
      
      config(m_targetXc, "targetXc");
      config(m_targetYc, "targetYc");
      
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
   
   //Run the app to get config, and check if help called
   if( rtimv.main(argc,argv) != 0)
   {
      return 0;
   }

   //If we don't have at least the first image file, we can't go on.
   if(rtimv.m_keys[0] == "")
   {
      std::cerr << "Must provide a SCExAO shared memory file\n";
      return -1;
   }
      
   imviewerForm imv(rtimv.m_keys);

   
   if(rtimv.m_autoscale)
   {
      imv.setAutoScale(true);
   }
   
   
   if(rtimv.m_nofpsgage)
   {
      imv.showFPSGage(false);
   }
   
   imv.targetXc(rtimv.m_targetXc);
   imv.targetYc(rtimv.m_targetYc);
   

   
   
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
