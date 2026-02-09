#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Uuid.hpp"

#include <ranges>

namespace Slic3r::Domain {


Project::Project() : m_metadata(generate_uuid()),  m_model(new Model()) {}

const ConfigContainer* Project::find_config_container(size_t id) const
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

ConfigContainer* Project::find_config_container(size_t id)
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

const ConfigContainer* Project::find_config_container_by_bed_instance_id(size_t id) const
{
    for (const auto& cc : m_config_containers) {
        if (find_by_id(cc->bed_instances(), id))
            return cc.get();
    }
    return nullptr;
}

ConfigContainer* Project::find_config_container_by_bed_instance_id(size_t id)
{
    for (auto& cc : m_config_containers) {
        if (find_by_id(cc->bed_instances(), id))
            return cc.get();
    }
    return nullptr;
}

const Domain::ModelObject* Project::find_object_by_id(size_t id) const
{
    return find_by_id<Domain::ModelObject>(m_model->objects, id);
}

const Domain::ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<Domain::ModelVolume>(obj->volumes, vol_id);
}

const ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

Domain::ModelObject* Project::find_object_by_id(size_t id)
{
    return find_by_id<Domain::ModelObject>(m_model->objects, id);
}

Domain::ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<Domain::ModelVolume>(obj->volumes, vol_id);
}

ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

const BedInstance* Project::find_bed_instance_by_id(size_t id) const
{
    for (const auto& cc : m_config_containers)
        if (auto* bed_inst = find_by_id(cc->bed_instances(), id))
            return bed_inst;
    return nullptr;
}

BedInstance* Project::find_bed_instance_by_id(size_t id)
{
    for (const auto& cc : m_config_containers)
        if (auto* bed_inst = find_by_id(cc->bed_instances(), id))
            return bed_inst;
    return nullptr;
}

const Bed* Project::find_bed_by_id(size_t id) const
{
    return find_by_id(m_bed_container.beds(), id);
}

Bed* Project::find_bed_by_id(size_t id)
{
    return find_by_id(m_bed_container.beds(), id);
}

namespace {

void visit(Project::ConfigContainerList& ccs, const std::function<void(Preset::SelectedPreset&)>& visitor)
{
    std::ranges::for_each(ccs, [&](auto& cc)
    {
        auto& selected_preset = cc->mutable_selected_preset();
        visitor(selected_preset);
    });
}

template <Preset::ConfigBoxLike Fdm, Preset::ConfigBoxLikeOrMonostate Sla>
struct PresetModifier
{
    using EP = Preset::EvaluatedPreset<Fdm, Sla>;
    using Selector = std::function<EP& (Preset::SelectedPreset& preset)>;
    using MultiSelector = std::function<std::vector<EP>& (Preset::SelectedPreset& preset)>;
    using SelectorVariant = std::variant<Selector, MultiSelector>;

    const EP& source;
    SelectorVariant selector;

    void operator()(Preset::SelectedPreset& preset) const
    {
        std::visit(
            overloaded{
                [&preset, this](const Selector& sel)
                {
                    EP& p = sel(preset);
                    if (p.id == source.id) {
                        p = source;
                    }
                },
                [&preset, this](const MultiSelector& sel)
                {
                    std::vector<EP>& ps = sel(preset);
                    std::ranges::for_each(
                        ps,
                        [&](auto& p)
                        {
                            if (p.id == source.id) {
                                p = source;
                            }
                        }
                    );
                }
            },
            selector
        );
    }
};

template <Preset::ConfigBoxLike Fdm, Preset::ConfigBoxLikeOrMonostate Sla>
PresetModifier<Fdm, Sla> make_preset_modifier(
    const Preset::EvaluatedPreset<Fdm, Sla>& source,
    const typename PresetModifier<Fdm, Sla>::SelectorVariant& selector
)
{
    PresetModifier<Fdm, Sla> modifier{source, selector};
    return modifier;
}

}

void Project::update_preset(const Preset::EvaluatedPrinterPreset::Preset& printer)
{
    visit(
        m_config_containers,
        make_preset_modifier(
            printer,
            [](Preset::SelectedPreset& preset) -> auto& { return preset.printer; }
        )
    );
}

void Project::update_preset(const Preset::EvaluatedPrintPreset::Preset& print)
{
    visit(
        m_config_containers,
        make_preset_modifier(
            print,
            [](Preset::SelectedPreset& preset) -> auto& { return preset.print; }
        )
    );

}

void Project::update_preset(const Preset::EvaluatedToolPrintPreset::Preset& tool_print)
{
    visit(
        m_config_containers,
        make_preset_modifier(
            tool_print,
            [](Preset::SelectedPreset& preset) -> auto& { return preset.tools; }
        )
    );
}

void Project::update_preset(const Preset::EvaluatedMaterialPreset::Preset& material)
{    visit(
        m_config_containers,
        make_preset_modifier(
            material,
            [](Preset::SelectedPreset& preset) -> auto& { return preset.materials; }
        )
    );
}


} // namespace Slic3r::Domain
