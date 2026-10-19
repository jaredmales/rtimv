/** \file rtimvMaskStats.hpp
 * \brief Shared mask-statistics state for local and remote image backends.
 */

#ifndef rtimv_rtimvMaskStats_hpp
#define rtimv_rtimvMaskStats_hpp

#include <cstdint>

namespace rtimv
{

/// Availability of statistics at mask-selected locations.
enum class maskStatsStatus
{
    unavailable,  ///< No current result is available.
    valid,        ///< At least one finite selected value contributed.
    noImage,      ///< The science image is unavailable.
    noMask,       ///< The mask image is unavailable.
    sizeMismatch, ///< Mask and science-image dimensions differ.
    empty,        ///< No finite selected values contributed.
    unsupported   ///< The remote server does not support mask statistics.
};

/// Coherent backend snapshot of the mask-statistics mode and its current result.
struct maskStatsState
{
    /// Whether calculation and display are enabled.
    bool m_enabled{ false };

    /// Availability of the current result, independent of the enabled state.
    maskStatsStatus m_status{ maskStatsStatus::unavailable };

    /// Number of finite processed pixels selected by mask values equal to one.
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

#endif // rtimv_rtimvMaskStats_hpp
