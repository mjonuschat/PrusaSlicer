"""Discovers boss-feature.json manifests and generates the C++ composition
headers and per-target source lists that wire BOSS features into the build.

Dependency-free by design (stdlib only) -- see the spec's "Why generation,
not a hand-maintained composition list" for why this exists as a generator
at all, and why it deliberately avoids adding jsonschema or any other
third-party package as a build dependency.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REQUIRED_MANIFEST_KEYS = {"name", "id", "key", "trait", "header", "components"}
OPTIONAL_MANIFEST_KEYS = {"vendored", "label", "config_options"}
NAME_PATTERN_CHARS = set("abcdefghijklmnopqrstuvwxyz0123456789-")
# Config option keys are snake_case, unlike the kebab-case feature name/key.
CONFIG_OPTION_KEY_CHARS = set("abcdefghijklmnopqrstuvwxyz0123456789_")

# The steps propagate() handles in StepsInvalidation.cpp. An `invalidates`
# value outside this set emits propagate(<x>) that either fails to compile or
# hits propagate()'s `default: PANIC`. Keep equal to propagate()'s cases.
STEP_NAMES = {
    "psWipeTower", "psAlertWhenSupportsNeeded", "psSkirtBrim", "psGCodeExport",
    "posSlice", "posPerimeters", "posPrepareInfill", "posInfill", "posIroning",
    "posSupportSpotsSearch", "posSupportMaterial", "posEstimateCurledExtrusions",
    "posCalculateOverhangingPerimeters",
}
CPP_IDENTIFIER = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
# A lexically valid identifier can still be a reserved word -- "class" or
# "namespace" as a trait basename or derived enum member name would parse
# as a keyword, not a name, and fail to compile. Reject the C++20 keyword
# set (https://en.cppreference.com/w/cpp/keyword) wherever an identifier is
# derived from manifest/trait data, not just checked for lexical shape.
CPP_KEYWORDS = {
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel",
    "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool",
    "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
    "class", "co_await", "co_return", "co_yield", "compl", "concept",
    "const", "consteval", "constexpr", "constinit", "const_cast",
    "continue", "decltype", "default", "delete", "do", "double",
    "dynamic_cast", "else", "enum", "explicit", "export", "extern",
    "false", "float", "for", "friend", "goto", "if", "inline", "int",
    "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
    "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
    "reflexpr", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast",
    "struct", "switch", "synchronized", "template", "this", "thread_local",
    "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
    "xor", "xor_eq",
}


def is_valid_cpp_identifier(name: str) -> bool:
    return bool(CPP_IDENTIFIER.match(name)) and name not in CPP_KEYWORDS


PATH_SAFE_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_./-")


def is_safe_relative_path(candidate: str) -> bool:
    # Every character is restricted to a small portable-path set (letters,
    # digits, '_', '.', '/', '-') rather than trying to enumerate and
    # escape every character that's unsafe in some generated context. This
    # value is embedded verbatim into a C++ string literal (#include
    # "...") and a CMake quoted argument with no escaping -- a backslash
    # would form an unintended C++ escape sequence, a semicolon would
    # split a CMake list argument, a quote would close the literal early,
    # and so on. Rejecting everything outside this set up front is simpler
    # and more robust than chasing each generated context's own quoting
    # rules one metacharacter at a time.
    if set(candidate) - PATH_SAFE_CHARS:
        return False
    # Reject anything that could escape the repo tree once concatenated
    # with a base directory: a leading '/' (POSIX-absolute), a drive
    # letter (Windows-absolute), or any '..' path component. Existence of
    # the file itself is deliberately not checked here -- target_sources()
    # accepts a nonexistent path without complaint at configure time, but
    # the subsequent build step fails loudly on it (a missing source file
    # is a compiler/linker error, not a silent no-op), which is an
    # adequate backstop for a mistake this check doesn't catch, without
    # duplicating filesystem access inside the generator's pure validation
    # logic.
    if candidate.startswith("/") or re.match(r"^[A-Za-z]:", candidate):
        return False
    return ".." not in candidate.split("/")


class ManifestError(Exception):
    """A manifest failed validation or conflicts with another manifest."""


@dataclass
class Manifest:
    path: Path
    name: str
    id: int
    key: str
    trait: str
    header: str
    label: str
    components: dict[str, dict]
    vendored: list[dict] = field(default_factory=list)
    config_options: list[dict] = field(default_factory=list)


def validate_manifest(path: Path, data: dict) -> Manifest:
    if not isinstance(data, dict):
        raise ManifestError(f"{path}: manifest must be a JSON object")

    missing = REQUIRED_MANIFEST_KEYS - data.keys()
    if missing:
        raise ManifestError(f"{path}: missing required key(s): {sorted(missing)}")

    unknown = data.keys() - REQUIRED_MANIFEST_KEYS - OPTIONAL_MANIFEST_KEYS
    if unknown:
        raise ManifestError(f"{path}: unknown key(s): {sorted(unknown)}")

    name = data["name"]
    if not isinstance(name, str) or not name or set(name) - NAME_PATTERN_CHARS:
        raise ManifestError(
            f"{path}: 'name' must be a lowercase kebab-case string, got {name!r}"
        )

    feature_id = data["id"]
    # 0 is reserved for the generated "None" choice in every enum-backed
    # BOSS option (see BossFillRegistry::register_config in Task 3) -- a
    # feature claiming it would silently collide with that sentinel.
    # The upper bound matches int32_t, the underlying type the generator
    # emits FillPatternKey as (see emit_fill_pattern_key) -- an id outside
    # that range would truncate or fail to compile as a generated enumerator.
    if (
        not isinstance(feature_id, int) or isinstance(feature_id, bool)
        or not (0 < feature_id <= 2**31 - 1)
    ):
        raise ManifestError(
            f"{path}: 'id' must be a positive integer no larger than 2^31-1, got {feature_id!r}"
        )

    key = data["key"]
    # Restricted to the same safe character set as 'name' (not just
    # "non-empty") because the generator embeds this value directly inside
    # a C++ string literal (std::string_view("...")) with no escaping --
    # a quote or backslash here would produce invalid generated C++.
    if not isinstance(key, str) or not key or set(key) - NAME_PATTERN_CHARS:
        raise ManifestError(
            f"{path}: 'key' must be a lowercase kebab-case string, got {key!r}"
        )
    # "none" is the serialized string BossFillRegistry::register_config()
    # (Task 3) always reserves for its own {0, "none", "None"} choice. A
    # fill feature also claiming "none" wouldn't just collide at the id
    # level (already rejected -- id 0 is reserved) -- EnumWrapper::
    # set_string() (ConfigValue.cpp) does a first-match linear search over
    # the choice list, and the registry's own "none" entry is always
    # first, so the feature's serialized identity would become permanently
    # unreachable through the string boundary even though its id is
    # distinct. Reject it here, not just at id 0.
    if key == "none":
        raise ManifestError(f"{path}: 'key' cannot be 'none' -- that string is reserved by the registry")

    trait = data["trait"]
    if not isinstance(trait, str) or "::" not in trait:
        raise ManifestError(
            f"{path}: 'trait' must be a fully qualified C++ type, got {trait!r}"
        )
    # Every "::"-separated segment must be a valid identifier, not just the
    # final one -- "Slic3r::Boss::class::Feature" would otherwise pass with
    # only the basename checked, and "class" as a namespace segment is just
    # as invalid C++ as it would be as the type name itself.
    for segment in trait.split("::"):
        if not is_valid_cpp_identifier(segment):
            raise ManifestError(
                f"{path}: 'trait' segment {segment!r} (in {trait!r}) is not a "
                f"valid, non-keyword C++ identifier"
            )

    header = data["header"]
    if not isinstance(header, str) or not header or not is_safe_relative_path(header):
        raise ManifestError(
            f"{path}: 'header' must be a non-empty, repo-relative path with no leading "
            f"'/' and no '..' component, got {header!r}"
        )

    label = data.get("label", name)
    if not isinstance(label, str) or not label:
        raise ManifestError(f"{path}: 'label' must be a non-empty string if present")

    components = data["components"]
    if not isinstance(components, dict) or not components:
        raise ManifestError(f"{path}: 'components' must be a non-empty object")
    for target, spec in components.items():
        # Restricted to the same safe character set as 'name' and 'key'
        # (not just "non-empty") because emit_sources_cmake() turns this
        # value into a directory name under the output tree and into a
        # CMake variable name -- a '/' or '..' here would write outside
        # that tree.
        if not isinstance(target, str) or not target or set(target) - NAME_PATTERN_CHARS:
            raise ManifestError(
                f"{path}: component target names must be lowercase kebab-case strings, got {target!r}"
            )
        if not isinstance(spec, dict):
            raise ManifestError(f"{path}: components.{target} must be an object")
        unknown_component_keys = spec.keys() - {"capabilities", "sources"}
        if unknown_component_keys:
            raise ManifestError(
                f"{path}: components.{target} has unknown key(s): {sorted(unknown_component_keys)}"
            )
        # 'capabilities' is optional -- a component that only contributes
        # plain source files (no registry dispatch) declares 'sources' with
        # no capabilities at all, rather than inventing a placeholder
        # capability or hand-editing a target's own CMakeLists.txt. At
        # least one of the two must be non-empty, or the component
        # declares nothing for the generator to do.
        capabilities = spec.get("capabilities", [])
        if not isinstance(capabilities, list):
            raise ManifestError(
                f"{path}: components.{target}.capabilities must be a list if present"
            )
        for cap in capabilities:
            if not isinstance(cap, str) or not cap:
                raise ManifestError(
                    f"{path}: components.{target}.capabilities entries must be non-empty strings"
                )
        sources = spec.get("sources", [])
        if not isinstance(sources, list):
            raise ManifestError(f"{path}: components.{target}.sources must be a list if present")
        for src in sources:
            if not isinstance(src, str) or not src or not is_safe_relative_path(src):
                raise ManifestError(
                    f"{path}: components.{target}.sources entries must be non-empty, "
                    f"repo-relative paths with no leading '/' and no '..' component, got {src!r}"
                )
        if not capabilities and not sources:
            raise ManifestError(
                f"{path}: components.{target} must declare a non-empty 'capabilities' list, "
                f"a non-empty 'sources' list, or both"
            )

    vendored = data.get("vendored", [])
    if not isinstance(vendored, list):
        raise ManifestError(f"{path}: 'vendored' must be a list if present")
    for entry in vendored:
        if not isinstance(entry, dict):
            raise ManifestError(f"{path}: each 'vendored' entry must be an object")
        unknown_vendored_keys = entry.keys() - {"name", "path", "kind"}
        if unknown_vendored_keys:
            raise ManifestError(
                f"{path}: vendored entry {entry!r} has unknown key(s): {sorted(unknown_vendored_keys)}"
            )
        for field_name in ("name", "path", "kind"):
            if not isinstance(entry.get(field_name), str) or not entry[field_name]:
                raise ManifestError(
                    f"{path}: vendored entry {entry!r} must have a non-empty string {field_name!r}"
                )
        if not is_safe_relative_path(entry["path"]):
            raise ManifestError(
                f"{path}: vendored entry 'path' {entry['path']!r} must be a repo-relative "
                f"path with no leading '/' and no '..' component"
            )

    config_options = data.get("config_options", [])
    if not isinstance(config_options, list):
        raise ManifestError(f"{path}: 'config_options' must be a list if present")
    for entry in config_options:
        if not isinstance(entry, dict):
            raise ManifestError(f"{path}: each 'config_options' entry must be an object")
        if entry.keys() - {"key", "invalidates", "store"}:
            raise ManifestError(
                f"{path}: config_options entry has unknown key(s): "
                f"{sorted(entry.keys() - {'key', 'invalidates', 'store'})}"
            )
        opt_key = entry.get("key")
        if not isinstance(opt_key, str) or not opt_key or set(opt_key) - CONFIG_OPTION_KEY_CHARS:
            raise ManifestError(
                f"{path}: config_options 'key' must be a snake_case string, got {opt_key!r}"
            )
        if "invalidates" not in entry:
            raise ManifestError(f"{path}: config_options entry {opt_key!r} must declare 'invalidates'")
        invalidates = entry["invalidates"]
        if not isinstance(invalidates, list):
            raise ManifestError(f"{path}: config_options entry {opt_key!r} 'invalidates' must be a list")
        for step in invalidates:
            if not isinstance(step, str):
                raise ManifestError(
                    f"{path}: config_options entry {opt_key!r} invalidates entry {step!r} "
                    f"is not a string -- must be a step name"
                )
            if step not in STEP_NAMES:
                raise ManifestError(
                    f"{path}: config_options entry {opt_key!r} invalidates unknown step {step!r} "
                    f"-- known steps are {sorted(STEP_NAMES)}"
                )

        store = entry.get("store")
        if store is not None:
            if not isinstance(store, dict):
                raise ManifestError(f"{path}: config_options entry {opt_key!r} 'store' must be an object")
            if store.keys() - {"home", "field", "kind"}:
                raise ManifestError(
                    f"{path}: config_options entry {opt_key!r} store has unknown key(s): "
                    f"{sorted(store.keys() - {'home', 'field', 'kind'})}"
                )
            if store.get("home") not in HOME_REGISTRIES:
                raise ManifestError(
                    f"{path}: config_options entry {opt_key!r} store.home {store.get('home')!r} "
                    f"is unknown -- known homes are {sorted(HOME_REGISTRIES)}"
                )
            if store.get("kind") not in STORAGE_KINDS:
                raise ManifestError(
                    f"{path}: config_options entry {opt_key!r} store.kind {store.get('kind')!r} "
                    f"is unknown -- known kinds are {sorted(STORAGE_KINDS)}"
                )
            if not isinstance(store.get("field"), str) or not is_valid_cpp_identifier(store["field"]):
                raise ManifestError(
                    f"{path}: config_options entry {opt_key!r} store.field {store.get('field')!r} "
                    f"is not a valid, non-keyword C++ identifier"
                )

    return Manifest(
        path=path,
        name=name,
        id=feature_id,
        key=key,
        trait=trait,
        header=header,
        label=label,
        components=components,
        vendored=vendored,
        config_options=config_options,
    )


def discover_manifests(features_dir: Path) -> list[Manifest]:
    manifests = []
    for manifest_path in sorted(features_dir.glob("*/boss-feature.json")):
        with manifest_path.open("r", encoding="utf-8") as fh:
            try:
                data = json.load(fh)
            except json.JSONDecodeError as exc:
                raise ManifestError(f"{manifest_path}: invalid JSON: {exc}") from exc
        manifests.append(validate_manifest(manifest_path, data))
    return manifests


def check_global_uniqueness(manifests: list[Manifest]) -> None:
    seen_names: dict[str, Path] = {}
    seen_ids: dict[int, Path] = {}
    seen_keys: dict[str, Path] = {}
    seen_option_keys: dict[str, Path] = {}
    seen_home_fields: dict[tuple[str, str], Path] = {}
    for m in manifests:
        if m.name in seen_names:
            raise ManifestError(
                f"{m.path}: duplicate feature name {m.name!r} (also declared in {seen_names[m.name]})"
            )
        if m.id in seen_ids:
            raise ManifestError(
                f"{m.path}: duplicate feature id {m.id!r} (also declared in {seen_ids[m.id]})"
            )
        if m.key in seen_keys:
            raise ManifestError(
                f"{m.path}: duplicate serialization key {m.key!r} (also declared in {seen_keys[m.key]})"
            )
        for opt in m.config_options:
            if opt["key"] in seen_option_keys:
                raise ManifestError(
                    f"{m.path}: duplicate config option key {opt['key']!r} "
                    f"(also declared in {seen_option_keys[opt['key']]})"
                )
            seen_option_keys[opt["key"]] = m.path
            store = opt.get("store")
            if store:
                home_field = (store["home"], store["field"])
                if home_field in seen_home_fields:
                    raise ManifestError(
                        f"{m.path}: duplicate store field {store['field']!r} in home "
                        f"{store['home']!r} (also declared in {seen_home_fields[home_field]})"
                    )
                seen_home_fields[home_field] = m.path
        seen_names[m.name] = m.path
        seen_ids[m.id] = m.path
        seen_keys[m.key] = m.path


# Known extension families. Adding a ninth (extrusion, seams, ...) in a
# later phase means adding one more entry here -- not touching any
# per-feature file. See the spec's "Generated feature composition".
CAPABILITY_REGISTRIES = {
    "fdm_config": {
        "target": "slic3r-domain",
        "alias": "BossFdmFeatures",
        "template": "Slic3r::Boss::BossConfigRegistry",
        "template_header": "boss/foundation/BossConfigRegistry.hpp",
        "output_header": "BossFdmFeatures.hpp",
    },
    "fill": {
        "target": "libslic3r",
        "alias": "BossFills",
        "template": "Slic3r::Boss::BossFillRegistry",
        "template_header": "boss/foundation/BossFillRegistry.hpp",
        "output_header": "BossFills.hpp",
    },
    "perimeter_policy": {
        "target": "libslic3r",
        "alias": "BossPerimeterPolicies",
        "template": "Slic3r::Boss::BossPerimeterPolicyRegistry",
        "template_header": "boss/foundation/BossPerimeterPolicyRegistry.hpp",
        "output_header": "BossPerimeterPolicies.hpp",
    },
    "label_objects": {
        "target": "libslic3r",
        "alias": "BossLabelObjects",
        "template": "Slic3r::Boss::BossLabelObjectsRegistry",
        "template_header": "boss/foundation/BossLabelObjectsRegistry.hpp",
        "output_header": "BossLabelObjects.hpp",
    },
    "solid_fill_policy": {
        "target": "libslic3r",
        "alias": "BossSolidFillPolicy",
        "template": "Slic3r::Boss::BossSolidFillPolicyRegistry",
        "template_header": "boss/foundation/BossSolidFillPolicyRegistry.hpp",
        "output_header": "BossSolidFillPolicy.hpp",
    },
    "extrusion_registry": {
        "target": "libslic3r",
        "alias": "BossExtrusionFeatures",
        "template": "Slic3r::Boss::BossExtrusionRegistry",
        "template_header": "boss/foundation/BossExtrusionRegistry.hpp",
        "output_header": "BossExtrusionFeatures.hpp",
    },
    "layer_filters": {
        "target": "libslic3r",
        "alias": "BossActiveLayerFilters",
        "template": "Slic3r::Boss::BossLayerFilters",
        "template_header": "boss/foundation/BossLayerFilters.hpp",
        "output_header": "BossActiveLayerFilters.hpp",
    },
}

# Storage homes: BOSS-owned override structs the generator writes and each
# upstream class embeds once. Each entry is a full contract for the home.
HOME_REGISTRIES = {
    "extrude_config": {
        "target": "libslic3r",
        "struct": "BossExtrudeConfigOverrides",
        "output_header": "BossExtrudeConfigOverrides.hpp",
        "includes": ["Slic3r/Domain/Config.hpp", "Slic3r/Domain/ConfigDefsFDM.hpp"],
    },
    "wipe_tower": {
        "target": "libslic3r",
        "struct": "BossWipeTowerOverrides",
        "output_header": "BossWipeTowerOverrides.hpp",
        "includes": ["Slic3r/Domain/Config.hpp"],
    },
}

# Storage kinds model the field's C++ type and how it reads from the config
# view. Raw storage only: never resolves FloatOrPercentage or reduces by context.
STORAGE_KINDS = {
    "scalar_double": {"type": "double", "read": 'config.get<double>("{key}")'},
    "scalar_bool": {"type": "bool", "read": 'config.get<bool>("{key}")'},
    "per_extruder_double": {
        "type": "std::vector<double>",
        "read": 'config.get<std::vector<double>>("{key}")',
    },
    "per_extruder_int": {
        "type": "std::vector<int>",
        "read": 'config.get<std::vector<int>>("{key}")',
    },
    "per_extruder_float_or_percent": {
        "type": "std::vector<Slic3r::Domain::FloatOrPercentage>",
        "read": 'config.get<std::vector<Slic3r::Domain::FloatOrPercentage>>("{key}")',
    },
}

ENUM_MEMBER_SUFFIX = "Feature"

# Keys a foundation registry registers on its own, gated on the same predicate
# as that registration. BossFillRegistry registers boss_fill_pattern only when
# its pack is non-empty (`if constexpr (sizeof...(Features) == 0) return;`,
# BossFillRegistry.hpp), so emit it only when the fill capability has >= 1 feature.
REGISTRY_OWNED_INVALIDATIONS = {
    "fill": {"key": "boss_fill_pattern", "invalidates": ["posPrepareInfill"]},
}


def boss_owned_keys(manifests: list[Manifest]) -> list[str]:
    keys = {opt["key"] for m in manifests for opt in m.config_options}
    for capability, owned in REGISTRY_OWNED_INVALIDATIONS.items():
        if features_for_capability(manifests, capability):
            keys.add(owned["key"])
    return sorted(keys)


def _boss_invalidation_entries(manifests: list[Manifest]) -> list[tuple[str, list[str]]]:
    entries: dict[str, list[str]] = {}
    for m in manifests:
        for opt in m.config_options:
            entries[opt["key"]] = opt["invalidates"]
    for capability, owned in REGISTRY_OWNED_INVALIDATIONS.items():
        if features_for_capability(manifests, capability):
            entries[owned["key"]] = owned["invalidates"]
    return sorted(entries.items())


REGISTRY_OWNED_KEYS = {owned["key"] for owned in REGISTRY_OWNED_INVALIDATIONS.values()}


def check_reserved_keys(manifests: list[Manifest]) -> None:
    # A feature manifest must not declare a config key a foundation registry
    # owns (for example boss_fill_pattern). Otherwise the entry would be
    # silently overwritten by the registry-owned metadata and could collide
    # during registration.
    for m in manifests:
        for opt in m.config_options:
            if opt["key"] in REGISTRY_OWNED_KEYS:
                raise ManifestError(
                    f"{m.path}: config option key {opt['key']!r} is reserved by a foundation "
                    f"registry and cannot be declared by a feature manifest"
                )


def features_for_capability(manifests: list[Manifest], capability: str) -> list[Manifest]:
    matching = [
        m for m in manifests
        if any(capability in spec.get("capabilities", []) for spec in m.components.values())
    ]
    return sorted(matching, key=lambda m: m.name)


def enum_member_name(manifest: Manifest) -> str:
    basename = manifest.trait.rsplit("::", 1)[-1]
    if basename.endswith(ENUM_MEMBER_SUFFIX):
        basename = basename[: -len(ENUM_MEMBER_SUFFIX)]
    return basename


# "None" is the reserved id-0 sentinel every generated FillPatternKey enum
# starts with (see emit_fill_pattern_key below) -- no fill feature's derived
# member name may collide with it, the same way no feature id may be 0.
RESERVED_ENUM_MEMBER_NAMES = {"None"}


def validate_fill_enum_names(fill_features: list[Manifest]) -> None:
    seen: dict[str, Path] = {}
    for feature in fill_features:
        member = enum_member_name(feature)
        if not member or not is_valid_cpp_identifier(member):
            raise ManifestError(
                f"{feature.path}: derived enum member name {member!r} "
                f"(from trait {feature.trait!r}) is not a valid, non-keyword C++ identifier"
            )
        if member in RESERVED_ENUM_MEMBER_NAMES:
            raise ManifestError(
                f"{feature.path}: derived enum member name {member!r} is reserved "
                f"-- rename the feature's trait so it doesn't derive to a reserved name"
            )
        if member in seen:
            raise ManifestError(
                f"{feature.path}: derived enum member name {member!r} collides with "
                f"{seen[member]} -- rename one feature's trait"
            )
        seen[member] = feature.path


def check_known_capabilities(manifests: list[Manifest]) -> None:
    known = set(CAPABILITY_REGISTRIES.keys())
    for m in manifests:
        for target, spec in m.components.items():
            for cap in spec.get("capabilities", []):
                if cap not in known:
                    raise ManifestError(
                        f"{m.path}: components.{target} declares unknown capability "
                        f"{cap!r} -- known capabilities are {sorted(known)}"
                    )


def check_capability_targets(manifests: list[Manifest]) -> None:
    # Each capability is owned by exactly one CMake target (CAPABILITY_
    # REGISTRIES[cap]["target"]) -- its trait's implementation and manifest
    # sources must be declared under that same component key, or the
    # feature would be woven into that target's generated composition
    # while its actual sources link into a different target entirely,
    # producing an undefined-symbol link error rather than a clear
    # generation-time message.
    for m in manifests:
        for target, spec in m.components.items():
            for cap in spec.get("capabilities", []):
                owning_target = CAPABILITY_REGISTRIES[cap]["target"]
                if target != owning_target:
                    raise ManifestError(
                        f"{m.path}: capability {cap!r} is declared under components.{target}, "
                        f"but that capability belongs to component {owning_target!r}"
                    )


def emit_composition_header(capability: str, manifests: list[Manifest], include_dir: Path) -> Path:
    reg = CAPABILITY_REGISTRIES[capability]
    features = features_for_capability(manifests, capability)
    lines = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py",
        "// from the boss-feature.json manifests under src/boss/include/boss/features/.",
        "#pragma once",
        "",
        "#include <string_view>",
        "",
        f'#include "{reg["template_header"]}"',
    ]
    for feature in features:
        lines.append(f'#include "{feature.header}"')
    lines.append("")
    lines.append("namespace Slic3r::Boss {")
    trait_list = ", ".join(f.trait for f in features)
    lines.append(f'using {reg["alias"]} = {reg["template"]}<{trait_list}>;')
    for feature in features:
        lines.append(
            f'static_assert({feature.trait}::id == {feature.id}, '
            f'"manifest/trait id mismatch for {feature.name}");'
        )
        lines.append(
            f'static_assert({feature.trait}::key == std::string_view("{feature.key}"), '
            f'"manifest/trait key mismatch for {feature.name}");'
        )
    lines.append("} // namespace Slic3r::Boss")
    lines.append("")

    header_dir = include_dir / "boss" / "generated"
    header_dir.mkdir(parents=True, exist_ok=True)
    header_path = header_dir / reg["output_header"]
    header_path.write_text("\n".join(lines), encoding="utf-8")
    return header_path


def emit_fill_pattern_key(manifests: list[Manifest], include_dir: Path) -> Path:
    # boss_fill_pattern's enum is generated, not hand-maintained, so a new
    # fill feature adds an enum member by existing (its manifest is
    # discovered) rather than by anyone editing a shared enum definition.
    fill_features = features_for_capability(manifests, "fill")
    validate_fill_enum_names(fill_features)
    lines = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py.",
        "#pragma once",
        "#include <cstdint>",
        "",
        "namespace Slic3r::Domain::Boss {",
        "enum class FillPatternKey : int32_t {",
        "    None = 0,",
    ]
    for feature in fill_features:
        lines.append(f"    {enum_member_name(feature)} = {feature.id},")
    lines.append("};")
    lines.append("} // namespace Slic3r::Domain::Boss")
    lines.append("")

    header_dir = include_dir / "boss" / "generated"
    header_dir.mkdir(parents=True, exist_ok=True)
    header_path = header_dir / "BossFillPatternKey.hpp"
    header_path.write_text("\n".join(lines), encoding="utf-8")
    return header_path


def emit_step_invalidations(manifests: list[Manifest], output_dir: Path) -> Path:
    lines = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py.",
        "#include <map>",
        "#include <string>",
        "#include <vector>",
        '#include "libslic3r/StepsInvalidation.hpp"',
        "",
        "namespace Slic3r::SlicingSync {",
        "std::map<std::string, std::vector<Step>> boss_step_invalidations()",
        "{",
        "    return {",
    ]
    for key, invalidates in _boss_invalidation_entries(manifests):
        propagations = ", ".join(f"propagate({step})" for step in sorted(invalidates))
        lines.append(f'        {{"{key}", steps({{{propagations}}})}},')
    lines += ["    };", "}", "} // namespace Slic3r::SlicingSync", ""]

    target_dir = output_dir / "libslic3r"
    target_dir.mkdir(parents=True, exist_ok=True)
    impl_path = target_dir / "BossStepInvalidations.cpp"
    impl_path.write_text("\n".join(lines), encoding="utf-8")
    return impl_path


def emit_config_option_keys(manifests: list[Manifest], output_dir: Path) -> Path:
    keys = boss_owned_keys(manifests)

    header_dir = output_dir / "include" / "boss" / "generated"
    header_dir.mkdir(parents=True, exist_ok=True)
    header = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py.",
        "#pragma once",
        "#include <set>",
        "#include <string>",
        "",
        "namespace Slic3r::Boss {",
        "const std::set<std::string>& boss_config_option_keys();",
        "} // namespace Slic3r::Boss",
        "",
    ]
    (header_dir / "BossConfigOptionKeys.hpp").write_text("\n".join(header), encoding="utf-8")

    impl = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py.",
        "#include <set>",
        "#include <string>",
        '#include "boss/generated/BossConfigOptionKeys.hpp"',
        "",
        "namespace Slic3r::Boss {",
        "const std::set<std::string>& boss_config_option_keys()",
        "{",
        "    static const std::set<std::string> keys{",
    ]
    for key in keys:
        impl.append(f'        "{key}",')
    impl += ["    };", "    return keys;", "}", "} // namespace Slic3r::Boss", ""]

    target_dir = output_dir / "slic3r-domain"
    target_dir.mkdir(parents=True, exist_ok=True)
    impl_path = target_dir / "BossConfigOptionKeys.cpp"
    impl_path.write_text("\n".join(impl), encoding="utf-8")
    return impl_path


def _fields_for_home(manifests: list[Manifest], home: str) -> list[tuple[str, dict]]:
    fields = []
    for m in manifests:
        for opt in m.config_options:
            store = opt.get("store")
            if store and store["home"] == home:
                fields.append((store["field"], {"key": opt["key"], "kind": store["kind"]}))
    return sorted(fields, key=lambda pair: pair[0])


def emit_override_struct(home: str, manifests: list[Manifest], output_dir: Path) -> Path:
    reg = HOME_REGISTRIES[home]
    fields = _fields_for_home(manifests, home)

    header = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py.",
        "#pragma once",
        "#include <vector>",
    ]
    for inc in reg["includes"]:
        header.append(f'#include "{inc}"')
    header += [
        "",
        "namespace Slic3r::Boss {",
        f'struct {reg["struct"]}',
        "{",
        f'    explicit {reg["struct"]}(const Slic3r::Domain::ConfigView& config);',
        "",
    ]
    for field_name, meta in fields:
        header.append(f'    {STORAGE_KINDS[meta["kind"]]["type"]} {field_name}{{}};')
    header += ["};", "} // namespace Slic3r::Boss", ""]

    header_dir = output_dir / "include" / "boss" / "generated"
    header_dir.mkdir(parents=True, exist_ok=True)
    (header_dir / reg["output_header"]).write_text("\n".join(header), encoding="utf-8")

    impl = [
        "// GENERATED FILE -- do not edit. Produced by cmake/boss/generate_boss_features.py.",
        f'#include "boss/generated/{reg["output_header"]}"',
        "",
        "namespace Slic3r::Boss {",
        f'{reg["struct"]}::{reg["struct"]}(const Slic3r::Domain::ConfigView& config)',
    ]
    if fields:
        init_lines = [
            f'{name}{{{STORAGE_KINDS[meta["kind"]]["read"].format(key=meta["key"])}}}'
            for name, meta in fields
        ]
        impl.append(f"    : {init_lines[0]}")
        impl.extend(f"    , {line}" for line in init_lines[1:])
        impl += ["{", "}", "} // namespace Slic3r::Boss", ""]
    else:
        impl += ["{", "    (void) config;", "}", "} // namespace Slic3r::Boss", ""]

    target_dir = output_dir / reg["target"]
    target_dir.mkdir(parents=True, exist_ok=True)
    impl_path = target_dir / f'{reg["struct"]}.cpp'
    impl_path.write_text("\n".join(impl), encoding="utf-8")
    return impl_path


# Generated translation units each target must compile, relative to output_dir.
# Emitted for every target on every run, so the zero-manifest build still links.
GENERATED_TARGET_SOURCES = {
    "libslic3r": [
        "libslic3r/BossStepInvalidations.cpp",
    ],
    "slic3r-domain": ["slic3r-domain/BossConfigOptionKeys.cpp"],
}
GENERATED_TARGET_SOURCES["libslic3r"] += [
    f'{reg["target"]}/{reg["struct"]}.cpp' for reg in HOME_REGISTRIES.values()
]


def emit_sources_cmake(manifests: list[Manifest], target: str, output_dir: Path) -> Path:
    # Manifests declare sources relative to CMAKE_SOURCE_DIR/src/ (the repo's
    # top-level src/ directory), e.g. "slic3r-domain/src/Slic3r/Domain/boss/
    # ZRotateConfig.cpp" -- so the real, correct concatenation below is
    # ".../src/slic3r-domain/src/Slic3r/Domain/boss/ZRotateConfig.cpp", which
    # matches this repo's actual "src/<component>/src/<Namespace>/..."
    # layout exactly (confirmed: the real file lives at that exact path).
    # This is NOT a doubled "src/" segment -- the component's own directory
    # genuinely nests a second "src/" beneath it, and the manifest's
    # declared path already includes that second segment on purpose.
    sources: list[str] = []
    for manifest in manifests:
        spec = manifest.components.get(target)
        if spec:
            sources.extend(spec.get("sources", []))
    var_name = "BOSS_" + target.upper().replace("-", "_") + "_SOURCES"
    quoted = " ".join(f'"${{CMAKE_SOURCE_DIR}}/src/{src}"' for src in sources)
    content = f"set({var_name} {quoted})\n"

    generated = GENERATED_TARGET_SOURCES.get(target, [])
    gen_var = "BOSS_" + target.upper().replace("-", "_") + "_GENERATED_SOURCES"
    gen_quoted = " ".join(f'"${{BOSS_GENERATED_DIR}}/{rel}"' for rel in generated)
    content += f"set({gen_var} {gen_quoted})\n"

    target_dir = output_dir / target
    target_dir.mkdir(parents=True, exist_ok=True)
    cmake_path = target_dir / "sources.cmake"
    cmake_path.write_text(content, encoding="utf-8")
    return cmake_path


def emit_vendored_cmake(manifests: list[Manifest], target: str, output_dir: Path) -> Path:
    # Guarded by `if (NOT TARGET ...)`: the same vendored entry can appear in
    # more than one target's vendored.cmake (a manifest's components can
    # span several targets), and every one of those files gets include()d in
    # the same CMake run. PRIVATE matches boss_target_sources()'s
    # target_sources(... PRIVATE ...) for this manifest's own sources -- a
    # vendored header is an implementation detail, not a public interface.
    lines: list[str] = []
    for manifest in manifests:
        if target not in manifest.components or not manifest.vendored:
            continue
        feature_dir = manifest.path.parent.name
        for entry in manifest.vendored:
            name, kind, path = entry["name"], entry["kind"], entry["path"]
            lines.append(f"if (NOT TARGET {name})")
            lines.append(f"    add_library({name} {kind})")
            lines.append(
                f'    target_include_directories({name} {kind} '
                f'"${{BOSS_FEATURES_DIR}}/{feature_dir}/{path}")'
            )
            lines.append("endif ()")
            lines.append(f"target_link_libraries({target} PRIVATE {name})")

    target_dir = output_dir / target
    target_dir.mkdir(parents=True, exist_ok=True)
    cmake_path = target_dir / "vendored.cmake"
    content = "\n".join(lines) + "\n" if lines else ""
    cmake_path.write_text(content, encoding="utf-8")
    return cmake_path


def generate(features_dir: Path, output_dir: Path) -> int:
    try:
        manifests = discover_manifests(features_dir)
        check_global_uniqueness(manifests)
        check_known_capabilities(manifests)
        check_capability_targets(manifests)
        check_reserved_keys(manifests)
    except ManifestError as exc:
        print(f"boss feature generation failed: {exc}", file=sys.stderr)
        return 1

    include_dir = output_dir / "include"
    try:
        for capability in CAPABILITY_REGISTRIES:
            emit_composition_header(capability, manifests, include_dir)
        emit_fill_pattern_key(manifests, include_dir)
        emit_step_invalidations(manifests, output_dir)
        emit_config_option_keys(manifests, output_dir)
        for home in HOME_REGISTRIES:
            emit_override_struct(home, manifests, output_dir)
    except ManifestError as exc:
        print(f"boss feature generation failed: {exc}", file=sys.stderr)
        return 1

    # Every component name appearing in any manifest gets its own sources.cmake,
    # even one with no BOSS sources at all (an empty list is still valid CMake).
    targets = {target for m in manifests for target in m.components} | {"slic3r-domain", "libslic3r", "libpgcode"}
    for target in targets:
        emit_sources_cmake(manifests, target, output_dir)
        emit_vendored_cmake(manifests, target, output_dir)

    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--features-dir", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    return generate(args.features_dir, args.output_dir)


if __name__ == "__main__":
    sys.exit(main())
