"""Dark Deception's hospital (Torment Therapy) Zone 1 and Zone 2 → UE-native stage data for this project's pipeline.

    python Tools/dd/prepare_stage.py

Reads the original's export (pak_reference_2, UE 4.24: the Steam version's pak converted to engine-independent files) and
writes Intermediate/Pipeline/dd/stage_ue.json — every mesh, texture and material the two maps use, and per zone the
placements, lights, reflection captures, fog, sky light, post process volumes and the gameplay actors, all in UE units
(cm, left-handed, z up) and /Game asset paths under /Game/DD.

Almost nothing is copied: the glTF and PNG of the export are imported straight from pak_reference_2 (its meshes keep
the original mesh space and their glTF material names are the mesh's material slot names). The exception is a mesh
whose sections share a material — UE's import would give those a single slot and shift every later slot index — whose
glTF is written again under Intermediate/Pipeline/dd/meshes/ with one material per section. Its .bin is not copied.

Env: PAK_REF2 — the export (default <repo>/pak_reference_2).
"""
import argparse
import datetime
import hashlib
import json
import math
import os
import re
import sys
import urllib.parse

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "dd")
MESH_OUT = os.path.join(OUT, "meshes")

GAME_ROOT = "/Game/DD"
CONTENT_PREFIX = "DDeception/Content/"
ENGINE_PREFIX = "Engine/Content/"

ZONES = (
    ("Zone1", "06_Hospital_Zone_01", "/Game/Stage/Maps/L_Hospital_Zone1"),
    ("Zone2", "06_Hospital_Zone_02", "/Game/Stage/Maps/L_Hospital_Zone2"),
)

# The original's master materials (their graphs are cooked away; the parameters are what the instances set).
# 'substance' is MM_Main_Substance: Albedo (sRGB) / Normal / Packed (R = occlusion, G = roughness, B = metallic).
MASTERS = {
    "/Game/Materials/MasterMaterials/MM_Main_Substance": "substance",
    "/Game/Materials/MasterMaterials/MM_Main_Substance_Emissive": "emissive",
    "/Game/Materials/MasterMaterials/MM_Main_Substance_AlphaColorMask": "alphamask",
    "/Game/Materials/MasterMaterials/MM_Main_Substance_Translucent": "translucent",
    "/Game/Materials/MasterMaterials/MM_Main_Substance_Glass_ColorMask": "glassmask",
    "/Game/Materials/01_Hotel/M_01_Hotel_Decals": "decal",
    "/Game/Materials/MasterMaterials/MM_Lit": "lit",
}
# Which texture parameter feeds which input of our rebuilt master materials.
TEX_KIND = {
    "Albedo": "albedo", "AlbedoT": "albedo", "Base Color": "albedo", "Texture": "albedo", "Diffuse": "albedo",
    "Normal": "normal", "NormalT": "normal",
    "Packed": "packed", "MasksT": "packed", "RMAO": "packed",
    "Emissive": "emissive",
}
# Actors placed by the level that the level build handles itself (meshes and lights are their own lists).
SKIP_ACTOR_CLASSES = {"StaticMeshActor", "PointLight", "SpotLight", "RectLight", "DirectionalLight", "SkyLight",
                      "ExponentialHeightFog", "SphereReflectionCapture", "BoxReflectionCapture", "Level", "World",
                      "WorldSettings", "RecastNavMesh", "LevelBounds"}
# The export leaves out every property that equals the component's archetype, so a component without a Mobility has
# its archetype's. UE's light actors make their light Stationary (UE 5.8 Light.cpp:190 APointLight, :244
# ADirectionalLight, SpotLight.cpp:27, RectLight.cpp:12 — the same since UE4), a sky light component is Stationary by
# itself (SkyLightComponent.cpp:312), AStaticMeshActor makes its mesh Static (StaticMeshActor.cpp:34) and any other
# component starts Movable (SceneComponent.cpp:124). Across all of the original's levels a PointLight's light leaves
# Mobility out 1,342 times, says Movable 779 and Static 193, and never says Stationary.
STATIONARY, STATIC, MOVABLE = ("EComponentMobility::Stationary", "EComponentMobility::Static",
                               "EComponentMobility::Movable")
NATIVE_MOBILITY = {
    ("PointLight", "LightComponent0"): STATIONARY, ("SpotLight", "LightComponent0"): STATIONARY,
    ("RectLight", "LightComponent0"): STATIONARY, ("DirectionalLight", "LightComponent0"): STATIONARY,
    ("SkyLight", "SkyLightComponent0"): STATIONARY, ("StaticMeshActor", "StaticMeshComponent0"): STATIC,
}
COMPONENT_MOBILITY = {"SkyLightComponent": STATIONARY}
# A light component's properties that are not the light (the transform, the attachment, editor bookkeeping).
LIGHT_BOOKKEEPING = ("LightGuid", "MapBuildDataId", "PreviewInfluenceRadius", "AttachParent", "UCSModifiedProperties",
                     "CreationMethod", "bNetAddressable")
