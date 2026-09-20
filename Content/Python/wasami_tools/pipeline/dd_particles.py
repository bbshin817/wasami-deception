"""Dark Deception's Cascade particle systems, rebuilt under /Game/DD from their exported packages (_assets/.../P_*.json)
through UWasamiCascadeLibrary (C++): every emitter, LOD level and module with the values the cook saved.

A mesh emitter is a sprite emitter whose LOD levels share a type data module (ParticleModuleTypeDataMesh), made in the
system like the other modules and handed to each LOD level beside them.

The cook keeps a module's distributions as their baked lookup tables (FRawDistribution: Table, MinValue, MaxValue)
without the distribution objects, and that table is what the game reads. They are written as they are, with no
distribution object, which UE 5.8 reads the same way (FRawDistributionFloat::GetValue uses the table; the editor only
re-bakes a table from a distribution object). Distributions the cook could not bake (a particle parameter) and the ones
it kept anyway are made as objects with their exported values. A GPU emitter (ParticleModuleTypeDataGpu) is the
exception: the editor builds its simulation from its modules' distribution objects (UParticleEmitter::Build →
CompileModule, which reads ColorOverLife.Distribution and the like without checking), so each of its tables without one
gets the object it was baked from, as near as the table tells (_table_distribution). Its colour, size and sub-image
index come from the simulation the cook saved instead (_gpu_resource), which is what the original draws.

Values are written in UE's text form by the properties' own names. What a package leaves out is at the class defaults
(the export holds what differs from them), except where UE 5.8 reads the old package differently (DETAIL_EPIC)."""
import decimal
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
    "lod": ("RequiredModule", "SpawnModule", "Modules", "TypeDataModule"),
}
# Recomputed from where each module is used (UParticleSystem::SetupLODValidity) and compared with the export after.
DERIVED = ("LODValidity",)
# Editor-only data the cook kept a reference to (a Cascade editor's curve list) and UE 4's saved-dirty flag of a
# distribution, which UE 5 does not save (a new distribution is dirty, which only asks the editor to bake it).
NOT_WRITTEN = ("CurveEdSetup", "bIsDirty")
# A GPU emitter's simulation data (ParticleModuleTypeDataGpu), which the cook saves and the editor builds from the
# emitter's modules (UParticleEmitter::Build → UParticleModuleTypeDataGpu::Build, run by UpdateModuleLists).
BUILT = ("EmitterInfo", "ResourceData")

# An FColor of an exported quantized curve reads B, G, R, A; the curve's channels are named R, G, B, A.
COLOR_CHANNEL = (2, 1, 0, 3)
# The modules whose CompileModule writes each of a GPU emitter's quantized curves, in the order a LOD level lists them,
# and the class of the last of them the saved curve can be read back into. The last one to run sets the curve; any
# other one composes with what is already there (ColorScaleOverLife scales the colour built so far), and a saved curve
# cannot be split between two modules, so a GPU emitter with one is raised on rather than guessed at.
GPU_COLOR_MODULES = ("ParticleModuleColor", "ParticleModuleColor_Seeded", "ParticleModuleColorOverLife",
                     "ParticleModuleColorScaleOverLife")
GPU_SIZE_MODULES = ("ParticleModuleSize", "ParticleModuleSize_Seeded", "ParticleModuleSizeMultiplyLife",
                    "ParticleModuleSizeScale")
# UParticleModuleSize::CompileModule only sets the size curve to a constant 1 (a particle's own size is the spawn
# module's), so an emitter whose last size module is that one has nothing to read the saved curve back into.
GPU_SIZE_CONSTANT = ("ParticleModuleSize", "ParticleModuleSize_Seeded")
GPU_SIZE_PROPERTY = {"ParticleModuleSizeMultiplyLife": "LifeMultiplier", "ParticleModuleSizeScale": "SizeScale"}
# EParticleSubUVInterpMethod's values that make UParticleModuleSubUV::CompileModule feed SubImageIndex into the
# simulation; under any other one the channel stays at zero and the module's own value is never read.
SUBUV_LINEAR = ("PSUVIM_Linear", "PSUVIM_Linear_Blend")

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
# FParticleEvent_GenerateInfo's members (an event generator module's Events), the same way. The events it would also send
# to the game (ParticleModuleEventsToSendToGame, objects) are written only when there are none.
EVENT_MEMBERS = (("Type", "text"), ("Frequency", "number"), ("ParticleFrequency", "number"), ("FirstTimeOnly", "bool"),
                 ("LastTimeOnly", "bool"), ("UseReflectedImpactVector", "bool"), ("bUseOrbitOffset", "bool"),
                 ("CustomName", "text"), ("ParticleModuleEventsToSendToGame", "none"))


