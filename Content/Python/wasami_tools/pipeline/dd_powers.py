"""Dark Deception's tablet powers: the sounds, camera shakes, camera anims, textures, materials and particle systems
the power system (UWasamiPowerComponent), the teleport's aim (AWasamiTeleportAim), Primal Fear (AWasamiPrimalPower),
Vanish (AWasamiVanishPower, UWasamiVanishWidget), the telekinesis (AWasamiTelekinesisPower) and the player's FX
(UWasamiChameleonComponent) use. The icons on the
tablet's sockets are the tablet's own (dd_tablet). The telepathy's markers (UWasamiTelepathyTrackerWidget) show
MM_Telepathy_Inst.

  M_Speedlines                  the original's graph, node for node (FlipBook at its defaults → T_Speedlines)
  M_DD_ChameleonCameraShake     the Chameleon pack's M_CameraShake, estimated (its graph is cooked away)
  M_DD_KySlash, M_DD_PPPRadialGradient, M_DD_DecalTeleport
                                estimated masters of the teleport aim's materials (their graphs are cooked away); the
                                original's paths hold instances of them with the original's parameter values
  M_DD_Primal                   the estimated master of Primal Fear's sphere (M_05_Primal's graph is cooked away)
  M_DD_LoopingSmoke, M_DD_WobblyVignette
                                estimated masters of Vanish's puff (M_LoopingSmoke1_Sheet) and vignette
                                (MM_WobblyVignette), whose graphs are cooked away; instances of them sit at the original's
                                paths
  M_DD_Telepathy                the estimated master of the telepathy's marker (MM_Telepathy's graph is cooked away);
                                MM_Telepathy is an instance of it and MM_Telepathy_Inst an instance of that, as the
                                original's
  M_DD_KyWall02, M_DD_KyAura7, M_DD_KyShockWave02, M_DD_KyStarDust
                                estimated masters of the telekinesis's force field (P_ky_forceField_Telekinesis), whose
                                graphs are cooked away; the original's paths hold instances of them, and the original's
                                instances are instances of those

Sources: pak_reference_2 (UE 4.24, the latest version), which the powers follow except the teleport (pak_reference).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty

# (pak_reference version, the original's path under /Game).
SOUNDS = (
    (2, "Audio/UI/power_refilled"),             # a power is ready again
    (2, "Audio/UI/Shard_Streak_Milestone_V5"),  # the speed boost starts
    # the teleport (pak_reference): aiming starts, the aim's loop, the move
    (1, "/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered"),
    (1, "Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227"),
    (1, "/Engine/VREditor/Sounds/UI/Teleport_Committed"),
    (2, "Audio/SharedGameplay/Stun_Wave_Attack_New_04"),  # Primal Fear
    (2, "Audio/SharedGameplay/Telepathy"),  # the telepathy starts (it ends with Teleport_Mode_Entered, the same file)
)
CAMERA_SHAKES = (
    (2, "UI/Menu/Streaks/BP_CameraShake_Streak"),  # the speed boost starts, the teleport moves (the same in both versions)
    (2, "Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop"),  # Primal Fear, at a scale of 25
)
CAMERA_ANIMS = (
    (2, "Animation/Camera/CameraAnim_SpeedBoost"),  # the view turns red while the speed boost lasts
    (1, "Animation/Camera/CameraAnim_Teleport"),    # the teleport's click: the view widens, flashes and settles
)
TEXTURES = (
    (2, "UI/Main/Powers/T_Speedlines"),    # UMG_SpeedBoost's lines (a 2 × 5 sheet; M_Speedlines reads it as 2 × 2)
    (2, "UI/Menu/Streaks/T_VignetteNew"),  # UMG_SpeedBoost's vignette
    (1, "ThirdParty/AdvancedMagicFX13/Textures/T_ky_slash01_4x4"),  # the teleport aim's slashes (4 × 4 frames)
    (2, "Textures/05_Circus/T_05_PortalMaps"),  # Primal Fear's sphere (R sparkles, G a centred glow, B cloudy noise)
    (2, "Particles/Shared/SmokeTest/T_LoopingSmoke_8x8"),  # Vanish's puff (8 × 8 frames of grey smoke over alpha)
    (2, "Textures/FX_Textures/T_perlinnoise"),  # MM_WobblyVignette's noise (grey, linear)
    # MM_Telepathy's noises: smoky R, streaky G, blotchy B (linear); cloudy in each channel (sRGB)
    (2, "ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16"),
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise"),
    # the telekinesis's force field (P_ky_forceField_Telekinesis): the aura's mask (linear), the ground ring's sheets
    # (M_ky_shockWave02_4x4's own and the circle MI_ky_shockWave02_4x4_nonD swaps in, both linear) and its panned mask
    # (sRGB), the dust's star (sRGB) and the sphere's wall sheet (sRGB)
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_maskRGB5"),
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_shockWave02_4x4"),
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_circle01_4x4"),
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_maskRGB3"),
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_dust_longStar"),
    (2, "ThirdParty/AdvancedMagicFX09/Textures/T_ky_wall02_4x4"),
)
# Static meshes the mesh emitters draw (dd_assets.static_mesh), made before the systems.
MESHES = (
    (2, "ThirdParty/AdvancedMagicFX09/Meshes/SM_ky_sphere"),             # the force field's sphere (radius 10 cm)
    (2, "ThirdParty/AdvancedMagicFX09/Meshes/SM_ky_windLine27midPoly"),  # the force field's swirling aura
)
# Cascade systems (dd_particles), made after the materials they use.
PARTICLE_SYSTEMS = (
    (1, "ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2"),  # the teleport aim's slashes and sparks
    (2, "ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff"),  # Vanish's five puffs of smoke
    # the telekinesis's force field: a shrinking blue sphere, a swirling aura, a ground ring and star dust
    (2, "ThirdParty/AdvancedMagicFX09/Particles/P_ky_forceField_Telekinesis"),
)

SPEEDLINES = "UI/Main/Powers/M_Speedlines"
FLIPBOOK = "/Engine/Functions/Engine_MaterialFunctions02/Texturing/FlipBook"
# The FlipBook call's output M_Speedlines takes its UVs from (its expression input's OutputIndex).
FLIPBOOK_UV_OUTPUT = 2
CAMERA_SHAKE_MASTER = "/Game/Pipeline/Materials/M_DD_ChameleonCameraShake"

# The teleport aim's materials (pak_reference): the original's path, and the master holding our estimate of its graph.
KY_SLASH = "ThirdParty/AdvancedMagicFX13/Materials/M_ky_slash01_4x4"
KY_SLASH_MASTER = "/Game/Pipeline/Materials/M_DD_KySlash"
KY_SLASH_TEXTURE = "ThirdParty/AdvancedMagicFX13/Textures/T_ky_slash01_4x4"
RADIAL_GRADIENT = "PyroParticlePack/Materials/PPP_Radial_Gradient_Doffed"
RADIAL_GRADIENT_MASTER = "/Game/Pipeline/Materials/M_DD_PPPRadialGradient"
DECAL_TELEPORT = "Blueprints/Main/Powers/M_Decal_Teleport"
DECAL_TELEPORT_MASTER = "/Game/Pipeline/Materials/M_DD_DecalTeleport"
# The estimate of M_Decal_Teleport. The classic game shows (2026-09-17, 04 record) a disc with a sharp edge that
# glows additively and pulses once a second. Its size is RadialGradientExponential at its defaults cut by CheapContrast,
# whose contrast sets the edge's width (about a tenth of the radius). The classic's Manor grades its colours too
# heavily to read them back, so the colour and the brightness come from the latest version's hospital (step 11b4,
# observations/README.md): through the engine's filmic tonemapper back to linear light, the pulse adds pure red
# (green and blue under 0.3 % of it) and swings between about 0.2 and 1.0 of it.
# TODO(仮): the pulse's two ends are rounded from that fit (0.17 – 0.22 and 0.94 – 1.05, with the floor under the
# decal unknown).
DECAL_CONTRAST = 5.0
DECAL_COLOR = (1.0, 0.0, 0.0, 1.0)
DECAL_PULSE_LOW = 0.2
DECAL_PULSE_HIGH = 1.0

# Primal Fear's sphere (pak_reference_2): the original's path, and the master holding our estimate of its graph.
PRIMAL = "Materials/05_Circus/M_05_Primal"
PRIMAL_MASTER = "/Game/Pipeline/Materials/M_DD_Primal"
PRIMAL_TEXTURE = "Textures/05_Circus/T_05_PortalMaps"
# The export keeps a Panner (Panner_1) as the sample's coordinates, but not its speed.
# TODO(仮): the speed and the knobs below are our estimate, matched to the latest version's recording of the sphere
# (observations/README.md, step 11b2). The knobs are the master's parameters (the original's instance sets none of them).
PRIMAL_PAN_SPEED = (0.1, 0.1)
PRIMAL_KNOBS = {
    "Tiling": 2.0,  # the mesh's UVs × this, into the Panner
    "SparkleGain": 10.0,  # R × this, saturated: the flakes
    "SparkleBrightness": 3.0,
    "CloudDark": 0.01,  # the emissive where B is 0 and where it is 1
    "CloudBright": 0.1,
    "CloudOpacity": 0.95,  # the opacity away from the flakes and the edge
    "EdgeDistance": 50.0,  # DepthFade's distance (cm): the glow where the sphere cuts the level
    "EdgeBrightness": 6.0,
}

# Vanish (pak_reference_2): the puff's material and the widget's, with the masters holding our estimates of their graphs.
LOOPING_SMOKE = "Particles/Shared/SmokeTest/M_LoopingSmoke1_Sheet"
LOOPING_SMOKE_MASTER = "/Game/Pipeline/Materials/M_DD_LoopingSmoke"
LOOPING_SMOKE_TEXTURE = "Particles/Shared/SmokeTest/T_LoopingSmoke_8x8"
WOBBLY_VIGNETTE = "Materials/Special/MM_WobblyVignette"
WOBBLY_VIGNETTE_MASTER = "/Game/Pipeline/Materials/M_DD_WobblyVignette"
VIGNETTE_TEXTURE = "UI/Menu/Streaks/T_VignetteNew"
PERLIN_TEXTURE = "Textures/FX_Textures/T_perlinnoise"
# The export keeps two Panners (Panner_2, Panner_3) and a LinearSine without their values. These are measured from the
# latest version's recording (observations/README.md, step 11b3): the two noises' speeds (UV a second, the widget's own
# UVs), the sine's period (each noise's blobs swell every half period, and the two take turns), and the gain.
WOBBLE_PAN_A = (0.16, 0.006)
WOBBLE_PAN_B = (0.095, -0.011)
WOBBLE_PERIOD = 10.4
WOBBLE_GAIN = 0.67
# The puff's CameraDepthFade: the export keeps the call without its inputs. The engine's defaults (512, 24) leave the
# puff, 92 cm ahead, at an eighth of its opacity, where the recording shows a haze as thick as the particles' alpha
# (observations/README.md, step 11b3), so the fade is whole by about 2.3 m there; any shorter fade looks the same.
# TODO(仮): the fade's values are only bounded by the recording (要確認「Vanish の煙の位置と明るさ」).
SMOKE_FADE_LENGTH = 64.0
SMOKE_FADE_OFFSET = 0.0

# The telepathy's marker (pak_reference_2): the original's master and its instance, and the master holding our estimate.
TELEPATHY = "Blueprints/Main/Powers/Telepathy/MM_Telepathy"
TELEPATHY_INST = "Blueprints/Main/Powers/Telepathy/MM_Telepathy_Inst"
TELEPATHY_MASTER = "/Game/Pipeline/Materials/M_DD_Telepathy"
TELEPATHY_NOISE_A = "ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16"
TELEPATHY_NOISE_B = "ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise"
# The export keeps the samples' Panners (Panner_0, Panner_1) without their speeds, and nothing of how the noises and
# the radial gradient make the opacity. Step 11b5 fitted the gain and the texture coordinate's tiling to the latest
# version's recorded markers (observations/README.md, "Telepathy の印の見直し"): the UI's additive blend adds
# sRGB(opacity) to the screen, and the recorded markers add a soft cloud of opacity about 0.02 (at most about 0.07)
# whose detail is coarser than the noises at a tiling of 1.
# TODO(仮): the speeds only fit how fast the recorded cloud changes (their direction is not known); the gain (0.38 to
# 0.42 fit) and the tiling (0.5 to 0.7) are fitted, not the original's values.
TELEPATHY_PAN_A = (0.05, -0.1)
TELEPATHY_PAN_B = (-0.04, -0.15)
TELEPATHY_GAIN = 0.4
TELEPATHY_UV_TILING = 0.6
# MM_Telepathy_Inst's values its parent has (its Size is not one of MM_Telepathy's parameters, and
# RefractionDepthBias is the engine's, which a UI material does not use).
TELEPATHY_INST_SCALARS = ("Speed",)

# The telekinesis's force field (AdvancedMagicFX09, pak_reference_2): its four materials' graphs are cooked away, so
# masters holding our estimates sit under /Game/Pipeline (dd_assets.estimated_materials).
KY09 = "ThirdParty/AdvancedMagicFX09/"
# The exports keep no Panner's speed, no TextureCoordinate's tiling and nothing of how the aura's samples bend each
# other's coordinates: placeholders until the force field is compared with the latest version. Each of the aura's four
# layers is (its TexCoord tiling (U across a wind line, V along it), its pan speed, the pan speed of the sample that
# bends its coordinates, how far that sample's B bends them).
# TODO(仮): the force field's four estimated graphs and these values are placeholders (their graphs are cooked away);
# compare with the latest version (progress record step 11; 要確認「テレキネシスの力場の材質の推定」).
AURA_LAYERS = (
    ((1.0, 1.0), (0.0, -1.5), (0.1, -0.5), 0.1),
    ((1.0, 2.0), (0.0, -2.2), (-0.1, -0.7), 0.1),
    ((1.0, 1.0), (0.0, -1.2), (0.15, -0.4), 0.15),
    ((1.0, 3.0), (0.0, -2.8), (-0.15, -0.9), 0.15),
)
SHOCKWAVE_PANS = ((0.05, 0.1), (-0.08, 0.06))  # the ground ring's two T_ky_maskRGB3 samples (Panner_2, Panner_3)


def _build_speedlines(mat):
    """M_Speedlines (pak_reference_2's export keeps its graph): a FlipBook call with every input at its default (2 × 2
    frames, phase from Time, TexCoord 0), its output 2 as a TextureSample of T_Speedlines' UVs, and that sample's RGB as
    the emissive colour. The opacity is not connected (1)."""
    call = MEL.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, -700, 0)
    call.set_editor_property("material_function", unreal.load_asset(FLIPBOOK))
    outputs = MEL.get_material_expression_output_names(call)
    if len(outputs) <= FLIPBOOK_UV_OUTPUT:
        raise RuntimeError("FlipBook has only the outputs %s" % (outputs,))
    sample = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -300, 0)
    sample.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path("UI/Main/Powers/T_Speedlines")))
    dd_assets.connect(call, str(outputs[FLIPBOOK_UV_OUTPUT]), sample, "UVs")
    MEL.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.log("M_Speedlines: FlipBook output %d is %s" % (FLIPBOOK_UV_OUTPUT, outputs[FLIPBOOK_UV_OUTPUT]))


def _build_camera_shake(mat):
    """The Chameleon pack's M_CameraShake. Its export keeps only the parameters ShakePower (0.01) and ShakeFQ (50) and
    one MakeFloat2; the rest is estimated: the scene is read from a viewport UV moved ShakePower along a circle that
    turns ShakeFQ times a second (Sine / Cosine with their period of 1)."""
    g = dd_stage._Graph(mat)
    power = g.scalar("ShakePower", 0.01, -1000, 200)
    frequency = g.scalar("ShakeFQ", 50.0, -1200, 0)
    time = g.node(unreal.MaterialExpressionTime, -1200, -150)
    phase = g.multiply(time, "", frequency, "", -1000, -50)
    sine = g.node(unreal.MaterialExpressionSine, -850, -100)
    dd_assets.connect(phase, "", sine, "")
    cosine = g.node(unreal.MaterialExpressionCosine, -850, 0)
    dd_assets.connect(phase, "", cosine, "")
    circle = g.node(unreal.MaterialExpressionAppendVector, -700, -50)
    dd_assets.connect(sine, "", circle, "A")
    dd_assets.connect(cosine, "", circle, "B")
    offset = g.multiply(circle, "", power, "", -550, 0)
    screen = g.node(unreal.MaterialExpressionScreenPosition, -700, -250)
    uv = g.binary(unreal.MaterialExpressionAdd, screen, "ViewportUV", offset, "", -400, -150)
    scene = g.node(unreal.MaterialExpressionSceneTexture, -250, -150)
    scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    dd_assets.connect(uv, "", scene, "UVs")
    g.out(scene, "Color", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _build_ky_slash(mat):
    """M_ky_slash01_4x4 (AdvancedMagicFX13), estimated. The cook kept its settings (translucent, unlit, two-sided, for
    sprites and mesh particles), a ParticleSubUV of T_ky_slash01_4x4 and the parameters hilightColor, alphaDensity,
    colorCorrect and depthFade; its emissive colour comes from a Lerp, and it had six more expressions. The texture
    packs the slash in R and its bright edge in G (B holds a cross the game never shows). The estimate spends those six
    on the particle's colour, the Lerp, a Power, two Multiplies and a DepthFade:
      emissive  the particle's colour lerped to hilightColor by G
      opacity   R^colorCorrect × alphaDensity × the particle's alpha, faded into the depth over depthFade
    The emissive is the particle's whole colour (13, 0, 0.22), which the tonemapper shows as salmon, and the power of R
    shapes only the opacity: the latest version's hospital shows a thick salmon ring with a white core where the
    slashes overlap (step 11b4, observations/README.md), which an emissive of the colour × R^colorCorrect under an
    opacity of saturate(R × alphaDensity) left thin and dark. (The engine saturates a translucent opacity itself.)"""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("used_with_particle_sprites", True)
    mat.set_editor_property("used_with_mesh_particles", True)
    g = dd_stage._Graph(mat)
    tex = g.node(unreal.MaterialExpressionTextureSampleParameterSubUV, -1300, -100)
    tex.set_editor_property("parameter_name", "Texture")
    tex.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(KY_SLASH_TEXTURE)))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1300, 250)
    hilight = g.vector("hilightColor", (3.9051918983459473, 4.095554828643799, 5.0, 1.0), -900, -300)
    g.out(g.lerp(particle, "RGB", hilight, "RGB", tex, "G", -450, -100), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    shape = g.power(tex, "R", g.scalar("colorCorrect", 2.0, -1100, 50), "", -900, 50)
    density = g.multiply(shape, "", g.scalar("alphaDensity", 1.5, -1100, 150), "", -750, 100)
    fade = g.node(unreal.MaterialExpressionDepthFade, -400, 200)
    dd_assets.connect(g.multiply(density, "", particle, "A", -600, 200), "", fade, "Opacity")
    dd_assets.connect(g.scalar("depthFade", 100.0, -600, 350), "", fade, "FadeDistance")
    g.out(fade, "", unreal.MaterialProperty.MP_OPACITY)


def _build_radial_gradient(mat):
    """PPP_Radial_Gradient_Doffed (PyroParticlePack), estimated. The cook kept its settings (translucent, unlit,
    responsive AA, no separate translucency; for sprites, beam trails and static lighting), its emissive colour (the
    particle colour's RGB) and the functions RadialGradient and CameraDepthFade. The estimate: an opacity of
    RadialGradient × CameraDepthFade (both at their defaults) × the particle's alpha. UE 5 has no separate-translucency
    switch; UE 4's off is UE 5's translucency pass before depth of field."""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("enable_responsive_aa", True)
    mat.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    mat.set_editor_property("used_with_particle_sprites", True)
    mat.set_editor_property("used_with_beam_trails", True)
    mat.set_editor_property("used_with_static_lighting", True)
    g = dd_stage._Graph(mat)
    particle = g.node(unreal.MaterialExpressionParticleColor, -900, -100)
    g.out(particle, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    gradient = dd_assets.function_call(g, "Gradient/RadialGradient", -900, 150)
    fade = dd_assets.function_call(g, "Opacity/CameraDepthFade", -900, 300)
    shape = g.multiply(gradient, "RadialGradient", fade, "Result", -650, 200)
    g.out(g.multiply(shape, "", particle, "A", -450, 150), "", unreal.MaterialProperty.MP_OPACITY)


def _build_decal_teleport(mat):
    """M_Decal_Teleport, estimated (see DECAL_*). The cook kept its settings (a deferred decal, translucent, the
    emissive decal blend) and the functions RadialGradientExponential, CheapContrast and LinearGradient; its emissive
    colour comes from a Multiply. The estimate: Color × Intensity × saturate(CheapContrast(RadialGradientExponential,
    Contrast)) × Lerp(PulseLow, PulseHigh, sin(2π time) / 2 + 1/2). Only the emissive colour is connected, which UE 5.8
    draws as an additive emissive decal (UE 4's DBM_Emissive; UE 5 dropped the property). LinearGradient is left out:
    the classic game's disc shows no gradient along either axis."""
    g = dd_stage._Graph(mat)
    gradient = dd_assets.function_call(g, "Gradient/RadialGradientExponential", -1300, 0)
    contrast = dd_assets.function_call(g, "ImageAdjustment/CheapContrast", -1050, 0)
    dd_assets.connect(gradient, "RadialGradientExponential", contrast, "In")
    dd_assets.connect(g.scalar("Contrast", DECAL_CONTRAST, -1300, 200), "", contrast, "Contrast")
    disc = g.node(unreal.MaterialExpressionSaturate, -850, 0)
    dd_assets.connect(contrast, "Result", disc, "")
    time = g.node(unreal.MaterialExpressionTime, -1300, 350)
    sine = g.node(unreal.MaterialExpressionSine, -1150, 350)
    dd_assets.connect(time, "", sine, "")
    half = dd_assets.constant(g, 0.5, -1150, 450)
    wave = g.binary(unreal.MaterialExpressionAdd, g.multiply(sine, "", half, "", -1000, 350), "", half, "", -850, 350)
    low = g.scalar("PulseLow", DECAL_PULSE_LOW, -850, 200)
    high = g.scalar("PulseHigh", DECAL_PULSE_HIGH, -850, 280)
    pulse = g.lerp(low, "", high, "", wave, "", -650, 250)
    colour = g.multiply(g.vector("Color", DECAL_COLOR, -850, -250), "RGB",
                        g.scalar("Intensity", 1.0, -850, -100), "", -650, -150)
    glow = g.multiply(colour, "", disc, "", -450, -50)
    g.out(g.multiply(glow, "", pulse, "", -250, 50), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _build_primal(mat):
    """M_05_Primal (pak_reference_2), estimated. The cook kept its settings (translucent, two-sided, used with static
    lighting; the default lit shading, as the cook writes any other), the parameters Color, Opacity and Desaturation, one
    sample of T_05_PortalMaps at a Panner's coordinates, and an Add as the emissive colour's last node (a cook keeps no
    material's opacity input, so whether it was connected is unknown), of 43 expressions.

    The estimate follows the recording (observations/README.md, step 11b2): seen from its centre, the sphere is a dark
    red veil clouded by B, strewn with bright flakes the size of R's sparkles at the mesh's own UVs (wide, blocky, as
    few texels magnified), and it glows where it cuts the level. So, sampling at the mesh's UVs × Tiling through the
    Panner: flakes = saturate(R × SparkleGain), clouds = Lerp(CloudDark, CloudBright, B), edge = 1 −
    DepthFade(EdgeDistance); the emissive colour is Add(Desaturation(Color × clouds, Desaturation), Color × (flakes ×
    SparkleBrightness + edge × EdgeBrightness)), and the opacity saturate(CloudOpacity + flakes + edge) × Opacity. The
    knobs' defaults are PRIMAL_KNOBS."""
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("used_with_static_lighting", True)
    g = dd_stage._Graph(mat, checked=True)
    knob = {name: g.scalar(name, value, -1500, -600 + 100 * i) for i, (name, value) in enumerate(PRIMAL_KNOBS.items())}
    pan = g.node(unreal.MaterialExpressionPanner, -1300, 0)
    pan.set_editor_property("speed_x", PRIMAL_PAN_SPEED[0])
    pan.set_editor_property("speed_y", PRIMAL_PAN_SPEED[1])
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1650, 100)
    dd_assets.connect(g.multiply(coords, "", knob["Tiling"], "", -1450, 100), "", pan, "Coordinate")
    tex = g.node(unreal.MaterialExpressionTextureSample, -1100, 0)
    tex.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(PRIMAL_TEXTURE)))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    dd_assets.connect(pan, "", tex, "UVs")
    colour = g.vector("Color", (1.0, 0.0, 0.0, 1.0), -1100, -300)

    flakes = g.node(unreal.MaterialExpressionSaturate, -700, 0)
    dd_assets.connect(g.multiply(tex, "R", knob["SparkleGain"], "", -850, 0), "", flakes, "")
    fade = g.node(unreal.MaterialExpressionDepthFade, -850, 300)  # its Opacity pin left at 1
    dd_assets.connect(knob["EdgeDistance"], "", fade, "FadeDistance")
    edge = g.node(unreal.MaterialExpressionOneMinus, -700, 300)
    dd_assets.connect(fade, "", edge, "")

    clouds = g.lerp(knob["CloudDark"], "", knob["CloudBright"], "", tex, "B", -850, -200)
    washed = g.node(unreal.MaterialExpressionDesaturation, -500, -250)
    dd_assets.connect(g.multiply(colour, "RGB", clouds, "", -650, -250), "", washed, "")
    dd_assets.connect(g.scalar("Desaturation", 0.0, -650, -150), "", washed, "Fraction")
    lit = g.binary(unreal.MaterialExpressionAdd,
                   g.multiply(flakes, "", knob["SparkleBrightness"], "", -550, 0), "",
                   g.multiply(edge, "", knob["EdgeBrightness"], "", -550, 300), "", -400, 100)
    glow = g.binary(unreal.MaterialExpressionAdd, washed, "", g.multiply(colour, "RGB", lit, "", -300, 0), "", -150, -100)
    g.out(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    cover = g.binary(unreal.MaterialExpressionAdd, knob["CloudOpacity"], "",
                     g.binary(unreal.MaterialExpressionAdd, flakes, "", edge, "", -550, 450), "", -400, 450)
    clamped = g.node(unreal.MaterialExpressionSaturate, -250, 450)
    dd_assets.connect(cover, "", clamped, "")
    opacity = g.multiply(clamped, "", g.scalar("Opacity", 1.0, -250, 600), "", -100, 500)
    g.out(opacity, "", unreal.MaterialProperty.MP_OPACITY)


def _build_looping_smoke(mat):
    """M_LoopingSmoke1_Sheet (pak_reference_2), estimated. The cook kept its settings (translucent, no separate
    translucency, for sprites; the default lit shading, as the cook writes any other), a ParticleSubUV of
    T_LoopingSmoke_8x8 and a CameraDepthFade call, of ten expressions; it kept no emissive colour, which it keeps where
    one is connected (296 of the export's 408 materials keep one; none keeps a base colour or an opacity). The estimate: a
    base colour of the frame's RGB × the particle's colour (UE saturates a base colour, so the puff's colours over 1 come
    out near white) and an opacity of the frame's alpha × the particle's alpha × the depth fade (its Fade Length and Fade
    Offset are the parameters FadeLength and FadeOffset, SMOKE_FADE_*)."""
    mat.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    mat.set_editor_property("used_with_particle_sprites", True)
    g = dd_stage._Graph(mat)
    frame = g.node(unreal.MaterialExpressionParticleSubUV, -900, 0)
    frame.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(LOOPING_SMOKE_TEXTURE)))
    particle = g.node(unreal.MaterialExpressionParticleColor, -900, 300)
    g.out(g.multiply(frame, "RGB", particle, "RGB", -600, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    fade = dd_assets.function_call(g, "Opacity/CameraDepthFade", -900, 500)
    dd_assets.connect(g.scalar("FadeLength", SMOKE_FADE_LENGTH, -1150, 500), "", fade, "Fade Length")
    dd_assets.connect(g.scalar("FadeOffset", SMOKE_FADE_OFFSET, -1150, 600), "", fade, "Fade Offset")
    alpha = g.multiply(frame, "A", particle, "A", -600, 250)
    g.out(g.multiply(alpha, "", fade, "Result", -400, 300), "", unreal.MaterialProperty.MP_OPACITY)


def _build_wobbly_vignette(mat):
    """MM_WobblyVignette (pak_reference_2), estimated (see WOBBLE_*). The cook kept its settings (the UI domain,
    translucent), its emissive colour (the RGB of T_VignetteNew at a TextureCoordinate: white), a second T_VignetteNew
    sample, two samples of T_perlinnoise at two Panners and a LinearSine call, of 22 expressions. The estimate, fitted to
    the latest version's recording (observations/README.md, step 11b3): an opacity of the second vignette's alpha ×
    Lerp(A × (1 − s), B × s, s) × a gain, where A and B are the two noises panning over the widget's UVs and s is
    LinearSine(Time, WobblePeriod). Each noise so fades with s squared: its blobs swell when s is at its end and all but
    go when s is halfway, which the recording shows twice a period, with the two noises' blobs taking turns. The widget's
    purple tints the white (the recording and PIE both blend toward sRGB (142, 110, 194))."""
    g = dd_stage._Graph(mat)
    vignette = unreal.load_asset(dd_assets.asset_path(VIGNETTE_TEXTURE))
    noise = unreal.load_asset(dd_assets.asset_path(PERLIN_TEXTURE))
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1500, -350)
    colour = g.node(unreal.MaterialExpressionTextureSample, -1250, -350)
    colour.set_editor_property("texture", vignette)
    dd_assets.connect(coords, "", colour, "UVs")
    g.out(colour, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    noises = []
    for speed, y in ((WOBBLE_PAN_A, 0), (WOBBLE_PAN_B, 250)):
        pan = g.node(unreal.MaterialExpressionPanner, -1500, y)
        pan.set_editor_property("speed_x", speed[0])
        pan.set_editor_property("speed_y", speed[1])
        sample = g.node(unreal.MaterialExpressionTextureSample, -1250, y)
        sample.set_editor_property("texture", noise)
        sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
        dd_assets.connect(pan, "", sample, "UVs")
        noises.append(sample)
    sine = dd_assets.function_call(g, "Utility/LinearSine", -1250, 500, dd_assets.FUNCTIONS_02)
    dd_assets.connect(g.node(unreal.MaterialExpressionTime, -1500, 500), "", sine, "Value")
    dd_assets.connect(g.scalar("WobblePeriod", WOBBLE_PERIOD, -1500, 600), "", sine, "Period")
    fade_a = g.node(unreal.MaterialExpressionOneMinus, -1100, 600)
    dd_assets.connect(sine, "Linear Sine", fade_a, "")
    part_a = g.multiply(noises[0], "R", fade_a, "", -1000, 50)
    part_b = g.multiply(noises[1], "R", sine, "Linear Sine", -1000, 300)
    wobble = g.lerp(part_a, "", part_b, "", sine, "Linear Sine", -850, 150)
    edge = g.node(unreal.MaterialExpressionTextureSample, -1250, -100)
    edge.set_editor_property("texture", vignette)
    shaped = g.multiply(edge, "A", wobble, "", -750, 0)
    gained = g.multiply(shaped, "", g.scalar("WobbleGain", WOBBLE_GAIN, -950, 350), "", -550, 50)
    clamped = g.node(unreal.MaterialExpressionSaturate, -400, 50)
    dd_assets.connect(gained, "", clamped, "")
    g.out(clamped, "", unreal.MaterialProperty.MP_OPACITY)


def _build_telepathy(mat):
    """MM_Telepathy (pak_reference_2), estimated (see TELEPATHY_*). The cook kept its settings (the UI domain,
    additive), its emissive colour (the Color parameter's RGB, red), the parameters Tiling and Speed, a sample of
    T_ky_noise16 (linear) at Panner_0 and one of T_ky_noise at Panner_1, and a RadialGradientExponential call, of 21
    expressions (the ExponentialDensity it lists is the gradient's own). The estimate: both noises read at TexCoord 0
    (tiled TELEPATHY_UV_TILING) × Tiling, panning by Time × Speed, and an opacity of saturate((the noises' R summed) ×
    the gradient × a gain). UI additive blending adds the colour × the opacity (× the widget's opacity)."""
    g = dd_stage._Graph(mat)
    g.out(g.vector("Color", (1.0, 0.0, 0.0, 1.0), -600, -350), "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1700, 0)
    coords.set_editor_property("u_tiling", TELEPATHY_UV_TILING)
    coords.set_editor_property("v_tiling", TELEPATHY_UV_TILING)
    tiled = g.multiply(coords, "", g.scalar("Tiling", 1.0, -1700, 100), "", -1500, 50)
    time = g.node(unreal.MaterialExpressionTime, -1700, 250)
    flow = g.multiply(time, "", g.scalar("Speed", 1.0, -1700, 350), "", -1500, 300)
    noises = []
    for rel, sampler, speed, y in ((TELEPATHY_NOISE_A, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, TELEPATHY_PAN_A, -100),
                                   (TELEPATHY_NOISE_B, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, TELEPATHY_PAN_B, 200)):
        pan = g.node(unreal.MaterialExpressionPanner, -1300, y)
        pan.set_editor_property("speed_x", speed[0])
        pan.set_editor_property("speed_y", speed[1])
        dd_assets.connect(tiled, "", pan, "Coordinate")
        dd_assets.connect(flow, "", pan, "Time")
        sample = g.node(unreal.MaterialExpressionTextureSample, -1100, y)
        sample.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(rel)))
        sample.set_editor_property("sampler_type", sampler)
        dd_assets.connect(pan, "", sample, "UVs")
        noises.append(sample)
    smoke = g.binary(unreal.MaterialExpressionAdd, noises[0], "R", noises[1], "R", -850, 50)
    gradient = dd_assets.function_call(g, "Gradient/RadialGradientExponential", -1100, 450)
    shaped = g.multiply(smoke, "", gradient, "RadialGradientExponential", -650, 150)
    gained = g.multiply(shaped, "", g.scalar("Gain", TELEPATHY_GAIN, -850, 300), "", -450, 200)
    clamped = g.node(unreal.MaterialExpressionSaturate, -300, 200)
    dd_assets.connect(gained, "", clamped, "")
    g.out(clamped, "", unreal.MaterialProperty.MP_OPACITY)


def make_telepathy_materials():
    """The telepathy's marker: the estimated master, MM_Telepathy as an instance of it with the original's parameter
    values, and MM_Telepathy_Inst as an instance of MM_Telepathy with its own."""
    master = dd_assets.material(TELEPATHY_MASTER, _build_telepathy, domain=unreal.MaterialDomain.MD_UI,
                                blend_mode=unreal.BlendMode.BLEND_ADDITIVE)
    scalars, vectors = dd_assets.parameter_defaults(TELEPATHY, 2)
    base = dd_assets.material_instance(dd_assets.asset_path(TELEPATHY), master, scalars=scalars, vectors=vectors)
    inst = dd_assets.main_export(dd_assets.export_json(TELEPATHY_INST, 2), TELEPATHY_INST)["props"]
    inst_scalars = {v["ParameterInfo"]["Name"]: v["ParameterValue"] for v in inst.get("ScalarParameterValues", [])
                    if v["ParameterInfo"]["Name"] in TELEPATHY_INST_SCALARS}
    marker = dd_assets.material_instance(dd_assets.asset_path(TELEPATHY_INST), base, scalars=inst_scalars)
    made = [master, base, marker]
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def _ky09_texture(name):
    return unreal.load_asset(dd_assets.asset_path(KY09 + "Textures/" + name))


def _sample(g, tex, sampler, uvs, x, y):
    """A TextureSample of tex at uvs (None: TexCoord 0)."""
    e = g.node(unreal.MaterialExpressionTextureSample, x, y)
    e.set_editor_property("texture", tex)
    e.set_editor_property("sampler_type", sampler)
    if uvs is not None:
        dd_assets.connect(uvs, "", e, "UVs")
    return e


def _panner(g, coordinate, speed, x, y):
    """A Panner of coordinate (None: TexCoord 0) moving (u, v) per second."""
    e = g.node(unreal.MaterialExpressionPanner, x, y)
    e.set_editor_property("speed_x", speed[0])
    e.set_editor_property("speed_y", speed[1])
    if coordinate is not None:
        dd_assets.connect(coordinate, "", e, "Coordinate")
    return e


def _build_wall02(mat, d):
    """M_ky_wall02_4x4_two (the force field's sphere), estimated. The cook kept its settings (translucent, unlit,
    two-sided, for sprites and mesh particles), the parameters opacity and baseColor, a SubUV sample of baseTex
    (T_ky_wall02_4x4: grey wisps) and a LinearInterpolate as the emissive colour, of ten expressions. The estimate: the
    emissive colour runs from baseColor in the gaps to the particle's colour on the wisps (a lerp by R), over an opacity
    of saturate(R + opacity) × the particle's alpha: a faint dark-blue veil with bright wisps."""
    dd_assets.particle_material(mat, two_sided=True)
    g = dd_stage._Graph(mat, checked=True)
    tex = g.node(unreal.MaterialExpressionTextureSampleParameterSubUV, -1000, 0)
    tex.set_editor_property("parameter_name", "baseTex")
    tex.set_editor_property("texture", _ky09_texture("T_ky_wall02_4x4"))
    particle = g.node(unreal.MaterialExpressionParticleColor, -1000, 300)
    colour = g.lerp(g.vector("baseColor", d["baseColor"], -1000, -250), "RGB", particle, "RGB", tex, "R", -600, -100)
    g.out(colour, "", MP.MP_EMISSIVE_COLOR)
    veil = dd_assets.add(g, tex, "R", g.scalar("opacity", d["opacity"], -1000, 200), "", -750, 100)
    clamped = dd_assets.single(g, unreal.MaterialExpressionSaturate, veil, "", -600, 100)
    g.out(g.multiply(clamped, "", particle, "A", -400, 150), "", MP.MP_OPACITY)


def _build_aura7(mat, d):
    """M_ky_aura7 (the force field's swirling aura: MI_ky_aura7c on SM_ky_windLine27midPoly), estimated (see
    AURA_LAYERS). The cook kept its settings (translucent, unlit, two-sided, for sprites, beam trails and mesh
    particles), a Multiply as the emissive colour, the parameters baseDensity, baseOpacity, hilightPower,
    hilightDensity, depthFade, maskU, maskV and maskRadiusControl, a DynamicParameter (maskOffsetY at 0, Param2 – 4 at
    1), a RadialGradientExponential call and eight samples of T_ky_maskRGB5 (linear; R wisps, G specks, B streaks),
    four at Adds and four at Panners, of 61 expressions. The mesh's lines are strips with U across them (0, 0.5, 1) and
    V along them. The estimate reads the samples as four layers, each a sample at a panned TexCoord bent by the B of a
    sample at another Panner:
      emissive  the particle's colour × (base × baseDensity + hilight), where base is the mean of layers 1 and 2's R
                and hilight = (the mean of layers 3 and 4's R)^hilightPower × hilightDensity (where both are bright)
      opacity   saturate(base × baseOpacity + hilight) × the mask × the particle's alpha, faded over depthFade. The mask
                is RadialGradientExponential at TexCoord × (maskU, maskV), centred on maskRadiusControl's RG with
                maskOffsetY added to G, of radius B and density A: a strip's middle line, in a window along it that the
                particle system moves (maskOffsetY 0.1 → 0.2 wipes the lines from their V = 0 ends)
    Param2 – 4 are not used (the particle system leaves them at 1)."""
    dd_assets.particle_material(mat, beam_trails=True, two_sided=True)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -400, 700)
    wisps = _ky09_texture("T_ky_maskRGB5")
    linear = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
    layers = []
    for i, (tiling, speed, bend_speed, bend) in enumerate(AURA_LAYERS):
        y = -700 + i * 350
        coords = g.node(unreal.MaterialExpressionTextureCoordinate, -2700, y)
        coords.set_editor_property("u_tiling", tiling[0])
        coords.set_editor_property("v_tiling", tiling[1])
        bender = _sample(g, wisps, linear, _panner(g, coords, bend_speed, -2500, y + 150), -2300, y + 150)
        bent = g.multiply(bender, "B", dd_assets.constant(g, bend, -2100, y + 250), "", -1950, y + 150)
        uvs = dd_assets.add(g, _panner(g, coords, speed, -2500, y), "", bent, "", -1800, y)
        layers.append(_sample(g, wisps, linear, uvs, -1600, y))
    half = dd_assets.constant(g, 0.5, -1400, -200)
    base = g.multiply(dd_assets.add(g, layers[0], "R", layers[1], "R", -1400, -600), "", half, "", -1200, -550)
    glint = g.multiply(dd_assets.add(g, layers[2], "R", layers[3], "R", -1400, 100), "", half, "", -1200, 150)
    sharp = g.power(glint, "", g.scalar("hilightPower", d["hilightPower"], -1200, 250), "", -1000, 150)
    hilight = g.multiply(sharp, "", g.scalar("hilightDensity", d["hilightDensity"], -1000, 250), "", -800, 200)
    dim = g.multiply(base, "", g.scalar("baseDensity", d["baseDensity"], -1200, -450), "", -1000, -550)
    glow = dd_assets.add(g, dim, "", hilight, "", -650, -300)
    g.out(g.multiply(glow, "", particle, "RGB", -400, -250), "", MP.MP_EMISSIVE_COLOR)

    # The mask along the lines.
    control = g.vector("maskRadiusControl", d["maskRadiusControl"], -1400, 1000)
    dynamic = dd_assets.dynamic_parameter(g, ("maskOffsetY", "Param2", "Param3", "Param4"), -1400, 1250,
                                          defaults=(0.0, 1.0, 1.0, 1.0))
    scale = g.binary(unreal.MaterialExpressionAppendVector, g.scalar("maskU", d["maskU"], -1400, 700), "",
                     g.scalar("maskV", d["maskV"], -1400, 800), "", -1200, 750)
    mask_uvs = g.multiply(g.node(unreal.MaterialExpressionTextureCoordinate, -1200, 600), "", scale, "", -1000, 650)
    centre_v = dd_assets.add(g, control, "G", dynamic, "maskOffsetY", -1150, 1150)
    centre = g.binary(unreal.MaterialExpressionAppendVector, control, "R", centre_v, "", -1000, 1000)
    mask = dd_assets.radial_gradient(g, None, None, -800, 850, uvs=mask_uvs, centre=centre)
    dd_assets.connect(control, "B", mask, "Radius")
    dd_assets.connect(control, "A", mask, "Density")
    veil = g.multiply(base, "", g.scalar("baseOpacity", d["baseOpacity"], -1000, -150), "", -800, -150)
    cover = dd_assets.single(g, unreal.MaterialExpressionSaturate,
                             dd_assets.add(g, veil, "", hilight, "", -650, 0), "", -500, 0)
    masked = g.multiply(cover, "", mask, "RadialGradientExponential", -350, 300)
    faded = g.multiply(masked, "", particle, "A", -200, 450)
    dd_assets.depth_faded_opacity(g, faded, g.scalar("depthFade", d["depthFade"], -200, 600), 0, 500)


