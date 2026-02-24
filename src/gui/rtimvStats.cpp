/** \file rtimvStats.cpp
 * \brief Definitions for the rtimvStats display dialog.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */
#include "rtimvStats.hpp"

#include <cmath>

rtimvStats::rtimvStats( RTIMV_BASE *imv, QWidget *Parent, Qt::WindowFlags f ) : QDialog( Parent, f )
{
    ui.setupUi( this );

    m_imv = imv;
    m_updateTimerTimeout = 50;

    connect( &m_updateTimer, SIGNAL( timeout() ), this, SLOT( updateGUI() ) );
    m_updateTimer.start( m_updateTimerTimeout );
}

rtimvStats::~rtimvStats()
{
}

void rtimvStats::updateGUI()
{
    if( m_imv == nullptr )
    {
        return;
    }

    char txt[50];

    float dataMin = m_imv->statsBox_min();
    float dataMax = m_imv->statsBox_max();
    float dataMean = m_imv->statsBox_mean();
    float dataMedian = m_imv->statsBox_median();

    if( std::fabs( dataMin ) < 1e-1 )
    {
        snprintf( txt, sizeof( txt ), "%0.04g", dataMin );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "%0.02f", dataMin );
    }
    ui.dataMin->setText( txt );

    if( std::fabs( dataMax ) < 1e-1 )
    {
        snprintf( txt, sizeof( txt ), "%0.04g", dataMax );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "%0.02f", dataMax );
    }
    ui.dataMax->setText( txt );

    if( std::fabs( dataMean ) < 1e-1 )
    {
        snprintf( txt, sizeof( txt ), "%0.04g", dataMean );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "%0.02f", dataMean );
    }
    ui.dataMean->setText( txt );

    if( std::fabs( dataMedian ) < 1e-1 )
    {
        snprintf( txt, sizeof( txt ), "%0.04g", dataMedian );
    }
    else
    {
        snprintf( txt, sizeof( txt ), "%0.02f", dataMedian );
    }
    ui.dataMedian->setText( txt );
}