RELATIVE = ("RelativeLocation", "RelativeRotation", "RelativeScale3D")
# The teleport's zones (BP_Power_Teleport_Zone): the class's Cube is a hidden, query-only mesh of the Teleport object
# channel, which the teleport's aim traces for. The level writes only what differs from the class's Cube_GEN_VARIABLE:
# Zone 1's floor mesh keeps the class's Z scale of 0.05, and the ambulances' roof boxes keep its engine cube as well.
TELEPORT_ZONE_CLASS = "BP_Power_Teleport_Zone_C"
TELEPORT_ZONE_COMPONENT = "Cube"
# Meshes the level build puts on Blueprint actors it places itself (the class leaves them unset, and the level export
# leaves the component's mesh out, as the class's own): Zone 2's lifts' LiftMesh (BP_06_LiftBase_Corner's and
# BP_06_Lift_03's / _04's), and Zone 2's altar (BP_01_Statue's StaticMeshComponent0) and the ring piece on it
# (BP_08_RingPiece's StaticMesh), and the defibrillators' two stands (BP_06_Defib's hospital_defibrillator_01 and _02, the
# same mesh). They come into `meshes` with their own materials.
CLASS_MESHES = ("/Game/Meshes/06_Hospital/hospital_zone_02_lifts_lift_01.hospital_zone_02_lifts_lift_01",
                "/Game/Meshes/06_Hospital/hospital_zone_02_lifts_lift_03.hospital_zone_02_lifts_lift_03",
                "/Game/Meshes/06_Hospital/hospital_zone_02_lifts_lift_04.hospital_zone_02_lifts_lift_04",
                "/Game/Meshes/00_Ballroom/ring_statue.ring_statue",
                "/Game/Meshes/Ring_Assets/ring_pieces/ring_piece06.ring_piece06",
                "/Game/Meshes/06_Hospital/hospital_defibrillator_01.hospital_defibrillator_01")
# Materials of meshes that are not the stage's static meshes: the garage lifts' skinned mesh (hospital_garage_lift_anim,
# its glTF's materials in order; dd_skeletal imports the mesh and puts these on its slots by name) and the saw traps'
# (hospital_sawTrap_*_anim: the blade, the stained metal, and short_01's diamond plate), and the ones the altar's and the
# ring piece's components put over their meshes' own (OverrideMaterials: the placed altar's, and
# BP_08_RingPiece's StaticMesh_GEN_VARIABLE's).
CLASS_MATERIALS = ("/Game/Materials/06_Hospital/M_06_Hospital_MetalPanel_04.M_06_Hospital_MetalPanel_04",
                   "/Game/Materials/06_Hospital/M_06_Hospital_Concrete_06_Painted1.M_06_Hospital_Concrete_06_Painted1",
                   "/Game/Materials/06_Hospital/M_06_Hospital_MetalBrushed_02.M_06_Hospital_MetalBrushed_02",
                   "/Game/Materials/07_FunPlace/M_07_TP_DiamondPlate.M_07_TP_DiamondPlate",
                   "/Game/Materials/06_Hospital/M_06_Hospital_SawBlade.M_06_Hospital_SawBlade",
                   "/Game/Materials/06_Hospital/M_06_Hospital_MetalStained.M_06_Hospital_MetalStained",
                   "/Game/Materials/06_Hospital/M_06_Hospital_Metal_DiamondPlate.M_06_Hospital_Metal_DiamondPlate",
                   "/Game/Materials/00_Ballroom/MM_00_Ballroom_Ring_Altar_Metal.MM_00_Ballroom_Ring_Altar_Metal",
                   "/Game/Meshes/Ring_Assets/ring_pieces/M_ring_metal.M_ring_metal",
                   "/Game/Meshes/Ring_Assets/ring_pieces/M_ring_metal2.M_ring_metal2")
# Component properties of a placement worth carrying over (the rest is either the transform or editor bookkeeping).
# bCastShadowAsTwoSided: Zone 1's five merged stage meshes (tiles_tile_01/02/03, parking, tunnel) are one-sided rooms
# seen from inside; without it their ceilings let the sun and the next room's lights through, both in the renderer's
# shadows and in Lightmass.
KEEP_COMPONENT_PROPS = ("bVisible", "bHiddenInGame", "CastShadow", "bCastDynamicShadow", "bCastStaticShadow",
                        "bCastShadowAsTwoSided", "bReceivesDecals", "Mobility", "CollisionProfileName", "CustomDepthStencilValue",
                        "bRenderCustomDepth", "LightmassSettings", "OverriddenLightMapRes", "bOverrideLightMapRes")


def jload(rel):
    with open(os.path.join(REF, rel), encoding="utf-8") as f:
        return json.load(f)


