#include "iostream"

#include "cubeCtrl.hpp"

cubeCtrl::cubeCtrl( bool mode,
                    float fps,
                    float desiredFPS,
                    float fpsMult,
                    int dir,
                    uint32_t frames,
                    uint32_t frame,
                    bool as,
                    QWidget *Parent,
                    Qt::WindowFlags f )
    : QWidget( Parent, f )
{
    ui.setupUi( this );

    cubeMode( mode );
    cubeFPS( fps, desiredFPS );
    cubeFPSMult( fpsMult );
    cubeDir( dir );
    cubeFrames( frames );
    cubeFrame( frame );
    autoScale( as );
}

void cubeCtrl::cubeMode( bool mode )
{
    m_cubeMode = mode;
    updateButtons();
}

void cubeCtrl::cubeFPS( float fps, float desiredFPS )
{
    m_desiredCubeFPS = desiredFPS;
    m_cubeFPS = fps;
    ui.lineEditFPS->setText( QString( "%1" ).arg( m_desiredCubeFPS, 0, 'f', 2 ) );

    ui.labelFPS->setText( QString( "FPS (%1 FPS)" ).arg( m_cubeFPS * m_cubeFPSMult, 0, 'f', 2 ) );
}

void cubeCtrl::cubeFPSMult( float fm )
{
    m_cubeFPSMult = fm;
    ui.labelFPS->setText( QString( "FPS (%1 FPS)" ).arg( m_cubeFPS * m_cubeFPSMult, 0, 'f', 2 ) );
    updateButtons();
}

void cubeCtrl::cubeDir( int d )
{
    m_cubeDir = d;
    updateButtons();
}

void cubeCtrl::autoScale( bool as )
{
    if( as )
    {
        ui.checkBoxAutoScale->setChecked( true );
    }
    else
    {
        ui.checkBoxAutoScale->setChecked( false );
    }
}

void cubeCtrl::on_lineEditFPS_editingFinished()
{
    bool ok;
    float fps = ui.lineEditFPS->text().toFloat( &ok );

    if( ok )
    {
        emit cubeFPSUpdated( fps );
    }
}

void cubeCtrl::on_buttonFPSD10_pressed()
{
    emit cubeFPSUpdated( m_desiredCubeFPS / 10.0 );
}

void cubeCtrl::on_buttonFPSD2_pressed()
{
    emit cubeFPSUpdated( m_desiredCubeFPS / 2.0 );
}

void cubeCtrl::on_buttonFPSX2_pressed()
{
    emit cubeFPSUpdated( m_desiredCubeFPS * 2.0 );
}

void cubeCtrl::on_buttonFPSX10_pressed()
{
    emit cubeFPSUpdated( m_desiredCubeFPS * 10.0 );
}

void cubeCtrl::on_buttonFastFastBackward_pressed()
{
    emit cubeFPSMultUpdated( 10.0 );
    emit cubeDirUpdated( -1 );
    emit cubeModeUpdated( 1 );
}

void cubeCtrl::on_buttonFastBackward_pressed()
{
    emit cubeFPSMultUpdated( 2.0 );
    emit cubeDirUpdated( -1 );
    emit cubeModeUpdated( 1 );
}

void cubeCtrl::on_buttonPlayBackward_pressed()
{
    emit cubeFPSMultUpdated( 1.0 );
    emit cubeDirUpdated( -1 );
    emit cubeModeUpdated( 1 );
}

void cubeCtrl::on_buttonStop_pressed()
{
    emit cubeModeUpdated( 0 );
}

void cubeCtrl::on_buttonPlayForward_pressed()
{
    emit cubeFPSMultUpdated( 1.0 );
    emit cubeDirUpdated( 1 );
    emit cubeModeUpdated( 1 );
}

void cubeCtrl::on_buttonFastForward_pressed()
{
    emit cubeFPSMultUpdated( 2.0 );
    emit cubeDirUpdated( 1 );
    emit cubeModeUpdated( 1 );
}

void cubeCtrl::on_buttonFastFastForward_pressed()
{
    emit cubeFPSMultUpdated( 10.0 );
    emit cubeDirUpdated( 1 );
    emit cubeModeUpdated( 1 );
}

void cubeCtrl::on_lineEditFrame_editingFinished()
{
    bool ok;
    uint32_t fno = ui.lineEditFrame->text().toULong( &ok );

    if( ok )
    {
        emit cubeFrameUpdated( fno );
    }
}

