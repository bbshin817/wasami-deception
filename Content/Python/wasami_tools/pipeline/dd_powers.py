"""Dark Deception's tablet powers: the sounds, camera shakes, camera anims, textures, materials and particle systems
the power system (UWasamiPowerComponent), the teleport's aim (AWasamiTeleportAim), Primal Fear (AWasamiPrimalPower),
Vanish (AWasamiVanishPower, UWasamiVanishWidget) and the player's FX (UWasamiChameleonComponent) use. The icons on the
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

Sources: pak_reference_2 (UE 4.24, the latest version), which the powers follow except the teleport (pak_reference).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

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
)
# Cascade systems (dd_particles), made after the materials they use.
PARTICLE_SYSTEMS = (
    (1, "ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2"),  # the teleport aim's slashes and sparks
    (2, "ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff"),  # Vanish's five puffs of smoke
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
# whose contrast sets the edge's width (about a tenth of the radius). The colour and the brightness are placeholders
# until they are compared with the latest version's hospital (the classic's Manor grades its colours too heavily to
# read them back).
DECAL_CONTRAST = 5.0
DECAL_COLOR = (1.0, 0.105, 0.09, 1.0)
DECAL_PULSE_LOW = 0.04
DECAL_PULSE_HIGH = 0.6

# Primal Fear's sphere (pak_reference_2): the original's path, and the master holding our estimate of its graph.
PRIMAL = "Materials/05_Circus/M_05_Primal"
PRIMAL_MASTER = "/Game/Pipeline/Materials/M_DD_Primal"
PRIMAL_TEXTURE = "Textures/05_Circus/T_05_PortalMaps"
# The export keeps a Panner (Panner_1) as the sample's coordinates, but not its speed: a placeholder until the sphere is
# compared with the latest version's hospital.
PRIMAL_PAN_SPEED = (0.1, 0.1)

# Vanish (pak_reference_2): the puff's material and the widget's, with the masters holding our estimates of their graphs.
LOOPING_SMOKE = "Particles/Shared/SmokeTest/M_LoopingSmoke1_Sheet"
LOOPING_SMOKE_MASTER = "/Game/Pipeline/Materials/M_DD_LoopingSmoke"
LOOPING_SMOKE_TEXTURE = "Particles/Shared/SmokeTest/T_LoopingSmoke_8x8"
WOBBLY_VIGNETTE = "Materials/Special/MM_WobblyVignette"
WOBBLY_VIGNETTE_MASTER = "/Game/Pipeline/Materials/M_DD_WobblyVignette"
VIGNETTE_TEXTURE = "UI/Menu/Streaks/T_VignetteNew"
PERLIN_TEXTURE = "Textures/FX_Textures/T_perlinnoise"
# The export keeps two Panners (Panner_2, Panner_3) and a LinearSine without their values: placeholders until the
# vignette is compared with the latest version (the noises' speeds, the sine's period, and how strongly the noise
# scales the vignette's alpha; the two noises' product averages 0.47).
WOBBLE_PAN_A = (0.03, 0.02)
WOBBLE_PAN_B = (-0.02, 0.03)
WOBBLE_PERIOD = 2.0
WOBBLE_GAIN = 2.0

# The telepathy's marker (pak_reference_2): the original's master and its instance, and the master holding our estimate.
TELEPATHY = "Blueprints/Main/Powers/Telepathy/MM_Telepathy"
TELEPATHY_INST = "Blueprints/Main/Powers/Telepathy/MM_Telepathy_Inst"
TELEPATHY_MASTER = "/Game/Pipeline/Materials/M_DD_Telepathy"
TELEPATHY_NOISE_A = "ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16"
TELEPATHY_NOISE_B = "ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise"
# The export keeps the samples' Panners (Panner_0, Panner_1) without their speeds, and nothing of how the noises and
# the radial gradient make the opacity: placeholders until the marker is compared with the latest version (the speeds,
# and the gain that makes the dim noises show).
TELEPATHY_PAN_A = (0.05, -0.1)
TELEPATHY_PAN_B = (-0.04, -0.15)
TELEPATHY_GAIN = 3.0
# MM_Telepathy_Inst's values its parent has (its Size is not one of MM_Telepathy's parameters, and
# RefractionDepthBias is the engine's, which a UI material does not use).
TELEPATHY_INST_SCALARS = ("Speed",)


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
    colorCorrect and depthFade; its emissive colour comes from a Lerp. The texture packs the slash in R and its bright
    edge in G (B holds a cross the game never shows). The estimate: the particle's colour × R^colorCorrect, lerped to
    hilightColor by G, over an opacity of saturate(R × alphaDensity) × the particle's alpha faded into the depth over
    depthFade. (The classic game's slashes darken what lies under their dim parts, as an opacity from R with a colour
    from a power of R does.)"""
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
    shape = g.power(tex, "R", g.scalar("colorCorrect", 2.0, -1100, 50), "", -900, -50)
    base = g.multiply(particle, "RGB", shape, "", -700, 0)
    hilight = g.vector("hilightColor", (3.9051918983459473, 4.095554828643799, 5.0, 1.0), -900, -300)
    g.out(g.lerp(base, "", hilight, "RGB", tex, "G", -450, -100), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    density = g.multiply(tex, "R", g.scalar("alphaDensity", 1.5, -1100, 150), "", -900, 150)
    clamped = g.node(unreal.MaterialExpressionSaturate, -750, 150)
    dd_assets.connect(density, "", clamped, "")
    fade = g.node(unreal.MaterialExpressionDepthFade, -400, 200)
    dd_assets.connect(g.multiply(clamped, "", particle, "A", -600, 200), "", fade, "Opacity")
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
    material's opacity input, so whether it was connected is unknown). The estimate: Desaturation(Color × B,
    Desaturation) + Color × R as the emissive colour, saturate(B + R) × Opacity as the opacity."""
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("used_with_static_lighting", True)
    g = dd_stage._Graph(mat)
    pan = g.node(unreal.MaterialExpressionPanner, -1300, 0)
    pan.set_editor_property("speed_x", PRIMAL_PAN_SPEED[0])
    pan.set_editor_property("speed_y", PRIMAL_PAN_SPEED[1])
    tex = g.node(unreal.MaterialExpressionTextureSample, -1100, 0)
    tex.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(PRIMAL_TEXTURE)))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    dd_assets.connect(pan, "", tex, "UVs")
    colour = g.vector("Color", (1.0, 0.0, 0.0, 1.0), -1100, -300)
    clouds = g.multiply(colour, "RGB", tex, "B", -850, -200)
    washed = g.node(unreal.MaterialExpressionDesaturation, -650, -200)
    dd_assets.connect(clouds, "", washed, "")
    dd_assets.connect(g.scalar("Desaturation", 0.0, -850, -50), "", washed, "Fraction")
    sparkles = g.multiply(colour, "RGB", tex, "R", -650, -350)
    glow = g.binary(unreal.MaterialExpressionAdd, washed, "", sparkles, "", -400, -250)
    g.out(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    cover = g.binary(unreal.MaterialExpressionAdd, tex, "B", tex, "R", -850, 150)
    clamped = g.node(unreal.MaterialExpressionSaturate, -650, 150)
    dd_assets.connect(cover, "", clamped, "")
    opacity = g.multiply(clamped, "", g.scalar("Opacity", 1.0, -650, 300), "", -400, 200)
    g.out(opacity, "", unreal.MaterialProperty.MP_OPACITY)


def _build_looping_smoke(mat):
    """M_LoopingSmoke1_Sheet (pak_reference_2), estimated. The cook kept its settings (translucent, no separate
    translucency, for sprites; the default lit shading, as the cook writes any other), a ParticleSubUV of
    T_LoopingSmoke_8x8 and a CameraDepthFade call, of ten expressions; it kept no emissive colour, which it keeps where
    one is connected. The estimate: a base colour of the frame's RGB × the particle's colour (UE saturates a base colour,
    so the puff's colours over 1 come out near white) and an opacity of the frame's alpha × the particle's alpha × the
    depth fade (at its defaults)."""
    mat.set_editor_property("translucency_pass", unreal.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    mat.set_editor_property("used_with_particle_sprites", True)
    g = dd_stage._Graph(mat)
    frame = g.node(unreal.MaterialExpressionParticleSubUV, -900, 0)
    frame.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(LOOPING_SMOKE_TEXTURE)))
    particle = g.node(unreal.MaterialExpressionParticleColor, -900, 300)
    g.out(g.multiply(frame, "RGB", particle, "RGB", -600, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    fade = dd_assets.function_call(g, "Opacity/CameraDepthFade", -900, 500)
    alpha = g.multiply(frame, "A", particle, "A", -600, 250)
    g.out(g.multiply(alpha, "", fade, "Result", -400, 300), "", unreal.MaterialProperty.MP_OPACITY)


def _build_wobbly_vignette(mat):
    """MM_WobblyVignette (pak_reference_2), estimated (see WOBBLE_*). The cook kept its settings (the UI domain,
    translucent), its emissive colour (the RGB of T_VignetteNew at a TextureCoordinate: white), a second T_VignetteNew
    sample, two samples of T_perlinnoise at two Panners and a LinearSine call, of 22 expressions. The estimate: an
    opacity of the second vignette's alpha × the two panning noises crossfaded by LinearSine(Time) × a gain. The widget's
    purple tints the white."""
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
    wobble = g.lerp(noises[0], "R", noises[1], "R", sine, "Linear Sine", -950, 150)
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
    expressions (the ExponentialDensity it lists is the gradient's own). The estimate: both noises read at TexCoord 0 ×
    Tiling, panning by Time × Speed, and an opacity of saturate((the noises' R summed) × the gradient × a gain). UI
    additive blending adds the colour × the opacity (× the widget's opacity)."""
    g = dd_stage._Graph(mat)
    g.out(g.vector("Color", (1.0, 0.0, 0.0, 1.0), -600, -350), "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    coords = g.node(unreal.MaterialExpressionTextureCoordinate, -1700, 0)
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
            + make_vanish_materials() + make_telepathy_materials())


def import_all():
    """Imports and builds every asset the powers use, then saves /Game/DD and /Game/Pipeline. Returns how many of each
    kind."""
    result = {"sounds": len([dd_assets.sound(rel, version) for version, rel in SOUNDS])}
    result["camera_shakes"] = len([dd_assets.camera_shake(rel, version) for version, rel in CAMERA_SHAKES])
    result["camera_anims"] = len([dd_assets.camera_anim(rel, version) for version, rel in CAMERA_ANIMS])
    result["textures"] = len([dd_assets.texture(rel, version) for version, rel in TEXTURES])
    result["materials"] = len(make_materials())
    result["particle_systems"] = len([dd_particles.particle_system(rel, version) for version, rel in PARTICLE_SYSTEMS])
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