def _number(value):
    """A number in UE's text form, never in exponent form: an array's text import loses the element after one written
    so (a lookup table of 128 values that ends '-5.5e-05,0.0' came in with 127, and its last entry read past the end:
    an assert in the particles' update)."""
    if isinstance(value, bool):
        raise TypeError("a bool where a number was expected: %r" % (value,))
    if isinstance(value, int):
        return str(value)
    text = repr(float(value))
    if "e" in text or "E" in text:
        text = format(decimal.Decimal(float(value)), "f")   # exact, however small
        text = text if "." in text else text + ".0"
    return text


def _vector(values):
    x, y, z = (list(values) + [0.0, 0.0, 0.0])[:3]
    return "(X=%s,Y=%s,Z=%s)" % (_number(x), _number(y), _number(z))


def _vector2(values):
    if len(values) != 2:
        raise ValueError("an FVector2D of %d numbers" % len(values))
    return "(X=%s,Y=%s)" % (_number(values[0]), _number(values[1]))


def _rotator(values):
    """An exported FRotator: [pitch, yaw, roll] in degrees."""
    if len(values) != 3:
        raise ValueError("an FRotator of %d numbers" % len(values))
    return "(Pitch=%s,Yaw=%s,Roll=%s)" % tuple(_number(v) for v in values)


def _curve(value, member):
    """An exported FInterpCurve* ({Points: [{InVal, OutVal, ArriveTangent, LeaveTangent, InterpMode}]}); member makes the
    text of one OutVal or tangent."""
    unknown = set(value) - {"Points", "bIsLooped", "LoopKeyOffset"}
    if unknown:
        raise ValueError("a curve with %s" % sorted(unknown))
    points = []
    for point in value.get("Points", ()):
        parts = ["InVal=" + _number(point["InVal"]), "OutVal=" + member(point["OutVal"])]
        for key in ("ArriveTangent", "LeaveTangent"):
            if key in point:
                parts.append("%s=%s" % (key, member(point[key])))
        parts.append("InterpMode=" + point.get("InterpMode", "CIM_Linear"))
        points.append("(%s)" % ",".join(parts))
    return "(Points=(%s),bIsLooped=%s,LoopKeyOffset=%s)" % (
        ",".join(points), "True" if value.get("bIsLooped") else "False", _number(value.get("LoopKeyOffset", 0.0)))


def _two_vectors(values):
    """An FTwoVectors: [v1 x, y, z, v2 x, y, z]."""
    if len(values) != 6:
        raise ValueError("an FTwoVectors of %d numbers" % len(values))
    return "(v1=%s,v2=%s)" % (_vector(values[:3]), _vector(values[3:]))


# The FInterpCurve kinds the distribution objects hold, and the text of one of their values.
CURVES = {"FInterpCurveFloat": _number, "FInterpCurveVector": _vector, "FInterpCurveVector2D": _vector2,
          "FInterpCurveTwoVectors": _two_vectors}

# ERawDistributionOperation (Distributions.h): a table's entries are single values (RDO_None) or low and high values
# to pick between at random (RDO_Random).
RDO_NONE = 1
RDO_RANDOM = 2