void cubeCtrl::on_scrollBarFrame_sliderMoved( int value )
{
    emit cubeFrameUpdated( value );
}

void cubeCtrl::on_scrollBarFrame_sliderReleased()
{
    emit cubeFrameUpdated( ui.scrollBarFrame->value() );
}

void cubeCtrl::on_buttonFrameDecrement2_pressed()
{
    emit cubeFrameDeltaUpdated( -10 );
}

void cubeCtrl::on_buttonFrameDecrement_pressed()
{
    emit cubeFrameDeltaUpdated( -1 );
}

void cubeCtrl::on_buttonFrameIncrement_pressed()
{
    emit cubeFrameDeltaUpdated( 1 );
}

void cubeCtrl::on_buttonFrameIncrement2_pressed()
{
    emit cubeFrameDeltaUpdated( 10 );
}

void cubeCtrl::on_checkBoxAutoScale_stateChanged( int state )
{
    if( state > 0 )
    {
        emit autoScaleUpdated( true );
    }
    else
    {
        emit autoScaleUpdated( false );
    }
}

void cubeCtrl::updateButtons()
{
    if( m_cubeMode == 0 )
    {
        ui.buttonFastFastBackward->setEnabled( true );
        ui.buttonFastBackward->setEnabled( true );
        ui.buttonPlayBackward->setEnabled( true );
        ui.buttonStop->setEnabled( false );
        ui.buttonPlayForward->setEnabled( true );
        ui.buttonFastForward->setEnabled( true );
        ui.buttonFastFastForward->setEnabled( true );
    }
    else if( m_cubeDir == -1 )
    {
        if( m_cubeFPSMult == 10.0 )
        {
            ui.buttonFastFastBackward->setEnabled( false );
            ui.buttonFastBackward->setEnabled( true );
            ui.buttonPlayBackward->setEnabled( true );
        }
        else if( m_cubeFPSMult == 2.0 )
        {
            ui.buttonFastFastBackward->setEnabled( true );
            ui.buttonFastBackward->setEnabled( false );
            ui.buttonPlayBackward->setEnabled( true );
        }
        else if( m_cubeFPSMult == 1.0 )
        {
            ui.buttonFastFastBackward->setEnabled( true );
            ui.buttonFastBackward->setEnabled( true );
            ui.buttonPlayBackward->setEnabled( false );
        }
        else
        {
            ui.buttonFastFastBackward->setEnabled( true );
            ui.buttonFastBackward->setEnabled( true );
            ui.buttonPlayBackward->setEnabled( true );
        }
        ui.buttonStop->setEnabled( true );
        ui.buttonPlayForward->setEnabled( true );
        ui.buttonFastForward->setEnabled( true );
        ui.buttonFastFastForward->setEnabled( true );
    }
    else
    {
        ui.buttonFastFastBackward->setEnabled( true );
        ui.buttonFastBackward->setEnabled( true );
        ui.buttonPlayBackward->setEnabled( true );
        ui.buttonStop->setEnabled( true );

        if( m_cubeFPSMult == 1.0 )
        {
            ui.buttonPlayForward->setEnabled( false );
            ui.buttonFastForward->setEnabled( true );
            ui.buttonFastFastForward->setEnabled( true );
        }
        else if( m_cubeFPSMult == 2.0 )
        {
            ui.buttonPlayForward->setEnabled( true );
            ui.buttonFastForward->setEnabled( false );
            ui.buttonFastFastForward->setEnabled( true );
        }
        else if( m_cubeFPSMult == 10.0 )
        {
            ui.buttonPlayForward->setEnabled( true );
            ui.buttonFastForward->setEnabled( true );
            ui.buttonFastFastForward->setEnabled( false );
        }
        else
        {
            ui.buttonPlayForward->setEnabled( true );
            ui.buttonFastForward->setEnabled( true );
            ui.buttonFastFastForward->setEnabled( true );
        }
    }
}

void cubeCtrl::cubeFrames( uint32_t fno )
{
    m_frames = fno;
    ui.labelFrames->setText( QString( "/ %1" ).arg( fno ) );
    ui.scrollBarFrame->setMaximum( fno );
}

void cubeCtrl::cubeFrame( uint32_t fno )
{
    m_frame = fno;

    if( !ui.lineEditFrame->hasFocus() )
    {
        ui.lineEditFrame->setText( QString( "%1" ).arg( fno ) );
    }

    if( !ui.scrollBarFrame->isSliderDown() )
    {
        ui.scrollBarFrame->setSliderPosition( fno );
    }
}
