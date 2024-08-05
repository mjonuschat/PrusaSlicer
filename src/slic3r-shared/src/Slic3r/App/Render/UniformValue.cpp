#include "UniformValue.hpp"
#include "Shader.hpp"

#include <type_traits>

namespace Slic3r::App::Render {

void set_uniform(Shader& shader, const char* param_name, const UniformValue& value)
{
    std::visit([param_name, &shader](auto&& arg) {
        shader.set_uniform(param_name, arg);
//        using T = std::decay_t<decltype(arg)>;
//        if constexpr (std::is_same_v<T, Vec2f>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, Vec2f>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, Vec3f>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, Matrix3f>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, Matrix4f>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, float>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, int>) {
//            shader.set_uniform(param_name, arg);
//        } else if constexpr (std::is_same_v<T, bool>) {
//            shader.set_uniform(param_name, arg);
//        } else {
//            static_assert(false, "Not exhaustive");
//        }
    }, value);
}

}
