#include "rtimvControlPanel.hpp"

rtimvControlPanel::rtimvControlPanel(rtimvMainWindow *v, Qt::WindowFlags f): QWidget(0, f)
{
   ui.setupUi(this);
   imv = v;
   
   setupMode();
   setupCombos();
   ui.tabWidget->setCurrentIndex(0);
   
   IgnoreZoomSliderChange = false;
   IgnoremaxdatSliderChange = false;
   IgnoremindatSliderChange = false;
   
   qgs_view = new QGraphicsScene();
   qgs_empty = new QGraphicsScene();
   
   ui.pointerView->setScene(qgs_empty);
   PointerViewEmpty = true;
   
   qpmi_view = qgs_view->addPixmap(*(v->getPixmap()));
      
   ui.viewView->setScene(qgs_view);
   
   viewLineVert = qgs_view->addLine(QLineF(.5*imv->nx(),0, .5*imv->nx(), imv->ny()), QColor("lime"));
   viewLineHorz = qgs_view->addLine(QLineF(0, .5*imv->ny(), imv->nx(), .5*imv->ny()), QColor("lime"));
   //viewBox = qgs_view->addRect(QRectF(0,0, imv->get_nx(), imv->get_ny()), QColor("lime"));
   viewBox = new StretchBox(0,0, imv->nx(), imv->ny());
   viewBox->setPen(QPen("lime"));
   viewBox->setFlag(QGraphicsItem::ItemIsSelectable, true);
   viewBox->setFlag(QGraphicsItem::ItemIsMovable, true);
   viewBox->setStretchable(false);
   viewBox->setEdgeTol(imv->nx());//5.*imv->nx()/ui.viewView->width() < 5 ? 5 : 5.*imv->nx()/ui.viewView->width() );
   qgs_view->addItem(viewBox);
   
   double viewZoom = (double)ui.viewView->width()/(double)imv->nx();
   ui.viewView->scale(viewZoom,viewZoom);
   
   PointerViewFixed = false;
   PointerViewWaiting = false;
   
   connect(imv, SIGNAL(showToolTipCoordsChanged(bool)), this, SLOT(showToolTipCoordsChanged(bool)));
   connect(this, SIGNAL(showToolTipCoords(bool)), imv, SLOT(showToolTipCoords(bool)));
   connect(imv, SIGNAL(showStaticCoordsChanged(bool)), this, SLOT(showStaticCoordsChanged(bool)));
   connect(this, SIGNAL(showStaticCoords(bool)), imv, SLOT(showStaticCoords(bool)));

   connect(imv, SIGNAL(targetXcChanged(float)), this, SLOT(targetXcChanged(float)));
   connect(imv, SIGNAL(targetYcChanged(float)), this, SLOT(targetYcChanged(float)));
   connect(imv, SIGNAL(targetVisibleChanged(bool)), this, SLOT(targetVisibleChanged(bool)));
                                                            

   connect(this, SIGNAL(targetXc(float)), imv, SLOT(targetXc(float)));
   connect(this, SIGNAL(targetYc(float)), imv, SLOT(targetYc(float)));
   connect(this, SIGNAL(targetVisible(bool)), imv, SLOT(targetVisible(bool)));

   init_panel();
   
   //connect(viewBox, SIGNAL(moved(const QRectF & )), this, SLOT(viewBoxMoved(const QRectF &)));

   statsBoxButtonState = false;

}

void rtimvControlPanel::setupMode()
{
   ViewViewMode = ViewViewNoImage;// ViewViewEnabled;
   PointerViewMode = PointerViewOnPress;//PointerViewEnabled;
}