def _table_distribution(raw, vector):
    """The distribution object (its class and values) a cook's baked table (an exported FRawDistribution) was made
    from, as near as the table tells. A table's entries sit TimeBias + i / TimeScale apart and are lerped between
    (FDistributionLookupTable::GetEntry), which a linear curve through them repeats; one entry is a constant, or a
    uniform range when the entry holds a low and a high value."""
    table = raw["Table"]
    op, count = table.get("Op", 0), table.get("EntryCount", 0)
    stride, sub = table.get("EntryStride", 0), table.get("SubEntryStride", 0)
    values = table.get("Values", [])
    width = 3 if vector else 1
    wanted = {RDO_NONE: (width, 0), RDO_RANDOM: (2 * width, width)}.get(op)
    if wanted is None or (stride, sub) != wanted or count < 1 or len(values) != count * stride or table.get("LockFlag"):
        raise ValueError("a baked table no distribution is made for: %r" % (table,))
    kind = "Vector" if vector else "Float"
    one = list if vector else (lambda v: v[0])
    entries = [values[i * stride:(i + 1) * stride] for i in range(count)]
    times = [table.get("TimeBias", 0.0) + (i / table["TimeScale"] if i else 0.0) for i in range(count)]
    if op == RDO_NONE:
        if count == 1:
            return "Distribution%sConstant" % kind, {"Constant": one(entries[0])}
        points = [{"InVal": t, "OutVal": one(e)} for t, e in zip(times, entries)]
        return "Distribution%sConstantCurve" % kind, {"ConstantCurve": {"Points": points}}
    if count == 1:
        return "Distribution%sUniform" % kind, {"Min": one(entries[0][:width]), "Max": one(entries[0][width:])}
    # A uniform curve's value: (X low, Y high) for a float, (v1 low, v2 high) for a vector.
    points = [{"InVal": t, "OutVal": list(e)} for t, e in zip(times, entries)]
    return "Distribution%sUniformCurve" % kind, {"ConstantCurve": {"Points": points}}


def _quantized_channel(resource, curve, channel):
    """One channel of a quantized curve of a GPU emitter's saved simulation ('Color' or 'Misc') as the values it stands
    for: Bias + Scale * c / 255, the way the simulation reads it back (FComposableDistribution::QuantizeVector4). A
    channel whose Scale is 0 is a constant at Bias, and so is every channel of a curve with no samples - the build
    leaves the samples of a one-entry curve alone, so what the cook saved there is an older build's leftovers."""
    scale = (resource.get("%sScale" % curve) or [0.0] * 4)[channel]
    bias = (resource.get("%sBias" % curve) or [0.0] * 4)[channel]
    samples = resource.get("Quantized%sSamples" % curve) or []
    if not scale or not samples:
        return [bias]
    return [bias + scale * s[COLOR_CHANNEL[channel]] / 255.0 for s in samples]


