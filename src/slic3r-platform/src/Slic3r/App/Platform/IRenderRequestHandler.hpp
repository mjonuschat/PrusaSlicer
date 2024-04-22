#pragma once

namespace Slic3r::App::Platform {

class IRenderRequestHandler {
public:
    virtual ~IRenderRequestHandler() = default;

    virtual void request_render() = 0;
};

}