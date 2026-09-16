"""Dark Deception's tablet: the plate the player holds (mesh, materials, textures), the screen's UI textures and font,
the two woosh sounds, and the minimap (the level's map plane, the render target the player's scene capture draws into,
and the materials that show it).

Everything lands under /Game/DD mirroring the original's own /Game tree, except the master materials we have to write
ourselves (the originals' graphs are cooked away), which go next to the stage's under /Game/Pipeline/Materials:
  M_DD_MapPlane   MM_Map_Parent — the map image on a plane in the world, read by the capture's SCS_BaseColor
  M_DD_MapScreen  M_NewMap — the capture's render target on the tablet's screen (a User Interface material)
  M_DD_Powers     MM_Powers — a power's icon in colour over the grey one, clockwise from 12 o'clock by `Percent`

Sources: pak_reference (UE 4.21) for the tablet and its UI, pak_reference_2 (UE 4.24) for the hospital's map images,
which the older version does not have.
"""
import json
import os

import unreal

from wasami_tools.pipeline import dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

MAP_PLANE_MASTER = "/Game/Pipeline/Materials/M_DD_MapPlane"
MAP_SCREEN_MASTER = "/Game/Pipeline/Materials/M_DD_MapScreen"
POWERS_MASTER = "/Game/Pipeline/Materials/M_DD_Powers"

# The original's own paths, under /Game/DD.
MESH = "Meshes/Player/Tablet/tablet_new_pCube2"
MINIMAP_TARGET = "UI/Minimap/T_NewMap"
FONT_FACE = "UI/Fonts/helvetica-neue-bold"
FONT = "UI/Fonts/helvetica-neue-bold_Font"

# The textures the tablet needs: (pak_reference version, the original's path under /Game).
TEXTURES = (
    (1, "Textures/Characters/Player/Tablet/Tablet_Front_D"),
    (1, "Textures/Characters/Player/Tablet/Tablet_Back_D"),
    (1, "Textures/Characters/Player/Tablet/Tablet_N"),
    (1, "Textures/Characters/Player/Tablet/Tablet_S"),
    (1, "UI/Tablet/tablet_screen_bg"),
    (1, "UI/Tablet/tablet_map_player"),
    (1, "UI/Tablet/tablet_map_shard"),
    (1, "UI/Tablet/tablet_map_ring"),
    (1, "UI/Tablet/tablet_map_bonus_shard"),
    (1, "UI/Tablet/tablet_map_arrow"),
    (1, "UI/Menu/Streaks/T_Vignette"),
    (1, "UI/RingAltar_UI/Textures/ring_altar_power_teleport_icon"),
    (1, "UI/RingAltar_UI/Textures/ring_altar_power_teleport_icon_inactive"),
    (1, "UI/RingAltar_UI/Textures/ring_altar_power_speed_boost_icon"),
    (1, "UI/RingAltar_UI/Textures/ring_altar_power_speed_boost_icon_inactive"),
    # The hospital's baked maps (the plane in the level carries them; chapter 6 is not in the older export).
    (2, "UI/Minimap/T_06_Zone01"),
    (2, "UI/Minimap/T_06_Zone2"),
)

# The tablet's two material slots: (the original's material, its albedo). Normal and packed are shared.
BODY_MATERIALS = (
    ("Materials/Player/Tablet/M_P_TabletBack", "Textures/Characters/Player/Tablet/Tablet_Back_D"),
    ("Materials/Player/Tablet/M_P_TabletFront", "Textures/Characters/Player/Tablet/Tablet_Front_D"),
)
BODY_NORMAL = "Textures/Characters/Player/Tablet/Tablet_N"
BODY_PACKED = "Textures/Characters/Player/Tablet/Tablet_S"

# The original's sounds: (its path under /Game, the SoundWave's own Volume).
SOUNDS = (
    ("Audio/SharedGameplay/05_Tablet_Woosh_v1_1", None),
    ("Audio/SharedGameplay/05_Tablet_Woosh_v2_1", None),
    ("Audio/UI/UI_Select_V3", 0.7),
)

# One material instance per zone's map image, as the original's MM_Map_06_Zone01 / _Zone2.
MAPS = (("UI/Minimap/MM_Map_06_Zone01", "UI/Minimap/T_06_Zone01"),
        ("UI/Minimap/MM_Map_06_Zone2", "UI/Minimap/T_06_Zone2"))
