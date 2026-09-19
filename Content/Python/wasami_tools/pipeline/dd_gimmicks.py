"""Dark Deception's hospital gimmicks: what the stage's moving parts play. The double doors (AWasamiDoubleDoors, after
the original's Blueprints/06_Hospital/BP_06_DoubleDoors): the swing's two sounds and their attenuation
(MonkeyAttenuation), and the locked rattle (Locked_Door, a SoundCue of two waves) with the attenuation the doors play it
through (01_Lobby_Attenuation). The doors' meshes and materials come with the stage's assets (dd_stage), and the level
build puts them on the placed doors (dd_level). The zone barrier (AWasamiZoneBarrier, after
Blueprints/Main/BP_ZoneBarrier): its hum and shatter, the sound it turns the player away with and that sound's
attenuation, its planes' materials (MM_SpeedBarrier, whose graph the cook took away, rebuilt from its compiled shader,
and the barrier's two instances of it) and the burst it breaks with (P_ky_impact3); the level build puts the materials
on the placed barriers. The tunnel's doors broken in (the zone flow, AWasamiZone1Flow): their crash
(DD_TT_Door_BustedOpen_02) and the burst of concrete the level's emitter Fracture_concrete_5 plays (BallisticsVFX's
Fracture_concrete_3, whose materials' graphs the cook took away: estimated, as the particle packs' others are). Zone 2's
cell: the needles' stab as its spikes reach the player (DD_Needle_Trap_R1_V3, AWasamiZone2Flow), and the particles its
sequences fire (06_Hospital_Zone2_Spikes: the sparks the spikes throw, P_06_NurseSparks, and the dust as they come down,
Fracture_dark_slow; 06_Hospital_Zone2_Cell_DoorPicked: the burst at the cell door, Concrete_impact_large), with the four
materials the cook took the graphs of, estimated off their compiled shaders. Zone 2's lifts (AWasamiLift and
AWasamiCornerLift, after Blueprints/06_Hospital/Lifts/Zone2): the clunk as they start and stop (DD_TT_GarageLift_Down,
through MonkeyAttenuation), the loop while they move (DD_TT_Lift_Loop, through 01_Lobby_Attenuation), and the garage
lifts' rising sound (DD_TT_GarageLift_Up); their meshes and materials come with the stage's assets. The garage lifts
(AWasamiGarageLift, after Blueprints/06_Hospital/Lifts/Garage): their skinned mesh and its animation (dd_skeletal). The
parking lot's nurses stabbing at the tunnel's doors (AWasamiEnemy06Chase's Hit FX): the slam (20-Elevator_Slams) and the
dust (P_06_NurseDoorHit, with Whisps_additive, an additive instance of the doors' estimated smoke). Zone 2's ring piece
over the altar (AWasamiRingPiece): its glow (P_08_RingPiece, with MI_ky_primitive_dynB_nonD1, an instance of the
shards' estimated M_ky_primitive). The garage's portal (AWasamiPortal, after Blueprints/00_Ballroom/BP_00_Teleport): its
disc's mesh and textures, M_00_Portal_Vortex (its graph cooked away, rebuilt from its compiled shaders) with the
original's instances, the lock's plane, this game's logo in place of the monkey, its sounds and camera shake, and
Mat_ParameterCol, the material parameter collection its materials read.

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24).
"""
import math
import os

import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_skeletal, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
VERSION = 2   # the hospital is only in the latest version

DOUBLE_DOOR_SOUNDS = (
    "Audio/06_Hospital/SFX_06_DoubleDoor_Open",
    "Audio/06_Hospital/SFX_06_DoubleDoor_Close",
    "Audio/01_Hotel/Locked_Door_v1",
    "Audio/01_Hotel/Locked_Door_v2",
)
DOUBLE_DOOR_CUES = (
    "Audio/01_Hotel/Locked_Door",
)
DOUBLE_DOOR_ATTENUATIONS = (
    "Audio/Misc/MonkeyAttenuation",
    "Audio/01_Hotel/01_Lobby_Attenuation",
)


KY = "ThirdParty/AdvancedMagicFX13/"
# The hum, the shatter, and the sound as a look and a click at the barrier is turned away (DD_RingBarrierDenied_louder,
# through DialogueAttenuation; the ring altar turns the player away with the same).
ZONE_BARRIER_SOUNDS = (
    "Audio/02_School/Barrier_Loop",
    "Audio/02_School/Barrier_Shatter",
    "Audio/RingStatue/DD_RingBarrierDenied_louder",
)
ZONE_BARRIER_ATTENUATIONS = (
    "Audio/01_Hotel/01_Lobby_Attenuation",
    "Audio/Misc/DialogueAttenuation",
)
SPEED_BARRIER_TEXTURE = "Textures/02_ElementarySchool/school_decal_speedBarrier_01_A"
ZONE_BARRIER_TEXTURES = (SPEED_BARRIER_TEXTURE, KY + "Textures/T_ky_flare14_4x4")
# MM_SpeedBarrier: a translucent, lit material whose graph the cook took away; its compiled translucent base pass
# (Tools/dd/cooked_shaders.py "Materials/Shared/MM_SpeedBarrier." --show 26) is what the graph below follows. The
# texture's UVs are scaled about the middle by Min Scale..Max Scale on a sine of Time x Size Pulse Speed, fully where
# the scene lies FadeDistance Secondary behind (a DepthFade) and not at all where it touches; its RGB x Color Multiplier
# is the colour, which glows by Emissive Pulse Min..Max on a sine of Time x Emissive Pulse Speed and, doubled, is the
# base colour; its alpha faded into the depth over FadeDistance, x Opacity Multiplier, is the opacity. The instances'
# Color + Emissive Multiplier is not in the shader (nothing reads it), so it is not made.
SPEED_BARRIER = "Materials/Shared/MM_SpeedBarrier"
ZONE_BARRIER_MATERIALS = ("Materials/Shared/MM_ZoneBarrier_Inst1", "Materials/Shared/MM_ZoneBarrier_Inst2")
# P_ky_impact3's materials: MI_ky_primitive2_trs and the estimated M_ky_flare01_primitive come with the shards' flash
# (dd_shards); MI_ky_flare14R is another instance of that one.
FLARE01 = KY + "Materials/M_ky_flare01_primitive"
IMPACT_MATERIALS = (KY + "Materials/MI_ky_flare14R",)
IMPACT_NEEDS = (FLARE01, KY + "Materials/MI_ky_primitive2_trs")
IMPACT = KY + "Particles/P_ky_impact3"


