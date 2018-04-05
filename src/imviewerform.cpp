#include "imviewerform.h"

imviewerForm::imviewerForm(imviewer_shmt shkey, QWidget * Parent, Qt::WindowFlags f) : imviewer(shkey, Parent, f)
{
   ui.setupUi(this);
   nup =0;
   
   imcp = 0;
   pointerOverZoom = 4.;
   
   //This will come up at some minimal size.
   ui.graphicsView->setGeometry(0,0, width(), height());
   
   qgs = new QGraphicsScene();
   ui.graphicsView->setScene(qgs);
   
   qpmi = 0;
   userBox = 0;
   statsBox = 0;
   
   
   rightClickDragging = false;
   
   NullMouseCoords = true;
   
   
   set_mindat(400);
   
   set_maxdat(600);
   

   

   userBox_i0 = 0;
   userBox_i1 = 32;
   userBox_j0 = 0;
   userBox_j1 = 32;
   userBox = new StretchBox(0,0,32,32);
   userBox->setPen(QPen("Yellow"));
   userBox->setVisible(false);
   userBox->setStretchable(true);
   connect(userBox, SIGNAL(moved(const QRectF & )), this, SLOT(userBoxMoved(const QRectF & )));
   connect(userBox, SIGNAL(rejectMouse()), this, SLOT(userBoxRejectMouse()));
   
   qgs->addItem(userBox);

   statsBox = new StretchBox(0,0,32,32);
   statsBox->setPen(QPen("Red"));
   statsBox->setVisible(false);
   statsBox->setStretchable(true);
   connect(statsBox, SIGNAL(moved(const QRectF & )), this, SLOT(statsBoxMoved(const QRectF & )));
   connect(statsBox, SIGNAL(rejectMouse()), this, SLOT(statsBoxRejectMouse()));
   
   qgs->addItem(statsBox);

   guideBox = new StretchBox(0,0,32,32);
   guideBox->setPen(QPen("Cyan"));
   guideBox->setVisible(false);
   guideBox->setStretchable(true);
   connect(guideBox, SIGNAL(moved(const QRectF & )), this, SLOT(guideBoxMoved(const QRectF & )));
   connect(guideBox, SIGNAL(rejectMouse()), this, SLOT(guideBoxRejectMouse()));
   
   qgs->addItem(guideBox);


   userCircle = new StretchCircle(512,512, 64, 64);
   userCircle->setPen(QPen("lime"));
   userCircle->setStretchable(true);
   userCircle->setVisible(false);
   

      
   connect(userCircle, SIGNAL(resized(const float &)), this, SLOT(userCircleResized(const float &)));
   connect(userCircle, SIGNAL(moved(const QRectF & )), this, SLOT(userCircleMoved(const QRectF & )));
   connect(userCircle, SIGNAL(mouseIn()), this, SLOT(userCircleMouseIn()));
   connect(userCircle, SIGNAL(mouseOut()), this, SLOT(userCircleMouseOut()));
   connect(userCircle, SIGNAL(rejectMouse()), this, SLOT(userCircleRejectMouse()));

   
   qgs->addItem(userCircle);
  
   
   
   targetVisible = false;
   
   cenLineVert = 0;//qgs->addLine(QLineF(.5*getNx(),0, .5*getNx(), getNy()), QColor("lime"));
   cenLineHorz = 0;//qgs->addLine(QLineF(0, .5*getNy(), getNx(), .5*getNy()), QColor("lime"));
   
   imStats = 0;
   setImsize(1024,1024); //Just for initial setup.   

   if(shmem_attached) timerout(); //Get first image

   imtimer.start(imtimer_timeout); //and set timer.



   nup = qgs->addLine(QLineF(512,400, 512, 624), QColor("skyblue"));
   nup_tip = qgs->addLine(QLineF(512,400, 536, 424), QColor("skyblue"));
   nup->setTransformOriginPoint ( QPointF(512,512) );
   nup_tip->setTransformOriginPoint ( QPointF(512,512) );

   QPen qp = nup->pen();
   qp.setWidth(5);

   nup->setPen(qp);
   nup_tip->setPen(qp);
   
      
}

