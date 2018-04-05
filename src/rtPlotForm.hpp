/** \file rtPlotForm.hpp
  * \author Jared R. Males (jaredmales@gmail.com)
  * \brief Declaration of a real-time plotting class
  *
  */

#ifndef rtimv_rtPlotForm_hpp
#define rtimv_rtPlotForm_hpp

#include <QDialog>
#include <QTimer>

#include "ui_rtplot.h"

#include <qwt/qwt_plot_curve.h>
#include <iostream>

#include "circleTimeSeries.hpp"

   
class rtPlotForm : public QDialog
{
   Q_OBJECT
   
   public:
      rtPlotForm(QString title, QString ylabel, size_t sz, bool useage, int pause, QWidget * Parent = 0, Qt::WindowFlags f = 0);
      ~rtPlotForm();
      
      int statsPause;

      QTimer updateTimer; ///< When this times out the GUI is updated.
      int updateTimerTimeout;
      

      QwtPlotCurve * curve;
      
      circleTimeSeries *data;

      void reset();
      
      bool autoY;
      bool plotY0;
      double plotMinY;
      double plotMaxY;
      
   protected slots:
      void updatePlot();
      void on_clearButton_pressed();
      void on_replotButton_pressed();
      
   private:
      Ui::rtPlotForm ui;
   
};

#endif //rtimv_rtPlotForm_hpp