BVFX = "ThirdParty/BallisticsVFX/Particles/"
DOORS_BUSTED_SOUNDS = (
    "Audio/06_Hospital/DD_TT_Door_BustedOpen_02",
)
CELL_SOUNDS = (
    "Audio/06_Hospital/DD_Needle_Trap_R1_V3",
)
NURSE = "Blueprints/Characters/Nurse/"
FLARES = BVFX + "FXMaterials/Flares/"
FRAGMENTS = BVFX + "FXMaterials/Fragments/"
SMOKE_DUST = BVFX + "FXMaterials/SmokeDust/"
DEBRIS_BASE = FRAGMENTS + "Textures/Stones2x2"
DEBRIS_NORMAL = FRAGMENTS + "Textures/Gravel2x2_normal"
WHISP_BASE = SMOKE_DUST + "Textures/whisp_One_512_8x8"
WHISP_NORMAL = SMOKE_DUST + "Textures/whisp_One_512_8x8_Normal"
BURST_TEXTURES = (DEBRIS_BASE, DEBRIS_NORMAL, WHISP_BASE, WHISP_NORMAL,
                  SMOKE_DUST + "Textures/whisp_redux_2048_12x12", SMOKE_DUST + "Textures/whisp_redux_2048_normal")
BURST = BVFX + "Destruction/Fractures/V2/Fracture_concrete_3"
DUST = "Textures/FX_Textures/dust"
FLARE_WHITE = FLARES + "Textures/Flare_white"
SQUIB_BASE = SMOKE_DUST + "Textures/Squib_one_1024_8x8"
SQUIB_NORMAL = SMOKE_DUST + "Textures/Squib_one_normal"
CELL_TEXTURES = (DUST, FLARE_WHITE, SQUIB_BASE, SQUIB_NORMAL)
# The cell's particle systems, the level's emitters the sequences fire have as their templates (P_06_NurseSparks_24,
# Dirt_impact_2_large_57, MetalDull_impact_Dyn_27). The two of BallisticsVFX also use the smoke and debris the doors
# broken in bring (import_doors_busted), which has to have run.
CELL_PARTICLES = (NURSE + "P_06_NurseSparks", BVFX + "Destruction/Fractures/V2/Fracture_dark_slow",
                  BVFX + "Impacts/LegacyFX/Small-Medium-Large/Concrete/Concrete_impact_large")
CELL_NEEDS = (SMOKE_DUST + "Whisps_trans", SMOKE_DUST + "Whisps_trans2", FRAGMENTS + "DebrisMaster")
# The parking lot's nurses stabbing at the tunnel's doors (AWasamiEnemy06Chase's Hit FX): the slam (20-Elevator_Slams, a
# SoundCue of three of its waves, through 01_Lobby_Attenuation) and the dust (P_06_NurseDoorHit, whose one material,
# Whisps_additive, is an additive instance of Whisps_trans, which import_doors_busted makes).
NURSE_DOOR_HIT_SOUNDS = tuple("Audio/01_Hotel/20-Elevator_Slams_V%d" % n for n in (1, 2, 3))
NURSE_DOOR_HIT_CUES = ("Audio/01_Hotel/20-Elevator_Slams",)
NURSE_DOOR_HIT_MATERIAL = SMOKE_DUST + "Whisps_additive"
NURSE_DOOR_HIT = "Particles/06_Hospital/P_06_NurseDoorHit"
LIFT_SOUNDS = (
    "Audio/06_Hospital/DD_TT_GarageLift_Down",
    "Audio/06_Hospital/DD_TT_GarageLift_Up",
    "Audio/06_Hospital/DD_TT_Lift_Loop",
)
LIFT_ATTENUATIONS = DOUBLE_DOOR_ATTENUATIONS   # MonkeyAttenuation and 01_Lobby_Attenuation
# Zone 2's altar and the ring piece over it (AWasamiRingStatue and AWasamiRingPiece, after Blueprints/01_Hotel/BP_01_Statue
# and Blueprints/08_BearHouse/BP_08_RingPiece_NoPickup): the piece's glow (P_08_RingPiece), whose emitter glowSub draws
# MI_ky_primitive_dynB_nonD1, an instance of M_ky_primitive; that and the other emitter's M_ky_polarGlow02 come with the
# shards (import_dd_shards). The altar's denied sound and its attenuation come with the zone barrier's, the meshes and
# their materials with the stage's assets.
RING_PIECE_MATERIAL = KY + "Materials/MI_ky_primitive_dynB_nonD1"
RING_PIECE_PARENT = KY + "Materials/M_ky_primitive"
RING_PIECE_NEEDS = (RING_PIECE_PARENT, KY + "Materials/M_ky_polarGlow02")
RING_PIECE = "Particles/08_BearHouse/P_08_RingPiece"
# The garage's portal (AWasamiPortal, after Blueprints/00_Ballroom/BP_00_Teleport): the disc's mesh, its textures, its
# master material (M_00_Portal_Vortex, whose graph the cook took away, rebuilt from its compiled shaders) and the
# original's eight instances of it (open and locked, the vortex also masked), the lock's plane (M_00_Portal_Lock, an
# instance of our estimate of M_00_Portal_Monkey) and this game's logo in place of the monkey, the loop and the unlock's
# sounds, the camera shake as it opens, and Mat_ParameterCol, whose Portal Extra Brightness the portal's materials read
# (Zone 2's flow sets it to 40, as the hospital's level does).
PORTAL_MESH = "Meshes/00_Ballroom/circle_portal_decal"
PORTAL_TEXTURES = ("Textures/00_Ballroom/decal_vortex", "Textures/00_Ballroom/portal_inner_active",
                   "Textures/00_Ballroom/portal_outer_active", "Textures/00_Ballroom/portal_lock")
PORTAL_SOUNDS = ("Audio/00_Ballroom/Portal_Sound_v3", "Audio/00_Ballroom/portal_unlocked")
PORTAL_CAMERA_SHAKE = "Blueprints/Main/BP_Portal_CameraShake"
PARAMETER_COLLECTION = "Materials/Special/Mat_ParameterCol"
PORTAL_VORTEX = "Materials/MasterMaterials/M_00_Portal_Vortex"
PORTAL_VORTEX_INSTANCES = tuple("Materials/00_Ballroom/M_00_Portal_Vortex_" + n for n in (
    "Inst", "Masked", "Locked_Inst", "Locked_Inst_Masked", "Outer_Inst", "Outer_Locked_Inst", "Inner_Inst",
    "Inner_Locked_Inst"))
