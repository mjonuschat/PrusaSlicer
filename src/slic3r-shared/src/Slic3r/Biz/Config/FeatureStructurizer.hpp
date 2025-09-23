#pragma once

#include "Slic3r/Domain/Preset/Types.hpp"

namespace Slic3r::Biz::Config {

Domain::JsonValue features_to_structure(const Domain::Preset::FeatureValueMap& features);
void remove_structurize_features(Domain::Preset::FeatureValueMap& features);
Domain::Preset::FeatureValueMap structure_to_features(const Domain::JsonValue& features);


}
