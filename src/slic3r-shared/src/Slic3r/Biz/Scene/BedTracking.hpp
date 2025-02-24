#pragma once

#include <set>

#include <Slic3r/Domain/Project.hpp>
#include <Slic3r/Domain/BedRef.hpp>


namespace Slic3r::Biz {

/**
 * @brief Information on changed bed instances when updating model-instance to bed-instance tracking.
 */
struct BedTrackingChanges
{
    using BedRefSet = std::set<Domain::BedRef>;

    /**
     * @brief Set of changed bed instances
     */
    BedRefSet updated_beds;

    /**
     * @brief Indicates if Project::unplaced_instances gets updated too.
     */
    bool unplaced_instances_updated{false};

    void append(const BedTrackingChanges& others)
    {
        std::ranges::copy(others.updated_beds, std::inserter(updated_beds, updated_beds.end()));
        unplaced_instances_updated = unplaced_instances_updated || others.unplaced_instances_updated;
    }
};

/**
 * @brief Remove single model instance from bed instance.
 * @param project Project to update
 * @param model_instance Model instance to be removed from bed instance
 */
//void remove_instance_from_bed(Domain::Project& project, ModelInstance* model_instance);

/**
 * @brief Rebuild all model-instance to bed links.
 * @param project Project to update
 */
BedTrackingChanges update_instances_bed_placement(Domain::Project& project);

/**
 * @brief Rebuild model-instance to bed links for given instances
 * @param project Project to update
 * @param instances List of instances to update
 * @param remove_original_links If true the original links are removed before update,
 * if false it is assumed that the instances are newly added and has no original links.
 */
BedTrackingChanges update_instances_bed_placement(
    Domain::Project& project, const Domain::ElementRefs& instances, bool remove_original_links = true
);

/**
 * @brief Rebuild model-instance to bed links for given instances
 * @param project Project to update
 * @param instances List of instances to update
 * @param remove_original_links If true the original links are removed before update,
 */
BedTrackingChanges update_instances_bed_placement(
    Domain::Project& project,
    const Domain::ModelInstanceList& instances, bool remove_original_links = true
);



}

