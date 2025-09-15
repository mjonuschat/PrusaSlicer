#include "Slic3r/Biz/Emboss/TextLines.hpp"
#include "libslic3r/TriangleMeshSlicer.hpp"
#include "Slic3r/Biz/Algorithms/Tesselate.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp" // unscaled
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp" // its_make_sphere
#include "Slic3r/Biz/Algorithms/ExPolygon.hpp" // to_linesf
#include "Slic3r/Biz/Algorithms/AABBTreeLines.hpp"
#include "Slic3r/Biz/Algorithms/AABBTreeIndirect.hpp"
#include "Slic3r/Domain/ExPolygonsIndex.hpp"
#include "Slic3r/Biz/Algorithms/ClipperUtils.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include <Slic3r/App/Scene/NodeBuilder.hpp>

using namespace Slic3r;

namespace {

struct TextLineNodeTag {};

// Used to move slice (text line) on place where is approx vertical center of text
// When copy value const double ASCENT_CENTER from Emboss.cpp and Vertical align is center than
// text line will cross object center
const double ascent_ratio_offset = 1 / 3.;

double calc_line_height_in_mm(
    const Domain::FontFile& ff,
    const Domain::FontProp& fp
); // return lineheight in mm

// Be careful it is not water tide and contain self intersections
// It is only for visualization purposes
indexed_triangle_set
its_create_torus(const Domain::Polygon& polygon, float radius, size_t steps = 20)
{
    assert(!polygon.empty());
    if (polygon.empty())
        return {};

    size_t count = polygon.size();
    if (count < 3)
        return {};

    // convert and scale to float
    std::vector<Domain::Vec2f> points_d;
    points_d.reserve(count);
    for (const Domain::Point& point : polygon.points)
        points_d.push_back(Biz::Algorithms::Scaling::unscaled<float>(point));

    // pre calculate normalized line directions
    auto calc_line_norm = [](const Domain::Vec2f& f, const Domain::Vec2f& s) -> Domain::Vec2f
    { return (s - f).normalized(); };
    std::vector<Domain::Vec2f> line_norm(points_d.size());
    for (size_t i = 0; i < count - 1; ++i)
        line_norm[i] = calc_line_norm(points_d[i], points_d[i + 1]);
    line_norm.back() = calc_line_norm(points_d.back(), points_d.front());

    // precalculate sinus and cosinus
    double angle_step = 2 * M_PI / steps;
    std::vector<std::pair<double, float>> sin_cos;
    sin_cos.reserve(steps);
    for (size_t s = 0; s < steps; ++s) {
        double angle = s * angle_step;
        sin_cos.emplace_back(
            radius * std::sin(angle),
            static_cast<float>(radius * std::cos(angle))
        );
    }

    indexed_triangle_set sphere = Biz::Algorithms::TriangleMesh::its_make_sphere(radius, 2 * M_PI / steps);

    // create torus model along polygon path
    indexed_triangle_set model;
    model.vertices.reserve(2 * steps * count + sphere.vertices.size() * count);
    model.indices.reserve(2 * steps * count + sphere.indices.size() * count);

    const Domain::Vec2f* prev_prev_point_d = &points_d[count - 2]; // one before back
    const Domain::Vec2f* prev_point_d      = &points_d.back();

    auto calc_angle = [](const Domain::Vec2f& d0, const Domain::Vec2f& d1)
    {
        double dot = d0.dot(d1);
        double det = d0.x() * d1.y() - d0.y() * d1.x(); // Determinant
        return std::atan2(det, dot); // atan2(y, x) or atan2(sin, cos)
    };

    // opposit previos direction of line - for calculate angle
    Domain::Vec2f opposit_prev_dir = (*prev_prev_point_d) - (*prev_point_d);
    for (size_t i = 0; i < count; ++i) {
        const Domain::Vec2f& point_d = points_d[i];
        // line segment direction
        Domain::Vec2f dir = point_d - (*prev_point_d);

        double angle               = calc_angle(opposit_prev_dir, dir);
        double allowed_preccission = 1e-6;
        if (angle >= (M_PI - allowed_preccission) || angle <= (-M_PI + allowed_preccission))
            continue; // it is almost line

        // perpendicular direction to line
        Domain::Vec2d p_dir(dir.y(), -dir.x());
        p_dir.normalize(); // Should done with double preccission
        // p_dir is tube unit side vector
        // tube unit top vector is z direction

        // Tube
        int prev_index = model.vertices.size() + 2 * sin_cos.size() - 2;
        for (const auto& [s, c] : sin_cos) {
            Domain::Vec2f side = (s * p_dir).cast<float>();
            Domain::Vec2f xy0  = side + (*prev_point_d);
            Domain::Vec2f xy1  = side + point_d;
            model.vertices.emplace_back(xy0.x(), xy0.y(), c); // pointing of prev index
            model.vertices.emplace_back(xy1.x(), xy1.y(), c);

            // create triangle indices
            int f0     = prev_index;
            int s0     = f0 + 1;
            int f1     = model.vertices.size() - 2;
            int s1     = f1 + 1;
            prev_index = f1;
            model.indices.push_back(Domain::Index3{ s0, f0, s1 });
            model.indices.push_back(Domain::Index3{ f1, s1, f0 });
        }

        prev_prev_point_d = prev_point_d;
        prev_point_d      = &point_d;
        opposit_prev_dir  = -dir;
    }

    // sphere on each point
    for (Domain::Vec2f& p : points_d) {
        indexed_triangle_set sphere_copy = sphere;
        its_translate(sphere_copy, Domain::Vec3f(p.x(), p.y(), 0.f));
        Domain::its_merge(model, sphere_copy);
    }

    return model;
}

// select closest contour for each line
Biz::Emboss::TextLines select_closest_contour(const std::vector<Domain::Polygons>& line_contours)
{
    Biz::Emboss::TextLines result;
    result.reserve(line_contours.size());
    Domain::Vec2d zero(0., 0.);
    for (const Domain::Polygons& polygons : line_contours) {
        if (polygons.empty()) {
            result.emplace_back();
            continue;
        }
        // Improve: use int values and polygons only
        // Slic3r::Polygons polygons = union_(polygons);
        // std::vector<Slic3r::Line> lines = to_lines(polygons);
        // AABBTreeIndirect::Tree<2, Point> tree;
        // size_t line_idx;
        // Point hit_point;
        // Point::Scalar distance = AABBTreeLines::squared_distance_to_indexed_lines(lines, tree, point, line_idx, hit_point);

        Domain::ExPolygons expolygons = Biz::Algorithms::ClipperUtils::union_ex(polygons);
        Domain::Line2ds linesf     = Biz::Algorithms::ExPolygon::to_linesf(expolygons);
        Biz::Algorithms::AABBTreeIndirect::Tree2d tree = Biz::Algorithms::AABBTreeLines::build_aabb_tree_over_indexed_lines(linesf);

        size_t line_idx = 0;
        Domain::Vec2d hit_point;
        // double distance =
        Biz::Algorithms::AABBTreeLines::squared_distance_to_indexed_lines(linesf, tree, zero, line_idx, hit_point);

        // conversion between index of point and expolygon
        Domain::ExPolygonsIndices cvt(expolygons);
        Domain::ExPolygonsIndex index = cvt.cvt(static_cast<uint32_t>(line_idx));

        const Domain::Polygon& polygon = index.is_contour() ?
            expolygons[index.expolygons_index].contour :
            expolygons[index.expolygons_index].holes[index.hole_index()];

        Domain::Point hit_point_int = hit_point.cast<Domain::coord_t>();
        Biz::Emboss::TextLine tl{polygon, Biz::Emboss::PolygonPoint{index.point_index, hit_point_int}};
        result.emplace_back(tl);
    }
    return result;
}

inline Eigen::AngleAxis<double> get_rotation()
{
    return Eigen::AngleAxis(-M_PI_2, Domain::Vec3d::UnitX());
}

indexed_triangle_set create_its(const Biz::Emboss::TextLines& lines, float radius)
{
    indexed_triangle_set its;
    // create model from polygons
    for (const Biz::Emboss::TextLine& line : lines) {
        const Domain::Polygon& polygon = line.polygon;
        if (polygon.empty())
            continue;
        indexed_triangle_set line_its = its_create_torus(polygon, radius);
        auto transl                   = Eigen::Translation3d(0., line.y, 0.);
        Domain::Transform3d tr        = transl * get_rotation();
        its_transform(line_its, tr);
        Domain::its_merge(its, line_its);
    }
    return its;
}

double calc_line_height_in_mm(const Domain::FontFile& ff, const Domain::FontProp& fp)
{
    int line_height = Biz::Emboss::get_line_height(ff, fp); // In shape size
    double scale    = Biz::Emboss::get_text_shape_scale(fp, ff);
    return line_height * scale;
}
} // namespace

