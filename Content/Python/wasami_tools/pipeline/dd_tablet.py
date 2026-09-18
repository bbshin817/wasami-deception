"""Dark Deception's tablet: the plate the player holds (mesh, materials, textures), the screen's UI textures and font,
the two woosh sounds, and the minimap (the level's map plane, the render target the player's scene capture draws into,
the materials that show it, and the arrow on the map).

Everything lands under /Game/DD mirroring the original's own /Game tree, except the master materials we have to write
ourselves (the originals' graphs are cooked away), which go next to the stage's under /Game/Pipeline/Materials:
  M_DD_MapPlane   MM_Map_Parent — the map image on a plane in the world, read by the capture's SCS_BaseColor
  M_DD_MapScreen  M_NewMap — the capture's render target on the tablet's screen (a User Interface material)
  M_DD_Powers     MM_Powers — a power's icon in colour over the grey one, clockwise from 12 o'clock by `Percent`
  M_DD_Arrow      M_Arrow — the map's arrow (BP_ArrowPointer's plane), a pulse of its `Color` cut out by T_Arrow

Sources: pak_reference (UE 4.21) for the tablet and its UI, pak_reference_2 (UE 4.24) for the hospital's map images
and the icons of the four powers the older tablet does not show (Telepathy, Primal Fear, Telekinesis, Vanish).
"""
import os

import unreal

from wasami_tools.pipeline import dd_assets, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

MAP_PLANE_MASTER = "/Game/Pipeline/Materials/M_DD_MapPlane"
MAP_SCREEN_MASTER = "/Game/Pipeline/Materials/M_DD_MapScreen"
POWERS_MASTER = "/Game/Pipeline/Materials/M_DD_Powers"
ARROW_MASTER = "/Game/Pipeline/Materials/M_DD_Arrow"

# The original's own paths, under /Game/DD.
MESH = "Meshes/Player/Tablet/tablet_new_pCube2"
MINIMAP_TARGET = "UI/Minimap/T_NewMap"
FONT_FACE = "UI/Fonts/helvetica-neue-bold"

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
    # The four powers the older version does not put on the tablet (the PNGs and settings are the same in both).
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_telepathy_icon"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_telepathy_icon_inactive"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_primal_icon"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_primal_icon_inactive"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_telekinesis_icon"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_telekinesis_icon_inactive"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_vanish_icon"),
    (2, "UI/RingAltar_UI/Textures/ring_altar_power_vanish_icon_inactive"),
    # The hospital's baked maps (the plane in the level carries them; chapter 6 is not in the older export).
    (2, "UI/Minimap/T_06_Zone01"),
    (2, "UI/Minimap/T_06_Zone2"),
    (2, "UI/Minimap/T_06_Zone2_02"),       # Zone 2's upper floor (BP_MapTexture_MultiFloor's Map)
    # The map's arrow (M_Arrow's texture).
    (2, "Materials/Special/T_Arrow"),
)

# The tablet's two material slots: (the original's material, its albedo). Normal and packed are shared.
BODY_MATERIALS = (
    ("Materials/Player/Tablet/M_P_TabletBack", "Textures/Characters/Player/Tablet/Tablet_Back_D"),
    ("Materials/Player/Tablet/M_P_TabletFront", "Textures/Characters/Player/Tablet/Tablet_Front_D"),
)
BODY_NORMAL = "Textures/Characters/Player/Tablet/Tablet_N"
BODY_PACKED = "Textures/Characters/Player/Tablet/Tablet_S"

# The original's sounds, under its /Game (the SoundWave's own settings come from the export).
SOUNDS = (
    "Audio/SharedGameplay/05_Tablet_Woosh_v1_1",
    "Audio/SharedGameplay/05_Tablet_Woosh_v2_1",
    "Audio/UI/UI_Select_V3",
)

# One material instance per zone's map image, as the original's MM_Map_06_Zone01 / _Zone2.
MAPS = (("UI/Minimap/MM_Map_06_Zone01", "UI/Minimap/T_06_Zone01"),
        ("UI/Minimap/MM_Map_06_Zone2", "UI/Minimap/T_06_Zone2"))
# The power icons, as the latest version's MM_Powers instances in Enum_RingAltar_Skills order:
# (instance, the icon file's stem under UI/RingAltar_UI/Textures). DisabledPower is the colour icon, EnabledPower the
# grey `_inactive` one, as the instances have them.
POWERS = (("Materials/MasterMaterials/MM_Powers_SpeedBoost", "ring_altar_power_speed_boost_icon"),
          ("Materials/MasterMaterials/MM_Powers_Inst_Teleport", "ring_altar_power_teleport_icon"),
          ("Materials/MasterMaterials/MM_Powers_Inst_Telepathy", "ring_altar_power_telepathy_icon"),
          ("Materials/MasterMaterials/MM_Powers_PrimalFear", "ring_altar_power_primal_icon"),
          ("Materials/MasterMaterials/MM_Powers_Inst_Telekinesis", "ring_altar_power_telekinesis_icon"),
          ("Materials/MasterMaterials/MM_Powers_Vanish", "ring_altar_power_vanish_icon"))
