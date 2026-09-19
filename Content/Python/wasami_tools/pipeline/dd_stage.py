"""Dark Deception's hospital (06_Hospital Zone 1 and Zone 2): imports the stage's meshes (the export's glTF), textures
(its PNG) and one material instance per material of the export, from the pipeline data written by
Tools/dd/prepare_stage.py. Everything lands under /Game/DD, mirroring the original's own /Game tree.

The original's master materials are rebuilt as five of ours (their graphs are cooked away; only the parameters and the
material settings survive, and for some the compiled shaders, Tools/dd/cooked_shaders.py):
  M_DD_Substance  MM_Main_Substance and its Emissive / AlphaColorMask / Translucent / Glass variants, and anything else
  M_DD_Decal      M_01_Hotel_Decals — a deferred decal material, which the level puts on plane meshes (mesh decals)
  M_DD_Unlit      MM_Lit — an unlit colour times a multiplier
  M_DD_Metal      MM_Main_Metal — the altar's brass, read back from its compiled shaders
  M_DD_SubstanceFresnel  MM_Main_Substance_Fresnel — the ring pieces' and the secret file's rim, read back the same way
An instance of m_crystal (the altar's orb) is not one of ours: dd_specials makes it with the special shards' crystals.
"""
import os
import struct
import zlib

import unreal

from wasami_tools.pipeline import paths, ue_props

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

# Bump when a master material's graph changes: ensure_masters rebuilds it in place (its instances keep it).
MASTER_VERSION = "1"
VERSION_TAG = "WasamiGraphVersion"

# Which of our masters each master of the export maps to (prepare_stage.py's `master`).
MASTER_OF = {"decal": paths.MASTER_DECAL, "lit": paths.MASTER_UNLIT, "metal": paths.MASTER_METAL,
             "fresnel": paths.MASTER_FRESNEL}
# The export's texture kinds → our texture parameters, per master.
TEX_PARAM = {
    paths.MASTER_SUBSTANCE: {"albedo": "Albedo", "normal": "Normal", "packed": "Packed", "emissive": "Emissive"},
    paths.MASTER_DECAL: {"albedo": "Texture"},
    paths.MASTER_UNLIT: {},
    paths.MASTER_METAL: {"normal": "Normal"},
    paths.MASTER_FRESNEL: {"albedo": "Albedo", "normal": "Normal", "packed": "Packed"},
}
# The export's parameters we can carry over. The rest (Normal Flatness, RefractionDepthBias, Emissive Multiplier,
# Fade Length (S) …) belong to graph parts that cook removed, so they are counted and skipped, not guessed.
SCALARS = {
    paths.MASTER_SUBSTANCE: ("Roughness Power", "Metallic Power", "Emissive Intensity", "Opacity Override"),
    paths.MASTER_DECAL: (),
    paths.MASTER_UNLIT: ("Light Multiplier",),
    paths.MASTER_METAL: ("Roughness", "Normal Flatness", "Hover Intensity"),
    paths.MASTER_FRESNEL: ("Roughness Power", "Metallic Power", "Normal Flatness", "Fresnel ExponentIn",
                           "BaseReflectFractionIn"),
}
VECTORS = {
    paths.MASTER_SUBSTANCE: ("Emissive Color Multiplier", "Mask Color"),
    paths.MASTER_DECAL: ("Color Multiplier",),
    paths.MASTER_UNLIT: ("Light Color",),
    paths.MASTER_METAL: ("Hover Color",),
    paths.MASTER_FRESNEL: ("Fresnel Setting",),
}
BLEND = {
    None: unreal.BlendMode.BLEND_OPAQUE,
    "BLEND_Opaque": unreal.BlendMode.BLEND_OPAQUE,
    "BLEND_Masked": unreal.BlendMode.BLEND_MASKED,
    "BLEND_Translucent": unreal.BlendMode.BLEND_TRANSLUCENT,
    "BLEND_Additive": unreal.BlendMode.BLEND_ADDITIVE,
    "BLEND_Modulate": unreal.BlendMode.BLEND_MODULATE,
    "BLEND_AlphaComposite": getattr(unreal.BlendMode, "BLEND_ALPHA_COMPOSITE", unreal.BlendMode.BLEND_TRANSLUCENT),
}
# Compressions UE keeps linear whatever SRGB says (UTexture::PostEditChangeProperty, Texture.cpp:772).
NO_SRGB_COMPRESSION = ("TC_Alpha", "TC_Normalmap", "TC_Masks", "TC_HDR", "TC_HDR_F32", "TC_HDR_Compressed",
                       "TC_HalfFloat", "TC_SingleFloat")
