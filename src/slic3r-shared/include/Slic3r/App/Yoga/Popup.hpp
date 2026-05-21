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

class Popup : public Object
{
public:
    struct Callbacks
    {
        std::function<void()> opened{nullptr};
        std::function<void()> closed{nullptr};
    };

    Popup();
    ~Popup() override;

    /**
     * @param item that the Popup is attached to
     */
    void attach_to_item(Item* item, Position prefered_position = Position::Right, float offset = 10);
    /**
     * @param item - any item from the tree, from which Popup will source RootItem
     */
    void attach_to_center();

    bool opened() const;
    void open();
    void open_at(const Vec2f& pos);
    void close();

    Window* content_item() const;

    void render(const Vec2f& pos, const Vec2f& size) override;
    void resize(const SizeInfo &size_info) override;

    void check_resized();

    float offset() const;
    void set_offset(float offset);

    Position preferred_position() const;
    void set_preferred_position(Position preferred_position);

    Callbacks& callbacks();

protected:
    void set_content_item(WindowPtr content_item);

    void root_item_about_to_update() override;

private:
    virtual void on_about_to_show();
    virtual void on_about_to_close();

    // ------------ intentionally hidden ---------------
    void prepend(ObjectPtr child) override;
    void append(ObjectPtr child) override;
    void insert(ObjectPtr child, size_t index) override;
    ObjectPtr remove(Object* child) override;
    // ------------ intentionally hidden ---------------

private:
    Callbacks m_callbacks;

    YGNodeRef m_popup_node = nullptr;
    Window* m_content_item = nullptr;

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
    Vec2f m_last_size;
    Vec2f m_last_attached_pos;
    SizeInfo m_last_size_info;

    // hack
    bool m_resized = false;
    bool m_opened = false;
};

} // namespace Slic3r::App::Yoga
