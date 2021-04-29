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
   
   nup =0;
   
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
   

   

   colorBox_i0 = 0;
   colorBox_i1 = 32;
   colorBox_j0 = 0;
   colorBox_j1 = 32;
   m_colorBox = new StretchBox(0,0,32,32);
   m_colorBox->setPenColor("Yellow");
   m_colorBox->setPenWidth(0.1);
   m_colorBox->setVisible(false);
   m_colorBox->setStretchable(true);
   m_colorBox->setRemovable(false);
   m_userBoxes.insert(m_colorBox);
   connect(m_colorBox, SIGNAL(moved(StretchBox *)), this, SLOT(colorBoxMoved(StretchBox * )));
   connect(m_colorBox, SIGNAL(rejectMouse(StretchBox *)), this, SLOT(userBoxRejectMouse(StretchBox *)));
   m_qgs->addItem(m_colorBox);

   m_statsBox = new StretchBox(0,0,32,32);
   m_statsBox->setPenColor("Red");
   m_statsBox->setPenWidth(0.1);
   m_statsBox->setVisible(false);
   m_statsBox->setStretchable(true);
   m_statsBox->setRemovable(true);
   m_userBoxes.insert(m_statsBox);
   connect(m_statsBox, SIGNAL(moved(StretchBox *)), this, SLOT(statsBoxMoved(StretchBox *)));
   connect(m_statsBox, SIGNAL(rejectMouse(StretchBox *)), this, SLOT(userBoxRejectMouse(StretchBox *)));
   connect(m_statsBox, SIGNAL(remove(StretchBox *)), this, SLOT(userBoxRemove(StretchBox *)));
   m_qgs->addItem(m_statsBox);
   
   
   m_targetVisible = false;
   
   m_cenLineVert = 0;
   m_cenLineHorz = 0;
   
   imStats = 0;
   m_timer.start(m_timeout);

   nup = m_qgs->addLine(QLineF(512,400, 512, 624), QColor("skyblue"));
   nup_tip = m_qgs->addLine(QLineF(512,400, 536, 424), QColor("skyblue"));
   nup->setTransformOriginPoint ( QPointF(512,512) );
   nup_tip->setTransformOriginPoint ( QPointF(512,512) );
   nup->setVisible(false);
   nup_tip->setVisible(false);
   
   QPen qp = nup->pen();
   qp.setWidth(5);

   nup->setPen(qp);
   nup_tip->setPen(qp);
   
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
         std::cerr << "fileName: " << fileName.toStdString() << "\n";
         QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
         QObject *plugin = loader.instance();
         if (plugin) 
         {
            std::cerr << "loading dynamic\n";
            int arv = loadPlugin(plugin);
            if( arv != 0 )
            {
               std::cerr << "unloading  . . . ";
               if(loader.unload()) std::cerr << "it worked!\n";
               else std::cerr << "it didn't work\n";
            }
            else
            {
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
   config.add("darksub", "", "darksub", argType::True, "", "darksub", false, "bool", "Set to false to turn off on at startup.  If a dark is supplied, darksub is otherwise on.");
   config.add("targetXc", "", "targetXc", argType::Required, "", "targetXc", false, "float", "The fractional x-coordinate of the target, 0<= x <=1");
   config.add("targetYc", "", "targetYc", argType::Required, "", "targetXc", false, "float", "The fractional y-coordinate of the target, 0<= y <=1");
}

void rtimvMainWindow::loadConfig()
{
   std::string imShmimKey;
   std::string darkShmimKey;
   
   std::string flatShmimKey;
   
   std::string maskShmimKey;
  
   std::string satMaskShmimKey;
   
   std::vector<std::string> keys;
   
   config(imShmimKey, "image.shmim_name");
   config(darkShmimKey, "dark.shmim_name");
   config(maskShmimKey, "mask.shmim_name");
   config(satMaskShmimKey, "satMask.shmim_name");
      
   keys.resize(4);
      
   if(imShmimKey != "") keys[0] = imShmimKey;
   if(darkShmimKey != "") keys[1] = darkShmimKey;
   if(maskShmimKey != "") keys[2] = maskShmimKey;
   if(satMaskShmimKey != "") keys[3] = satMaskShmimKey;
   
   //The command line always overrides the config
   if(config.nonOptions.size() > 0) keys[0] = config.nonOptions[0];
   if(config.nonOptions.size() > 1) keys[1] = config.nonOptions[1];
   if(config.nonOptions.size() > 2) keys[2] = config.nonOptions[2];
   if(config.nonOptions.size() > 3) keys[3] = config.nonOptions[3];
   
   //m_title= keys[0];

   startup(keys);
   
   if(m_images[0] == nullptr)
   {
      std::cerr << "no image specified.  not starting.\n";
      exit(0);
   }
   else
   {
      m_title = m_images[0]->imageName();
   }
   
   //Now load remaining options, respecting coded defaults.
   config(m_autoScale, "autoscale");

   bool nofpsgage = !m_showFPSGage;
   config(nofpsgage, "nofpsgage");
   m_showFPSGage = !nofpsgage;

   config(m_targetXc, "targetXc");
   config(m_targetYc, "targetYc");
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
   
   

//    //Resize the user color box
//    colorBox_i0 = m_ny*.25;
//    colorBox_i1 = m_ny*.75;
// 
//    colorBox_j0 = m_nx*.25;
//    colorBox_j1 = m_nx*.75;
// 
//    
//    m_colorBox->setRect(m_colorBox->mapRectFromScene(colorBox_i0, colorBox_j0, colorBox_i1-colorBox_i0, colorBox_j1-colorBox_j0));
//    m_colorBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);

   
   //resize the boxes
   std::unordered_set<StretchBox *>::iterator ubit = m_userBoxes.begin();
   while(ubit != m_userBoxes.end())
   {
      StretchBox *sb = *ubit;
      sb->setRect(sb->mapRectFromScene(m_ny*.35, m_nx*.35, .4*m_ny, .4*m_nx));
      sb->setEdgeTol(RTIMV_EDGETOL_DEFAULT/ScreenZoom < RTIMV_EDGETOL_DEFAULT ? RTIMV_EDGETOL_DEFAULT : RTIMV_EDGETOL_DEFAULT/ScreenZoom);
      sb->setPenWidth(RTIMV_TOOLLINEWIDTH_DEFAULT /ScreenZoom);
      ++ubit;
   }
   
   //resize the circles 
   std::unordered_set<StretchCircle *>::iterator ucit = m_userCircles.begin();
   while(ucit != m_userCircles.end())
   {
      StretchCircle *sc = *ucit;
      sc->setRect(sc->mapRectFromScene(m_ny*.35, m_nx*.35, .4*m_ny, .4*m_nx));
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
   }
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
   
   if(nup)
   {
      nup->setLine(ui.graphicsView->xCen(), ui.graphicsView->yCen()-.1*m_ny/m_zoomLevel, ui.graphicsView->xCen(), ui.graphicsView->yCen()+.1*m_ny/m_zoomLevel);
      
      nup->setTransformOriginPoint ( QPointF(ui.graphicsView->xCen(),ui.graphicsView->yCen()) );
         
      nup_tip->setLine(QLineF(ui.graphicsView->xCen(),ui.graphicsView->yCen()-.1*m_ny/m_zoomLevel, ui.graphicsView->xCen() + .02*m_nx/m_zoomLevel,ui.graphicsView->yCen()-.1*m_ny/m_zoomLevel + .012*m_ny/m_zoomLevel));
      nup_tip->setTransformOriginPoint (  QPointF(ui.graphicsView->xCen(),ui.graphicsView->yCen()) );

      QPen qp = nup->pen();
   
      float wid = 5/(m_zoomLevel*ScreenZoom);
      if(wid > 3) wid = 3;
      qp.setWidth(wid);

      nup->setPen(qp);
      nup_tip->setPen(qp);
   }
  
  
   char zlstr[16];
   snprintf(zlstr,16, "%0.1fx", m_zoomLevel);
   ui.graphicsView->zoomText(zlstr);
   
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
   //if(guideBox) m_qpmi->stackBefore(guideBox);
   
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
      imStats->set_imdata();
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
   
   if(!m_nullMouseCoords)
   {
      ui.graphicsView->textCoordX(mx-0.5);
      ui.graphicsView->textCoordY(m_qpmi->boundingRect().height() - my-0.5);
      
      
      idx_x = ((int64_t)(mx-0));
      if(idx_x < 0) idx_x = 0;
      if(idx_x > (int64_t) m_nx-1) idx_x = m_nx-1;
      
      idx_y = (int)(m_qpmi->boundingRect().height() - (my-0));
      if(idx_y < 0) idx_y = 0;
      if(idx_y > (int64_t) m_ny-1) idx_y = m_ny-1;

      pixelF _pixel = pixel();
      
      if(_pixel != nullptr)
      ui.graphicsView->textPixelVal(_pixel(this,  (int)(idx_y*m_nx) + (int)(idx_x)) );

      //m_qpmi->setToolTip("test\nnewt\nxxxyy");
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
      if(m_images[0]->valid())    //have to check this after checking nullptr
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
         else
         {
            ui.graphicsView->fpsGageText(age, true);
         } 
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


}

void rtimvMainWindow::addUserBox()
{
   float w;
   if(m_nx < m_ny) w = m_nx/4;
   else w = m_ny/4;
   
   std::pair<std::unordered_set<StretchBox *>::iterator,bool> it = m_userBoxes.insert(new StretchBox(0.5*(m_nx)-w/2,0.5*(m_ny)-w/2, w, w));
   
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

   
   m_qgs->addItem(sb);
      
}

void rtimvMainWindow::addUserCircle()
{
   float w;
   if(m_nx < m_ny) w = m_nx/4;
   else w = m_ny/4;
   
   std::pair<std::unordered_set<StretchCircle *>::iterator,bool> it = m_userCircles.insert(new StretchCircle(0.5*(m_nx)-w/2,0.5*(m_ny)-w/2, w, w));
   
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

   
   m_qgs->addItem(sc);
      
}

void rtimvMainWindow::addUserLine()
{
   float w;
   if(m_nx < m_ny) w = m_nx/4;
   else w = m_ny/4;
   
   std::pair<std::unordered_set<StretchLine *>::iterator,bool> it = m_userLines.insert(new StretchLine(0.5*(m_nx)-w/2,0.5*(m_ny)-w/2, 0.5*(m_nx)+w/2,0.5*(m_ny)+w/2));
   
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

   
   m_qgs->addItem(sl);
      
}

int rtimvMainWindow::targetXc( float txc )
{
   if(txc < 0) txc = 0;
   if(txc > 1) txc = 1;
   
   m_targetXc = txc;
   
   setTarget();
   
   return 0;
}
     
float rtimvMainWindow::targetXc()
{
   return m_targetXc;
}
      
int rtimvMainWindow::targetYc( float tyc )
{
   if(tyc < 0) tyc = 0;
   if(tyc > 1) tyc = 1;
   
   m_targetYc = tyc;
   
   setTarget();
   
   return 0;
}
     
float rtimvMainWindow::targetYc()
{
   return m_targetYc;
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
   m_statsBox->setVisible(true);
   
   if(!imStats)
   {
      imStats = new rtimvStats(this, this, 0);
      imStats->setAttribute(Qt::WA_DeleteOnClose); //Qt will delete imstats when it closes.
      connect(imStats, SIGNAL(finished(int )), this, SLOT(imStatsClosed(int )));
   }

   statsBoxMoved(m_statsBox);

   imStats->show();
    
   imStats->activateWindow();
   
}

void rtimvMainWindow::doHideStatsBox()
{
   m_statsBox->setVisible(false);

   if (imStats)
   {
      delete imStats;
      //imStats->hide();
      //imStats->close(); 
      imStats = 0; //imStats is set to delete on close
   }

}

void rtimvMainWindow::imStatsClosed(int result)
{
   static_cast<void>(result);
   
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
   
   QPointF np = m_qpmi->mapFromItem(m_statsBox, QPointF(m_statsBox->rect().x(),m_statsBox->rect().y()));
   QPointF np2 = m_qpmi->mapFromItem(m_statsBox, QPointF(m_statsBox->rect().x()+m_statsBox->rect().width(),m_statsBox->rect().y()+m_statsBox->rect().height()));

   if(imStats) 
   {
      imStats->set_imdata(m_nx, m_ny, np.x() + .5, np2.x(), m_ny-np2.y()+.5, m_ny-np.y());
   }
   

}


void rtimvMainWindow::colorBoxMoved(StretchBox * sb)
{
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
   
   char tmp[256];
   snprintf(tmp, 256, "%0.1f x %0.1f", sb->rect().width(), sb->rect().height());
   
   ui.graphicsView->coords->setText(tmp);
}

void rtimvMainWindow::userBoxMoved(StretchBox * sb)
{
   QRectF newr = sb->rect();
   QPointF np = m_qpmi->mapFromItem(sb, QPointF(newr.x(),newr.y()));
   
   float x = np.x()+.5*newr.width();
   float y = np.y()+.5*newr.height();
   
   ui.graphicsView->coords->setGeometry(x*ScreenZoom-50., y*ScreenZoom-50., 200,40);
   
   float cx = sb->rect().x() + sb->pos().x() + 0.5*sb->rect().width();
   float cy = sb->rect().y() + sb->pos().y() + 0.5*sb->rect().height();
   m_objCenH->setPen(sb->pen());
   m_objCenV->setPen(sb->pen());
   float w = sb->penWidth();
   if(w < 1) w = 1;
   m_objCenH->setLine(cx-2*w, cy, cx+2*w, cy);
   m_objCenV->setLine(cx, cy-2*w, cx, cy+2*w);

}

void rtimvMainWindow::userBoxMouseIn(StretchBox * sb)
{
   userBoxResized(sb);
   userBoxMoved(sb);
   ui.graphicsView->coords->setVisible(true);
   m_objCenH->setVisible(true);
   m_objCenV->setVisible(true);

}

void rtimvMainWindow::userBoxMouseOut(StretchBox * sb)
{
   static_cast<void>(sb);
   ui.graphicsView->coords->setVisible(false);
   m_objCenH->setVisible(false);
   m_objCenV->setVisible(false);
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
}

void rtimvMainWindow::userCircleResized(StretchCircle * sc)
{
   char tmp[256];
   snprintf(tmp, 256, "%0.1f", sc->radius());
   
   ui.graphicsView->coords->setText(tmp);
}

void rtimvMainWindow::userCircleMoved(StretchCircle * sc)
{
   QRectF newr = sc->rect();
   QPointF np = m_qpmi->mapFromItem(sc, QPointF(newr.x(),newr.y()));
   
   float x = np.x()+.5*newr.width();
   float y = np.y()+.5*newr.height();
   
   ui.graphicsView->coords->setGeometry(x*ScreenZoom-50., y*ScreenZoom-50., 70,40);
  
   float cx = sc->rect().x() + sc->pos().x() + 0.5*sc->rect().width();
   float cy = sc->rect().y() + sc->pos().y() + 0.5*sc->rect().height();
   m_objCenH->setPen(sc->pen());
   m_objCenV->setPen(sc->pen());
   float w = sc->penWidth();
   if(w < 1) w = 1;
   m_objCenH->setLine(cx-2*w, cy, cx+2*w, cy);
   m_objCenV->setLine(cx, cy-2*w, cx, cy+2*w);
}

void rtimvMainWindow::userCircleMouseIn(StretchCircle * sc)
{
   userCircleResized(sc);
   userCircleMoved(sc);
   ui.graphicsView->coords->setVisible(true);
   m_objCenH->setVisible(true);
   m_objCenV->setVisible(true);
}

void rtimvMainWindow::userCircleMouseOut(StretchCircle * sc)
{
   static_cast<void>(sc);
   ui.graphicsView->coords->setVisible(false);
   m_objCenH->setVisible(false);
   m_objCenV->setVisible(false);
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
}

void rtimvMainWindow::userLineResized(StretchLine * sl)
{
   userLineMoved(sl); //Move the text along with us.
   
   char tmp[256];
   snprintf(tmp, 256, "%0.1f @ %0.1f", sl->length(), sl->angle());
   
   ui.graphicsView->coords->setText(tmp);
}

void rtimvMainWindow::userLineMoved(StretchLine * sl)
{
   QPointF np = m_qpmi->mapFromItem(sl, sl->line().p2());
   
   float x = np.x();
   float y = np.y();
   
   ui.graphicsView->coords->setGeometry(x*ScreenZoom, y*ScreenZoom-20., 200,40);
 
   float w = sl->penWidth();
   if(w < 1) w = 1;
   float lhx = sl->line().x1() - w*1.5;
   float lhy = sl->line().y1() - w*1.5;
   m_lineHead->setRect(lhx, lhy, 3*w, 3*w);
}

void rtimvMainWindow::userLineMouseIn(StretchLine * sl)
{
   m_lineHead->setPen(sl->pen());
   float w = sl->penWidth();
   if(w < 1) w = 1;
   float lhx = sl->line().x1() - w*1.5;
   float lhy = sl->line().y1() - w*1.5;
   m_lineHead->setRect(lhx, lhy, 3*w, 3*w);
   m_lineHead->setVisible(true);

   userLineResized(sl);
   userLineMoved(sl);
   ui.graphicsView->coords->setVisible(true);
}

void rtimvMainWindow::userLineMouseOut(StretchLine * sl)
{
   static_cast<void>(sl);
   m_lineHead->setVisible(false);
   ui.graphicsView->coords->setVisible(false);
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
}

void rtimvMainWindow::post_setUserBoxActive(bool usba)
{
   m_colorBox->setVisible(usba);
}


void rtimvMainWindow::keyPressEvent(QKeyEvent * ke)
{
   //First deal with the control sequences
   if(ke->modifiers() == Qt::ControlModifier) 
   {
      switch(ke->key())
      {
         case Qt::Key_C:
            center();
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
   }
   else 
   {
      ui.graphicsView->zoomText("autoscale off");
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
}

void rtimvMainWindow::toggleColorBox()
{
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
   }
}

void rtimvMainWindow::toggleStatsBox()
{
   if(m_statsBox->isVisible())
   {
      doHideStatsBox();
      ui.graphicsView->zoomText("stats off");
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
      if(imcp)
      {
         imcp->statsBoxButtonState = true;
         imcp->ui.statsBoxButton->setText("Hide Stats Box");
      }
   }

}

void rtimvMainWindow::toggleNorthArrow()
{
   if(!nup) return;
   
   if(nup->isVisible())
   {
      nup->setVisible(false);
      nup_tip->setVisible(false);
      ui.graphicsView->zoomText("North Off");
   }
   else
   {
      nup->setVisible(true);
      nup_tip->setVisible(true);
      ui.graphicsView->zoomText("North On");
   }
   
}

void rtimvMainWindow::showFPSGage( bool sfg )
{
   m_showFPSGage = sfg;
   if(m_showFPSGage)
   {
      ui.graphicsView->zoomText("fps gage on");
   }
   else
   {
      ui.graphicsView->fpsGageText("");
      ui.graphicsView->zoomText("fps gage off");
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
   }
   else
   {
      ui.graphicsView->zoomText("dark sub. off");
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
   }
   else
   {
      ui.graphicsView->zoomText("mask off");
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
   }
   else
   {
      ui.graphicsView->zoomText("sat mask off");
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
   }
   else
   {
      set_cbStretch(stretchLog);
      ui.graphicsView->zoomText("log stretch");
   }
}

void rtimvMainWindow::toggleTarget()
{
   if(m_targetVisible)
   {
      m_cenLineVert->setVisible(false);
      m_cenLineHorz->setVisible(false);
      m_targetVisible = false;
      ui.graphicsView->zoomText("target off");
   }
   else
   {
      m_cenLineVert->setVisible(true);
      m_cenLineHorz->setVisible(true);
      m_targetVisible=true;
      ui.graphicsView->zoomText("target on");
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
   help += "a: toggle autoscale           \tb: add box                     \n";
   help += "c: add circle                 \tf: toggle FPS gauge            \n";
   help += "h: toggle help                \tl: add line                    \n";
   help += "n: toggle north arrow         \tp: launch control panel        \n";
   help += "r: re-stretch color table     \ts: toggle statistics box       \n";
   help += "t: toggle target cross        \tx: freeze real-time            \n";
   help += "z: toggle color box\n";
   
   help += "\n";
   help += "D: toggle dark subtraction    \tL: toggle log scale            \n";
   help += "M: toggle mask                \tS: toggle saturation mask      \n";
   
   help += "\n";
   help += "1-9: change zoom level\n";
   help += "\n";
   help += "[: shrink to fit              \t]: grow to fit\n";
   help += "\n";
   help += "ctrl-c: center image          \tdelete: remove selected object \n";
   
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

int rtimvMainWindow::fontLuminance(QTextEdit* qte)
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
   
   //std::cerr << mxul << " " << myul << " " << mxlr << " " << mylr << "\n";
   //std::cerr << m_qim->pixelIndex(mxul, myul) << " " << m_qim->pixelIndex(mxlr, mylr) << "\n";
   
   double avgLum = 0;
   int N =0;
   for(unsigned x = mxul; x<= mxlr; ++x)
   {
      for(unsigned y=myul; y<=mylr; ++y)
      {
         avgLum += pow(m_lightness[ m_qim->pixelIndex(x,y) ],2);
         ++N;
      }
   }
   avgLum/=N;
   avgLum = sqrt(avgLum);
   //std::cerr << "avgLum: " << avgLum << "\n";
   
   QColor sb("skyblue");
   
   //int flight = sb.lightness();
   //std::cout << avgLum << " " << flight << "\n";
   
   if(avgLum > 100 && avgLum <= 140)
   {
      QColor nc = QColor::fromHsl(sb.hslHue(), sb.hslSaturation(), 255);
      qte->setTextColor(nc);
   }
   else if(avgLum > 140)
   {
      QColor nc = QColor::fromHsl(sb.hslHue(), sb.hslSaturation(), 0);
      qte->setTextColor(nc);
   }
   else 
   {
      qte->setTextColor(sb);
   }
   
   return 0;
   
}

int rtimvMainWindow::fontLuminance()
{   
   fontLuminance(ui.graphicsView->m_fpsGage);
   
   if(!m_nullMouseCoords)
   {
      fontLuminance(ui.graphicsView->m_textCoordX);
      fontLuminance(ui.graphicsView->m_textCoordY);
      fontLuminance(ui.graphicsView->m_textPixelVal);
      fontLuminance(ui.graphicsView->m_zoomText);
   }
   
   for(size_t n=0; n< ui.graphicsView->m_statusText.size(); ++n)
   {
      if(ui.graphicsView->m_statusText[n]->toPlainText().size() > 0) fontLuminance(ui.graphicsView->m_statusText[n]);
   }
   
   
   return 0;
   
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
