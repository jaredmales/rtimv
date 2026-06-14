/** \file rtimvFilters_mtf_test.cpp
 * \brief Tests for the rtimv modulation transfer function helper.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */

#include "rtimvFilters.hpp"

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>

namespace
{

/// Wrap a Fourier index into a positive image index.
int wrapIndex( int idx, int dim )
{
    idx %= dim;
    if( idx < 0 )
    {
        idx += dim;
    }

    return idx;
}

/// Compute a slow reference MTF for small test images.
mx::improc::eigenImage<float> referenceMTF( const mx::improc::eigenImage<float> &inim )
{
    constexpr float pi = 3.14159265358979323846f;

    const int nx = static_cast<int>( inim.rows() );
    const int ny = static_cast<int>( inim.cols() );
    mx::improc::eigenImage<float> outim( nx, ny );

    std::complex<float> dc{ 0, 0 };
    for( int y = 0; y < ny; ++y )
    {
        for( int x = 0; x < nx; ++x )
        {
            const float value = inim( x, y );
            dc += std::complex<float>( std::isfinite( value ) ? value : 0.0f, 0.0f );
        }
    }

    const float dcMod = std::abs( dc );
    const bool normalize = std::isfinite( dcMod ) && dcMod > 0;
    const float invDc = normalize ? 1.0f / dcMod : 1.0f;

    for( int ky = 0; ky < ny; ++ky )
    {
        for( int kx = 0; kx < nx; ++kx )
        {
            std::complex<float> sum{ 0, 0 };
            for( int y = 0; y < ny; ++y )
            {
                for( int x = 0; x < nx; ++x )
                {
                    const float value = inim( x, y );
                    const float safeValue = std::isfinite( value ) ? value : 0.0f;
                    const float phase =
                        -2.0f * pi * ( static_cast<float>( kx * x ) / nx + static_cast<float>( ky * y ) / ny );
                    sum += safeValue * std::complex<float>( std::cos( phase ), std::sin( phase ) );
                }
            }

            float modulus = std::abs( sum );
            if( normalize )
            {
                modulus *= invDc;
            }

            outim( wrapIndex( kx + nx / 2, nx ), wrapIndex( ky + ny / 2, ny ) ) = modulus;
        }
    }

    return outim;
}

/// Check two images for approximate equality.
bool imageNear( const mx::improc::eigenImage<float> &lhs, ///< [in] first image to compare.
                const mx::improc::eigenImage<float> &rhs, ///< [in] second image to compare.
                float tol                                 ///< [in] absolute tolerance.
)
{
    if( lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols() )
    {
        return false;
    }

    for( int y = 0; y < lhs.cols(); ++y )
    {
        for( int x = 0; x < lhs.rows(); ++x )
        {
            if( std::fabs( lhs( x, y ) - rhs( x, y ) ) > tol )
            {
                std::cerr << "mismatch at " << x << ", " << y << ": " << lhs( x, y ) << " != " << rhs( x, y ) << '\n';
                return false;
            }
        }
    }

    return true;
}

/// Exercise one image against the reference implementation.
bool checkImage( const mx::improc::eigenImage<float> &inim, ///< [in] image to transform.
                 float tol                                  ///< [in] absolute tolerance.
)
{
    rtimv::mtfContext ctx;
    mx::improc::eigenImage<float> mtf;
    rtimv::calculateMTF( mtf, inim, ctx );

    return imageNear( mtf, referenceMTF( inim ), tol );
}

} // namespace

int main()
{
    mx::improc::eigenImage<float> delta = mx::improc::eigenImage<float>::Zero( 4, 4 );
    delta( 0, 0 ) = 1.0f;
    if( !checkImage( delta, 1e-5f ) )
    {
        return 1;
    }

    mx::improc::eigenImage<float> constant = mx::improc::eigenImage<float>::Ones( 5, 3 );
    if( !checkImage( constant, 1e-5f ) )
    {
        return 1;
    }

    mx::improc::eigenImage<float> oddEven( 5, 4 );
    for( int y = 0; y < oddEven.cols(); ++y )
    {
        for( int x = 0; x < oddEven.rows(); ++x )
        {
            oddEven( x, y ) = static_cast<float>( 1 + x + 3 * y );
        }
    }
    if( !checkImage( oddEven, 1e-4f ) )
    {
        return 1;
    }

    mx::improc::eigenImage<float> nonFinite = oddEven;
    nonFinite( 2, 1 ) = std::numeric_limits<float>::quiet_NaN();
    if( !checkImage( nonFinite, 1e-4f ) )
    {
        return 1;
    }

    return 0;
}