def _build_shockwave02(mat, d):
    """M_ky_shockWave02_4x4 (the force field's ground ring: MI_ky_shockWave02_4x4_nonD), estimated (see
    SHOCKWAVE_PANS). The cook kept its settings (translucent, unlit, two-sided, for sprites and mesh particles), an Add
    as the emissive colour, the parameters baseDensity, depthFade, coreDensity, coreHardness, hilightDetailPower,
    coreHilightPower and coreColor, a SubUV sample of baseTex (T_ky_shockWave02_4x4, linear; the instance swaps in
    T_ky_circle01_4x4, whose R is a ring with spiky edges) whose RGB goes through the static mask selectCh (R), and two
    samples of T_ky_maskRGB3 (sRGB) at Panners, of 32 expressions. The estimate, with shape the selected channel and
    noise the two samples' R (sparse bright scratches) summed:
      emissive  the particle's colour × base + coreColor × (core + detail), where base = shape × baseDensity, core =
                saturate(shape^coreHilightPower × coreDensity) (the ring's hottest line) and detail =
                saturate((shape × noise)^hilightDetailPower × coreHardness) (sparks along it)
      opacity   saturate(base + core + detail) × the particle's alpha, faded over depthFade (the instance's 0 fades
                nothing)"""
    dd_assets.particle_material(mat, two_sided=True)
    g = dd_stage._Graph(mat, checked=True)
    tex = g.node(unreal.MaterialExpressionTextureSampleParameterSubUV, -1700, -200)
    tex.set_editor_property("parameter_name", "baseTex")
    tex.set_editor_property("texture", _ky09_texture("T_ky_shockWave02_4x4"))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    shape = g.node(unreal.MaterialExpressionStaticComponentMaskParameter, -1450, -200)
    shape.set_editor_property("parameter_name", "selectCh")
    shape.set_editor_property("default_r", True)
    dd_assets.connect(tex, "RGB", shape, "")
    scratches = _ky09_texture("T_ky_maskRGB3")
    colour = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
    noises = [_sample(g, scratches, colour, _panner(g, None, speed, -1700, 200 + i * 250), -1500, 200 + i * 250)
              for i, speed in enumerate(SHOCKWAVE_PANS)]
    noise = dd_assets.add(g, noises[0], "R", noises[1], "R", -1250, 300)
    particle = g.node(unreal.MaterialExpressionParticleColor, -700, 500)

    base = g.multiply(shape, "", g.scalar("baseDensity", d["baseDensity"], -1250, -350), "", -1050, -350)
    hottest = g.power(shape, "", g.scalar("coreHilightPower", d["coreHilightPower"], -1250, -100), "", -1050, -150)
    dense = g.multiply(hottest, "", g.scalar("coreDensity", d["coreDensity"], -1050, -50), "", -850, -150)
    core = dd_assets.single(g, unreal.MaterialExpressionSaturate, dense, "", -700, -150)
    sparks = g.power(g.multiply(shape, "", noise, "", -1050, 150), "",
                     g.scalar("hilightDetailPower", d["hilightDetailPower"], -1050, 250), "", -850, 150)
    hard = g.multiply(sparks, "", g.scalar("coreHardness", d["coreHardness"], -850, 250), "", -700, 150)
    detail = dd_assets.single(g, unreal.MaterialExpressionSaturate, hard, "", -550, 150)
    hot = dd_assets.add(g, core, "", detail, "", -450, 0)
    tinted = g.multiply(g.vector("coreColor", d["coreColor"], -600, -300), "RGB", hot, "", -300, -100)
    coloured = g.multiply(particle, "RGB", base, "", -450, -350)
    g.out(dd_assets.add(g, coloured, "", tinted, "", -150, -250), "", MP.MP_EMISSIVE_COLOR)
    cover = dd_assets.single(g, unreal.MaterialExpressionSaturate, dd_assets.add(g, base, "", hot, "", -300, 150),
                             "", -150, 150)
    faded = g.multiply(cover, "", particle, "A", 0, 300)
    dd_assets.depth_faded_opacity(g, faded, g.scalar("depthFade", d["depthFade"], 0, 450), 200, 350)


