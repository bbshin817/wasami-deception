"""A GPU emitter's simulation data, the original's against ours.

    python Tools/dd/gpu_emitters.py <asset> [--pak 1|2] [--ours] [--full] [--samples N]

<asset> is part of the package path under Content, e.g. "Fracture_concrete_3" or "06_Hospital/P_06_Defib". Every
exported particle system whose path contains it is read.

A GPU emitter (ParticleModuleTypeDataGpu) is the one kind of emitter whose look is not in its modules: the editor
compiles them into FGPUSpriteEmitterInfo and FGPUSpriteResourceData (UParticleModuleTypeDataGpu::Build, WITH_EDITOR
only), and the cooked game reads those back and draws from them alone. So the cook's saved ResourceData is what the
original looks like, whatever its modules say. This prints it, and with --ours the one UE 5.8 built for our rebuilt
system under /Game/DD (through the running editor, Tools/ue_remote.py), marking every row that differs with '*'.

Our editor rebuilds ResourceData from the modules' distribution objects at every load (UParticleEmitter::PostLoad ->
UpdateModuleLists -> Build), so a row that differs is a distribution that did not come back from the cook's baked
lookup table the way the original's was, not something to write onto the asset.

The quantized curves (QuantizedColorSamples R:G:B:A over life, QuantizedMiscSamples R:SizeX G:SizeY B:SubImageIndex,
QuantizedSimulationAttrSamples R:DragScale G:VectorFieldScale B:Resilience) are FColors of a value Bias + Scale *
c/255; they print as their count and their first, middle and last entries (--samples N for more, --full for all).

Env: PAK_REF / PAK_REF2 - the exports (default <repo>/pak_reference, <repo>/pak_reference_2).
"""
import argparse
import json
import os
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REFS = {1: os.environ.get("PAK_REF", os.path.join(ROOT, "pak_reference")),
        2: os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))}
UE_REMOTE = os.path.join(ROOT, "Tools", "ue_remote.py")
# Our /Game/DD holds the original's /Game/<rel> (dd_assets.asset_path).
DD_ROOT = "/Game/DD"
GPU = "ParticleModuleTypeDataGpu"