void rtimvControlPanel::setupCombos()
{
   //First we insert a bunch of blanks so inserting by enum works correctly.
   for(int i=0;i<rtimvBase::colorbar_modes_max;i++) ui.scaleModeCombo->insertItem(i,"");
   ui.scaleModeCombo->insertItem(rtimvBase::minmaxglobal, "Min/Max Global");
   ui.scaleModeCombo->insertItem(rtimvBase::minmaxview, "Min/Max View");
   ui.scaleModeCombo->insertItem(rtimvBase::minmaxbox, "Min/Max Box");
   ui.scaleModeCombo->insertItem(rtimvBase::user, "User");
   ui.scaleModeCombo->setCurrentIndex(rtimvBase::user);
   ui.scaleModeCombo->setMaxCount(rtimvBase::colorbar_modes_max);

   for(int i=0;i<rtimvBase::cbStretches_max;i++) ui.scaleTypeCombo->insertItem(i,"");
   ui.scaleTypeCombo->insertItem(rtimvBase::stretchLinear, "Linear");
   ui.scaleTypeCombo->insertItem(rtimvBase::stretchLog, "Log");
   ui.scaleTypeCombo->insertItem(rtimvBase::stretchPow, "Power");
   ui.scaleTypeCombo->insertItem(rtimvBase::stretchSqrt, "Square Root");
   ui.scaleTypeCombo->insertItem(rtimvBase::stretchSquare, "Squared");
   ui.scaleTypeCombo->setMaxCount(rtimvBase::cbStretches_max);
   
   for(int i=0;i<rtimvBase::colorbarMax;i++) ui.colorbarCombo->insertItem(i,"");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarGrey, "Grey");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarJet, "Jet");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarHot, "Hot");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarBone, "Bone");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarRed, "Red");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarGreen, "Green");
   ui.colorbarCombo->insertItem(rtimvBase::colorbarBlue, "Blue");
   ui.colorbarCombo->setMaxCount(rtimvBase::colorbarMax);
   
   ui.pointerViewModecomboBox->insertItem(PointerViewEnabled, "Enabled");
   ui.pointerViewModecomboBox->insertItem(PointerViewOnPress, "On mouse press");
   ui.pointerViewModecomboBox->insertItem(PointerViewDisabled, "Disabled");
   ui.pointerViewModecomboBox->setCurrentIndex(PointerViewOnPress);


}

void rtimvControlPanel::init_panel()
{
   IgnoreZoomSliderChange = true;
   update_ZoomSlider();
   IgnoreZoomSliderChange = false;
   update_ZoomEntry();
   
   update_xycenEntry();
   update_whEntry();
   
//    if(imv->get_abs_fixed())
//    {
//       ui.absfixedButton->setEnabled(false);
//       ui.relfixedButton->setEnabled(true);
//    }
//    else
//    {
//       ui.absfixedButton->setEnabled(true);
//       ui.relfixedButton->setEnabled(false);
//    }
   
   IgnoremindatSliderChange = true;
   update_mindatSlider();
   IgnoremindatSliderChange = false;
   update_mindatEntry();
   update_mindatEntry();
   
   IgnoremaxdatSliderChange = true;
   update_maxdatSlider();
   IgnoremaxdatSliderChange = false;
   update_maxdatEntry();
   update_maxdatEntry();
   
   IgnorebiasSliderChange = true;
   update_biasSlider();
   IgnorebiasSliderChange = false;
   update_biasEntry();
   update_biasRelEntry();
   
   IgnorecontrastSliderChange = true;
   update_contrastSlider();
   IgnorecontrastSliderChange = false;
   update_contrastEntry();
   update_contrastRelEntry();

   ui.scaleTypeCombo->setCurrentIndex(imv->get_cbStretch());
   ui.scaleModeCombo->setCurrentIndex(imv->get_colorbar_mode());
   ui.colorbarCombo->setCurrentIndex(imv->get_current_colorbar());

   //initialize the button state for coordinate displays
   showToolTipCoordsChanged(imv->showToolTipCoords());
   showStaticCoordsChanged(imv->showStaticCoords());

   targetXcChanged(imv->targetXc());
   targetYcChanged(imv->targetYc());
   targetVisibleChanged(imv->targetVisible());

   ui.imtimerspinBox->setValue(imv->imageTimeout());

}

void rtimvControlPanel::update_panel()
{
   IgnoreZoomSliderChange = true;
   update_ZoomSlider();
   IgnoreZoomSliderChange = false;
   update_ZoomEntry();
   
   update_xycenEntry();
   update_whEntry();
   
   //if(imv->get_abs_fixed())
   //{
   update_mindatRelEntry();
   update_maxdatRelEntry();
   update_biasRelEntry();
   update_contrastRelEntry();
   //}
   //else
   //{
   update_mindatEntry();
   update_maxdatEntry();
   update_biasEntry();
   update_contrastEntry();
   //}
   IgnoremindatSliderChange = true;
   update_mindatSlider();
   IgnoremindatSliderChange = false;
   
   IgnoremaxdatSliderChange = true;
   update_maxdatSlider();
   IgnoremaxdatSliderChange = false;
   
   IgnorebiasSliderChange = true;
   update_biasSlider();
   IgnorebiasSliderChange = false;
   
   IgnorecontrastSliderChange = true;
   update_contrastSlider();
   IgnorecontrastSliderChange = false;

   ui.scaleTypeCombo->setCurrentIndex(imv->get_cbStretch());
   ui.scaleModeCombo->setCurrentIndex(imv->get_colorbar_mode());
   ui.colorbarCombo->setCurrentIndex(imv->get_current_colorbar());
}

