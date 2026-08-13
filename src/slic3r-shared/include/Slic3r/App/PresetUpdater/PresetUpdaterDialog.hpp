#pragma once

#include "Slic3r/App/PresetUpdater/PresetUpdaterModel.hpp"
#include "Slic3r/App/PresetUpdater/PresetUpdaterSourceRow.hpp"
#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/Biz/ObservableListSortFilter.hpp"

namespace Slic3r::App {

class Navigator;

namespace Yoga {
class Item;
class LayoutButton;
class Text;
} // namespace Yoga


/// One instance per render module, both bound to the single PresetUpdaterModel owned by AppServices.
class PresetUpdaterDialog : public Yoga::Dialog, public IPresetUpdaterModelListener
{
public:
    using SourceFactory = Yoga::
        ViewFactory<PresetUpdaterSourceRow, PresetUpdaterSourceRowState, PresetUpdaterModel&>;
    using SourceListView =
        Yoga::ListView<PresetUpdaterSourceRow, PresetUpdaterSourceRowState, SourceFactory>;
    using SourceSortFilter = Biz::ObservableListSortFilter<PresetUpdaterSourceRowState>;

    PresetUpdaterDialog(PresetUpdaterModel& model, Navigator& navigator);
    ~PresetUpdaterDialog() override;

    void on_preset_updater_model_changed() override;

    void resize(const Yoga::SizeInfo& size_info) override;

protected:
    void close_action() override;

private:
    void build_content();
    void build_footer(Yoga::Item* parent);
    void pick_zip_archive();
    bool source_visible(const PresetUpdaterSourceRowState& source) const;

    void apply_size_limits(const Yoga::SizeInfo& size_info);

    PresetUpdaterModel& m_model;
    Navigator& m_navigator;

    SourceListView* m_online_list_view{nullptr};
    SourceListView* m_local_list_view{nullptr};

    Biz::UnsharedPointer<SourceSortFilter> m_online_filter;
    Biz::UnsharedPointer<SourceSortFilter> m_local_filter;

    bool m_forced_mode{false};

    Yoga::Text* m_offline_note{nullptr};

    Yoga::LayoutButton* m_add_zip_button{nullptr};
    Yoga::LayoutButton* m_repo_link_button{nullptr};
    Yoga::LayoutButton* m_update_everything_button{nullptr};
    Yoga::LayoutButton* m_footer_close_button{nullptr};
    Yoga::LayoutButton* m_exit_app_button{nullptr};
    Yoga::Item* m_forced_banner{nullptr};

    float m_applied_max_width{-1.f};
    float m_applied_max_height{-1.f};
};

} // namespace Slic3r::App
