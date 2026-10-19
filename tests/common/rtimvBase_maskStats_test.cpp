/** \file rtimvBase_maskStats_test.cpp
 * \brief Exercise mask selection, processing, and input lifecycle through rtimvBase.
 */

#include <QCoreApplication>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "rtimvBase.hpp"

namespace
{

/// Require a condition independently of NDEBUG.
void require( bool condition /**< [in] required condition */, const char *message /**< [in] failure diagnostic */
)
{
    if( !condition )
        throw std::runtime_error( message );
}

/// Controllable image source, including validity changes without a new-frame notification.
class TestImage : public rtimvImage
{
  public:
    /// Construct an image using the backend's raw-data mutex.
    explicit TestImage( std::mutex *mutex /**< [in] shared raw-data lock */ );

    /// Source width.
    uint32_t m_width{ 2 };

    /// Source height.
    uint32_t m_height{ 2 };

    /// Current source validity.
    bool m_valid{ true };

    /// Update code returned once by update().
    int m_update{ RTIMVIMAGE_NOUPDATE };

    /// Row-major source pixels.
    std::vector<float> m_pixels{ 1, 100, 3, 200 };

    /// Set an unused source key.
    int imageKey( const std::string &key /**< [in] unused key */ ) override;

    /// Get a synthetic source key.
    std::string imageKey() override;

    /// Get a synthetic source name.
    std::string imageName() override;

    /// Ignore the display timeout.
    void timeout( int value /**< [in] unused timeout */ ) override;

    /// Get source width.
    uint32_t nx() override;

    /// Get source height.
    uint32_t ny() override;

    /// Get source depth.
    uint32_t nz() override;

    /// Get the current frame.
    uint32_t imageNo() override;

    /// Get acquisition time.
    double imageTime() override;

    /// Consume the next update notification.
    int update() override;

    /// Invalidate the source without emitting a new frame.
    void detach() override;

    /// Get current validity.
    bool valid() override;

    /// Read a source pixel with bounds checking.
    float pixel( size_t index /**< [in] row-major index */ ) override;

    /// Get the synthetic frame rate.
    float fpsEst() override;

    /// Get the source sample size.
    size_t bytesPerPixel() override;
};

TestImage::TestImage( std::mutex *mutex ) : rtimvImage( mutex )
{
}

int TestImage::imageKey( const std::string &key )
{
    static_cast<void>( key );
    return 0;
}

std::string TestImage::imageKey()
{
    return "test";
}

std::string TestImage::imageName()
{
    return "test";
}

void TestImage::timeout( int value )
{
    static_cast<void>( value );
}

uint32_t TestImage::nx()
{
    return m_width;
}

uint32_t TestImage::ny()
{
    return m_height;
}

uint32_t TestImage::nz()
{
    return 1;
}

uint32_t TestImage::imageNo()
{
    return 0;
}

double TestImage::imageTime()
{
    return 0;
}

int TestImage::update()
{
    const int result = m_update;
    m_update = RTIMVIMAGE_NOUPDATE;
    return result;
}

void TestImage::detach()
{
    m_valid = false;
}

bool TestImage::valid()
{
    return m_valid;
}

float TestImage::pixel( size_t index )
{
    return m_pixels.at( index );
}

float TestImage::fpsEst()
{
    return 0;
}

size_t TestImage::bytesPerPixel()
{
    return sizeof( float );
}

/// Backend with real processing and no GUI hooks.
class TestBase : public rtimvBase
{
  public:
    /// Install synthetic science and mask sources.
    TestBase();

    /// Destroy synthetic sources before the base schedules QObject cleanup.
    ~TestBase();

    /// Return a synthetic source for single-threaded fixture setup.
    TestImage &source( size_t index /**< [in] source slot */ );

