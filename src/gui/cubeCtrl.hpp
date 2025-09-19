
#ifndef __cubeCtrl_hpp__
#define __cubeCtrl_hpp__

#include "ui_cubeCtrl.h"

class cubeCtrl : public QWidget
{
    Q_OBJECT

  protected:
    bool m_cubeMode;
    float m_desiredCubeFPS;
    float m_cubeFPS;
    float m_cubeFPSMult;
    int m_cubeDir;
    uint32_t m_frames;
    uint32_t m_frame;

  private:
    cubeCtrl() = delete;

  public:
    cubeCtrl( bool mode,
              float fps,
              float desiredFPS,
              float fpsMult,
              int dir,
              uint32_t frames,
              uint32_t frame,
              bool autoScale,
              QWidget *Parent = nullptr,            ///< [in] [optional] Qt parent widget
              Qt::WindowFlags f = Qt::WindowFlags() ///< [in] [optional] Qt flags for this widget
    );

    const QPushButton * restretchButton() const
    {
        return ui.buttonRestretch;
    }

  public slots:
    void cubeMode( bool mode );
    void cubeFPS( float fps, float desiredFPS );
    void cubeFPSMult( float fm );
    void cubeDir( int d );
    void autoScale(bool as);

    void on_lineEditFPS_editingFinished();
    void on_lineEditFPS_returnPressed();

    void on_buttonFPSD10_pressed();
    void on_buttonFPSD2_pressed();
    void on_buttonFPSX2_pressed();
    void on_buttonFPSX10_pressed();

    void on_buttonFastFastBackward_pressed();
    void on_buttonFastBackward_pressed();
    void on_buttonPlayBackward_pressed();
    void on_buttonStop_pressed();
    void on_buttonPlayForward_pressed();
    void on_buttonFastForward_pressed();
    void on_buttonFastFastForward_pressed();

    void on_lineEditFrame_editingFinished();
    void on_lineEditFrame_returnPressed();

    void on_scrollBarFrame_sliderMoved(int value);
    void on_scrollBarFrame_sliderReleased();

    void on_buttonFrameDecrement2_pressed();
    void on_buttonFrameDecrement_pressed();
    void on_buttonFrameIncrement_pressed();
    void on_buttonFrameIncrement2_pressed();

    void on_checkBoxAutoScale_stateChanged(int state);

    void cubeFrames(uint32_t);
    void cubeFrame(uint32_t);

    void updateButtons();

  signals:
    void cubeModeUpdated( bool );
    void cubeFPSUpdated( float );
    void cubeFPSMultUpdated( float );
    void cubeDirUpdated( int );
    void cubeFrameUpdated( uint32_t);
    void cubeFrameDeltaUpdated( int32_t);
    void autoScaleUpdated( bool );

  private:
    Ui::cubeCtrl ui;
};

#endif //__cubeCtrl_hpp__