# ------------------------------------------------------------------------------------------------ names and paths
def safe_segment(s):
    """A package path segment UE accepts: [A-Za-z0-9_-], anything else '_', with a short hash when changed."""
    t = re.sub(r"[^A-Za-z0-9_-]", "_", s)
    if t != s:
        t = "%s_%s" % (t.strip("_") or "x", hashlib.md5(s.encode("utf-8")).hexdigest()[:6])
    return t


def asset_of(object_path):
    """'/Game/Meshes/06_Hospital/x.x' → '/Game/DD/Meshes/06_Hospital/x'. None for the engine's own assets, which are
    used where they are instead of being imported."""
    if not object_path:
        return None
    p = object_path.split(".")[0]
    if not p.startswith("/Game/"):
        return None
    return GAME_ROOT + "/" + "/".join(safe_segment(s) for s in p[len("/Game/"):].split("/"))


def texture_asset(png):
    """'DDeception/Content/Textures/06_Hospital/x.png' → '/Game/DD/Textures/06_Hospital/x'."""
    rel = png.replace("\\", "/")
    if rel.startswith(CONTENT_PREFIX):
        rel = rel[len(CONTENT_PREFIX):]
        root = GAME_ROOT
    elif rel.startswith(ENGINE_PREFIX):
        rel = rel[len(ENGINE_PREFIX):]
        root = GAME_ROOT + "/_Engine"
    else:
        return None
    rel = re.sub(r"\.[A-Za-z0-9]+$", "", rel)
    return root + "/" + "/".join(safe_segment(s) for s in rel.split("/"))


# ------------------------------------------------------------------------------------------------ transforms
def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return [aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz]


def quat_rotate(q, v):
    x, y, z, w = q
    tx, ty, tz = 2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0])
    return [v[0] + w * tx + (y * tz - z * ty), v[1] + w * ty + (z * tx - x * tz), v[2] + w * tz + (x * ty - y * tx)]


def pyr_quat(pyr):
    """UE FRotator [pitch, yaw, roll] in degrees → quaternion (x, y, z, w), as FRotator::Quaternion()."""
    p, y, r = (math.radians(v) * 0.5 for v in pyr)
    sp, cp, sy, cy, sr, cr = math.sin(p), math.cos(p), math.sin(y), math.cos(y), math.sin(r), math.cos(r)
    return [cr * sp * sy - sr * cp * cy, -cr * sp * cy - sr * cp * sy, cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy]


def compose(parent, rel_loc, rel_pyr, rel_scale):
    """A relative transform under an absolute one → absolute (UE FTransform composition)."""
    pq, ps, pl = parent["quat_xyzw"], parent["scale"], parent["location"]
    q = quat_mul(pq, pyr_quat(rel_pyr))
    scaled = [rel_loc[i] * ps[i] for i in range(3)]
    rotated = quat_rotate(pq, scaled)
    return {"location": [round(pl[i] + rotated[i], 4) for i in range(3)],
            "quat_xyzw": [round(v, 8) for v in q],
            "scale": [round(ps[i] * rel_scale[i], 6) for i in range(3)]}


def world_of(entry):
    w = entry.get("world")
    if not w:
        return None
    return {"location": [round(v, 4) for v in w["location"]],
            "quat_xyzw": [round(v, 8) for v in w["quat_xyzw"]],
            "scale": [round(v, 6) for v in w["scale"]]}


# ------------------------------------------------------------------------------------------------ glTF
def sectioned_gltf(src, rel, slots, problems):
    """The export's glTF written again with one material per section, under MESH_OUT; returns the new file's path.

    The original's StaticMesh keeps a slot per section even when two sections use the same material, but a glTF makes
    them point at one material and UE's import then makes one slot — every later slot index would shift. The section's
    material name is kept in the new name ('M_06_Hospital_Floor_02__3') so the slots stay readable. The .bin is not
    copied: the new glTF points back at the export's through a relative URI."""
    with open(src, encoding="utf-8") as f:
        gltf = json.load(f)
    names, n = [], 0
    for mesh in gltf.get("meshes", []):
        for prim in mesh.get("primitives", []):
            prim["material"] = n
            base = (slots[n] or "").split(".")[-1] if n < len(slots) else ""
            names.append({"name": "%s__%d" % (safe_segment(base or "Slot"), n)})
            n += 1
    if n != len(slots):
        problems.append("%s: %d glTF primitives, %d material slots" % (rel, n, len(slots)))
    gltf["materials"] = names
    for k in ("images", "textures", "samplers"):
        gltf.pop(k, None)
    for k in ("extensionsUsed", "extensionsRequired"):
        if k in gltf:
            gltf[k] = [e for e in gltf[k] if not e.startswith(("KHR_materials", "KHR_texture"))]
            if not gltf[k]:
                del gltf[k]
    dst = os.path.join(MESH_OUT, *rel.split("/")[1:])     # rel starts with '_meshes_gltf/'
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    for buf in gltf.get("buffers", []):
        uri = buf.get("uri")
        if uri and not uri.startswith("data:"):
            target = os.path.join(os.path.dirname(src), urllib.parse.unquote(uri).replace("/", os.sep))
            back = os.path.relpath(target, os.path.dirname(dst)).replace(os.sep, "/")
            buf["uri"] = urllib.parse.quote(back, safe="/._-")
    with open(dst, "w", encoding="utf-8") as f:
        json.dump(gltf, f, separators=(",", ":"))
    return dst