void imviewerForm::postSetImsize()
{
   ui.graphicsView->xCen(0.5);
   ui.graphicsView->yCen(0.5);
   
   ScreenZoom = std::max((float)width()/(float)nx,(float)height()/(float)ny);
   
   if(imcp)
   {
      QTransform transform;
      float viewZoom = (float)imcp->ui.viewView->width()/(float)nx;
      
      transform.scale(viewZoom, viewZoom);
      imcp->ui.viewView->setTransform(transform);
   }
   set_ZoomLevel(1.0);

   //Resize the user color box
   userBox_i0 = ny*.25;
   userBox_i1 = ny*.75;

   userBox_j0 = nx*.25;
   userBox_j1 = nx*.75;

   //std::cout << userBox_i0 << " " << userBox_i1 - userBox_i0 << " " << userBox_j0 << " " << userBox_j1 - userBox_j0<< "\n";
   userBox->setRect(userBox->mapRectFromScene(userBox_i0, userBox_j0, userBox_i1-userBox_i0, userBox_j1-userBox_j0));
   userBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);

   //resize the stats box
   statsBox->setRect(statsBox->mapRectFromScene(ny*.25, nx*.3, .4*ny, .4*nx));
   statsBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);
   //statsBoxMoved(statsBox->rect());
   
   //resize the guide box
   guideBox->setRect(statsBox->mapRectFromScene(ny*.3, nx*.3, .4*ny, .4*nx));
   guideBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);
   //guideBoxMoved(guideBox->rect());
   
   //resize the circle
   userCircle->setRect(userCircle->mapRectFromScene(ny*.35, nx*.35, .4*ny, .4*nx));
   userCircle->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);
   //userCircleMoved(guideBox->rect());
   
   if(!cenLineVert)
   {
      cenLineVert = qgs->addLine(QLineF(.5*getNx(),0, .5*getNx(), getNy()), QColor("lime"));
      cenLineHorz = qgs->addLine(QLineF(0, .5*getNy(), getNx(), .5*getNy()), QColor("lime"));
      if(targetVisible)
      {
         cenLineVert->setVisible(true);
         cenLineHorz->setVisible(true);
      }
      else
      {
         cenLineVert->setVisible(false);
         cenLineHorz->setVisible(false);
      } 
   }
   else
   {
      cenLineVert->setLine(QLineF(.5*getNx(),0, .5*getNx(), getNy()));
      cenLineHorz->setLine(QLineF(0, .5*getNy(), getNx(), .5*getNy()));
   }
}

void imviewerForm::post_set_ZoomLevel()
{
   QTransform transform;
   
   ui.graphicsView->nX(nx);
   ui.graphicsView->nY(ny);
   ui.graphicsView->zoomLevel(ZoomLevel);
   ui.graphicsView->screenZoom(ScreenZoom);
   
   transform.scale(ZoomLevel*ScreenZoom, ZoomLevel*ScreenZoom);
   
   ui.graphicsView->setTransform(transform);
   transform.scale(pointerOverZoom, pointerOverZoom);
   if(imcp) imcp->ui.pointerView->setTransform(transform);
   change_center();
   
   if(nup)
   {
      nup->setLine(ui.graphicsView->actXCen()*nx, ui.graphicsView->actYCen()*ny-.1*ny/ZoomLevel, ui.graphicsView->actXCen()*nx, ui.graphicsView->actYCen()*ny+.1*ny/ZoomLevel);
      nup->setTransformOriginPoint ( QPointF(ui.graphicsView->actXCen()*nx,ui.graphicsView->actYCen()*ny) );
         
      nup_tip->setLine(QLineF(ui.graphicsView->actXCen()*nx,ui.graphicsView->actYCen()*ny-.1*ny/ZoomLevel, ui.graphicsView->actXCen()*nx + .02*nx/ZoomLevel,ui.graphicsView->actYCen()*ny-.1*ny/ZoomLevel + .012*ny/ZoomLevel));
      nup_tip->setTransformOriginPoint (  QPointF(ui.graphicsView->actXCen()*nx,ui.graphicsView->actYCen()*ny) );

      QPen qp = nup->pen();
   
      float wid = 5/(ZoomLevel*ScreenZoom);
      if(wid > 3) wid = 3;
      //if(wid < 1) wid = 1;
      //std::cout << wid << "\n";
      qp.setWidth(wid);

      nup->setPen(qp);
      nup_tip->setPen(qp);
   }
  
  
   char zlstr[16];
   snprintf(zlstr,16, "%0.1fx", ZoomLevel);
   ui.graphicsView->zoomText(zlstr);
   
}

