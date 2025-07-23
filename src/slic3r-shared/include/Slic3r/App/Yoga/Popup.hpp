///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Yoga {

class RootItem;
class Popup;
using WindowPtr = std::unique_ptr<Window>;
using PopupPtr = std::unique_ptr<Popup>;

class Popup
{
public:
    struct Callbacks
    {
        std::function<void()> opened{nullptr};
        std::function<void()> closed{nullptr};
    };

    Popup();
    virtual ~Popup();
    Popup(const Popup& rhs) = delete;
    Popup& operator=(Popup& rhs) = delete;

    /**
     * @param item that the Popup is attached to
     */
    void attach_to_item(Item* item, Position prefered_position = Position::Right, float offset = 10);
    /**
     * @param item - any item from the tree, from which Popup will source RootItem
     */
    void attach_to_center(Item* item);
    /**
     * @param item - any item from the tree, from which Popup will source RootItem
     */
    void detach(Item* item);

    bool opened() const;
    void open();
    void open_at(const Vec2f& pos);
    void open_at(Item* item, Position prefered_position = Position::Right, float offset = 10);
    void close();

    Window* content_item() const;

    void render(const Vec2f& size);
    void style_node();
    void resize(const Vec2f& size);
    void process_events(Vec2f pos, Vec2f size);
    void check_resized();

    float offset() const;
    void set_offset(float offset);

    Position preferred_position() const;
    void set_preferred_position(Position preferred_position);

    RootItem* get_or_find_root_item();
    void set_root_item(RootItem* root_item);

    Callbacks& callbacks();

protected:
    void set_content_item(WindowPtr content_item);

private:
    void find_root_item();

    virtual void on_about_to_show();

private:
    Callbacks m_callbacks;

    RootItem* m_root_item = nullptr; ///< Set & Cached on the fly
    Item* m_parent = nullptr;
    WindowPtr m_content_item;
    YGNodeRef m_popup_node = nullptr;

    enum class AttachedType
    {
        Item,        ///< Attaches popup to an Item, uses Preferred position and offset
        Center,      ///< Attaches popup to the center of the screen
        FreeStanding ///< popup can be moved anywhere within the screen
    };

    AttachedType m_attached_type = AttachedType::FreeStanding;
    Item* m_attached_to = nullptr;
    Position m_preferred_position = Position::Right;
    float m_offset = 10;

    // hack
    bool m_resized = false;
    bool m_opened = false;
};

} // namespace Slic3r::App::Yoga