# ------------------------------------------------------------------------------------------------ the export's index
class Export:
    """The parts of pak_reference_2 this reads, loaded once."""

    def __init__(self):
        self.meshes = jload("_meshes.json")
        self.materials = jload("_materials.json")
        textures = jload("_textures.json")
        self.textures = {}
        for v in (textures.values() if isinstance(textures, dict) else textures):
            if isinstance(v, dict) and v.get("png"):
                self.textures[v["png"]] = v
        self.assets = {}
        for dirpath, _dirs, files in os.walk(os.path.join(REF, "_assets")):
            for f in files:
                if f.endswith(".json"):
                    self.assets.setdefault(f[:-5], os.path.join(dirpath, f))
        self._bp_cache = {}

    def mesh(self, object_path):
        return self.meshes.get(object_path) or self.meshes.get(object_path.split(".")[0])

    def lightmap(self, mesh_path):
        """The StaticMesh's own (LightMapResolution, LightMapCoordinateIndex) from its package, with UStaticMesh's
        defaults (4 and 0) for what the export leaves out."""
        file = os.path.join(REF, "_assets", CONTENT_PREFIX.rstrip("/"), *mesh_path[len("/Game/"):].split("/")) + ".json"
        props = {}
        if os.path.exists(file):
            with open(file, encoding="utf-8") as f:
                props = next((x.get("props") or {} for x in json.load(f).get("exports", []) if x.get("class") == "StaticMesh"), {})
        return props.get("LightMapResolution", 4), props.get("LightMapCoordinateIndex", 0)

    def material(self, object_path):
        return self.materials.get(object_path) or self.materials.get(object_path.split(".")[0])

    def blueprint(self, class_name):
        """The exported package of a Blueprint class ('PointLight_Blueprint_C' → its _assets json), or None."""
        name = class_name[:-2] if class_name.endswith("_C") else class_name
        if name not in self._bp_cache:
            path = self.assets.get(name)
            data = None
            if path:
                with open(path, encoding="utf-8") as f:
                    data = json.load(f)
            self._bp_cache[name] = data
        return self._bp_cache[name]

    @staticmethod
    def parent_class(bp, class_name):
        """A Blueprint's parent class — a Blueprint ('BP_08_RingPiece_C') or a native class ('PointLight') — read from
        the class's super, else guessed from the import table."""
        for x in bp.get("exports", []):
            if x.get("name") == class_name and x.get("super"):
                return x["super"].rsplit(".", 1)[-1]
        imports = [i for i in (bp.get("imports") or []) if isinstance(i, str)]
        names = set(imports)
        for i in imports:
            if i.startswith("/Game/"):
                parent = i.rsplit("/", 1)[-1] + "_C"
                if parent in names and parent != class_name:
                    return parent
        return None

    def light_template(self, class_name, comp_name, depth=0):
        """The defaults of a light component of a Blueprint class: the component of that name, else any light of the
        class, else the same lookup on its parent class (a child Blueprint keeps the parent's components)."""
        bp = self.blueprint(class_name)
        if not bp or depth > 4:
            return None
        exports = bp.get("exports", [])
        for want in (comp_name, comp_name + "_GEN_VARIABLE"):
            for x in exports:
                if x.get("name") == want and str(x.get("class", "")).endswith("LightComponent"):
                    return x.get("props") or {}
        for x in exports:
            if str(x.get("class", "")).endswith("LightComponent"):
                return x.get("props") or {}
        parent = self.parent_class(bp, class_name)
        return self.light_template(parent, comp_name, depth + 1) if parent and parent.endswith("_C") else None

    def component_template(self, class_name, comp_name):
        """The class defaults of a Blueprint's construction-script component (its <name>_GEN_VARIABLE), or None."""
        bp = self.blueprint(class_name)
        for x in (bp or {}).get("exports", []):
            if x.get("name") == comp_name + "_GEN_VARIABLE":
                return x.get("props") or {}
        return None

    def default_mobility(self, class_name, comp_name, comp_class, depth=0):
        """The Mobility of an actor's component when the level leaves it out: its archetype's. In a Blueprint that is
        the class's own record of the component (the CDO's subobject, or the construction script's <name>_GEN_VARIABLE),
        then whatever that record was made from — a native actor's subobject, a component class's defaults, or the
        parent Blueprint's record."""
        if not class_name or not class_name.endswith("_C"):
            return NATIVE_MOBILITY.get((class_name, comp_name)) or COMPONENT_MOBILITY.get(comp_class, MOVABLE)
        bp = self.blueprint(class_name)
        if not bp or depth > 6:
            return COMPONENT_MOBILITY.get(comp_class, MOVABLE)
        for x in bp.get("exports", []):
            if x.get("name") == comp_name + "_GEN_VARIABLE" or (
                    x.get("name") == comp_name and (x.get("outer") or "").startswith("Default__")):
                if (x.get("props") or {}).get("Mobility"):
                    return x["props"]["Mobility"]
                native = re.match(r"^/Script/\w+\.Default__(\w+?)(?:\.(\w+))?$", x.get("template") or "")
                if native and native.group(2):
                    return self.default_mobility(native.group(1), native.group(2), comp_class, depth + 1)
                if native:
                    return COMPONENT_MOBILITY.get(native.group(1), MOVABLE)
                break                                  # made from the parent Blueprint's record
        return self.default_mobility(self.parent_class(bp, class_name), comp_name, comp_class, depth + 1)


