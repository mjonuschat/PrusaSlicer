#include "Slic3r/Biz/Algorithms/FacetsAnnotation.hpp"
#include "Slic3r/Biz/Algorithms/TriangleSelector.hpp"

//FIXME: Temporarily use includes from libslic3r until ModelVolume is moved into the domain.
#include <libslic3r/Model.hpp>

namespace Slic3r::Biz::Algorithms::FacetsAnnotation {

bool has_facets(const Domain::FacetsAnnotation& facets_annotation, TriangleStateType type)
{
    return TriangleSelector::has_facets(facets_annotation.triangle_splitting_data, type);
}

indexed_triangle_set get_facets(const Domain::FacetsAnnotation& facets_annotation, const ModelVolume& model_volume, TriangleStateType type)
{
    TriangleSelector selector(model_volume.mesh());
    // Reset of TriangleSelector is done inside TriangleSelector's constructor, so we don't need it
    // to perform it again in deserialize().
    selector.deserialize(facets_annotation.triangle_splitting_data, false);
    return selector.get_facets(type);
}

indexed_triangle_set get_facets_strict(const Domain::FacetsAnnotation& facets_annotation, const ModelVolume& model_volume, TriangleStateType type)
{
    TriangleSelector selector(model_volume.mesh());
    // Reset of TriangleSelector is done inside TriangleSelector's constructor, so we don't need it
    // to perform it again in deserialize().
    selector.deserialize(facets_annotation.triangle_splitting_data, false);
    return selector.get_facets_strict(type);
}

Domain::indexed_triangle_set_with_color get_all_facets_with_colors(const Domain::FacetsAnnotation& facets_annotation, const ModelVolume& model_volume)
{
    TriangleSelector selector(model_volume.mesh());
    // Reset of TriangleSelector is done inside TriangleSelector's constructor, so we don't need it
    // to perform it again in deserialize().
    selector.deserialize(facets_annotation.triangle_splitting_data, false);
    return selector.get_all_facets_with_colors();
}

Domain::indexed_triangle_set_with_color get_all_facets_strict_with_colors(const Domain::FacetsAnnotation& facets_annotation, const ModelVolume& model_volume)
{
    TriangleSelector selector(model_volume.mesh());
    // Reset of TriangleSelector is done inside TriangleSelector's constructor, so we don't need it
    // to perform it again in deserialize().
    selector.deserialize(facets_annotation.triangle_splitting_data, false);
    return selector.get_all_facets_strict_with_colors();
}

} // namespace Slic3r::Biz::Algorithms::FacetsAnnotation
