#pragma once
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/BatchObservableList.hpp"

namespace Slic3r::Biz {

/**
 * @brief Listener called whenever a selection index (of a list for instance) changes.
 */
struct IListSelectionChangedListener
{
    virtual ~IListSelectionChangedListener() = default;

    /**
     * @brief Called whenever selected item changes.
     * @param new_selection New selection index, if no `selection Domain::INVALID_ID` is passed instead.
     */
    virtual void on_list_selection_changed(Domain::SelectionId new_selection) = 0;
};

template <typename T, ObservableListType<T> ObservableListT = BatchObservableList<T>>
class ObservableListWithSelection : public WithListeners<IListSelectionChangedListener>
{
public:
    using List = ObservableListT;

    List& items()
    {
        return m_items;
    }

    const List& items() const
    {
        return m_items;
    }

    size_t selected_index() const
    {
        return m_selected_index;
    }

    void set_selected_index(size_t index)
    {
        m_selected_index = index;
        invoke_listeners<IListSelectionChangedListener>([index](IListSelectionChangedListener* l) {
            l->on_list_selection_changed(index);
        });
    }

    bool set_selected(const std::function<bool(const T&)>& predicate)
    {
        size_t n = m_items.size();
        for (size_t i = 0; i < n; ++i) {
            if (predicate(m_items.at(i))) {
                set_selected_index(i);
                return true;
            }
        }
        set_selected_index(Domain::INVALID_ID);
        return false;
    }

private:
    List m_items;
    Domain::SelectionId m_selected_index = Domain::INVALID_ID;
};

} // namespace Slic3r::Biz