# ------------------------------------------------------------------------------------------------ materials
def resolve_material(ex, object_path, out, problems):
    """One material of the export → the entry the editor needs: which master it is built on, its textures (by our own
    kind), scalars, vectors and the blend settings. Parameters resolve child → parent, the child winning; the root
    Material's own texture parameters are the last resort (an instance that sets none draws with those)."""
    key = object_path.split(".")[0]
    if key in out:
        return key
    chain, cur, seen = [], key, set()
    while cur and cur not in seen:
        seen.add(cur)
        m = ex.material(cur)
        if not m:
            problems.append("material not in _materials.json: " + cur)
            break
        chain.append((cur, m))
        cur = (m.get("parent") or "").split(".")[0] or None
    if not chain:
        return None
    root_path, root = chain[-1][0], chain[-1][1]
    entry = {
        "asset": asset_of(key), "source": key, "root": root_path, "master": MASTERS.get(root_path, "other"),
        "textures": {}, "scalars": {}, "vectors": {}, "switches": {},
        "blend": root.get("blend_mode"), "shading": root.get("shading_model"),
        "twoSided": bool(root.get("two_sided")), "clip": root.get("opacity_mask_clip"),
        "unknownParams": [],
    }
    # the root Material's defaults first, then each instance from the root down, so the closest child wins
    for t in (root.get("texture_expressions") or []):
        if t.get("param") and t.get("png"):
            entry["textures"].setdefault(t["param"], t["png"])
    for path, m in reversed(chain):
        for name, t in (m.get("textures") or {}).items():
            png = t.get("png") if isinstance(t, dict) else None
            if png:
                entry["textures"][name] = png
        entry["scalars"].update(m.get("scalars") or {})
        entry["vectors"].update(m.get("vectors") or {})
        over = m.get("base_property_overrides") or {}
        if over.get("bOverride_BlendMode") and over.get("BlendMode"):
            entry["blend"] = over["BlendMode"]
        if over.get("bOverride_TwoSided"):
            entry["twoSided"] = bool(over.get("TwoSided"))
        if over.get("OpacityMaskClipValue") is not None:
            entry["clip"] = over["OpacityMaskClipValue"]
        if over.get("bOverride_ShadingModel") and over.get("ShadingModel"):
            entry["shading"] = over["ShadingModel"]
    kinds = {}
    for name, png in entry["textures"].items():
        kind = TEX_KIND.get(name)
        if kind is None:
            entry["unknownParams"].append(name)
            continue
        kinds.setdefault(kind, png)
    entry["kinds"] = kinds
    out[key] = entry
    return key


def note_texture(ex, png, kind, textures, problems):
    if not png or png in textures:
        return
    info = ex.textures.get(png) or {}
    path = os.path.join(REF, png.replace("/", os.sep))
    if not os.path.exists(path):
        problems.append("texture png missing: " + png)
        return
    textures[png] = {
        "asset": texture_asset(png), "file": path, "kind": kind,
        "srgb": info.get("srgb"), "compression": info.get("compression"), "lodGroup": info.get("lod_group"),
        "format": info.get("format"), "size": info.get("exported_size"),
    }


# ------------------------------------------------------------------------------------------------ one zone
def note_mesh(ex, mesh_path, info, meshes, problems):
    """Adds a mesh the level uses to `meshes` once (re-sectioning its glTF when sections share a material, with its
    lightmap settings) and returns its key."""
    key = mesh_path.split(".")[0]
    if key in meshes:
        return key
    gltf = info.get("gltf")
    file = os.path.join(REF, gltf.replace("/", os.sep)) if gltf else None
    if file and not os.path.exists(file):
        problems.append("mesh gltf missing: " + gltf)
        file = None
    slots = [s.get("material") for s in (info.get("material_slots") or [])]
    resectioned = False
    if file and len(set(slots)) != len(slots):     # sections sharing a material would collapse into one slot
        try:
            file = sectioned_gltf(file, gltf, slots, problems)
            resectioned = True
        except Exception as e:  # noqa: BLE001
            problems.append("could not re-section %s: %s" % (gltf, e))
    lightmap_res, lightmap_uv = ex.lightmap(key) if key.startswith("/Game/") else (None, None)
    meshes[key] = {
        "asset": asset_of(key), "source": key, "file": file, "engine": not key.startswith("/Game/"),
        "slots": slots, "resectioned": resectioned,
        "lightmapUv": lightmap_uv, "lightmapResolution": lightmap_res, "bodySetup": bool(info.get("body_setup")),
        "bounds": info.get("bounds"),
    }
    return key