def _build_star_dust(mat, d):
    """M_ky_starDust (the force field's star dust: MI_ky_starDust_sq), estimated. The cook kept its settings
    (translucent, unlit, responsive AA, for sprites and mesh particles), its emissive colour (the particle colour's
    RGB), a static switch useDistanceSize as the world position offset (B a constant), the parameters threshold,
    starPower (no default: 0), maskRadius, maskDensity and fadeValue, a DynamicParameter (flashTime, flashPower,
    starDensity; defaults 0), the functions DiamondGradient, RadialGradientExponential and Blend_Screen, two samples of
    T_ky_dust_longStar (sRGB; one at TexCoord 0, one at a Rotator) and the static switch swSQdust (on by default; A
    (on) a Multiply_28, B (off) a Multiply_9), of 40 expressions. T_ky_dust_longStar is 1 along its middle row and
    falls to 0.37 at its top and bottom, so a high power of it is a thin line. The instance (_sq) turns swSQdust off,
    and the latest version's recording (observations/README.md, step 11b4) shows its dust as small squares, flat
    inside with a soft edge about as wide, a tenth of the sprite across: so the off side, whose Multiply's number is
    older than the star's expressions, is the square. DiamondGradient is the product of two tents, (1 − |2u − 1|) ×
    (1 − |2v − 1|), to the power of its Falloff; near its centre that is a diamond, and a power of starDensity (35 –
    68 from the particle system) × flashPower (3 – 10) leaves ln(flashPower) / starDensity of it opaque and fades over
    about as much again. The estimate:
      off  DiamondGradient(Falloff = starDensity) × flashPower
      on   a four-pointed star: each sample's R to the power starDensity, the second at the TexCoord turned by a
           Rotator over Time × flashTime (the two lines turn against each other, and the star twinkles), screened
           together, × RadialGradientExponential(maskRadius, maskDensity) (the arms fade out) × flashPower
    The opacity is saturate(the switch) × the particle's alpha, faded over fadeValue. useDistanceSize's on side
    (threshold) and starPower are not made, and the world position offset is left unconnected."""
    dd_assets.particle_material(mat, responsive_aa=True)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -400, 400)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)
    dynamic = dd_assets.dynamic_parameter(g, ("flashTime", "flashPower", "starDensity", "Param4"), -1900, 250)
    star = _ky09_texture("T_ky_dust_longStar")
    colour = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
    along = _sample(g, star, colour, None, -1400, -250)
    turn = g.node(unreal.MaterialExpressionRotator, -1600, 50)
    dd_assets.connect(g.multiply(g.node(unreal.MaterialExpressionTime, -1900, 50), "", dynamic, "flashTime",
                                 -1750, 50), "", turn, "Time")
    across = _sample(g, star, colour, turn, -1400, 50)
    line_a = g.power(along, "R", dynamic, "starDensity", -1150, -200)
    line_b = g.power(across, "R", dynamic, "starDensity", -1150, 50)
    screen = dd_assets.function_call(g, "Blends/Blend_Screen", -950, -100, dd_assets.FUNCTIONS_03)
    dd_assets.connect(line_a, "", screen, "Base")
    dd_assets.connect(line_b, "", screen, "Blend")
    cross = dd_assets.channel(g, screen, "Result", "R", -750, -100)
    arms = dd_assets.radial_gradient(g, g.scalar("maskRadius", d["maskRadius"], -950, 150),
                                     g.scalar("maskDensity", d["maskDensity"], -950, 250), -750, 150)
    shaped = g.multiply(cross, "", arms, "RadialGradientExponential", -550, 0)
    glint = g.multiply(shaped, "", dynamic, "flashPower", -400, 50)
    diamond = dd_assets.function_call(g, "Gradient/DiamondGradient", -750, -350)
    dd_assets.connect(dynamic, "starDensity", diamond, "Falloff")
    square = g.multiply(diamond, "DiamondGradient", dynamic, "flashPower", -400, -300)
    dust = g.switch("swSQdust", glint, "", square, "", -250, -100)
    dust.set_editor_property("default_value", True)
    clamped = dd_assets.single(g, unreal.MaterialExpressionSaturate, dust, "", -100, -100)
    faded = g.multiply(clamped, "", particle, "A", 50, 0)
    dd_assets.depth_faded_opacity(g, faded, g.scalar("fadeValue", d["fadeValue"], 50, 150), 250, 50)


