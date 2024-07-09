#pragma once

#include <vector>
#include <cstddef>

namespace Slic3r::App::Render {

using ResourceId = unsigned int;
using ResourceIds = std::vector<ResourceId>;

class Context;
class Device
{
    friend class Context;
    explicit Device(Context& context): m_context(context) {}
public:
    void set_viewport();
    void clear_buffers(bool depth);
    void set_depth_test_enabled(bool enabled);
    void set_blending_enabled(bool enabled);

private:
    Context& m_context;
    ResourceId m_bound_vertex_buffer{0};
    ResourceId m_bound_index_buffer{0};
    ResourceIds m_bound_textures{0};
    size_t m_active_texture_unit{0};
};

}
