/** \file rtimvColor.hpp
 * \brief Management of the color table in rtimv
 */

#ifndef rtimv_rtimvColor_hpp
#define rtimv_rtimvColor_hpp

#include <string_view>

#include "colorMaps.hpp"

namespace rtimv
{

enum class stretch
{
    linear, ///< The pixel values are scaled linearly to between m_minScaleData and m_maxScaleData
    log,    ///< The pixel values are scaled logarithmically between m_minScaleData and m_maxScaleData
    pow,    ///< the pixel values are scaled as \f$ 1000^p/1000 \f$ between m_minScaleData and m_maxScaleData
    sqrt,   ///< the pixel values are scaled as \f$ \sqrt(p) \f$ between m_minScaleData and m_maxScaleData
    square  ///< the pixel values are scaled as \f$ p^2 \f$ between m_minScaleData and m_maxScaleData
};

enum class colorbar
{
    grey,
    jet,
    hot,
    heat,
    bb,
    bone,
    red,
    green,
    blue
};

/// Get the configuration name of a colorbar.
inline std::string_view colorbarName( colorbar cb /**< [in] colorbar to name */ )
{
    switch( cb )
    {
    case colorbar::grey:
        return "grey";
    case colorbar::jet:
        return "jet";
    case colorbar::hot:
        return "hot";
    case colorbar::heat:
        return "heat";
    case colorbar::bb:
        return "bb";
    case colorbar::bone:
        return "bone";
    case colorbar::red:
        return "red";
    case colorbar::green:
        return "green";
    case colorbar::blue:
        return "blue";
    }

    return "grey";
}

/// Resolve a configured colorbar name.
inline bool colorbarFromString( colorbar &cb,         /**< [out] resolved colorbar */
                                std::string_view name /**< [in] configured name */
)
{
    if( name == "grey" || name == "gray" )
    {
        cb = colorbar::grey;
        return true;
    }

    if( name == "jet" )
    {
        cb = colorbar::jet;
        return true;
    }

    if( name == "hot" )
    {
        cb = colorbar::hot;
        return true;
    }

    if( name == "heat" )
    {
        cb = colorbar::heat;
        return true;
    }

    if( name == "bb" )
    {
        cb = colorbar::bb;
        return true;
    }

    if( name == "bone" )
    {
        cb = colorbar::bone;
        return true;
    }

    if( name == "red" )
    {
        cb = colorbar::red;
        return true;
    }

    if( name == "green" )
    {
        cb = colorbar::green;
        return true;
    }

    if( name == "blue" )
    {
        cb = colorbar::blue;
        return true;
    }

    return false;
}

enum class colormode
{
    minmaxglobal,
    minmaxbox,
    user
};

/// How to choose the display color for ordinary masked pixels.
enum class maskColorMode
{
    fixed,  ///< Use the configured color or the window background.
    mean,   ///< Use the rounded mean of the displayed palette indices.
    median, ///< Use the lower median of the displayed palette indices.
    mode    ///< Use the most frequent displayed palette index.
};

} // namespace rtimv

#endif // rtimv_rtimvColor_hpp