void imviewerForm::postChangeImdata()
{
   if(fps_ave > 1.0) ui.graphicsView->fpsGageText( fps_ave );
  
   if(!saturated)
   {
      ui.graphicsView->warningText("Saturated!");
   }
   else
   {
      ui.graphicsView->warningText("");
   }
   
   if(!qpmi) qpmi = qgs->addPixmap(qpm);//QPixmap::fromImage(*qim));
   else qpmi->setPixmap(qpm);
        
   if(userBox) qpmi->stackBefore(userBox);
   if(statsBox) qpmi->stackBefore(statsBox);
   if(guideBox) qpmi->stackBefore(guideBox);
   
   
   
//    else
//    {
      
//    if(targetVisible)
//    {
//       qpmi->stackBefore(cenLineVert);
//       qpmi->stackBefore(cenLineHorz);
//    }
   
   if(imcp)
   {
      if(imcp->ViewViewMode == ViewViewEnabled)
      {
         if(!imcp->qpmi_view) imcp->qpmi_view = imcp->qgs_view->addPixmap(qpm);
         imcp->qpmi_view->setPixmap(qpm);
         
         imcp->qpmi_view->stackBefore(imcp->viewLineVert);
      }
   }
   update_MouseCoords(); //This is to update the pixel val box if set.
   
   if(imcp)
   {
      imcp->update_panel();
   }
   
#if RT_SYSTEM == RT_SYSTEM_VISAO
   if(imStats) 
   {
      if(applyDark && dark_sim.imdata) imStats->set_imdata(imdata, frame_time, dark_sim.imdata);
      else  imStats->set_imdata(imdata, frame_time,0);
   }
#endif

#if RT_SYSTEM == RT_SYSTEM_SCEXAO
   if(imStats) 
   {
      imStats->set_imdata(imdata, frame_time,0);
   }
#endif

#if RT_SYSTEM == RT_SYSTEM_VISAO
   size_t sz;
   if(!aosb && useAOStatus) aosb = (VisAO::aosystem_status_board*) attach_shm(&sz,  STATUS_aosystem, 0);
   
   if(aosb && useAOStatus)
   {
      //std::cout << -aosb->pa << "\n";
      float sgn = -1;
      nup->setRotation(aosb->rotoffset+90);//(aosb->rotang-aosb->el)+sgn*aosb->pa);
      nup_tip->setRotation(aosb->rotoffset+90);//(aosb->rotang-aosb->el)+sgn*aosb->pa);
   }
   
#endif

   if(applyDarkChanged)
   {
      applyDarkChanged = 0;
      reStretch();
      
   }
}

void imviewerForm::launchControlPanel()
{
   if(!imcp)
   {
      imcp = new imviewerControlPanel(this, Qt::Tool);
      connect(imcp, SIGNAL(launchStatsBox()), this, SLOT(doLaunchStatsBox()));
      connect(imcp, SIGNAL(hideStatsBox()), this, SLOT(doHideStatsBox()));
   }
   
   imcp->show();
   
   imcp->activateWindow();
}



// void imviewerForm::on_buttonPanelLaunch_clicked()
// {
//    launchControlPanel();
// }
// 
void imviewerForm::freezeRealTime()
{
   if(RealTimeStopped)
   {
      set_RealTimeStopped(false);
   }
   else
   {
      set_RealTimeStopped(true);
      ui.graphicsView->fpsGageText(0.0);
   }
}

void imviewerForm::reStretch()
{
   
   if(get_abs_fixed())
   {
      if(get_colorbar_mode() == user)
      {
         set_colorbar_mode(minmaxglobal);
      }
      
      if(get_colorbar_mode() == minmaxglobal)
      {
         set_mindat(get_imdat_min());
         set_maxdat(get_imdat_max());
         changeImdata(false);
      }

      if(get_colorbar_mode() == minmaxbox)
      {
         set_mindat(userBox_min);
         set_maxdat(userBox_max);
         changeImdata(false);
      }
   }
}
// 
// void imviewerForm::on_darkSubCheckBox_stateChanged(int st)
// {
//    if(st == 0)
//    {
//       applyDark = 0;
//       applyDarkChanged = 1;
//    }
//    else
//    {
//       applyDark = 1;
//       applyDarkChanged = 1;
//       //std::cout << dark_sim.imdata << std::endl;
//       
//    }
// }