# FGPUSpriteResourceData's members in the order the header declares them, with the value a default-constructed one
# has: what the export leaves out is at that value (UE's tagged serialisation writes only what differs).
RESOURCE_DEFAULTS = (
    ("QuantizedColorSamples", []), ("QuantizedMiscSamples", []), ("QuantizedSimulationAttrSamples", []),
    ("ColorScale", [0.0] * 4), ("ColorBias", [0.0] * 4), ("MiscScale", [0.0] * 4), ("MiscBias", [0.0] * 4),
    ("SimulationAttrCurveScale", [0.0] * 4), ("SimulationAttrCurveBias", [0.0] * 4),
    ("SubImageSize", [0.0] * 4), ("SizeBySpeed", [0.0] * 4),
    ("ConstantAcceleration", [0.0] * 3), ("OrbitOffsetBase", [0.0] * 3), ("OrbitOffsetRange", [0.0] * 3),
    ("OrbitFrequencyBase", [0.0] * 3), ("OrbitFrequencyRange", [0.0] * 3),
    ("OrbitPhaseBase", [0.0] * 3), ("OrbitPhaseRange", [0.0] * 3),
    ("GlobalVectorFieldScale", 0.0), ("GlobalVectorFieldTightness", -1.0),
    ("PerParticleVectorFieldScale", 0.0), ("PerParticleVectorFieldBias", 0.0),
    ("DragCoefficientScale", 0.0), ("DragCoefficientBias", 0.0),
    ("ResilienceScale", 0.0), ("ResilienceBias", 0.0),
    ("CollisionRadiusScale", 0.0), ("CollisionRadiusBias", 0.0), ("CollisionTimeBias", 0.0),
    ("CollisionRandomSpread", 0.0), ("CollisionRandomDistribution", 2.0),
    ("OneMinusFriction", 0.0), ("RotationRateScale", 0.0), ("CameraMotionBlurAmount", 0.0),
    ("ScreenAlignment", "PSA_Square"), ("LockAxisFlag", "EPAL_NONE"), ("PivotOffset", [-0.5, -0.5]),
    ("bUseVelocityForMotionBlur", False), ("bRemoveHMDRoll", False),
    ("MinFacingCameraBlendDistance", 0.0), ("MaxFacingCameraBlendDistance", 0.0),
)
# FGPUSpriteEmitterInfo's, the same way. The modules it points at (RequiredModule, SpawnModule, SpawnModules) and the
# baked distributions it carries (VectorFieldScale and the like, which the runtime reads through the curve textures)
# are printed as their names and their ranges, not compared.
EMITTER_DEFAULTS = (
    ("ConstantAcceleration", [0.0] * 3), ("PointAttractorPosition", [0.0] * 3), ("PointAttractorRadiusSq", 0.0),
    ("OrbitOffsetBase", [0.0] * 3), ("OrbitOffsetRange", [0.0] * 3),
    ("InvMaxSize", [0.0] * 2), ("InvRotationRateScale", 0.0), ("MaxLifetime", 0.0), ("MaxParticleCount", 0.0),
    ("ScreenAlignment", "PSA_Square"), ("LockAxisFlag", "EPAL_NONE"), ("bEnableCollision", False),
    ("CollisionMode", "SceneDepth"), ("bUseVelocityForMotionBlur", False), ("bRemoveHMDRoll", False),
    ("MinFacingCameraBlendDistance", 0.0), ("MaxFacingCameraBlendDistance", 0.0),
)
OBJECT_MEMBERS = ("RequiredModule", "SpawnModule", "SpawnPerUnitModule", "SpawnModules")
# What the editor's text form of the two structs holds besides the members above: a baked distribution (the runtime's
# curve, which Build writes from the same modules) and the local vector field's struct, printed but not compared.
SKIP_MEMBERS = ("VectorFieldScale", "DragCoefficient", "PointAttractorStrength", "Resilience", "LocalVectorField",
                "DynamicColor", "DynamicAlpha", "DynamicColorScale", "DynamicAlphaScale")

# ExportText writes floats with six decimals, so two values agree when they are that near.
EPSILON = 2e-6


# ------------------------------------------------------------------ the original's export
def systems(asset, version):
    """The exported packages of every particle system whose path under Content contains asset, oldest path first."""
    root = os.path.join(REFS[version], "_assets", "DDeception", "Content")
    needle = asset.replace("\\", "/").lower()
    found = []
    for folder, _, files in os.walk(root):
        for name in files:
            if not name.endswith(".json"):
                continue
            path = os.path.join(folder, name)
            rel = os.path.relpath(path, root).replace("\\", "/")[:-len(".json")]
            if needle not in rel.lower():
                continue
            with open(path, encoding="utf-8") as f:
                pkg = json.load(f)
            main = next((e for e in pkg["exports"] if e["name"] == name[:-len(".json")]), None)
            if main is not None and main["class"] == "ParticleSystem":
                found.append((rel, pkg))
    if not found:
        raise SystemExit("no particle system of pak_reference%s contains %r"
                         % ("" if version == 1 else "_2", asset))
    return sorted(found)


def gpu_emitters(pkg, name):
    """Every GPU emitter of the exported system: (emitter name, LOD level, module name, ResourceData, EmitterInfo)."""
    exports = {}
    for e in pkg["exports"]:
        exports["%s.%s" % (e["outer"], e["name"]) if e.get("outer") else e["name"]] = e
    out = []
    for key in exports[name]["props"]["Emitters"]:
        emitter = exports[key]["props"]
        for lod_key in emitter["LODLevels"]:
            lod = exports[lod_key]["props"]
            module = exports.get(lod.get("TypeDataModule") or "")
            if module is None or module["class"] != GPU:
                continue
            out.append((emitter.get("EmitterName", key), lod.get("Level", 0), module["name"],
                        module["props"].get("ResourceData", {}), module["props"].get("EmitterInfo", {})))
    return out


