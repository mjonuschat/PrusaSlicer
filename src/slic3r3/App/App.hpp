#pragma once
#include <memory>
#include "View/Frame.hpp"
#include "View/FrameNavigable.hpp"

namespace Slic3r::Biz {
class DataManager;
}


namespace Slic3r::App {

class App : public View::FrameNavigable
{
public:
    void init(int argc, char** argv);
    void render_imgui();
    void render();

    void navigate_to_platter() override;
    void navigate_to_config() override;

private:
    std::unique_ptr<View::Frame> m_main_frame;


    View::Frame* m_active_frame {nullptr};
};

}