TRANSLUCENT_BLENDS = ("BLEND_Translucent", "BLEND_Additive", "BLEND_Modulate", "BLEND_AlphaComposite")


def _tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


# ------------------------------------------------------------------------------------------------ pipeline assets
def ensure_mesh_pipeline():
    """The glTF assets pipeline without per-type sub folders, materials, textures or generated collision."""
    if EAL.does_asset_exist(paths.MESH_PIPELINE):
        return unreal.load_asset(paths.MESH_PIPELINE)
    EAL.duplicate_asset("/Interchange/Pipelines/DefaultGLTFAssetsPipeline", paths.MESH_PIPELINE)
    pl = unreal.load_asset(paths.MESH_PIPELINE)
    pl.set_editor_property("asset_type_sub_folders", False)
    pl.set_editor_property("scene_name_sub_folder", False)
    mp = pl.get_editor_property("material_pipeline")
    mp.set_editor_property("import_materials", False)
    mp.get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
    mesh = pl.get_editor_property("mesh_pipeline")
    mesh.set_editor_property("fallback_collision_type", unreal.InterchangeMeshCollision.NONE)
    EAL.save_asset(paths.MESH_PIPELINE)
    return pl


def _solid_png(path, rgb):
    """A 4 × 4 PNG of one colour (written with the standard library: the editor's Python has no imaging module)."""
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    rows = b"".join(b"\x00" + bytes(rgb) * 4 for _ in range(4))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(png)


def ensure_default_packed():
    """The Packed parameter's default: occlusion 1, roughness 0.5, metallic 0. Linear with default compression, the
    Linear Color sampler type the Packed node uses, as the original's packed textures are (TC_Default, not sRGB).
    A node whose default texture is of another sampler type fails to compile, and every instance of the master then
    draws the engine's default material. Returns (texture, whether it was made or its settings changed)."""
    png = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "defaults", "packed_4x4.png")
    entry = {"file": png, "asset": paths.DEFAULT_PACKED, "srgb": False, "compression": None, "lodGroup": None}
    if EAL.does_asset_exist(paths.DEFAULT_PACKED):
        tex = unreal.load_asset(paths.DEFAULT_PACKED)
        if not apply_texture_settings(tex, entry):
            return tex, False
    else:
        _solid_png(png, (255, 128, 0))
        tex = import_texture(entry)
    EAL.save_asset(paths.DEFAULT_PACKED, only_if_is_dirty=False)
    return tex, True


# ------------------------------------------------------------------------------------------------ master materials
def clear_expressions(mat):
    """Removes every expression of a material. UE 5.8's DeleteAllMaterialExpressions removes from the list it walks, so
    one pass leaves about half behind (they stay in the graph, and compile); it is repeated until none is left."""
    for _ in range(64):
        if not MEL.get_num_material_expressions(mat):
            return
        MEL.delete_all_material_expressions(mat)
    raise RuntimeError("the expressions of %s could not be cleared" % mat.get_path_name())


def _material(asset_path):
    """Loads the master material, or makes it; returns (material, whether its graph has to be built)."""
    if EAL.does_asset_exist(asset_path):
        mat = unreal.load_asset(asset_path)
        if EAL.get_metadata_tag(mat, VERSION_TAG) == MASTER_VERSION:
            return mat, False
        clear_expressions(mat)
        return mat, True
    folder, name = paths.split(asset_path)
    return _tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew()), True


