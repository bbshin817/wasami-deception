"""Dark Deception's Cascade particle systems, rebuilt under /Game/DD from their exported packages (_assets/.../P_*.json)
through UWasamiCascadeLibrary (C++): every emitter, LOD level and module with the values the cook saved.

The cook keeps a module's distributions as their baked lookup tables (FRawDistribution: Table, MinValue, MaxValue)
without the distribution objects, and that table is what the game reads. They are written as they are, with no
distribution object, which UE 5.8 reads the same way (FRawDistributionFloat::GetValue uses the table; the editor only
re-bakes a table from a distribution object). Distributions the cook could not bake (a particle parameter) and the ones
it kept anyway are made as objects with their exported values.

Values are written in UE's text form by the properties' own names. What a package leaves out is at the class defaults
(the export holds what differs from them), except where UE 5.8 reads the old package differently (DETAIL_EPIC)."""
import json
import struct

import unreal

from wasami_tools.pipeline import dd_assets, paths

EAL = unreal.EditorAssetLibrary
LIB = unreal.WasamiCascadeLibrary

# What the builder sets itself (the structure), per exported class kind.
STRUCTURE = {
    "system": ("Emitters",),
    "emitter": ("LODLevels",),
    "lod": ("RequiredModule", "SpawnModule", "Modules"),
}
# Recomputed from where each module is used (UParticleSystem::SetupLODValidity) and compared with the export after.
DERIVED = ("LODValidity",)
# Editor-only data the cook kept a reference to (a Cascade editor's curve list) and UE 4's saved-dirty flag of a
# distribution, which UE 5 does not save (a new distribution is dirty, which only asks the editor to bake it).
NOT_WRITTEN = ("CurveEdSetup", "bIsDirty")

# EParticleDetailMode's bits. UE 5 added Epic (FParticleSystemCustomVersion::AddEpicDetailMode); an emitter saved before
# that gets Epic where it has High when it loads (UParticleEmitter::PostLoad). The original's packages are older, and the
# Epic effects quality runs at r.DetailMode 3 (BaseScalability.ini), where an emitter without the bit would not show.
DETAIL_HIGH = 1 << 2
DETAIL_EPIC = 1 << 3

RAW_DISTRIBUTIONS = ("FRawDistributionFloat", "FRawDistributionVector")
TABLE_DEFAULTS = (("TimeScale", 0.0), ("TimeBias", 0.0), ("Op", 0), ("EntryCount", 0), ("EntryStride", 0),
                  ("SubEntryStride", 0), ("LockFlag", 0))
# FParticleBurst's members and defaults (a spawn module's BurstList; CountLow -1 turns the range off).
BURST_DEFAULTS = (("Count", 0), ("CountLow", -1), ("Time", 0.0))
# FEmitterDynamicParameter's members (a dynamic parameter module's DynamicParams) and the form of each; what an export
# leaves out stays at the struct's defaults.
DYNAMIC_PARAMETER_MEMBERS = (("ParamName", "text"), ("bUseEmitterTime", "bool"), ("bSpawnTimeOnly", "bool"),
                             ("ValueMethod", "text"), ("bScaleVelocityByParamValue", "bool"),
                             ("ParamValue", "distribution"))


def _number(value):
    if isinstance(value, bool):
        raise TypeError("a bool where a number was expected: %r" % (value,))
    if isinstance(value, int):
        return str(value)
    return repr(float(value))


def _vector(values):
    x, y, z = (list(values) + [0.0, 0.0, 0.0])[:3]
    return "(X=%s,Y=%s,Z=%s)" % (_number(x), _number(y), _number(z))


def _vector2(values):
    if len(values) != 2:
        raise ValueError("an FVector2D of %d numbers" % len(values))
    return "(X=%s,Y=%s)" % (_number(values[0]), _number(values[1]))


def _box(values):
    """An exported FBox: min, max, and IsValid packed into a float's bytes (its low byte; the rest is padding)."""
    if len(values) != 7:
        raise ValueError("an FBox of %d numbers" % len(values))
    valid = struct.unpack("<I", struct.pack("<f", values[6]))[0] & 0xFF
    return "(Min=%s,Max=%s,IsValid=%s)" % (_vector(values[:3]), _vector(values[3:6]), "True" if valid else "False")


