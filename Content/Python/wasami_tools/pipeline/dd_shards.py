"""The soul shard (AWasamiShard, after the latest version's BP_Shard): what it shows and plays.

  SM_WasamiMochi   this game's shard, the Wasami mochi (SourceArt/Wasami/wasami_mochi.glb, the WebGL version's 6000
                   triangles with its three 1024² JPEGs), under /Game/Wasami/Shard with its textures and MI_WasamiMochi
  M_DD_WasamiMochi the mochi's master: the glTF's metallic-roughness material, two-sided, glowing with its own base
                   colour × Glow (the WebGL version's game.shard.glow, which keeps it legible on a dark stage)
  M_DD_MapMark     the estimated master of the minimap's marks; M_Shard (the shard's plane) is an instance of it
  the pickup       Soul_Shard_Pickup_v2 and its cue (a modulator of pitch 0.9 – 1.1), OnlyFew, and
                   BP_CameraShake_ShardCollect

Sources: pak_reference_2 (UE 4.24, the latest version), which the shards follow; the pickup's sound, cue and shake are
the same in both versions. The collect flash (P_ky_flash3) comes later.
"""
import json
import os
import struct

import unreal

from wasami_tools.pipeline import dd_assets, dd_stage, paths

EAL = unreal.EditorAssetLibrary

# (pak_reference version, the original's path under /Game).
SOUNDS = ((2, "Audio/SharedGameplay/Soul_Shard_Pickup_v2"),)
SOUND_CONCURRENCIES = ((2, "Audio/OnlyFew"),)       # PlaySound2D's concurrency: one pickup sound at a time
SOUND_CUES = ((2, "Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue"),)  # after the waves it plays
CAMERA_SHAKES = ((2, "Blueprints/Shared/BP_CameraShake_ShardCollect"),)

MOCHI_SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "wasami_mochi.glb")
MOCHI_FOLDER = paths.WASAMI_ROOT + "/Shard"
MOCHI_MESH = MOCHI_FOLDER + "/SM_WasamiMochi"
MOCHI_MATERIAL = MOCHI_FOLDER + "/MI_WasamiMochi"
MOCHI_MASTER = "/Game/Pipeline/Materials/M_DD_WasamiMochi"
MOCHI_GLOW = 0.3
# The glb's embedded pictures, taken out to be imported: (the glTF material's texture, our parameter and texture name,
# sRGB, compression, LOD group). glTF's normal maps point Y up; UE's point it down, so the green channel is flipped.
MOCHI_TEXTURES = (
    ("baseColorTexture", "BaseColor", True, None, None),
    ("metallicRoughnessTexture", "MetallicRoughness", False, None, None),
    ("normalTexture", "Normal", False, "TC_Normalmap", "TEXTUREGROUP_WorldNormalMap"),
)
MOCHI_EXTRACTED = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "shard")

MAP_MARK_MASTER = "/Game/Pipeline/Materials/M_DD_MapMark"
SHARD_MARK = "Materials/Shared/M_Shard"
# M_Shard's export keeps one Constant3Vector as its emissive colour, without its value, and the default lit shading.
# The player's scene capture reads the base colour, so the estimate gives both the colour. The WebGL version measured the
# original's marks on screen as #d21ee6 (210, 30, 230); the tablet shows its map through the stage's tonemapping (the
# map's lines match the latest version's screen that way), so the colour was found by measuring our tablet in PIE: this
# one shows (211, 29, 217). Blue cannot go higher: a base colour stops at 1. (sRGB → linear of #d21ee6 would show
# (205, 9, 206).)
SHARD_MARK_COLOR = (0.70, 0.0071, 1.0, 1.0)