void rtimvControlPanel::update_ZoomSlider()
{
   ui.ZoomSlider->setSliderPosition((int)((imv->zoomLevel()-imv->zoomLevelMin())/(imv->zoomLevelMax()-imv->zoomLevelMin()) * (ui.ZoomSlider->maximum()-ui.ZoomSlider->minimum())+.5));
}

void rtimvControlPanel::update_ZoomEntry()
{
   char newz[10];
   sprintf(newz, "%4.2f", imv->zoomLevel());
   ui.ZoomEntry->setText(newz);
}

void rtimvControlPanel::on_ZoomSlider_valueChanged(int value)
{
   double zl;
   if(!IgnoreZoomSliderChange)
   {
      zl = imv->zoomLevelMin() + ((double)value/((double) ui.ZoomSlider->maximum() - (double)ui.ZoomSlider->minimum()))*(imv->zoomLevelMax()-imv->zoomLevelMin());
      imv->zoomLevel(zl);
   }
   update_ZoomEntry();
}

void rtimvControlPanel::on_Zoom1_clicked()
{
   imv->zoomLevel(1.0);
   IgnoreZoomSliderChange = true;
   update_ZoomSlider();
   IgnoreZoomSliderChange = false;
}

void rtimvControlPanel::on_Zoom2_clicked()
{
	imv->zoomLevel(2.0);
	IgnoreZoomSliderChange = true;
	update_ZoomSlider();
	IgnoreZoomSliderChange = false;
}

void rtimvControlPanel::on_Zoom4_clicked()
{
	imv->zoomLevel(4.0);
	IgnoreZoomSliderChange = true;
	update_ZoomSlider();
	IgnoreZoomSliderChange = false;
}

void rtimvControlPanel::on_Zoom8_clicked()
{
	imv->zoomLevel(8.0);
	IgnoreZoomSliderChange = true;
	update_ZoomSlider();
	IgnoreZoomSliderChange = false;
}

void rtimvControlPanel::on_Zoom16_clicked()
{
	imv->zoomLevel(16.0);
	IgnoreZoomSliderChange = true;
	update_ZoomSlider();
	IgnoreZoomSliderChange = false;
}

void rtimvControlPanel::on_ZoomEntry_editingFinished()
{
	imv->zoomLevel(ui.ZoomEntry->text().toDouble());
	IgnoreZoomSliderChange = true;
	update_ZoomSlider();
	IgnoreZoomSliderChange = false;
}

void rtimvControlPanel::on_overZoom1_clicked()
{
   imv->setPointerOverZoom(1.0);
}


void rtimvControlPanel::on_overZoom2_clicked()
{
   imv->setPointerOverZoom(2.0);
}
      
void rtimvControlPanel::on_overZoom4_clicked()
{
   imv->setPointerOverZoom(4.0);
}
      
void rtimvControlPanel::set_ViewViewMode(int vvm)
{
   if(vvm < 0 || vvm >= ViewViewModeMax)
   {
      ViewViewMode = ViewViewEnabled;
   }
   else ViewViewMode = vvm;
   
   if(ViewViewMode == ViewViewEnabled)
   {
      qpmi_view = qgs_view->addPixmap(*(imv->getPixmap()));
      viewBox->setEdgeTol(5.*imv->nx()/qgs_view->width());
      
      qpmi_view->stackBefore(viewLineVert);
   }
   if(ViewViewMode == ViewViewNoImage && qpmi_view)
   {
      qgs_view->removeItem(qpmi_view);
      delete qpmi_view;
      qpmi_view = 0;
   }
}

