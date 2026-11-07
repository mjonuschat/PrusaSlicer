// src/slic3r-domain/src/Slic3r/Domain/boss/StructuredFuzzySkinConfig.cpp
#include "boss/features/structured-fuzzy-skin/StructuredFuzzySkinFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"
#include "boss/foundation/BossL.hpp"

namespace Slic3r::Boss {

void StructuredFuzzySkinFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;
    using Slic3r::Domain::Boss::FuzzySkinNoiseType;

    ConfigItemDef* def = defs.add("fuzzy_skin_noise_type", typeid(EnumWrapper));
    def->location = FDMConfigLocation::Print;
    def->overrides_in = std::set<ConfigLocation>{FDMConfigLocation::Object, FDMConfigLocation::Volume};
    def->label = BossL("Noise type");
    def->category = ConfigItemDef::Category::Print_WallsPerimeters;
    def->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_FuzzySkin;
    def->order = 3;
    def->gui_type = ConfigItemDef::GUIType::combobox;
    def->tooltip = BossL("Type of noise used for fuzzy skin displacement. "
                         "\"Classic\" uses uniform random noise (original behavior). "
                         "Structured noise types produce coherent textures that flow vertically across layers.");
    def->init_fn = init_with(
        FuzzySkinNoiseType::Classic,
        {{int(FuzzySkinNoiseType::Classic),     "classic",     BossL("Classic")},
         {int(FuzzySkinNoiseType::Perlin),      "perlin",      BossL("Perlin")},
         {int(FuzzySkinNoiseType::Billow),      "billow",      BossL("Billow")},
         {int(FuzzySkinNoiseType::RidgedMulti), "ridgedmulti", BossL("Ridged Multifractal")},
         {int(FuzzySkinNoiseType::Voronoi),     "voronoi",     BossL("Voronoi")}}
    );

    def = defs.add("fuzzy_skin_feature_size", typeid(double));
    def->location = FDMConfigLocation::Print;
    def->overrides_in = std::set<ConfigLocation>{FDMConfigLocation::Object, FDMConfigLocation::Volume};
    def->label = BossL("Feature size");
    def->category = ConfigItemDef::Category::Print_WallsPerimeters;
    def->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_FuzzySkin;
    def->order = 4;
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->tooltip = BossL("The base size of coherent noise features. "
                         "Higher values produce larger, more spread out features.");
    def->units = {BossL("mm")};
    def->min = 0.1;
    def->max = 500;
    def->init_fn = init_with(1.0);

    def = defs.add("fuzzy_skin_octaves", typeid(int));
    def->location = FDMConfigLocation::Print;
    def->overrides_in = std::set<ConfigLocation>{FDMConfigLocation::Object, FDMConfigLocation::Volume};
    def->label = BossL("Noise octaves");
    def->category = ConfigItemDef::Category::Print_WallsPerimeters;
    def->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_FuzzySkin;
    def->order = 5;
    def->gui_type = ConfigItemDef::GUIType::spinbox;
    def->tooltip = BossL("Number of octaves of noise to layer together. "
                         "Higher values add finer detail but increase computation.");
    def->min = 1;
    def->max = 10;
    def->init_fn = init_with(4);

    def = defs.add("fuzzy_skin_persistence", typeid(double));
    def->location = FDMConfigLocation::Print;
    def->overrides_in = std::set<ConfigLocation>{FDMConfigLocation::Object, FDMConfigLocation::Volume};
    def->label = BossL("Noise persistence");
    def->category = ConfigItemDef::Category::Print_WallsPerimeters;
    def->option_group = ConfigItemDef::OptionGroup::Print_WallsPerimeters_FuzzySkin;
    def->order = 6;
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->tooltip = BossL("Controls how much each successive octave contributes. "
                         "Lower values produce smoother noise, higher values add more fine detail.");
    def->min = 0.01;
    def->max = 1;
    def->init_fn = init_with(0.5);
}

} // namespace Slic3r::Boss
