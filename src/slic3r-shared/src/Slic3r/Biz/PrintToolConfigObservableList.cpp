///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/PrintToolConfigObservableList.hpp"

#include <Slic3r/Domain/Preset/HwConfig.hpp>
#include <Slic3r/Domain/Config.hpp>
#include <Slic3r/Domain/Preset/SelectedPreset.hpp>

namespace Slic3r::Biz {

PrintToolConfigObservableList::PrintToolConfigObservableList(
    const Domain::Workbench& workbench,
    Scene::SceneInteractor& scene_interactor
) :
    m_workbench(workbench),
    m_scene_interactor(scene_interactor),
    m_scene_bed_changed_listener_scope(scene_interactor, *this),
    m_selected_bed_changed_listener_scope(scene_interactor, *this),
    m_print_tool_shared_context{false, m_extruder_candidates}
{}

const PrintToolItem& PrintToolConfigObservableList::at(size_t index) const
{
    return m_items.at(index);
}

size_t PrintToolConfigObservableList::size() const
{
    return m_items.size();
}

void PrintToolConfigObservableList::set_sources(
    const Domain::SelectionId selected_project_id,
    Domain::Preset::SelectedPreset& selected_preset,
    const std::vector<Domain::ConfigBox*>& tool_config_boxes
)
{
    m_selected_project_id = selected_project_id;
    m_extruder_candidates.clear();

    Domain::ConfigBox& print_config_box = selected_preset.print.config_box();
    m_print_tool_shared_context.has_multiple_extruders =
        Domain::Preset::get_feature<bool>(selected_preset.hw_config.features, "multi_extruder")
            .value_or(false);

    const std::vector<Domain::ConfigItem>& print_items = print_config_box.items.all_items();
    if (!m_items.empty()
        && print_items.size() == m_items.size()
        && m_tool_config_boxes.size() == tool_config_boxes.size())
    {
        m_print_config_box  = &print_config_box;
        m_tool_config_boxes = tool_config_boxes;
        // we shall simply update items
        update_items();
    } else {
        // construct items from scratch
        invoke_listeners<IListObserver<PrintToolItem>>([](IListObserver<PrintToolItem>* l)
                                                       { l->on_will_be_reset(); });

        m_print_config_box  = &print_config_box;
        m_tool_config_boxes = tool_config_boxes;
        m_items.clear();
        for (const Domain::ConfigItem& print_item : print_items) {
            std::vector<PrintToolItem::ToolOverride> tool_items;
            std::vector<const Domain::ConfigItem*> tool_overrides;
            if (print_item.def().overrides_in.contains(Domain::FDMConfigLocation::Tool)) {
                tool_items.reserve(tool_config_boxes.size());
                for (const Domain::ConfigBox* tool_config_box : tool_config_boxes) {
                    const Domain::ConfigItem* tool_override =
                        tool_config_box->overrides.find(print_item.name());
                    bool overriden = tool_config_box->overrides.get(print_item.name()).has_value();
                    ASSERT(tool_override);
                    tool_items.emplace_back(tool_override, overriden);
                    tool_overrides.emplace_back(overriden ? tool_override : nullptr);
                }
            }
            m_items.emplace_back(
                print_item.name(),
                false,
                &print_item,
                tool_items,
                Domain::apply_compatibility_rule(
                    &print_item.value(),
                    tool_overrides,
                    m_extruder_candidates
                ),
                m_print_tool_shared_context
            );
        }

        invoke_listeners<IListObserver<PrintToolItem>>([](IListObserver<PrintToolItem>* l)
                                                       { l->on_reset(); });
    }
}

void PrintToolConfigObservableList::set_print_value(
    const std::string& key,
    const Domain::ConfigValue& value
)
{
    PrintToolItems::iterator index_it = find_item(key);

    if (index_it != m_items.end() && index_it->print_item->value() != value) {
        const size_t index = std::distance(m_items.begin(), index_it);

        m_print_config_box->items.opt(key).set(value);

        index_it->update_value();
        invoke_listeners<IListObserver<PrintToolItem>>([index](IListObserver<PrintToolItem>* l)
                                                       { l->on_updated(index); });
    }
}

void PrintToolConfigObservableList::set_tool_override(
    const std::string& key,
    size_t index,
    bool override
)
{
    PrintToolItems::iterator index_it = find_item(key);

    if (index_it != m_items.cend() && index < index_it->tool_overrides.size()) {
        if (override) {
            m_tool_config_boxes.at(index)->overrides.enable(key);
        } else {
            m_tool_config_boxes.at(index)->overrides.disable(key);
        }

        index_it->tool_overrides.at(index).second = override;
        index_it->update_value();

        const size_t override_index = std::distance(m_items.begin(), index_it);
        invoke_listeners<IListObserver<PrintToolItem>>(
            [override_index](IListObserver<PrintToolItem>* l) { l->on_updated(override_index); }
        );
    }
}

void PrintToolConfigObservableList::set_tool_value(
    const std::string& key,
    size_t index,
    const Domain::ConfigValue& value
)
{
    PrintToolItems::iterator index_it = find_item(key);

    if (index_it != m_items.cend()
        && index < index_it->tool_overrides.size()
        && index_it->tool_overrides.at(index).first->value() != value)
    {
        m_tool_config_boxes.at(index)->overrides.set(key, value);
        index_it->tool_overrides.at(index).second = true;
        index_it->update_value();

        const size_t override_index = std::distance(m_items.begin(), index_it);
        invoke_listeners<IListObserver<PrintToolItem>>(
            [override_index](IListObserver<PrintToolItem>* l) { l->on_updated(override_index); }
        );
    }
}

void PrintToolConfigObservableList::set_project_id(const Domain::SelectionId selected_project_id)
{
    if (m_selected_project_id != selected_project_id) {
        m_selected_project_id = selected_project_id;

        update_extruders();
    }
}

const Domain::ConfigValue* PrintToolConfigObservableList::find_print_value(
    const std::string& name
) const
{
    Domain::ConfigItem* found_item = m_print_config_box->items.find(name);
    return found_item ? &found_item->value() : nullptr;
}

const Domain::ConfigValue*
PrintToolConfigObservableList::find_tool_value(const std::string& name, size_t index) const
{
    Domain::ConfigItem* found_item = m_tool_config_boxes.at(index)->items.find(name);
    return found_item ? &found_item->value() : nullptr;
}

void PrintToolConfigObservableList::on_bed_instance_extruder_candidates_changed(
    Domain::SelectionId project_id,
    Domain::BedRef instance,
    const std::vector<unsigned int>& extruder_candidates
)
{
    if (m_selected_project_id == project_id
        && m_scene_interactor.bed_selection().last_selected_bed() == instance)
    {
        update_extruders();
    }
}

void PrintToolConfigObservableList::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Scene::BedSelection& bed_selection
)
{
    if (m_selected_project_id == project_id) {
        update_extruders();
    }
}

