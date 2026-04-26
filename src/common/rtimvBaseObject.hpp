/** \file rtimvBaseObject.hpp
 * \brief Declarations for the rtimvBaseObject base class
 *
 */

#ifndef rtimv_rtimvBaseObject_hpp
#define rtimv_rtimvBaseObject_hpp

#include <cstdint>

#include <QObject>
#include <QTimer>

// forward
class RTIMV_BASE;

struct rtimvBaseObject : public QObject
{
    Q_OBJECT

  private:
    /// Raw back-pointer to the owning base/client object used by queued Qt slots.
    RTIMV_BASE *m_parent{ nullptr };

  public:
    QTimer m_imageTimer; ///< When this times out rtimvBase checks for a new image.

    QTimer m_cubeTimer; ///< When this times out rtimvBase increments the cube frame number.

    QTimer m_cubeFrameUpdateTimer; ///< When this times out the current frame number signal is sent.

    QTimer m_connectionTimer; ///< When this times out the GRPC client checks for a disconnection

    rtimvBaseObject() = delete;

    /// Construct the Qt helper object that forwards timer and signal activity to the owning base object.
    rtimvBaseObject( RTIMV_BASE *parent, QObject *QParent );

    /// Detach from the owning base object and stop all internal timers/connections for shutdown.
    Q_INVOKABLE void shutdown();

    /// Update the number of images in the cube
    void emit_nzUpdated( uint32_t n /**< [in] the current number of images in the cube */ );

    /// Update the cube mode
    void emit_cubeModeUpdated( bool mode /**< [in] the current cube mode (true is playing back, false is stopped */ );

    /// Update the cube FPS
    void emit_cubeFPSUpdated( float fps, /**< [in] the current actual FPS*/
                              float desiredFPS /**< [in] the desired FPS*/ );

    /// Update the cube FPS multiplier
    void emit_cubeFPSMultUpdated( float fpsMult /**< [in] the current FPS multiplier*/ );

    /// Update the cube direction
    void emit_cubeDirUpdated( int dir /**< [in] the current cube direction (+1 is forward, -1 is backward)*/ );

    /// Update the cube frame number
    void emit_cubeFrameUpdated( uint32_t fno /**< [in] the current cube frame number*/ );

    /// Emit a pixel-value update.
    void emit_pixelValueUpdated( uint32_t x,  /**< [in] x coordinate of the pixel */
                                 uint32_t y,  /**< [in] y coordinate of the pixel */
                                 float value, /**< [in] the calibrated pixel value */
                                 bool valid   /**< [in] true when value is valid */
    );

    /// Emit a color-box min/max update.
    void emit_colorBoxUpdated( int64_t i0, /**< [in] upper-left x coordinate */
                               int64_t i1, /**< [in] lower-right x coordinate */
                               int64_t j0, /**< [in] upper-left y coordinate */
                               int64_t j1, /**< [in] lower-right y coordinate */
                               float min,  /**< [in] minimum calibrated value */
                               float max,  /**< [in] maximum calibrated value */
                               bool valid  /**< [in] true when min/max are valid */
    );

    /// Emit a stats-box update.
    void emit_statsBoxUpdated( int64_t i0,   /**< [in] upper-left x coordinate */
                               int64_t i1,   /**< [in] lower-right x coordinate */
                               int64_t j0,   /**< [in] upper-left y coordinate */
                               int64_t j1,   /**< [in] lower-right y coordinate */
                               float min,    /**< [in] minimum calibrated value */
                               float max,    /**< [in] maximum calibrated value */
                               float mean,   /**< [in] mean calibrated value */
                               float median, /**< [in] median calibrated value */
                               bool valid    /**< [in] true when stats are valid */
    );

  signals:

    /// Update the number of images in the cube
    void nzUpdated( uint32_t n /**< [in] the current number of images in the cube */ );

    /// Update the cube mode
    void cubeModeUpdated( bool mode /**< [in] the current cube mode (true is playing back, false is stopped */ );