# ------------------------------------------------------------------ the editor's text form
def _scalar(token):
    token = token.strip()
    if token in ("True", "False"):
        return token == "True"
    try:
        return float(token) if "." in token or "e" in token or "E" in token else int(token)
    except ValueError:
        return token


def parse_ue_text(text):
    """UE's ExportText form of a struct as a dict; FColor, FVector* and the like come out as lists of their members
    in the order the text names them (an FColor is B, G, R, A, as the original's export reads it)."""
    value, end = _parse_group(text, 0)
    if end != len(text):
        raise ValueError("text left over at %d: %r" % (end, text[end:end + 40]))
    return value


def _parse_group(text, i):
    """A group '(...)': a dict of its Key=Value entries, or a list when it has none."""
    if text[i] != "(":
        return _parse_bare(text, i)
    i += 1
    entries, values = {}, []
    if text[i] == ")":
        return [], i + 1
    while True:
        key, j = None, i
        depth = 0
        while j < len(text):
            c = text[j]
            if c == '"':
                _, j = _parse_string(text, j)
                continue
            if c == "(":
                depth += 1
            elif c == ")":
                if depth == 0:
                    break
                depth -= 1
            elif c == "=" and depth == 0:
                key = text[i:j]
                break
            elif c == "," and depth == 0:
                break
            j += 1
        if key is not None:
            value, i = _parse_any(text, j + 1)
            entries[key] = value
        else:
            value, i = _parse_any(text, i)
            values.append(value)
        if text[i] == ",":
            i += 1
            continue
        if text[i] == ")":
            break
        raise ValueError("%r at %d" % (text[i], i))
    out = entries if entries else values
    if isinstance(out, dict) and set(out) in ({"B", "G", "R", "A"}, {"X", "Y"}, {"X", "Y", "Z"}, {"X", "Y", "Z", "W"}):
        out = [out[k] for k in (("B", "G", "R", "A") if "B" in out else "XYZW") if k in out]
    return out, i + 1


def _parse_string(text, i):
    out, i = [], i + 1
    while text[i] != '"':
        if text[i] == "\\":
            i += 1
        out.append(text[i])
        i += 1
    return "".join(out), i + 1


def _parse_bare(text, i):
    j = i
    while j < len(text) and text[j] not in ",)":
        j += 1
    return _scalar(text[i:j]), j


def _parse_any(text, i):
    if text[i] == "(":
        return _parse_group(text, i)
    if text[i] == '"':
        return _parse_string(text, i)
    return _parse_bare(text, i)


OURS_SCRIPT = r'''
import json
import unreal
LIB = unreal.WasamiCascadeLibrary
def main():
    out = []
    for path in %(paths)s:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            out.append({"asset": path, "missing": True})
            continue
        system = unreal.load_asset(path)
        for emitter in LIB.get_emitters(system):
            for lod in LIB.get_lod_levels(emitter):
                td = LIB.get_lod_type_data_module(lod)
                if td is None or td.get_class().get_name() != "ParticleModuleTypeDataGpu":
                    continue
                out.append({"asset": path, "emitter": LIB.get_property_text(emitter, "EmitterName"),
                            "level": int(LIB.get_property_text(lod, "Level")), "module": td.get_name(),
                            "resource": LIB.get_property_text(td, "ResourceData"),
                            "info": LIB.get_property_text(td, "EmitterInfo")})
    print("<<<gpu>>>" + json.dumps(out))
main()
'''


def ours(paths):
    """What UE 5.8 built for our rebuilt systems: {(asset, emitter name, LOD level): (ResourceData, EmitterInfo)}."""
    code = OURS_SCRIPT % {"paths": json.dumps(paths)}
    run = subprocess.run([sys.executable, UE_REMOTE, "-c", code], capture_output=True, text=True)
    if run.returncode or "<<<gpu>>>" not in run.stdout:
        raise SystemExit("the editor did not answer (%s)\n%s%s" % (run.returncode, run.stdout, run.stderr))
    out = {}
    for entry in json.loads(run.stdout.split("<<<gpu>>>", 1)[1].splitlines()[0]):
        if entry.get("missing"):
            print("!! %s is not made yet" % entry["asset"])
            continue
        out[(entry["asset"], entry["emitter"], entry["level"])] = (parse_ue_text(entry["resource"]),
                                                                  parse_ue_text(entry["info"]))
    return out


