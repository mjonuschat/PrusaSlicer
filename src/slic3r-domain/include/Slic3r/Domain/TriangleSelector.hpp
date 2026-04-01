#pragma once

#include <cstdint>
#include <vector>

namespace cereal {
class access;
} // namespace cereal

namespace Slic3r::Domain::TriangleSelector {

enum class TriangleStateType : int8_t
{
    // Maximum is 3. The value is serialized in TriangleSelector into 2 bits.
    NONE      = 0,
    ENFORCER  = 1,
    BLOCKER   = 2,
    // For the fuzzy skin, we use just two values (NONE and FUZZY_SKIN).
    FUZZY_SKIN = ENFORCER,
    // Maximum is 15. The value is serialized in TriangleSelector into 6 bits using a 2 bit prefix code.
    Extruder1 = ENFORCER,
    Extruder2 = BLOCKER,
    Extruder3,
    Extruder4,
    Extruder5,
    Extruder6,
    Extruder7,
    Extruder8,
    Extruder9,
    Extruder10,
    Extruder11,
    Extruder12,
    Extruder13,
    Extruder14,
    Extruder15,
    Count
};

struct TriangleBitStreamMapping
{
    // Index of the triangle to which we assign the bitstream containing splitting information.
    int triangle_idx        = -1;
    // Index of the first bit of the bitstream assigned to this triangle.
    int bitstream_start_idx = -1;

    TriangleBitStreamMapping() = default;
    explicit TriangleBitStreamMapping(int triangleIdx, int bitstreamStartIdx) : triangle_idx(triangleIdx), bitstream_start_idx(bitstreamStartIdx) {}

    bool operator==(const TriangleBitStreamMapping& rhs) const;
    bool operator!=(const TriangleBitStreamMapping& rhs) const;
};

struct TriangleSplittingData {
    // Vector of triangles and its indexes to the bitstream.
    std::vector<TriangleBitStreamMapping> triangles_to_split;
    // Bit stream containing splitting information.
    std::vector<bool>                     bitstream;
    // Array indicating which triangle state types are used (encoded inside bitstream).
    std::vector<bool>                     used_states { std::vector<bool>(static_cast<std::size_t>(TriangleStateType::Count), false) };

    TriangleSplittingData() = default;

    bool operator==(const TriangleSplittingData& rhs) const;
    bool operator!=(const TriangleSplittingData& rhs) const;

    /**
     * Reset all used states before they are recomputed based on the bitstream.
     */
    void reset_used_states();

    /**
     * Update used states based on the bitstream. It just iterated over the bitstream from the bitstream_start_idx till the end.
     */
    void update_used_states(std::size_t bitstream_start_idx);
};

} // namespace Slic3r::Domain::TriangleSelector
