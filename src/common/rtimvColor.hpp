/** \file rtimvColor.hpp
 * \brief Management of the color table in rtimv
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimv_rtimvColor_hpp
#define rtimv_rtimvColor_hpp

#include "colorMaps.hpp"

namespace rtimv
{

enum class stretch
{
    linear, ///< The pixel values are scaled linearly to between m_minScaleData and m_maxScaleData
    log,    ///< The pixel values are scaled logarithmically between m_minScaleData and m_maxScaleData
    pow,    ///< the pixel values are scaled as \f$ 1000^p/1000 \f$ between m_minScaleData and m_maxScaleData
    sqrt,   ///< the pixel values are scaled as \f$ \sqrt(p) \f$ between m_minScaleData and m_maxScaleData
    square ///< the pixel values are scaled as \f$ p^2 \f$ between m_minScaleData and m_maxScaleData
};

enum class colorbar
{
    grey,
    jet,
    hot,
    bone,
    red,
    green,
    blue
};

enum class colormode
{
    minmaxglobal,
    minmaxbox,
    user
};

} //namespace rtimv

#endif // rtimv_rtimvColor_hpp