# The power sockets, as the original's MM_Powers instances: (instance, colour icon, grey icon).
POWERS = (("Materials/MasterMaterials/MM_Powers_Inst_Teleport",
           "UI/RingAltar_UI/Textures/ring_altar_power_teleport_icon",
           "UI/RingAltar_UI/Textures/ring_altar_power_teleport_icon_inactive"),
          ("Materials/MasterMaterials/MM_Powers_SpeedBoost",
           "UI/RingAltar_UI/Textures/ring_altar_power_speed_boost_icon",
           "UI/RingAltar_UI/Textures/ring_altar_power_speed_boost_icon_inactive"))


def _tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def _instance(asset_path, parent):
    """Loads or creates the material instance at asset_path and points it at parent."""
    if EAL.does_asset_exist(asset_path):
        mic = unreal.load_asset(asset_path)
    else:
        folder, name = paths.split(asset_path)
        mic = _tools().create_asset(name, folder, unreal.MaterialInstanceConstant,
                                    unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mic, parent)
    return mic


def _pak(version):
    return paths.DD_PAK if version == 1 else paths.DD_PAK2


def asset(rel):
    """The original's /Game/<rel> under our /Game/DD."""
    return paths.DD_ROOT + "/" + rel


def _texture_settings(version, rel):
    """The original texture's sRGB, compression and LOD group, from the export's _textures.json."""
    with open(os.path.join(_pak(version), "_textures.json"), encoding="utf-8") as f:
        table = json.load(f)
    key = "DDeception/Content/%s.uasset" % rel
    entry = table.get(key)
    if entry is None:
        raise KeyError("no texture %s in %s/_textures.json" % (key, _pak(version)))
    return {"srgb": entry["srgb"], "compression": entry["compression"], "lodGroup": entry["lod_group"]}


# ------------------------------------------------------------------------------------------------ textures / mesh
def import_textures():
    """Imports the PNGs the tablet uses with the original's texture settings."""
    done = []
    for version, rel in TEXTURES:
        entry = _texture_settings(version, rel)
        entry["file"] = os.path.join(_pak(version), "DDeception", "Content", *rel.split("/")) + ".png"
        entry["asset"] = asset(rel)
        if not os.path.exists(entry["file"]):
            raise FileNotFoundError(entry["file"])
        dd_stage.import_texture(entry)
        done.append(entry["asset"])
    return done


def import_mesh():
    """The tablet plate (18.5 × 1 × 23.8 cm, slots phong2 = back and phong3 = front), with its two materials."""
    gltf = os.path.join(paths.DD_PAK2, "_meshes_gltf", "Meshes", "Player", "Tablet", "tablet_new_pCube2.gltf")
    mesh = dd_stage.import_mesh({"file": gltf, "asset": asset(MESH), "slots": ["phong2", "phong3"]}, nanite=False)
    slots = [unreal.StaticMaterial(material_interface=unreal.load_asset(asset(rel)), material_slot_name=name)
             for (rel, _), name in zip(BODY_MATERIALS, ("phong2", "phong3"))]
    mesh.set_editor_property("static_materials", slots)
    return mesh


def make_body_materials():
    """M_P_TabletBack / M_P_TabletFront: instances of the stage's M_DD_Substance, as the original's are of
    MM_Main_Substance, with the same albedo / normal / packed textures."""
    master = dd_stage.ensure_masters()[paths.MASTER_SUBSTANCE]
    out = []
    for rel, albedo in BODY_MATERIALS:
        mic = _instance(asset(rel), master)
        for param, tex_rel in (("Albedo", albedo), ("Normal", BODY_NORMAL), ("Packed", BODY_PACKED)):
            MEL.set_material_instance_texture_parameter_value(mic, param, unreal.load_asset(asset(tex_rel)))
        out.append(asset(rel))
    return out