def ensure_masters():
    """Builds (or rebuilds) the master materials and returns them by package path."""
    out = {}
    _, packed_changed = ensure_default_packed()
    for asset_path, build in ((paths.MASTER_SUBSTANCE, _build_substance), (paths.MASTER_DECAL, _build_decal),
                              (paths.MASTER_UNLIT, _build_unlit), (paths.MASTER_METAL, _build_metal),
                              (paths.MASTER_FRESNEL, _build_substance_fresnel)):
        mat, needs_build = _material(asset_path)
        if needs_build:
            build(mat)
            EAL.set_metadata_tag(mat, VERSION_TAG, MASTER_VERSION)
        if needs_build or (packed_changed and asset_path == paths.MASTER_SUBSTANCE):
            MEL.recompile_material(mat)
            EAL.save_asset(asset_path, only_if_is_dirty=False)
        out[asset_path] = mat
    return out


class _Graph:
    """Small helpers over MaterialEditingLibrary. A checked graph raises where a connection fails (the library only
    returns False, for a pin name it does not find)."""

    def __init__(self, mat, checked=False):
        self.mat = mat
        self.checked = checked

    def link(self, a, a_pin, b, b_pin):
        if not MEL.connect_material_expressions(a, a_pin, b, b_pin) and self.checked:
            raise RuntimeError("%s: could not connect %s.%s to %s.%s"
                               % (self.mat.get_name(), a.get_name(), a_pin, b.get_name(), b_pin))

    def node(self, cls, x, y):
        return MEL.create_material_expression(self.mat, cls, x, y)

    def texture(self, param, tex, sampler, x, y):
        e = self.node(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("texture", tex)
        e.set_editor_property("sampler_type", sampler)
        return e

    def scalar(self, param, value, x, y):
        e = self.node(unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("default_value", value)
        return e

    def vector(self, param, value, x, y):
        e = self.node(unreal.MaterialExpressionVectorParameter, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("default_value", unreal.LinearColor(*value))
        return e

    def const3(self, value, x, y):
        e = self.node(unreal.MaterialExpressionConstant3Vector, x, y)
        e.set_editor_property("constant", unreal.LinearColor(*value))
        return e

    def binary(self, cls, a, a_pin, b, b_pin, x, y):
        e = self.node(cls, x, y)
        self.link(a, a_pin, e, "A")
        self.link(b, b_pin, e, "B")
        return e

    def multiply(self, a, a_pin, b, b_pin, x, y):
        return self.binary(unreal.MaterialExpressionMultiply, a, a_pin, b, b_pin, x, y)

    def power(self, base, base_pin, exp, exp_pin, x, y):
        e = self.node(unreal.MaterialExpressionPower, x, y)
        self.link(base, base_pin, e, "Base")
        self.link(exp, exp_pin, e, "Exp")
        return e

    def lerp(self, a, a_pin, b, b_pin, alpha, alpha_pin, x, y):
        e = self.node(unreal.MaterialExpressionLinearInterpolate, x, y)
        self.link(a, a_pin, e, "A")
        self.link(b, b_pin, e, "B")
        self.link(alpha, alpha_pin, e, "Alpha")
        return e

    def switch(self, param, on, on_pin, off, off_pin, x, y):
        e = self.node(unreal.MaterialExpressionStaticSwitchParameter, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("default_value", False)
        self.link(on, on_pin, e, "True")
        self.link(off, off_pin, e, "False")
        return e

    def out(self, e, pin, prop):
        if not MEL.connect_material_property(e, pin, prop) and self.checked:
            raise RuntimeError("%s: could not connect %s.%s to %s" % (self.mat.get_name(), e.get_name(), pin, prop))


def _build_substance(mat):
    """MM_Main_Substance and its variants: Albedo (× Mask Color over its alpha), Normal, Packed (R occlusion,
    G roughness, B metallic, each through its Power) and an emissive branch. Blend mode, two-sidedness and the mask
    clip come from each instance's property overrides, as the original's instances carry them.

    Not reproduced: `Normal Flatness` (the instances set 1.2 – 3.0 against a master default of 0, and the graph that
    used it is gone, so neither a 0–1 flatten nor an XY multiplier can be confirmed — the normal is used as it is)."""
    mat.set_editor_property("used_with_skeletal_mesh", True)   # the garage lifts' skinned mesh (dd_skeletal)
    g = _Graph(mat)
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    black = unreal.load_asset("/Engine/EngineResources/Black") or white
    flat = unreal.load_asset("/Engine/EngineMaterials/DefaultNormal") or white
    packed_default, _ = ensure_default_packed()
    tcs = unreal.MaterialSamplerType

    albedo = g.texture("Albedo", white, tcs.SAMPLERTYPE_COLOR, -1400, -650)
    mask_color = g.vector("Mask Color", (1.0, 1.0, 1.0, 1.0), -1400, -420)
    white3 = g.const3((1.0, 1.0, 1.0, 1.0), -1400, -300)
    masked = g.lerp(white3, "", mask_color, "", albedo, "A", -1100, -400)
    tinted = g.multiply(albedo, "RGB", masked, "", -850, -500)
    base = g.switch("UseMaskColor", tinted, "", albedo, "RGB", -600, -600)
    g.out(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    opacity = g.multiply(albedo, "A", g.scalar("Opacity Override", 1.0, -1100, -150), "", -850, -200)
    g.out(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    g.out(opacity, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    normal = g.texture("Normal", flat, tcs.SAMPLERTYPE_NORMAL, -1400, 0)
    g.out(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    packed = g.texture("Packed", packed_default, tcs.SAMPLERTYPE_LINEAR_COLOR, -1400, 320)
    g.out(packed, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    rough = g.power(packed, "G", g.scalar("Roughness Power", 1.0, -1100, 430), "", -800, 380)
    metal = g.power(packed, "B", g.scalar("Metallic Power", 1.0, -1100, 560), "", -800, 510)
    g.out(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(metal, "", unreal.MaterialProperty.MP_METALLIC)

    emissive = g.texture("Emissive", black, tcs.SAMPLERTYPE_COLOR, -1400, 750)
    emissive = g.multiply(emissive, "RGB", g.vector("Emissive Color Multiplier", (1.0, 1.0, 1.0, 1.0), -1400, 950), "", -1100, 800)
    emissive = g.multiply(emissive, "", g.scalar("Emissive Intensity", 1.0, -1100, 1080), "", -850, 850)
    off = g.const3((0.0, 0.0, 0.0, 1.0), -1100, 1200)
    g.out(g.switch("UseEmissive", emissive, "", off, "", -600, 900), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _build_decal(mat):
    """M_01_Hotel_Decals: a deferred decal that writes colour and roughness through the DBuffer. The level puts these
    on plane static meshes (mesh decals), as the original does — it places no DecalActor at all.

    UE version difference: the original sets DecalBlendMode = DBM_DBuffer_ColorRoughness, but UE 5.8 deprecated that
    property ("No longer used", and unreadable from Python) — a decal's DBuffer channels now follow which outputs the
    material connects. Connecting base colour and opacity, and nothing else, gives the same channels.

    Not reproduced: `Roughness Power` and `Emissive Multiplier` (no roughness texture survives to apply them to)."""
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    g = _Graph(mat)
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    tex = g.texture("Texture", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -900, -200)
    colour = g.multiply(tex, "RGB", g.vector("Color Multiplier", (1.0, 1.0, 1.0, 1.0), -900, 100), "", -600, -150)
    g.out(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(tex, "A", unreal.MaterialProperty.MP_OPACITY)


def _build_unlit(mat):
    """MM_Lit: an unlit surface of Light Color × Light Multiplier (the original also reads a material parameter
    collection, `All Lights`, which nothing in the hospital's instances overrides)."""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    g = _Graph(mat)
    colour = g.multiply(g.vector("Light Color", (0.0, 0.0, 0.0, 1.0), -900, -100),
                        "", g.scalar("Light Multiplier", 1.0, -900, 100), "", -600, -50)
    g.out(colour, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _build_metal(mat):
    """MM_Main_Metal, as its cooked base pass pixel shader computes it (Tools/dd/cooked_shaders.py
    "MasterMaterials/MM_Main_Metal."): a constant brass base colour (0.276042, 0.255386, 0.148085), metallic 1, the
    default specular 0.5, Roughness (0.25), the Normal texture pulled towards (0, 0, 1) by Normal Flatness (0), and a
    rim of Fresnel (exponent 6, base reflect fraction 0.001: 0.001 + 0.999 (1 − N·V)^6) × Hover Intensity (0) ×
    Hover Color (white). Only 00_Ballroom's level Blueprint raises Hover Intensity; the hospital leaves it 0."""
    g = _Graph(mat, checked=True)
    flat = unreal.load_asset("/Engine/EngineMaterials/DefaultNormal")
    g.out(g.const3((0.276042, 0.255386, 0.148085, 1.0), -600, -500), "", unreal.MaterialProperty.MP_BASE_COLOR)
    one = g.node(unreal.MaterialExpressionConstant, -600, -380)
    one.set_editor_property("r", 1.0)
    g.out(one, "", unreal.MaterialProperty.MP_METALLIC)
    g.out(g.scalar("Roughness", 0.25, -600, -280), "", unreal.MaterialProperty.MP_ROUGHNESS)

    normal = g.texture("Normal", flat, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -1100, -100)
    normal = g.lerp(normal, "RGB", g.const3((0.0, 0.0, 1.0, 1.0), -1100, 150), "",
                    g.scalar("Normal Flatness", 0.0, -1100, 250), "", -600, -50)
    g.out(normal, "", unreal.MaterialProperty.MP_NORMAL)

    fresnel = g.node(unreal.MaterialExpressionFresnel, -1100, 400)
    fresnel.set_editor_property("exponent", 6.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.001)
    rim = g.multiply(fresnel, "", g.scalar("Hover Intensity", 0.0, -1100, 550), "", -850, 450)
    rim = g.multiply(rim, "", g.vector("Hover Color", (1.0, 1.0, 1.0, 1.0), -1100, 650), "RGB", -600, 500)
    g.out(rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _build_substance_fresnel(mat):
    """MM_Main_Substance_Fresnel, as its cooked base pass pixel shader computes it (Tools/dd/cooked_shaders.py
    "MasterMaterials/MM_Main_Substance_Fresnel."): the base colour is Albedo, roughness Packed.G ^ Roughness Power and
    metallic Packed.B ^ Metallic Power (Packed.R, the occlusion, is not read), the default specular 0.5, the Normal
    texture pulled towards (0, 0, 1) by Normal Flatness (0), and an emissive rim of Fresnel (exponent Fresnel ExponentIn
    5, base reflect fraction BaseReflectFractionIn 0.04, over the mapped normal) × Fresnel Setting's RGB (black)."""
    g = _Graph(mat, checked=True)
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    flat = unreal.load_asset("/Engine/EngineMaterials/DefaultNormal")
    packed_default, _ = ensure_default_packed()
    tcs = unreal.MaterialSamplerType

    albedo = g.texture("Albedo", white, tcs.SAMPLERTYPE_COLOR, -1100, -650)
    g.out(albedo, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    packed = g.texture("Packed", packed_default, tcs.SAMPLERTYPE_LINEAR_COLOR, -1400, -350)
    rough = g.power(packed, "G", g.scalar("Roughness Power", 1.0, -1100, -300), "", -800, -350)
    metal = g.power(packed, "B", g.scalar("Metallic Power", 1.0, -1100, -180), "", -800, -230)
    g.out(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    g.out(metal, "", unreal.MaterialProperty.MP_METALLIC)

    normal = g.texture("Normal", flat, tcs.SAMPLERTYPE_NORMAL, -1100, 0)
    normal = g.lerp(normal, "RGB", g.const3((0.0, 0.0, 1.0, 1.0), -1100, 250), "",
                    g.scalar("Normal Flatness", 0.0, -1100, 350), "", -600, 50)
    g.out(normal, "", unreal.MaterialProperty.MP_NORMAL)

    fresnel = g.node(unreal.MaterialExpressionFresnel, -800, 500)
    g.link(g.scalar("Fresnel ExponentIn", 5.0, -1100, 480), "", fresnel, "ExponentIn")
    g.link(g.scalar("BaseReflectFractionIn", 0.04, -1100, 600), "", fresnel, "BaseReflectFractionIn")
    rim = g.multiply(fresnel, "", g.vector("Fresnel Setting", (0.0, 0.0, 0.0, 1.0), -1100, 720), "RGB", -600, 550)
    g.out(rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


# ------------------------------------------------------------------------------------------------ meshes
def import_mesh(entry, nanite):
    ensure_mesh_pipeline()
    folder, name = paths.split(entry["asset"])
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.destination_name = name
    params.override_pipelines = [unreal.SoftObjectPath(paths.object_path(paths.MESH_PIPELINE))]
    src = unreal.InterchangeManager.create_source_data(entry["file"])
    unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(folder, src, params)
    mesh = unreal.load_asset(entry["asset"])
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("import of %s made no StaticMesh at %s" % (entry["file"], entry["asset"]))
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", bool(nanite))
    if nanite:
        # the fallback mesh is what complex collision uses: keep it whole so walls and floors collide where drawn
        for prop, value in (("fallback_target", unreal.NaniteFallbackTarget.RELATIVE_ERROR), ("fallback_relative_error", 0.0)):
            try:
                settings.set_editor_property(prop, value)
            except Exception as e:  # noqa: BLE001
                unreal.log_warning("nanite %s: %s" % (prop, e))
    mesh.set_editor_property("nanite_settings", settings)
    body = mesh.get_editor_property("body_setup")
    if body is not None:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    slots = mesh.get_editor_property("static_materials")
    if len(slots) != len(entry["slots"]):
        unreal.log_warning("mesh %s: %d slots imported, the export has %d" % (entry["asset"], len(slots), len(entry["slots"])))
    setup_lightmap(mesh, entry)
    return mesh


def setup_lightmap(mesh, entry):
    """Gives the mesh the original's lightmap resolution and UV channel, for the baked lighting the original uses.
    Returns whether anything changed (changing the build settings rebuilds the mesh).

    No lightmap UV is laid out by UE, even where the original has none: the merged stage meshes (Zone 1 and 2's tiles,
    Zone 2's miniboss room) carry a TEXCOORD_1 that is (0,0) everywhere, so the original baked them to a single texel."""
    changed = False
    sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    build = sub.get_lod_build_settings(mesh, 0)
    if build.get_editor_property("generate_lightmap_u_vs"):
        build.set_editor_property("generate_lightmap_u_vs", False)
        sub.set_lod_build_settings(mesh, 0, build)
        changed = True
    for prop, value in (("light_map_resolution", entry["lightmapResolution"]), ("light_map_coordinate_index", entry["lightmapUv"])):
        if mesh.get_editor_property(prop) != value:
            mesh.set_editor_property(prop, value)
            changed = True
    return changed


def translucent_meshes(stage):
    """Meshes drawn with a translucent, additive or decal material somewhere: kept off Nanite, which draws opaque and
    masked materials only."""
    out = set()
    for zone in stage["zones"].values():
        for p in zone["placements"]:
            for key in p["materials"]:
                m = stage["materials"].get(key) if key else None
                if m and (m["master"] == "decal" or m["blend"] in TRANSLUCENT_BLENDS):
                    out.add(p["mesh"])
    return out


# ------------------------------------------------------------------------------------------------ textures
def apply_texture_settings(tex, entry):
    """Sets the compression, sRGB and LOD group the original's texture had. Returns whether anything changed.

    sRGB stays off where UE turns it off anyway (Texture.cpp: alpha, normal map, masks and the HDR compressions) — the
    export keeps it on for two HDR skies, and asking for it again would count them as changed on every refresh."""
    compression = entry.get("compression") or "TC_Default"
    want = {
        "compression_settings": ue_props.enum_member(unreal.TextureCompressionSettings, compression),
        "srgb": bool(entry.get("srgb")) and compression not in NO_SRGB_COMPRESSION,
        "lod_group": (ue_props.enum_member(unreal.TextureGroup, entry["lodGroup"])
                      if entry.get("lodGroup") else unreal.TextureGroup.TEXTUREGROUP_WORLD),
    }
    changed = False
    for name, value in want.items():
        if tex.get_editor_property(name) != value:
            tex.set_editor_property(name, value)
            changed = True
    return changed


def import_texture(entry):
    folder, name = paths.split(entry["asset"])
    task = unreal.AssetImportTask()
    task.filename = entry["file"]
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    task.factory = unreal.TextureFactory()
    _tools().import_asset_tasks([task])
    tex = unreal.load_asset(entry["asset"])
    if tex is None:
        raise RuntimeError("import of %s made nothing at %s" % (entry["file"], entry["asset"]))
    apply_texture_settings(tex, entry)
    return tex


# ------------------------------------------------------------------------------------------------ materials
def master_of(material):
    return MASTER_OF.get(material["master"], paths.MASTER_SUBSTANCE)


def _make_crystal(m):
    """An instance of m_crystal: dd_specials' estimate of it makes the crystals (it rebuilds its master), this one among
    them, in place."""
    from wasami_tools.pipeline import dd_specials   # it imports this module
    made = {a.get_path_name().split(".")[0] for a in dd_specials.make_crystal()}
    if m["asset"] not in made:
        raise RuntimeError("%s is not in dd_specials.CRYSTAL_INSTANCES" % m["source"])
    return unreal.load_asset(m["asset"])


def make_material(m, textures, skipped=None):
    """One material instance of the export → a MaterialInstanceConstant of our matching master. One that exists is
    remade in place (its parameters cleared, then set again): what the levels placed keeps pointing at the same asset."""
    if m["master"] == "crystal":
        return _make_crystal(m)
    master_path = master_of(m)
    masters = ensure_masters()
    if EAL.does_asset_exist(m["asset"]):
        mic = unreal.load_asset(m["asset"])
    else:
        folder, name = paths.split(m["asset"])
        mic = _tools().create_asset(name, folder, unreal.MaterialInstanceConstant,
                                    unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mic, masters[master_path])
    MEL.clear_all_material_instance_parameters(mic)

    params = TEX_PARAM[master_path]
    for kind, png in (m.get("kinds") or {}).items():
        param = params.get(kind)
        entry = textures.get(png)
        if not param or not entry:
            continue
        tex = unreal.load_asset(entry["asset"])
        if tex is not None:
            MEL.set_material_instance_texture_parameter_value(mic, param, tex)
    if master_path == paths.MASTER_SUBSTANCE:
        MEL.set_material_instance_static_switch_parameter_value(mic, "UseEmissive", "emissive" in (m.get("kinds") or {}))
        MEL.set_material_instance_static_switch_parameter_value(mic, "UseMaskColor", m["master"] in ("alphamask", "glassmask"))
    for key, value in (m.get("scalars") or {}).items():
        if key in SCALARS[master_path]:
            MEL.set_material_instance_scalar_parameter_value(mic, key, float(value))
        elif skipped is not None:
            skipped[key] = skipped.get(key, 0) + 1
    for key, value in (m.get("vectors") or {}).items():
        if key in VECTORS[master_path]:
            MEL.set_material_instance_vector_parameter_value(mic, key, unreal.LinearColor(*[float(v) for v in value]))
        elif skipped is not None:
            skipped[key] = skipped.get(key, 0) + 1

    over = mic.get_editor_property("base_property_overrides")
    over.set_editor_property("override_blend_mode", True)
    over.set_editor_property("blend_mode", BLEND.get(m.get("blend"), unreal.BlendMode.BLEND_OPAQUE))
    over.set_editor_property("override_two_sided", True)
    over.set_editor_property("two_sided", bool(m.get("twoSided")))
    if m.get("clip") is not None:
        over.set_editor_property("override_opacity_mask_clip_value", True)
        over.set_editor_property("opacity_mask_clip_value", float(m["clip"]))
    unlit = m.get("shading") == "MSM_Unlit" and master_path == paths.MASTER_SUBSTANCE
    over.set_editor_property("override_shading_model", unlit)
    if unlit:
        over.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mic.set_editor_property("base_property_overrides", over)
    MEL.update_material_instance(mic)
    return mic


# ------------------------------------------------------------------------------------------------ batch
def import_batch(max_items):
    """Creates the next `max_items` assets that are missing, meshes first, then textures, then materials."""
    if max_items < 1:
        raise ValueError("max_items must be at least 1.")
    stage = paths.load_dd_stage()
    ensure_mesh_pipeline()
    ensure_masters()
    no_nanite = translucent_meshes(stage)
    todo = []
    for key, m in stage["meshes"].items():
        if m["engine"] or not m["file"]:
            continue
        todo.append(("meshes", m["asset"], lambda m=m, key=key: import_mesh(m, key not in no_nanite)))
    for _png, t in stage["textures"].items():
        todo.append(("textures", t["asset"], lambda t=t: import_texture(t)))
    for m in stage["materials"].values():
        if m["asset"]:
            todo.append(("materials", m["asset"], lambda m=m: make_material(m, stage["textures"])))
    totals = {k: sum(1 for kind, _a, _f in todo if kind == k) for k in ("meshes", "textures", "materials")}
    missing = [(kind, asset, fn) for kind, asset, fn in todo if not EAL.does_asset_exist(asset)]
    imported = 0
    with unreal.ScopedSlowTask(min(max_items, len(missing)), "Importing the hospital's assets") as task:
        for _kind, asset, fn in missing[:max_items]:
            task.enter_progress_frame(1, asset)
            fn()
            imported += 1
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    EAL.save_directory(paths.PIPELINE_ROOT, only_if_is_dirty=True, recursive=True)
    left = {k: sum(1 for kind, _a, _f in missing[imported:] if kind == k) for k in totals}
    out = {"imported": imported, "remaining": len(missing) - imported}
    for k, total in totals.items():
        out[k + "_total"] = total
        out[k + "_done"] = total - left[k]
    return out


def refresh_settings():
    """Brings the already imported assets up to this module: rebuilds a master material whose graph version is old,
    re-applies each texture's settings and each mesh's lightmap settings, and recompiles every material instance (an instance with static switches keeps
    a failed shader map from an older master until it is updated). An instance on one of these masters that is no
    longer the one its root maps to (a master added since, as M_DD_Metal or M_DD_SubstanceFresnel) is remade in place on the right one, and
    one of m_crystal by dd_specials; one another module has put on a master of its own is left alone."""
    stage = paths.load_dd_stage()
    rebuilt = 0
    for asset_path in TEX_PARAM:
        if not EAL.does_asset_exist(asset_path) or EAL.get_metadata_tag(unreal.load_asset(asset_path), VERSION_TAG) != MASTER_VERSION:
            rebuilt += 1
    masters = ensure_masters()
    changed = 0
    with unreal.ScopedSlowTask(len(stage["textures"]), "Updating the stage's texture settings") as task:
        for t in stage["textures"].values():
            task.enter_progress_frame(1, t["asset"])
            tex = unreal.load_asset(t["asset"]) if EAL.does_asset_exist(t["asset"]) else None
            if tex is not None and apply_texture_settings(tex, t):
                changed += 1
    lightmaps = 0
    meshes = [m for m in stage["meshes"].values() if not m["engine"] and EAL.does_asset_exist(m["asset"])]
    with unreal.ScopedSlowTask(len(meshes), "Updating the stage's lightmap settings") as task:
        for m in meshes:
            task.enter_progress_frame(1, m["asset"])
            mesh = unreal.load_asset(m["asset"])
            if isinstance(mesh, unreal.StaticMesh) and setup_lightmap(mesh, m):
                lightmaps += 1
    updated = remade = 0
    with unreal.ScopedSlowTask(len(stage["materials"]), "Recompiling the stage's material instances") as task:
        for m in stage["materials"].values():
            task.enter_progress_frame(1, m["asset"] or "")
            mic = unreal.load_asset(m["asset"]) if m["asset"] and EAL.does_asset_exist(m["asset"]) else None
            if not isinstance(mic, unreal.MaterialInstanceConstant):
                continue
            parent = mic.get_editor_property("parent")
            parent = parent.get_path_name() if parent else ""
            ours = parent in (paths.object_path(p) for p in masters)
            if ours and (m["master"] == "crystal" or parent != paths.object_path(master_of(m))):
                make_material(m, stage["textures"])
                remade += 1
            else:
                MEL.update_material_instance(mic)
            updated += 1
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    EAL.save_directory(paths.PIPELINE_ROOT, only_if_is_dirty=True, recursive=True)
    return {"masters_rebuilt": rebuilt, "textures_updated": changed, "lightmaps_updated": lightmaps,
            "materials_updated": updated, "materials_remade": remade}