def _channel_distribution(channels, vector):
    """The distribution object (its class and values) that builds one or more channels of a quantized curve back: a
    constant when every channel holds one value, else a curve through the samples, evenly over 0..1 (the life the
    simulation reads the curve over). A channel of one value beside longer ones is that value all along."""
    count = max(len(c) for c in channels)
    kind = "Vector" if vector else "Float"
    one = (lambda values: values) if vector else (lambda values: values[0])
    if count == 1:
        return "Distribution%sConstant" % kind, {"Constant": one([c[0] for c in channels])}
    points = [{"InVal": i / (count - 1.0), "OutVal": one([c[min(i, len(c) - 1)] for c in channels])}
              for i in range(count)]
    return "Distribution%sConstantCurve" % kind, {"ConstantCurve": {"Points": points}}


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

    def __init__(self, rel, version, target=None, adjust=None):
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
        if adjust is not None:
            adjust(self.exports)
        self._gpu_distributions()
        self.made = {}
        self.target = target or dd_assets.asset_path(rel)

    def _gpu_distributions(self):
        """Every GPU emitter's modules, which the editor builds the simulation from at every load: each baked table
        without a distribution object gets the one it was baked from, and then the simulation the cook saved is read
        back into the modules it was built from (_gpu_resource)."""
        for lod in [e for e in self.exports.values() if e["class"] == "ParticleLODLevel"]:
            p = lod["props"]
            type_data = self.exports.get(p.get("TypeDataModule") or "")
            if type_data is None or type_data["class"] != "ParticleModuleTypeDataGpu":
                continue
            modules = [p["RequiredModule"], p["SpawnModule"]] + list(p.get("Modules", ()))
            for key in modules:
                for prop, raw in self.exports[key]["props"].items():
                    if not isinstance(raw, dict) or "Table" not in raw or raw.get("Distribution"):
                        continue
                    # A vector's entries are 3 values (6 with a low and a high), a float's 1 (or 2).
                    cls, values = _table_distribution(raw, raw["Table"].get("EntryStride", 0) in (3, 6))
                    self._gpu_distribution(key, prop, cls, values)
            self._gpu_resource(type_data["props"], modules)

    def _gpu_distribution(self, key, prop, cls, values):
        """One module property's distribution object, added to the exports (named Baked<property>, after what the
        module's own default subobjects are not called) and pointed at by the property."""
        name = "Baked" + prop
        self.exports["%s.%s" % (key, name)] = {"name": name, "outer": key, "class": cls, "props": values}
        self.exports[key]["props"].setdefault(prop, {})["Distribution"] = "%s.%s" % (key, name)

    def _gpu_owner(self, modules, group, wanted, what):
        """The module of a GPU emitter's LOD level whose distributions one of the quantized curves is read back into:
        the last of the modules that write it (their CompileModule runs in the order the level lists them), which has
        to be one that sets the curve rather than composing with what is already there."""
        found = [(self.exports[key]["class"], key) for key in modules if self.exports[key]["class"] in group]
        if not found:
            return None, None
        cls, key = found[-1]
        if cls not in wanted:
            raise ValueError("%s: %s of a GPU emitter is composed by %s, which the saved simulation cannot be split "
                             "between" % (self.rel, what, cls))
        return cls, key

    def _gpu_resource(self, type_data, modules):
        """A GPU emitter's colour, size and sub-image index from the simulation the cook saved (ResourceData), which is
        what the original draws: UParticleModuleTypeDataGpu::Build is editor-only, so a cooked game reads the saved
        simulation and never looks at the modules again. Where the two disagree - BallisticsVFX's do, their modules
        being of another version than the cook - the saved one is the original's look, and the module gets the
        distribution the editor builds it back from (it rebuilds the simulation at every load, so writing the saved
        values onto the module would do nothing).

        Quantized to 8 bits over the curve's own range, the saved curve is the value the game reads, so reading it back
        loses nothing; only the editor's own resampling (OptimizeLookupTable) rounds a corner of a curve it did not
        sample on by a step or two."""
        resource = type_data.get("ResourceData")
        if not resource:
            return

        def read_back(key, prop, channels, vector):
            """One property from the saved simulation - unless the cook kept the module's own distribution object,
            which is the value the original's own build read, so the editor builds the same simulation from it. Only a
            property the cook left as a baked table alone can be of another version than the simulation: the GPU path
            never reads the table, so it was never baked again."""
            ref = (self.exports[key]["props"].get(prop) or {}).get("Distribution")
            if ref and ref != "%s.Baked%s" % (key, prop):
                return
            self._gpu_distribution(key, prop, *_channel_distribution(channels, vector))

        # The colour curve is R:G:B from ColorOverLife and A from AlphaOverLife.
        _, key = self._gpu_owner(modules, GPU_COLOR_MODULES, ("ParticleModuleColorOverLife",), "the colour")
        if key is None:
            raise ValueError("%s: a GPU emitter with no colour module to read the saved simulation back into"
                             % self.rel)
        read_back(key, "ColorOverLife", [_quantized_channel(resource, "Color", c) for c in (0, 1, 2)], True)
        read_back(key, "AlphaOverLife", [_quantized_channel(resource, "Color", 3)], False)

        # The misc curve is R:SizeX G:SizeY B:SubImageIndex. The size is the curve times the largest a particle gets,
        # which the build stores as its inverse; an axis a multiply-life module does not multiply reads back as the
        # constant 1 its mask leaves, which builds the same simulation. Z is never read (the curve keeps X and Y).
        max_size = [1.0 / v if v else 1.0
                    for v in (type_data.get("EmitterInfo") or {}).get("InvMaxSize") or (1.0, 1.0)]
        size = [[v / max_size[c] for v in _quantized_channel(resource, "Misc", c)] for c in (0, 1)]
        cls, key = self._gpu_owner(modules, GPU_SIZE_MODULES,
                                   tuple(GPU_SIZE_PROPERTY) + GPU_SIZE_CONSTANT, "the size")
        if cls in GPU_SIZE_PROPERTY:
            read_back(key, GPU_SIZE_PROPERTY[cls], size + [size[0]], True)
        elif any(len(c) > 1 or abs(c[0] - 1.0) > 1e-3 for c in size):
            raise ValueError("%s: a GPU emitter whose saved size curve is %s with no module to read it back into"
                             % (self.rel, [c[:4] for c in size]))

        index = _quantized_channel(resource, "Misc", 2)
        _, key = self._gpu_owner(modules, ("ParticleModuleSubUV",), ("ParticleModuleSubUV",), "the sub-image index")
        required = self.exports[modules[0]]["props"]
        method = required.get("InterpolationMethod", "PSUVIM_None")
        if key is not None and method in SUBUV_LINEAR:
            read_back(key, "SubImageIndex", [index], False)
        elif len(index) > 1 or index[0]:
            raise ValueError("%s: a GPU emitter whose saved sub-image index is %s with no module to read it back into "
                             "(%s)" % (self.rel, index[:4], method))

    # ------------------------------------------------------------------------------------------ values
    def _object(self, value):
        """An exported object reference → its path here: an object of this package → the one made for it; an asset of
        the original's /Game → the asset under /Game/DD (which has to exist); an engine asset (/Engine/...) → the same
        one, which this engine has to have."""
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
        if value.startswith("/Engine/"):
            if not EAL.does_asset_exist(value.partition(".")[0]):
                raise RuntimeError("%s uses %s, which this engine does not have" % (self.rel, value))
            return _quoted(value)
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

    def _struct(self, value, members, what):
        """An exported struct by its members' forms (DYNAMIC_PARAMETER_MEMBERS, EVENT_MEMBERS): text, number, bool, a
        distribution (a lookup table) or none (an array that has to be empty)."""
        kinds = dict(members)
        unknown = set(value) - set(kinds)
        if unknown:
            raise ValueError("%s: %s with %s" % (self.rel, what, sorted(unknown)))
        parts = []
        for key, kind in members:
            if key not in value:
                continue
            v = value[key]
            if kind == "bool":
                if not isinstance(v, bool):
                    raise ValueError("%s: %s's %s of %r" % (self.rel, what, key, v))
                text = "True" if v else "False"
            elif kind == "text":
                text = _quoted(v)
            elif kind == "number":
                text = _number(v)
            elif kind == "none":
                if v:
                    raise ValueError("%s: %s's %s of %r" % (self.rel, what, key, v))
                text = "()"
            else:
                text = self._raw_distribution(v, False)
            parts.append("%s=%s" % (key, text))
        return "(%s)" % ",".join(parts)

    def _dynamic_parameter(self, value):
        """An exported FEmitterDynamicParameter ({ParamName, ..., ParamValue}; its distribution a lookup table)."""
        return self._struct(value, DYNAMIC_PARAMETER_MEMBERS, "a dynamic parameter")

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
        if cpp == "FRotator":
            return _rotator(value)
        if cpp in CURVES and isinstance(value, dict):
            return _curve(value, CURVES[cpp])
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
            # A property's own text (PPF_None) takes a name or string whole, quotes and all, so those go bare; an enum
            # reads a token, quoted or not. (Built before 2026-09-19, every EmitterName and a receiver's EventName kept
            # the quotes, and P_06_Defib's lightning never took its emitters' 'born'.)
            return value if cpp in ("FName", "FString") else _quoted(value)
        if cpp in ("TArray", "TArray<float>") and all(isinstance(v, (int, float)) for v in value):
            return "(%s)" % ",".join(_number(v) for v in value)
        if cpp in ("TArray", "TArray<FParticleSystemLOD>") and all(v == {} for v in value):
            # Structs at their defaults (UParticleSystem::LODSettings, one per LOD distance).
            return "(%s)" % ",".join("()" for _ in value)
        if cpp == "TArray<FParticleBurst>" and all(isinstance(v, dict) for v in value):
            return "(%s)" % ",".join(_burst(v) for v in value)
        if cpp == "TArray<FEmitterDynamicParameter>" and all(isinstance(v, dict) for v in value):
            return "(%s)" % ",".join(self._dynamic_parameter(v) for v in value)
        if cpp == "TArray<FParticleEvent_GenerateInfo>" and all(isinstance(v, dict) for v in value):
            return "(%s)" % ",".join(self._struct(v, EVENT_MEMBERS, "an event") for v in value)
        raise ValueError("%s.%s: no text form for a %s (%r)" % (obj.get_name(), key, cpp, value))

    def _write(self, obj, props, skip=()):
        for key, value in props.items():
            if key in skip or key in DERIVED or key in NOT_WRITTEN or key in BUILT:
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
        try:
            self._build()
        except Exception:
            # Nothing half-built is left: an emitter without its LOD levels would fail UE's asset registry tags
            # (UParticleSystem::GetAssetRegistryTags → HasGPUEmitter reads LODLevels[0]), which asserts the editor down the next time
            # anything asks for them (EditorAssetLibrary.does_asset_exist, a save).
            LIB.reset_particle_system(self.system)
            raise
        LIB.finish_particle_system(self.system)
        self._check()
        EAL.save_asset(self.target, only_if_is_dirty=False)
        return self.target

    def _build(self):
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
                required = self._module(lod_props["RequiredModule"])
                spawn = self._module(lod_props["SpawnModule"])
                modules = [self._module(k) for k in lod_props.get("Modules", ())]
                type_data = self._module(lod_props["TypeDataModule"]) if lod_props.get("TypeDataModule") else None
                # After its modules: a LOD level names one of them, its event generator (EventGenerator).
                self._write(lod, lod_props, STRUCTURE["lod"])
                if not LIB.add_lod_level(emitter, lod, required, spawn, modules, type_data):
                    raise RuntimeError("%s: %s was not added" % (self.rel, lod_key))

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
                type_data = LIB.get_lod_type_data_module(lod)
                got_type = by_object.get(type_data.get_path_name()) if type_data else None
                if got_type != p.get("TypeDataModule"):
                    raise RuntimeError("%s: the type data module of %s is %s" % (self.rel, lod_key, got_type))
        for key, obj in self.made.items():
            saved = self.exports[key]["props"].get("LODValidity")
            if saved is not None and int(LIB.get_property_text(obj, "LODValidity")) != saved:
                raise RuntimeError("%s: %s is valid in LODs %s, the cook saved %s"
                                   % (self.rel, key, LIB.get_property_text(obj, "LODValidity"), saved))


