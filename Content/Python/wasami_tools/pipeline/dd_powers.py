"""Dark Deception's tablet powers: the sounds, camera shakes, camera anims, textures, materials and particle systems
the power system (UWasamiPowerComponent), the teleport's aim (AWasamiTeleportAim) and the player's FX
(UWasamiChameleonComponent) use. The icons on the tablet's sockets are the tablet's own (dd_tablet).

  M_Speedlines                  the original's graph, node for node (FlipBook at its defaults → T_Speedlines)
  M_DD_ChameleonCameraShake     the Chameleon pack's M_CameraShake, estimated (its graph is cooked away)
  M_DD_KySlash, M_DD_PPPRadialGradient, M_DD_DecalTeleport
                                estimated masters of the teleport aim's materials (their graphs are cooked away); the
                                original's paths hold instances of them with the original's parameter values

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
)
CAMERA_SHAKES = (
    (2, "UI/Menu/Streaks/BP_CameraShake_Streak"),  # the speed boost starts, the teleport moves (the same in both versions)
)
CAMERA_ANIMS = (
    (2, "Animation/Camera/CameraAnim_SpeedBoost"),  # the view turns red while the speed boost lasts
    (1, "Animation/Camera/CameraAnim_Teleport"),    # the teleport's click: the view widens, flashes and settles
)
TEXTURES = (
    (2, "UI/Main/Powers/T_Speedlines"),    # UMG_SpeedBoost's lines (a 2 × 5 sheet; M_Speedlines reads it as 2 × 2)
    (2, "UI/Menu/Streaks/T_VignetteNew"),  # UMG_SpeedBoost's vignette
    (1, "ThirdParty/AdvancedMagicFX13/Textures/T_ky_slash01_4x4"),  # the teleport aim's slashes (4 × 4 frames)
)
# Cascade systems (dd_particles), made after the materials they use.
PARTICLE_SYSTEMS = (
    (1, "ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2"),  # the teleport aim's slashes and sparks
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
FUNCTIONS = "/Engine/Functions/Engine_MaterialFunctions01/"
# The estimate of M_Decal_Teleport. The classic game shows (2026-09-17, 04 record) a disc with a sharp edge that
# glows additively and pulses once a second. Its size is RadialGradientExponential at its defaults cut by CheapContrast,
# whose contrast sets the edge's width (about a tenth of the radius). The colour and the brightness are placeholders
# until they are compared with the latest version's hospital (the classic's Manor grades its colours too heavily to
# read them back).
DECAL_CONTRAST = 5.0
DECAL_COLOR = (1.0, 0.105, 0.09, 1.0)
DECAL_PULSE_LOW = 0.04
DECAL_PULSE_HIGH = 0.6


def _connect(a, a_pin, b, b_pin):
    if not MEL.connect_material_expressions(a, a_pin, b, b_pin):
        raise RuntimeError("could not connect %s.%s to %s.%s" % (a.get_name(), a_pin, b.get_name(), b_pin))


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
    _connect(call, str(outputs[FLIPBOOK_UV_OUTPUT]), sample, "UVs")
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
    _connect(phase, "", sine, "")
    cosine = g.node(unreal.MaterialExpressionCosine, -850, 0)
    _connect(phase, "", cosine, "")
    circle = g.node(unreal.MaterialExpressionAppendVector, -700, -50)
    _connect(sine, "", circle, "A")
    _connect(cosine, "", circle, "B")
    offset = g.multiply(circle, "", power, "", -550, 0)
    screen = g.node(unreal.MaterialExpressionScreenPosition, -700, -250)
    uv = g.binary(unreal.MaterialExpressionAdd, screen, "ViewportUV", offset, "", -400, -150)
    scene = g.node(unreal.MaterialExpressionSceneTexture, -250, -150)
    scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    _connect(uv, "", scene, "UVs")
    g.out(scene, "Color", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _function(g, name, x, y):
    call = g.node(unreal.MaterialExpressionMaterialFunctionCall, x, y)
    call.set_editor_property("material_function", unreal.load_asset(FUNCTIONS + name))
    return call


def _constant(g, value, x, y):
    e = g.node(unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", value)
    return e


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
    _connect(density, "", clamped, "")
    fade = g.node(unreal.MaterialExpressionDepthFade, -400, 200)
    _connect(g.multiply(clamped, "", particle, "A", -600, 200), "", fade, "Opacity")
    _connect(g.scalar("depthFade", 100.0, -600, 350), "", fade, "FadeDistance")
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
    gradient = _function(g, "Gradient/RadialGradient", -900, 150)
    fade = _function(g, "Opacity/CameraDepthFade", -900, 300)
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
    gradient = _function(g, "Gradient/RadialGradientExponential", -1300, 0)
    contrast = _function(g, "ImageAdjustment/CheapContrast", -1050, 0)
    _connect(gradient, "RadialGradientExponential", contrast, "In")
    _connect(g.scalar("Contrast", DECAL_CONTRAST, -1300, 200), "", contrast, "Contrast")
    disc = g.node(unreal.MaterialExpressionSaturate, -850, 0)
    _connect(contrast, "Result", disc, "")
    time = g.node(unreal.MaterialExpressionTime, -1300, 350)
    sine = g.node(unreal.MaterialExpressionSine, -1150, 350)
    _connect(time, "", sine, "")
    half = _constant(g, 0.5, -1150, 450)
    wave = g.binary(unreal.MaterialExpressionAdd, g.multiply(sine, "", half, "", -1000, 350), "", half, "", -850, 350)
    low = g.scalar("PulseLow", DECAL_PULSE_LOW, -850, 200)
    high = g.scalar("PulseHigh", DECAL_PULSE_HIGH, -850, 280)
    pulse = g.lerp(low, "", high, "", wave, "", -650, 250)
    colour = g.multiply(g.vector("Color", DECAL_COLOR, -850, -250), "RGB",
                        g.scalar("Intensity", 1.0, -850, -100), "", -650, -150)
    glow = g.multiply(colour, "", disc, "", -450, -50)
    g.out(g.multiply(glow, "", pulse, "", -250, 50), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def slash_parameters():
    """M_ky_slash01_4x4's parameter defaults and texture, from its export: ({name: value}, {name: [r, g, b, a]}, the
    texture's rel)."""
    scalars, vectors, texture = {}, {}, None
    for e in dd_assets.export_json(KY_SLASH, 1)["exports"]:
        p = e["props"]
        if e["class"] == "MaterialExpressionScalarParameter":
            scalars[p["ParameterName"]] = p["DefaultValue"]
        elif e["class"] == "MaterialExpressionVectorParameter":
            vectors[p["ParameterName"]] = p["DefaultValue"]
        elif e["class"] == "MaterialExpressionParticleSubUV":
            texture = dd_assets.game_rel(p["Texture"])
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


def make_materials():
    speedlines = dd_assets.material(dd_assets.asset_path(SPEEDLINES), _build_speedlines,
                                    domain=unreal.MaterialDomain.MD_UI, blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    shake = dd_assets.material(CAMERA_SHAKE_MASTER, _build_camera_shake, domain=unreal.MaterialDomain.MD_POST_PROCESS)
    return [speedlines.get_path_name(), shake.get_path_name()] + make_teleport_materials()


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
