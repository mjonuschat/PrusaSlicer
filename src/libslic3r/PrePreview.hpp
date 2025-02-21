#pragma once

#include "Slic3r/Biz/libpgcode/ProcessorResult.hpp"
#include "libpgcode/include/Slic3r/Biz/libpgcode/Types.hpp"
#include "libslic3r/Print.hpp"

namespace Slic3r::Biz::Print {

using MoveVerticesPerLayer = std::map<int, libpgcode::MoveVertices>;

MoveVerticesPerLayer get_wipe_tower_preview(const Slic3r::Print& print);

MoveVerticesPerLayer get_skirt_preview(
    const Slic3r::Print& print, std::vector<int>&& print_zs
);

MoveVerticesPerLayer get_brim_preview(const Slic3r::Print& print, const float height);

MoveVerticesPerLayer get_perimeters_preview(const PrintObject& object);

MoveVerticesPerLayer get_infill_preview(const PrintObject& object);

MoveVerticesPerLayer get_supports_preview(const PrintObject& object);


class Preview {
public:
    void update(MoveVerticesPerLayer&& moves);
    libpgcode::ProcessorResult generate_result(const Slic3r::Print& print) const;
    std::vector<int> get_scaled_print_zs() const;

private:
    MoveVerticesPerLayer m_moves_per_layer;
    mutable std::mutex m_mutex;
};

libpgcode::ProcessorResult get_result_preview(const Slic3r::Print& print);
}