POWER_ICONS = "UI/RingAltar_UI/Textures/"
# The map's arrow: the original's M_Arrow (its graph cooked away) as an instance of M_DD_Arrow with its default Color,
# and M_Arrow_Inst, the one BP_ArrowPointer's plane has, as an instance of that with its own.
ARROW = "Materials/Special/M_Arrow"
ARROW_INSTANCE = "Materials/Special/M_Arrow_Inst"
ARROW_TEXTURE = "Materials/Special/T_Arrow"


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


def asset(rel):
    """The original's /Game/<rel> under our /Game/DD."""
    return paths.DD_ROOT + "/" + rel


# ------------------------------------------------------------------------------------------------ textures / mesh
def import_textures():
    """Imports the PNGs the tablet uses with the original's texture settings."""
    return [dd_assets.texture(rel, version) for version, rel in TEXTURES]


def import_mesh():
    """The tablet plate (18.5 × 1 × 23.8 cm, slots phong2 = back and phong3 = front), with its two materials."""
    gltf = os.path.join(paths.DD_PAK2, "_meshes_gltf", "Meshes", "Player", "Tablet", "tablet_new_pCube2.gltf")
    # The StaticMesh's own lightmap settings (64 texels on UV 2), with UStaticMesh's defaults where the export has none.
    props = dd_assets.main_export(dd_assets.export_json(MESH, 2), MESH)["props"]
    mesh = dd_stage.import_mesh({"file": gltf, "asset": asset(MESH), "slots": ["phong2", "phong3"],
                                 "lightmapResolution": props.get("LightMapResolution", 4),
                                 "lightmapUv": props.get("LightMapCoordinateIndex", 0)}, nanite=False)
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
    return dd_assets.font(FONT_FACE)


def import_sounds():
    """The tablet's woosh up / down and the UI select the map's resize plays."""
    return [dd_assets.sound(rel, 1) for rel in SOUNDS]


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


def _build_arrow(mat):
    """M_Arrow, read from its compiled base pass (Tools/dd/cooked_shaders.py "Materials/Special/M_Arrow." --show 23):
    the base and emissive colour are Lerp(Color + 0.6, Color, sin(2π Time)) — a pulse a second between the colour and a
    whiter one — and T_Arrow's red is the opacity mask (a plain texture sample, not a parameter, as the original's).
    The base colour is what the minimap's capture reads (SCS_BaseColor)."""
    g = dd_stage._Graph(mat)
    _, vectors = dd_assets.parameter_defaults(ARROW, 2)
    colour = g.vector("Color", vectors["Color"], -900, -100)
    whiter = g.node(unreal.MaterialExpressionAdd, -650, -250)
    whiter.set_editor_property("const_b", 0.6)
    dd_assets.connect(colour, "RGB", whiter, "A")
    time = g.node(unreal.MaterialExpressionTime, -900, 150)
    wave = dd_assets.single(g, unreal.MaterialExpressionSine, time, "", -650, 150)
    pulse = g.lerp(whiter, "", colour, "RGB", wave, "", -400, -100)
    g.out(pulse, "", unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(pulse, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    tex = g.node(unreal.MaterialExpressionTextureSample, -650, 350)
    tex.set_editor_property("texture", unreal.load_asset(asset(ARROW_TEXTURE)))
    g.out(tex, "R", unreal.MaterialProperty.MP_OPACITY_MASK)


def make_arrow_materials():
    """M_DD_Arrow (masked, the default clip value 0.3333 as the original's), M_Arrow with its default Color and
    M_Arrow_Inst with its own Color and base property overrides. Returns M_Arrow_Inst's path."""
    master = dd_assets.material(ARROW_MASTER, _build_arrow, blend_mode=unreal.BlendMode.BLEND_MASKED)
    _, vectors = dd_assets.parameter_defaults(ARROW, 2)
    base = dd_assets.material_instance(asset(ARROW), master, vectors=vectors)
    scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(ARROW_INSTANCE, 2)
    if scalars or textures or masks or switches:
        raise RuntimeError("%s sets more than Color: %s" % (ARROW_INSTANCE, (scalars, textures, masks, switches)))
    mic = dd_assets.material_instance(asset(ARROW_INSTANCE), base, vectors=vectors)
    dd_assets.base_property_overrides(mic, ARROW_INSTANCE, 2)
    return asset(ARROW_INSTANCE)


def make_minimap_materials():
    """The three masters and the instances the level and the screen use."""
    ensure_render_target()
    plane = dd_assets.material(MAP_PLANE_MASTER, _build_map_plane)
    ui = {"domain": unreal.MaterialDomain.MD_UI, "blend_mode": unreal.BlendMode.BLEND_TRANSLUCENT}
    dd_assets.material(MAP_SCREEN_MASTER, _build_map_screen, **ui)
    powers = dd_assets.material(POWERS_MASTER, _build_powers, **ui)

    out = []
    for rel, tex_rel in MAPS:
        mic = _instance(asset(rel), plane)
        MEL.set_material_instance_texture_parameter_value(mic, "Texture", unreal.load_asset(asset(tex_rel)))
        out.append(asset(rel))
    for rel, icon in POWERS:
        mic = _instance(asset(rel), powers)
        colour = unreal.load_asset(asset(POWER_ICONS + icon))
        grey = unreal.load_asset(asset(POWER_ICONS + icon + "_inactive"))
        MEL.set_material_instance_texture_parameter_value(mic, "DisabledPower", colour)
        MEL.set_material_instance_texture_parameter_value(mic, "EnabledPower", grey)
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
    result["arrow"] = 1 if make_arrow_materials() else 0
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