PORTAL_MONKEY = "Materials/00_Ballroom/M_00_Portal_Monkey"
PORTAL_LOGO_MASTER = dd_assets.PIPELINE_MATERIALS + "M_DD_PortalLogo"
PORTAL_LOCK = "Materials/00_Ballroom/M_00_Portal_Lock"
# This game's logo (Tools/dd/prepare_portal_logo.py draws it) and its instance of the logo's master.
PORTAL_LOGO_FILE = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "fx", "portal_wasami.png")
PORTAL_LOGO_TEXTURE = paths.WASAMI_ROOT + "/Portal/T_Portal_Wasami"
# The logo's colour (its image is white; 2026-09-19, the user: the original colours white images in the game, and the
# symbol's red stood out from the rings'): the rings' red (192 in their sRGB textures, 0.5271 linear) at the rings'
# mean brightness against the logo's, (Glow Multiplier 0.05 + Base Glow 0.1) x 0.2 = 0.03 against 0.05 (the original's
# M_00_Portal_Vortex_Outer_Inst and _Inner_Inst; their strobe, 0.1 x sin, averages 0). The original's monkey, red in
# its image, shows at the rings' brightest.
PORTAL_LOGO_TINT = (0.5271 * 0.03 / 0.05, 0.0, 0.0, 1.0)
PORTAL_LOGO = paths.WASAMI_ROOT + "/Portal/MI_Portal_Wasami"
# The monkey's texture settings (_textures.json: sRGB, default compression, the UI group).
PORTAL_LOGO_SETTINGS = {"srgb": True, "compression": None, "lodGroup": "TEXTUREGROUP_UI"}
# The instances name a second texture, Albedo_1, which none of the vortex's compiled shaders samples (the graph's
# compile dropped it), so the estimate has no such parameter and the instances' values of it are left out.
PORTAL_LEFT_OUT = ("Albedo_1",)


def _build_speed_barrier(mat):
    scalars, vectors = dd_assets.parameter_defaults(SPEED_BARRIER, VERSION)
    g = dd_stage._Graph(mat, checked=True)
    mp = unreal.MaterialProperty
    time = g.node(unreal.MaterialExpressionTime, -2400, -500)

    def pulse(speed, low, high, x, y):
        """low..high on a sine (period 1) of Time x speed."""
        timed = g.multiply(time, "", g.scalar(speed, scalars[speed], x, y + 100), "", x + 200, y)
        wave = dd_assets.single(g, unreal.MaterialExpressionSine, timed, "", x + 350, y)
        return g.lerp(g.scalar(low, scalars[low], x + 350, y + 100), "", g.scalar(high, scalars[high], x + 350, y + 200),
                      "", wave, "", x + 600, y)

    # The UVs, scaled about the middle where the scene lies behind.
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -2000, 0)
    centred = g.node(unreal.MaterialExpressionSubtract, -1800, 0)
    centred.set_editor_property("const_b", 0.5)
    g.link(uv, "", centred, "A")
    scale = pulse("Size Pulse Speed", "Min Scale", "Max Scale", -2400, -300)
    scaled = g.binary(unreal.MaterialExpressionDivide, centred, "", scale, "", -1600, 0)
    back = g.node(unreal.MaterialExpressionAdd, -1450, 0)
    back.set_editor_property("const_b", 0.5)
    g.link(scaled, "", back, "A")
    behind = g.node(unreal.MaterialExpressionDepthFade, -1450, 150)
    dd_assets.connect(g.scalar("FadeDistance Secondary", scalars["FadeDistance Secondary"], -1650, 200), "", behind,
                      "FadeDistance")
    coords = g.lerp(uv, "", back, "", behind, "", -1250, 0)
    tex = g.texture("Texture", unreal.load_asset(dd_assets.asset_path(SPEED_BARRIER_TEXTURE)),
                    unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1050, 0)
    g.link(coords, "", tex, "UVs")

    colour = g.multiply(tex, "RGB", g.vector("Color Multiplier", vectors["Color Multiplier"], -1050, 300), "RGB",
                        -750, 0)
    glow = pulse("Emissive Pulse Speed", "Emissive Pulse Min", "Emissive Pulse Max", -1350, 450)
    g.out(g.multiply(colour, "", glow, "", -450, 100), "", mp.MP_EMISSIVE_COLOR)
    g.out(dd_assets.add(g, colour, "", colour, "", -450, -100), "", mp.MP_BASE_COLOR)
    fade = g.node(unreal.MaterialExpressionDepthFade, -650, 300)
    dd_assets.connect(tex, "A", fade, "Opacity")
    dd_assets.connect(g.scalar("FadeDistance", scalars["FadeDistance"], -850, 400), "", fade, "FadeDistance")
    opacity = g.multiply(fade, "", g.scalar("Opacity Multiplier", scalars["Opacity Multiplier"], -650, 450), "",
                         -450, 300)
    g.out(opacity, "", mp.MP_OPACITY)


def _collection_parameter(g, name, x, y):
    """Mat_ParameterCol's scalar name in graph g (the collection has to have been made, make_parameter_collection)."""
    e = g.node(unreal.MaterialExpressionCollectionParameter, x, y)
    e.set_editor_property("collection", unreal.load_asset(dd_assets.asset_path(PARAMETER_COLLECTION)))
    e.set_editor_property("parameter_name", name)
    return e