# ------------------------------------------------------------------------------------------------ font and sounds
def import_font():
    """The original's helvetica-neue-bold as a font face and a runtime Font asset (its UMG texts use the Font)."""
    ttf = os.path.join(paths.DD_PAK, "DDeception", "Content", "UI", "Fonts", "helvetica-neue-bold.ttf")
    folder, name = paths.split(asset(FONT_FACE))
    task = unreal.AssetImportTask()
    task.filename = ttf
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    task.factory = unreal.FontFileImportFactory()
    _tools().import_asset_tasks([task])
    face = unreal.load_asset(asset(FONT_FACE))
    if face is None:
        raise RuntimeError("the font face did not import to %s" % asset(FONT_FACE))

    folder, name = paths.split(asset(FONT))
    if EAL.does_asset_exist(asset(FONT)):
        font = unreal.load_asset(asset(FONT))
    else:
        font = _tools().create_asset(name, folder, unreal.Font, unreal.FontFactory())
    font.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)
    # FCompositeFont's members are not exposed to Python, so the typeface goes in as the struct's own text form.
    composite = unreal.CompositeFont()
    composite.import_text('(DefaultTypeface=(Fonts=((Name="Default",Font=(FontFaceAsset=FontFace\'"%s"\','
                          'LoadingPolicy=LazyLoad,SubFaceIndex=0)))),FallbackTypeface=(Typeface=(Fonts=),'
                          'ScalingFactor=1.000000),SubTypefaces=,bEnableAscentDescentOverride=True)'
                          % paths.object_path(asset(FONT_FACE)))
    font.set_editor_property("composite_font", composite)
    return asset(FONT)


def import_sounds():
    """The tablet's woosh up / down and the UI select the map's resize plays, with the SoundWave's own Volume."""
    out = []
    for rel, volume in SOUNDS:
        ogg = os.path.join(paths.DD_PAK, "DDeception", "Content", *rel.split("/")) + ".ogg"
        if not os.path.exists(ogg):
            raise FileNotFoundError(ogg)
        folder, name = paths.split(asset(rel))
        task = unreal.AssetImportTask()
        task.filename = ogg
        task.destination_path = folder
        task.destination_name = name
        task.replace_existing = True
        task.automated = True
        task.save = False
        task.factory = unreal.SoundFactory()
        _tools().import_asset_tasks([task])
        wave = unreal.load_asset(asset(rel))
        if wave is None:
            raise RuntimeError("the sound did not import to %s" % asset(rel))
        if volume is not None:
            wave.set_editor_property("volume", volume)
        out.append(asset(rel))
    return out


# ------------------------------------------------------------------------------------------------ minimap / powers
def ensure_render_target():
    """T_NewMap: what the player's scene capture draws the map into (512 × 512 RGBA8, as the original's)."""
    path = asset(MINIMAP_TARGET)
    if EAL.does_asset_exist(path):
        rt = unreal.load_asset(path)
    else:
        folder, name = paths.split(path)
        rt = _tools().create_asset(name, folder, unreal.TextureRenderTarget2D, unreal.TextureRenderTargetFactoryNew())
    rt.set_editor_property("size_x", 512)
    rt.set_editor_property("size_y", 512)
    rt.set_editor_property("render_target_format", unreal.TextureRenderTargetFormat.RTF_RGBA8)
    return path


def _master(asset_path, build, domain_ui=False):
    """Loads or creates one of our master materials and (re)builds its graph."""
    if EAL.does_asset_exist(asset_path):
        mat = unreal.load_asset(asset_path)
        MEL.delete_all_material_expressions(mat)
    else:
        folder, name = paths.split(asset_path)
        mat = _tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if domain_ui:
        mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    build(mat)
    MEL.recompile_material(mat)
    return mat


def _build_map_plane(mat):
    """MM_Map_Parent: the map image as base colour on a lit surface — the capture reads SCS_BaseColor, which an unlit
    material does not write."""
    g = dd_stage._Graph(mat)
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    tex = g.texture("Texture", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -600, -100)
    g.out(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)