namespace Slic3r::Biz::Emboss {
TextLines create_text_lines(
    const Domain::Transform3d& text_tr,
    const Domain::ModelVolumePtrs& volumes_to_slice,
    const Domain::FontFile& ff,
    const Domain::FontProp& fp,
    unsigned count_lines,
    double* line_height_mm_ptr
)
{
    Domain::FontProp::VerticalAlign align = fp.align.vertical;

    double line_height_mm = calc_line_height_in_mm(ff, fp);
    assert(line_height_mm > 0);
    if (line_height_mm <= 0)
        return {};

    // size_in_mm .. contain volume scale and should be ascent value in mm
    double line_offset       = fp.size_in_mm * ascent_ratio_offset;
    double first_line_center = line_offset + get_align_y_offset_in_mm(align, count_lines, ff, fp);
    std::vector<float> line_centers(count_lines);
    for (size_t i = 0; i < count_lines; ++i)
        line_centers[i] = static_cast<float>(first_line_center - i * line_height_mm);

    // contour transformation
    Domain::Transform3d c_trafo     = text_tr * get_rotation();
    Domain::Transform3d c_trafo_inv = c_trafo.inverse();

    std::vector<Domain::Polygons> line_contours(count_lines);
    for (const Domain::ModelVolume* volume : volumes_to_slice) {
        MeshSlicingParams slicing_params;
        slicing_params.trafo = c_trafo_inv * volume->get_matrix();
        for (size_t i = 0; i < count_lines; ++i) {
            const Domain::Polygons polys =
                Slic3r::slice_mesh(volume->mesh().its, line_centers[i], slicing_params);
            if (polys.empty())
                continue;
            Domain::Polygons& contours = line_contours[i];
            contours.insert(contours.end(), polys.begin(), polys.end());
        }
    }

    // fix for text line out of object
    // When move text close to edge - line center could be out of object
    for (Domain::Polygons& contours : line_contours) {
        if (!contours.empty())
            continue;

        // use line center at zero, there should be some contour.
        float line_center = 0.f;
        for (const Domain::ModelVolume* volume : volumes_to_slice) {
            MeshSlicingParams slicing_params;
            slicing_params.trafo = c_trafo_inv * volume->get_matrix();
            const Domain::Polygons polys =
                Slic3r::slice_mesh(volume->mesh().its, line_center, slicing_params);
            if (polys.empty())
                continue;
            contours.insert(contours.end(), polys.begin(), polys.end());
        }
    }

    TextLines result = select_closest_contour(line_contours);
    assert(result.size() == count_lines);
    assert(line_centers.size() == count_lines);
    // Fill centers
    for (size_t i = 0; i < count_lines; ++i)
        result[i].y = line_centers[i];

    if (line_height_mm_ptr != nullptr)
        *line_height_mm_ptr = line_height_mm;

    return result;
}

TextLinesModel::TextLinesModel(TextPresetManager& preset_manager,
    Biz::ProjectInteractor& project_interactor,
    App::Plater::PlaterScenePresenter& scene_presenter,
    App::Render::Device& device
    ):
    m_preset_manager(preset_manager),
    m_project_interactor(project_interactor),
    m_scene_presenter(scene_presenter),
    m_device(device)
{
    Domain::ColorRGBA gray_color = Domain::ColorRGBA::LIGHT_GRAY();
    gray_color.a(.7f);
    m_material = App::Render::Material{}
        .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", gray_color)
        .set_transparent(true);
}

void TextLinesModel::create_text_lines(unsigned count_lines, const Domain::Transform3d* text_tr)
{
    reset();

    Domain::Transform3d text_tr_data; // text volume transformation for text_tr pointer
    Domain::ModelVolumePtrs volumes_to_slice; // object parts to slice

    App::Scene::Scene& scene = m_scene_presenter.scene();
    App::Scene::Node* object_node = nullptr; // Place to append visualzation
    if (text_tr == nullptr) {
        // volume with text is selected and keep transformation
        const Domain::ModelVolume* text_volume_ptr = get_selected_text_volume(m_project_interactor);
        if (text_volume_ptr == nullptr)
            return;
        text_tr_data = text_volume_ptr->get_matrix(); // copy transformation
        text_tr = &text_tr_data;
        volumes_to_slice = prepare_volumes_to_slice(*text_volume_ptr);

        size_t instance_id = m_project_interactor.scene_interactor()
            .object_selection().elements.front().instance_id;
        size_t object_id = text_volume_ptr->get_object()->id().id;
        object_node = scene.root().query_first([object_id, instance_id](const App::Scene::Node* n){
            const auto* tag = n->tag_of_type<App::Plater::SceneNodeTag>();
            return tag != nullptr && 
                tag->volume_id == 0 &&
                tag->object_id == object_id &&
                tag->instance_id == instance_id;
            });
    } else {
        // before create of the text volume is known only future transformation
        const Domain::Project& project = m_project_interactor.selected_project();
        const Biz::Scene::ObjectSelection& selection =
            m_project_interactor.scene_interactor().object_selection();
        if (selection.elements.empty())
            return; // no object for create slice
        const Domain::ElementRef& el = selection.elements.front();
        const Domain::ModelObject * object = project.find_object_by_id(el.object_id);

        volumes_to_slice = prepare_volumes_to_slice(*object);

        object_node = scene.root().query_first([&el](const App::Scene::Node* n) {
            const auto* tag = n->tag_of_type<App::Plater::SceneNodeTag>();
            return tag != nullptr &&
                tag->volume_id == 0 &&
                tag->object_id == el.object_id &&
                tag->instance_id == el.instance_id;
            });
    }    

    const auto& ffc = m_preset_manager.get_font_file_with_cache();
    assert(ffc.has_value());
    if (!ffc.has_value())
        return;
    const auto& ff_ptr = ffc.font_file;
    assert(ff_ptr != nullptr);
    if (ff_ptr == nullptr)
        return;
    const Domain::FontFile& ff = *ff_ptr;
    const Domain::FontProp& fp = m_preset_manager.get_font_prop();

    double line_height_mm;
    m_lines = Emboss::create_text_lines(
            *text_tr,
            volumes_to_slice,
            ff,
            fp,
            count_lines,
            &line_height_mm);
    if (m_lines.empty())
        return;

    bool is_mirrored = has_reflection(*text_tr);
    float radius = static_cast<float>(line_height_mm / 20.);
    
    // create node and append into scene
    indexed_triangle_set its = create_its(m_lines, radius);

    m_geometry = App::Render::geometry_from_triangle_mesh(m_device, its);

    TextLineNodeTag tag{};
    int layer_index = int(App::Plater::PlaterSceneLayer::DocumentObjects);
    App::Scene::NodeBuilder builder{ scene };
    builder
        .set_debug_name("Text lines")
        .set_transform(*text_tr)
        .set_tag(tag)
        .set_mesh(m_geometry.get(), m_material, layer_index);
    scene.add_child(builder.build().release(), object_node);
}

void TextLinesModel::reset()
{
    if (m_lines.empty())
        return; // already reseted

    m_lines.clear();
    m_geometry.release();

    auto is_text_line = [](const App::Scene::Node* n) {
        return n->has_tag_of_type<TextLineNodeTag>();
    };

    App::Scene::Node::NodeList nodes;
    m_scene_presenter.scene().root().query(is_text_line, nodes);
    // Should by always only one
    for (auto node : nodes) m_scene_presenter.scene().remove_child(node);
}

const Domain::ModelVolume* get_selected_text_volume(const Domain::Project& project, const Biz::Scene::ObjectSelection& selection) {
    if (selection.elements.size() != 1)
        return nullptr; // multiple volumes selected

    const Domain::ElementRef& selected = selection.elements.front();
    const Domain::ModelVolume* volume_ptr = nullptr;
    if (selected.has_volume()) {
        volume_ptr = project.find_volume_by_id(selected.object_id, selected.volume_id);
    } else {
        // Check is selected object contain only volume with text
        const Domain::ModelObject* object_ptr = project.find_object_by_id(selected.object_id);
        if (object_ptr == nullptr)
            return nullptr; // after delete volume
        if (object_ptr->volumes.size() != 1)
            return nullptr;
        volume_ptr = object_ptr->volumes.front();
    }

    if (volume_ptr == nullptr)
        return nullptr;

    if (!volume_ptr->text_configuration.has_value())
        return nullptr; // selected volume is not text

    return volume_ptr;
}

const Domain::ModelVolume* get_selected_text_volume(const Biz::ProjectInteractor& project_interactor) {
    const Domain::Project& project = project_interactor.selected_project();
    const Biz::Scene::ObjectSelection &selection = 
        project_interactor.scene_interactor().object_selection();
    return get_selected_text_volume(project, selection);
}


} // namespace Slic3r::Biz::Emboss