# ------------------------------------------------------------------ printing
def same(a, b):
    if isinstance(a, bool) or isinstance(b, bool):
        return bool(a) == bool(b)
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return abs(a - b) <= EPSILON * max(1.0, abs(a), abs(b))
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    return a == b


def _number(value):
    if isinstance(value, float):
        return ("%.6g" % value)
    return str(value)


def show(value, samples, full):
    """One member's value in a line: a curve of quantized FColors as its count and a few of its entries."""
    if isinstance(value, list) and value and isinstance(value[0], list) and len(value[0]) == 4 \
            and all(isinstance(v, int) for v in value[0]):
        if full or len(value) <= samples:
            picked = list(enumerate(value))
        else:
            step = (len(value) - 1) / (samples - 1.0)
            picked = [(int(round(i * step)), value[int(round(i * step))]) for i in range(samples)]
        # An FColor reads B,G,R,A; printed as R,G,B,A, the way the curve's channels are named.
        return "%d [%s]" % (len(value), " ".join("%d:%d,%d,%d,%d" % (i, c[2], c[1], c[0], c[3]) for i, c in picked))
    if isinstance(value, list):
        return "(%s)" % ", ".join(_number(v) for v in value)
    return _number(value)


def report(title, members, cook, mine, samples, full):
    print("  %s" % title)
    for key, default in members:
        theirs = cook.get(key, default)
        if mine is None:
            print("      %-32s %s" % (key, show(theirs, samples, full)))
            continue
        ours_value = mine.get(key, default)
        mark = "  " if same(theirs, ours_value) else "* "
        print("    %s%-32s %s" % (mark, key, show(theirs, samples, full)))
        if mark == "* ":
            print("      %-32s %s" % ("(ours)", show(ours_value, samples, full)))
    extra = [k for k in cook if k not in dict(members) and k not in OBJECT_MEMBERS and k not in SKIP_MEMBERS]
    for key in extra:
        print("      %-32s %s   (not compared)" % (key, show(cook[key], samples, full)))
    for key in OBJECT_MEMBERS:
        if key in cook:
            value = cook[key]
            names = [v.rsplit(".", 1)[-1] for v in value] if isinstance(value, list) else [value.rsplit(".", 1)[-1]]
            print("      %-32s %s" % (key, ", ".join(names)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("asset")
    ap.add_argument("--pak", type=int, default=2, choices=(1, 2))
    ap.add_argument("--ours", action="store_true", help="compare with what our editor built under /Game/DD")
    ap.add_argument("--full", action="store_true", help="every entry of the quantized curves")
    ap.add_argument("--samples", type=int, default=3, help="how many entries of a quantized curve to print")
    args = ap.parse_args()

    found = systems(args.asset, args.pak)
    mine = ours(["%s/%s" % (DD_ROOT, rel) for rel, _ in found]) if args.ours else None
    for rel, pkg in found:
        name = rel.rsplit("/", 1)[-1]
        emitters = gpu_emitters(pkg, name)
        print("\n== /Game/%s   %d GPU emitter%s" % (rel, len(emitters), "" if len(emitters) == 1 else "s"))
        for emitter, level, module, resource, info in emitters:
            print("\n-- %s (LOD %d, %s)" % (emitter, level, module))
            built = mine.get(("%s/%s" % (DD_ROOT, rel), emitter, level)) if mine is not None else None
            if mine is not None and built is None:
                print("   ours has no such GPU emitter")
            report("ResourceData", RESOURCE_DEFAULTS, resource, built[0] if built else None, args.samples, args.full)
            report("EmitterInfo", EMITTER_DEFAULTS, info, built[1] if built else None, args.samples, args.full)


if __name__ == "__main__":
    main()
