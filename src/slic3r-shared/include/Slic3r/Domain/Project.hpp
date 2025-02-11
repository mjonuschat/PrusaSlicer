#pragma once

#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/FindById.hpp"

namespace Slic3r {
class Model;
class ModelObject;
class ModelVolume;
class ModelInstance;
enum class ModelVolumeType : int;
}

namespace Slic3r::Domain {

/**
 * All data that can be loaded/saved into .3mf file.
 */
class Project final
{
public:
    using ConfigContainerList = std::vector<std::unique_ptr<ConfigContainer>>;

    Project();

    void load(const std::string& file_path);

    void set_file_name(const std::string& file_name) { m_file_name = file_name; }
    [[nodiscard]] const std::string& file_name() const { return m_file_name; }

    [[nodiscard]] ConfigContainerList& config_containers() { return m_config_containers; }
    [[nodiscard]] const ConfigContainerList& config_containers() const { return m_config_containers; }

    [[nodiscard]] const Model& model() const { return *m_model; }
    [[nodiscard]] Model& model() { return *m_model; }

    [[nodiscard]] const ConfigContainer* find_config_container(size_t id) const;
    [[nodiscard]] ConfigContainer* find_config_container(size_t id);

    [[nodiscard]] const ModelObject* find_object_by_id(size_t id) const;
    [[nodiscard]] const ModelVolume* find_volume_by_id(size_t obj_id, size_t vol_id) const;
    [[nodiscard]] const ModelInstance* find_instance_by_id(size_t obj_id, size_t inst_id) const;
    [[nodiscard]] ModelObject* find_object_by_id(size_t id);
    [[nodiscard]] ModelVolume* find_volume_by_id(size_t obj_id, size_t vol_id);
    [[nodiscard]] ModelInstance* find_instance_by_id(size_t obj_id, size_t inst_id);

    [[nodiscard]] const BedInstance* find_bed_instance_by_id(size_t id) const;
    [[nodiscard]] BedInstance* find_bed_instance_by_id(size_t id);

    [[nodiscard]] const BedContainer& bed_container() const { return m_bed_container; }
    [[nodiscard]] BedContainer& bed_container() { return m_bed_container; }
    [[nodiscard]] const Bed* find_bed_by_id(size_t id) const;
    [[nodiscard]] Bed* find_bed_by_id(size_t id);

    /**
     * @brief Remove single model instance from bed instance.
     * @param model_instance Model instance to be removed from bed instance
     */
    void remove_instance_from_bed(ModelInstance* model_instance);

    /**
     * @brief Rebuild all model-instance to bed links.
     */
    void update_instances_bed_placement();

    /**
     * @brief Rebuild model-instance to bed links for given instances
     * @param instances List of instacnes to update
     * @param remove_original_links If true the original links are removed before update,
     * if false it is assumed that the instances are newly added and has no original links.
     */
    void update_instances_bed_placement(const ElementRefs& instances, bool remove_original_links = true);

    /**
     * @brief Rebuild model-instance to bed links for given instances
     * @param instances List of instacnes to update
     * @param remove_original_links If true the original links are removed before update,
     */
    void update_instances_bed_placement(
        const ModelInstanceList& instances, bool remove_original_links = true
    );

    ModelInstanceList& unplaced_model_instances() { return m_unplaced_model_instances; }
    const ModelInstanceList& unplaced_model_instances() const { return m_unplaced_model_instances; }
private:
    BedInstance* find_bed_instance_for_bounds(const BoundingBoxf& bounds);
    void update_instance_bed_placement(ModelInstance& inst);

private:
    std::string m_file_name;
    ConfigContainerList m_config_containers;
    BedContainer m_bed_container;
    std::unique_ptr<Model> m_model;
    ModelInstanceList m_unplaced_model_instances;
};

} // namespace Slic3r::Domain