def _build_portal_vortex(mat):
    """M_00_Portal_Vortex, whose graph the cook took away; the graph writes out its compiled shaders
    (Tools/dd/cooked_shaders.py "MasterMaterials/M_00_Portal_Vortex." --show 40, the translucent base pass, and
    "00_Ballroom/M_00_Portal_Vortex_Masked." --show 3, the masked instance's depth pass). Albedo is sampled at the UVs
    turned about the middle by Time x Speed x 0.25 radians; its RGB, greyed by Desaturation (the 0.3, 0.59, 0.11
    luminance), x (a sine of Time x Strobe Speed (period 1) x Strobe Intensity + Glow Multiplier + Base Glow) x 0.2 x
    (1 + Mat_ParameterCol's Portal Extra Brightness, lerped to its Portal Brightness Locked by Locked) is the emissive
    colour; its alpha is the opacity, and plus DitherTemporalAA's dither the opacity mask of the masked instances. The
    original is lit but has no base colour, so no light shows on it: the estimate is unlit."""
    scalars, _ = dd_assets.parameter_defaults(PORTAL_VORTEX, VERSION)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    # bEnableSeparateTranslucency false: drawn before the depth of field (UE 5's translucency pass).
    mat.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    mat.set_editor_property("used_with_static_lighting", True)
    g = dd_stage._Graph(mat, checked=True)
    time = g.node(unreal.MaterialExpressionTime, -2600, -300)

    def param(name, x, y):
        return g.scalar(name, scalars.get(name, 0.0), x, y)

    # The UVs turned about the middle.
    angle = g.multiply(g.multiply(time, "", param("Speed", -2600, -200), "", -2400, -250), "",
                       dd_assets.constant(g, 0.25, -2400, -150), "", -2250, -250)
    trig = []
    for cls, y in ((unreal.MaterialExpressionSine, -300), (unreal.MaterialExpressionCosine, -150)):
        e = g.node(cls, -2100, y)
        e.set_editor_property("period", 2.0 * math.pi)  # the angle in radians
        g.link(angle, "", e, "")
        trig.append(e)
    sine, cosine = trig
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -2600, 0)
    centred = g.node(unreal.MaterialExpressionSubtract, -2400, 0)
    centred.set_editor_property("const_b", 0.5)
    g.link(uv, "", centred, "A")
    u = dd_assets.channel(g, centred, "", "R", -2250, 0)
    v = dd_assets.channel(g, centred, "", "G", -2250, 100)
    turned_u = g.binary(unreal.MaterialExpressionSubtract, g.multiply(cosine, "", u, "", -1950, -50), "",
                        g.multiply(sine, "", v, "", -1950, 50), "", -1800, 0)
    turned_v = dd_assets.add(g, g.multiply(sine, "", u, "", -1950, 150), "", g.multiply(cosine, "", v, "", -1950, 250),
                             "", -1800, 200)
    turned = g.binary(unreal.MaterialExpressionAppendVector, turned_u, "", turned_v, "", -1650, 100)
    back = g.node(unreal.MaterialExpressionAdd, -1500, 100)
    back.set_editor_property("const_b", 0.5)
    g.link(turned, "", back, "A")
    albedo = g.texture("Albedo", unreal.load_asset(dd_assets.asset_path(PORTAL_TEXTURES[0])),
                       unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1350, 100)
    g.link(back, "", albedo, "UVs")

    # The colour and its strobe.
    grey = g.node(unreal.MaterialExpressionDesaturation, -1000, 0)
    g.link(albedo, "RGB", grey, "")
    g.link(param("Desaturation", -1200, -100), "", grey, "Fraction")
    strobe_phase = g.multiply(time, "", param("Strobe Speed", -1400, -500), "", -1200, -500)
    strobe = dd_assets.single(g, unreal.MaterialExpressionSine, strobe_phase, "", -1050, -500)
    strobe = g.multiply(strobe, "", param("Strobe Intensity", -1050, -400), "", -900, -500)
    glow = dd_assets.add(g, param("Glow Multiplier", -1050, -300), "", param("Base Glow", -1050, -200), "", -900, -300)
    strobe = dd_assets.add(g, strobe, "", glow, "", -750, -450)
    strobe = g.multiply(strobe, "", dd_assets.constant(g, 0.2, -750, -350), "", -600, -450)
    extra = g.lerp(_collection_parameter(g, "Portal Extra Brightness", -1000, -800), "",
                   _collection_parameter(g, "Portal Brightness Locked", -1000, -700), "",
                   param("Locked", -1000, -600), "", -750, -750)
    extra = dd_assets.add(g, extra, "", dd_assets.constant(g, 1.0, -750, -650), "", -600, -700)
    brightness = g.multiply(strobe, "", extra, "", -450, -550)
    g.out(g.multiply(grey, "", brightness, "", -300, -200), "", MP.MP_EMISSIVE_COLOR)
    g.out(albedo, "A", MP.MP_OPACITY)
    dither = dd_assets.function_call(g, "Utility/DitherTemporalAA", -700, 300, dd_assets.FUNCTIONS_02)
    g.link(albedo, "A", dither, "Alpha Threshold")
    g.out(dither, "", MP.MP_OPACITY_MASK)


def _build_portal_logo(mat):
    """M_00_Portal_Monkey, whose graph the cook took away; the graph writes out its compiled translucent base pass
    (Tools/dd/cooked_shaders.py "00_Ballroom/M_00_Portal_Monkey." --show 40): Albedo sampled at the UVs scaled about the
    middle by Scale (ScaleUVsByCenter), its RGB x 0.05 x (1 + Mat_ParameterCol's Portal Extra Brightness) the emissive
    colour and its alpha the opacity. Unlit, as the vortex (no base colour). Albedo defaults to the lock: the monkey is
    the original's character, which this game does not show. This game adds Tint, multiplied into the RGB: white by
    default (the lock as the original), the rings' red on this game's white logo (PORTAL_LOGO_TINT)."""
    scalars, _ = dd_assets.parameter_defaults(PORTAL_MONKEY, VERSION)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_static_lighting", True)
    g = dd_stage._Graph(mat, checked=True)
    scaled = dd_assets.function_call(g, "Texturing/ScaleUVsByCenter", -1000, 0)
    g.link(g.node(unreal.MaterialExpressionTextureCoordinate, -1200, 0), "", scaled, "UVs")
    g.link(g.scalar("Scale", scalars.get("Scale", 1.0), -1200, 100), "", scaled, "Texture Scale")
    albedo = g.texture("Albedo", unreal.load_asset(dd_assets.asset_path(PORTAL_TEXTURES[3])),
                       unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -750, 0)
    g.link(scaled, "", albedo, "UVs")
    extra = dd_assets.add(g, _collection_parameter(g, "Portal Extra Brightness", -750, -300), "",
                          dd_assets.constant(g, 1.0, -750, -200), "", -550, -250)
    tinted = g.multiply(albedo, "RGB", g.vector("Tint", (1.0, 1.0, 1.0, 1.0), -750, 200), "RGB", -550, 50)
    colour = g.multiply(tinted, "", dd_assets.constant(g, 0.05, -550, -100), "", -400, -50)
    g.out(g.multiply(colour, "", extra, "", -250, -150), "", MP.MP_EMISSIVE_COLOR)
    g.out(albedo, "A", MP.MP_OPACITY)


def make_parameter_collection():
    """Mat_ParameterCol with the original's scalars (their names and defaults, in its order), saved. A parameter's id
    (protected from Python) is made with it; an existing collection keeps its parameters and their ids, so the materials
    reading it stay valid. Returns the collection."""
    target = dd_assets.asset_path(PARAMETER_COLLECTION)
    if EAL.does_asset_exist(target):
        collection = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        collection = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    props = dd_assets.main_export(dd_assets.export_json(PARAMETER_COLLECTION, VERSION), PARAMETER_COLLECTION)["props"]
    if props.get("VectorParameters"):
        raise NotImplementedError("%s has vector parameters" % PARAMETER_COLLECTION)
    existing = {str(s.get_editor_property("parameter_name")): s
                for s in collection.get_editor_property("scalar_parameters")}
    scalars = []
    for p in props["ScalarParameters"]:
        s = existing.get(p["ParameterName"]) or unreal.CollectionScalarParameter()
        s.set_editor_property("parameter_name", p["ParameterName"])
        s.set_editor_property("default_value", float(p.get("DefaultValue", 0.0)))
        scalars.append(s)
    collection.set_editor_property("scalar_parameters", scalars)
    EAL.save_loaded_asset(collection, only_if_is_dirty=False)
    return collection