def _build_map_screen(mat):
    """M_NewMap: the capture's render target on the screen. Its alpha is not what the capture writes, so the UI
    material's opacity is 1."""
    g = dd_stage._Graph(mat)
    rt = unreal.load_asset(asset(MINIMAP_TARGET))
    tex = g.texture("Texture", rt, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, -600, -100)
    one = g.node(unreal.MaterialExpressionConstant, -600, 200)
    one.set_editor_property("r", 1.0)
    g.out(tex, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(one, "", unreal.MaterialProperty.MP_OPACITY)


def _build_powers(mat):
    """MM_Powers: `DisabledPower` (the colour icon) shows over `EnabledPower` (the grey one) in the sector from
    12 o'clock clockwise by `Percent` (1 = ready, 0 = spent). The original's graph is cooked away; the parameters, the
    two textures' roles and the direction come from its instances and the game's picture (WebGL 10 記録)."""
    g = dd_stage._Graph(mat)
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    colour = g.texture("DisabledPower", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1400, -400)
    grey = g.texture("EnabledPower", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1400, -100)
    percent = g.scalar("Percent", 1.0, -1400, 200)

    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1400, 400)
    centred = g.node(unreal.MaterialExpressionSubtract, -1200, 400)
    MEL.connect_material_expressions(uv, "", centred, "A")
    centred.set_editor_property("const_b", 0.5)
    x = g.node(unreal.MaterialExpressionComponentMask, -1000, 350)
    x.set_editor_property("r", True)
    x.set_editor_property("g", False)
    MEL.connect_material_expressions(centred, "", x, "")
    y = g.node(unreal.MaterialExpressionComponentMask, -1000, 470)
    y.set_editor_property("r", False)
    y.set_editor_property("g", True)
    MEL.connect_material_expressions(centred, "", y, "")
    # UV's +v points down, so 12 o'clock is −v: atan2(u, −v) turns clockwise from the top.
    up = g.node(unreal.MaterialExpressionMultiply, -800, 470)
    MEL.connect_material_expressions(y, "", up, "A")
    up.set_editor_property("const_b", -1.0)
    angle = g.node(unreal.MaterialExpressionArctangent2, -600, 400)
    MEL.connect_material_expressions(x, "", angle, "Y")
    MEL.connect_material_expressions(up, "", angle, "X")
    turns = g.node(unreal.MaterialExpressionDivide, -430, 400)
    MEL.connect_material_expressions(angle, "", turns, "A")
    turns.set_editor_property("const_b", 2.0 * 3.14159265358979)
    wrapped = g.node(unreal.MaterialExpressionAdd, -300, 400)
    MEL.connect_material_expressions(turns, "", wrapped, "A")
    wrapped.set_editor_property("const_b", 1.0)
    frac = g.node(unreal.MaterialExpressionFrac, -180, 400)
    MEL.connect_material_expressions(wrapped, "", frac, "")

    # mask = frac < Percent, as ceil(saturate(Percent − frac)): 1 inside the sector, 0 outside (MaterialExpressionIf's
    # constants are not exposed to Python, and a one-input node's pin has no name — connect it with "").
    gap = g.node(unreal.MaterialExpressionSubtract, 0, 300)
    MEL.connect_material_expressions(percent, "", gap, "A")
    MEL.connect_material_expressions(frac, "", gap, "B")
    inside = g.node(unreal.MaterialExpressionSaturate, 120, 300)
    MEL.connect_material_expressions(gap, "", inside, "")
    mask = g.node(unreal.MaterialExpressionCeil, 240, 300)
    MEL.connect_material_expressions(inside, "", mask, "")

    rgb = g.lerp(grey, "RGB", colour, "RGB", mask, "", 250, -200)
    g.out(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    alpha = g.lerp(grey, "A", colour, "A", mask, "", 250, 100)
    g.out(alpha, "", unreal.MaterialProperty.MP_OPACITY)


def make_minimap_materials():
    """The three masters and the instances the level and the screen use."""
    ensure_render_target()
    plane = _master(MAP_PLANE_MASTER, _build_map_plane)
    _master(MAP_SCREEN_MASTER, _build_map_screen, domain_ui=True)
    powers = _master(POWERS_MASTER, _build_powers, domain_ui=True)

    out = []
    for rel, tex_rel in MAPS:
        mic = _instance(asset(rel), plane)
        MEL.set_material_instance_texture_parameter_value(mic, "Texture", unreal.load_asset(asset(tex_rel)))
        out.append(asset(rel))
    for rel, colour_rel, grey_rel in POWERS:
        mic = _instance(asset(rel), powers)
        MEL.set_material_instance_texture_parameter_value(mic, "DisabledPower", unreal.load_asset(asset(colour_rel)))
        MEL.set_material_instance_texture_parameter_value(mic, "EnabledPower", unreal.load_asset(asset(grey_rel)))
        MEL.set_material_instance_scalar_parameter_value(mic, "Percent", 1.0)
        out.append(asset(rel))
    return out


# ------------------------------------------------------------------------------------------------ everything
def import_all():
    """Imports and builds the whole tablet, then saves /Game/DD and /Game/Pipeline."""
    result = {"textures": len(import_textures())}
    result["materials"] = len(make_body_materials())
    import_mesh()
    result["meshes"] = 1
    result["fonts"] = 1 if import_font() else 0
    result["sounds"] = len(import_sounds())
    result["minimap"] = len(make_minimap_materials())
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
