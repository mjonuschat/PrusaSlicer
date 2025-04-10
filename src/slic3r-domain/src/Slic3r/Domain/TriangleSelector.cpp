#include "Slic3r/Domain/TriangleSelector.hpp"

#include <libassert/assert.hpp>

namespace Slic3r::Domain::TriangleSelector {

bool TriangleBitStreamMapping::operator==(const TriangleBitStreamMapping& rhs) const
{
    return this->triangle_idx == rhs.triangle_idx && this->bitstream_start_idx == rhs.bitstream_start_idx;
}

bool TriangleBitStreamMapping::operator!=(const TriangleBitStreamMapping& rhs) const
{
    return !(rhs == *this);
}

template<class Archive>
void TriangleBitStreamMapping::serialize(Archive& ar)
{
    ar(this->triangle_idx, this->bitstream_start_idx);
}

bool TriangleSplittingData::operator==(const TriangleSplittingData& rhs) const
{
    return this->triangles_to_split == rhs.triangles_to_split
        && this->bitstream == rhs.bitstream
        && this->used_states == rhs.used_states;
}

bool TriangleSplittingData::operator!=(const TriangleSplittingData& rhs) const
{
    return !(rhs == *this);
}

void TriangleSplittingData::reset_used_states()
{
    this->used_states.resize(static_cast<size_t>(TriangleStateType::Count), false);
    std::fill(this->used_states.begin(), this->used_states.end(), false);
}

void TriangleSelector::TriangleSplittingData::update_used_states(const size_t bitstream_start_idx)
{
    ASSERT(bitstream_start_idx < this->bitstream.size());
    ASSERT(!this->bitstream.empty() && this->bitstream.size() != bitstream_start_idx);
    ASSERT((this->bitstream.size() - bitstream_start_idx) % 4 == 0);

    if (this->bitstream.empty() || this->bitstream.size() == bitstream_start_idx)
        return;

    size_t nibble_idx = bitstream_start_idx;

    auto read_next_nibble = [&data_bitstream = std::as_const(this->bitstream), &nibble_idx]() -> uint8_t {
        ASSERT(nibble_idx + 3 < data_bitstream.size());
        uint8_t code = 0;
        for (size_t bit_idx = 0; bit_idx < 4; ++bit_idx) {
            code |= data_bitstream[nibble_idx++] << bit_idx;
        }

        return code;
    };

    while (nibble_idx < this->bitstream.size()) {
        const uint8_t code = read_next_nibble();

        if (const bool is_split = (code & 0b11) != 0; is_split)
            continue;

        const uint8_t facet_state = (code & 0b1100) == 0b1100 ? read_next_nibble() + 3 : code >> 2;
        ASSERT(facet_state < this->used_states.size());
        if (facet_state >= this->used_states.size())
            continue;

        this->used_states[facet_state] = true;
    }
}

template<class Archive>
void TriangleSplittingData::serialize(Archive& ar)
{
    ar(this->triangles_to_split, this->bitstream, this->used_states);
}

} // namespace Slic3r::Domain::TriangleSelector