def make_portal_materials():
    """The portal's materials at the original's paths (the vortex's master and its eight instances, the lock) and this
    game's logo, saved. Returns their paths."""
    vortex = dd_assets.material(dd_assets.asset_path(PORTAL_VORTEX), _build_portal_vortex,
                                blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    logo = dd_assets.material(PORTAL_LOGO_MASTER, _build_portal_logo, blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    made = [vortex, logo]
    for parent, children in ((vortex, PORTAL_VORTEX_INSTANCES), (logo, (PORTAL_LOCK,))):
        known = {kind: {str(n) for n in names(parent)} for kind, names in (
            ("scalars", MEL.get_scalar_parameter_names), ("textures", MEL.get_texture_parameter_names))}
        for rel in children:
            scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(rel, VERSION)
            textures = {k: v for k, v in textures.items() if k not in PORTAL_LEFT_OUT}
            unknown = (set(scalars) - known["scalars"]) | (set(textures) - known["textures"])
            if unknown or vectors or masks or switches:
                raise RuntimeError("%s sets %s, which the estimate does not have"
                                   % (rel, sorted(unknown) or (vectors, masks, switches)))
            mic = dd_assets.material_instance(dd_assets.asset_path(rel), parent, scalars=scalars, textures=textures)
            dd_assets.base_property_overrides(mic, rel, VERSION)
            made.append(mic)
    made.append(dd_assets.material_instance(PORTAL_LOGO, logo, vectors={"Tint": PORTAL_LOGO_TINT},
                                            textures={"Albedo": PORTAL_LOGO_TEXTURE}))
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def _lit_particle(mat, responsive_aa=False, spherical_normals=False):
    """The settings the cook kept on BallisticsVFX's smoke and debris: lit translucency (volumetric, directional), for
    sprites."""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("translucency_lighting_mode",
                            unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_DIRECTIONAL)
    mat.set_editor_property("used_with_particle_sprites", True)
    mat.set_editor_property("enable_responsive_aa", responsive_aa)
    mat.set_editor_property("generate_spherical_particle_normals", spherical_normals)


def _sub_uv(g, param, rel, sampler, blend, x, y):
    """A SubUV sample (the frame the emitter's SubUV module picks; blended between frames for its Linear_Blend) of
    param, by default the texture rel."""
    e = g.node(unreal.MaterialExpressionTextureSampleParameterSubUV, x, y)
    e.set_editor_property("parameter_name", param)
    e.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(rel)))
    e.set_editor_property("sampler_type", sampler)
    e.set_editor_property("blend", blend)
    return e


def _flattened(g, normal, pin, amount, x, y):
    """The normal eased towards the particle's facing (0, 0, 1) by amount."""
    return g.lerp(normal, pin, g.const3((0.0, 0.0, 1.0), x - 200, y + 100), "", amount, "", x, y)


def _build_whisp_directional(mat, d):
    """whispOne_Master_directional, estimated. The cook kept its settings (translucent, lit volumetric directional, for
    sprites), the parameters Base Overlay, FlattenNormal, Fade Distance, Opacity, Radius, Master Opacity and the
    textures Base and Normal, the static switch MacroUVNoise and the bool Normal Map, of 36 expressions. Its compiled
    shadow pass reads the opacity: Base's alpha x Opacity x the particle's alpha, faded into the depth over Fade
    Distance, x Master Opacity (Radius fades it only within a few centimetres of the camera, which is not made, nor the
    macro UV noise). The estimate adds a colour of Base's RGB x Base Overlay x the particle's colour, and Normal eased
    flat by FlattenNormal."""
    _lit_particle(mat)
    g = dd_stage._Graph(mat, checked=True)
    st = unreal.MaterialSamplerType
    base = _sub_uv(g, "Base", WHISP_BASE, st.SAMPLERTYPE_COLOR, True, -1300, 0)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1300, 300)
    tinted = g.multiply(base, "RGB", g.vector("Base Overlay", d["Base Overlay"], -1100, -150), "", -900, -50)
    g.out(g.multiply(tinted, "", particle, "RGB", -700, 0), "", MP.MP_BASE_COLOR)
    normal = _sub_uv(g, "Normal", WHISP_NORMAL, st.SAMPLERTYPE_NORMAL, True, -1300, 600)
    g.out(_flattened(g, normal, "RGB", g.scalar("FlattenNormal", d["FlattenNormal"], -1100, 750), -900, 600), "",
          MP.MP_NORMAL)
    alpha = g.multiply(base, "A", g.scalar("Opacity", d["Opacity"], -1100, 150), "", -900, 150)
    alpha = g.multiply(alpha, "", particle, "A", -750, 200)
    alpha = g.multiply(alpha, "", g.scalar("Master Opacity", d["Master Opacity"], -750, 300), "", -600, 200)
    dd_assets.depth_faded_opacity(g, alpha, g.scalar("Fade Distance", d["Fade Distance"], -600, 350), -400, 250)


def _build_whisp_amb(mat, d):
    """whispOne_Master_amb, estimated. The cook kept its settings (translucent, lit volumetric directional, for
    sprites), the parameters ColourOverlay, FlattenNormal, Opacity, Fade Distance, Radius, Hardness, the texture Base, a
    SubUV sample of whisp_One_512_8x8_Normal and the static switch CamFade, of 24 expressions. Its one instance
    (whispOne_Master_amb_Inst) turns CamFade off, which the estimate leaves out with Radius and Hardness (taken as that
    fade's): a colour of Base's RGB x ColourOverlay x the particle's colour, the normal eased flat by FlattenNormal, and
    an opacity of Base's alpha x Opacity x the particle's alpha, faded into the depth over Fade Distance (as
    whispOne_Master_directional's)."""
    _lit_particle(mat)
    g = dd_stage._Graph(mat, checked=True)
    st = unreal.MaterialSamplerType
    base = _sub_uv(g, "Base", WHISP_BASE, st.SAMPLERTYPE_COLOR, True, -1300, 0)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1300, 300)
    tinted = g.multiply(base, "RGB", g.vector("ColourOverlay", d["ColourOverlay"], -1100, -150), "", -900, -50)
    g.out(g.multiply(tinted, "", particle, "RGB", -700, 0), "", MP.MP_BASE_COLOR)
    normal = g.node(unreal.MaterialExpressionParticleSubUV, -1300, 600)
    normal.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(WHISP_NORMAL)))
    normal.set_editor_property("sampler_type", st.SAMPLERTYPE_NORMAL)
    normal.set_editor_property("blend", True)
    g.out(_flattened(g, normal, "RGB", g.scalar("FlattenNormal", d["FlattenNormal"], -1100, 750), -900, 600), "",
          MP.MP_NORMAL)
    alpha = g.multiply(base, "A", g.scalar("Opacity", d["Opacity"], -1100, 150), "", -900, 150)
    alpha = g.multiply(alpha, "", particle, "A", -750, 200)
    dd_assets.depth_faded_opacity(g, alpha, g.scalar("Fade Distance", d["Fade Distance"], -750, 350), -550, 250)


