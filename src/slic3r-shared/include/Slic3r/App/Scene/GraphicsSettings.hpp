#pragma once

#include "Slic3r/Biz/Platform/WithListeners.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace Slic3r::App::Scene {

enum class ShadingType : uint8_t
{
    Legacy,
    Shadows,
    AO,
    PBR
};

static const std::vector<std::string> SHADING_TYPE_NAMES = {
    "Legacy",
    "Shadows",
    "Ambient occlusion",
    "Physically based rendering"
};

class IGraphicsSettingsChangedListener
{
public:
    virtual ~IGraphicsSettingsChangedListener() = default;
    virtual void on_shading_type_changed(ShadingType shading_type) = 0;
};

class GraphicsSettings : public WithListeners<IGraphicsSettingsChangedListener>
{
public:
    ShadingType shading_type() const { return m_shading_type; }
    void set_shading_type(ShadingType shading_type) {
        if (m_shading_type != shading_type) {
            m_shading_type = shading_type;
            invoke_listeners<IGraphicsSettingsChangedListener>([this](auto* l) { l->on_shading_type_changed(m_shading_type); });
        }
    }

    bool debug_windows_enabled() const { return m_debug_windows_enabled; }
    void set_debug_windows_enabled(bool enabled) { m_debug_windows_enabled = enabled; }

    bool shadows_enabled() const { return m_shading_type > ShadingType::Legacy; }
    bool ao_enabled() const { return m_shading_type > ShadingType::Shadows; }
    bool pbr_enabled() const { return m_shading_type > ShadingType::AO; }

private:
    ShadingType m_shading_type{ ShadingType::PBR };
    bool m_debug_windows_enabled{ false };
};

} // namespace Slic3r::App::Scene