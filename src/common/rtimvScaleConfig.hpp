/** \file rtimvScaleConfig.hpp
 * \brief Startup configuration for display scale limits.
 */

#ifndef rtimv_rtimvScaleConfig_hpp
#define rtimv_rtimvScaleConfig_hpp

#include <optional>
#include <string>
#include <vector>

namespace mx::app
{
struct appConfigurator;
}

namespace rtimv
{

/// Optional initial endpoints for the display stretch.
struct scaleLimits
{
    /// Configured minimum, or empty to use the first frame's minimum.
    std::optional<float> m_min;

    /// Configured maximum, or empty to use the first frame's maximum.
    std::optional<float> m_max;
};

/// Register the startup scale limit options.
void setupScaleLimitsConfig( mx::app::appConfigurator &config /**< [in,out] configurator receiving the options */ );

/// Read and validate the startup scale limits.
scaleLimits loadScaleLimitsConfig( mx::app::appConfigurator &config /**< [in,out] parsed configuration */ );

/// Forward explicit startup limits as arguments without losing floating-point precision.
void appendScaleLimitsArgs( std::vector<std::string> &argv, /**< [in,out] arguments receiving the options */
                            const scaleLimits &limits       /**< [in] explicit startup limits */
);

} // namespace rtimv

#endif // rtimv_rtimvScaleConfig_hpp
