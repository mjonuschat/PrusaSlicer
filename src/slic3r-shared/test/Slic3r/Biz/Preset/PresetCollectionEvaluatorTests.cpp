#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Preset/PresetCollectionEvaluator.hpp"
#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"

using namespace Slic3r::Domain::Preset;
using namespace Slic3r::Biz::Preset;
using namespace Slic3r::Biz;



void collect_named_presets(NamedPresets& named_presets, PresetNodePath path) {
    const auto& node = *path.back();
    if (!node.id.empty())
        named_presets[node.id] = path;

    for (const auto& child : node.variants) {
        PresetNodePath child_path = path;
        child_path.push_back(&child);
        collect_named_presets(named_presets, child_path);
    }
}

PresetCollectionEvaluator create_evaluator(const IO::PresetLoader& loader, PresetKind kind)
{
    static NamedPresets named_presets;

    named_presets.clear();

    const auto& preset_nodes = loader.presets().find(kind)->second;
    for (const auto& p : preset_nodes)
        collect_named_presets(named_presets, {&p});

    return PresetCollectionEvaluator{preset_nodes, named_presets, {}, {}};
}


TEST_CASE("Preset Collection Evaluator")
{
    SECTION("simplest")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
features:
  f1: 321
)";

        IO::PresetLoader loader;
        loader.load_from_string(yaml);
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        auto evals = eval.eval_preset({}, false);
        REQUIRE(evals.size() == 1);
        const auto& p = evals.front();
        REQUIRE(p.id == "*common*");
        REQUIRE(p.conditions.empty() == true);
        REQUIRE(p.values.size() == 3);
        REQUIRE(std::get<double>(p.values.find("a")->second) == 1);
        REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
        REQUIRE(std::get<bool>(p.values.find("c")->second) == true);
        REQUIRE(std::get<double>(p.features.find("f1")->second) == 321);
    }

    SECTION("simple variants")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
features:
  f1: 1
variants:
  - condition: 'tool.nozzle_high_flow'
    features:
      f2: 1
    values:
      c: false
      d: 1
  - values:
      d: 2
    features:
      f2: 2
)";

        IO::PresetLoader loader;
        loader.load_from_string(yaml);
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        for (const auto& [
            nozzle_high_flow, expected_c_value, expected_d_value, expected_condition_count
        ] : {
            std::make_tuple(true, false, 1, 1), std::make_tuple(false, true, 2, 0)
        }) {
            Expr::ValueMap overrides;
            overrides["tool.nozzle_high_flow"] = nozzle_high_flow;
            auto evals = eval.eval_preset({overrides}, false);
            REQUIRE(evals.size() == 1);
            const auto& p = evals.front();
            REQUIRE(p.id == "*common*");
            REQUIRE(p.conditions.size() == expected_condition_count);
            REQUIRE(std::get<double>(p.values.find("a")->second) == 1);
            REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
            REQUIRE(std::get<bool>(p.values.find("c")->second) == expected_c_value);
            REQUIRE(std::get<double>(p.values.find("d")->second) == expected_d_value);
            REQUIRE(std::get<double>(p.features.find("f1")->second) == 1);
            REQUIRE(std::get<double>(p.features.find("f2")->second) == expected_d_value);
        }
    }

    SECTION("simple multiple variants")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
variants:
  - name: 'Var 1'
    values:
      c: false
      d: 1
  - name: 'Var 2'
    values:
      d: 2
)";

        IO::PresetLoader loader;
        loader.load_from_string(yaml);
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        auto evals = eval.eval_preset({}, false);
        REQUIRE(evals.size() == 2);

        for (const auto& [
            index, expected_name, expected_c_value, expected_d_value
        ] : {
            std::make_tuple(0, "Var 1", false, 1), std::make_tuple(1, "Var 2", true, 2)
        }) {

            const auto& p = evals[index];
            REQUIRE(p.id == "*common*");
            REQUIRE(p.name == expected_name);
            REQUIRE(p.conditions.empty() == true);
            REQUIRE(std::get<double>(p.values.find("a")->second) == 1);
            REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
            REQUIRE(std::get<bool>(p.values.find("c")->second) == expected_c_value);
            REQUIRE(std::get<double>(p.values.find("d")->second) == expected_d_value);
        }
    }

    SECTION("simple inherits")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