def _build_debris(mat, d):
    """DebrisMaster, estimated. The cook kept its settings (translucent, lit volumetric directional, responsive AA,
    spherical particle normals, for sprites), the parameter Desat, SubUV samples of Base Map (Stones2x2) and Normal
    Map (Gravel2x2_normal) and its normal (Normal Map's RGB), of 10 expressions. The estimate: a colour of Base Map's
    RGB desaturated by Desat x the particle's colour, and an opacity of Base Map's alpha x the particle's alpha."""
    _lit_particle(mat, responsive_aa=True, spherical_normals=True)
    g = dd_stage._Graph(mat, checked=True)
    st = unreal.MaterialSamplerType
    base = _sub_uv(g, "Base Map", DEBRIS_BASE, st.SAMPLERTYPE_COLOR, False, -1100, 0)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1100, 300)
    grey = g.node(unreal.MaterialExpressionDesaturation, -850, 0)
    dd_assets.connect(base, "RGB", grey, "")
    dd_assets.connect(g.scalar("Desat", d["Desat"], -1050, 200), "", grey, "Fraction")
    g.out(g.multiply(grey, "", particle, "RGB", -650, 0), "", MP.MP_BASE_COLOR)
    g.out(g.multiply(base, "A", particle, "A", -650, 200), "", MP.MP_OPACITY)
    normal = _sub_uv(g, "Normal Map", DEBRIS_NORMAL, st.SAMPLERTYPE_NORMAL, False, -1100, 500)
    g.out(normal, "RGB", MP.MP_NORMAL)


def _build_nurse_sparks(mat, d):
    """M_06_NurseSparks, estimated. The cook kept its settings (additive, lit, no separate translucency, for sprites),
    its emissive colour (the particle colour's RGB), a sample of dust and a CameraDepthFade call, of 9 expressions. Its
    compiled translucent base pass (Tools/dd/cooked_shaders.py "Nurse/M_06_NurseSparks." --show 28) is what the graph
    follows: a base and an emissive colour of the particle colour's RGB, and an opacity of dust's alpha to the power 50
    x the particle's alpha, faded into the depth over 50 and by a CameraDepthFade of 400 from 24."""
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    mat.set_editor_property("used_with_particle_sprites", True)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1100, -100)
    g.out(particle, "RGB", MP.MP_BASE_COLOR)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)
    dust = g.node(unreal.MaterialExpressionTextureSample, -1300, 200)
    dust.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(DUST)))
    sharp = g.power(dust, "A", dd_assets.constant(g, 50.0, -1300, 450), "", -1050, 250)
    alpha = g.multiply(sharp, "", particle, "A", -850, 200)
    fade = g.node(unreal.MaterialExpressionDepthFade, -650, 200)
    dd_assets.connect(alpha, "", fade, "Opacity")
    dd_assets.connect(dd_assets.constant(g, 50.0, -850, 350), "", fade, "FadeDistance")
    near = dd_assets.function_call(g, "Opacity/CameraDepthFade", -650, 400)
    dd_assets.connect(dd_assets.constant(g, 400.0, -850, 450), "", near, "Fade Length")
    dd_assets.connect(dd_assets.constant(g, 24.0, -850, 550), "", near, "Fade Offset")
    g.out(g.multiply(fade, "", near, "Result", -400, 300), "", MP.MP_OPACITY)


def _build_spark(mat, d):
    """M_Spark (BallisticsVFX), estimated. The cook kept its settings (additive, unlit, no translucent shadow, for
    sprites) and a sample of Flare_white, of 4 expressions. Its compiled translucent base pass
    (Tools/dd/cooked_shaders.py "Flares/M_Spark." --show 5): an emissive colour of Flare_white's RGB x the particle
    colour's, and an opacity of their alphas multiplied."""
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    dd_assets.particle_material(mat)
    mat.set_editor_property("used_with_mesh_particles", False)
    mat.set_editor_property("translucent_shadow_density_scale", 0.0)
    g = dd_stage._Graph(mat, checked=True)
    flare = g.node(unreal.MaterialExpressionTextureSample, -900, 0)
    flare.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(FLARE_WHITE)))
    particle = g.node(unreal.MaterialExpressionParticleColor, -900, 300)
    g.out(g.multiply(flare, "RGB", particle, "RGB", -600, 0), "", MP.MP_EMISSIVE_COLOR)
    g.out(g.multiply(flare, "A", particle, "A", -600, 250), "", MP.MP_OPACITY)


def _build_radial_gradient(mat, d):
    """M_Radial_Gradient (BallisticsVFX), estimated. The cook kept its settings (translucent, unlit, responsive AA, for
    sprites and beam trails) and a RadialGradient call, of 6 expressions. Its compiled translucent base pass
    (Tools/dd/cooked_shaders.py "Flares/M_Radial_Gradient." --show 5), which the graph writes out: an emissive colour of
    the particle colour's RGB, and an opacity of max(1 - 2 x the UVs' distance from the middle, 0) to the fourth x the
    particle's alpha."""
    dd_assets.particle_material(mat, beam_trails=True, responsive_aa=True)
    mat.set_editor_property("used_with_mesh_particles", False)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -900, -100)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1500, 200)
    middle = g.node(unreal.MaterialExpressionConstant2Vector, -1500, 350)
    middle.set_editor_property("r", 0.5)
    middle.set_editor_property("g", 0.5)
    distance = g.binary(unreal.MaterialExpressionDistance, uv, "", middle, "", -1300, 250)
    doubled = g.multiply(distance, "", dd_assets.constant(g, 2.0, -1300, 400), "", -1150, 250)
    inside = dd_assets.single(g, unreal.MaterialExpressionOneMinus, doubled, "", -1000, 250)
    clamped = g.node(unreal.MaterialExpressionMax, -850, 250)
    clamped.set_editor_property("const_b", 0.0)
    g.link(inside, "", clamped, "A")
    shape = g.power(clamped, "", dd_assets.constant(g, 4.0, -850, 400), "", -700, 250)
    g.out(g.multiply(shape, "", particle, "A", -450, 200), "", MP.MP_OPACITY)


# The lit translucency's values Squib_one's export sets away from UE's defaults (its names → UE 5.8's).
SQUIB_LIGHTING = (("TranslucencyDirectionalLightingIntensity", "translucency_directional_lighting_intensity"),
                  ("TranslucentShadowDensityScale", "translucent_shadow_density_scale"),
                  ("TranslucentSelfShadowSecondDensityScale", "translucent_self_shadow_second_density_scale"),
                  ("TranslucentSelfShadowSecondOpacity", "translucent_self_shadow_second_opacity"))


