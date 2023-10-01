#include "rtimvMainWindow.hpp"

#define RTIMV_EDGETOL_DEFAULT (7.5)
#define RTIMV_TOOLLINEWIDTH_DEFAULT (0.75)

rtimvMainWindow::rtimvMainWindow( int argc,
                                  char ** argv,
                                  QWidget * Parent, 
                                  Qt::WindowFlags f
                                ) : rtimvBase(Parent, f)
{
   m_configPathCLBase_env = "RTIMV_CONFIG_PATH"; //Tells mx::application to look for this env var.
   
   setup(argc,argv);
   
   if(doHelp)
   {
      help();
      exit(0);
   }
   
   ui.setupUi(this);
   
   m_northArrow =0;
   
   imcp = 0;
   pointerOverZoom = 4.;
   
   resize(height(), height()); //make square.
   
   
   //This will come up at some minimal size.
   ui.graphicsView->setGeometry(0,0, width(), height());
   
   m_qgs = new QGraphicsScene();
   ui.graphicsView->setScene(m_qgs);
   
   m_colorBox = 0;
   
   rightClickDragging = false;

   m_nullMouseCoords = true;
   
   mindat(400);
   
   maxdat(600);
   
   m_targetVisible = false;
   
   m_cenLineVert = 0;
   m_cenLineHorz = 0;
   
   imStats = 0;
   m_timer.start(m_timeout);

   m_northArrow = m_qgs->addLine(QLineF(512,400, 512, 624), QColor(RTIMV_DEF_GAGEFONTCOLOR));
   m_northArrowTip = m_qgs->addLine(QLineF(512,400, 536, 424), QColor(RTIMV_DEF_GAGEFONTCOLOR));
   m_northArrow->setTransformOriginPoint ( QPointF(512,512) );
   m_northArrowTip->setTransformOriginPoint ( QPointF(512,512) );
   m_northArrow->setVisible(false);
   m_northArrowTip->setVisible(false);
   
   QPen qp = m_northArrow->pen();
   qp.setWidth(5);

   m_northArrow->setPen(qp);
   m_northArrowTip->setPen(qp);
   
   m_lineHead = new QGraphicsEllipseItem;
   m_lineHead->setVisible(false);
   m_qgs->addItem(m_lineHead);
   
   m_objCenV = new QGraphicsLineItem;
   m_objCenV->setVisible(false);
   m_qgs->addItem(m_objCenV);
   
   m_objCenH = new QGraphicsLineItem;
   m_objCenH->setVisible(false);
   m_qgs->addItem(m_objCenH);
   
   /* ========================================= */
   /* now load plugins                          */
   /* ========================================= */
   
   /* -- static plugins -- */
   const auto staticInstances = QPluginLoader::staticInstances();
   
   for (QObject *plugin : staticInstances)
   {
      static_cast<void>(plugin);
      std::cerr << "loaded static plugins\n";
   }
   
   QDir pluginsDir = QDir(QCoreApplication::applicationDirPath());

   #if defined(Q_OS_WIN)
   if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
   {
      pluginsDir.cdUp();
   }
   #elif defined(Q_OS_MAC)
   if (pluginsDir.dirName() == "MacOS") 
   {
      pluginsDir.cdUp();
      pluginsDir.cdUp();
      pluginsDir.cdUp();
   }
   #endif
   
   if(pluginsDir.cd("plugins"))   
   {
      const auto entryList = pluginsDir.entryList(QDir::Files);
   
      for (const QString &fileName : entryList) 
      {
         QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
         QObject *plugin = loader.instance();
         if (plugin) 
         {
            int arv = loadPlugin(plugin);
            if( arv != 0 )
            {
               if(!loader.unload())
               {
                  std::cerr << "rtimv: unloading an unused plugin failed\n";
               }
            }
            else
            {
               std::cerr << "rtimv: loaded plugin " << fileName.toStdString() << "\n";
               m_pluginFileNames += fileName;
            }
         }
      }
   }
   
   setWindowTitle(m_title.c_str());
}

rtimvMainWindow::~rtimvMainWindow()
{
   if(imStats) delete imStats;
}

void rtimvMainWindow::setupConfig()
{
   config.add("image.key", "", "image.key", argType::Required, "image", "key", false, "string", "The main image key. Specifies the protocol, location, and name of the main image.");
   config.add("image.shmim_name", "", "image.shmim_name", argType::Required, "image", "shmim_name", false, "string", "Same as image.key. Deprecated -- do not use for new configs.");
   
   config.add("dark.key", "", "dark.key", argType::Required, "dark", "key", false, "string", "The dark image key. Specifies the protocol, location, and name of the dark image.");   
   config.add("dark.shmim_name", "", "dark.shmim_name", argType::Required, "dark", "shmim_name", false, "string", "Same as dark.key. Deprecated -- do not use for new configs.");
   
   config.add("mask.key", "", "mask.key", argType::Required, "mask", "key", false, "string", "The mask image key. Specifies the protocol, location, and name of the mask image.");   
   config.add("mask.shmim_name", "", "mask.shmim_name", argType::Required, "mask", "shmim_name", false, "string", "Same as mask.key. Deprecated -- do not use for new configs.");
   
   config.add("satMask.key", "", "satMask.key", argType::Required, "satMask", "key", false, "string", "The saturation mask image key. Specifies the protocol, location, and name of the saturation mask image.");
   config.add("satMask.shmim_name", "", "satMask.shmim_name", argType::Required, "satMask", "shmim_name", false, "string", "Same as satMask.key. Deprecated -- do not use for new configs.");

   config.add("autoscale", "", "autoscale", argType::True, "", "autoscale", false, "bool", "Set to turn autoscaling on at startup");
   config.add("nofpsgage", "", "nofpsgage", argType::True, "", "nofpsgage", false, "bool", "Set to turn the fps gage off at startup");
   config.add("darksub", "", "darksub", argType::True, "", "darksub", false, "bool", "Set to false to turn off on at startup.  If a dark is supplied, darksub is otherwise on.");
   config.add("targetXc", "", "targetXc", argType::Required, "", "targetXc", false, "float", "The fractional x-coordinate of the target, 0<= x <=1");
   config.add("targetYc", "", "targetYc", argType::Required, "", "targetYc", false, "float", "The fractional y-coordinate of the target, 0<= y <=1");

   config.add("mouse.pointerCoords", "", "mouse.pointerCoords", argType::Required, "mouse", "pointerCoords", false, "bool", "Show or don't show the pointer coordinates.  Default is true.");
   config.add("mouse.staticCoords", "", "mouse.staticCoords", argType::Required, "mouse", "staticCoords", false, "bool", "Show or don't show the static coordinates at bottom of display.  Default is false.");

   config.add("mzmq.always", "Z", "mzmq.always", argType::True, "mzmq", "always", false, "bool", "Set to make milkzmq the protocol for bare image names.  Note that local shmims can not be used if this is set.");
   config.add("mzmq.server", "s", "mzmq.server", argType::Required, "mzmq", "server", false, "string", "The default server for milkzmq.  The default default is localhost.  This will be overridden by an image specific server specified in a key.");
   config.add("mzmq.port", "p", "mzmq.port", argType::Required, "mzmq", "port", false, "int", "The default port for milkzmq.  The default default is 5556.  This will be overridden by an image specific port specified in a key.");
   
   config.add("north.enabled", "", "north.enabled", argType::Required, "north", "enabled", false, "bool", "Whether or not to enable the north arrow. Default is true.");
   config.add("north.offset", "", "north.offset", argType::Required, "north", "offset", false, "float", "Offset in degrees c.c.w. to apply to the north angle. Default is 0.");
   config.add("north.scale", "", "north.scale", argType::Required, "north", "scale", false, "float", "Scaling factor to apply to north angle to convert to degrees c.c.w. on the image.  Default is -1.");
}

void rtimvMainWindow::loadConfig()
{
   std::string imKey;
   std::string darkKey;
   
   std::string flatKey;
   
   std::string maskKey;
  
   std::string satMaskKey;
   
   std::vector<std::string> keys;
   
   //Set up milkzmq
   config(m_mzmqAlways, "mzmq.always");
   config(m_mzmqServer, "mzmq.server");
   config(m_mzmqPort, "mzmq.port");

   
   //Check for use of deprecated shmim_name keyword by itself, but use key if available
   if(!config.isSet("image.key")) config(imKey, "image.shmim_name");
   else config(imKey, "image.key");
   
   if(!config.isSet("dark.key")) config(darkKey, "dark.shmim_name");
   else config(darkKey, "dark.key");
      
   if(!config.isSet("mask.key")) config(maskKey, "mask.shmim_name");
   else config(maskKey, "mask.key");
   
   if(!config.isSet("satMask.key")) config(satMaskKey, "satMask.shmim_name");
   else config(satMaskKey, "satMask.key");
   
   //Populate the key vector, a "" means no image specified
   keys.resize(4);
      
   if(imKey != "") keys[0] = imKey;
   if(darkKey != "") keys[1] = darkKey;
   if(maskKey != "") keys[2] = maskKey;
   if(satMaskKey != "") keys[3] = satMaskKey;
   
   //The command line always overrides the config
   if(config.nonOptions.size() > 0) keys[0] = config.nonOptions[0];
   if(config.nonOptions.size() > 1) keys[1] = config.nonOptions[1];
   if(config.nonOptions.size() > 2) keys[2] = config.nonOptions[2];
   if(config.nonOptions.size() > 3) keys[3] = config.nonOptions[3];
   
   startup(keys);
   
   if(m_images[0] == nullptr)
   {
      if(doHelp)
      {
         help();
      }
      else
      {
         std::cerr << "rtimv: No valid image specified so cowardly refusing to start.  Use -h for help.\n";
      }
      
      exit(0);
   }
   else
   {
      m_title = m_images[0]->imageName();
   }
   
   //Now load remaining options, respecting coded defaults.
   config(m_autoScale, "autoscale");
   config(m_subtractDark, "darksub");

   bool nofpsgage = !m_showFPSGage;
   config(nofpsgage, "nofpsgage");
   m_showFPSGage = !nofpsgage;

   config(m_targetXc, "targetXc");
   config(m_targetYc, "targetYc");
   
   config(m_showToolTipCoords, "mouse.pointerCoords");
   config(m_showStaticCoords, "mouse.staticCoords");

   config(m_northArrowEnabled, "north.enabled");
   config(m_northAngleOffset, "north.offset");
   config(m_northAngleScale, "north.scale");

}