float imviewerForm::get_act_xcen()
{
   return ui.graphicsView->actXCen();
}

float imviewerForm::get_act_ycen()
{
   return ui.graphicsView->actYCen();
}

void imviewerForm::setPointerOverZoom(float poz)
{
   pointerOverZoom = poz;
   post_set_ZoomLevel();
}



void imviewerForm::change_center(bool movezoombox)
{   
   ui.graphicsView->centerOn(ui.graphicsView->actXCen()*ui.graphicsView->nX(), ui.graphicsView->actYCen()*ui.graphicsView->nY());
  
   if(imcp)
   {
      
      imcp->viewLineVert->setLine(ui.graphicsView->xCen()*nx, 0, ui.graphicsView->xCen()*nx, ny);
      imcp->viewLineHorz->setLine(0, ui.graphicsView->yCen()*ny, nx, ui.graphicsView->yCen()*ny);
      
      if(ZoomLevel <= 1.0) imcp->viewBox->setVisible(false);
      else
      {
         imcp->viewBox->setVisible(true);
         if(movezoombox)
         {
            QPointF tmpp = imcp->viewBox->mapFromParent(ui.graphicsView->actXCen()*nx - .5*nx/ZoomLevel, ui.graphicsView->actYCen()*ny-.5*ny/ZoomLevel);
            imcp->viewBox->setRect(tmpp.x(), tmpp.y(), nx/ZoomLevel, ny/ZoomLevel);
            //imcp->viewBox->xoff = tmpp.x();
            //imcp->viewBox->yoff = tmpp.y();
         }
         
      }
      imcp->ui.viewView->centerOn(.5*nx, .5*ny);
      imcp->update_panel();
   }
   
}

void imviewerForm::set_viewcen(float x, float y, bool movezoombox)
{
   ui.graphicsView->xCen(x);
   ui.graphicsView->yCen(y);
   change_center(movezoombox);
}

void imviewerForm::changeCenter()
{
   change_center();
}

void imviewerForm::resizeEvent(QResizeEvent *)
{
   ScreenZoom = std::max((float)width()/(float)nx,(float)height()/(float)ny);
   change_center();
   ui.graphicsView->setGeometry(0,0,width(), height());
   post_set_ZoomLevel();
   
}

void imviewerForm::mouseMoveEvent(QMouseEvent *e)
{
   nullMouseCoords();
   (void)(e);
}

void imviewerForm::nullMouseCoords()
{
   if(!NullMouseCoords)
   {
      if(imcp)
      {
         imcp->nullMouseCoords();
      }
      
      NullMouseCoords = true;
      
      ui.graphicsView->textCoordX("");
      ui.graphicsView->textCoordY("");
      ui.graphicsView->textPixelVal("");
   }
}

void imviewerForm::update_MouseCoords()
{
//   char tmpr[20];

   int idx_x, idx_y;
   
   if(!imdata) return;
   
   if(ui.graphicsView->mouseImX() < 0 || ui.graphicsView->mouseImY() < 0)
   {
      nullMouseCoords();
   }
   
   if(!NullMouseCoords)
   {
      ui.graphicsView->textCoordX(ui.graphicsView->mouseImX());
      ui.graphicsView->textCoordY(ui.graphicsView->mouseImY());
      
      
      idx_x = ((int)(ui.graphicsView->mouseImX()));
      if(idx_x < 0) idx_x = 0;
      if(idx_x > nx-1) idx_x = nx-1;
      idx_y = (int)(ui.graphicsView->mouseImY());
      if(idx_y < 0) idx_y = 0;
      if(idx_y > ny-1) idx_y = ny-1;

      ui.graphicsView->textPixelVal(pixget(imdata, (int)(idx_y*ny) + (int)(idx_x)));
      
      if(imcp)
      {
         #if RT_SYSTEM == RT_SYSTEM_VISAO        
         if(!applyDark)
         {
            imcp->updateMouseCoords(ui.graphicsView->mouseImX(), ui.graphicsView->mouseImY(), pixget(imdata,idx_y*ny + idx_x ));
         }
         else
         {
            imcp->updateMouseCoords(ui.graphicsView->mouseImX(), ui.graphicsView->mouseImY(), ( pixget(imdata,idx_y*ny + idx_x)- pixget(dark_sim.imdata,(int)(idx_y*ny) + (int)(idx_x)) ));
         }
         #else
         imcp->updateMouseCoords(ui.graphicsView->mouseImX(), ui.graphicsView->mouseImY(), pixget(imdata,idx_y*ny + idx_x) );
         #endif

      }
   }
   
   if(rightClickDragging)
   {
      float dx = ui.graphicsView->mouseImX() - rightClickStart.x()*nx;
      float dy = ui.graphicsView->mouseImY() - rightClickStart.y()*ny;
      
      float dbias = dx/(nx/ZoomLevel);
      float dcontrast = -1.*dy/(ny/ZoomLevel);
      
      set_bias(biasStart + .5*dbias*.5*(imdat_max+imdat_min));
      set_contrast(contrastStart + dcontrast*.5*(imdat_max-imdat_min));
      if(!amChangingimdata) changeImdata();
   }
}