# (the original's material, the master holding our estimate, its builder, the original's instances of it)
TELEKINESIS_MATERIALS = (
    ("M_ky_wall02_4x4_two", "M_DD_KyWall02", _build_wall02, ()),
    ("M_ky_aura7", "M_DD_KyAura7", _build_aura7, ("MI_ky_aura7c",)),
    ("M_ky_shockWave02_4x4", "M_DD_KyShockWave02", _build_shockwave02, ("MI_ky_shockWave02_4x4_nonD",)),
    ("M_ky_starDust", "M_DD_KyStarDust", _build_star_dust, ("MI_ky_starDust_sq",)),
)


def make_telekinesis_materials():
    """The force field's materials (dd_assets.estimated_materials). Returns the package paths."""
    return [a.get_path_name() for a in dd_assets.estimated_materials(KY09 + "Materials/", TELEKINESIS_MATERIALS, 2)]


def slash_parameters():
    """M_ky_slash01_4x4's parameter defaults and texture, from its export: ({name: value}, {name: [r, g, b, a]}, the
    texture's rel)."""
    scalars, vectors = dd_assets.parameter_defaults(KY_SLASH, 1)
    texture = None
    for e in dd_assets.export_json(KY_SLASH, 1)["exports"]:
        if e["class"] == "MaterialExpressionParticleSubUV":
            texture = dd_assets.game_rel(e["props"]["Texture"])
    return scalars, vectors, texture


