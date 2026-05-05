///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Render/ImguiTypes.hpp"
#include "Slic3r/App/Yoga/Namespace.hpp"
#include <vector>

namespace Slic3r::App::Yoga {

class Item;
class Icon;
class Text;

// Todo: This class should be moved to Slic3r::App
// Todo: KeyIcon component should be introduced instead of HelpIcon svg icon
class GizmoDialogHelp
{
public:
    explicit GizmoDialogHelp() {}
  
    /**
     * Initialize Help with container fot help items    
     */
    void init(Item* container);

    struct HelpIcon
    {
        Render::Icon icon{Render::Icon::None};
        Vec2f min_size{25.f, 25.f};
    };

    void add_item(const std::vector<HelpIcon>& icons, const std::string title, bool is_grayed = true);

    /**
     * @note Sometimes we need to change:
     * 1. One of the icons, OR
     * 2. The tint of an icon, OR
     * 3. The color of the title.
     */

    /**
     * Getter for the item's title.
     * @param item_index Index of the item.
     */
    Text* title(int item_index);

    /**
     * Getter for the item's icon.
     * @param item_index Index of the item.
     * @param icon_index Index of the icon in the icons vector (default is 0).
     */
    Icon* icon(int item_index, int icon_index = 0);

private:
    struct HelpItem
    {
        /**
         * HelpItem can contains several icons
         */
        std::vector<Icon*> icons;
        Text* title{nullptr};
    };

    std::vector<HelpItem> m_items;
    Item* m_container{nullptr};
};

} // namespace Slic3r::App::Yoga
