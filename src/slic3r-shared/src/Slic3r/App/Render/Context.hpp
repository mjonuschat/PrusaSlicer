#pragma once
#include <libslic3r/Semver.hpp>
#include <memory>

namespace Slic3r::App::Render {

class ShaderManager;
class TextureManager;

class Context
{
private:
    Context();

public:
    static Context& instance();

    const Semver& gl_version() const { return m_opengl_version; }
    const Semver& glsl_version() const { return m_glsl_version; }

    bool is_vao_available() const
    {
        return m_vao_available;
    }

    bool is_es() const
    {
#if SLIC3R_OPENGL_ES //|| defined(__EMSCRIPTEN__)
        return true;
#else
        return false;
#endif
    }

    bool is_core_profile() const { return m_core_profile; }
    void log_gl_info() const;

    [[nodiscard]] ShaderManager& shader_manager() const { return *m_shader_manager; }
    [[nodiscard]] TextureManager& texture_manager() const { return *m_texture_manager; }

private:
    Semver m_opengl_version;
    Semver m_glsl_version;
    bool m_core_profile;
    bool m_vao_available;

    std::unique_ptr<ShaderManager> m_shader_manager;
    std::unique_ptr<TextureManager> m_texture_manager;
};

} // namespace Slic3r::App::Render