def _glb(path):
    """A binary glTF's JSON and BIN chunks."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"glTF":
        raise ValueError("%s is not a binary glTF" % path)
    json_length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + json_length])
    bin_offset = 20 + json_length
    bin_length = struct.unpack_from("<I", data, bin_offset)[0]
    return gltf, data[bin_offset + 8:bin_offset + 8 + bin_length]


def _extract_textures():
    """Writes the mochi's embedded pictures next to the pipeline's other intermediates and imports them. Returns
    {parameter: texture}."""
    gltf, blob = _glb(MOCHI_SOURCE)
    material = gltf["materials"][0]
    slots = dict(material.get("pbrMetallicRoughness", {}))
    slots.update({k: v for k, v in material.items() if k.endswith("Texture")})
    os.makedirs(MOCHI_EXTRACTED, exist_ok=True)
    out = {}
    for slot, param, srgb, compression, lod_group in MOCHI_TEXTURES:
        image = gltf["images"][gltf["textures"][slots[slot]["index"]]["source"]]
        view = gltf["bufferViews"][image["bufferView"]]
        start = view.get("byteOffset", 0)
        extension = {"image/jpeg": ".jpg", "image/png": ".png"}[image["mimeType"]]
        file = os.path.join(MOCHI_EXTRACTED, "T_WasamiMochi_%s%s" % (param, extension))
        with open(file, "wb") as f:
            f.write(blob[start:start + view["byteLength"]])
        tex = dd_stage.import_texture({"file": file, "asset": "%s/T_WasamiMochi_%s" % (MOCHI_FOLDER, param),
                                       "srgb": srgb, "compression": compression, "lodGroup": lod_group})
        if param == "Normal":
            tex.set_editor_property("flip_green_channel", True)
        EAL.save_loaded_asset(tex, only_if_is_dirty=False)
        out[param] = tex
    return out


def _build_mochi(mat, textures):
    """glTF's metallic-roughness material with every factor at 1 (the glb's): the base colour, metallic from B and
    roughness from G of the metallic-roughness map, the normal map, two-sided; and the base colour × Glow as emissive."""
    mat.set_editor_property("two_sided", True)
    g = dd_stage._Graph(mat)
    tcs = unreal.MaterialSamplerType
    base = g.texture("BaseColor", textures["BaseColor"], tcs.SAMPLERTYPE_COLOR, -900, -300)
    g.out(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    glow = g.multiply(base, "RGB", g.scalar("Glow", MOCHI_GLOW, -900, -100), "", -600, -200)
    g.out(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    packed = g.texture("MetallicRoughness", textures["MetallicRoughness"], tcs.SAMPLERTYPE_LINEAR_COLOR, -900, 100)
    g.out(packed, "B", unreal.MaterialProperty.MP_METALLIC)
    g.out(packed, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    normal = g.texture("Normal", textures["Normal"], tcs.SAMPLERTYPE_NORMAL, -900, 400)
    g.out(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)


def import_mochi():
    """The mochi's textures, master, instance and mesh (Nanite, its slot on the instance). Returns the package paths."""
    if not os.path.exists(MOCHI_SOURCE):
        raise FileNotFoundError("%s is missing (git lfs pull)" % MOCHI_SOURCE)
    textures = _extract_textures()
    master = dd_assets.material(MOCHI_MASTER, lambda mat: _build_mochi(mat, textures))
    instance = dd_assets.material_instance(MOCHI_MATERIAL, master, scalars={"Glow": MOCHI_GLOW},
                                           textures={p: t.get_path_name().split(".")[0] for p, t in textures.items()})
    mesh = dd_stage.import_mesh({"file": MOCHI_SOURCE, "asset": MOCHI_MESH, "slots": [None],
                                 "lightmapResolution": 4, "lightmapUv": 0}, nanite=True)
    mesh.set_material(0, instance)
    made = list(textures.values()) + [master, instance, mesh]
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def _build_map_mark(mat):
    """A minimap mark (M_Shard and its kind), estimated: Color as the base colour, which the map's scene capture reads,
    and as the emissive colour, the one input the cook kept (a constant)."""
    g = dd_stage._Graph(mat)
    colour = g.vector("Color", (1.0, 1.0, 1.0, 1.0), -600, 0)
    g.out(colour, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(colour, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def make_map_mark():
    """The marks' estimated master and M_Shard, an instance of it at the original's path."""
    master = dd_assets.material(MAP_MARK_MASTER, _build_map_mark)
    mark = dd_assets.material_instance(dd_assets.asset_path(SHARD_MARK), master,
                                       vectors={"Color": SHARD_MARK_COLOR})
    for asset in (master, mark):
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [master.get_path_name(), mark.get_path_name()]


def import_all():
    """Imports and builds everything the shards use, then saves /Game/DD, /Game/Pipeline and /Game/Wasami. Returns how
    many of each kind."""
    result = {"sounds": len([dd_assets.sound(rel, version) for version, rel in SOUNDS])}
    result["sound_concurrencies"] = len([dd_assets.sound_concurrency(rel, version) for version, rel in SOUND_CONCURRENCIES])
    result["sound_cues"] = len([dd_assets.sound_cue(rel, version) for version, rel in SOUND_CUES])
    result["camera_shakes"] = len([dd_assets.camera_shake(rel, version) for version, rel in CAMERA_SHAKES])
    result["materials"] = len(make_map_mark())
    result["mochi"] = len(import_mochi())
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT, paths.WASAMI_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