features:
  f1: 1
  f2: 1
---
kind: printer
id: 'Printer'
name: 'Printer'
inherits:
  - '*common*'
values:
  c: false
  d: "y"
features:
  f2: 2
)";

        IO::PresetLoader loader;
        try {
            loader.load_from_string(yaml);
        }
        catch (Yaml::ParseError& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        }
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        auto evals = eval.eval_preset({}, false);
        REQUIRE(evals.size() == 2);
        auto it = std::find_if(evals.begin(), evals.end(), [](const auto& p){ return p.name == "Printer"; });
        REQUIRE(it != evals.end());
        const auto& p = *it;
        REQUIRE(p.id == "Printer");
        REQUIRE(p.conditions.empty() == true);
        REQUIRE(p.values.size() == 4);
        REQUIRE(std::get<double>(p.values.find("a")->second) == 1);
        REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
        REQUIRE(std::get<bool>(p.values.find("c")->second) == false);
        REQUIRE(std::get<std::string>(p.values.find("d")->second) == "y");
        REQUIRE(std::get<double>(p.features.find("f1")->second) == 1);
        REQUIRE(std::get<double>(p.features.find("f2")->second) == 2);
    }

    SECTION("simple inherits product")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
variants:
  - name: '*common*@A'
    values:
      d: 1
    features:
      f1: 1
  - name: '*common*@B'
    values:
      d: 2
    features:
      f1: 2
---
kind: printer
id: 'Printer'
name: 'Printer'
inherits:
  - '*common*'
values:
  c: false
  e: 42
variants:
  - name: 'PrinterA'
    values:
      f: 1
    features:
      f2: 1
  - name: 'PrinterB'
    values:
      f: 2
    features:
      f2: 2
)";

        IO::PresetLoader loader;
        try {
            loader.load_from_string(yaml);
        }
        catch (Yaml::ParseError& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        }
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        auto evals = eval.eval_preset({}, false);
        REQUIRE(evals.size() == 4 + 2);
        for (const auto& [name, expected_d, expected_f] : {
            std::make_tuple("PrinterA@A", 1, 1),
            std::make_tuple("PrinterA@B", 2, 1),
            std::make_tuple("PrinterB@A", 1, 2),
            std::make_tuple("PrinterB@B", 2, 2),
        }) {
            auto it = std::find_if(evals.begin(), evals.end(), [&name](const auto& p){ return p.name == name; });
            REQUIRE(it != evals.end());
            const auto& p = *it;
            REQUIRE(p.id == "Printer");
            REQUIRE(p.conditions.empty() == true);
            REQUIRE(p.values.size() == 6);
            REQUIRE(std::get<double>(p.values.find("a")->second) == 1);
            REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
            REQUIRE(std::get<bool>(p.values.find("c")->second) == false);
            REQUIRE(std::get<double>(p.values.find("d")->second) == expected_d);
            REQUIRE(std::get<double>(p.values.find("e")->second) == 42);
            REQUIRE(std::get<double>(p.values.find("f")->second) == expected_f);

            REQUIRE(std::get<double>(p.features.find("f1")->second) == expected_d);
            REQUIRE(std::get<double>(p.features.find("f2")->second) == expected_f);
        }
    }

    SECTION("simple unconditional inherits")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
features:
  f1: 1
variants:
  - id: '*common*@A'
    values:
      d: 1
    features:
      f2: 1
    variants:
      - id: '*common*@A1'
        values:
          g: 11
  - id: '*common*@B'
    features:
      f2: 2
    values:
      d: 2
