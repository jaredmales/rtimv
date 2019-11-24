#include "imviewerform.hpp"

imviewerForm::imviewerForm( const std::vector<std::string> & shkeys, 
                            QWidget * Parent, 
                            Qt::WindowFlags f
                          ) : imviewer(shkeys, Parent, f)
{
   ui.setupUi(this);
   
   nup =0;
   
   imcp = 0;
   pointerOverZoom = 4.;
   
   resize(height(), height()); //make square.
   setWindowTitle(shkeys[0].c_str());
   
   //This will come up at some minimal size.
   ui.graphicsView->setGeometry(0,0, width(), height());
   
   qgs = new QGraphicsScene();
   ui.graphicsView->setScene(qgs);
   
   qpmi = 0;
   userBox = 0;
   statsBox = 0;
   
   
   rightClickDragging = false;
   
   NullMouseCoords = true;
   
   
   mindat(400);
   
   maxdat(600);
   

   

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
  
   
   
   m_targetVisible = false;
   
   m_cenLineVert = 0;//qgs->addLine(QLineF(.5*getNx(),0, .5*getNx(), getNy()), QColor("lime"));
   m_cenLineHorz = 0;//qgs->addLine(QLineF(0, .5*getNy(), getNx(), .5*getNy()), QColor("lime"));
   
   imStats = 0;
   m_timer.start(m_timeout);

   nup = qgs->addLine(QLineF(512,400, 512, 624), QColor("skyblue"));
   nup_tip = qgs->addLine(QLineF(512,400, 536, 424), QColor("skyblue"));
   nup->setTransformOriginPoint ( QPointF(512,512) );
   nup_tip->setTransformOriginPoint ( QPointF(512,512) );
   nup->setVisible(false);
   nup_tip->setVisible(false);
   
   QPen qp = nup->pen();
   qp.setWidth(5);

   nup->setPen(qp);
   nup_tip->setPen(qp);
   
      
}

void imviewerForm::onConnect()
{
   squareDown();
}

void imviewerForm::postSetImsize()
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
   
   

   //Resize the user color box
   userBox_i0 = m_ny*.25;
   userBox_i1 = m_ny*.75;

   userBox_j0 = m_nx*.25;
   userBox_j1 = m_nx*.75;

   //std::cout << userBox_i0 << " " << userBox_i1 - userBox_i0 << " " << userBox_j0 << " " << userBox_j1 - userBox_j0<< "\n";
   userBox->setRect(userBox->mapRectFromScene(userBox_i0, userBox_j0, userBox_i1-userBox_i0, userBox_j1-userBox_j0));
   userBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);

   //resize the stats box
   statsBox->setRect(statsBox->mapRectFromScene(m_ny*.25, m_nx*.3, .4*m_ny, .4*m_nx));
   statsBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);
   //statsBoxMoved(statsBox->rect());
   
   //resize the guide box
   guideBox->setRect(statsBox->mapRectFromScene(m_ny*.3, m_nx*.3, .4*m_ny, .4*m_nx));
   guideBox->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);
   //guideBoxMoved(guideBox->rect());
   
   //resize the circle
   userCircle->setRect(userCircle->mapRectFromScene(m_ny*.35, m_nx*.35, .4*m_ny, .4*m_nx));
   userCircle->setEdgeTol(5./ScreenZoom < 5 ? 5 : 5./ScreenZoom);
   //userCircleMoved(guideBox->rect());
   
   setTarget();
}

void imviewerForm::setTarget()
{
   if(!m_cenLineVert)
   {
      m_cenLineVert = qgs->addLine(QLineF(m_targetXc*nx(),0, m_targetXc*nx(), ny()), QColor("lime"));
      m_cenLineHorz = qgs->addLine(QLineF(0, (1.0-m_targetYc)*ny(), nx(), (1.0-m_targetYc)*ny()), QColor("lime"));
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

void imviewerForm::post_zoomLevel()
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

void imviewerForm::postChangeImdata()
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
   
   if(!qpmi) //This happens on first time through
   {
      qpmi = qgs->addPixmap(m_qpm);
      //So we need to initialize the viewport center, etc.
      set_viewcen(0.5,0.5);
      post_zoomLevel();
   }
   else qpmi->setPixmap(m_qpm);
        
   if(userBox) qpmi->stackBefore(userBox);
   if(statsBox) qpmi->stackBefore(statsBox);
   if(guideBox) qpmi->stackBefore(guideBox);
   
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
      //imStats->set_imdata(m_imData, frame_time,0);
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
      if(m_showFPSGage) ui.graphicsView->fpsGageText(0.0);
   }
}

void imviewerForm::reStretch()
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
      mindat(userBox_min);
      maxdat(userBox_max);
      changeImdata(false);
   }
}



void imviewerForm::setPointerOverZoom(float poz)
{
   pointerOverZoom = poz;
   post_zoomLevel();
}



void imviewerForm::change_center(bool movezoombox)
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

