#pragma once
#include <memory>
#include "slic3r3/App/View/BaseRenderModule.hpp"

namespace Slic3r::Domain {
class Workbench;
}

namespace Slic3r::Biz {
class DataManager;
}


namespace Slic3r::App {
namespace Platform {
class IRenderingPlatform;
}

class App
{
public:
    void init(int argc, char** argv);
    void render(Platform::IRenderingPlatform& rendering_platform);


private:
    std::unique_ptr<Domain::Workbench> m_workbench;

    std::unique_ptr<View::BaseRenderModule> m_render_module;
};

}

