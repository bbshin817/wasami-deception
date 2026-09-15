"""Chaotic Customer 2 Zone_1: imports the stage's meshes (glb, one slot per section), textures (png / hdr) and creates
one material instance of M_CC2_Standard per material of the export (stage_ue.json from Tools/cc2/prepare_stage.py)."""
import os
import struct
import zlib

import unreal

from wasami_tools.pipeline import paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

# Bump when the master material's graph changes: ensure_master_material rebuilds it in place (its instances keep it).
MASTER_VERSION = "2"
VERSION_TAG = "WasamiGraphVersion"


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


def _white_png(path):
    """A 4 × 4 white RGB PNG (written with the standard library: the editor's Python has no imaging module)."""
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    rows = b"".join(b"\x00" + b"\xff\xff\xff" * 4 for _ in range(4))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(png)


def ensure_default_masks():
    """White linear texture with Masks compression: the ORM parameter's default (a Masks sampler rejects sRGB colour
    textures such as the engine's WhiteSquareTexture, and the material then falls back to the default material)."""
    if EAL.does_asset_exist(paths.DEFAULT_MASKS):
        return unreal.load_asset(paths.DEFAULT_MASKS)
    png = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "defaults", "white_4x4.png")
    _white_png(png)
    tex = import_texture(png, paths.DEFAULT_MASKS, "orm")
    EAL.save_asset(paths.DEFAULT_MASKS)
    return tex


def ensure_master_material():
    """M_CC2_Standard: textures or constants for base colour (× tint), normal, ORM (occlusion / roughness / metallic),
    emissive (× colour × strength), and the base colour's alpha as opacity (mask) when switched on. Blend mode and
    two-sidedness come from each instance's property overrides. Rebuilt in place when MASTER_VERSION changes."""
    if EAL.does_asset_exist(paths.MASTER_MATERIAL):
        mat = unreal.load_asset(paths.MASTER_MATERIAL)
        if EAL.get_metadata_tag(mat, VERSION_TAG) == MASTER_VERSION:
            return mat
        MEL.delete_all_material_expressions(mat)
    else:
        folder, name = paths.split(paths.MASTER_MATERIAL)
        mat = _tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    _build_master_graph(mat)
    EAL.set_metadata_tag(mat, VERSION_TAG, MASTER_VERSION)
    MEL.recompile_material(mat)
    EAL.save_asset(paths.MASTER_MATERIAL, only_if_is_dirty=False)
    return mat