def make_teleport_materials():
    """The teleport aim's three materials: the estimated masters, and instances of them at the original's paths (the
    slash's with the original's parameter values)."""
    translucent = unreal.BlendMode.BLEND_TRANSLUCENT
    slash = dd_assets.material(KY_SLASH_MASTER, _build_ky_slash, blend_mode=translucent)
    gradient = dd_assets.material(RADIAL_GRADIENT_MASTER, _build_radial_gradient, blend_mode=translucent)
    decal = dd_assets.material(DECAL_TELEPORT_MASTER, _build_decal_teleport,
                               domain=unreal.MaterialDomain.MD_DEFERRED_DECAL, blend_mode=translucent)
    scalars, vectors, texture = slash_parameters()
    instances = [
        dd_assets.material_instance(dd_assets.asset_path(KY_SLASH), slash, scalars=scalars, vectors=vectors,
                                    textures={"Texture": dd_assets.asset_path(texture)}),
        dd_assets.material_instance(dd_assets.asset_path(RADIAL_GRADIENT), gradient),
        dd_assets.material_instance(dd_assets.asset_path(DECAL_TELEPORT), decal),
    ]
    made = [slash, gradient, decal] + instances
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def make_primal_material():
    """Primal Fear's sphere: the estimated master, and an instance of it at the original's path with the original's
    parameter values."""
    master = dd_assets.material(PRIMAL_MASTER, _build_primal, blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    scalars, vectors = dd_assets.parameter_defaults(PRIMAL, 2)
    instance = dd_assets.material_instance(dd_assets.asset_path(PRIMAL), master, scalars=scalars, vectors=vectors)
    for asset in (master, instance):
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [master.get_path_name(), instance.get_path_name()]


def make_vanish_materials():
    """Vanish's puff and vignette: the estimated masters, and instances of them at the original's paths (neither
    original has parameters)."""
    translucent = unreal.BlendMode.BLEND_TRANSLUCENT
    smoke = dd_assets.material(LOOPING_SMOKE_MASTER, _build_looping_smoke, blend_mode=translucent)
    vignette = dd_assets.material(WOBBLY_VIGNETTE_MASTER, _build_wobbly_vignette,
                                  domain=unreal.MaterialDomain.MD_UI, blend_mode=translucent)
    made = [smoke, vignette,
            dd_assets.material_instance(dd_assets.asset_path(LOOPING_SMOKE), smoke),
            dd_assets.material_instance(dd_assets.asset_path(WOBBLY_VIGNETTE), vignette)]
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def make_materials():
    speedlines = dd_assets.material(dd_assets.asset_path(SPEEDLINES), _build_speedlines,
                                    domain=unreal.MaterialDomain.MD_UI, blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    shake = dd_assets.material(CAMERA_SHAKE_MASTER, _build_camera_shake, domain=unreal.MaterialDomain.MD_POST_PROCESS)
    return ([speedlines.get_path_name(), shake.get_path_name()] + make_teleport_materials() + make_primal_material()
            + make_vanish_materials() + make_telepathy_materials() + make_telekinesis_materials())


def import_all():
    """Imports and builds every asset the powers use, then saves /Game/DD and /Game/Pipeline. Returns how many of each
    kind."""
    result = {"sounds": len([dd_assets.sound(rel, version) for version, rel in SOUNDS])}
    result["camera_shakes"] = len([dd_assets.camera_shake(rel, version) for version, rel in CAMERA_SHAKES])
    result["camera_anims"] = len([dd_assets.camera_anim(rel, version) for version, rel in CAMERA_ANIMS])
    result["textures"] = len([dd_assets.texture(rel, version) for version, rel in TEXTURES])
    result["meshes"] = len([dd_assets.static_mesh(rel, version) for version, rel in MESHES])
    result["materials"] = len(make_materials())
    result["particle_systems"] = len([dd_particles.particle_system(rel, version) for version, rel in PARTICLE_SYSTEMS])
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
