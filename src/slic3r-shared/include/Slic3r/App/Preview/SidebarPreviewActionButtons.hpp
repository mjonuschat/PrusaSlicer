#pragma once

#include "Slic3r/App/SidebarActionButtons.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Preview {

enum class ActionButtonsLayoutType
{
    None,
    WithoutConnect,
    WithConnect
};

struct ActionButtonsLayout
{
    Yoga::ObjectPtr layout;
    Yoga::Object* layout_raw;
    Yoga::LayoutButton* primary_button;
    std::vector<Yoga::LayoutButton*> secondary_buttons;
    Yoga::LayoutButton* navigation_button;
};

class SidebarPreviewActionButtons :
    public SidebarActionButtons,
    public Biz::UserAccount::IUserAccountListener,
    public Biz::IStatusCacheChangedListener,
    public Biz::ISelectedBedInstancesChangedListener,
    public Biz::RemovableDrive::IRemovableDriveStatusListener
{
public:
    SidebarPreviewActionButtons(Navigator* render_module_navigator);
    ~SidebarPreviewActionButtons();

    void render_body(Yoga::Vec2f pos, Yoga::Vec2f size) override;

    void on_init(Biz::ProjectInteractor* project_interactor) override;

    void switch_layout(ActionButtonsLayoutType layout_type);

    void on_user_account_id_success(bool, const std::string&) override;
    void on_user_account_logged_out() override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;
    void on_status_cache_status_code_changed(const Domain::SlicingId slicing_id) override;

    void on_removable_drive_status_changed(
        const boost::filesystem::path&,
        Biz::RemovableDrive::RemovableDriveStatus
    ) override;

private:
    ActionButtonsLayout m_layout_with_connect;
    ActionButtonsLayout m_layout_without_connect;

    std::unique_ptr<Yoga::LayoutButton> get_primary_button();
    void update_buttons();

    ActionButtonsLayoutType active_layout() const;
};
} // namespace Slic3r::App::Preview
