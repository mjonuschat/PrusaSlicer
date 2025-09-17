#include "Slic3r/App/RenderModuleHelper.hpp"
#if ENABLED_DEBUG_OUTLINE
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <imgui/imgui.h>
#endif // ENABLED_DEBUG_OUTLINE

namespace Slic3r::App {

#if ENABLED_DEBUG_OUTLINE
class ImguiVecRender
{
public:
    void operator()(const char* label, const Domain::Vec2f& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }

    void operator()(const char* label, const Domain::Vec2d& v)
    {
        fill_data<2>(v);
        ImGui::InputFloat2(label, m_data);
    }

    void operator()(const char* label, const Domain::Vec3d& v)
    {
        fill_data<3>(v);
        ImGui::InputFloat3(label, m_data);
    }

    void operator()(const char* label, const Domain::Vec4f& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }

    void operator()(const char* label, const Domain::Vec4d& v)
    {
        fill_data<4>(v);
        ImGui::InputFloat4(label, m_data);
    }

private:
    template <size_t N, typename VecT>
    void fill_data(const VecT& data)
    {
        for (size_t i = 0; i < N; i++)
            m_data[i] = static_cast<float>(data[i]);
    }

private:
    float m_data[4];
};

void imgui_scenegraph_node_info(const Scene::Node& node)
{
    ImGuiTreeNodeFlags node_flags = 0; // ImGuiTreeNodeFlags_DefaultOpen;
    if (node.children().empty())
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    const std::string& name = node.debug_name();
    if (!node.enabled())
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(90, 90, 90, 255));
    if (ImGui::TreeNodeEx(&node, node_flags, "%s %s%s%s%s",
        name.empty() ? "Node" : name.c_str(),
        node.has_render_component() ? "(R)" : "",
        node.has_material_override() ? "(M)" : "",
        node.has_imgui_render_component() ? "(I)" : "",
        node.has_raycast_component() ? "(C)" : "")) {
        static const Scene::Node* opened_node = nullptr;

        ImGui::SameLine();
        if (ImGui::SmallButton("info"))
            opened_node = (opened_node == &node) ? nullptr : &node;

        if (opened_node == &node) {
            auto transform{node.world_transform()};

            ImguiVecRender vec_render;
            for (size_t i = 0; i < 4; i++) {
                ImGui::PushID(i);
                vec_render("##", Domain::Vec4d{transform.matrix().row(i)});
                ImGui::PopID();
            }
        }

        for (const auto& ch : node.children()) {
            imgui_scenegraph_node_info(*ch);
        }
        ImGui::TreePop();
    }
    if (!node.enabled())
      ImGui::PopStyleColor();
}
#endif // ENABLED_DEBUG_OUTLINE

} // namespace Slic3r::App