void:: rtimvControlPanel::update_xycenEntry()
{
   char tmps[10];
   
   if(!ui.xcenEntry->hasFocus())
   {
      snprintf(tmps,10,"%.1f", imv->get_xcen());//*imv->nx());
      ui.xcenEntry->setText(tmps);
   }
   if(!ui.ycenEntry->hasFocus())
   {
      snprintf(tmps,10,"%.1f", imv->get_ycen());//*imv->ny());
      ui.ycenEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_whEntry()
{
   char tmps[10];
   if(!ui.widthEntry->hasFocus())
   {
      snprintf(tmps,10,"%.1f", ((double)imv->nx())/imv->zoomLevel());
      ui.widthEntry->setText(tmps);
   }
   
   if(!ui.heightEntry->hasFocus())
   {
      snprintf(tmps,10,"%.1f", ((double)imv->ny())/imv->zoomLevel());
      ui.heightEntry->setText(tmps);
   }
}

void rtimvControlPanel::on_ViewViewModecheckBox_stateChanged(int st)
{
	enableViewViewMode(st);
}

void rtimvControlPanel::enableViewViewMode(int state)
{
	if(state) set_ViewViewMode(ViewViewEnabled);
	else set_ViewViewMode(ViewViewNoImage);
}

void rtimvControlPanel::on_xcenEntry_editingFinished()
{
	imv->set_viewcen(ui.xcenEntry->text().toDouble()/imv->nx(), imv->get_ycen());;
}

void rtimvControlPanel::on_ycenEntry_editingFinished()
{
	imv->set_viewcen(imv->get_xcen(), ui.ycenEntry->text().toDouble()/imv->ny());
}

void rtimvControlPanel::on_widthEntry_editingFinished()
{
	double newzoom;
	newzoom = ((double)imv->nx())/ui.widthEntry->text().toDouble();
	imv->zoomLevel(newzoom);
	update_panel();
}
	
void rtimvControlPanel::on_heightEntry_editingFinished()
{
	double newzoom;
	newzoom = ((double)imv->ny())/ui.heightEntry->text().toDouble();
	imv->zoomLevel(newzoom);
	update_panel();
}

void rtimvControlPanel::on_view_center_clicked()
{
	imv->set_viewcen(.5, .5);
}

void rtimvControlPanel::on_view_ul_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx() - 1./imv->zoomLevel(), imv->get_ycen()/imv->ny()-1./imv->zoomLevel());
}

void rtimvControlPanel::on_view_up_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx(), imv->get_ycen()/imv->ny()-1./imv->zoomLevel());
}

void rtimvControlPanel::on_view_ur_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx() + 1./imv->zoomLevel(), imv->get_ycen()/imv->ny() - 1./imv->zoomLevel());
}

void rtimvControlPanel::on_view_right_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx() + 1./imv->zoomLevel(), imv->get_ycen()/imv->ny());
}

void rtimvControlPanel::on_view_dr_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx() + 1./imv->zoomLevel(), imv->get_ycen()/imv->ny()+1./imv->zoomLevel());
}

void rtimvControlPanel::on_view_down_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx(), imv->get_ycen()/imv->ny()+1./imv->zoomLevel());
}

void rtimvControlPanel::on_view_dl_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx() - 1./imv->zoomLevel(), imv->get_ycen()/imv->ny()+1./imv->zoomLevel());
}

void rtimvControlPanel::on_view_left_clicked()
{
	imv->set_viewcen(imv->get_xcen()/imv->nx() - 1./imv->zoomLevel(), imv->get_ycen()/imv->ny());
}

void rtimvControlPanel::updateMouseCoords(double x, double y, double v)
{
	if(!PointerViewFixed)
	{
		char tmpr[15];
		sprintf(tmpr, "%8.2f", x);
		ui.TextCoordX->setText(tmpr);
	
		sprintf(tmpr, "%8.2f", y);
		ui.TextCoordY->setText(tmpr);
		
		sprintf(tmpr, "%8.2f", v);
		ui.TextPixelVal->setText(tmpr);
	
		if(PointerViewEmpty && PointerViewMode == PointerViewEnabled ) 
		{
			ui.pointerView->setScene(imv->get_qgs());
			PointerViewEmpty = false;
		}
	 	ui.pointerView->centerOn(x, y);
	
	}
}	

void rtimvControlPanel::nullMouseCoords()
{
   if(!PointerViewFixed)
   {
      ui.pointerView->setScene(qgs_empty);
      PointerViewEmpty = true;
      ui.TextCoordX->setText("");
      ui.TextCoordY->setText("");
      ui.TextPixelVal->setText("");
   }
}

		
void rtimvControlPanel::viewLeftPressed(QPointF mp)
{
   static_cast<void>(mp);

   if(PointerViewMode == PointerViewOnPress)
   {
      ui.pointerView->setScene(imv->get_qgs());
      PointerViewEmpty = false;
      if(PointerViewFixed)
      {
         ui.pointerView->centerOn(pointerViewCen.x(), pointerViewCen.y());
      }
   }
}

