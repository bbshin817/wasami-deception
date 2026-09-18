"""Dark Deception's hospital gimmicks: what the stage's moving parts play. The double doors (AWasamiDoubleDoors, after
the original's Blueprints/06_Hospital/BP_06_DoubleDoors): the swing's two sounds and their attenuation (MonkeyAttenuation),
and the locked rattle (Locked_Door, a SoundCue of two waves) with the attenuation the doors play it through
(01_Lobby_Attenuation). The doors' meshes and materials come with the stage's assets (dd_stage), and the level build puts
them on the placed doors (dd_level). The zone barrier (AWasamiZoneBarrier, after Blueprints/Main/BP_ZoneBarrier): its
hum and shatter, its planes' materials (MM_SpeedBarrier, whose graph the cook took away, rebuilt from its compiled
shader, and the barrier's two instances of it) and the burst it breaks with (P_ky_impact3); the level build puts the
materials on the placed barriers. The tunnel's doors broken in (the zone flow, AWasamiZone1Flow): their crash
(DD_TT_Door_BustedOpen_02) and the burst of concrete the level's emitter Fracture_concrete_5 plays (BallisticsVFX's
Fracture_concrete_3, whose materials' graphs the cook took away: estimated, as the particle packs' others are).

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_stage, paths

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
ZONE_BARRIER_SOUNDS = (
    "Audio/02_School/Barrier_Loop",
    "Audio/02_School/Barrier_Shatter",
)
ZONE_BARRIER_ATTENUATIONS = (
    "Audio/01_Hotel/01_Lobby_Attenuation",
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
FRAGMENTS = BVFX + "FXMaterials/Fragments/"
SMOKE_DUST = BVFX + "FXMaterials/SmokeDust/"
DEBRIS_BASE = FRAGMENTS + "Textures/Stones2x2"
DEBRIS_NORMAL = FRAGMENTS + "Textures/Gravel2x2_normal"
WHISP_BASE = SMOKE_DUST + "Textures/whisp_One_512_8x8"
WHISP_NORMAL = SMOKE_DUST + "Textures/whisp_One_512_8x8_Normal"
BURST_TEXTURES = (DEBRIS_BASE, DEBRIS_NORMAL, WHISP_BASE, WHISP_NORMAL,
                  SMOKE_DUST + "Textures/whisp_redux_2048_12x12", SMOKE_DUST + "Textures/whisp_redux_2048_normal")
BURST = BVFX + "Destruction/Fractures/V2/Fracture_concrete_3"


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


# (the original's material, the master holding our estimate, its builder, the original's instances of it)
SMOKE_MATERIALS = (
    ("whispOne_Master_directional", "M_DD_WhispDirectional", _build_whisp_directional,
     ("Whisps_trans", "Whisps_trans2")),
    ("whispOne_Master_amb", "M_DD_WhispAmb", _build_whisp_amb, ("whispOne_Master_amb_Inst",)),
)
DEBRIS_MATERIALS = (
    ("DebrisMaster", "M_DD_Debris", _build_debris, ()),
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


def import_double_doors():
    """The double doors' sounds, SoundCue and attenuations. Returns how many of each."""
    result = {"attenuations": len([dd_assets.sound_attenuation(rel, VERSION) for rel in DOUBLE_DOOR_ATTENUATIONS]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in DOUBLE_DOOR_SOUNDS])}
    result["sound_cues"] = len([dd_assets.sound_cue(rel, VERSION) for rel in DOUBLE_DOOR_CUES])
    return result


def import_all():
    """Imports the gimmicks' assets (the double doors', the zone barrier's and the doors broken in), then saves
    /Game/DD."""
    result = {"double_door_" + key: count for key, count in import_double_doors().items()}
    result.update({"zone_barrier_" + key: count for key, count in import_zone_barrier().items()})
    result.update({"doors_busted_" + key: count for key, count in import_doors_busted().items()})
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