    /// Update the cube FPS
    void cubeFPSUpdated( float fps, /**< [in] the current actual FPS*/
                         float desiredFPS /**< [in] the desired FPS*/ );

    /// Update the cube FPS multiplier
    void cubeFPSMultUpdated( float fpsMult /**< [in] the current FPS multiplier*/ );

    /// Update the cube direction
    void cubeDirUpdated( int dir /**< [in] the current cube direction (+1 is forward, -1 is backward)*/ );

    /// Update the cube frame number
    void cubeFrameUpdated( uint32_t fno /**< [in] the current cube frame number*/ );

    /// Pixel value updated.
    void pixelValueUpdated( uint32_t x,  /**< [in] x coordinate of the pixel */
                            uint32_t y,  /**< [in] y coordinate of the pixel */
                            float value, /**< [in] the calibrated pixel value */
                            bool valid   /**< [in] true when value is valid */
    );

    /// Color-box min/max updated.
    void colorBoxUpdated( int64_t i0, /**< [in] upper-left x coordinate */
                          int64_t i1, /**< [in] lower-right x coordinate */
                          int64_t j0, /**< [in] upper-left y coordinate */
                          int64_t j1, /**< [in] lower-right y coordinate */
                          float min,  /**< [in] minimum calibrated value */
                          float max,  /**< [in] maximum calibrated value */
                          bool valid  /**< [in] true when min/max are valid */
    );

    /// Stats-box values updated.
    void statsBoxUpdated( int64_t i0,   /**< [in] upper-left x coordinate */
                          int64_t i1,   /**< [in] lower-right x coordinate */
                          int64_t j0,   /**< [in] upper-left y coordinate */
                          int64_t j1,   /**< [in] lower-right y coordinate */
                          float min,    /**< [in] minimum calibrated value */
                          float max,    /**< [in] maximum calibrated value */
                          float mean,   /**< [in] mean calibrated value */
                          float median, /**< [in] median calibrated value */
                          bool valid    /**< [in] true when stats are valid */
    );

  public slots:

    /// Calls the parent's rtimvBase::imageTimeout( int )
    void imageTimeout( int to /**< [in] the new image display timeout*/ );

    /// Calls the parent's rtimvBase::cubeMode( bool )
    void cubeMode( bool cm /**< [in] the new cube mode */ );

    /// Calls the parent's rtimvBase::cubeFPS( float )
    void cubeFPS( float fps /**< [in] the new cube FPS*/ );

    /// Calls the parent's rtimvBase::cubeFPSMult( float )
    void cubeFPSMult( float mult /**< [in] the new cube FPS multiplier */ );

    /// Calls the parent's rtimvBase::cubeDir( int )
    void cubeDir( int dir /**< [in] the new cube direction*/ );

    /// Calls the parent's rtimvBase::cubeFrame( uint32_t )
    void cubeFrame( uint32_t fno /**< [in] the new cube frame number*/ );

    /// Calls the parent's rtimvBase::cubeFrameDelta( int32_t )
    void cubeFrameDelta( int32_t dfno /**< [in] the change in image number */ );

    /// Calls the parent's rtimvBase::updateImages()
    void updateImages();

    /// Calls the parent's rtimvBase::updateCube()
    void updateCube();

    /// Calls the parent's rtimvBase::updateCubeFrame()
    void updateCubeFrame();

    // GRPC Stuff:

  public:
    void emit_ImageNeeded();

    void emit_ImageWaiting();

  signals:

    void ImageNeeded();

    void ImageWaiting();

  public slots:

    void reconnect();

    /// Schedule a delayed ImagePlease call on the foundation object's Qt thread.
    void scheduleImagePlease( int delayMs /**< [in] delay before requesting the next image, ms */ );

    void ImagePlease();

    void ImageReceived();
};

#endif // rtimv_rtimvBase_hpp