PrintToolConfigObservableList::PrintToolItems::iterator PrintToolConfigObservableList::find_item(
    const std::string& name
)
{
    return std::find_if(
        m_items.begin(),
        m_items.end(),
        [name](const PrintToolItem& item) { return item.name == name; }
    );
}

void PrintToolConfigObservableList::update_extruders()
{
    const std::vector<unsigned> extruder_candidates =
        m_workbench.project(m_selected_project_id)
            .find_bed_instance_by_id(
                m_scene_interactor.bed_selection().last_selected_bed().instance_id
            )
            ->extruder_candidates;

    const std::set<unsigned> extruder_candidates_set{
        extruder_candidates.cbegin(),
        extruder_candidates.cend()
    };
    if (m_extruder_candidates != extruder_candidates_set) {
        m_extruder_candidates = extruder_candidates_set;
        update_items();
    }
}

void PrintToolConfigObservableList::update_items()
{
    for (PrintToolItem& tool_print_item : m_items) {
        tool_print_item.print_item = m_print_config_box->items.find(tool_print_item.name);

        tool_print_item.tool_overrides.clear();
        std::vector<const Domain::ConfigItem*> tool_overrides;
        if (tool_print_item.print_item->def().overrides_in.contains(
                Domain::FDMConfigLocation::Tool
            ))
        {
            for (const Domain::ConfigBox* tool_config_box : m_tool_config_boxes) {
                const Domain::ConfigItem* tool_override =
                    tool_config_box->overrides.find(tool_print_item.print_item->name());
                bool overriden =
                    tool_config_box->overrides.get(tool_print_item.print_item->name()).has_value();
                ASSERT(tool_override);
                tool_print_item.tool_overrides.emplace_back(tool_override, overriden);
                tool_overrides.emplace_back(overriden ? tool_override : nullptr);
            }
        }
        tool_print_item.value = Domain::apply_compatibility_rule(
            &tool_print_item.print_item->value(),
            tool_overrides,
            m_extruder_candidates
        );
    }

    invoke_listeners<IListObserver<PrintToolItem>>(
        [this](IListObserver<PrintToolItem>* l)
        { l->on_updated(IndexRange{0, m_items.size() - 1}); }
    );
}

} // namespace Slic3r::Biz