void rtimvControlPanel::viewLeftClicked(QPointF mp)
{
   if(PointerViewMode == PointerViewOnPress)
   {
      ui.pointerView->setScene(qgs_empty);
      PointerViewEmpty = true;
   }
   if(PointerViewWaiting)
   {
      set_pointerViewCen(mp);
   }
}

void rtimvControlPanel::set_PointerViewMode(int pvm)
{
   if(pvm < 0 || pvm >= PointerViewModeMax || pvm == PointerViewEnabled)
   {
      PointerViewMode = PointerViewEnabled;
      ui.pointerView->setScene(imv->get_qgs());
      PointerViewEmpty = false;
   }
   if(pvm == PointerViewOnPress)
   {
      PointerViewMode = PointerViewOnPress;
      ui.pointerView->setScene(qgs_empty);
      PointerViewEmpty = true;
   }
   if(pvm == PointerViewDisabled)
   {
      PointerViewMode = PointerViewDisabled;
      ui.pointerView->setScene(qgs_empty);
      PointerViewEmpty = true;
   }
}

void rtimvControlPanel::on_pointerViewModecomboBox_activated(int pvm)
{
   set_PointerViewMode(pvm);
}

void rtimvControlPanel::on_pointerSetLocButton_clicked()
{
   if(!PointerViewFixed)
   {
      PointerViewWaiting = true;
      ui.pointerSetLocButton->setText("Waiting");
   }
   else
   {
      ui.pointerSetLocButton->setText("Set Location");
      PointerViewWaiting = false;
      PointerViewFixed = false;
   }
}

void rtimvControlPanel::set_pointerViewCen(QPointF mp)
{
   std::cerr << mp.x() << " " << mp.y() << "\n";

   pointerViewCen = mp;
   ui.pointerSetLocButton->setText("Clear Location");
   PointerViewWaiting = false;
   PointerViewFixed = true;
}

void rtimvControlPanel::on_scaleTypeCombo_activated(int ct)
{
   imv->set_cbStretch(ct);
   
   
   imv->changeImdata();
}

void rtimvControlPanel::on_colorbarCombo_activated(int cb)
{
   imv->load_colorbar(cb);
}

void rtimvControlPanel::update_mindatSlider()
{
   int pos;
   
   pos = (int)(ui.mindatSlider->minimum() + (imv->mindat()-imv->get_imdat_min())/(imv->get_imdat_max()-imv->get_imdat_min())*(ui.mindatSlider->maximum()-ui.mindatSlider->minimum())+.5);
   
   IgnoremindatSliderChange = true;
   ui.mindatSlider->setSliderPosition(pos);
   IgnoremindatSliderChange = false;
}