void imviewerForm::set_viewcen(float x, float y, bool movezoombox)
{
   QPointF sp( x* qpmi->boundingRect().width(), y*qpmi->boundingRect().height() );
   QPointF vp = ui.graphicsView->mapFromScene(sp);
   
   ui.graphicsView->mapCenterToScene(vp.x(), vp.y());
   change_center(movezoombox);
}

void imviewerForm::squareDown()
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

void imviewerForm::squareUp()
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
      
void imviewerForm::changeCenter()
{
   change_center();
}

void imviewerForm::resizeEvent(QResizeEvent *)
{
   ScreenZoom = std::min((float)width()/(float)m_nx,(float)height()/(float)m_ny);
   change_center();
   ui.graphicsView->setGeometry(0,0,width(), height());
   post_zoomLevel();
   
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

void imviewerForm::updateMouseCoords()
{
   int64_t idx_x, idx_y; //image size are uint32_t, so this allows signed comparison without overflow issues
   
   if(!qpmi) return;
   
   if(ui.graphicsView->mouseViewX() < 0 || ui.graphicsView->mouseViewY() < 0)
   {
      nullMouseCoords();
   }
   
   QPointF pt = ui.graphicsView->mapToScene(ui.graphicsView->mouseViewX(),ui.graphicsView->mouseViewY());
   
   float mx = pt.x();
   float my = pt.y();
   
   if( mx < 0 || mx > qpmi->boundingRect().width() || my < 0 || my > qpmi->boundingRect().height() ) 
   {
      nullMouseCoords();
   }
   
   if(!NullMouseCoords)
   {
      ui.graphicsView->textCoordX(mx-0.5);
      ui.graphicsView->textCoordY(qpmi->boundingRect().height() - my-0.5);
      
      
      idx_x = ((int64_t)(mx-0));
      if(idx_x < 0) idx_x = 0;
      if(idx_x > (int64_t) m_nx-1) idx_x = m_nx-1;
      
      idx_y = (int)(qpmi->boundingRect().height() - (my-0));
      if(idx_y < 0) idx_y = 0;
      if(idx_y > (int64_t) m_ny-1) idx_y = m_ny-1;

      pixelF _pixel = pixel();
      
      if(_pixel != nullptr)
      ui.graphicsView->textPixelVal(_pixel(this,  (int)(idx_y*m_nx) + (int)(idx_x)) );

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

void imviewerForm::changeMouseCoords()
{
   NullMouseCoords = false;
   updateMouseCoords();
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
   
   rightClickStart = mp;//ui.graphicsView->mapToScene(mp.x(),mp.y());
   biasStart = bias();
   contrastStart = contrast();
   
}

void imviewerForm::viewRightClicked(QPointF mp)
{
   rightClickDragging = false;  
   (void)(mp);
}

void imviewerForm::onWheelMoved(int delta)
{
   float dz;
   if(delta > 0)   dz = 1.02;//1.41421;
   else dz = 0.98;//0.70711;
   
   zoomLevel(dz*zoomLevel());
   
}


void imviewerForm::updateFPS()
{
   updateAge();
}

void imviewerForm::updateAge()
{
   if(m_showFPSGage && m_images[0] != nullptr  )
   {      
      if(m_images[0]->valid())    //have to check this after checking nullptr
      {
         struct timespec tstmp;
         
         clock_gettime(CLOCK_REALTIME, &tstmp);
            
         double timetmp = (double)tstmp.tv_sec + ((double)tstmp.tv_nsec)/1e9;
         
         double fpsTime = m_images[0]->m_image.md->atime.tv_sec + ((double) m_images[0]->m_image.md->atime.tv_nsec)/1e9;
         
         double age = timetmp - fpsTime;
            
         if(m_images[0]->m_fpsEst > 1.0 && age < 2.0) 
         {
            ui.graphicsView->fpsGageText(m_images[0]->m_fpsEst);
         }
         else
         {
            ui.graphicsView->fpsGageText(age, true);
         } 
      }
   }

   
   if(m_showLoopStat)
   {
      ui.graphicsView->loopText("Loop OPEN", "red");
   }
   
//    if(m_showSaveStat)
//    {
//       if(curr_saved == 1)
//       {
//          ui.graphicsView->saveBoxText("S", "lime");
//       }
//       else
//       {
//          ui.graphicsView->saveBoxText("X", "red");
//       }
//    }
}


int imviewerForm::targetXc( float txc )
{
   if(txc < 0) txc = 0;
   if(txc > 1) txc = 1;
   
   m_targetXc = txc;
   
   setTarget();
   
   return 0;
}
     
float imviewerForm::targetXc()
{
   return m_targetXc;
}
      
int imviewerForm::targetYc( float tyc )
{
   if(tyc < 0) tyc = 0;
   if(tyc > 1) tyc = 1;
   
   m_targetYc = tyc;
   
   setTarget();
   
   return 0;
}
     
float imviewerForm::targetYc()
{
   return m_targetYc;
}
      

void imviewerForm::doLaunchStatsBox()
{
   return;
   
   /*
   statsBox->setVisible(true);
   
   if(!imStats)
   {
//       imStats = new imviewerStats(pixget, type_size, this, 0);
//       imStats->setAttribute(Qt::WA_DeleteOnClose); //Qt will delete imstats when it closes.
      //imStats->set_imdata(m_imData, frame_time, 0);
      connect(imStats, SIGNAL(finished(int )), this, SLOT(imStatsClosed(int )));
   }

   statsBoxMoved(statsBox->rect());

   imStats->show();
    
   imStats->activateWindow();
   */
}

void imviewerForm::doHideStatsBox()
{
   return;
   /*
   statsBox->setVisible(false);

   if (imStats)
   {
      //imStats->hide();
      imStats->close(); 
      imStats = 0; //imStats is set to delete on close
   }
   */
}

void imviewerForm::imStatsClosed(int result)
{
   /*
   statsBox->setVisible(false);
   imStats = 0; //imStats is set to delete on close
   if(imcp)
   {
      imcp->statsBoxButtonState = false;
      imcp->ui.statsBoxButton->setText("Show Stats Box");
   }
   //imcp->on_statsBoxButton_clicked();
   //doHideStatsBox();
   */
   (void)(result);
   
}

void imviewerForm::statsBoxMoved(const QRectF & newr)
{
   /*
   QPointF np = qpmi->mapFromItem(statsBox, QPointF(statsBox->rect().x(),statsBox->rect().y()));
   QPointF np2 = qpmi->mapFromItem(statsBox, QPointF(statsBox->rect().x()+statsBox->rect().width(),statsBox->rect().y()+statsBox->rect().height()));

   if(imStats) 
   {
      //imStats->set_imdata(m_imData, frame_time, m_nx, m_ny, np.x() + .5, np2.x(), m_ny-np2.y()+.5, m_ny-np.y(), 0);
   }
   */
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

   userBox_i1 = (int) (np2.x() + .5);
   userBox_i0 = (int) np.x();
   userBox_j0 = m_ny-(int) (np2.y() + .5);
   userBox_j1 = m_ny-(int) np.y();
   
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
   
   guideBox_j0 = m_ny-np2.y() + .5;
   guideBox_j1 = m_ny-np.y();
  

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
   char key = ke->text()[0].toLatin1();
   
   switch(key)
   {
      case 'a':
         toggleAutoScale();
         break;
      case 'b':
         toggleUserBox();
         break;
      case 'c':
         center();
         break;
      case 'f':
         toggleFPSGage();
         break;
      case 'p':
         launchControlPanel();
         break;
      case 'r':
         reStretch();
         break;
      case 's':
         toggleStatsBox();
         break;
      case 't':
         toggleTarget();
         break;
      case 'x':
         freezeRealTime();
         break;
      case 'D':
         toggleDarkSub();
         break;
      case 'L':
         toggleLogLinear();
         break;
      case 'M':
         toggleApplyMask();
         break;
      case 'S':
         toggleApplySatMask();
         break;
      case '1':
         zoomLevel(1.0);
         break;
      case '2':
         zoomLevel(2.0);
         break;
      case '3':
         zoomLevel(3.0);
         break;
      case '4':
         zoomLevel(4.0);
         break;
      case '5':
         zoomLevel(5.0);
         break;
      case '6':
         zoomLevel(6.0);
         break;
      case '7':
         zoomLevel(7.0);
         break;
      case '8':
         zoomLevel(8.0);
         break;
      case '9':
         zoomLevel(9.0);
         break;
      case '[':
         squareDown();
         break;
      case ']':
         squareUp();
         break;
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
   
    
   
   
   if(ke->text() == "o")
   {
      if(userCircle->isVisible()) userCircle->setVisible(false);
      else userCircle->setVisible(true);
      
   }
   
   QWidget::keyPressEvent(ke);
}

int imviewerForm::setAutoScale( bool as )
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

   return 0;
}

int imviewerForm::toggleAutoScale()
{
   if(m_autoScale) 
   {
      return setAutoScale(false);
   }
   else 
   {
      return setAutoScale(true);
   }      
}

int imviewerForm::center()
{
   set_viewcen(.5, .5);
   post_zoomLevel();
      
   ui.graphicsView->zoomText("centered");
   
   return 0;
}

int imviewerForm::toggleUserBox()
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
      ui.graphicsView->zoomText("user box scale");
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
      ui.graphicsView->zoomText("global scale");
   }
   return 0;   
}

int imviewerForm::toggleStatsBox()
{
   if(statsBox->isVisible())
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
   
   return 0;
}

int imviewerForm::showFPSGage( bool sfg )
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
   
   return 0;
}

int imviewerForm::toggleFPSGage()
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

int imviewerForm::setDarkSub( bool ds )
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
   return 0;
}

int imviewerForm::toggleDarkSub()
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

int imviewerForm::setApplyMask( bool am )
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
   return 0;
}

int imviewerForm::toggleApplyMask()
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

int imviewerForm::setApplySatMask( bool as )
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
   
   return 0;
}

int imviewerForm::toggleApplySatMask()
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

int imviewerForm::toggleLogLinear()
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
      
   return 0;
}

int imviewerForm::toggleTarget()
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
      
   return 0;
}