---
kind: printer
id: 'Printer'
name: 'Printer'
unconditional_inherits:
  - '*common*@A'
values:
  c: false
  e: 42
)";

        IO::PresetLoader loader;
        try {
            loader.load_from_string(yaml);
        }
        catch (Yaml::ParseError& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        }
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        auto evals = eval.eval_preset({}, false);
        REQUIRE(evals.size() == 2 + 1);
        for (const auto& [name, expected_d] : {
            std::make_tuple("Printer", 1),
        }) {
            auto it = std::find_if(evals.begin(), evals.end(), [&name](const auto& p){ return p.name == name; });
            REQUIRE(it != evals.end());
            const auto& p = *it;
            REQUIRE(p.id == "Printer");
            REQUIRE(p.conditions.size() == 0);
            REQUIRE(p.values.size() == 5);
            REQUIRE(std::get<double>(p.values.find("a")->second) == 1);
            REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
            REQUIRE(std::get<bool>(p.values.find("c")->second) == false);
            REQUIRE(std::get<double>(p.values.find("d")->second) == expected_d);
            REQUIRE(std::get<double>(p.values.find("e")->second) == 42);

            REQUIRE(std::get<double>(p.features.find("f1")->second) == 1);
            REQUIRE(std::get<double>(p.features.find("f2")->second) == expected_d);
        }
    }
    SECTION("product inherits")
    {
        const char* yaml = R"(
kind: printer
id: '*common*'
values:
  a: 1
  b: "x"
  c: true
features:
  f3: 1
variants:
  - id: '*common*@A'
    features:
      f1: 1
    values:
      d: 1
  - id: '*common*@B'
    features:
      f1: 2
    values:
      d: 2
---
kind: printer
id: '*commonPrusa*'
values:
  a: 2
variants:
  - id: '*commonPrusa*@A'
    values:
      f: 1
    features:
      f2: 1
  - id: '*commonPrusa*@B'
    values:
      f: 2
    features:
      f2: 2
---
kind: printer
id: 'Printer'
name: 'Printer'
inherits:
  - '*common*'
  - '*commonPrusa*'
values:
  c: false
  e: 42
features:
  f3: 2
)";

        IO::PresetLoader loader;
        try {
            loader.load_from_string(yaml);
        }
        catch (Yaml::ParseError& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        }
        auto eval = create_evaluator(loader, PresetKind::FdmPrinter);

        auto evals = eval.eval_preset({}, false);
        REQUIRE(evals.size() == 2 + 2 + 2 * 2);
        for (const auto& [name, expected_d, expected_f] : {
            std::make_tuple("Printer@A@A", 1, 1),
            std::make_tuple("Printer@B@A", 1, 2),
            std::make_tuple("Printer@A@B", 2, 1),
            std::make_tuple("Printer@B@B", 2, 2),
        }) {
            INFO("Preset: " << name);
            auto it = std::find_if(evals.begin(), evals.end(), [&name](const auto& p){ return p.id == name; });
            REQUIRE(it != evals.end());
            const auto& p = *it;
            REQUIRE(p.id == name);
            REQUIRE(p.conditions.size() == 0);
            REQUIRE(p.values.size() == 6);
            REQUIRE(std::get<double>(p.values.find("a")->second) == 2);
            REQUIRE(std::get<std::string>(p.values.find("b")->second) == "x");
            REQUIRE(std::get<bool>(p.values.find("c")->second) == false);
            REQUIRE(std::get<double>(p.values.find("d")->second) == expected_d);
            REQUIRE(std::get<double>(p.values.find("e")->second) == 42);
            REQUIRE(std::get<double>(p.values.find("f")->second) == expected_f);

            REQUIRE(std::get<double>(p.features.find("f1")->second) == expected_d);
            REQUIRE(std::get<double>(p.features.find("f2")->second) == expected_f);
            REQUIRE(std::get<double>(p.features.find("f3")->second) == 2);
        }
    }
}
