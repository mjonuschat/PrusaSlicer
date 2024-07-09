#pragma once

#include "Shader.hpp"

#include <vector>
#include <string>
#include <memory>

namespace Slic3r::App::Render {
class Context;

class ShaderManager
{
public:
    explicit ShaderManager(Context& context) : m_context(context) {}

    std::pair<bool, std::string> init();
    // call this method before to release the OpenGL context
    void shutdown();

    // returns nullptr if not found
    Shader* get_shader(const std::string& shader_name);

    // returns currently active shader, nullptr if none
    Shader* get_current_shader();
private:
    Context& m_context;
    std::vector<std::unique_ptr<Shader>> m_shaders;
};


}