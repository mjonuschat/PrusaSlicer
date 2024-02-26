#pragma once

namespace Slic3r::App::View {
class Frame;

struct FrameNavigable {
    virtual void navigate_to_platter() = 0;
    virtual void navigate_to_config() = 0;
};

}
