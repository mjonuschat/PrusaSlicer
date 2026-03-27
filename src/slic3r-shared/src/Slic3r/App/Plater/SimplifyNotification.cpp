#include "Slic3r/App/Plater/SimplifyNotification.hpp"

#include <Slic3r/App/AppServices.hpp>
#include <Slic3r/App/PopNotification/PopNotificationCenter.hpp>

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Assert.hpp"

#include "fmt/format.h"

namespace Slic3r::App::Plater {
using namespace Slic3r;
using namespace Slic3r::Biz;

namespace {
bool exist_item(const SimplifyNotification::Item& item, const ProjectInteractor& project_interactor) {
    Domain::ElementRef e = item.element;
    const Domain::Project& project = project_interactor.project(item.project_id);
    const Domain::ModelVolume* volume = project.find_volume_by_id(e.object_id, e.volume_id);
    return volume != nullptr;
}

bool check_volume_exist(
    SimplifyNotification::Item& item, // fill result
    const ProjectInteractor& project_interactor, 
    const SimplifyNotification::Items& items) {

    if (!exist_item(item, project_interactor)) {
        // find first existing
        bool found = false;
        for (auto it = items.rbegin(); it != items.rend(); ++it)
        {
            if (exist_item(*it, project_interactor)) {
                item = *it;
                found = true;
                break;
            }
        }
        if (!found)
            return false; 
    }
    return true;
}
} // namespace

SimplifyNotification::SimplifyNotification(
    ProjectInteractor& project_interactor,
    Scene::GizmoManager& gizmo_manager, 
    PopNotification::PopNotificationCenter& notify) :
    m_project_interactor(project_interactor),
    m_notify(notify)
{
    m_project_interactor.scene_interactor().add_listener<Biz::Scene::ISceneChangedListener>(this);
    m_create_simplify_fn = [this, &gizmo_manager](const Item& item_)->std::function<bool()> {
        return [this, item_, &gizmo_manager]() -> bool {
            Item item = item_; // copy to remove const qualifier
            if (!check_volume_exist(item, m_project_interactor, m_meshes_to_simplify))
                return true; // can't opne simplify without volume

            m_project_interactor.select_project(item.project_id);
            Biz::Scene::ObjectSelection selection{
                .mode = Biz::Scene::SelectionMode::Volume,
                .elements = {item.element}
            };
            m_project_interactor.scene_interactor().set_object_selection(selection);
            if (gizmo_manager.current_tool_type() != Scene::ToolType::Simplify) {
                gizmo_manager.toggle_activate_tool(Scene::ToolType::Simplify, Domain::PrinterTechnology::FFF);
            }
            return true; // close popup
        };
    };
}

SimplifyNotification::~SimplifyNotification(){
    m_project_interactor.scene_interactor()
        .remove_listener<Biz::Scene::ISceneChangedListener>(this);
}

namespace {
bool need_simplify(const indexed_triangle_set& its) {
    return its.indices.size() > 1000000; // 1 milion triangles
}

std::string create_button_name(const Domain::ModelVolume& volume) {
    return _u8L("Simplify") + " \"" + volume.name + "\"";
}

std::string create_list_text(const Domain::ModelVolume& volume) {
    std::string count = fmt::to_string(float(volume.mesh().its.indices.size() / size_t(100000)) / 10.f);
    return fmt::format("{}({}M {})", volume.name, count, _u8L("Triangles"));
}

using namespace Slic3r::App::PopNotification;

std::string create_message(const SimplifyNotification::Items& items)
{
    if (items.size() == 1)
        return _u8L(
            "Triangle mesh with more than 1M triangles could be slow to process.\n"
            "It is highly recommended to reduce the amount of triangles."
        );
    std::string message =
        _u8L(
            "Application keeps multiple huge meshes.\n"
            "It is highly recommended to reduce the amount of triangles for"
        )
        + ":";
    for (const SimplifyNotification::Item& item : items)
        message += "\n \t" + item.list_text;
    return message;
}

namespace {
bool close_notification(PopNotificationCenter& notify) {
    auto& list = notify.observable_list();
    bool is_open = false;
    for (size_t i = 0; i < list.size(); i++) {
        if (list.at(i).type == PopNotificationType::SimplifySuggestion) {
            is_open = true;
            break;
        }
    }
    if (!is_open)
        return false;

    list.close_notifications_of_type(
        PopNotificationType::SimplifySuggestion);
    return true;

}
} // namespace

void recreate_notification(
    PopNotificationCenter& notify, 
    const SimplifyNotification::Items& items,
    const SimplifyNotification::CreateSimplifyFn& create_fn,
    bool open_when_closed = false)
{    
    if (bool was_open = close_notification(notify);
        (!open_when_closed && !was_open) || items.empty())
        return;

    const SimplifyNotification::Item& item = items.back();
    PopNotificationData data{
        .type = PopNotificationType::SimplifySuggestion,
        .level = PopNotificationLevel::Regular,
        .timeout = 0s,
        .layout = PopNotificationLayoutTextButtons {
            .text = create_message(items),
            .buttons = { PopNotificationButtonData {
                .text = item.button_name,
                .callback = create_fn(item)
            }}
        }
    };
    auto matcher = [](const PopNotificationPayload&, const PopNotificationPayload&) { return false; };
    notify.upsert_notifcation(data, matcher);
}
} // namespace

void SimplifyNotification::on_volume_added(
    Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    bool exist_change = false;
    const Domain::Project& project = m_project_interactor.project(project_id);
    for (const Domain::ElementRef& element : volumes) {
        const Domain::ModelVolume* volume = 
            project.find_volume_by_id(element.object_id, element.volume_id);
        if (volume == nullptr || !need_simplify(volume->mesh().its))
            continue;            
        m_meshes_to_simplify.push_back(Item{
            .project_id = project_id,
            .element = element,
            .button_name = create_button_name(*volume),
            .list_text = create_list_text(*volume)
        });
        exist_change = true;
    }
    if (exist_change)
        recreate_notification(m_notify, m_meshes_to_simplify, m_create_simplify_fn, true);
}

void SimplifyNotification::on_volume_removed(
    Domain::SelectionId project_id, const Domain::ElementRefs& volumes)
{
    bool exist_change = false;
    for (const Domain::ElementRef& el : volumes) {
        const auto [first, last] = std::ranges::remove_if(
            m_meshes_to_simplify, [project_id, &el](const Item& it) {
                return it.project_id == project_id && it.element == el; });
        if (first == last)
            continue;
        m_meshes_to_simplify.erase(first, last);
        exist_change = true;
    }
    if (exist_change)
        recreate_notification(m_notify, m_meshes_to_simplify, m_create_simplify_fn);
}

void SimplifyNotification::on_instance_added(
    Domain::SelectionId project_id, const Domain::ElementRefs& instances)
{
    Domain::ElementRefs el_volumes;
    const Domain::Project& project = m_project_interactor.project(project_id);
    for (const Domain::ElementRef& el_instance : instances) {
        const Domain::ModelObject* object = project.find_object_by_id(el_instance.object_id);
        if (object == nullptr || object->instances.size() != 1)
            continue; // no first instance

        // collect volumes of the just added object
        el_volumes.reserve(el_volumes.size() + object->volumes.size());
        for (const Domain::ModelVolume* volume : object->volumes)
            el_volumes.emplace_back(el_instance.object_id, el_instance.instance_id, volume->id().id);
    }
    if (el_volumes.empty())
        return; // no added volume

    on_volume_added(project_id, el_volumes);
}

void SimplifyNotification::on_instance_removed(
    Domain::SelectionId project_id, const Domain::ElementRefs& instances) 
{
    bool exist_change = false;
    const Domain::Project& project = m_project_interactor.project(project_id);
    for (const Domain::ElementRef& el_instance : instances) {
        if (project.find_object_by_id(el_instance.object_id) != nullptr )
            continue; // not last instance

        // find items from object
        const auto [first, last] = std::ranges::remove_if(m_meshes_to_simplify, 
            [project_id, object_id = el_instance.object_id](const Item& it) { 
                return it.project_id == project_id && it.element.object_id == object_id;
            });
        if (first == last)
            continue; // no object volume in list
        m_meshes_to_simplify.erase(first, last);
        exist_change = true;
    }
    if (exist_change)
        recreate_notification(m_notify, m_meshes_to_simplify, m_create_simplify_fn);
}

void SimplifyNotification::on_simplify(
    Domain::SelectionId project_id, 
    const Domain::ElementRefs& simplified_volumes)
{
    bool exist_change = false;
    for (const Domain::ElementRef& el : simplified_volumes) {
        const auto [first, last] = std::ranges::remove_if(m_meshes_to_simplify,
            [project_id, &el](const Item& it) {
                return it.project_id == project_id && 
                    it.element.object_id == el.object_id && 
                    it.element.volume_id == el.volume_id;
            });
        if (first == last)
            continue; // no object volume in list
        m_meshes_to_simplify.erase(first, last);
        exist_change = true;
    }
    if (exist_change)
        recreate_notification(m_notify, m_meshes_to_simplify, m_create_simplify_fn);

    // after simplify apply is assumption that you do not want to simplify more
    // even when model has more than 1M triangles. Soo do not check result of simplificaiton.
}
} // namespace Slic3r::App::Plater