def _build_squib(mat, d):
    """Squib_one (BallisticsVFX), estimated. The cook kept its settings (translucent, lit volumetric directional,
    spherical particle normals, for sprites, and its translucent lighting and shadow values), the parameters Base,
    FlattenNormal, Fade Distance, Opacity and MasterOpacity, the static switch Cam close fade? (on), a SubUV sample of
    Squib_one_normal and a FlattenNormal call, of 23 expressions. Its compiled translucent base pass
    (Tools/dd/cooked_shaders.py "SmokeDust/Squib_one." --show 28) is what the graph follows: a base colour of Base's RGB
    x the particle colour's (Base sampled at the sprite's own UVs, its SubUV frame), the normal Squib_one_normal's
    blended frames eased flat by FlattenNormal, and an opacity of Base's alpha x the particle's alpha x Opacity, faded
    into the depth over Fade Distance, faded near the camera (none at 25 cm, whole at 250: a CameraDepthFade of 225
    from 25; the switch, which nothing turns off, is not made) and x MasterOpacity."""
    _lit_particle(mat, spherical_normals=True)
    props = dd_assets.main_export(dd_assets.export_json(SMOKE_DUST + "Squib_one", VERSION), SMOKE_DUST + "Squib_one")
    for key, name in SQUIB_LIGHTING:
        mat.set_editor_property(name, float(props["props"][key]))
    g = dd_stage._Graph(mat, checked=True)
    st = unreal.MaterialSamplerType
    base = g.texture("Base", unreal.load_asset(dd_assets.asset_path(SQUIB_BASE)), st.SAMPLERTYPE_COLOR, -1300, 0)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1300, 300)
    g.out(g.multiply(base, "RGB", particle, "RGB", -900, 0), "", MP.MP_BASE_COLOR)
    normal = g.node(unreal.MaterialExpressionParticleSubUV, -1300, 700)
    normal.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(SQUIB_NORMAL)))
    normal.set_editor_property("sampler_type", st.SAMPLERTYPE_NORMAL)
    normal.set_editor_property("blend", True)
    g.out(_flattened(g, normal, "RGB", g.scalar("FlattenNormal", d["FlattenNormal"], -1100, 850), -900, 700), "",
          MP.MP_NORMAL)
    alpha = g.multiply(base, "A", particle, "A", -1000, 200)
    alpha = g.multiply(alpha, "", g.scalar("Opacity", d["Opacity"], -1000, 300), "", -850, 250)
    fade = g.node(unreal.MaterialExpressionDepthFade, -650, 250)
    dd_assets.connect(alpha, "", fade, "Opacity")
    dd_assets.connect(g.scalar("Fade Distance", d["Fade Distance"], -850, 400), "", fade, "FadeDistance")
    near = dd_assets.function_call(g, "Opacity/CameraDepthFade", -650, 450)
    dd_assets.connect(dd_assets.constant(g, 225.0, -850, 500), "", near, "Fade Length")
    dd_assets.connect(dd_assets.constant(g, 25.0, -850, 600), "", near, "Fade Offset")
    faded = g.multiply(fade, "", near, "Result", -450, 300)
    g.out(g.multiply(faded, "", g.scalar("MasterOpacity", d["MasterOpacity"], -650, 650), "", -300, 350), "",
          MP.MP_OPACITY)


# (the original's material, the master holding our estimate, its builder, the original's instances of it)
SMOKE_MATERIALS = (
    ("whispOne_Master_directional", "M_DD_WhispDirectional", _build_whisp_directional,
     ("Whisps_trans", "Whisps_trans2")),
    ("whispOne_Master_amb", "M_DD_WhispAmb", _build_whisp_amb, ("whispOne_Master_amb_Inst",)),
)
DEBRIS_MATERIALS = (
    ("DebrisMaster", "M_DD_Debris", _build_debris, ()),
)
# The cell's particles' materials, by folder. The builders set their own blend where it is additive.
CELL_MATERIALS = (
    (NURSE, (("M_06_NurseSparks", "M_DD_NurseSparks", _build_nurse_sparks, ()),)),
    (FLARES, (("M_Spark", "M_DD_BvfxSpark", _build_spark, ()),
              ("M_Radial_Gradient", "M_DD_BvfxRadialGradient", _build_radial_gradient, ()))),
    (SMOKE_DUST, (("Squib_one", "M_DD_Squib", _build_squib, ()),)),
)
# What the estimates do without, which the original's instances set: whispOne_Master_amb's camera fade (its instance
# turns it off) and whispOne_Master_directional's Radius (a fade within centimetres of the camera).
SMOKE_LEFT_OUT = ("CamFade", "Radius")


def make_zone_barrier_materials():
    """MM_SpeedBarrier and the barrier's instances of it (with their own values and overrides), and P_ky_impact3's
    MI_ky_flare14R, at the original's paths (saved)."""
    speed_barrier = dd_assets.material(dd_assets.asset_path(SPEED_BARRIER), _build_speed_barrier,
                                       blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    # bEnableSeparateTranslucency false: drawn before the depth of field (UE 5's translucency pass).
    speed_barrier.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    unreal.MaterialEditingLibrary.recompile_material(speed_barrier)
    made = [speed_barrier]
    for parent, children in ((speed_barrier, ZONE_BARRIER_MATERIALS),
                             (unreal.load_asset(dd_assets.asset_path(FLARE01)), IMPACT_MATERIALS)):
        known = {str(n) for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(parent)}
        for rel in children:
            scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(rel, VERSION)
            mic = dd_assets.material_instance(dd_assets.asset_path(rel), parent,
                                              scalars={k: v for k, v in scalars.items() if k in known},
                                              vectors=vectors, textures=textures, static_masks=masks,
                                              static_switches=switches)
            dd_assets.base_property_overrides(mic, rel, VERSION)
            made.append(mic)
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def import_zone_barrier():
    """The zone barrier's sounds, attenuation, textures, materials and burst. The burst's other materials come with the
    shards (WasamiDDTools.import_dd_shards), which has to have run. Returns how many of each."""
    missing = [rel for rel in IMPACT_NEEDS if not EAL.does_asset_exist(dd_assets.asset_path(rel))]
    if missing:
        raise RuntimeError("missing %s: run WasamiDDTools.import_dd_shards first" % ", ".join(missing))
    result = {"attenuations": len([dd_assets.sound_attenuation(rel, VERSION) for rel in ZONE_BARRIER_ATTENUATIONS]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in ZONE_BARRIER_SOUNDS]),
              "textures": len([dd_assets.texture(rel, VERSION) for rel in ZONE_BARRIER_TEXTURES])}
    result["materials"] = len(make_zone_barrier_materials())
    dd_particles.particle_system(IMPACT, VERSION)
    result["particle_systems"] = 1
    return result


def import_doors_busted():
    """The tunnel's doors broken in: their crash, and the burst of concrete (Fracture_concrete_3) with its textures and
    estimated materials. Returns how many of each."""
    result = {"sounds": len([dd_assets.sound(rel, VERSION) for rel in DOORS_BUSTED_SOUNDS]),
              "textures": len([dd_assets.texture(rel, VERSION) for rel in BURST_TEXTURES])}
    made = dd_assets.estimated_materials(SMOKE_DUST, SMOKE_MATERIALS, VERSION, SMOKE_LEFT_OUT)
    made += dd_assets.estimated_materials(FRAGMENTS, DEBRIS_MATERIALS, VERSION)
    result["materials"] = len(made)
    dd_particles.particle_system(BURST, VERSION)
    result["particle_systems"] = 1
    return result


