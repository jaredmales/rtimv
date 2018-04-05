/** \file rtPlotForm.cpp
  * \author Jared R. Males (jaredmales@gmail.com)
  * \brief Definitions for a real-time plotting class
  *
  */

#include "rtPlotForm.hpp"
#include <iostream>

rtPlotForm::rtPlotForm(QString title, QString ylabel, size_t sz, bool useage, int pause, QWidget * Parent, Qt::WindowFlags f) : QDialog(Parent, f)
{
   ui.setupUi(this);
   
   updateTimerTimeout = pause;
   QString wintit = "rtimv Plot: " + title;
   setWindowTitle(wintit);
   
   
   QwtText qtitle(title);
   qtitle.setFont(QFont("Sans Serif", 10));
   ui.qwtPlot->setTitle(qtitle);
   
   QwtText qxlabel("Age (sec)");
   qxlabel.setFont(QFont("Sans Serif", 10));
   ui.qwtPlot->setAxisTitle( QwtPlot::xBottom,qxlabel );
   
   QwtText qylabel(ylabel);
   qylabel.setFont(QFont("Sans Serif", 10));
   ui.qwtPlot->setAxisTitle( QwtPlot::yLeft, qylabel);
   
   ui.qwtPlot->setCanvasBackground(QColor(255,255,255));
   ui.qwtPlot->setAutoReplot(false);
   
   
   curve = new QwtPlotCurve(title);
   data = new circleTimeSeries(sz, useage);

   curve->setData(data);
   curve->attach(ui.qwtPlot);
   
   connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updatePlot()));
   updateTimer.start(updateTimerTimeout);
   
   autoY = true;
   plotY0 = true;
}

rtPlotForm::~rtPlotForm()
{
   //if(data) delete data; //-->Curve deletes data
   
   if(curve) delete curve;
}

void rtPlotForm::reset()
{
   data->clear();
   updatePlot();
}

void rtPlotForm::updatePlot()
{
   //curve->setData(data);
   
   double tmax, ymin, ymax;
   
   if(data->_size > 0)
   {
      ymax = data->maxY;
      ymin = data->minY;
      tmax = data->x(0);
   }
   else
   {
      ymax = 1;
      ymin = 1;
      tmax = 10.;
   }

   double _yaxmin = 0;
   if(!plotY0) _yaxmin = ymin;
   
   double xmax = ((double)ui.xScaleSlider->value()/(double)ui.xScaleSlider->maximum())*tmax;
   double _ymax = _yaxmin + ((double)ui.yMaxSlider->value()/(double)ui.yMaxSlider->maximum())*(ymax - _yaxmin);
   double _ymin = _yaxmin + ((double)ui.yMinSlider->value()/(double)ui.yMinSlider->maximum())*(ymax - _yaxmin);
   
   ui.qwtPlot->setAxisScale(2, 0.,  xmax  ,0);

   ui.qwtPlot->setAxisScale(0, _ymin,  _ymax  ,0);
      
   ui.qwtPlot->replot();
   
}   

void rtPlotForm::on_replotButton_pressed()
{
   updatePlot();
}

void rtPlotForm::on_clearButton_pressed()
{
   reset();
}
