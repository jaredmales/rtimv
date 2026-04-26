/** \file rtimvBaseObject.cpp
 * \brief Definitions for the rtimvBaseObject class
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#include "rtimvBaseObject.hpp"

// #include "rtimvBase.hpp"

// This file defines the base class, and must also define the RTIMV_BASE class
// Example:
// -DRTIMV_BASE_INCLUDE="rtimvBase.hpp"
//
// where "rtimvBase.hpp" includes:
// #define RTIMV_BASE rtimvBase
//
#include RTIMV_BASE_INCLUDE

rtimvBaseObject::rtimvBaseObject( RTIMV_BASE *parent, QObject *QParent ) : QObject( QParent ), m_parent( parent )
{
    connect( &m_imageTimer, SIGNAL( timeout() ), this, SLOT( updateImages() ) );
    connect( &m_cubeTimer, SIGNAL( timeout() ), this, SLOT( updateCube() ) );
    connect( &m_cubeFrameUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateCubeFrame() ) );

    // clang-format off
    #ifdef RTIMV_GRPC

    connect( &m_connectionTimer, SIGNAL(timeout()), this, SLOT(reconnect()));
    connect( this, SIGNAL(ImageNeeded()), this, SLOT(ImagePlease()));
    connect( this, SIGNAL(ImageWaiting()), this, SLOT(ImageReceived()));

    #endif
    // clang-format on
}

void rtimvBaseObject::shutdown()
{
    m_parent = nullptr;

    m_imageTimer.stop();
    m_cubeTimer.stop();
    m_cubeFrameUpdateTimer.stop();
    m_connectionTimer.stop();

    disconnect();
}

void rtimvBaseObject::emit_nzUpdated( uint32_t n )
{
    emit nzUpdated( n );
}

void rtimvBaseObject::emit_cubeModeUpdated( bool mode )
{
    emit cubeModeUpdated( mode );
}

void rtimvBaseObject::emit_cubeFPSUpdated( float fps, float desiredFPS )
{
    emit cubeFPSUpdated( fps, desiredFPS );
}

void rtimvBaseObject::emit_cubeFPSMultUpdated( float fpsMult )
{
    emit cubeFPSMultUpdated( fpsMult );
}

void rtimvBaseObject::emit_cubeDirUpdated( int dir )
{
    emit cubeDirUpdated( dir );
}

void rtimvBaseObject::emit_cubeFrameUpdated( uint32_t fno )
{
    emit cubeFrameUpdated( fno );
}

void rtimvBaseObject::emit_pixelValueUpdated( uint32_t x, uint32_t y, float value, bool valid )
{
    emit pixelValueUpdated( x, y, value, valid );
}

void rtimvBaseObject::emit_colorBoxUpdated(
    int64_t i0, int64_t i1, int64_t j0, int64_t j1, float min, float max, bool valid )
{
    emit colorBoxUpdated( i0, i1, j0, j1, min, max, valid );
}

void rtimvBaseObject::emit_statsBoxUpdated(
    int64_t i0, int64_t i1, int64_t j0, int64_t j1, float min, float max, float mean, float median, bool valid )
{
    emit statsBoxUpdated( i0, i1, j0, j1, min, max, mean, median, valid );
}

void rtimvBaseObject::imageTimeout( int to )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->imageTimeout( to );
}

void rtimvBaseObject::cubeMode( bool cm )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->cubeMode( cm );
}

void rtimvBaseObject::cubeFPS( float fps )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->cubeFPS( fps );
}

void rtimvBaseObject::cubeFPSMult( float mult )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->cubeFPSMult( mult );
}

void rtimvBaseObject::cubeDir( int dir )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->cubeDir( dir );
}

void rtimvBaseObject::cubeFrame( uint32_t fno )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->cubeFrame( fno );
}

void rtimvBaseObject::cubeFrameDelta( int32_t dfno )
{
    if( !m_parent )
    {
        return;
    }

    m_parent->cubeFrameDelta( dfno );
}

void rtimvBaseObject::updateImages()
{
    // clang-format off
    #ifndef RTIMV_GRPC

    if( !m_parent )
    {
        return;
    }

    m_parent->updateImages();

    #endif
    // clang-format on
}

void rtimvBaseObject::updateCube()
{
    if( !m_parent )
    {
        return;
    }

    m_parent->updateCube();
}

void rtimvBaseObject::updateCubeFrame()
{
    if( !m_parent )
    {
        return;
    }

    m_parent->updateCubeFrame();
}

void rtimvBaseObject::emit_ImageNeeded()
{
    emit ImageNeeded();
}

void rtimvBaseObject::emit_ImageWaiting()
{
    emit ImageWaiting();
}

void rtimvBaseObject::reconnect()
{
    // clang-format off
    #ifdef RTIMV_GRPC

    if( !m_parent )
    {
        return;
    }

    m_parent->reconnect();

    #endif
    // clang-format on
}

void rtimvBaseObject::scheduleImagePlease( int delayMs )
{
    // clang-format off
    #ifdef RTIMV_GRPC

    QTimer::singleShot( delayMs, this, SLOT( ImagePlease() ) );

    #else
    static_cast<void>( delayMs );
    #endif
    // clang-format on
}

void rtimvBaseObject::ImagePlease()
{
    // clang-format off
    #ifdef RTIMV_GRPC

    if( !m_parent )
    {
        return;
    }

    m_parent->ImagePlease();

    #endif
    // clang-format on
}

void rtimvBaseObject::ImageReceived()
{
    // clang-format off
    #ifdef RTIMV_GRPC

    if( !m_parent )
    {
        return;
    }

    m_parent->ImageReceived();

    #endif
    // clang-format on
}
