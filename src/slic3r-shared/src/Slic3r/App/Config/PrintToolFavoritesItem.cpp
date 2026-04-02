#include "Slic3r/App/Config/PrintToolFavoritesItem.hpp"

#include <Slic3r/Domain/Config.hpp>

#include "Slic3r/Biz/PrintToolConfigBoxInteractor.hpp"
#include "Slic3r/Biz/PrintToolConfigObservableList.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PrintToolFavoritesItem::PrintToolFavoritesItem(Biz::ProjectInteractor& project_interactor) :
    m_rows_filter_list(std::make_shared<Biz::ObservableListSortFilter<Biz::PrintToolItem>>())
{
    set_object_name("PrintToolFavoritesItem");
    set_orientation(Orientation::Vertical);
    set_flex_shrink(0);

    m_rows_filter_list->set_filter_fn(
        [](const Biz::PrintToolItem& item) -> bool { return item.is_favorite; }
    );

    m_rows_filter_list->set_sort_fn(
        [](const Biz::PrintToolItem& lhs, const Biz::PrintToolItem& rhs)
        {
            return lhs.print_item->def().category < rhs.print_item->def().category
                || (lhs.print_item->def().category == rhs.print_item->def().category
                    && lhs.print_item->def().option_group < rhs.print_item->def().option_group)
                || (lhs.print_item->def().category == rhs.print_item->def().category
                    && lhs.print_item->def().option_group == rhs.print_item->def().option_group
                    && lhs.print_item->def().order < rhs.print_item->def().order);
        }
    );

    m_rows_filter_list->set_source_model(
        project_interactor.preset_interactor().print_tool_cbi().observable_list()
    );

    m_rows_list_view = emplace_back<PrintToolRowListView>(PrintToolRowListViewFactory{
        project_interactor.preset_interactor().print_tool_cbi(),
        project_interactor.preset_interactor(),
        project_interactor
    });
    m_rows_list_view->set_object_name("PrintToolRowFavoritesListView");
    m_rows_list_view->set_orientation(Orientation::Vertical);

    m_rows_list_view->set_source_list(m_rows_filter_list.get());
}

} // namespace Slic3r::App
