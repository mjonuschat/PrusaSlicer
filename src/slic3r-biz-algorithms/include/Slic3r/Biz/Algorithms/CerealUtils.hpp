#pragma once

// The point of this header file is to contain all cerealization templates
// which require cereal includes. This would ideally be only included
// in cpp files which actually need to serialize something.

// The usual 'void serialize(Archive&)' template methods can as well stay
// with the respective class so we don't split the object from its serialization
// recipe. Only when the cereal header is needed, it should be moved here.

#include "cereal/cereal.hpp"
#include <cereal/specialize.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/variant.hpp>

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigValue.hpp"
#include "Slic3r/Domain/Expr/ExprAst.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/TriangleMesh.hpp"


namespace cereal {
    template<class Archive> void serialize(Archive& archive, Slic3r::Domain::Vec2d &v) { archive(v.x(), v.y()); }
    template<class Archive> void serialize(Archive& archive, Slic3r::Domain::Vec2crd &v) { archive(v.x(), v.y()); }
    template<class Archive> void serialize(Archive& archive, Slic3r::Domain::SquareMatrix4d &m){ archive(binary_data(m.data(), 4*4*sizeof(double))); }
    template<class Archive, class T, int N> inline void serialize(Archive& archive, Eigen::Transform<T, N, Eigen::Affine, Eigen::DontAlign>& t){ archive(t.matrix()); }

    template<class Archive> void serialize(Archive& ar, Slic3r::Domain::ConfigBox& box)
    {
        using namespace Slic3r::Domain;

        auto archive_item = [&ar](ConfigItem& item) {
            item.visit(overloaded(
                [&ar](EnumWrapper& ew) {
                    int val = ew.value();
                    ar(val);
                    if constexpr (Archive::is_loading::value)
                        ew.set_index(ew.index_of_value(val));
                },
                [&ar](EnumVectorWrapper& evw) {
                    auto vals = evw.get_indexes();
                    ar(vals);
                    if constexpr (Archive::is_loading::value)
                        evw.set_indexes(vals);
                },
                [&ar](auto& sth_else) {
                    ar(sth_else);
                }
            ));
        };

        for (ConfigItem& item : box.items.all_items())
            archive_item(item);
        for (ConfigItem& item : box.overrides.all_items())
            archive_item(item);
        {
            // Now load/save the active overrides.
            std::vector<std::string> overridden_names;
            if (Archive::is_saving::value) {
                const auto& overridden_items = box.overrides.overriden_items();
                for (const ConfigItem& item : overridden_items)
                    overridden_names.emplace_back(item.name());
            }
            ar(overridden_names);
            if (Archive::is_loading::value) {
                for (const std::string& name : overridden_names)
                    box.overrides.enable(name);
            }
        }
    }

    template <class Archive>
    struct specialize<Archive, Slic3r::Domain::Expr::ExprAst, cereal::specialization::non_member_serialize> {};

    // Custom non-member serialize for ExprAst
    template <class Archive>
    void serialize(Archive& ar, Slic3r::Domain::Expr::ExprAst& variant)
    {
        if constexpr (Archive::is_saving::value) {
            ar(variant.which());
            boost::apply_visitor([&](const auto& value) {
                ar(value);
            }, variant);
        } else {
            int which;
            ar(which);
            switch (which)
            {
                case 0: { bool t; ar(t); variant = t; break; }
                case 1: { double t; ar(t); variant = t; break; }
                case 2: { std::string t; ar(t); variant = t; break; }
                case 3: { Slic3r::Domain::Expr::RegEx t; ar(t); variant = t; break; }
                case 4: { Slic3r::Domain::Expr::Binary t; ar(t); variant = t; break; }
                case 5: { Slic3r::Domain::Expr::Unary t; ar(t); variant = t; break; }
                case 6: { Slic3r::Domain::Expr::FuncCall t; ar(t); variant = t; break; }
                case 7: { Slic3r::Domain::Expr::VarRef t; ar(t); variant = t; break; }
                default: PANIC("Invalid type index for ExprAst variant");
            }
        }
    }

    template <class Archive>
    struct specialize<Archive, Slic3r::Domain::Preset::RootPresetNode, cereal::specialization::non_member_serialize> {};

    template<class Archive> void serialize(Archive& archive, Slic3r::Domain::Preset::RootPresetNode& node)
    {
        archive(cereal::base_class<Slic3r::Domain::Preset::PresetNode>(&node), node.kind);
    }

    template <class Archive> void serialize(Archive & ar, const std::nullptr_t &) {
        // We don't need to read any data; the object is already a nullptr.
        // We don't need to write any data to represent a null value.
    }



    template <class Archive>
    struct specialize<Archive, Slic3r::Domain::TriangleMesh, cereal::specialization::non_member_load_save>
    {};

    template <class Archive>
    void load(Archive& archive, Slic3r::Domain::TriangleMesh& mesh)
    {
        archive.loadBinary(
            reinterpret_cast<char*>(const_cast<Slic3r::Domain::TriangleMeshStats*>(&mesh.stats())),
            sizeof(Slic3r::Domain::TriangleMeshStats)
        );
        archive(mesh.its.indices, mesh.its.vertices);
    }

    template <class Archive>
    void save(Archive& archive, const Slic3r::Domain::TriangleMesh& mesh)
    {
        archive.saveBinary(
            reinterpret_cast<const char*>(&mesh.stats()),
            sizeof(Slic3r::Domain::TriangleMeshStats)
        );
        archive(mesh.its.indices, mesh.its.vertices);
    }
}

