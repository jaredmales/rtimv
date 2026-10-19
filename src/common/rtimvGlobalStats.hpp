/** \file rtimvGlobalStats.hpp
 * \brief Shared global-statistics state for local and remote image backends.
 */

#ifndef rtimv_rtimvGlobalStats_hpp
#define rtimv_rtimvGlobalStats_hpp

#include <cstdint>

namespace rtimv
{

/// Availability of statistics across the image or its mask-selected locations.
enum class globalStatsStatus
{
    unavailable,  ///< No current result is available.
    valid,        ///< At least one finite selected value contributed.
    noImage,      ///< The science image is unavailable.
    noMask,       ///< The mask image is unavailable.
    sizeMismatch, ///< Required input dimensions differ from the processed image.
    empty,        ///< No finite selected values contributed.
    unsupported   ///< The remote server does not support global statistics.
};

/// Coherent backend snapshot of the global-statistics mode and its current result.
struct globalStatsState
{
    /// Whether calculation and display are enabled.
    bool m_enabled{ false };

    /// Whether the ordinary mask restricts the sample to locations whose mask value equals one.
    bool m_useMask{ true };

    /// Availability of the current result, independent of the enabled state.
    globalStatsStatus m_status{ globalStatsStatus::unavailable };

    /// Number of finite processed pixels included under the current selection policy.
    uint64_t m_count{ 0 };

    /// Minimum of the selected finite processed values.
    float m_min{ 0 };

    /// Maximum of the selected finite processed values.
    float m_max{ 0 };

    /// Mean of the selected finite processed values.
    float m_mean{ 0 };

    /// Median of the selected finite processed values.
    float m_median{ 0 };
};

} // namespace rtimv

#endif // rtimv_rtimvGlobalStats_hpp
