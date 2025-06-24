#pragma once

#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::App::Plater {

struct BedThumbnailTexture
{
    Domain::SelectionId project_id;
    Domain::BedRef bed_ref;
    Render::TexturePtr thumbnail;
};

using BedThumbnailTextures = std::vector<BedThumbnailTexture>;

} // namespace Slic3r::App::Plater