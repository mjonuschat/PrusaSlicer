#pragma once

namespace Slic3r::Biz::Preset {

class PresetChangeListener {
public:
    virtual ~PresetChangeListener() = default;

    virtual void selected_printer_changed() = 0;
    virtual void selected_print_changed() = 0;
    virtual void selected_material_changed() = 0;
};

}