def slot_materials(ex, slots, over, materials, problems):
    """Each slot's material key: the component's override, else the mesh's own (None for none or UE's grid)."""
    used = []
    for i in range(max(len(slots), len(over))):
        m = (over[i] if i < len(over) else None) or (slots[i] if i < len(slots) else None)
        if not m or "WorldGridMaterial" in m:
            used.append(None)
            continue
        used.append(resolve_material(ex, m, materials, problems) if m.startswith("/Game/") else None)
    return used


def teleport_zones(ex, prefix, full, actor_class, meshes, materials, problems):
    """Every teleport zone's Cube as a placement: the class's Cube_GEN_VARIABLE under what the level changes (its mesh,
    visibility, materials), the class's relative transform where the level has none, and the class's collision as
    `collision` (BP_Power_Teleport_Zone: object type Teleport, query only, an overlap with every engine channel)."""
    template = ex.component_template(TELEPORT_ZONE_CLASS, TELEPORT_ZONE_COMPONENT)
    if template is None:
        problems.append("no %s in %s" % (TELEPORT_ZONE_COMPONENT, TELEPORT_ZONE_CLASS))
        return []
    body = template.get("BodyInstance") or {}
    collision = {
        "objectType": body.get("ObjectType"), "enabled": body.get("CollisionEnabled"),
        "responses": {r["Channel"]: r["Response"]
                      for r in (body.get("CollisionResponses") or {}).get("ResponseArray", [])},
    }
    out = []
    for e in full:
        path = e.get("path") or ""
        parts = path[len(prefix):].split(".") if path.startswith(prefix) else []
        if (len(parts) != 2 or parts[1] != TELEPORT_ZONE_COMPONENT
                or actor_class.get(parts[0]) != TELEPORT_ZONE_CLASS or not e.get("world")):
            continue
        own = e.get("props") or {}
        world = world_of(e)                            # holds the level's relative transform only
        rel = {k: template[k] for k in RELATIVE if k in template and k not in own}
        if rel:
            world = compose(world, rel.get("RelativeLocation") or [0, 0, 0], rel.get("RelativeRotation") or [0, 0, 0],
                            rel.get("RelativeScale3D") or [1, 1, 1])
        mesh_path = own.get("StaticMesh") or template.get("StaticMesh")
        info = ex.mesh(mesh_path) if mesh_path else None
        if not info:
            problems.append("teleport zone mesh not in _meshes.json: %s (%s)" % (mesh_path, path))
            continue
        key = note_mesh(ex, mesh_path, info, meshes, problems)
        over = own.get("OverrideMaterials") or template.get("OverrideMaterials") or []
        props = {
            "bVisible": own.get("bVisible", template.get("bVisible", True)),
            "bHiddenInGame": own.get("bHiddenInGame", template.get("bHiddenInGame", False)),
            "CastShadow": own.get("CastShadow", template.get("CastShadow", True)),
            "Mobility": own.get("Mobility") or ex.default_mobility(TELEPORT_ZONE_CLASS, TELEPORT_ZONE_COMPONENT,
                                                                   e.get("class") or "StaticMeshComponent"),
        }
        out.append({
            "path": path, "actor": parts[0], "actorClass": TELEPORT_ZONE_CLASS, "mesh": key,
            "materials": slot_materials(ex, meshes[key]["slots"], over, materials, problems),
            "world": world, "props": props, "collision": collision,
        })
    return out


def brush_volume(by_path, component_path, problems):
    """A volume's brush (BlockingVolume, TriggerVolume): the box of its one convex element and the brush component's
    collision and Mobility as the level writes them (left-out ones are the volume class's own)."""
    comp = (by_path.get(component_path) or {}).get("props") or {}
    body = ((by_path.get(comp.get("BrushBodySetup") or "") or {}).get("props") or {}).get("AggGeom") or {}
    elems = body.get("ConvexElems") or []
    if len(elems) != 1:
        problems.append("%s: %d convex elements in the brush" % (component_path, len(elems)))
    out = {"brushBox": elems[0]["ElemBox"][:6] if elems else None}
    instance = comp.get("BodyInstance") or {}
    collision = {k: instance[k] for k in ("CollisionProfileName", "CollisionEnabled", "ObjectType") if k in instance}
    responses = (instance.get("CollisionResponses") or {}).get("ResponseArray")
    if responses:
        collision["responses"] = {r["Channel"]: r["Response"] for r in responses}
    out["brushCollision"] = collision
    if comp.get("Mobility"):
        out["brushMobility"] = comp["Mobility"]
    return out