void imviewerForm::changeMouseCoords()
{
   NullMouseCoords = false;
   update_MouseCoords();
}

void imviewerForm::viewLeftPressed(QPointF mp)
{
   if(imcp)
   {
      imcp->viewLeftPressed(mp);
   }
}

void imviewerForm::viewLeftClicked(QPointF mp)
{
   if(imcp)
   {
      imcp->viewLeftClicked(mp);
   }
}

void imviewerForm::viewRightPressed(QPointF mp)
{
   rightClickDragging = true;
   rightClickStart = mp;
   biasStart = get_bias();
   contrastStart = get_contrast();
   
}

void imviewerForm::viewRightClicked(QPointF mp)
{
   rightClickDragging = false;  
   (void)(mp);
}

void imviewerForm::onWheelMoved(int delta)
{
   float dz;
   //std::cout << delta << "\n";
   if(delta > 0)   dz = 1.41421;//*delta/120.;
   else dz = 0.70711;//*delta/120.;
   
   set_ZoomLevel(dz*get_ZoomLevel());
   
}


// void imviewerForm::stale_fps()
// {
//    
// }

void imviewerForm::update_age()
{
   struct timeval tvtmp;
    
   gettimeofday(&tvtmp, 0);
   double timetmp = (double)tvtmp.tv_sec + ((double)tvtmp.tv_usec)/1e6;
   
   double age = timetmp - fps_time0;
   
   if(age <= 1.0 && fps_ave > 1.0) 
   {
      ui.graphicsView->fpsGageText(fps_ave);
   }
   else
   {
      ui.graphicsView->fpsGageText(timetmp-fps_time0, true);
      
      //This reset the fps averaging so that it doesn't jitter when next image shows up.
      if(fps_hist.size() >= n_ave_fps) fps_hist.clear();      
   } 
   

   
   ui.graphicsView->loopText("Loop OPEN", "red");
  
  if(curr_saved == 1)
  {
     ui.graphicsView->saveBoxText("S", "lime");
  }
  else
  {
     ui.graphicsView->saveBoxText("X", "red");
  }   
}




void imviewerForm::doLaunchStatsBox()
{
   statsBox->setVisible(true);
   
   if(!imStats)
   {
      imStats = new imviewerStats(pixget, type_size, this, 0);
      imStats->setAttribute(Qt::WA_DeleteOnClose); //Qt will delete imstats when it closes.
#if RT_SYSTEM == RT_SYSTEM_VISAO
      if(applyDark) imStats->set_imdata(imdata, frame_time, dark_sim.imdata);
      else imStats->set_imdata(imdata, frame_time, 0);
#else
      imStats->set_imdata(imdata, frame_time, 0);
#endif
      connect(imStats, SIGNAL(finished(int )), this, SLOT(imStatsClosed(int )));
   }

   statsBoxMoved(statsBox->rect());

   imStats->show();
    
   imStats->activateWindow();
}

void imviewerForm::doHideStatsBox()
{
   statsBox->setVisible(false);

   if (imStats)
   {
      //imStats->hide();
      imStats->close(); 
      imStats = 0; //imStats is set to delete on close
   }
}

