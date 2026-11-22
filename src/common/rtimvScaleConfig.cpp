/** \file rtimvScaleConfig.cpp
 * \brief Parse and forward startup display scale limits.
 */

#include "rtimvScaleConfig.hpp"

#include <cmath>
#include <format>
#include <locale>
#include <sstream>
#include <stdexcept>

#include <mx/app/appConfigurator.hpp>

namespace
{

/// Read a finite floating-point limit when it was explicitly configured.
std::optional<float> readLimit( mx::app::appConfigurator &config /**< [in,out] parsed configuration */,
                                const std::string &name /**< [in] option to read */
)
{
    if( !config.isSet( name ) )
    {
        return {};
    }

    std::string value;
    config( value, name );
    std::istringstream input( value );
    input.imbue( std::locale::classic() );

    float limit = 0;
    if( !( input >> limit ) || !( input >> std::ws ).eof() || !std::isfinite( limit ) )
    {
        throw std::runtime_error( std::format( "rtimv: {} must be a finite number, got '{}'", name, value ) );
    }

    return limit;
}

} // namespace

namespace rtimv
{

void setupScaleLimitsConfig( mx::app::appConfigurator &config )
{
    config.add( "scale.min",
                "",
                "scale.min",
                mx::app::argType::Required,
                "scale",
                "min",
                false,
                "float",
                "Minimum display scale value at startup. Used when autoscale is false." );

    config.add( "scale.max",
                "",
                "scale.max",
                mx::app::argType::Required,
                "scale",
                "max",
                false,
                "float",
                "Maximum display scale value at startup. Used when autoscale is false." );
}

scaleLimits loadScaleLimitsConfig( mx::app::appConfigurator &config )
{
    scaleLimits limits{ readLimit( config, "scale.min" ), readLimit( config, "scale.max" ) };
    if( limits.m_min && limits.m_max &&
        ( *limits.m_min >= *limits.m_max || !std::isfinite( *limits.m_max - *limits.m_min ) ) )
    {
        throw std::runtime_error( "rtimv: scale.min must be less than scale.max and the scale range must be finite" );
    }

    return limits;
}

void appendScaleLimitsArgs( std::vector<std::string> &argv, const scaleLimits &limits )
{
    if( limits.m_min )
    {
        argv.push_back( std::format( "--scale.min={}", *limits.m_min ) );
    }
    if( limits.m_max )
    {
        argv.push_back( std::format( "--scale.max={}", *limits.m_max ) );
    }
}

} // namespace rtimv