def particle_system(rel, version=1, target=None, adjust=None):
    """Rebuilds the original's Cascade system /Game/<rel> ('ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2')
    under /Game/DD from pak_reference (1) or pak_reference_2 (2). The materials it uses have to be made first. Returns
    the package path.

    A version of our own is made elsewhere by giving its package path as target and a function adjust, which gets the
    export's objects ({path in the package: export}) and may change their values ("props") in place before anything is
    made; the structure has to stay the original's, which the rebuild is checked against."""
    return _Build(rel, version, target, adjust).run()


def describe(asset_path):
    """The particle system's emitters, LOD levels, modules and type data (a mesh emitter's) with a few values, for
    checking a rebuild."""
    system = unreal.load_asset(asset_path)
    out = []
    for emitter in LIB.get_emitters(system):
        entry = {"name": LIB.get_property_text(emitter, "EmitterName"),
                 "detail": LIB.get_property_text(emitter, "DetailModeBitmask"),
                 "legacy_spawning": LIB.get_property_text(emitter, "bUseLegacySpawningBehavior"), "lods": []}
        for lod in LIB.get_lod_levels(emitter):
            modules = [(m.get_class().get_name(), m.get_name(), LIB.get_property_text(m, "LODValidity"))
                       for m in LIB.get_lod_modules(lod)]
            level = {"level": LIB.get_property_text(lod, "Level"), "modules": modules}
            type_data = LIB.get_lod_type_data_module(lod)
            if type_data:
                level["type_data"] = (type_data.get_class().get_name(), type_data.get_name(),
                                      LIB.get_property_text(type_data, "LODValidity"),
                                      LIB.get_property_text(type_data, "Mesh"))
            entry["lods"].append(level)
        out.append(entry)
    return json.dumps(out, indent=1)
