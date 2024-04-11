#pragma once
namespace Slic3r::App::Platform {

class IRenderingPlatform
{
public:
    virtual ~IRenderingPlatform() = default;

    virtual bool init() = 0;
    virtual bool should_quit() = 0;
    virtual void poll_events() = 0;
    virtual void begin_imgui_frame() = 0;
    virtual void end_imgui_frame() = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
};

}