def _build_master_graph(mat):
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    black = unreal.load_asset("/Engine/EngineResources/Black") or white
    flat = unreal.load_asset("/Engine/EngineMaterials/DefaultNormal") or white
    masks = ensure_default_masks()

    def node(cls, x, y):
        return MEL.create_material_expression(mat, cls, x, y)

    def texture(param, tex, sampler, x, y):
        e = node(unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("texture", tex)
        e.set_editor_property("sampler_type", sampler)
        return e

    def scalar(param, value, x, y):
        e = node(unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("default_value", value)
        return e

    def vector(param, value, x, y):
        e = node(unreal.MaterialExpressionVectorParameter, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("default_value", unreal.LinearColor(*value))
        return e

    def switch(param, on, on_pin, off, off_pin, x, y):
        e = node(unreal.MaterialExpressionStaticSwitchParameter, x, y)
        e.set_editor_property("parameter_name", param)
        e.set_editor_property("default_value", False)
        MEL.connect_material_expressions(on, on_pin, e, "True")
        MEL.connect_material_expressions(off, off_pin, e, "False")
        return e

    def multiply(a, a_pin, b, b_pin, x, y):
        e = node(unreal.MaterialExpressionMultiply, x, y)
        MEL.connect_material_expressions(a, a_pin, e, "A")
        MEL.connect_material_expressions(b, b_pin, e, "B")
        return e

    def const(value, x, y):
        e = node(unreal.MaterialExpressionConstant, x, y)
        e.set_editor_property("r", value)
        return e

    one = const(1.0, -900, 0)
    # base colour and opacity
    base_tex = texture("BaseColor", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1200, -600)
    base_rgb = switch("UseBaseColorTex", base_tex, "RGB", one, "", -900, -600)
    tint = vector("BaseColorTint", (1.0, 1.0, 1.0, 1.0), -900, -450)
    base = multiply(base_rgb, "", tint, "", -650, -600)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    alpha = switch("UseAlphaOpacity", base_tex, "A", one, "", -900, -300)
    opacity = multiply(alpha, "", scalar("Opacity", 1.0, -900, -200), "", -650, -300)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    # normal
    normal_tex = texture("Normal", flat, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -1200, -50)
    flat_const = node(unreal.MaterialExpressionConstant3Vector, -1200, 150)
    flat_const.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    normal = switch("UseNormalTex", normal_tex, "RGB", flat_const, "", -650, -50)
    MEL.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
    # occlusion / roughness / metallic
    orm = texture("ORM", masks, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, -1200, 300)
    occlusion = switch("UseORMTex", orm, "R", one, "", -650, 250)
    roughness = switch("UseORMTex", orm, "G", scalar("Roughness", 0.5, -900, 400), "", -650, 350)
    metallic = switch("UseORMTex", orm, "B", scalar("Metallic", 0.0, -900, 500), "", -650, 450)
    MEL.connect_material_property(occlusion, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    MEL.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    # emissive
    emissive_tex = texture("Emissive", black, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1200, 650)
    emissive_rgb = switch("UseEmissiveTex", emissive_tex, "RGB", one, "", -900, 650)
    emissive = multiply(emissive_rgb, "", vector("EmissiveColor", (0.0, 0.0, 0.0, 1.0), -900, 800), "", -650, 650)
    emissive = multiply(emissive, "", scalar("EmissiveStrength", 1.0, -650, 800), "", -400, 650)
    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


# ------------------------------------------------------------------------------------------------ meshes
def import_mesh(glb, asset, collision, nanite):
    ensure_mesh_pipeline()
    folder, name = paths.split(asset)
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.destination_name = name
    params.override_pipelines = [unreal.SoftObjectPath(paths.object_path(paths.MESH_PIPELINE))]
    src = unreal.InterchangeManager.create_source_data(glb)
    unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(folder, src, params)
    mesh = unreal.load_asset(asset)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("import of %s made no StaticMesh at %s" % (glb, asset))
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
    if collision != "none":
        body = mesh.get_editor_property("body_setup")
        if body is not None:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    return mesh


# ------------------------------------------------------------------------------------------------ textures
def texture_settings(kind):
    """(compression, sRGB, LOD group) per kind; the importer's own guesses (a grayscale or non-power-of-two picture as a
    UI texture, a bluish one as a normal map) do not match the master material's samplers."""
    tcs, group = unreal.TextureCompressionSettings, unreal.TextureGroup
    return {
        "albedo": (tcs.TC_DEFAULT, True, group.TEXTUREGROUP_WORLD),
        "emissive": (tcs.TC_DEFAULT, True, group.TEXTUREGROUP_WORLD),
        "normal": (tcs.TC_NORMALMAP, False, group.TEXTUREGROUP_WORLD_NORMAL_MAP),
        "orm": (tcs.TC_MASKS, False, group.TEXTUREGROUP_WORLD),
        "lut": (tcs.TC_VECTOR_DISPLACEMENTMAP, True, group.TEXTUREGROUP_COLOR_LOOKUP_TABLE),
    }.get(kind)


def apply_texture_settings(tex, kind):
    """Sets the kind's compression, sRGB and LOD group (and no mips for LUTs). Returns whether anything changed."""
    want = texture_settings(kind)
    if want is None:
        return False
    names = ("compression_settings", "srgb", "lod_group")
    if tuple(tex.get_editor_property(n) for n in names) == want:
        return False
    for n, v in zip(names, want):
        tex.set_editor_property(n, v)
    if kind == "lut":
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    return True


def import_texture(path, asset, kind):
    folder, name = paths.split(asset)
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    task.factory = unreal.TextureFactory()
    _tools().import_asset_tasks([task])
    tex = unreal.load_asset(asset)
    if tex is None:
        raise RuntimeError("import of %s made nothing at %s" % (path, asset))
    apply_texture_settings(tex, kind)
    return tex


# ------------------------------------------------------------------------------------------------ materials
BLEND = {
    "OPAQUE": unreal.BlendMode.BLEND_OPAQUE,
    "MASK": unreal.BlendMode.BLEND_MASKED,
    "BLEND": unreal.BlendMode.BLEND_TRANSLUCENT,
    "ADD": unreal.BlendMode.BLEND_ADDITIVE,
}


def make_material(m, textures):
    """A MaterialInstanceConstant of M_CC2_Standard for one export material (stage_ue.json `materials` entry), as the
    WebGL version's material table read them (its scripts/prepare-cc2-textures.mjs): a texture wins over the constant
    colour, an ORM texture over the roughness / metallic constants, the emissive texture over the emissive colour."""
    master = ensure_master_material()
    folder, name = paths.split(m["asset"])
    mic = _tools().create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mic, master)
    t = m.get("textures") or {}

    def tex(kind):
        return unreal.load_asset(textures[t[kind]]["asset"]) if t.get(kind) else None

    def switch(param, on):
        MEL.set_material_instance_static_switch_parameter_value(mic, param, bool(on))

    base, normal, orm, emissive = tex("diffuse"), tex("normal"), tex("orm"), tex("emissive")
    switch("UseBaseColorTex", base)
    if base:
        MEL.set_material_instance_texture_parameter_value(mic, "BaseColor", base)
    elif m.get("constColor"):
        c = m["constColor"]
        MEL.set_material_instance_vector_parameter_value(mic, "BaseColorTint", unreal.LinearColor(c[0], c[1], c[2], 1.0))
    switch("UseNormalTex", normal)
    if normal:
        MEL.set_material_instance_texture_parameter_value(mic, "Normal", normal)
    switch("UseORMTex", orm)
    if orm:
        MEL.set_material_instance_texture_parameter_value(mic, "ORM", orm)
    else:
        MEL.set_material_instance_scalar_parameter_value(mic, "Roughness", 0.8 if m.get("decal") else (m["roughness"] if m.get("roughness") is not None else 0.5))
        MEL.set_material_instance_scalar_parameter_value(mic, "Metallic", m["metallic"] if m.get("metallic") is not None else 0.0)
    switch("UseEmissiveTex", emissive)
    if emissive:
        MEL.set_material_instance_texture_parameter_value(mic, "Emissive", emissive)
        MEL.set_material_instance_vector_parameter_value(mic, "EmissiveColor", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    elif m.get("emissiveColor"):
        c = m["emissiveColor"]
        MEL.set_material_instance_vector_parameter_value(mic, "EmissiveColor", unreal.LinearColor(c[0], c[1], c[2], 1.0))

    blend = "BLEND" if m.get("decal") else m.get("blend", "OPAQUE")
    uses_alpha = bool(base) and (m.get("decal") or (m.get("alpha") and blend in ("MASK", "BLEND", "ADD")))
    if blend == "MASK" and not uses_alpha and m.get("opacity") != 0:
        blend = "OPAQUE"
    switch("UseAlphaOpacity", uses_alpha)
    if m.get("opacity") is not None:
        MEL.set_material_instance_scalar_parameter_value(mic, "Opacity", m["opacity"])
    over = mic.get_editor_property("base_property_overrides")
    over.set_editor_property("override_blend_mode", True)
    over.set_editor_property("blend_mode", BLEND.get(blend, unreal.BlendMode.BLEND_OPAQUE))
    over.set_editor_property("override_two_sided", True)
    over.set_editor_property("two_sided", bool(m.get("twoSided") or m.get("decal")))
    if blend == "MASK":
        over.set_editor_property("override_opacity_mask_clip_value", True)
        over.set_editor_property("opacity_mask_clip_value", float(m.get("clip") or 0.3333))
    mic.set_editor_property("base_property_overrides", over)
    MEL.update_material_instance(mic)
    return mic


def translucent_meshes(stage):
    """Meshes drawn with a translucent or additive material somewhere: kept off Nanite, which draws only opaque and
    masked materials."""
    out = set()
    for p in stage["placements"]:
        for key in p["materials"]:
            m = stage["materials"].get(key) if key else None
            if m and (m.get("decal") or m.get("blend") in ("BLEND", "ADD")):
                out.add(p["mesh"])
    return out


# ------------------------------------------------------------------------------------------------ batch
def import_batch(max_items):
    if max_items < 1:
        raise ValueError("max_items must be at least 1.")
    stage = paths.load_cc2_stage()
    ensure_mesh_pipeline()
    ensure_master_material()
    no_nanite = translucent_meshes(stage)
    todo = []
    for rel, m in stage["meshes"].items():
        todo.append(("meshes", m["asset"], lambda rel=rel, m=m: import_mesh(m["glb"], m["asset"], m["collision"], rel not in no_nanite)))
    for path, t in stage["textures"].items():
        todo.append(("textures", t["asset"], lambda path=path, t=t: import_texture(path, t["asset"], t["kind"])))
    for m in stage["materials"].values():
        todo.append(("materials", m["asset"], lambda m=m: make_material(m, stage["textures"])))
    totals = {k: sum(1 for kind, _a, _f in todo if kind == k) for k in ("meshes", "textures", "materials")}
    missing = [(kind, asset, fn) for kind, asset, fn in todo if not EAL.does_asset_exist(asset)]
    imported = 0
    with unreal.ScopedSlowTask(min(max_items, len(missing)), "Importing Chaotic Customer 2 assets") as task:
        for kind, asset, fn in missing[:max_items]:
            task.enter_progress_frame(1, asset)
            fn()
            imported += 1
    EAL.save_directory(paths.CC2_ROOT, only_if_is_dirty=True, recursive=True)
    remaining = len(missing) - imported
    left = {k: sum(1 for kind, _a, _f in missing[imported:] if kind == k) for k in totals}
    out = {"imported": imported, "remaining": remaining}
    for k, total in totals.items():
        out[k + "_total"] = total
        out[k + "_done"] = total - left[k]
    return out


def refresh_settings():
    """Brings already imported assets up to this module: rebuilds the master material if its graph version is old,
    re-applies each texture kind's settings, and recompiles every material instance (an instance with static switches
    keeps a failed shader map from an older master until it is updated)."""
    stage = paths.load_cc2_stage()
    master = unreal.load_asset(paths.MASTER_MATERIAL) if EAL.does_asset_exist(paths.MASTER_MATERIAL) else None
    rebuilt = master is None or EAL.get_metadata_tag(master, VERSION_TAG) != MASTER_VERSION
    ensure_master_material()
    changed = 0
    with unreal.ScopedSlowTask(len(stage["textures"]), "Updating the stage's texture settings") as task:
        for t in stage["textures"].values():
            task.enter_progress_frame(1, t["asset"])
            tex = unreal.load_asset(t["asset"])
            if tex is not None and apply_texture_settings(tex, t["kind"]):
                changed += 1
    updated = 0
    with unreal.ScopedSlowTask(len(stage["materials"]), "Recompiling the stage's material instances") as task:
        for m in stage["materials"].values():
            task.enter_progress_frame(1, m["asset"])
            mic = unreal.load_asset(m["asset"])
            if isinstance(mic, unreal.MaterialInstanceConstant):
                MEL.update_material_instance(mic)
                updated += 1
    EAL.save_directory(paths.CC2_ROOT, only_if_is_dirty=True, recursive=True)
    EAL.save_directory("/Game/Pipeline", only_if_is_dirty=True, recursive=True)
    return {"master_rebuilt": int(rebuilt), "textures_updated": changed, "materials_updated": updated}
