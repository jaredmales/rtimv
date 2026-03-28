/** \file rtimvColorGRPC.hpp
 * \brief Conversion of color table enums in grpc for rtimv
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 *
 */

#ifndef rtimv_rtimvColorGRPC_hpp
#define rtimv_rtimvColorGRPC_hpp

#include "colorMaps.hpp"

namespace rtimv
{

inline remote_rtimv::Colorbar colorbar2grpc( rtimv::colorbar cb )
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

inline rtimv::colorbar grpc2colorbar( remote_rtimv::Colorbar colorbar )
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

inline remote_rtimv::Colormode colormode2grpc( rtimv::colormode cm )
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

inline rtimv::colormode grpc2colormode( remote_rtimv::Colormode colormode )
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

inline remote_rtimv::Colorstretch stretch2grpc( rtimv::stretch cs )
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

inline rtimv::stretch grpc2stretch( remote_rtimv::Colorstretch stretch )
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

} // namespace rtimv

#endif // rtimv_rtimvColorGRPC_hpp
