#include "Context.hpp"

#include "commonGL.hpp"
#include "ShaderManager.hpp"
#include "TextureManager.hpp"

#include <Slic3r/Log.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <sstream>

namespace Slic3r::App::Render {

const char* getGlString(GLenum name)
{
    const char* val = reinterpret_cast<const char*>(glGetString(name));
    glCheck();
    return val;
}


std::string getGlString(GLenum name, const std::string& defaultValue)
{
    const char* val = reinterpret_cast<const char*>(glGetString(name));
    glCheck();
    return val == nullptr ? defaultValue : val;
}

static const std::string VERSION_NA = "n/a";

Semver parse_version(const std::string& s)
{
    if (s == VERSION_NA)
        return Semver::invalid();
    std::vector<std::string> tokens;
    boost::split(tokens, s, boost::is_any_of(" "), boost::token_compress_on);

    if (tokens.empty())
        return Semver::invalid();

#if SLIC3R_OPENGL_ES
    const std::string version_container = (tokens.size() > 1 && boost::istarts_with(tokens[1], "ES")
                                          ) ?
        tokens[2] :
        tokens[0];
#endif // SLIC3R_OPENGL_ES

    std::vector<std::string> numbers;
#if SLIC3R_OPENGL_ES
    boost::split(numbers, version_container, boost::is_any_of("."), boost::token_compress_on);
#else
    boost::split(numbers, tokens[0], boost::is_any_of("."), boost::token_compress_on);
#endif // SLIC3R_OPENGL_ES

    unsigned int gl_major = 0;
    unsigned int gl_minor = 0;

    if (numbers.size() > 0)
        gl_major = ::atoi(numbers[0].c_str());

    if (numbers.size() > 1)
        gl_minor = ::atoi(numbers[1].c_str());

    return Semver(gl_major, gl_minor, 0);
}

Context::Context()
{
    std::string gl_version = getGlString(GL_VERSION, VERSION_NA);
    m_opengl_version = parse_version(gl_version);
    std::string glsl_version = getGlString(GL_SHADING_LANGUAGE_VERSION, VERSION_NA);
    m_glsl_version = parse_version(glsl_version);
    m_core_profile = !GLEW_ARB_compatibility;
#ifdef EMSCRIPTEN
    m_vao_available = GLEW_OES_vertex_array_object;
#else
    m_vao_available = true;
#endif

    m_shader_manager = std::make_unique<ShaderManager>(*this);
    m_texture_manager = std::make_unique<TextureManager>(*this);
}

void Context::log_gl_info() const
{
    SPDLOG_INFO("OpenGL Vendor: {}", getGlString(GL_VENDOR));
    SPDLOG_INFO("OpenGL Version: {}", getGlString(GL_VERSION));
    SPDLOG_INFO("GLSL Version: {}", getGlString(GL_SHADING_LANGUAGE_VERSION));
    SPDLOG_INFO("OpenGL Renderer: {}", getGlString(GL_RENDERER));
#ifdef EMSCRIPTEN
    SPDLOG_INFO("OpenGL Extensions: {}", getGlString(GL_EXTENSIONS));
#else
    if (is_core_profile()) {
        int n;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);

        std::ostringstream oss;
        for (int i = 0; i < n; i++) {
            if (i > 0)
                oss << " ";
            oss << reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        }
        SPDLOG_INFO("OpenGL Extensions: {}", oss.str());
    } else
        SPDLOG_INFO("OpenGL Extensions: {}", getGlString(GL_EXTENSIONS));
#endif
}

Context& Context::instance()
{
    static Context inst;
    return inst;
}

} // namespace Slic3r::App::Render
