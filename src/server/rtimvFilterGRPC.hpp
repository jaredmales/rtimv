/** \file rtimvFilterGRPC.hpp
 * \brief Conversion of filter enums for gRPC in rtimv.
 *
 * \author Jared R. Males (jaredmales@gmail.com)
 */

#ifndef rtimv_rtimvFilterGRPC_hpp
#define rtimv_rtimvFilterGRPC_hpp

#include "rtimv.grpc.pb.h"
#include "rtimvFilters.hpp"

namespace rtimv
{

inline remote_rtimv::HPFilter hpFilter2grpc( rtimv::hpFilter filter )
{
    switch( filter )
    {
    case rtimv::hpFilter::gaussian:
        return remote_rtimv::HPFILTER_GAUSSIAN;
    case rtimv::hpFilter::median:
        return remote_rtimv::HPFILTER_MEDIAN;
    case rtimv::hpFilter::mean:
        return remote_rtimv::HPFILTER_MEAN;
    case rtimv::hpFilter::fourier:
        return remote_rtimv::HPFILTER_FOURIER;
    case rtimv::hpFilter::radprof:
        return remote_rtimv::HPFILTER_RADPROF;
    default:
        return remote_rtimv::HPFILTER_UNKNOWN;
    }
}

inline rtimv::hpFilter grpc2hpFilter( remote_rtimv::HPFilter filter )
{
    switch( filter )
    {
    case remote_rtimv::HPFILTER_GAUSSIAN:
        return rtimv::hpFilter::gaussian;
    case remote_rtimv::HPFILTER_MEDIAN:
        return rtimv::hpFilter::median;
    case remote_rtimv::HPFILTER_MEAN:
        return rtimv::hpFilter::mean;
    case remote_rtimv::HPFILTER_FOURIER:
        return rtimv::hpFilter::fourier;
    case remote_rtimv::HPFILTER_RADPROF:
        return rtimv::hpFilter::radprof;
    default:
        return static_cast<rtimv::hpFilter>( -1 );
    }
}

inline remote_rtimv::LPFilter lpFilter2grpc( rtimv::lpFilter filter )
{
    switch( filter )
    {
    case rtimv::lpFilter::gaussian:
        return remote_rtimv::LPFILTER_GAUSSIAN;
    case rtimv::lpFilter::median:
        return remote_rtimv::LPFILTER_MEDIAN;
    case rtimv::lpFilter::mean:
        return remote_rtimv::LPFILTER_MEAN;
    default:
        return remote_rtimv::LPFILTER_UNKNOWN;
    }
}

inline rtimv::lpFilter grpc2lpFilter( remote_rtimv::LPFilter filter )
{
    switch( filter )
    {
    case remote_rtimv::LPFILTER_GAUSSIAN:
        return rtimv::lpFilter::gaussian;
    case remote_rtimv::LPFILTER_MEDIAN:
        return rtimv::lpFilter::median;
    case remote_rtimv::LPFILTER_MEAN:
        return rtimv::lpFilter::mean;
    default:
        return static_cast<rtimv::lpFilter>( -1 );
    }
}

} // namespace rtimv

#endif // rtimv_rtimvFilterGRPC_hpp
