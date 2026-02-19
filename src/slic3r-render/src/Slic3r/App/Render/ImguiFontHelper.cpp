#include "Slic3r/App/Render/ImguiFontHelper.hpp"

#include "Slic3r/Exception.hpp"

#include <Slic3r/Assert.hpp>
#include <Slic3r/Log.hpp>
#include "Slic3r/Directories.hpp"

#include <imgui/imgui.h>
#include <boost/filesystem/operations.hpp>

namespace Slic3r::App::Render {

ImguiFontHelper::ImguiFontHelper()
{
    create_font_texture();
}

ImFont* ImguiFontHelper::font(Render::ImguiFontType type)
{
    auto it = m_fonts.find(type);
    DEBUG_ASSERT(it != m_fonts.end());
    return (it != m_fonts.end()) ? it->second : nullptr;
}

static ImFont* load_font(
    const std::string& filename,
    const std::string& filename_cjk,
    const std::string& icon_font
)
{
    ImGuiIO& io            = ImGui::GetIO();
    const std::string path = Slic3r::resources_dir() + "/fonts/" + filename;
    ASSERT(boost::filesystem::exists(path), "font file does not exists");
    const std::string path_cjk = Slic3r::resources_dir() + "/fonts/" + filename_cjk;
    ASSERT(boost::filesystem::exists(path_cjk), "cjk font file does not exists");
    const std::string path_icons = Slic3r::resources_dir() + "/fonts/" + icon_font;
    ASSERT(boost::filesystem::exists(path_icons), "icon font file does not exists");
    ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str());

    if (font == nullptr) {
        SPDLOG_WARN("Fallback to default font");
        font = io.Fonts->AddFontDefault();
        if (font == nullptr) {
            throw Slic3r::RuntimeError("ImGui: Could not load default font");
        }
    }

    ImFontConfig config;
    config.MergeMode = true;

    io.Fonts->AddFontFromFileTTF(path_cjk.c_str(), 0, &config);
    io.Fonts->AddFontFromFileTTF(path_icons.c_str(), 0, &config);

    return font;
}

void ImguiFontHelper::create_font_texture()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    m_fonts[ImguiFontType::Regular] =
        load_font("NotoSans-Regular.ttf", "NotoSansCJK-Regular.ttc", "Slic3rIcons.ttf");
    m_fonts[ImguiFontType::Bold] =
        load_font("NotoSans-Bold.ttf", "NotoSansCJK-Bold.ttc", "Slic3rIcons.ttf");
    m_fonts[ImguiFontType::Italic] =
        load_font("NotoSans-Italic.ttf", "NotoSansCJK-Italic.ttc", "Slic3rIcons.ttf");
}

} // namespace Slic3r::App::Render
