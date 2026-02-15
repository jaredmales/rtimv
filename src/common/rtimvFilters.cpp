/** \file rtimvFilters.cpp
 * \brief Image filtering definitions for rtimv.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */

#include "rtimvFilters.hpp"

#include <cmath>

#include <mx/improc/imageFilters.hpp>

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

} // namespace rtimv