void imviewerForm::imStatsClosed(int result)
{
   statsBox->setVisible(false);
   imStats = 0; //imStats is set to delete on close
   if(imcp)
   {
      imcp->statsBoxButtonState = false;
      imcp->ui.statsBoxButton->setText("Show Stats Box");
   }
   //imcp->on_statsBoxButton_clicked();
   //doHideStatsBox();
   
   (void)(result);
}

void imviewerForm::statsBoxMoved(const QRectF & newr)
{

   QPointF np = qpmi->mapFromItem(statsBox, QPointF(statsBox->rect().x(),statsBox->rect().y()));
   QPointF np2 = qpmi->mapFromItem(statsBox, QPointF(statsBox->rect().x()+statsBox->rect().width(),statsBox->rect().y()+statsBox->rect().height()));

   if(imStats) 
   {
#if RT_SYSTEM == RT_SYSTEM_VISAO
      if(applyDark) imStats->set_imdata(imdata, frame_time, nx, ny, np.x() + .5, np2.x(), ny-np2.y()+.5, ny-np.y(), dark_sim.imdata);
      else imStats->set_imdata(imdata, frame_time, nx, ny, np.x() + .5, np2.x(), ny-np2.y()+.5, ny-np.y(), 0);
#else
      imStats->set_imdata(imdata, frame_time, nx, ny, np.x() + .5, np2.x(), ny-np2.y()+.5, ny-np.y(), 0);
#endif 
   }
   
   (void)(newr);

}

void imviewerForm::statsBoxRejectMouse()
{
   statsBox->stackBefore(userBox);
   statsBox->stackBefore(guideBox);
   statsBox->stackBefore(userCircle);
}

void imviewerForm::userBoxMoved(const QRectF & newr)
{
   
   QPointF np = qpmi->mapFromItem(userBox, QPointF(newr.x(),newr.y()));
   QPointF np2 = qpmi->mapFromItem(userBox, QPointF(newr.x()+newr.width(),newr.y()+newr.height()));

   userBox_j0 = (int) (np2.x() + .5);
   userBox_j1 = (int) np.x();
   userBox_i0 = ny-(int) (np2.y() + .5);
   userBox_i1 = ny-(int) np.y();

   setUserBoxActive(true); //recalcs and recolors.
   
   
}

void imviewerForm::userBoxRejectMouse()
{
   
   userBox->stackBefore(statsBox);
   userBox->stackBefore(guideBox);
   userBox->stackBefore(userCircle);
   
}


void imviewerForm::guideBoxMoved(const QRectF & newr)
{
   
   QPointF np = qpmi->mapFromItem(guideBox, QPointF(newr.x(),newr.y()));
   QPointF np2 = qpmi->mapFromItem(guideBox, QPointF(newr.x()+newr.width(),newr.y()+newr.height()));

   
   guideBox_i0 = np.x() + .5;
   guideBox_i1 = np2.x();
   
   guideBox_j0 = ny-np2.y() + .5;
   guideBox_j1 = ny-np.y();
  
   //std::cout << guideBox_i0 << " " << guideBox_j0 << " " << guideBox_i1 << " " << guideBox_j1 << "\n";
   
#if RT_SYSTEM == RT_SYSTEM_VISAO
   if(statusboard_shmemptr > 0)
   {
      VisAO::imviewer_status_board * isb = (VisAO::imviewer_status_board *) statusboard_shmemptr;
      
      isb->guidebox_x0 = guideBox_i0;
      isb->guidebox_x1 = guideBox_i1;
      isb->guidebox_y0 = guideBox_j0;
      isb->guidebox_y1 = guideBox_j1;
   }
#endif

}

void imviewerForm::guideBoxRejectMouse()
{
   
   guideBox->stackBefore(statsBox);
   guideBox->stackBefore(userBox);
   guideBox->stackBefore(userCircle);
}

void imviewerForm::userCircleResized(const float &rad)
{
   //std::cout << rad << "\n";

   char tmp[256];
   snprintf(tmp, 256, "%0.1f", rad);
   
   ui.graphicsView->coords->setText(tmp);
}

