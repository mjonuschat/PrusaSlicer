#include "Slic3r/App/Render/Types.hpp"
#include "Slic3r/App/Platform/ScreenInfo.hpp"


namespace Slic3r::App::Render {

Rect Rect::from(int x, int y, const Platform::ScreenInfo& screen)
{
    return {
        0, 0, static_cast<int>(screen.physical_width()), static_cast<int>(screen.physical_height())
    };
}

}
