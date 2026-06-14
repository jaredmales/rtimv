/** \file rtimvFilters.cpp
 * \brief Image filtering definitions for rtimv.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */

#include "rtimvFilters.hpp"

#include <cmath>

#include <mx/improc/imageFilters.hpp>
#include <mx/math/ft/ftTypes.hpp>

namespace
{

/// Round to the nearest odd integer and enforce a minimum of 1.
int nearestOdd( float x )
{
    int n = static_cast<int>( std::lround( x ) );

    if( n < 1 )
    {
        n = 1;
    }

    if( ( n % 2 ) == 0 )
    {
        ++n;
    }

    return n;
}

/// Wrap a possibly negative Fourier index into an image dimension.
int wrapIndex( int idx, int dim )
{
    idx %= dim;
    if( idx < 0 )
    {
        idx += dim;
    }

    return idx;
}

} // namespace

namespace rtimv
{

void applyHPFilter( mx::improc::eigenImage<float> &outim,
                    constImageRef inim,
                    hpFilter filter,
                    float fw,
                    mx::improc::eigenImage<float> &work )
{
    outim.resize( inim.rows(), inim.cols() );
    work.resize( inim.rows(), inim.cols() );

    if( filter == hpFilter::none || fw <= 0 )
    {
        outim = inim;
        return;
    }

    if( filter == hpFilter::gaussian )
    {
        mx::improc::filterImage( work, inim, mx::improc::gaussKernel<mx::improc::eigenImage<float>, 2>( fw ) );
        outim = inim - work;
        return;
    }

    work = inim;

    if( filter == hpFilter::median )
    {
        mx::improc::medianSmooth( work, inim, nearestOdd( fw ) );
        outim = inim - work;
        return;
    }

    if( filter == hpFilter::mean )
    {
        mx::improc::meanSmooth( work, inim, nearestOdd( fw ) );
        outim = inim - work;
        return;
    }

    // Placeholder implementations currently pass input through unchanged.
    outim = inim;
}

void applyLPFilter( mx::improc::eigenImage<float> &outim, constImageRef inim, lpFilter filter, float fw )
{
    outim.resize( inim.rows(), inim.cols() );

    if( filter == lpFilter::none || fw <= 0 )
    {
        outim = inim;
        return;
    }

    if( filter == lpFilter::gaussian )
    {
        mx::improc::filterImage( outim, inim, mx::improc::gaussKernel<mx::improc::eigenImage<float>, 2>( fw ) );
        return;
    }

    outim = inim;

    if( filter == lpFilter::median )
    {
        mx::improc::medianSmooth( outim, inim, nearestOdd( fw ) );
        return;
    }

    if( filter == lpFilter::mean )
    {
        mx::improc::meanSmooth( outim, inim, nearestOdd( fw ) );
        return;
    }
}

void calculateMTF( mx::improc::eigenImage<float> &outim, constImageRef inim, mtfContext &ctx )
{
    const int nx = static_cast<int>( inim.rows() );
    const int ny = static_cast<int>( inim.cols() );

    outim.resize( nx, ny );

    if( nx <= 0 || ny <= 0 )
    {
        return;
    }

    ctx.m_fftInput.resize( static_cast<size_t>( nx * ny ) );

    const int packedY = ny / 2 + 1;
    ctx.m_fftOutput.resize( static_cast<size_t>( nx * ny ) );

    for( int y = 0; y < ny; ++y )
    {
        for( int x = 0; x < nx; ++x )
        {
            const float value = inim( x, y );
            ctx.m_fftInput[static_cast<size_t>( x * ny + y )] = std::isfinite( value ) ? value : 0.0f;
        }
    }

    if( ctx.m_nx != nx || ctx.m_ny != ny )
    {
        ctx.m_fft.plan( nx, ny, mx::math::ft::dir::forward, false );
        ctx.m_nx = nx;
        ctx.m_ny = ny;
    }

    ctx.m_fft( ctx.m_fftOutput.data(), ctx.m_fftInput.data() );

    const auto packedCoeff = [&]( int x, int y ) -> const std::complex<float> &
    { return ctx.m_fftOutput[static_cast<size_t>( x * packedY + y )]; };

    const float dc = std::abs( packedCoeff( 0, 0 ) );
    const bool normalize = std::isfinite( dc ) && dc > 0;
    const float invDc = normalize ? 1.0f / dc : 1.0f;

    outim.setZero();

    const int xShift = nx / 2;
    const int yShift = ny / 2;

    for( int y = 0; y < ny; ++y )
    {
        for( int x = 0; x < nx; ++x )
        {
            const int srcX = y < packedY ? x : wrapIndex( -x, nx );
            const int srcY = y < packedY ? y : ny - y;

            float modulus = std::abs( packedCoeff( srcX, srcY ) );
            if( normalize )
            {
                modulus *= invDc;
            }

            if( !std::isfinite( modulus ) )
            {
                modulus = 0.0f;
            }

            outim( wrapIndex( x + xShift, nx ), wrapIndex( y + yShift, ny ) ) = modulus;
        }
    }
}

} // namespace rtimv