void imviewerForm::userCircleMoved(const QRectF &newr)
{
   QPointF np = qpmi->mapFromItem(userCircle, QPointF(newr.x(),newr.y()));
   
   float x = np.x()+.5*newr.width();
   float y = np.y()+.5*newr.height();
   
   //std::cout << ScreenZoom << " " << ZoomLevel << "\n";
   ui.graphicsView->coords->setGeometry(x*ScreenZoom-35., y*ScreenZoom-20., 70,40);
  
}

void imviewerForm::userCircleMouseIn()
{
   ui.graphicsView->coords->setVisible(true);
   
}
void imviewerForm::userCircleMouseOut()
{
   ui.graphicsView->coords->setVisible(false);
}

void imviewerForm::userCircleRejectMouse()
{
   
   userCircle->stackBefore(statsBox);
   userCircle->stackBefore(guideBox);
   userCircle->stackBefore(userBox);
   
}


void imviewerForm::post_setUserBoxActive(bool usba)
{
   userBox->setVisible(usba);
}


void imviewerForm::keyPressEvent(QKeyEvent * ke)
{

   if(ke->text() == "p")
   {
      launchControlPanel();
      return;
   }

   if(ke->text() == "r")
   {
      reStretch();
      return;
   }

   if(ke->text() == "s")
   {
      if(statsBox->isVisible())
      {
         doHideStatsBox();
         if(imcp)
         {
            imcp->statsBoxButtonState = false;
            imcp->ui.statsBoxButton->setText("Show Stats Box");
         }
      }
      else
      {
         doLaunchStatsBox();
         if(imcp)
         {
            imcp->statsBoxButtonState = true;
            imcp->ui.statsBoxButton->setText("Hide Stats Box");
         }
      }
      return;
   }

   if(ke->text() == "x")
   {
      freezeRealTime();
      return;
   }
   
   if(ke->text() == "1")
   {
      set_ZoomLevel(1.0);
      return;
   }
   
   if(ke->text() == "2")
   {
      set_ZoomLevel(2.0);
      return;
   }
   
   if(ke->text() == "3")
   {
      set_ZoomLevel(3.0);
      return;
   }
   
   if(ke->text() == "4")
   {
      set_ZoomLevel(4.0);
      return;
   }
   
   if(ke->text() == "5")
   {
      set_ZoomLevel(5.0);
      return;
   }
   
   if(ke->text() == "6")
   {
      set_ZoomLevel(6.0);
      return;
   }
   
   if(ke->text() == "7")
   {
      set_ZoomLevel(7.0);
      return;
   }
   
   if(ke->text() == "8")
   {
      set_ZoomLevel(8.0);
      return;
   }
   
   if(ke->text() == "9")
   {
      set_ZoomLevel(9.0);
      return;
   }
   
   if(ke->text() == "b")
   {
      if(!userBoxActive)
      {
         if(imcp)
         {
            imcp->on_scaleModeCombo_activated(imviewer::minmaxbox);
         }
         else
         {
            userBox->setVisible(true);
            setUserBoxActive(true);
         }
      }
      else
      {
         if(imcp)
         {
            imcp->on_scaleModeCombo_activated(imviewer::minmaxglobal);
         }
         else
         {
            userBox->setVisible(false);
            setUserBoxActive(false);
         }
      }
      return;
   }
   
   if(ke->text() == "n")
   {
      if(!nup) return;
      if(nup->isVisible())
      {
         nup->setVisible(false);
         nup_tip->setVisible(false);
      }
      else
      {
         nup->setVisible(true);
         nup_tip->setVisible(true);
      }
      return;
   }

   if(ke->text() == "g")
   {
      if(!guideBox->isVisible())
      {
         guideBox->setVisible(true);
         
      }
      else
      {
         guideBox->setVisible(false);
         
      }
      return;
   }
   
   if(ke->text() == "t")
   {
      if(targetVisible)
      {
         cenLineVert->setVisible(false);
         cenLineHorz->setVisible(false);
         targetVisible = false;
      }
      else
      {
         cenLineVert->setVisible(true);
         cenLineHorz->setVisible(true);
         targetVisible=true;
      }
   } 
   
   if(ke->text() == "c")
   {
      set_viewcen(.5, .5);
      post_set_ZoomLevel();
   }
   
   if(ke->text() == "o")
   {
      if(userCircle->isVisible()) userCircle->setVisible(false);
      else userCircle->setVisible(true);
      
   }
   
   QWidget::keyPressEvent(ke);
}