void rtimvControlPanel::update_mindatEntry()
{
   char tmps[15];
   if(!ui.mindatEntry->hasFocus())
   {
      sprintf(tmps, "%14.1f", imv->mindat());
      ui.mindatEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_mindatRelEntry()
{
   char tmps[15];
   if(!ui.mindatRelEntry->hasFocus())
   {
      sprintf(tmps, "%5.3f", (imv->mindat()-imv->get_imdat_min())/(imv->get_imdat_max()-imv->get_imdat_min()));
      ui.mindatRelEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_maxdatSlider()
{
   int pos;
   pos = (int)(ui.maxdatSlider->minimum() + (imv->maxdat()-imv->get_imdat_min())/(imv->get_imdat_max()-imv->get_imdat_min())*(ui.maxdatSlider->maximum()-ui.maxdatSlider->minimum())+.5);
   
   IgnoremaxdatSliderChange = true;
   ui.maxdatSlider->setSliderPosition(pos);
   IgnoremaxdatSliderChange = false;
}

////

void rtimvControlPanel::update_maxdatEntry()
{
   char tmps[15];
   if(!ui.maxdatEntry->hasFocus())
   {
      sprintf(tmps, "%14.1f", imv->maxdat());
      ui.maxdatEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_maxdatRelEntry()
{
   char tmps[15];
   if(!ui.maxdatRelEntry->hasFocus())
   {
      sprintf(tmps, "%5.3f", (imv->maxdat()-imv->get_imdat_min())/(imv->get_imdat_max()-imv->get_imdat_min()));
      ui.maxdatRelEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_biasSlider()
{
   int pos;
   pos = (int)(ui.biasSlider->minimum() + (.5*(imv->maxdat()+imv->mindat())-imv->get_imdat_min())/(imv->get_imdat_max()-imv->get_imdat_min())*(ui.biasSlider->maximum()-ui.biasSlider->minimum())+.5);
   
   IgnorebiasSliderChange = true;
   ui.biasSlider->setSliderPosition(pos);
   IgnorebiasSliderChange = false;
}

void rtimvControlPanel::update_biasEntry()
{
   char tmps[15];
   if(!ui.biasEntry->hasFocus())
   {
      sprintf(tmps, "%14.1f", 0.5*(imv->maxdat()+imv->mindat()));
      ui.biasEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_biasRelEntry()
{
   char tmps[15];
   if(!ui.biasRelEntry->hasFocus())
   {
      sprintf(tmps, "%5.3f", (0.5*(imv->maxdat()+imv->mindat())-imv->get_imdat_min())/(imv->get_imdat_max()-imv->get_imdat_min()));
      ui.biasRelEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_contrastSlider()
{
   int pos;
   pos = (int)(ui.contrastSlider->minimum() + (imv->maxdat()-imv->mindat())/(imv->get_imdat_max()-imv->get_imdat_min())*(ui.biasSlider->maximum()-ui.biasSlider->minimum())+.5);
   
   IgnorecontrastSliderChange = true;
   ui.contrastSlider->setSliderPosition(pos);
   IgnorecontrastSliderChange = false;
}

void rtimvControlPanel::update_contrastEntry()
{
   char tmps[15];
   if(!ui.contrastEntry->hasFocus())
   {
      sprintf(tmps, "%14.1f", (imv->maxdat()-imv->mindat()));
      ui.contrastEntry->setText(tmps);
   }
}

void rtimvControlPanel::update_contrastRelEntry()
{
   char tmps[15];
   if(!ui.contrastRelEntry->hasFocus())
   {
      sprintf(tmps, "%5.1f", imv->get_imdat_max()-imv->get_imdat_min()/(imv->maxdat()-imv->mindat()));
      ui.contrastRelEntry->setText(tmps);
   }
}

void rtimvControlPanel::on_scaleModeCombo_activated(int index)
{

   if(index == rtimvBase::minmaxglobal)
   {
      imv->setUserBoxActive(false);
      imv->set_colorbar_mode(rtimvBase::minmaxglobal);
      imv->maxdat(imv->get_imdat_max());
      imv->mindat(imv->get_imdat_min());
      imv->changeImdata();
      update_mindatEntry();
      update_maxdatEntry();
      update_biasEntry();
      update_contrastEntry();
      update_mindatRelEntry();
      update_maxdatRelEntry();
      update_biasRelEntry();
      update_contrastRelEntry();
      
   }

   if(index == rtimvBase::user)
   {
      imv->setUserBoxActive(false);
      imv->set_colorbar_mode(rtimvBase::user);
   }
   
   if(index == rtimvBase::minmaxbox)
   {
      imv->m_colorBox->setVisible(true);
      imv->setUserBoxActive(true);
      update_mindatEntry();
      update_maxdatEntry();
      update_biasEntry();
      update_contrastEntry();
      update_mindatRelEntry();
      update_maxdatRelEntry();
      update_biasRelEntry();
      update_contrastRelEntry();
   }
   else
   {
      imv->m_colorBox->setVisible(false);
      imv->setUserBoxActive(false);
   }
}

void rtimvControlPanel::on_mindatSlider_valueChanged(int value)
{
   if(!IgnoremindatSliderChange)
   {
      double sc = ((double)(value - ui.mindatSlider->minimum()))/((double)(ui.mindatSlider->maximum()-ui.mindatSlider->minimum()));
      imv->mindat(imv->get_imdat_min() + (imv->get_imdat_max()-imv->get_imdat_min())*sc);
      imv->changeImdata();
      imv->setUserBoxActive(false);
      imv->set_colorbar_mode(rtimvBase::user);
      ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);

      update_mindatEntry();
      update_biasEntry();
      update_contrastEntry();
   }
}

void rtimvControlPanel::on_mindatEntry_editingFinished()
{
   imv->mindat(ui.mindatEntry->text().toDouble());
   update_mindatEntry();
   update_biasEntry();
   update_contrastEntry();
   update_mindatSlider();
   update_biasSlider();
   update_contrastSlider();
   imv->changeImdata();
   imv->setUserBoxActive(false);
   imv->set_colorbar_mode(rtimvBase::user);
   ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);
}

void rtimvControlPanel::on_maxdatSlider_valueChanged(int value)
{
   if(!IgnoremaxdatSliderChange)
   {
      double sc = ((double)(value - ui.maxdatSlider->minimum()))/((double)(ui.maxdatSlider->maximum()-ui.maxdatSlider->minimum()));
      imv->maxdat(imv->get_imdat_min() + (imv->get_imdat_max()-imv->get_imdat_min())*sc);
      imv->changeImdata();
      imv->setUserBoxActive(false);
      imv->set_colorbar_mode(rtimvBase::user);
      ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);

      update_maxdatEntry();
      update_biasEntry();
      update_contrastEntry();
   }
   
}

void rtimvControlPanel::on_maxdatEntry_editingFinished()
{
   imv->maxdat(ui.maxdatEntry->text().toDouble());
   update_maxdatEntry();
   update_biasEntry();
   update_maxdatSlider();
   update_biasSlider();
   imv->changeImdata();
   imv->setUserBoxActive(false);
   imv->set_colorbar_mode(rtimvBase::user);
   ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);
}

void rtimvControlPanel::on_biasSlider_valueChanged(int value)
{
   if(!IgnorebiasSliderChange)
   {
      double bias = ((double)(value - ui.biasSlider->minimum()))/((double)(ui.biasSlider->maximum()-ui.biasSlider->minimum()));
      imv->bias_rel(bias);
      imv->changeImdata();
      imv->setUserBoxActive(false);
      imv->set_colorbar_mode(rtimvBase::user);
      ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);

      update_mindatEntry();
      update_maxdatEntry();
      update_biasEntry();
      update_contrastEntry();
   }
}

void rtimvControlPanel::on_biasEntry_editingFinished()
{
   imv->bias(ui.biasEntry->text().toDouble());
   imv->setUserBoxActive(false);
   imv->set_colorbar_mode(rtimvBase::user);
   ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);
   imv->changeImdata();
   
   update_mindatEntry();
   update_maxdatEntry();
   update_biasEntry();
   update_contrastEntry();
   
}

void rtimvControlPanel::on_contrastSlider_valueChanged(int value)
{
   if(!IgnorecontrastSliderChange)
   {
      double cont = ((double)(value - ui.contrastSlider->minimum()))/((double)(ui.contrastSlider->maximum()-ui.contrastSlider->minimum()));
      cont = cont * (imv->get_imdat_max() - imv->get_imdat_min());
      imv->contrast(cont);
      imv->changeImdata();
      imv->setUserBoxActive(false);
      imv->set_colorbar_mode(rtimvBase::user);
      ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);

      update_mindatEntry();
      update_maxdatEntry();
      update_biasEntry();
      update_contrastEntry();
   }
}


void rtimvControlPanel::on_contrastEntry_editingFinished()
{
   imv->contrast(ui.contrastEntry->text().toDouble());
   imv->setUserBoxActive(false);
   imv->set_colorbar_mode(rtimvBase::user);
   ui.scaleModeCombo->setCurrentIndex(SCALEMODE_USER);
   imv->changeImdata();
   
   update_mindatEntry();
   update_maxdatEntry();
   update_biasEntry();
   update_contrastEntry();
   
}

void rtimvControlPanel::on_imtimerspinBox_valueChanged(int to)
{
   imv->imageTimeout(to);
}

void rtimvControlPanel::on_statsBoxButton_clicked()
{
   if(statsBoxButtonState)
   {
      emit hideStatsBox();
      ui.statsBoxButton->setText("Show Stats Box");
      statsBoxButtonState = false;
   }
   else
   {
      emit launchStatsBox();
      ui.statsBoxButton->setText("Hide Stats Box");
      statsBoxButtonState = true;
   }
   
   
}

void rtimvControlPanel::viewBoxMoved ( const QRectF & vbr)
{
   //QPointF newcenV = ui.viewView->mapFromScene(newcen);
   //double viewZoom = (double)ui.viewView->width()/(double)imv->nx();
   //QRectF vbr = viewBox->rect();
   
   QPointF np = qpmi_view->mapFromItem(viewBox, QPointF(vbr.x(),vbr.y()));
   QPointF np2 = qpmi_view->mapFromItem(viewBox, QPointF(vbr.bottom(),vbr.right()));

   
   //std::cout <<  np.x() << " " << np.y() << " " << (np2.x()-np.x()) << " " << (np2.y()-np.y()) << "\n\n";
   
   
   
   double nxcen = np.x() + .5*(np2.x()-np.x());
   double nycen = np.y() + .5*(np2.y()-np.y());
   //std::cout << nxcen/1024. << " " << nycen/1024. << " " << ui.viewView->width() << " " << ui.viewView->height() << "\n";
   
   imv->set_viewcen(nxcen/(double)imv->nx(), nycen/(double)imv->nx(), true);
   

   
}

void rtimvControlPanel::showToolTipCoordsChanged(bool sttc)
{
   if(sttc)
   {
      ui.toolTipCoordsButton->setText("Hide Pointer Coords");
   }
   else
   {
      ui.toolTipCoordsButton->setText("Show Pointer Coords");
   }
}

void rtimvControlPanel::showStaticCoordsChanged(bool ssc)
{
   if(ssc)
   {
      ui.staticCoordsButton->setText("Hide Static Coords");
   }
   else
   {
      ui.staticCoordsButton->setText("Show Static Coords");
   }
}

void rtimvControlPanel::on_toolTipCoordsButton_clicked()
{
   //Toggle
   emit showToolTipCoords(!imv->showToolTipCoords());
}

void rtimvControlPanel::on_staticCoordsButton_clicked()
{
   //Toggle
   emit showStaticCoords(!imv->showStaticCoords());
}

void rtimvControlPanel::targetXcChanged( float txc )
{
   char str[16];
   snprintf(str, sizeof(str), "%0.3f", txc);
   ui.lineEditTargetFractionX->setText(str);

   snprintf(str, sizeof(str), "%0.2f", txc*(imv->nx()) -0.5 );
   ui.lineEditTargetPixelX->setText(str);
}

void rtimvControlPanel::targetYcChanged( float tyc )
{
   char str[16];
   snprintf(str, sizeof(str), "%0.3f", tyc);
   ui.lineEditTargetFractionY->setText(str);

   snprintf(str, sizeof(str), "%0.2f", tyc*(imv->ny()) -0.5 );
   ui.lineEditTargetPixelY->setText(str);
}

void rtimvControlPanel::targetVisibleChanged( bool tv )
{
   if(tv)
   {
      ui.buttonTargetCross->setText("hide");
   }
   else
   {
      ui.buttonTargetCross->setText("show");
   }
}

void rtimvControlPanel::on_buttonTargetCross_clicked()
{
   //toggle
   emit targetVisible(!imv->targetVisible());
}

void rtimvControlPanel::on_lineEditTargetPixelX_returnPressed()
{
   float txc = -1;
   bool ok = false;
   try
   {
      txc = ui.lineEditTargetPixelX->text().toFloat(&ok);
   }
   catch(...)
   {
      return;
   }
      
   if(txc == -1 || ok == false) return;

   txc = (txc + 0.5)/imv->nx();

   emit targetXc(txc);
}

void rtimvControlPanel::on_lineEditTargetPixelY_returnPressed()
{
   float tyc = -1;
   bool ok = false;
   try
   {
      tyc = ui.lineEditTargetPixelY->text().toFloat(&ok);
   }
   catch(...)
   {
      return;
   }
      
   if(tyc == -1 || ok == false) return;

   tyc = (tyc + 0.5)/imv->ny();

   emit targetYc(tyc);
}

void rtimvControlPanel::on_lineEditTargetFractionX_returnPressed()
{
   float txc = -1;
   bool ok = false;
   try
   {
      txc = ui.lineEditTargetFractionX->text().toFloat(&ok);
   }
   catch(...)
   {
      return;
   }
      
   if(txc == -1 || ok == false) return;

   emit targetXc(txc);
}


void rtimvControlPanel::on_lineEditTargetFractionY_returnPressed()
{
   float tyc = -1;
   bool ok = false;
   try
   {
      tyc = ui.lineEditTargetFractionY->text().toFloat(&ok);
   }
   catch(...)
   {
      return;
   }
      
   if(tyc == -1 || ok == false) return;

   emit targetYc(tyc);
}
