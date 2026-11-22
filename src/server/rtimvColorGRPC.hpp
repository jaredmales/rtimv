/** \file rtimvColorGRPC.hpp
 * \brief Conversion of color table enums in grpc for rtimv
 */

#ifndef rtimv_rtimvColorGRPC_hpp
#define rtimv_rtimvColorGRPC_hpp

#include "colorMaps.hpp"

namespace rtimv
{

/// Encode a colorbar as its gRPC enum value.
inline remote_rtimv::Colorbar colorbar2grpc( rtimv::colorbar cb /**< [in] colorbar to encode */ )
{
    if( cb == rtimv::colorbar::grey )
    {
        return remote_rtimv::COLORBAR_GREY;
    }
    else if( cb == rtimv::colorbar::jet )
    {
        return remote_rtimv::COLORBAR_JET;
    }
    else if( cb == rtimv::colorbar::hot )
    {
        return remote_rtimv::COLORBAR_HOT;
    }
    else if( cb == rtimv::colorbar::heat )
    {
        return remote_rtimv::COLORBAR_HEAT;
    }
    else if( cb == rtimv::colorbar::bb )
    {
        return remote_rtimv::COLORBAR_BB;
    }
    else if( cb == rtimv::colorbar::bone )
    {
        return remote_rtimv::COLORBAR_BONE;
    }
    else if( cb == rtimv::colorbar::red )
    {
        return remote_rtimv::COLORBAR_RED;
    }
    else if( cb == rtimv::colorbar::green )
    {
        return remote_rtimv::COLORBAR_GREEN;
    }
    else if( cb == rtimv::colorbar::blue )
    {
        return remote_rtimv::COLORBAR_BLUE;
    }

    return remote_rtimv::COLORBAR_UNKNOWN;
}

/// Decode a colorbar from its gRPC enum value.
inline rtimv::colorbar grpc2colorbar( remote_rtimv::Colorbar colorbar /**< [in] colorbar to decode */ )
{
    if( colorbar == remote_rtimv::COLORBAR_GREY )
    {
        return rtimv::colorbar::grey;
    }
    else if( colorbar == remote_rtimv::COLORBAR_JET )
    {
        return rtimv::colorbar::jet;
    }
    else if( colorbar == remote_rtimv::COLORBAR_HOT )
    {
        return rtimv::colorbar::hot;
    }
    else if( colorbar == remote_rtimv::COLORBAR_HEAT )
    {
        return rtimv::colorbar::heat;
    }
    else if( colorbar == remote_rtimv::COLORBAR_BB )
    {
        return rtimv::colorbar::bb;
    }
    else if( colorbar == remote_rtimv::COLORBAR_BONE )
    {
        return rtimv::colorbar::bone;
    }
    else if( colorbar == remote_rtimv::COLORBAR_RED )
    {
        return rtimv::colorbar::red;
    }
    else if( colorbar == remote_rtimv::COLORBAR_GREEN )
    {
        return rtimv::colorbar::green;
    }
    else if( colorbar == remote_rtimv::COLORBAR_BLUE )
    {
        return rtimv::colorbar::blue;
    }

    return static_cast<rtimv::colorbar>( -1 );
}

/// Encode a color mode as its gRPC enum value.
inline remote_rtimv::Colormode colormode2grpc( rtimv::colormode cm /**< [in] color mode to encode */ )
{
    if( cm == rtimv::colormode::minmaxglobal )
    {
        return ( remote_rtimv::COLORMODE_GLOBAL );
    }
    else if( cm == rtimv::colormode::minmaxbox )
    {
        return ( remote_rtimv::COLORMODE_BOX );
    }
    else if( cm == rtimv::colormode::user )
    {
        return ( remote_rtimv::COLORMODE_USER );
    }

    return remote_rtimv::COLORMODE_UNKNOWN;
}

/// Decode a color mode from its gRPC enum value.
inline rtimv::colormode grpc2colormode( remote_rtimv::Colormode colormode /**< [in] color mode to decode */ )
{
    if( colormode == remote_rtimv::COLORMODE_GLOBAL )
    {
        return rtimv::colormode::minmaxglobal;
    }
    else if( colormode == remote_rtimv::COLORMODE_BOX )
    {
        return rtimv::colormode::minmaxbox;
    }
    else if( colormode == remote_rtimv::COLORMODE_USER )
    {
        return rtimv::colormode::user;
    }

    return static_cast<rtimv::colormode>( -1 );
}

/// Encode a color stretch as its gRPC enum value.
inline remote_rtimv::Colorstretch stretch2grpc( rtimv::stretch cs /**< [in] color stretch to encode */ )
{
    if( cs == rtimv::stretch::linear )
    {
        return ( remote_rtimv::COLORSTRETCH_LINEAR );
    }
    else if( cs == rtimv::stretch::log )
    {
        return ( remote_rtimv::COLORSTRETCH_LOG );
    }
    else if( cs == rtimv::stretch::pow )
    {
        return ( remote_rtimv::COLORSTRETCH_POW );
    }
    else if( cs == rtimv::stretch::sqrt )
    {
        return ( remote_rtimv::COLORSTRETCH_SQRT );
    }
    else if( cs == rtimv::stretch::square )
    {
        return ( remote_rtimv::COLORSTRETCH_SQUARE );
    }

    return remote_rtimv::COLORSTRETCH_UNKNOWN;
}

/// Decode a color stretch from its gRPC enum value.
inline rtimv::stretch grpc2stretch( remote_rtimv::Colorstretch stretch /**< [in] color stretch to decode */ )
{
    if( stretch == remote_rtimv::COLORSTRETCH_LINEAR )
    {
        return rtimv::stretch::linear;
    }
    else if( stretch == remote_rtimv::COLORSTRETCH_LOG )
    {
        return rtimv::stretch::log;
    }
    else if( stretch == remote_rtimv::COLORSTRETCH_POW )
    {
        return rtimv::stretch::pow;
    }
    else if( stretch == remote_rtimv::COLORSTRETCH_SQRT )
    {
        return rtimv::stretch::sqrt;
    }
    else if( stretch == remote_rtimv::COLORSTRETCH_SQUARE )
    {
        return rtimv::stretch::square;
    }

    return static_cast<rtimv::stretch>( -1 );
}

/// Encode the requested masked-pixel color statistic.
inline remote_rtimv::MaskColorMode maskColorMode2grpc( maskColorMode mode /**< [in] mode to encode */ )
{
    switch( mode )
    {
    case maskColorMode::mean:
        return remote_rtimv::MASK_COLOR_MODE_MEAN;
    case maskColorMode::median:
        return remote_rtimv::MASK_COLOR_MODE_MEDIAN;
    case maskColorMode::mode:
        return remote_rtimv::MASK_COLOR_MODE_MODE;
    default:
        return remote_rtimv::MASK_COLOR_MODE_FIXED;
    }
}

/// Decode the requested masked-pixel color statistic.
inline maskColorMode grpc2maskColorMode( remote_rtimv::MaskColorMode mode /**< [in] mode to decode */ )
{
    switch( mode )
    {
    case remote_rtimv::MASK_COLOR_MODE_MEAN:
        return maskColorMode::mean;
    case remote_rtimv::MASK_COLOR_MODE_MEDIAN:
        return maskColorMode::median;
    case remote_rtimv::MASK_COLOR_MODE_MODE:
        return maskColorMode::mode;
    default:
        return maskColorMode::fixed;
    }
}

} // namespace rtimv

#endif // rtimv_rtimvColorGRPC_hpp