def _quoted(text):
    return '"%s"' % text.replace("\\", "\\\\").replace('"', '\\"')


def _burst(value):
    """An exported FParticleBurst ({Count, CountLow, Time}; what it leaves out is at the struct's defaults)."""
    unknown = set(value) - {k for k, _ in BURST_DEFAULTS}
    if unknown:
        raise ValueError("a burst with %s" % sorted(unknown))
    return "(%s)" % ",".join("%s=%s" % (k, _number(value.get(k, d))) for k, d in BURST_DEFAULTS)


class _Build:
    """One particle system's rebuild: the export's objects by their path in the package, and what was made for them."""

    def __init__(self, rel, version):
        self.rel = rel
        self.version = version
        pkg = dd_assets.export_json(rel, version)
        self.name = rel.rsplit("/", 1)[-1]
        self.exports = {}
        for e in pkg["exports"]:
            key = e["name"] if e.get("outer") in (None, "") else "%s.%s" % (e["outer"], e["name"])
            self.exports[key] = e
        self.system_export = self.exports[self.name]
        if self.system_export["class"] != "ParticleSystem":
            raise ValueError("%s is a %s, not a particle system" % (rel, self.system_export["class"]))
        self.made = {}
        self.target = dd_assets.asset_path(rel)

    # ------------------------------------------------------------------------------------------ values
    def _object(self, value):
        """An exported object reference → its path here: an object of this package → the one made for it; an asset of
        the original's /Game → the asset under /Game/DD (which has to exist)."""
        if value is None:
            return "None"
        if value in self.made:
            return _quoted(self.made[value].get_path_name())
        if value.startswith("/Game/"):
            package, _, name = value.partition(".")
            ours = dd_assets.asset_path(dd_assets.game_rel(package))
            if not EAL.does_asset_exist(ours):
                raise RuntimeError("%s uses %s, which is not made yet (%s)" % (self.rel, value, ours))
            return _quoted("%s.%s" % (ours, name or paths.split(ours)[1]))
        raise ValueError("%s: a reference to %s, which is neither in the package nor an asset" % (self.rel, value))

    def _raw_distribution(self, raw, vector):
        parts = ["MinValue=" + _number(raw.get("MinValue", 0.0)), "MaxValue=" + _number(raw.get("MaxValue", 0.0))]
        if vector:
            parts.append("MinValueVec=" + _vector(raw.get("MinValueVec", ())))
            parts.append("MaxValueVec=" + _vector(raw.get("MaxValueVec", ())))
        table = raw.get("Table") or {}
        unknown = set(table) - {k for k, _ in TABLE_DEFAULTS} - {"Values"}
        if unknown:
            raise ValueError("%s: a lookup table with %s" % (self.rel, sorted(unknown)))
        fields = ["%s=%s" % (k, _number(table.get(k, d))) for k, d in TABLE_DEFAULTS]
        fields.insert(2, "Values=(%s)" % ",".join(_number(v) for v in table.get("Values", ())))
        parts.append("Table=(%s)" % ",".join(fields))
        parts.append("Distribution=" + self._object(raw.get("Distribution")))
        unknown = set(raw) - {"MinValue", "MaxValue", "MinValueVec", "MaxValueVec", "Table", "Distribution"}
        if unknown:
            raise ValueError("%s: a distribution with %s" % (self.rel, sorted(unknown)))
        return "(%s)" % ",".join(parts)

    def _dynamic_parameter(self, value):
        """An exported FEmitterDynamicParameter ({ParamName, ..., ParamValue}; its distribution a lookup table)."""
        kinds = dict(DYNAMIC_PARAMETER_MEMBERS)
        unknown = set(value) - set(kinds)
        if unknown:
            raise ValueError("%s: a dynamic parameter with %s" % (self.rel, sorted(unknown)))
        parts = []
        for key, kind in DYNAMIC_PARAMETER_MEMBERS:
            if key not in value:
                continue
            v = value[key]
            if kind == "bool":
                if not isinstance(v, bool):
                    raise ValueError("%s: a dynamic parameter's %s of %r" % (self.rel, key, v))
                text = "True" if v else "False"
            elif kind == "text":
                text = _quoted(v)
            else:
                text = self._raw_distribution(v, False)
            parts.append("%s=%s" % (key, text))
        return "(%s)" % ",".join(parts)

    def _text(self, obj, key, value):
        cpp = LIB.get_property_type(obj, key)
        if not cpp:
            raise RuntimeError("%s has no property %s" % (obj.get_class().get_name(), key))
        if cpp in RAW_DISTRIBUTIONS:
            if not isinstance(value, dict):
                raise ValueError("%s.%s: a %s from %r" % (obj.get_name(), key, cpp, value))
            return self._raw_distribution(value, cpp == "FRawDistributionVector")
        if cpp == "FBox":
            return _box(value)
        if cpp == "FVector":
            return _vector(value)
        if cpp == "FVector2D":
            return _vector2(value)
        # A bitfield bool's C++ type reads as its storage ('uint8').
        if isinstance(value, bool) and cpp in ("bool", "uint8", "uint16", "uint32", "uint64"):
            return "True" if value else "False"
        if cpp == "bool":
            raise ValueError("%s.%s: a bool from %r" % (obj.get_name(), key, value))
        if cpp.startswith(("TObjectPtr<", "TSubclassOf<")) or cpp.endswith("*"):
            return self._object(value)
        if cpp in ("float", "double", "int32", "uint8", "int8", "uint16", "int16", "uint32", "int64"):
            return _number(value)
        if cpp in ("FName", "FString") or cpp.startswith("TEnumAsByte<") or cpp.startswith("E"):
            if not isinstance(value, str):
                raise ValueError("%s.%s: a %s from %r" % (obj.get_name(), key, cpp, value))
            return _quoted(value)
        if cpp in ("TArray", "TArray<float>") and all(isinstance(v, (int, float)) for v in value):
            return "(%s)" % ",".join(_number(v) for v in value)
        if cpp in ("TArray", "TArray<FParticleSystemLOD>") and all(v == {} for v in value):
            # Structs at their defaults (UParticleSystem::LODSettings, one per LOD distance).
            return "(%s)" % ",".join("()" for _ in value)
        if cpp == "TArray<FParticleBurst>" and all(isinstance(v, dict) for v in value):
            return "(%s)" % ",".join(_burst(v) for v in value)
        if cpp == "TArray<FEmitterDynamicParameter>" and all(isinstance(v, dict) for v in value):
            return "(%s)" % ",".join(self._dynamic_parameter(v) for v in value)
        raise ValueError("%s.%s: no text form for a %s (%r)" % (obj.get_name(), key, cpp, value))

    def _write(self, obj, props, skip=()):
        for key, value in props.items():
            if key in skip or key in DERIVED or key in NOT_WRITTEN:
                continue
            text = self._text(obj, key, value)
            error = LIB.set_property_text(obj, key, text)
            if error is None or error:
                raise RuntimeError("%s: %s.%s = %s was not written: %s" % (self.rel, obj.get_name(), key, text, error))

    # ------------------------------------------------------------------------------------------ objects
    def _make(self, outer, key):
        e = self.exports.get(key)
        if e is None:
            raise KeyError("%s: no export %s" % (self.rel, key))
        obj = LIB.make_object(outer, e["class"], e["name"])
        if obj is None:
            raise RuntimeError("%s: could not make %s (%s)" % (self.rel, key, e["class"]))
        self.made[key] = obj
        return obj

    def _make_distributions(self, module, props):
        """The distribution objects a module's values point at (kept by the cook), with their own values."""
        for value in props.values():
            ref = value.get("Distribution") if isinstance(value, dict) else None
            if ref and ref not in self.made:
                dist = self._make(module, ref)
                self._write(dist, self.exports[ref]["props"])

    def _module(self, key):
        if key in self.made:
            return self.made[key]
        module = self._make(self.system, key)
        props = self.exports[key]["props"]
        self._make_distributions(module, props)
        self._write(module, props)
        return module

    def run(self):
        folder, name = paths.split(self.target)
        if EAL.does_asset_exist(self.target):
            self.system = unreal.load_asset(self.target)
        else:
            self.system = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, folder, unreal.ParticleSystem, unreal.ParticleSystemFactoryNew())
        LIB.reset_particle_system(self.system)
        self.made[self.name] = self.system
        props = self.system_export["props"]
        self._write(self.system, props, STRUCTURE["system"])

        for emitter_key in props["Emitters"]:
            emitter = self._make(self.system, emitter_key)
            emitter_props = dict(self.exports[emitter_key]["props"])
            mask = emitter_props.get("DetailModeBitmask")
            if mask is not None and mask & DETAIL_HIGH:
                emitter_props["DetailModeBitmask"] = mask | DETAIL_EPIC
            self._write(emitter, emitter_props, STRUCTURE["emitter"])
            if not LIB.add_emitter(self.system, emitter):
                raise RuntimeError("%s: %s was not added" % (self.rel, emitter_key))
            for lod_key in emitter_props["LODLevels"]:
                lod = self._make(emitter, lod_key)
                lod_props = self.exports[lod_key]["props"]
                if lod_props.get("TypeDataModule"):
                    raise NotImplementedError("%s: %s has a type data module" % (self.rel, lod_key))
                self._write(lod, lod_props, STRUCTURE["lod"] + ("TypeDataModule",))
                required = self._module(lod_props["RequiredModule"])
                spawn = self._module(lod_props["SpawnModule"])
                modules = [self._module(k) for k in lod_props.get("Modules", ())]
                if not LIB.add_lod_level(emitter, lod, required, spawn, modules):
                    raise RuntimeError("%s: %s was not added" % (self.rel, lod_key))
        LIB.finish_particle_system(self.system)
        self._check()
        EAL.save_asset(self.target, only_if_is_dirty=False)
        return self.target

    def _check(self):
        """The rebuilt structure against the export: emitters, LOD levels and their modules in order, and each module's
        LOD validity as UE worked it out against the one the cook saved."""
        by_object = {obj.get_path_name(): key for key, obj in self.made.items()}
        emitters = [by_object.get(o.get_path_name()) for o in LIB.get_emitters(self.system)]
        if emitters != list(self.system_export["props"]["Emitters"]):
            raise RuntimeError("%s: emitters %s" % (self.rel, emitters))
        for emitter_key, emitter in zip(emitters, LIB.get_emitters(self.system)):
            lod_keys = self.exports[emitter_key]["props"]["LODLevels"]
            lods = LIB.get_lod_levels(emitter)
            if [by_object.get(o.get_path_name()) for o in lods] != list(lod_keys):
                raise RuntimeError("%s: the LOD levels of %s" % (self.rel, emitter_key))
            for lod_key, lod in zip(lod_keys, lods):
                p = self.exports[lod_key]["props"]
                want = [p["RequiredModule"], p["SpawnModule"]] + list(p.get("Modules", ()))
                got = [by_object.get(o.get_path_name()) for o in LIB.get_lod_modules(lod)]
                if got != want:
                    raise RuntimeError("%s: the modules of %s are %s" % (self.rel, lod_key, got))
        for key, obj in self.made.items():
            saved = self.exports[key]["props"].get("LODValidity")
            if saved is not None and int(LIB.get_property_text(obj, "LODValidity")) != saved:
                raise RuntimeError("%s: %s is valid in LODs %s, the cook saved %s"
                                   % (self.rel, key, LIB.get_property_text(obj, "LODValidity"), saved))


def particle_system(rel, version=1):
    """Rebuilds the original's Cascade system /Game/<rel> ('ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2')
    under /Game/DD from pak_reference (1) or pak_reference_2 (2). The materials it uses have to be made first. Returns
    the package path."""
    return _Build(rel, version).run()


def describe(asset_path):
    """The particle system's emitters, LOD levels and modules with a few values, for checking a rebuild."""
    system = unreal.load_asset(asset_path)
    out = []
    for emitter in LIB.get_emitters(system):
        entry = {"name": LIB.get_property_text(emitter, "EmitterName"),
                 "detail": LIB.get_property_text(emitter, "DetailModeBitmask"),
                 "legacy_spawning": LIB.get_property_text(emitter, "bUseLegacySpawningBehavior"), "lods": []}
        for lod in LIB.get_lod_levels(emitter):
            modules = [(m.get_class().get_name(), m.get_name(), LIB.get_property_text(m, "LODValidity"))
                       for m in LIB.get_lod_modules(lod)]
            entry["lods"].append({"level": LIB.get_property_text(lod, "Level"), "modules": modules})
        out.append(entry)
    return json.dumps(out, indent=1)