void rtimvMainWindow::onConnect()
{
   setWindowTitle(m_title.c_str());
   

   squareDown();
}

void rtimvMainWindow::postSetImsize()
{      
   ScreenZoom = std::min((float) ui.graphicsView->viewport()->width()/(float)m_nx,(float)ui.graphicsView->viewport()->height()/(float)m_ny);
   zoomLevel(1.0);
   set_viewcen(.5, .5);
   post_zoomLevel();
   
   if(imcp)
   {
      QTransform transform;
      float viewZoom = (float)imcp->ui.viewView->width()/(float)m_nx;
      
      transform.scale(viewZoom, viewZoom);
      imcp->ui.viewView->setTransform(transform);
   }
   
   //resize the boxes
   float w;
   if(m_nx < m_ny) w = m_nx/4;
   else w = m_ny/4;
   float xc = 0.5*(m_nx)-w/2;
   float yc = 0.5*(m_ny)-w/2;

   std::unordered_set<StretchBox *>::iterator ubit = m_userBoxes.begin();
   while(ubit != m_userBoxes.end())
   {
      StretchBox *sb = *ubit;
      sb->setRect(sb->mapRectFromScene(xc, yc, w, w));
      xc += (RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom)*10;
      yc += (RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom)*10;
      sb->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
      sb->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
      ++ubit;
   }
   
   if(m_statsBox) statsBoxMoved(m_statsBox);
   if(m_colorBox) colorBoxMoved(m_colorBox);
   
   //resize the circles 
   std::unordered_set<StretchCircle *>::iterator ucit = m_userCircles.begin();
   while(ucit != m_userCircles.end())
   {
      StretchCircle *sc = *ucit;
      sc->setRect(sc->mapRectFromScene(xc, yc, w, w));
      xc += (RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom)*10;
      yc += (RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom)*10;
      sc->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
      sc->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
      ++ucit;
   }
     
   //resize the lines
   std::unordered_set<StretchLine *>::iterator ulit = m_userLines.begin();
   while(ulit != m_userLines.end())
   {
      StretchLine *sl = *ulit;
      sl->setLine(0.5*(m_nx)-0.2*m_nx,0.5*(m_ny)-0.2*m_ny, 0.2*(m_nx),0.2*(m_ny));
      sl->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
      sl->setPenWidth(2*RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
      ++ulit;
   }
   
   
   setTarget();
}


void rtimvMainWindow::post_zoomLevel()
{
   QTransform transform;
   
   ui.graphicsView->zoomLevel(m_zoomLevel);
   ui.graphicsView->screenZoom(ScreenZoom);
   
   transform.scale(m_zoomLevel*ScreenZoom, m_zoomLevel*ScreenZoom);
   
   ui.graphicsView->setTransform(transform);
   transform.scale(pointerOverZoom, pointerOverZoom);
   if(imcp) imcp->ui.pointerView->setTransform(transform);
   change_center();
   
   char zlstr[16];
   snprintf(zlstr,16, "%0.1fx", m_zoomLevel);
   ui.graphicsView->zoomText(zlstr);
   fontLuminance(ui.graphicsView->m_zoomText);
   
}

void rtimvMainWindow::postChangeImdata()
{
   //if(fps_ave > 1.0) ui.graphicsView->fpsGageText( fps_ave );
  
   if(saturated)
   {
      ui.graphicsView->warningText("Saturated!");
   }
   else
   {
      ui.graphicsView->warningText("");
   }
   
   if(!m_qpmi) //This happens on first time through
   {
      m_qpmi = m_qgs->addPixmap(m_qpm);
      //So we need to initialize the viewport center, etc.
      set_viewcen(0.5,0.5);
      post_zoomLevel();
   }
   else m_qpmi->setPixmap(m_qpm);
        
   if(m_colorBox) m_qpmi->stackBefore(m_colorBox);
   if(m_statsBox) m_qpmi->stackBefore(m_statsBox);
   if(m_objCenH) m_qpmi->stackBefore(m_objCenH);
   if(m_objCenV) m_qpmi->stackBefore(m_objCenV);
   if(m_lineHead) m_qpmi->stackBefore(m_lineHead);
   if(m_northArrow) m_qpmi->stackBefore(m_northArrow);
   if(m_northArrowTip) m_qpmi->stackBefore(m_northArrowTip);

   if(imcp)
   {
      if(imcp->ViewViewMode == ViewViewEnabled)
      {
         if(!imcp->qpmi_view) imcp->qpmi_view = imcp->qgs_view->addPixmap(m_qpm);
         imcp->qpmi_view->setPixmap(m_qpm);
         
         imcp->qpmi_view->stackBefore(imcp->viewLineVert);
      }
   }
   updateMouseCoords(); //This is to update the pixel val box if set.
   
   if(imcp)
   {
      imcp->update_panel();
   }
   
   if(imStats) 
   {
      imStats->setImdata();
   }

   
}

void rtimvMainWindow::launchControlPanel()
{
   if(!imcp)
   {
      imcp = new rtimvControlPanel(this, Qt::Tool);
      connect(imcp, SIGNAL(launchStatsBox()), this, SLOT(doLaunchStatsBox()));
      connect(imcp, SIGNAL(hideStatsBox()), this, SLOT(doHideStatsBox()));
   }
   
   imcp->show();
   
   imcp->activateWindow();
}

void rtimvMainWindow::centerNorthArrow()
{
   if(m_northArrow && m_northArrowTip)
   {
      m_northArrow->setLine(ui.graphicsView->xCen(), ui.graphicsView->yCen()-.1*m_ny/m_zoomLevel, ui.graphicsView->xCen(), ui.graphicsView->yCen()+.1*m_ny/m_zoomLevel);
      
      m_northArrow->setTransformOriginPoint ( QPointF(ui.graphicsView->xCen(),ui.graphicsView->yCen()) );
         
      m_northArrowTip->setLine(QLineF(ui.graphicsView->xCen(),ui.graphicsView->yCen()-.1*m_ny/m_zoomLevel, ui.graphicsView->xCen() + .02*m_nx/m_zoomLevel,ui.graphicsView->yCen()-.1*m_ny/m_zoomLevel + .012*m_ny/m_zoomLevel));
      m_northArrowTip->setTransformOriginPoint (  QPointF(ui.graphicsView->xCen(),ui.graphicsView->yCen()) );

      QPen qp = m_northArrow->pen();
   
      float wid = 5/(m_zoomLevel*ScreenZoom);
      if(wid > 3) wid = 3;
      qp.setWidth(wid);

      m_northArrow->setPen(qp);
      m_northArrowTip->setPen(qp);
   }

   updateNorthArrow();
}

void rtimvMainWindow::updateNorthArrow()
{
   if(m_northArrow && m_northArrowTip)
   {
      float ang = northAngle();
      m_northArrow->setRotation(ang);
      m_northArrowTip->setRotation(ang);
   }
}

float rtimvMainWindow::northAngle()
{
   float north = 0;
   if(m_dictionary.count("rtimv.north.angle") > 0)
   {
      m_dictionary["rtimv.north.angle"].getBlob((char *) &north, sizeof(float));
      
      north = -1*(m_northAngleOffset + m_northAngleScale*north); //negative because QT is c.w.
   }

   return north;
} 

void rtimvMainWindow::northAngleRaw(float north)
{
   m_dictionary["rtimv.north.angle"].setBlob((char *) &north, sizeof(float));
}

QGraphicsScene * rtimvMainWindow::get_qgs()
{
   return m_qgs;
}

void rtimvMainWindow::freezeRealTime()
{
   if(RealTimeStopped)
   {
      set_RealTimeStopped(false);
   }
   else
   {
      set_RealTimeStopped(true);
      if(m_showFPSGage) ui.graphicsView->fpsGageText(0.0);
   }
}

void rtimvMainWindow::reStretch()
{
   if(get_colorbar_mode() == user)
   {
      set_colorbar_mode(minmaxglobal);
   }
   
   if(get_colorbar_mode() == minmaxglobal)
   {
      mindat(get_imdat_min());
      maxdat(get_imdat_max());
      changeImdata(false);
   }

   if(get_colorbar_mode() == minmaxbox)
   {
      mindat(colorBox_min);
      maxdat(colorBox_max);
      changeImdata(false);
   }
}



void rtimvMainWindow::setPointerOverZoom(float poz)
{
   pointerOverZoom = poz;
   post_zoomLevel();
}



void rtimvMainWindow::change_center(bool movezoombox)
{   
   ui.graphicsView->centerOn(ui.graphicsView->xCen(), ui.graphicsView->yCen());
   
   centerNorthArrow();

   if(imcp)
   {
      
      imcp->viewLineVert->setLine(ui.graphicsView->xCen(), 0, ui.graphicsView->xCen(), m_ny);
      imcp->viewLineHorz->setLine(0, ui.graphicsView->yCen(), m_nx, ui.graphicsView->yCen());
      
      if(m_zoomLevel <= 1.0) imcp->viewBox->setVisible(false);
      else
      {
         imcp->viewBox->setVisible(true);
         if(movezoombox)
         {
            QPointF tmpp = imcp->viewBox->mapFromParent(ui.graphicsView->xCen() - .5*m_nx/m_zoomLevel, ui.graphicsView->yCen()-.5*m_ny/m_zoomLevel);
            imcp->viewBox->setRect(tmpp.x(), tmpp.y(), m_nx/m_zoomLevel, m_ny/m_zoomLevel);
         }
         
      }
      imcp->ui.viewView->centerOn(.5*m_nx, .5*m_ny);
      imcp->update_panel();
   }
}

void rtimvMainWindow::set_viewcen(float x, float y, bool movezoombox)
{
   QPointF sp( x* m_qpmi->boundingRect().width(), y*m_qpmi->boundingRect().height() );
   QPointF vp = ui.graphicsView->mapFromScene(sp);
   
   ui.graphicsView->mapCenterToScene(vp.x(), vp.y());
   change_center(movezoombox);
}

void rtimvMainWindow::squareDown()
{
   double imrat = ((double)nx())/ny();
   double winrat = ((double) width())/height();
   
   ///\todo make threshold responsive to current dimensions so we don't enter loops.
   if( fabs( 1.0 - imrat/winrat) < 0.01) return;
   
   if(width() <= height())
   {
      resize(width(), width()/imrat);
   }
   else
   {
      resize(height()*imrat, height());
   }
}

void rtimvMainWindow::squareUp()
{
   double imrat = ((double)nx())/ny();
   double winrat = ((double) width())/height();
   
   ///\todo make threshold responsive to current dimensions so we don't enter loops.
   if( fabs( 1.0 - imrat/winrat) < 0.01) return;
   
   if(width() <= height())
   {
      resize(height()*imrat, height());
   }
   else
   {
      resize(width(), width()/imrat);            
   }
}
      
void rtimvMainWindow::changeCenter()
{
   change_center();
}

void rtimvMainWindow::resizeEvent(QResizeEvent *)
{
   ScreenZoom = std::min((float)width()/(float)m_nx,(float)height()/(float)m_ny);
   change_center();
   ui.graphicsView->setGeometry(0,0,width(), height());
   post_zoomLevel();
   
}

void rtimvMainWindow::mouseMoveEvent(QMouseEvent *e)
{
   nullMouseCoords();
   (void)(e);
}

void rtimvMainWindow::nullMouseCoords()
{
   if(!m_nullMouseCoords)
   {
      if(imcp)
      {
         imcp->nullMouseCoords();
      }
      
      m_nullMouseCoords = true;
      
      ui.graphicsView->textCoordX("");
      ui.graphicsView->textCoordY("");
      ui.graphicsView->textPixelVal("");

      ui.graphicsView->hideMouseToolTip();
   }
}

void rtimvMainWindow::updateMouseCoords()
{
   int64_t idx_x, idx_y; //image size are uint32_t, so this allows signed comparison without overflow issues
   
   if(!m_qpmi) return;
   
   if(ui.graphicsView->mouseViewX() < 0 || ui.graphicsView->mouseViewY() < 0)
   {
      nullMouseCoords();
   }
   
   QPointF pt = ui.graphicsView->mapToScene(ui.graphicsView->mouseViewX(),ui.graphicsView->mouseViewY());
   
   float mx = pt.x();
   float my = pt.y();
   
   if( mx < 0 || mx > m_qpmi->boundingRect().width() || my < 0 || my > m_qpmi->boundingRect().height() ) 
   {
      nullMouseCoords();
   }
   
   if(m_userItemSelected)
   {
      nullMouseCoords();
      userItemMouseCoords(m_userItemMouseViewX, m_userItemMouseViewY);
   }

   if(!m_nullMouseCoords)
   {
      idx_x = ((int64_t)(mx-0));
      if(idx_x < 0) idx_x = 0;
      if(idx_x > (int64_t) m_nx-1) idx_x = m_nx-1;
      
      idx_y = (int)(m_qpmi->boundingRect().height() - (my-0));
      if(idx_y < 0) idx_y = 0;
      if(idx_y > (int64_t) m_ny-1) idx_y = m_ny-1;

      pixelF _pixel = pixel();
      
      float val = 0;
      if(_pixel != nullptr) val = _pixel(this,  (int)(idx_y*m_nx) + (int)(idx_x));
      
      if(m_showStaticCoords)
      {
         ui.graphicsView->textCoordX(mx-0.5);
         ui.graphicsView->textCoordY(m_qpmi->boundingRect().height() - my-0.5);
         ui.graphicsView->textPixelVal( val );
      }

      if(m_showToolTipCoords)
      {
         char valStr[32];
         char posStr[32];
         
         if(fabs(val) < 1e-1)
         {
            snprintf(valStr, sizeof(valStr), "%0.04g", val);
         }
         else
         {
            snprintf(valStr, sizeof(valStr), "%0.02f", val);
         }

         snprintf(posStr, sizeof(posStr), "%0.2f %0.2f", mx-0.5, m_qpmi->boundingRect().height() - my-0.5 );

         ui.graphicsView->showMouseToolTip(valStr, posStr, QPoint(ui.graphicsView->mouseViewX(),ui.graphicsView->mouseViewY()));
         fontLuminance(ui.graphicsView->m_mouseCoords);
      }

      if(imcp)
      {
         if(_pixel != nullptr)
         imcp->updateMouseCoords(mx, my, _pixel(this, idx_y*m_nx + idx_x) );
      }
   }
   
   //Adjust bias and contrast
   if(rightClickDragging)
   {
      float dx = ui.graphicsView->mouseViewX() - rightClickStart.x();
      float dy = ui.graphicsView->mouseViewY() - rightClickStart.y();
      
      float dbias = dx/ui.graphicsView->viewport()->width();
      float dcontrast = -1.*dy/ui.graphicsView->viewport()->height();
      
      bias(biasStart + dbias*.5*(imdat_max+imdat_min));
      contrast(contrastStart + dcontrast*(imdat_max-imdat_min));
      if(!amChangingimdata) changeImdata();
   }
   
   
}

bool rtimvMainWindow::showToolTipCoords()
{
   return m_showToolTipCoords;
}

bool rtimvMainWindow::showStaticCoords()
{
   return m_showStaticCoords;
}

void rtimvMainWindow::showToolTipCoords(bool sttc)
{
   m_showToolTipCoords = sttc;
   emit showToolTipCoordsChanged(m_showToolTipCoords);
}

void rtimvMainWindow::showStaticCoords(bool ssc)
{
   m_showStaticCoords = ssc;
   emit showStaticCoordsChanged(m_showStaticCoords);
}


void rtimvMainWindow::changeMouseCoords()
{
   m_nullMouseCoords = false;
   updateMouseCoords();
}

void rtimvMainWindow::viewLeftPressed(QPointF mp)
{
   if(imcp)
   {
      imcp->viewLeftPressed(mp);
   }
}

void rtimvMainWindow::viewLeftClicked(QPointF mp)
{
   if(imcp)
   {
      imcp->viewLeftClicked(mp);
   }
}

void rtimvMainWindow::viewRightPressed(QPointF mp)
{
   rightClickDragging = true;
   
   rightClickStart = mp;//ui.graphicsView->mapToScene(mp.x(),mp.y());
   biasStart = bias();
   contrastStart = contrast();
   
}

void rtimvMainWindow::viewRightClicked(QPointF mp)
{
   rightClickDragging = false;  
   (void)(mp);
}

void rtimvMainWindow::onWheelMoved(int delta)
{
   float dz;
   if(delta > 0)   dz = 1.02;//1.41421;
   else dz = 0.98;//0.70711;
   
   zoomLevel(dz*zoomLevel());
   
}


void rtimvMainWindow::updateFPS()
{
   updateAge();
}

void rtimvMainWindow::updateAge()
{
   //Check the font luminance to make sure it is visible
   fontLuminance();

   if(m_showFPSGage && m_images[0] != nullptr  )
   {      
      struct timespec tstmp;
         
      clock_gettime(CLOCK_REALTIME, &tstmp);
            
      double timetmp = (double)tstmp.tv_sec + ((double)tstmp.tv_nsec)/1e9;
         
      double fpsTime = m_images[0]->imageTime();//m_image.md->atime.tv_sec + ((double) m_images[0]->m_image.md->atime.tv_nsec)/1e9;
         
      double age = timetmp - fpsTime;
            
      if(m_images[0]->fpsEst() > 1.0 && age < 2.0) 
      {
         ui.graphicsView->fpsGageText(m_images[0]->fpsEst());
      }
      else if(age < 86400*10000) //only if age is reasonable
      {
         ui.graphicsView->fpsGageText(age, true);
      }
      else  
      {
         ui.graphicsView->fpsGageText("");
      }
   }

   for(size_t n=0;n<m_overlays.size(); ++n)
   {
      m_overlays[n]->updateOverlay();
   }
   
   if(m_showLoopStat)
   {
      ui.graphicsView->loopText("Loop OPEN", "red");
   }

   if(m_northArrowEnabled)
   {
      updateNorthArrow();
   }
}

void rtimvMainWindow::updateNC()
{
   for(size_t n=0;n<m_overlays.size(); ++n)
   {
      m_overlays[n]->updateOverlay();
   }
}

void rtimvMainWindow::userBoxItemSize(StretchBox * sb)
{
   char tmp[256];
   snprintf(tmp, 256, "%0.1f x %0.1f", sb->rect().width(), sb->rect().height());
   ui.graphicsView->m_userItemSize->setText(tmp);
   
   QFontMetrics fm(ui.graphicsView->m_userItemSize->currentFont());
   QSize textSize = fm.size(0, tmp);

   QRectF sbr = sb->sceneBoundingRect();
   QPoint qr = ui.graphicsView->mapFromScene(QPointF(sbr.x(), sbr.y()));
 
   ui.graphicsView->m_userItemSize->resize(textSize.width()+5,textSize.height()+5);
   ui.graphicsView->m_userItemSize->move(qr.x(), qr.y());
   
   fontLuminance(ui.graphicsView->m_userItemSize);
}

void rtimvMainWindow::userBoxItemCoords(StretchBox * sb)
{
   m_userItemXCen = sb->rect().x() + sb->pos().x() + 0.5*sb->rect().width();
   m_userItemYCen = sb->rect().y() + sb->pos().y() + 0.5*sb->rect().height();

   m_objCenH->setPen(sb->pen());
   m_objCenV->setPen(sb->pen());

   float w = std::max(sb->rect().width(), sb->rect().height());
   if(w < 100) w = 100;
   m_objCenH->setLine(m_userItemXCen-w*0.05, m_userItemYCen, m_userItemXCen+w*0.05, m_userItemYCen);
   m_objCenV->setLine(m_userItemXCen, m_userItemYCen-w*0.05, m_userItemXCen, m_userItemYCen+w*0.05);
}

void rtimvMainWindow::userCircleItemSize(StretchCircle * sc)
{
   char tmp[32];
   snprintf(tmp, sizeof(tmp), "r=%0.1f", sc->radius());

   ui.graphicsView->m_userItemSize->setText(tmp);
   
   QFontMetrics fm(ui.graphicsView->m_userItemSize->currentFont());
   QSize textSize = fm.size(0, tmp);
   
   ui.graphicsView->m_userItemSize->setGeometry(sc->rect().x() + sc->pos().x() + sc->rect().width()*0.5 - sc->radius()*0.707, sc->rect().y() + sc->pos().y()+ sc->rect().height()*0.5 - sc->radius()*0.707, textSize.width()+5,textSize.height()+5);

   //Take scene coordinates to viewport coordinates.
   QRectF sbr = sc->sceneBoundingRect();
   QPoint qr = ui.graphicsView->mapFromScene(QPointF(sbr.x()+sc->rect().width()*0.5 - sc->radius()*0.707, sbr.y()+ sc->rect().height()*0.5 - sc->radius()*0.707));
 
   ui.graphicsView->m_userItemSize->resize(textSize.width()+5,textSize.height()+5);
   ui.graphicsView->m_userItemSize->move(qr.x(), qr.y());

   fontLuminance(ui.graphicsView->m_userItemSize);
}

void rtimvMainWindow::userCircleItemCoords(StretchCircle * sc)
{
   m_userItemXCen = sc->rect().x() + sc->pos().x() + 0.5*sc->rect().width();
   m_userItemYCen = sc->rect().y() + sc->pos().y() + 0.5*sc->rect().height();

   m_objCenH->setPen(sc->pen());
   m_objCenV->setPen(sc->pen());

   float w = std::max(sc->rect().width(), sc->rect().height());
   if(w < 100) w = 100;
   m_objCenH->setLine(m_userItemXCen-w*0.05, m_userItemYCen, m_userItemXCen+w*0.05, m_userItemYCen);
   m_objCenV->setLine(m_userItemXCen, m_userItemYCen-w*0.05, m_userItemXCen, m_userItemYCen+w*0.05);

}

void rtimvMainWindow::userItemMouseCoords( float mx,
                                           float my
                                         )
{
   int idx_x = ((int64_t)(mx-0));
   if(idx_x < 0) idx_x = 0;
   if(idx_x > (int64_t) m_nx-1) idx_x = m_nx-1;
      
   int idx_y = (int)(m_qpmi->boundingRect().height() - (my-0));
   if(idx_y < 0) idx_y = 0;
   if(idx_y > (int64_t) m_ny-1) idx_y = m_ny-1;

   pixelF _pixel = pixel();
      
   float val = 0;
   if(_pixel != nullptr) val = _pixel(this,  (int)(idx_y*m_nx) + (int)(idx_x));
      
   char valStr[32];
   char posStr[32];
   
   if(fabs(val) < 1e-1)
   {
      snprintf(valStr, sizeof(valStr), "%0.04g", val);
   }
   else
   {
      snprintf(valStr, sizeof(valStr), "%0.02f", val);
   }


   snprintf(posStr, sizeof(posStr), "%0.2f %0.2f", mx-0.5, m_qpmi->boundingRect().height() - my-0.5 );

   std::string str = std::string(valStr) + "\n" + posStr;
   ui.graphicsView->m_userItemMouseCoords->setPlainText(str.c_str());

   QFontMetrics fm(ui.graphicsView->m_userItemMouseCoords->currentFont());
   QSize textSize = fm.size(0, str.c_str());
   ui.graphicsView->m_userItemMouseCoords->setGeometry(mx, my, textSize.width()+5,textSize.height()+5);

   fontLuminance(ui.graphicsView->m_userItemMouseCoords);

}

void rtimvMainWindow::userBoxItemMouseCoords(StretchBox * sb)
{
   QRectF sbr = sb->sceneBoundingRect();
   QPointF qr = QPointF(sbr.x()+0.5*sbr.width(),sbr.y()+0.5*sbr.height()); 

   m_userItemMouseViewX = qr.x();
   m_userItemMouseViewY = qr.y();
   userItemMouseCoords(qr.x(), qr.y());
}

void rtimvMainWindow::userCircleItemMouseCoords(StretchCircle * sc)
{
   QRectF sbr = sc->sceneBoundingRect();
   QPointF qr = QPointF(sbr.x()+0.5*sbr.width(),sbr.y()+0.5*sbr.height());

   m_userItemMouseViewX = qr.x();
   m_userItemMouseViewY = qr.y();
   userItemMouseCoords(qr.x(), qr.y());
}

void rtimvMainWindow::addUserBox()
{
   float w;

   float znx = m_nx/m_zoomLevel;
   float zny = m_ny/m_zoomLevel;
   if(znx < zny) w = znx/4;
   else w = zny/4;
   
   std::pair<std::unordered_set<StretchBox *>::iterator,bool> it = m_userBoxes.insert(new StretchBox(ui.graphicsView->xCen()-w/2, ui.graphicsView->yCen()-w/2, w, w));
   
   StretchBox * sb = *it.first;
   
   sb->setPenColor("lime");
   sb->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
   sb->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
  
   sb->setMaintainCenter(true);
   sb->setStretchable(true);
   sb->setVisible(true);
   
   connect(sb, SIGNAL(resized(StretchBox *)), this, SLOT(userBoxResized(StretchBox *)));
   connect(sb, SIGNAL(moved(StretchBox *)), this, SLOT(userBoxMoved(StretchBox *)));
   connect(sb, SIGNAL(mouseIn(StretchBox *)), this, SLOT(userBoxMouseIn(StretchBox *)));
   connect(sb, SIGNAL(mouseOut(StretchBox *)), this, SLOT(userBoxMouseOut(StretchBox *)));
   connect(sb, SIGNAL(rejectMouse(StretchBox *)), this, SLOT(userBoxRejectMouse(StretchBox *)));
   connect(sb, SIGNAL(remove(StretchBox*)), this, SLOT(userBoxRemove(StretchBox*)));
   connect(sb, SIGNAL(selected(StretchBox*)), this, SLOT(userBoxSelected(StretchBox*)));
   connect(sb, SIGNAL(deSelected(StretchBox*)), this, SLOT(userBoxDeSelected(StretchBox*)));
   
   m_qgs->addItem(sb);
      
}

void rtimvMainWindow::addUserCircle()
{
   float w;

   float znx = m_nx/m_zoomLevel;
   float zny = m_ny/m_zoomLevel;
   if(znx < zny) w = znx/4;
   else w = zny/4;
   
   std::pair<std::unordered_set<StretchCircle *>::iterator,bool> it = m_userCircles.insert(new StretchCircle(ui.graphicsView->xCen()-w/2, ui.graphicsView->yCen()-w/2, w, w));
   
   StretchCircle * sc = *it.first;
   
   sc->setPenColor("lime");
   sc->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
   sc->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
   
   sc->setStretchable(true);
   sc->setVisible(true);
   
   connect(sc, SIGNAL(resized(StretchCircle *)), this, SLOT(userCircleResized(StretchCircle *)));
   connect(sc, SIGNAL(moved(StretchCircle *)), this, SLOT(userCircleMoved(StretchCircle *)));
   connect(sc, SIGNAL(mouseIn(StretchCircle *)), this, SLOT(userCircleMouseIn(StretchCircle *)));
   connect(sc, SIGNAL(mouseOut(StretchCircle *)), this, SLOT(userCircleMouseOut(StretchCircle *)));
   connect(sc, SIGNAL(rejectMouse(StretchCircle *)), this, SLOT(userCircleRejectMouse(StretchCircle *)));
   connect(sc, SIGNAL(remove(StretchCircle*)), this, SLOT(userCircleRemove(StretchCircle*)));
   connect(sc, SIGNAL(selected(StretchCircle *)), this, SLOT(userCircleSelected(StretchCircle *)));
   connect(sc, SIGNAL(deSelected(StretchCircle*)), this, SLOT(userCircleDeSelected(StretchCircle*)));
   
   m_qgs->addItem(sc);
      
}

void rtimvMainWindow::addUserLine()
{
   float w;

   float znx = m_nx/m_zoomLevel;
   float zny = m_ny/m_zoomLevel;
   if(znx < zny) w = znx/4;
   else w = zny/4;
   
   std::pair<std::unordered_set<StretchLine *>::iterator,bool> it = m_userLines.insert(new StretchLine(ui.graphicsView->xCen()-w/2, ui.graphicsView->yCen()-w/2, ui.graphicsView->xCen()+w/2, ui.graphicsView->yCen()+w/2));
   
   StretchLine * sl = *it.first;
   
   sl->setPenColor("lime");
   sl->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
   sl->setPenWidth(2*RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
   
   sl->setStretchable(true);
   sl->setVisible(true);
   
   connect(sl, SIGNAL(resized(StretchLine *)), this, SLOT(userLineResized(StretchLine *)));
   connect(sl, SIGNAL(moved(StretchLine *)), this, SLOT(userLineMoved(StretchLine *)));
   connect(sl, SIGNAL(mouseIn(StretchLine *)), this, SLOT(userLineMouseIn(StretchLine *)));
   connect(sl, SIGNAL(mouseOut(StretchLine *)), this, SLOT(userLineMouseOut(StretchLine *)));
   connect(sl, SIGNAL(rejectMouse(StretchLine *)), this, SLOT(userLineRejectMouse(StretchLine *)));
   connect(sl, SIGNAL(remove(StretchLine*)), this, SLOT(userLineRemove(StretchLine*)));
   connect(sl, SIGNAL(selected(StretchLine *)), this, SLOT(userLineSelected(StretchLine *)));
   connect(sl, SIGNAL(deSelected(StretchLine*)), this, SLOT(userLineDeSelected(StretchLine*)));

   
   m_qgs->addItem(sl);
      
}

float rtimvMainWindow::targetXc()
{
   return m_targetXc;
}

float rtimvMainWindow::targetYc()
{
   return m_targetYc;
}

bool rtimvMainWindow::targetVisible()
{
   return m_targetVisible;
}

void rtimvMainWindow::targetXc( float txc )
{
   if(txc < 0) txc = 0;
   if(txc > 1) txc = 1;
   
   m_targetXc = txc;
   
   setTarget();
   
   emit targetXcChanged(m_targetXc);

}
     
void rtimvMainWindow::targetYc( float tyc )
{
   if(tyc < 0) tyc = 0;
   if(tyc > 1) tyc = 1;
   
   m_targetYc = tyc;
   
   setTarget();
   
   emit targetYcChanged(m_targetYc);

}
     
void rtimvMainWindow::targetVisible(bool tv)
{
   if(m_targetVisible != tv)
   {
      if(tv)
      {
         ui.graphicsView->zoomText("target on");
         fontLuminance(ui.graphicsView->m_zoomText);
      }
      else
      {
         ui.graphicsView->zoomText("target off");
         fontLuminance(ui.graphicsView->m_zoomText);
      }
   }
   m_targetVisible = tv;
   setTarget();
   emit targetVisibleChanged(m_targetVisible);
}

void rtimvMainWindow::setTarget()
{
   if(!m_cenLineVert)
   {
      m_cenLineVert = m_qgs->addLine(QLineF(m_targetXc*nx(),0, m_targetXc*nx(), ny()), QColor("lime"));
      m_cenLineHorz = m_qgs->addLine(QLineF(0, (1.0-m_targetYc)*ny(), nx(), (1.0-m_targetYc)*ny()), QColor("lime"));
      if(m_targetVisible)
      {
         m_cenLineVert->setVisible(true);
         m_cenLineHorz->setVisible(true);
      }
      else
      {
         m_cenLineVert->setVisible(false);
         m_cenLineHorz->setVisible(false);
      } 
   }
   else
   {
      m_cenLineVert->setLine(QLineF(m_targetXc*nx(),0, m_targetXc*nx(), ny()));
      m_cenLineHorz->setLine(QLineF(0, (1.0-m_targetYc)*ny(), nx(), (1.0-m_targetYc)*ny()));
      if(m_targetVisible)
      {
         m_cenLineVert->setVisible(true);
         m_cenLineHorz->setVisible(true);
      }
      else   //ui.graphicsView->m_userItemSize->setGeometry(sb->rect().x() + sb->pos().x(), sb->rect().y() + sb->pos().y(), 200,40);

      {
         m_cenLineVert->setVisible(false);
         m_cenLineHorz->setVisible(false);
      }
   }
}

void rtimvMainWindow::addStretchBox(StretchBox * sb)
{
   if(sb == nullptr) return;
   
   m_userBoxes.insert(sb);
   
   connect(sb, SIGNAL(rejectMouse(StretchBox *)), this, SLOT(userBoxRejectMouse(StretchBox *)));
   connect(sb, SIGNAL(remove(StretchBox*)), this, SLOT(userBoxRemove(StretchBox*)));
      
   m_qgs->addItem(sb);
}

void rtimvMainWindow::addStretchCircle(StretchCircle * sc)
{
   if(sc == nullptr) return;
   
   m_userCircles.insert(sc);
   
   connect(sc, SIGNAL(rejectMouse(StretchCircle *)), this, SLOT(userCircleRejectMouse(StretchCircle *)));
   connect(sc, SIGNAL(remove(StretchCircle*)), this, SLOT(userCircleRemove(StretchCircle*)));
      
   m_qgs->addItem(sc);
}

void rtimvMainWindow::addStretchLine(StretchLine * sl)
{
   if(sl == nullptr) return;
   
   m_userLines.insert(sl);
   
   connect(sl, SIGNAL(rejectMouse(StretchLine *)), this, SLOT(userLineRejectMouse(StretchLine *)));
   connect(sl, SIGNAL(remove(StretchLine*)), this, SLOT(userLineRemove(StretchLine*)));
   
   m_qgs->addItem(sl);
}

void rtimvMainWindow::doLaunchStatsBox()
{
   if(!m_statsBox) return;

   m_statsBox->setVisible(true);
   
   if(!imStats)
   {
      imStats = new rtimvStats(this, this, Qt::WindowFlags());
      imStats->setAttribute(Qt::WA_DeleteOnClose); //Qt will delete imstats when it closes.
      connect(imStats, SIGNAL(finished(int )), this, SLOT(imStatsClosed(int )));
   }

   statsBoxMoved(m_statsBox);

   imStats->show();
    
   imStats->activateWindow();
   
}

void rtimvMainWindow::doHideStatsBox()
{
   if(m_statsBox) m_statsBox->setVisible(false);

   if (imStats)
   {
      delete imStats;
      imStats = 0; //imStats is set to delete on close
   }

}

void rtimvMainWindow::imStatsClosed(int result)
{
   static_cast<void>(result);
   
   if(!m_statsBox) return;

   m_statsBox->setVisible(false);
   imStats = 0; //imStats is set to delete on close
   if(imcp)
   {
      imcp->statsBoxButtonState = false;
      imcp->ui.statsBoxButton->setText("Show Stats Box");
   }
//    imcp->on_m_statsBoxButton_clicked();
   
}

void rtimvMainWindow::statsBoxMoved(StretchBox * sb)
{
   static_cast<void>(sb);
   
   if(!m_statsBox) return;

   QPointF np = m_qpmi->mapFromItem(m_statsBox, QPointF(m_statsBox->rect().x(),m_statsBox->rect().y()));
   QPointF np2 = m_qpmi->mapFromItem(m_statsBox, QPointF(m_statsBox->rect().x()+m_statsBox->rect().width(),m_statsBox->rect().y()+m_statsBox->rect().height()));

   if(imStats) 
   {
      imStats->setImdata(m_nx, m_ny, np.x(), np2.x(), m_ny-np2.y(), m_ny-np.y());
   }
}


void rtimvMainWindow::colorBoxMoved(StretchBox * sb)
{
   if(!m_colorBox) return;
   
   QRectF newr = sb->rect();
   
   QPointF np = m_qpmi->mapFromItem(m_colorBox, QPointF(newr.x(),newr.y()));
   QPointF np2 = m_qpmi->mapFromItem(m_colorBox, QPointF(newr.x()+newr.width(),newr.y()+newr.height()));

   colorBox_i1 = (int) (np2.x() + .5);
   colorBox_i0 = (int) np.x();
   colorBox_j0 = m_ny-(int) (np2.y() + .5);
   colorBox_j1 = m_ny-(int) np.y();
   
   setUserBoxActive(true); //recalcs and recolors.
}

void rtimvMainWindow::userBoxResized(StretchBox * sb)
{
   userBoxItemSize(sb);
   userBoxItemMouseCoords(sb);
   userBoxItemCoords(sb);
}

void rtimvMainWindow::userBoxMoved(StretchBox * sb)
{
   userBoxItemSize(sb);
   userBoxItemMouseCoords(sb);
   userBoxItemCoords(sb);
}

void rtimvMainWindow::userBoxMouseIn(StretchBox * sb)
{
   userBoxMoved(sb);
}

void rtimvMainWindow::userBoxMouseOut(StretchBox * sb)
{
   static_cast<void>(sb);
}

void rtimvMainWindow::userBoxRejectMouse(StretchBox * sb)
{
   std::unordered_set<StretchBox *>::iterator ubit = m_userBoxes.begin();
   while(ubit != m_userBoxes.end())
   {
      if(sb != *ubit) sb->stackBefore(*ubit);
      ++ubit;
   }
   
   std::unordered_set<StretchCircle *>::iterator ucit = m_userCircles.begin();
   while(ucit != m_userCircles.end())
   {
      sb->stackBefore(*ucit);
      ++ucit;
   }
   
   std::unordered_set<StretchLine *>::iterator ulit = m_userLines.begin();
   while(ulit != m_userLines.end())
   {
      sb->stackBefore(*ulit);
      ++ulit;
   }
}

void rtimvMainWindow::userBoxRemove(StretchBox * sb)
{
   if(sb == m_statsBox)
   {
      doHideStatsBox();
      return;
   }
   
   userBoxMouseOut(sb); //This cleans up any gui items associated with the box
   m_userBoxes.erase(sb); //Remove it from our list
   m_qgs->removeItem(sb); //Remove it from the scene
   sb->deleteLater(); //clean it up after we're no longer in an asynch function

   ui.graphicsView->m_userItemSize->setVisible(false);
   ui.graphicsView->m_userItemMouseCoords->setVisible(false);
   m_objCenH->setVisible(false);
   m_objCenV->setVisible(false);
   m_nullMouseCoords=false;
   m_userItemSelected = false;
}

void rtimvMainWindow::userBoxSelected(StretchBox * sb)
{
   static_cast<void>(sb);
   ui.graphicsView->m_userItemSize->setVisible(true);
   ui.graphicsView->m_userItemMouseCoords->setVisible(true);
   m_objCenH->setVisible(true);
   m_objCenV->setVisible(true);
   nullMouseCoords();
   m_userItemXCen = sb->rect().x() + sb->pos().x() + 0.5*sb->rect().width();
   m_userItemYCen = sb->rect().y() + sb->pos().y() + 0.5*sb->rect().height();
   m_userItemSelected = true;
}

void rtimvMainWindow::userBoxDeSelected(StretchBox * sb)
{
   static_cast<void>(sb);
   ui.graphicsView->m_userItemSize->setVisible(false);
   ui.graphicsView->m_userItemMouseCoords->setVisible(false);
   m_objCenH->setVisible(false);
   m_objCenV->setVisible(false);
   m_nullMouseCoords=false;
   m_userItemSelected = false;
}


void rtimvMainWindow::userCircleResized(StretchCircle * sc)
{
   userCircleItemSize(sc);
   userCircleItemMouseCoords(sc);
   userCircleItemCoords(sc);
}

void rtimvMainWindow::userCircleMoved(StretchCircle * sc)
{
   userCircleItemSize(sc);
   userCircleItemMouseCoords(sc);
   userCircleItemCoords(sc);
}

void rtimvMainWindow::userCircleMouseIn(StretchCircle * sc)
{
   userCircleResized(sc);
}

void rtimvMainWindow::userCircleMouseOut(StretchCircle * sc)
{
   static_cast<void>(sc);
}

void rtimvMainWindow::userCircleRejectMouse(StretchCircle * sc)
{  
   std::unordered_set<StretchBox *>::iterator ubit = m_userBoxes.begin();
   while(ubit != m_userBoxes.end())
   {
      sc->stackBefore(*ubit);
      ++ubit;
   }
   
   std::unordered_set<StretchCircle *>::iterator ucit = m_userCircles.begin();
   while(ucit != m_userCircles.end())
   {
      if(sc != *ucit) sc->stackBefore(*ucit);
      ++ucit;
   }   
   
   std::unordered_set<StretchLine *>::iterator ulit = m_userLines.begin();
   while(ulit != m_userLines.end())
   {
      sc->stackBefore(*ulit);
      ++ulit;
   }
}

void rtimvMainWindow::userCircleRemove(StretchCircle * sc)
{
   userCircleMouseOut(sc); //This cleans up any gui items associated with the circle
   m_userCircles.erase(sc); //Remove it from our list
   m_qgs->removeItem(sc); //Remove it from the scene
   sc->deleteLater(); //clean it up after we're no longer in an asynch function

   ui.graphicsView->m_userItemSize->setVisible(false);
   ui.graphicsView->m_userItemMouseCoords->setVisible(false);
   m_objCenH->setVisible(false);
   m_objCenV->setVisible(false);
   m_userItemSelected = false;
}

void rtimvMainWindow::userCircleSelected(StretchCircle * sc)
{
   ui.graphicsView->m_userItemSize->setVisible(true);
   ui.graphicsView->m_userItemMouseCoords->setVisible(true);
   m_objCenH->setVisible(true);
   m_objCenV->setVisible(true);
   m_userItemXCen = sc->rect().x() + sc->pos().x() + 0.5*sc->rect().width();
   m_userItemYCen = sc->rect().y() + sc->pos().y() + 0.5*sc->rect().height();
   m_userItemSelected = true;
}

void rtimvMainWindow::userCircleDeSelected(StretchCircle * sb)
{
   static_cast<void>(sb);
   ui.graphicsView->m_userItemSize->setVisible(false);
   ui.graphicsView->m_userItemMouseCoords->setVisible(false);
   m_objCenH->setVisible(false);
   m_objCenV->setVisible(false);
   m_userItemSelected = false;
}

void rtimvMainWindow::userLineResized(StretchLine * sl)
{
   userLineMoved(sl); //Move the text along with us.
   
   float ang = fmod(sl->angle() -90 + northAngle(), 360.0);

   if(ang < 0) ang += 360.0;
   
   char tmp[256];
   snprintf(tmp, 256, "%0.1f @ %0.1f", sl->length(), ang);
   
   ui.graphicsView->m_userItemSize->setText(tmp);

   m_userItemXCen = sl->line().x1();
   m_userItemYCen = sl->line().y1();

   QPointF qr = QPointF(sl->line().x1(), sl->line().y1());

   m_userItemMouseViewX = qr.x();
   m_userItemMouseViewY = qr.y();
   userItemMouseCoords(qr.x(), qr.y());
}

void rtimvMainWindow::userLineMoved(StretchLine * sl)
{
   QPointF np = m_qpmi->mapFromItem(sl, sl->line().p2());
   
   float x = np.x();
   float y = np.y();
   
   ui.graphicsView->m_userItemSize->setGeometry(x*ScreenZoom, y*ScreenZoom-20., 200,40);
 
   m_userItemXCen = sl->line().x1();
   m_userItemYCen = sl->line().y1();

   QPointF qr = QPointF(sl->line().x1(), sl->line().y1()); 

   m_userItemMouseViewX = qr.x();
   m_userItemMouseViewY = qr.y();
   userItemMouseCoords(qr.x(), qr.y());

   float w = sl->penWidth();
   if(w < 1) w = 1;
   float lhx = sl->line().x1() - w*1.5;
   float lhy = sl->line().y1() - w*1.5;
   m_lineHead->setRect(lhx, lhy, 3*w, 3*w);
}

void rtimvMainWindow::userLineMouseIn(StretchLine * sl)
{
   m_lineHead->setPen(sl->pen());

   m_userItemXCen = sl->line().x1();
   m_userItemYCen = sl->line().y1();


   float w = sl->penWidth();
   if(w < 1) w = 1;
   float lhx = sl->line().x1() - w*1.5;
   float lhy = sl->line().y1() - w*1.5;
   m_lineHead->setRect(lhx, lhy, 3*w, 3*w);

   userLineResized(sl);
   userLineMoved(sl);
}

void rtimvMainWindow::userLineMouseOut(StretchLine * sl)
{
   static_cast<void>(sl);
}

void rtimvMainWindow::userLineRejectMouse(StretchLine * sl)
{  
   std::unordered_set<StretchBox *>::iterator ubit = m_userBoxes.begin();
   while(ubit != m_userBoxes.end())
   {
      sl->stackBefore(*ubit);
      ++ubit;
   }
   
   std::unordered_set<StretchCircle *>::iterator ucit = m_userCircles.begin();
   while(ucit != m_userCircles.end())
   {
      sl->stackBefore(*ucit);
      ++ucit;
   }
   
   std::unordered_set<StretchLine *>::iterator ulit = m_userLines.begin();
   while(ulit != m_userLines.end())
   {
      if(sl != *ulit) sl->stackBefore(*ulit);
      ++ulit;
   }
}

void rtimvMainWindow::userLineRemove(StretchLine * sl)
{
   userLineMouseOut(sl); //This cleans up any gui items associated with the line
   m_userLines.erase(sl); //Remove it from our list
   m_qgs->removeItem(sl); //Remove it from the scene
   sl->deleteLater(); //clean it up after we're no longer in an asynch function
   ui.graphicsView->m_userItemMouseCoords->setVisible(false);
   ui.graphicsView->m_userItemSize->setVisible(false);
   m_lineHead->setVisible(false);
   m_userItemSelected = false;
}

void rtimvMainWindow::userLineSelected(StretchLine * sl)
{
   static_cast<void>(sl);
   ui.graphicsView->m_userItemSize->setVisible(true);
   ui.graphicsView->m_userItemMouseCoords->setVisible(true);
   m_lineHead->setVisible(true);

   m_userItemXCen = sl->line().x1();
   m_userItemYCen = sl->line().y1();
   m_userItemSelected = true;
}

void rtimvMainWindow::userLineDeSelected(StretchLine * sl)
{
   static_cast<void>(sl);
   ui.graphicsView->m_userItemSize->setVisible(false);
   ui.graphicsView->m_userItemMouseCoords->setVisible(false);
   m_lineHead->setVisible(false);
   m_userItemSelected = false;
}

void rtimvMainWindow::savingState(bool ss)
{
   if(ss)
   {
      ui.graphicsView->saveBoxFontColor("lightgreen");
      ui.graphicsView->m_saveBox->setText("S");
   }
   else
   {
      ui.graphicsView->saveBoxFontColor("red");
      ui.graphicsView->m_saveBox->setText("X");

   }
}

void rtimvMainWindow::post_setUserBoxActive(bool usba)
{
   if(!m_colorBox) return;

   m_colorBox->setVisible(usba);
}


void rtimvMainWindow::keyPressEvent(QKeyEvent * ke)
{
   //First deal with the control sequences
   if(ke->modifiers() & Qt::ControlModifier) 
   {
      switch(ke->key())
      {
         case Qt::Key_C:
            center();
            break;
         case Qt::Key_Plus:
            zoomLevel(zoomLevel() + 0.1);
            break;
         case Qt::Key_Minus:
            zoomLevel(zoomLevel() - 0.1);
            break;
      }
   }
   else //Finally deal with unmodified keys
   {
      char key = ke->text()[0].toLatin1();

      switch(ke->key())
      {
         case Qt::Key_A:
            if(key == 'a') return toggleAutoScale();
            break;
         case Qt::Key_B:
            if(key == 'b') return addUserBox();
            break;
         case Qt::Key_C:
            if(key == 'c') return addUserCircle();
            break;
         case Qt::Key_D:
            if(key == 'D') return toggleDarkSub();
            break;
         case Qt::Key_F:
            if(key == 'f') return toggleFPSGage();
            break;
         case Qt::Key_H:
            if(key == 'h') return toggleHelp();
            break;
         case Qt::Key_L:
            if(key == 'l') return addUserLine();
            if(key == 'L') return toggleLogLinear();
            break;
         case Qt::Key_M:
            if(key == 'M') return toggleApplyMask();
            break;
         case Qt::Key_N:
            if(key == 'n') return toggleNorthArrow();
            break;
         case Qt::Key_P:
            if(key == 'p') return launchControlPanel();
            break;
         case Qt::Key_R:
            if(key == 'r') return reStretch();
            break;
         case Qt::Key_S:
            if(key == 's') return toggleStatsBox();
            if(key == 'S') return toggleApplySatMask();
            break;
         case Qt::Key_T:
            if(key == 't') return toggleTarget();
            break;
         case Qt::Key_X:
            if(key == 'x') return freezeRealTime();
            break;
         case Qt::Key_Z:
            if(key == 'z') return toggleColorBox();
            break;            
         case Qt::Key_1:
            return zoomLevel(1.0);
            break;
         case Qt::Key_2:
            return zoomLevel(2.0);
            break;
         case Qt::Key_3:
            return zoomLevel(3.0);
            break;
         case Qt::Key_4:
            return zoomLevel(4.0);
            break;
         case Qt::Key_5:
            return zoomLevel(5.0);
            break;
         case Qt::Key_6:
            return zoomLevel(6.0);
            break;
         case Qt::Key_7:
            return zoomLevel(7.0);
            break;
         case Qt::Key_8:
            return zoomLevel(8.0);
            break;
         case Qt::Key_9:
            return zoomLevel(9.0);
            break;
         case Qt::Key_BracketLeft:
            return squareDown();
            break;
         case Qt::Key_BracketRight:
            return squareUp();
         default:
            break;
      }
   }
  
   for(size_t n=0;n<m_overlays.size(); ++n)
   {
      m_overlays[n]->keyPressEvent(ke);
   }
}

void rtimvMainWindow::setAutoScale( bool as )
{
   m_autoScale = as;
   if(m_autoScale) 
   {
      ui.graphicsView->zoomText("autoscale on");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else 
   {
      ui.graphicsView->zoomText("autoscale off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
}

void rtimvMainWindow::toggleAutoScale()
{
   if(m_autoScale) 
   {
      setAutoScale(false);
   }
   else 
   {
      setAutoScale(true);
   }      
}

void rtimvMainWindow::center()
{
   set_viewcen(.5, .5);
   post_zoomLevel();
      
   ui.graphicsView->zoomText("centered");
   fontLuminance(ui.graphicsView->m_zoomText);
}

void rtimvMainWindow::toggleColorBox()
{
   if(m_colorBox == nullptr)
   {
      float w;
      if(m_nx < m_ny) w = m_nx/4;
      else w = m_ny/4;
         
      colorBox_i0 = 0.5*(m_nx)-w/2;
      colorBox_i1 = colorBox_i0 + w;
      colorBox_j0 = 0.5*(m_ny)-w/2;
      colorBox_j1 = colorBox_j0 + w;
      
      m_colorBox = new StretchBox(colorBox_i0, colorBox_j0, w, w);

      m_colorBox->setPenColor("Yellow");
      m_colorBox->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
      m_colorBox->setVisible(false);
      m_colorBox->setStretchable(true);
      m_colorBox->setRemovable(false);
      m_userBoxes.insert(m_colorBox);
      connect(m_colorBox, SIGNAL(moved(StretchBox *)), this, SLOT(colorBoxMoved(StretchBox * )));
      connect(m_colorBox, SIGNAL(rejectMouse(StretchBox *)), this, SLOT(userBoxRejectMouse(StretchBox *)));
      m_qgs->addItem(m_colorBox);
   }

   if(!colorBoxActive)
   {
      if(imcp)
      {
         imcp->on_scaleModeCombo_activated(rtimvBase::minmaxbox);
      }
      else
      {
         m_colorBox->setVisible(true);
         setUserBoxActive(true);
      }
      ui.graphicsView->zoomText("color box scale");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      if(imcp)
      {
         imcp->on_scaleModeCombo_activated(rtimvBase::minmaxglobal);
      }
      else
      {
         m_colorBox->setVisible(false);
         setUserBoxActive(false);
      }
      ui.graphicsView->zoomText("global scale");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
}

void rtimvMainWindow::toggleStatsBox()
{
   if(m_statsBox == nullptr)
   {
      float w;
      if(m_nx < m_ny) w = m_nx/4;
      else w = m_ny/4;
   
      m_statsBox = new StretchBox(0.5*(m_nx)-w/2,0.5*(m_ny)-w/2, w, w);
      m_statsBox->setPenColor("#3DA5FF");
      m_statsBox->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
      m_statsBox->setVisible(false);
      m_statsBox->setStretchable(true);
      m_statsBox->setRemovable(true);
      m_userBoxes.insert(m_statsBox);
      connect(m_statsBox, SIGNAL(moved(StretchBox *)), this, SLOT(statsBoxMoved(StretchBox *)));
      connect(m_statsBox, SIGNAL(rejectMouse(StretchBox *)), this, SLOT(userBoxRejectMouse(StretchBox *)));
      connect(m_statsBox, SIGNAL(remove(StretchBox *)), this, SLOT(userBoxRemove(StretchBox *)));
      m_qgs->addItem(m_statsBox);
   }

   if(m_statsBox->isVisible())
   {
      doHideStatsBox();
      ui.graphicsView->zoomText("stats off");
      fontLuminance(ui.graphicsView->m_zoomText);
      if(imcp)
      {
         imcp->statsBoxButtonState = false;
         imcp->ui.statsBoxButton->setText("Show Stats Box");
      }
   }
   else
   {
      doLaunchStatsBox();
      ui.graphicsView->zoomText("stats on");
      fontLuminance(ui.graphicsView->m_zoomText);
      if(imcp)
      {
         imcp->statsBoxButtonState = true;
         imcp->ui.statsBoxButton->setText("Hide Stats Box");
      }
   }

}

void rtimvMainWindow::toggleNorthArrow()
{
   if(!m_northArrow) return;
   
   if(!m_northArrowEnabled)
   {
      m_northArrow->setVisible(false);
      m_northArrowTip->setVisible(false);
      return;
   }

   if(m_northArrow->isVisible())
   {
      m_northArrow->setVisible(false);
      m_northArrowTip->setVisible(false);
      ui.graphicsView->zoomText("North Off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      m_northArrow->setVisible(true);
      m_northArrowTip->setVisible(true);
      ui.graphicsView->zoomText("North On");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   
}

void rtimvMainWindow::showFPSGage( bool sfg )
{
   m_showFPSGage = sfg;
   if(m_showFPSGage)
   {
      ui.graphicsView->zoomText("fps gage on");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      ui.graphicsView->fpsGageText("");
      ui.graphicsView->zoomText("fps gage off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
}

void rtimvMainWindow::toggleFPSGage()
{
   if(m_showFPSGage)
   {
      return showFPSGage(false);
   }
   else
   {
      return showFPSGage(true);
   }
}

void rtimvMainWindow::setDarkSub( bool ds )
{
   m_subtractDark = ds;
   if(m_subtractDark)
   {
      ui.graphicsView->zoomText("dark sub. on");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      ui.graphicsView->zoomText("dark sub. off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   changeImdata(false);
}

void rtimvMainWindow::toggleDarkSub()
{
   if(m_subtractDark)
   {
      return setDarkSub(false);
   }
   else
   {
      return setDarkSub(true);
   }
}

void rtimvMainWindow::setApplyMask( bool am )
{
   m_applyMask = am;
   if(m_applyMask)
   {
      ui.graphicsView->zoomText("mask on");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      ui.graphicsView->zoomText("mask off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   changeImdata(false);
}

void rtimvMainWindow::toggleApplyMask()
{
   if(m_applyMask)
   {
      return setApplyMask(false);
   }
   else
   {
      return setApplyMask(true);
   }
}

void rtimvMainWindow::setApplySatMask( bool as )
{
   m_applySatMask = as;
   if(m_applySatMask)
   {
      ui.graphicsView->zoomText("sat mask on");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      ui.graphicsView->zoomText("sat mask off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   
   changeImdata(false);
   
}

void rtimvMainWindow::toggleApplySatMask()
{
   if(m_applySatMask)
   {
      return setApplySatMask(false);
   }
   else
   {
      return setApplySatMask(true);
   }
}

void rtimvMainWindow::toggleLogLinear()
{
   int s = get_cbStretch();
   
   if(s == stretchLog)
   {
      set_cbStretch(stretchLinear);
      ui.graphicsView->zoomText("linear stretch");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      set_cbStretch(stretchLog);
      ui.graphicsView->zoomText("log stretch");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
}

void rtimvMainWindow::toggleTarget()
{
   if(m_targetVisible)
   {
      targetVisible(false);
/*      m_cenLineVert->setVisible(false);
      m_cenLineHorz->setVisible(false);
      m_targetVisible = false;*/
      ui.graphicsView->zoomText("target off");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   else
   {
      targetVisible(true);
      /*m_cenLineVert->setVisible(true);
      m_cenLineHorz->setVisible(true);
      m_targetVisible=true;*/
      ui.graphicsView->zoomText("target on");
      fontLuminance(ui.graphicsView->m_zoomText);
   }
}

std::string rtimvMainWindow::generateHelp()
{
   std::string help;
   help  = "                       rtimv online help                    \n";
   help += "                     press 'h' to exit help        \n";
   help += "\n";
   help += "Shortcuts:\n";
   //      "01234567890123456789012345678901234567890123456789012345678901234567890123456789
   help += "a: toggle autoscale           b: add box                     \n";
   help += "c: add circle                 f: toggle FPS gauge            \n";
   help += "h: toggle help                l: add line                    \n";
   help += "n: toggle north arrow         p: launch control panel        \n";
   help += "r: re-stretch color table     s: toggle statistics box       \n";
   help += "t: toggle target cross        x: freeze real-time            \n";
   help += "z: toggle color box\n";
   
   help += "\n";
   help += "D: toggle dark subtraction    L: toggle log scale            \n";
   help += "M: toggle mask                S: toggle saturation mask      \n";
   
   help += "\n";
   help += "1-9: change zoom level\n";
   help += "ctrl +: zoom in\n";
   help += "ctrl -: zoom out\n";
   help += "\n";
   help += "[: shrink to fit              ]: grow to fit\n";
   help += "\n";
   help += "ctrl c: center image          delete: remove selected object \n";
   
   return help;
}

void rtimvMainWindow::toggleHelp()
{
   if(ui.graphicsView->m_helpText->isVisible())
   {
      ui.graphicsView->m_helpText->setVisible(false);
   }
   else
   {
      std::string help = generateHelp();
      ui.graphicsView->helpTextText(help.c_str());
      ui.graphicsView->m_helpText->setVisible(true);
   }
}

template<typename realT>
realT sRGBtoLinRGB( int rgb )
{
   realT V = ((realT) rgb)/255.0;
   
   if( V <= 0.0405 ) return V/12.92;
   
   return pow( (V+0.055)/1.055, 2.4);
}

template<typename realT>
realT linRGBtoLuminance( realT linR,
                         realT linG,
                         realT linB
                       )
{
   return  0.2126*linR + 0.7152*linG + 0.0722*linB;
}

template<typename realT>
realT pLightness( realT lum )
{
   if(lum <= static_cast<realT>(216)/static_cast<realT>(24389))
   {
      return lum*static_cast<realT>(24389)/static_cast<realT>(27);
   }
   
   return pow(lum, static_cast<realT>(1)/static_cast<realT>(3))*116 - 16;
      
}

void rtimvMainWindow::fontLuminance( QTextEdit* qte, 
                                     bool print
                                   )
{
   
   QPointF ptul = ui.graphicsView->mapToScene(qte->x(),qte->y());
   QPointF ptlr = ui.graphicsView->mapToScene(qte->x()+qte->width(),qte->y()+qte->height());
   
   unsigned myul = ptul.y();
   if(myul > m_ny-1) myul = 0;

   unsigned mxul = ptul.x();
   if(mxul > m_nx-1) mxul = 0;
   
   unsigned mylr = ptlr.y();
   if(mylr > m_ny-1) mylr = m_ny-1;
   
   unsigned mxlr = ptlr.x();
   if(mxlr > m_nx-1) mxlr = m_nx-1;
   
   if(mxul == 0 && myul == 0 && mxlr == 0 && mylr == 0) return;
   if(mxul == mxlr || myul == mylr) return;

   double avgLum = 0;
   int N = 0;
   for(unsigned x = mxul; x<= mxlr; ++x)
   {
      for(unsigned y=myul; y<=mylr; ++y)
      {
         avgLum += pow(m_lightness[ m_qim->pixelIndex(x,y) ],m_lumPwr); 
         ++N;
      }
   }
   avgLum /= N;
   avgLum = pow(avgLum, 1.0/m_lumPwr);

   if(print) std::cerr << "avgLum: " << avgLum << "\n";

   if(avgLum <= m_lumThresh)
   {
      qte->setTextBackgroundColor(QColor(0,0,0,0));
   }
   else if(avgLum < m_lumMax)
   {
      int op = (avgLum - m_lumThresh)/(m_lumMax - m_lumThresh) * m_opacityMax + 0.5;
      qte->setTextBackgroundColor(QColor(0,0,0,op));
   }
   else
   {
      qte->setTextBackgroundColor(QColor(0,0,0,m_opacityMax));
   }

   return;

}

void rtimvMainWindow::fontLuminance()
{   
   fontLuminance(ui.graphicsView->m_fpsGage);
   
   fontLuminance(ui.graphicsView->m_zoomText);

   if(!m_nullMouseCoords)
   {
      if(m_showStaticCoords)
      {
         fontLuminance(ui.graphicsView->m_textCoordX);
         fontLuminance(ui.graphicsView->m_textCoordY);
         fontLuminance(ui.graphicsView->m_textPixelVal);   
      }

      if(m_showToolTipCoords)
      {
         fontLuminance(ui.graphicsView->m_mouseCoords);
      }
   }
   
   for(size_t n=0; n< ui.graphicsView->m_statusText.size(); ++n)
   {
      if(ui.graphicsView->m_statusText[n]->toPlainText().size() > 0) fontLuminance(ui.graphicsView->m_statusText[n]);
   }
   
   fontLuminance(ui.graphicsView->m_saveBox);
   
   return;
   
}


int rtimvMainWindow::loadPlugin(QObject *plugin)
{
   auto rdi = qobject_cast<rtimvDictionaryInterface *>(plugin);
   if (rdi)
   {
      rdi->attachDictionary(&m_dictionary, config);
   }
   
   auto roi = qobject_cast<rtimvOverlayInterface *>(plugin);
   if (roi)
   {
      rtimvOverlayAccess roa;
      roa.m_mainWindowObject = this;
      roa.m_colorBox = m_colorBox;
      roa.m_statsBox = m_statsBox;
      roa.m_userBoxes = &m_userBoxes;
      roa.m_userCircles = &m_userCircles;
      roa.m_userLines = &m_userLines;
      roa.m_graphicsView = ui.graphicsView;
      roa.m_dictionary = &m_dictionary;
      
      int arv = roi->attachOverlay(roa, config);
      
      if(arv < 0)
      {
         std::cerr << "Error from attachOverlay\n";
         return arv;
      }
      else if(arv > 0)
      {
         return arv;
      }
      else
      {
         m_overlays.push_back(roi);
      }
   }
   
   return 0;
}
