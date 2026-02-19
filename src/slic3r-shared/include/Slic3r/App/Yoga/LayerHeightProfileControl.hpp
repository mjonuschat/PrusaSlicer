#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/Domain/LayerHeightProfile.hpp"

#include <vector>

namespace Slic3r::App::Yoga {

class LayerHeightProfileControl : public Item
{
public:
    LayerHeightProfileControl();

    void set_object_max_z(float object_max_z);
    void set_min_layer_height(float min_layer_height);
    void set_max_layer_height(float max_layer_height);
    void set_default_layer_height(float default_layer_height);

    void set_layer_height_profile(const Domain::ZHeightPairs& layer_height_profile);

protected:
    float object_max_z() const;
    float min_layer_height_value() const;
    float max_layer_height_value() const;

    float project_layer_height(float layer_height, float out_range_min, float out_range_max) const;
    float project_layer_z(float layer_z, float out_range_min, float out_range_max) const;

    void render_baseline(const Vec2f& pos, const Vec2f& size) const;
    void render_layer_height_profile(const Vec2f& pos, const Vec2f& size) const;

private:
    float m_object_max_z         = 0.f;
    float m_min_layer_height     = 0.f;
    float m_max_layer_height     = 0.f;
    float m_default_layer_height = 0.f;

    Domain::ZHeightPairs m_layer_height_profile;
};

} // namespace Slic3r::App::Yoga
