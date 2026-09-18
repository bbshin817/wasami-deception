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
dust (P_06_NurseDoorHit, with Whisps_additive, an additive instance of the doors' estimated smoke).

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_skeletal, dd_stage, paths

EAL = unreal.EditorAssetLibrary
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
    nurses' stabs at the doors, the lifts' and the garage lifts'), then saves /Game/DD."""
    result = {"double_door_" + key: count for key, count in import_double_doors().items()}
    result.update({"zone_barrier_" + key: count for key, count in import_zone_barrier().items()})
    result.update({"doors_busted_" + key: count for key, count in import_doors_busted().items()})
    result.update({"cell_" + key: count for key, count in import_cell().items()})
    result.update({"nurse_door_hit_" + key: count for key, count in import_nurse_door_hit().items()})
    result.update({"lift_" + key: count for key, count in import_lifts().items()})
    result.update({"garage_lift_" + key: count for key, count in import_garage_lift().items()})
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