def import_cell():
    """Zone 2's cell: the needles' stab, and the particles its sequences fire with their textures and estimated
    materials. The doors broken in (import_doors_busted) have to have been imported. Returns how many of each."""
    missing = [rel for rel in CELL_NEEDS if not EAL.does_asset_exist(dd_assets.asset_path(rel))]
    if missing:
        raise RuntimeError("missing %s: run import_doors_busted first" % ", ".join(missing))
    result = {"sounds": len([dd_assets.sound(rel, VERSION) for rel in CELL_SOUNDS]),
              "textures": len([dd_assets.texture(rel, VERSION) for rel in CELL_TEXTURES])}
    made = []
    for folder, entries in CELL_MATERIALS:
        made += dd_assets.estimated_materials(folder, entries, VERSION)
    result["materials"] = len(made)
    result["particle_systems"] = len([dd_particles.particle_system(rel, VERSION) for rel in CELL_PARTICLES])
    return result


def import_nurse_door_hit():
    """The parking lot's nurses' Hit FX: the slam's waves and SoundCue, the dust's material and particle system. The doors
    broken in (import_doors_busted) have to have been imported. Returns how many of each."""
    parent_rel = SMOKE_DUST + "Whisps_trans"
    if not EAL.does_asset_exist(dd_assets.asset_path(parent_rel)):
        raise RuntimeError("missing %s: run import_doors_busted first" % parent_rel)
    result = {"sounds": len([dd_assets.sound(rel, VERSION) for rel in NURSE_DOOR_HIT_SOUNDS])}
    result["sound_cues"] = len([dd_assets.sound_cue(rel, VERSION) for rel in NURSE_DOOR_HIT_CUES])
    parent = unreal.load_asset(dd_assets.asset_path(parent_rel))
    known = {str(n) for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(parent)}
    scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(NURSE_DOOR_HIT_MATERIAL, VERSION)
    scalars = {k: v for k, v in scalars.items() if k not in SMOKE_LEFT_OUT}
    unknown = set(scalars) - known
    if unknown or vectors or textures or masks or switches:
        raise RuntimeError("%s sets %s, which the estimate of Whisps_trans does not have"
                           % (NURSE_DOOR_HIT_MATERIAL, sorted(unknown) or (vectors, textures, masks, switches)))
    mic = dd_assets.material_instance(dd_assets.asset_path(NURSE_DOOR_HIT_MATERIAL), parent, scalars=scalars)
    dd_assets.base_property_overrides(mic, NURSE_DOOR_HIT_MATERIAL, VERSION)
    EAL.save_loaded_asset(mic, only_if_is_dirty=False)
    result["materials"] = 1
    dd_particles.particle_system(NURSE_DOOR_HIT, VERSION)
    result["particle_systems"] = 1
    return result


def import_ring_statue():
    """The ring piece's glow: its material (an instance of the shards' M_ky_primitive) and particle system. The shards
    (WasamiDDTools.import_dd_shards) have to have been imported. Returns how many of each."""
    missing = [rel for rel in RING_PIECE_NEEDS if not EAL.does_asset_exist(dd_assets.asset_path(rel))]
    if missing:
        raise RuntimeError("missing %s: run WasamiDDTools.import_dd_shards first" % ", ".join(missing))
    parent = unreal.load_asset(dd_assets.asset_path(RING_PIECE_PARENT))
    known = {str(n) for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(parent)}
    scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(RING_PIECE_MATERIAL, VERSION)
    unknown = set(scalars) - known
    if unknown or vectors or textures or masks or switches:
        raise RuntimeError("%s sets %s, which the estimate of M_ky_primitive does not have"
                           % (RING_PIECE_MATERIAL, sorted(unknown) or (vectors, textures, masks, switches)))
    mic = dd_assets.material_instance(dd_assets.asset_path(RING_PIECE_MATERIAL), parent, scalars=scalars)
    dd_assets.base_property_overrides(mic, RING_PIECE_MATERIAL, VERSION)
    EAL.save_loaded_asset(mic, only_if_is_dirty=False)
    dd_particles.particle_system(RING_PIECE, VERSION)
    return {"materials": 1, "particle_systems": 1}


def import_portal():
    """The garage portal's collection, textures, mesh, materials, logo, sounds and camera shake (saved). The logo's
    image has to have been drawn (python Tools/dd/prepare_portal_logo.py). Returns how many of each."""
    if not os.path.exists(PORTAL_LOGO_FILE):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_portal_logo.py first." % PORTAL_LOGO_FILE)
    make_parameter_collection()
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in PORTAL_TEXTURES]) + 1}
    logo = dd_stage.import_texture(dict(PORTAL_LOGO_SETTINGS, file=PORTAL_LOGO_FILE, asset=PORTAL_LOGO_TEXTURE))
    EAL.save_loaded_asset(logo, only_if_is_dirty=False)
    dd_assets.static_mesh(PORTAL_MESH, VERSION)
    result["meshes"] = 1
    result["materials"] = len(make_portal_materials())
    result["sounds"] = len([dd_assets.sound(rel, VERSION) for rel in PORTAL_SOUNDS])
    dd_assets.camera_shake(PORTAL_CAMERA_SHAKE, VERSION)
    result["camera_shakes"] = 1
    result["parameter_collections"] = 1
    return result


def import_double_doors():
    """The double doors' sounds, SoundCue and attenuations. Returns how many of each."""
    result = {"attenuations": len([dd_assets.sound_attenuation(rel, VERSION) for rel in DOUBLE_DOOR_ATTENUATIONS]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in DOUBLE_DOOR_SOUNDS])}
    result["sound_cues"] = len([dd_assets.sound_cue(rel, VERSION) for rel in DOUBLE_DOOR_CUES])
    return result


def import_lifts():
    """The lifts' sounds and attenuations. Returns how many of each."""
    return {"attenuations": len([dd_assets.sound_attenuation(rel, VERSION) for rel in LIFT_ATTENUATIONS]),
            "sounds": len([dd_assets.sound(rel, VERSION) for rel in LIFT_SOUNDS])}


def import_garage_lift():
    """The garage lifts' skinned mesh and its animation (their sounds come with the lifts'). Returns how many of each."""
    dd_skeletal.import_garage_lift()
    return {"skeletal_meshes": 1, "animations": 1}


def import_all():
    """Imports the gimmicks' assets (the double doors', the zone barrier's, the doors broken in, the cell's, the
    nurses' stabs at the doors, the lifts', the garage lifts', the ring piece's and the portal's), then saves /Game/DD."""
    result = {"double_door_" + key: count for key, count in import_double_doors().items()}
    result.update({"zone_barrier_" + key: count for key, count in import_zone_barrier().items()})
    result.update({"doors_busted_" + key: count for key, count in import_doors_busted().items()})
    result.update({"cell_" + key: count for key, count in import_cell().items()})
    result.update({"nurse_door_hit_" + key: count for key, count in import_nurse_door_hit().items()})
    result.update({"lift_" + key: count for key, count in import_lifts().items()})
    result.update({"garage_lift_" + key: count for key, count in import_garage_lift().items()})
    result.update({"ring_piece_" + key: count for key, count in import_ring_statue().items()})
    result.update({"portal_" + key: count for key, count in import_portal().items()})
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