def read_zone(ex, map_name, level_path, meshes, textures, materials, problems):
    scene = jload("_levels/%s.scene.json" % map_name)
    full = jload("_levels/%s.full.json" % map_name)
    prefix = map_name + ".PersistentLevel."
    by_path = {e["path"]: e for e in full if e.get("path")}
    actor_class, actor_world = {}, {}
    for e in full:
        p = e.get("path") or ""
        if p.startswith(prefix):
            rest = p[len(prefix):]
            if "." not in rest:
                actor_class[rest] = e.get("class")
    for e in full:                                   # an actor's transform lives on its root component
        p = e.get("path") or ""
        if p.startswith(prefix) and e.get("world"):
            actor = p[len(prefix):].split(".")[0]
            actor_world.setdefault(actor, world_of(e))

    def actor_of(path):
        return path[len(prefix):].split(".")[0] if path.startswith(prefix) else path

    # ---------------------------------------------------------------- meshes
    placements = []
    for e in scene["static_meshes"]:
        mesh_path = e.get("mesh")
        info = ex.mesh(mesh_path) if mesh_path else None
        if not info:
            problems.append("mesh not in _meshes.json: %s (%s)" % (mesh_path, e["path"]))
            continue
        key = note_mesh(ex, mesh_path, info, meshes, problems)
        if actor_class.get(actor_of(e["path"])) == TELEPORT_ZONE_CLASS:
            continue                                   # teleport_zones() places these, with the class's collision
        used = slot_materials(ex, meshes[key]["slots"], e.get("override_materials") or [], materials, problems)
        props = {k: v for k, v in ((by_path.get(e["path"]) or {}).get("props") or {}).items() if k in KEEP_COMPONENT_PROPS}
        if not props.get("Mobility"):
            props["Mobility"] = ex.default_mobility(actor_class.get(actor_of(e["path"])), e["path"].rsplit(".", 1)[-1],
                                                    by_path.get(e["path"], {}).get("class") or "StaticMeshComponent")
        body = ((by_path.get(e["path"]) or {}).get("props") or {}).get("BodyInstance") or {}
        if isinstance(body, dict) and body.get("CollisionProfileName"):
            props["CollisionProfileName"] = body["CollisionProfileName"]
        placements.append({
            "path": e["path"], "actor": actor_of(e["path"]), "actorClass": actor_class.get(actor_of(e["path"])),
            "mesh": key, "materials": used, "world": world_of(e), "props": props,
        })
    zones = teleport_zones(ex, prefix, full, actor_class, meshes, materials, problems)
    placements.extend(zones)

    # ---------------------------------------------------------------- lights (the sky light comes in this list too)
    lights, sky = [], None
    for e in scene["lights"]:
        # the component's own values in the level (scene.json's "light" is only a part of them: it has no
        # SoftSourceRadius, LightingChannels, SourceWidth/Height or MaxDrawDistance)
        own = dict((by_path.get(e["path"]) or {}).get("props") or {}) or dict(e.get("light") or {})
        actor = actor_of(e["path"])
        owner = actor_class.get(actor)
        comp_name = e["path"].rsplit(".", 1)[-1]
        world = world_of(e)
        props = own
        if owner and owner.endswith("_C"):            # a light inside a Blueprint: the class defaults, then the level's
            found = ex.light_template(owner, comp_name)
            if found is None:
                problems.append("no light defaults for %s in %s" % (e["path"], owner))
            else:
                props = {k: v for k, v in found.items() if k not in RELATIVE}
                props.update(own)
                # the export's world holds the level's relative transform only; the class's counts where the level
                # has none of its own
                rel = {k: found[k] for k in RELATIVE if k in found and k not in own}
                if world and rel:
                    if any(k in own for k in RELATIVE):
                        problems.append("%s: relative transform split between the level and %s" % (e["path"], owner))
                    world = compose(world, rel.get("RelativeLocation") or [0, 0, 0],
                                    rel.get("RelativeRotation") or [0, 0, 0], rel.get("RelativeScale3D") or [1, 1, 1])
        if not props.get("Mobility"):
            props["Mobility"] = ex.default_mobility(owner, comp_name, e["class"])
        for k in LIGHT_BOOKKEEPING + RELATIVE:
            props.pop(k, None)
        if e["class"] == "SkyLightComponent":
            sky = {"path": e["path"], "world": world, "props": props}
            continue
        lights.append({"path": e["path"], "actor": actor, "actorClass": owner, "class": e["class"],
                       "world": world, "props": props})

    # ---------------------------------------------------------------- environment
    captures, fog, post = [], None, []
    for e in scene["other_placed"]:
        c, props = e.get("class"), dict(e.get("props") or {})
        if c in ("SphereReflectionCaptureComponent", "BoxReflectionCaptureComponent"):
            cube = props.get("Cubemap")
            captures.append({"path": e["path"], "kind": "sphere" if c.startswith("Sphere") else "box",
                             "world": world_of(e), "brightness": props.get("Brightness"),
                             "sourceType": props.get("ReflectionSourceType"), "cubemap": cube,
                             "influenceRadius": props.get("InfluenceRadius")})
        elif c == "ExponentialHeightFogComponent":
            fog = {"path": e["path"], "world": world_of(e),
                   "props": {k: v for k, v in props.items() if not k.startswith("Relative")}}
    for e in full:
        if e.get("class") == "PostProcessVolume":
            props = e.get("props") or {}
            post.append({"path": e["path"], "unbound": bool(props.get("bUnbound")),
                         "priority": props.get("Priority"), "blendWeight": props.get("BlendWeight"),
                         "settings": props.get("Settings") or {}})

    # the sky light's cubemap and the captures' are textures of the export too
    for holder in ([sky] if sky else []) + captures:
        cube = (holder.get("props") or {}).get("Cubemap") if "props" in holder else holder.get("cubemap")
        if isinstance(cube, str) and cube.startswith("/Game/"):
            info = None
            for png, t in ex.textures.items():
                if png.endswith("/" + cube.split("/")[-1].split(".")[0] + ".png") or png.endswith(
                        "/" + cube.split("/")[-1].split(".")[0] + ".hdr"):
                    info = png
                    break
            if info:
                note_texture(ex, info, "cubemap", textures, problems)
                holder["cubemapFile"] = info

    # ---------------------------------------------------------------- gameplay actors (everything else)
    actors = []
    for name, cls in sorted(actor_class.items()):
        if cls in SKIP_ACTOR_CLASSES or cls is None:
            continue
        own = (by_path.get(prefix + name) or {}).get("props") or {}
        props = {k: v for k, v in own.items()
                 if not isinstance(v, (list, dict)) and not str(v).startswith(map_name + ".")}
        for k, v in own.items():                     # a map keyed by the level's actors, by their names
            if isinstance(v, dict) and v and all(str(x).startswith(prefix) for x in v):
                props[k] = {actor_of(x): y for x, y in v.items()}   # BP_MapTexture_MultiFloor's Map
        entry = {"name": name, "class": cls, "world": actor_world.get(name), "props": props}
        root = (by_path.get(own.get("RootComponent") or "") or {}).get("props") or {}
        if root.get("AttachParent"):
            entry["attachParent"] = actor_of(root["AttachParent"])   # moves with it (the ambulances, the spikes)
        if own.get("BrushComponent"):
            entry.update(brush_volume(by_path, own["BrushComponent"], problems))
        actors.append(entry)

    return {
        "map": map_name, "level": level_path, "placements": placements, "lights": lights, "captures": captures,
        "fog": fog, "sky": sky, "postProcess": post, "actors": actors,
        "counts": {"placements": len(placements), "teleportZones": len(zones), "lights": len(lights), "captures": len(captures),
                   "postProcess": len(post), "actors": len(actors)},
    }


