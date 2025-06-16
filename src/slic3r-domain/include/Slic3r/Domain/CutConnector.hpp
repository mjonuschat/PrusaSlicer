#pragma once

#include "Slic3r/Domain/Types.hpp"

#include <vector>

namespace Slic3r::Domain {

enum class CutConnectorType : int
{
    Plug,
    Dowel,
    Snap,
    Undef
};

enum class CutConnectorStyle : int
{
    Prism,
    Frustum,
    Undef
};

enum class CutConnectorShape : int
{
    Triangle,
    Square,
    Hexagon,
    Circle,
    Undef
};

struct CutConnectorAttributes
{
    CutConnectorType type{CutConnectorType::Plug};
    CutConnectorStyle style{CutConnectorStyle::Prism};
    CutConnectorShape shape{CutConnectorShape::Circle};

    CutConnectorAttributes() = default;

    CutConnectorAttributes(CutConnectorType t, CutConnectorStyle st, CutConnectorShape sh)
        : type(t), style(st), shape(sh)
    {}

    CutConnectorAttributes(const CutConnectorAttributes& rhs)
        : CutConnectorAttributes(rhs.type, rhs.style, rhs.shape)
    {}

    bool operator<(const CutConnectorAttributes& other) const;

    template<class Archive>
    void serialize(Archive& ar);
};

struct CutConnector
{
    Vec3d pos;
    Transform3d rotation_m;
    float radius;
    float height;
    float radius_tolerance;// [0.f : 1.f]
    float height_tolerance;// [0.f : 1.f]
    float z_angle {0.f};
    CutConnectorAttributes attribs;

    CutConnector()
        : pos(Vec3d::Zero()), rotation_m(Transform3d::Identity()), radius(5.f), height(10.f), radius_tolerance(0.f), height_tolerance(0.1f), z_angle(0.f)
    {}

    CutConnector(Vec3d p, Transform3d rot, float r, float h, float rt, float ht, float za, CutConnectorAttributes attributes)
        : pos(p), rotation_m(rot), radius(r), height(h), radius_tolerance(rt), height_tolerance(ht), z_angle(za), attribs(attributes)
    {}

    CutConnector(const CutConnector& rhs) :
        CutConnector(rhs.pos, rhs.rotation_m, rhs.radius, rhs.height, rhs.radius_tolerance, rhs.height_tolerance, rhs.z_angle, rhs.attribs) {}

    template<class Archive>
    void serialize(Archive& ar);
};

using CutConnectors = std::vector<CutConnector>;

class CutId
{
    size_t m_unique_id;      // 0 = invalid
    size_t m_check_sum;      // check sum of CutParts in initial Object
    size_t m_connectors_cnt; // connectors count

public:
    CutId() { invalidate(); }
    CutId(size_t id, size_t check_sum, size_t connectors_cnt)
        : m_unique_id{id}, m_check_sum{check_sum}, m_connectors_cnt{connectors_cnt}
    {}

    bool operator<(const CutId& rhs) const;
    CutId& operator=(const CutId& rhs);

    void invalidate();

    void init();

    bool has_same_id(const CutId& rhs) const;
    bool is_equal(const CutId& rhs) const;

    size_t id() const;
    bool valid() const;
    size_t check_sum() const;
    void increase_check_sum(size_t cnt);

    size_t connectors_cnt() const;
    void increase_connectors_cnt(size_t connectors_cnt);

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(m_unique_id, m_check_sum, m_connectors_cnt);
    }
};

} // namespace Slic3r::Domain