  protected:
    /// Ignore GUI image allocation work in the backend test.
    void mtxL_postSetImsize( const uniqueLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the backend test.
    void mtxL_postColormode( rtimv::colormode mode /**< [in] requested mode */,
                             const sharedLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the backend test.
    void mtxL_postRecolor( const uniqueLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the backend test.
    void mtxL_postRecolor( const sharedLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the backend test.
    void mtxL_postChangeImdata( const sharedLockT &lock /**< [in] held lock */ ) override;

    /// Ignore GUI work in the backend test.
    void post_zoomLevel() override;
};

TestBase::TestBase()
{
    m_images.resize( 4, nullptr );
    m_images[0] = new TestImage( &m_rawMutex );
    m_images[2] = new TestImage( &m_rawMutex );
    m_images[1] = new TestImage( &m_rawMutex );
    source( 1 ).m_pixels = { 1, 1, 1, 1 };
    source( 2 ).m_pixels = { 1, 0, 1, 0 };
    mtxUL_changeImdata();
}

TestBase::~TestBase()
{
    for( auto &image : m_images )
    {
        delete image;
        image = nullptr;
    }
}

TestImage &TestBase::source( size_t index )
{
    return *static_cast<TestImage *>( m_images.at( index ) );
}

void TestBase::mtxL_postSetImsize( const uniqueLockT &lock )
{
    static_cast<void>( lock );
}

void TestBase::mtxL_postColormode( rtimv::colormode mode, const sharedLockT &lock )
{
    static_cast<void>( mode );
    static_cast<void>( lock );
}

void TestBase::mtxL_postRecolor( const uniqueLockT &lock )
{
    static_cast<void>( lock );
}

void TestBase::mtxL_postRecolor( const sharedLockT &lock )
{
    static_cast<void>( lock );
}

void TestBase::mtxL_postChangeImdata( const sharedLockT &lock )
{
    static_cast<void>( lock );
}

void TestBase::post_zoomLevel()
{
}

/// Check the known two-pixel sample without including excluded pixels as zeros.
void checkPair( const rtimv::maskStatsState &stats /**< [in] measured result */ )
{
    require( stats.m_enabled && stats.m_status == rtimv::maskStatsStatus::valid, "missing valid result" );
    require( stats.m_count == 2 && stats.m_min == 1 && stats.m_max == 3 && stats.m_mean == 2 && stats.m_median == 2,
             "incorrect selected-pixel statistics" );
}

/// Verify selection, independent controls, and source lifecycle.
void testSelection()
{
    TestBase base;
    require( !base.maskStats().m_enabled, "mask stats should default off" );
    base.mtxUL_maskStats( true );
    checkPair( base.maskStats() );
    base.subtractDark( true );
    require( base.maskStats().m_min == 0 && base.maskStats().m_mean == 1, "dark subtraction was not honored" );
    base.subtractDark( false );
    base.applyMask( true );
    checkPair( base.maskStats() );
    base.statsBox( true );
    base.statsBox( false );
    checkPair( base.maskStats() );
    base.applyMask( false );

    base.source( 2 ).m_pixels = { 0, 1, 0, 1 };
    base.source( 2 ).m_update = RTIMVIMAGE_IMUPDATE;
    base.updateImages();
    auto stats = base.maskStats();
    require( stats.m_count == 2 && stats.m_min == 100 && stats.m_max == 200 && stats.m_mean == 150,
             "mask-only update did not refresh selection" );

    base.source( 2 ).detach();
    base.updateImages();
    stats = base.maskStats();
    require( stats.m_enabled && stats.m_status == rtimv::maskStatsStatus::noMask && stats.m_count == 0,
             "mask loss retained stale results" );
    base.source( 2 ).m_valid = true;
    base.updateImages();
    require( base.maskStats().m_status == rtimv::maskStatsStatus::valid, "mask recovery without a frame failed" );
    base.source( 2 ).m_width = 1;
    base.updateImages();
    require( base.maskStats().m_status == rtimv::maskStatsStatus::sizeMismatch, "mask mismatch was accepted" );
    base.source( 2 ).m_width = 2;
    base.updateImages();

    base.source( 0 ).detach();
    base.updateImages();
    require( base.maskStats().m_status == rtimv::maskStatsStatus::noImage && base.maskStats().m_count == 0,
             "science image loss retained stale results" );
    base.source( 0 ).m_valid = true;
    base.updateImages();
    require( base.maskStats().m_status == rtimv::maskStatsStatus::valid, "science image recovery failed" );

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    base.source( 0 ).m_pixels = { 0, -4, nan, inf };
    base.source( 2 ).m_pixels = { 1, 1, 1, 1 };
    base.mtxUL_changeImdata();
    stats = base.maskStats();
    require( stats.m_count == 2 && stats.m_min == -4 && stats.m_max == 0 && stats.m_mean == -2 && stats.m_median == -2,
             "zero, negative, or non-finite sample handling failed" );
    base.source( 2 ).m_pixels = { 0, 0, 1, 1 };
    base.mtxUL_changeImdata();
    require( base.maskStats().m_status == rtimv::maskStatsStatus::empty, "all non-finite selection should be empty" );
    base.source( 2 ).m_pixels = { 0, 0.5f, nan, inf };
    base.mtxUL_changeImdata();
    require( base.maskStats().m_status == rtimv::maskStatsStatus::empty, "nonbinary mask values were included" );

    base.source( 0 ).m_width = base.source( 2 ).m_width = 1;
    base.source( 0 ).m_height = base.source( 2 ).m_height = 1;
    base.source( 0 ).m_pixels = { 7 };
    base.source( 2 ).m_pixels = { 1 };
    base.mtxUL_changeImdata();
    stats = base.maskStats();
    require( stats.m_count == 1 && stats.m_min == 7 && stats.m_max == 7 && stats.m_median == 7,
             "single-pixel image failed" );
    base.source( 0 ).m_width = base.source( 2 ).m_width = 2;
    base.source( 0 ).m_height = base.source( 2 ).m_height = 3;
    base.source( 0 ).m_pixels = { 1, 2, 3, 4, 5, 6 };
    base.source( 2 ).m_pixels = { 1, 1, 1, 1, 1, 1 };
    base.mtxUL_changeImdata();
    stats = base.maskStats();
    require( stats.m_count == 6 && stats.m_mean == 3.5f && stats.m_median == 3.5f && stats.m_max == 6,
             "nonsquare image or even median failed" );
    base.mtxUL_maskStats( false );
    base.mtxUL_changeImdata();
    require( !base.maskStats().m_enabled && base.maskStats().m_count == 0, "disable retained results" );
}

/// Check that mask reduction samples the actual processed buffer, including its final row and column.
void checkProcessedSelection( TestBase &base /**< [in] backend with three selected pixels */ )
{
    std::vector<float> selected{ base.calPixel( 0, 0 ), base.calPixel( 1, 2 ), base.calPixel( 2, 2 ) };
    std::sort( selected.begin(), selected.end() );
    const auto stats = base.maskStats();
    const double mean = ( static_cast<double>( selected[0] ) + selected[1] + selected[2] ) / 3;
    require( stats.m_count == 3 && stats.m_min == selected[0] && stats.m_max == selected[2] &&
                 stats.m_median == selected[1] &&
                 std::fabs( stats.m_mean - mean ) < 1e-5 * std::max( 1.0, std::fabs( mean ) ),
             "mask statistics did not use selected processed pixels" );
}

/// Exercise HP/LP and MTF without creating an alternate processing path.
void testProcessing()
{
    TestBase base;
    base.source( 0 ).m_width = base.source( 2 ).m_width = 3;
    base.source( 0 ).m_height = base.source( 2 ).m_height = 3;
    base.source( 0 ).m_pixels = { 1, 100, 2, 200, 3, 300, 4, 400, 5 };
    base.source( 2 ).m_pixels = { 1, 0, 0, 0, 0, 0, 0, 1, 1 };
    base.mtxUL_maskStats( true );
    base.lpFilter( rtimv::lpFilter::mean );
    base.lpfFW( 3 );
    base.applyLPFilter( true );
    checkProcessedSelection( base );
    base.applyMask( true );
    checkProcessedSelection( base );
    require( base.calPixel( 1, 1 ) != 0, "fixture did not filter a nonzero value into an excluded location" );
    base.hpFilter( rtimv::hpFilter::mean );
    base.hpfFW( 3 );
    base.applyHPFilter( true );
    checkProcessedSelection( base );
    base.applyMTF( true );
    checkProcessedSelection( base );
    base.applyMask( false );
    checkProcessedSelection( base );
}

} // namespace

/// Run backend regression checks.
int main( int argc /**< [in] argument count */, char **argv /**< [in] arguments */ )
{
    QCoreApplication app( argc, argv );
    try
    {
        testSelection();
        testProcessing();
    }
    catch( const std::exception &error )
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
