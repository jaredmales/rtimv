/** \file rtimvFilters.hpp
 * \brief Image filtering declarations for rtimv.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */

#ifndef rtimv_rtimvFilters_hpp
#define rtimv_rtimvFilters_hpp

#include <complex>
#include <vector>

#include <Eigen/Dense>

#include <mx/improc/eigenImage.hpp>
#include <mx/math/ft/fftT.hpp>

namespace rtimv
{

/// Available high-pass filter implementations.
enum class hpFilter
{
    none,     ///< No high-pass filtering.
    gaussian, ///< Subtract a Gaussian-smoothed image (fw is FWHM in pixels).
    median,   ///< Subtract a median-smoothed image (fw is full width in pixels).
    mean,     ///< Subtract a mean-smoothed image (fw is full width in pixels).
    fourier,  ///< Fourier high-pass filter placeholder (currently pass-through).
    radprof   ///< Radial-profile subtraction placeholder (currently pass-through).
};

/// Available low-pass filter implementations.
enum class lpFilter
{
    none,     ///< No low-pass filtering.
    gaussian, ///< Gaussian smoothing (fw is FWHM in pixels).
    median,   ///< Median smoothing (fw is full width in pixels).
    mean      ///< Mean smoothing (fw is full width in pixels).
};

/// Shared read-only reference type for 2-D floating-point images.
using constImageRef = Eigen::Ref<const mx::improc::eigenImage<float>>;

/// Working state for repeated MTF calculations.
struct mtfContext
{
    /// Real-valued row-major input passed to FFTW after sanitizing non-finite pixels.
    std::vector<float> m_fftInput;

    /// Packed real-to-complex FFT output.
    std::vector<std::complex<float>> m_fftOutput;

    /// Reusable real-to-complex FFT plan.
    mx::math::ft::fftT<float, std::complex<float>, 2, 0> m_fft;

    /// Planned x dimension.
    int m_nx{ 0 };

    /// Planned y dimension.
    int m_ny{ 0 };
};

/// Apply a high-pass filter.
/**
 * The input image is not modified. Output image dimensions follow \p inim.
 */
void applyHPFilter( mx::improc::eigenImage<float> &outim, ///< [out] Filtered image result.
                    constImageRef inim,                   ///< [in] Input image.
                    hpFilter filter,                      ///< [in] Selected high-pass filter.
                    float fw,                             ///< [in] Filter width parameter in pixels.
                    mx::improc::eigenImage<float> &work   ///< [in,out] Temporary smoothing work buffer.
);

/// Apply a low-pass filter.
/**
 * The input image is not modified. Output image dimensions follow \p inim.
 */
void applyLPFilter( mx::improc::eigenImage<float> &outim, ///< [out] Filtered image result.
                    constImageRef inim,                   ///< [in] Input image.
                    lpFilter filter,                      ///< [in] Selected low-pass filter.
                    float fw                              ///< [in] Filter width parameter in pixels.
);

/// Calculate a display-ready modulation transfer function image.
/**
 * The output is the normalized modulus of the 2-D Fourier transform with zero frequency shifted to the image center.
 */
void calculateMTF( mx::improc::eigenImage<float> &outim, ///< [out] Full-size MTF image.
                   constImageRef inim,                   ///< [in] Input image.
                   mtfContext &ctx                       ///< [in,out] Reusable FFT working state.
);

} // namespace rtimv

#endif // rtimv_rtimvFilters_hpp
