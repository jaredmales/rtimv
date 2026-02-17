/** \file rtimvStats.hpp
 * \brief Declarations for the rtimvStats display dialog.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */
#ifndef rtimv_rtimvStats_hpp
#define rtimv_rtimvStats_hpp

#include <cstdio>

#include <QDialog>
#include <QTimer>

#include "ui_rtimvStats.h"

//#define RTIMV_BASE_QWIDGET

#include RTIMV_BASE_INCLUDE

/// Class to display statistics for the configured stats box.
/** This class is display-only. Statistical calculations and region state are
 * maintained in RTIMV_BASE and queried here for presentation.
 */
class rtimvStats : public QDialog
{
    Q_OBJECT

  public:
    /// Constructor
    rtimvStats( RTIMV_BASE *imv,              ///< [in] The rtimv instance this is connected to
                QWidget *Parent = nullptr,            ///< [in] [optional] Qt parent widget
                Qt::WindowFlags f = Qt::WindowFlags() ///< [in] [optional] Qt flags for this widget
    );

    /// Destructor
    ~rtimvStats();

  protected:
    RTIMV_BASE *m_imv{ nullptr }; ///< The rtimv instance this is connected to

    QTimer m_updateTimer;           ///< When this times out the GUI is updated if needed.
    int m_updateTimerTimeout{ 50 }; ///< The GUI update timeout, milliseconds.

  protected slots:

    /// Update the GUI.
    /** Called when m_updateTimer times out.
     */
    void updateGUI();

  private:
    Ui::statsform ui;
};

#endif // rtimv_rtimvStats_hpp