# ------------------------------------------------------------------------------------------------ main
def main():
    global MESH_OUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default=OUT, help="where stage_ue.json is written")
    args = parser.parse_args()
    if not os.path.isdir(REF):
        print("missing: %s (set PAK_REF2)" % REF, file=sys.stderr)
        return 1
    MESH_OUT = os.path.join(args.out, "meshes")

    ex = Export()
    meshes, textures, materials, problems = {}, {}, {}, []
    zones = {}
    for key, map_name, level in ZONES:
        zones[key] = read_zone(ex, map_name, level, meshes, textures, materials, problems)
    for mesh_path in CLASS_MESHES:
        info = ex.mesh(mesh_path)
        if not info:
            problems.append("class mesh not in _meshes.json: " + mesh_path)
            continue
        key = note_mesh(ex, mesh_path, info, meshes, problems)
        slot_materials(ex, meshes[key]["slots"], [], materials, problems)
    slot_materials(ex, list(CLASS_MATERIALS), [], materials, problems)

    for m in materials.values():
        for kind, png in m["kinds"].items():
            note_texture(ex, png, kind, textures, problems)

    out = {
        "generated": datetime.datetime.now().isoformat(timespec="seconds"),
        "source": {"ref": REF, "maps": [z[1] for z in ZONES]},
        "meshes": meshes, "textures": textures, "materials": materials, "zones": zones, "problems": problems,
    }
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "stage_ue.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1)

    print("meshes %d (engine %d, 区画を分け直した %d), textures %d, materials %d"
          % (len(meshes), sum(1 for m in meshes.values() if m["engine"]),
             sum(1 for m in meshes.values() if m.get("resectioned")), len(textures), len(materials)))
    for key, z in zones.items():
        print("  %s %s: %s" % (key, z["map"], ", ".join("%s %d" % (k, v) for k, v in z["counts"].items())))
    masters = {}
    for m in materials.values():
        masters[m["master"]] = masters.get(m["master"], 0) + 1
    print("  マスター:", masters)
    print("  problems %d" % len(problems))
    for p in problems[:20]:
        print("   ! " + p)
    return 0


if __name__ == "__main__":
    sys.exit(main())
