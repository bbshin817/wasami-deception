"""The special shards: the stun orb (AWasamiPowerOrb, after the latest version's Blueprints/Main/BP_PowerOrb) and the
red shard (AWasamiBonusShard, after BP_BonusShard), and what they show and play.

  the bodies     power_orb (the orb) and soul_shard (the red shard, scaled up 20 times), with the original's crystal
                 instances m_crystal_Inst3 (orange, the orb's) and m_crystal_Inst (red) of m_crystal, whose graph the
                 cook took away: estimated off its compiled shaders (M_DD_Crystal). Its third instance, m_crystal_Inst2
                 (purple), is the stage's altar's orb
  the map marks  M_PowerOrb (a plane of one colour, an instance of the shards' M_DD_MapMark) and M_Bonus_Shard and
                 M_Enemy (the enemies' marks while a red shard reveals them): planes cut to T_EnemyTriangle, instances of
                 M_DD_MapMarkMasked. The colours are the constants the compiled shaders keep
  the flashes    AdvancedMagicFX13's P_ky_flash_PowerOrb_Appear / _Disappear and P_ky_flash_BonusOrb_Appear /
                 _Disappear (as a shard moves to its next spawn point), P_ky_impact (the orb taken) and P_ky_impact1 (the
                 red shard taken), with their textures and the materials the shards' flash (dd_shards) and the zone
                 barrier's burst (dd_gimmicks) have not made: MI_ky_flare01b_primitiveG / R (instances of the shards'
                 estimated M_ky_flare01_primitive), M_ky_primitiveColor with MI_ky_primitiveColor and M_ky_lensFlare02
                 (estimated off their compiled shaders)
  the sounds     8-Dark_power_ball_countdown_ (the orb taken), Bonus_Shard_Pickup_v1 (the red shard taken) and
                 Stun_Wave_Attack_New_04 (the collect effects' wave)

The pickup's cue (Soul_Shard_Pickup_v2_Cue) comes with the shards, the shake BP_CameraShake_Streak with the powers,
01_Hotel_Lobby_ElevatorShakeStop and M_05_Primal (the stun effect's sphere) with the powers, T_VignetteNew and the font
of UMG_VignetteSides with the powers and the tablet. PPP_Collect_Shard is not made: no flow of either Blueprint
activates it.

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24; the hospital is only
in the latest version).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
VERSION = 2

SOUNDS = ("Audio/SharedGameplay/8-Dark_power_ball_countdown_", "Audio/SharedGameplay/Bonus_Shard_Pickup_v1",
          "Audio/SharedGameplay/Stun_Wave_Attack_New_04")
KY = "ThirdParty/AdvancedMagicFX13/"
TEXTURES = ("Textures/Shared/T_EnemyTriangle", KY + "Textures/T_ky_flash01_4x4", KY + "Textures/T_ky_lensFlare01",
            KY + "Textures/T_ky_maskRGB3")
ORB_MESH = "Meshes/Shared/power_orb"
MESHES = (ORB_MESH, "Meshes/Ring_Assets/soul_shard")

CRYSTAL = "Materials/Fords_Materials/m_crystal"
CRYSTAL_MASTER = dd_assets.PIPELINE_MATERIALS + "M_DD_Crystal"
ORB_CRYSTAL = "Materials/Fords_Materials/m_crystal_Inst3"
# m_crystal_Inst2 is the altar's orb (ring_statue_orb, purple): the stage import has this module make it.
CRYSTAL_INSTANCES = (ORB_CRYSTAL, "Materials/Fords_Materials/m_crystal_Inst",
                     "Materials/Fords_Materials/m_crystal_Inst2")
# The parameters of m_crystal the estimate does without (none left: distortion_normal is read since 2026-09-21).
CRYSTAL_LEFT_OUT = ()
DEFAULT_CUBE = "/Engine/EngineResources/DefaultTextureCube"
# distortion_normal's default in the original's master, which the orb's and the altar's orb's instances set again. The
# original cooks it into its pak (Engine/Content/EditorShapes/Textures/T_ShapeNormal), so the path is kept as it is.
SHAPE_NORMAL = "/Engine/EditorShapes/Textures/T_ShapeNormal"

# The marks' colours, as the compiled shaders keep their Constant3Vector (their emissive colour; the export keeps the
# node without its value). M_Shard's own is (0.482, 0, 1), which dd_shards set otherwise after measuring the tablet.
MAP_MARK_MASTER = "/Game/Pipeline/Materials/M_DD_MapMark"   # dd_shards' (make_map_mark)
MASKED_MARK_MASTER = dd_assets.PIPELINE_MATERIALS + "M_DD_MapMarkMasked"
ENEMY_TRIANGLE = "Textures/Shared/T_EnemyTriangle"
ORB_MARK = ("Materials/Shared/M_PowerOrb", (1.0, 0.2903, 0.0, 1.0))
MASKED_MARKS = (("Materials/Shared/M_Bonus_Shard", (1.0, 0.0, 0.0, 1.0)),
                ("Materials/Shared/M_Enemy", (1.0, 0.0, 0.0, 1.0)))
MASK_CLIP = 0.3333   # the materials' OpacityMaskClipValue (the engine's default, which the shaders compare with)

FLARE01 = KY + "Materials/M_ky_flare01_primitive"   # dd_shards' estimate's instance at the original's path
FLARE01B = (KY + "Materials/MI_ky_flare01b_primitiveG", KY + "Materials/MI_ky_flare01b_primitiveR")
PARTICLE_NEEDS = (FLARE01, KY + "Materials/MI_ky_flare01_primitiveG", KY + "Materials/MI_ky_flare01_primitiveR",
                  KY + "Materials/MI_ky_primitive2_trs", KY + "Materials/M_ky_empty", KY + "Materials/M_ky_polarGlow02",
                  KY + "Materials/M_ky_primitive_dyn2", KY + "Materials/MI_ky_flare14R")
PARTICLE_SYSTEMS = tuple(KY + "Particles/" + name for name in (
    "P_ky_flash_PowerOrb_Appear", "P_ky_flash_PowerOrb_Disappear", "P_ky_flash_BonusOrb_Appear",
    "P_ky_flash_BonusOrb_Disappear", "P_ky_impact", "P_ky_impact1"))


def _build_crystal(mat, d):
    """m_crystal, estimated. The cook kept its outputs (metallic, specular, emissive; the normal unconnected), its
    parameters (color1, color2, emissive_col, Fresnel Setting, Additive Emissive; emissive_entensity, emissive_speed,
    tile_ratio, roughness; distortion_normal, surface_normal, env_cubemap), a FeatureLevelSwitch between two Noise
    nodes, BoundingBoxBased_0-1_UVW and a TextureSampleParameterCube read along a Custom node. Its SM5 base pass reads
    (opaque, lit):
      emissive  Noise(ReflectionVector × 0.75 + time × emissive_speed; the 3D texture gradient noise, turbulent,
                4 levels, −0.5 … 0.5) × emissive_col × emissive_entensity + Fresnel(5, 0.04) × Fresnel Setting +
                Additive Emissive
      t         Noise(world position × tile_ratio − time × emissive_speed; the texture simplex noise, turbulent,
                4 levels) + the bounding box's Z (0 … 1) − 0.5
      base      lerp(color2, color1, t) + env_cubemap along refract(−camera, normal, 0.66) × 0.5
      metallic  t × 0.5; specular t; roughness roughness
    The shader's max(emissive, 0) and the saturates on the base colour, the metallic, the specular and the roughness
    are the engine's own (MaterialTemplate.ush's GetMaterial*), not nodes: the export has the emissive on an Add and
    the specular on an Add, and the engine puts its clamp after the editor's SelectionColor lerp. They are left out.
    Both the reflection and the refraction are read about the same normal: distortion_normal (sampled at UV × 0.1)
    turned into the world and added to the vertex normal, left unnormalized. T_ShapeNormal, what every instance puts
    there, is flat to within a 255th, so the sum is twice the vertex normal: not the same vector a plain reflection
    gives, which is why it is read as the shader has it rather than left out."""
    g = dd_stage._Graph(mat, checked=True)
    time = g.node(unreal.MaterialExpressionTime, -2400, 0)
    drift = g.multiply(time, "", g.scalar("emissive_speed", d["emissive_speed"], -2400, 100), "", -2200, 50)

    # The normal both vectors are read about.
    uv = g.multiply(g.node(unreal.MaterialExpressionTextureCoordinate, -3300, -750), "",
                    dd_assets.constant(g, 0.1, -3300, -620), "", -3100, -700)
    bump = g.texture("distortion_normal", unreal.load_asset(SHAPE_NORMAL),
                     unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -2950, -800)
    dd_assets.connect(uv, "", bump, "UVs")
    to_world = g.node(unreal.MaterialExpressionTransform, -2700, -800)
    to_world.set_editor_property("transform_source_type",
                                 unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_TANGENT)
    to_world.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    dd_assets.connect(bump, "", to_world, "")
    bent = dd_assets.add(g, to_world, "", g.node(unreal.MaterialExpressionVertexNormalWS, -2700, -650), "",
                         -2550, -750)

    # The glow: turbulent noise along the reflection about that normal.
    reflection = g.node(unreal.MaterialExpressionReflectionVectorWS, -2400, -400)
    reflection.set_editor_property("normalize_custom_world_normal", False)
    dd_assets.connect(bent, "", reflection, "CustomWorldNormal")
    along = g.multiply(reflection, "", dd_assets.constant(g, 0.75, -2400, -300), "", -2200, -350)
    glow_at = dd_assets.add(g, along, "", drift, "", -2000, -300)
    glow = _noise(g, glow_at, unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_TEX3D, -0.5, 0.5, -1800, -300)
    colour = g.vector("emissive_col", d["emissive_col"], -1800, -500)
    lit = g.multiply(g.multiply(glow, "", colour, "RGB", -1550, -400), "",
                     g.scalar("emissive_entensity", d["emissive_entensity"], -1550, -250), "", -1350, -350)
    fresnel = g.node(unreal.MaterialExpressionFresnel, -1550, -150)
    fresnel.set_editor_property("exponent", 5.0)
    fresnel.set_editor_property("base_reflect_fraction", 0.04)
    rim = g.multiply(fresnel, "", g.vector("Fresnel Setting", d["Fresnel Setting"], -1550, 0), "RGB", -1350, -150)
    emissive = dd_assets.add(g, dd_assets.add(g, lit, "", rim, "", -1150, -300), "",
                             g.vector("Additive Emissive", d["Additive Emissive"], -1350, 0), "RGB", -950, -250)
    g.out(emissive, "", MP.MP_EMISSIVE_COLOR)

    # The body: noise through the world, rising up the bounding box.
    world = g.node(unreal.MaterialExpressionWorldPosition, -2400, 300)
    tiled = g.multiply(world, "", g.scalar("tile_ratio", d["tile_ratio"], -2400, 400), "", -2200, 350)
    body_at = g.binary(unreal.MaterialExpressionSubtract, tiled, "", drift, "", -2000, 300)
    body = _noise(g, body_at, unreal.NoiseFunction.NOISEFUNCTION_SIMPLEX_TEX, 0.0, 1.0, -1800, 300)
    box = dd_assets.function_call(g, "UVs/BoundingBoxBased_0-1_UVW", -2000, 550, dd_assets.FUNCTIONS_02)
    t = dd_assets.add(g, dd_assets.add(g, body, "", box, "B", -1600, 400), "",
                      dd_assets.constant(g, -0.5, -1600, 550), "", -1400, 450)
    mixed = g.lerp(g.vector("color2", d["color2"], -1400, 150), "RGB", g.vector("color1", d["color1"], -1400, 300),
                   "RGB", t, "", -1150, 250)
    refract = g.node(unreal.MaterialExpressionCustom, -1400, 700)
    refract.set_editor_property("code", "return refract(-V, N, 0.66);")
    refract.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    refract.set_editor_property("description", "Refract")
    inputs = []
    for name in ("V", "N"):
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        inputs.append(pin)
    refract.set_editor_property("inputs", inputs)
    dd_assets.connect(g.node(unreal.MaterialExpressionCameraVectorWS, -1650, 650), "", refract, "V")
    dd_assets.connect(bent, "", refract, "N")
    cube = g.node(unreal.MaterialExpressionTextureSampleParameterCube, -1150, 650)
    cube.set_editor_property("parameter_name", "env_cubemap")
    cube.set_editor_property("texture", unreal.load_asset(DEFAULT_CUBE))
    dd_assets.connect(refract, "", cube, "UVs")
    halved = g.multiply(cube, "RGB", dd_assets.constant(g, 0.5, -1150, 850), "", -900, 650)
    g.out(dd_assets.add(g, mixed, "", halved, "", -800, 400), "", MP.MP_BASE_COLOR)
    g.out(g.multiply(t, "", dd_assets.constant(g, 0.5, -1150, 500), "", -900, 500), "", MP.MP_METALLIC)
    g.out(t, "", MP.MP_SPECULAR)
    g.out(g.scalar("roughness", d["roughness"], -900, 800), "", MP.MP_ROUGHNESS)


def _noise(g, position, function, low, high, x, y):
    """A turbulent Noise of 4 levels at position (the scale 1, the level scale 2: what the crystal's shader runs)."""
    e = g.node(unreal.MaterialExpressionNoise, x, y)
    e.set_editor_property("noise_function", function)
    e.set_editor_property("scale", 1.0)
    e.set_editor_property("turbulence", True)
    e.set_editor_property("levels", 4)
    e.set_editor_property("level_scale", 2.0)
    e.set_editor_property("output_min", low)
    e.set_editor_property("output_max", high)
    dd_assets.connect(position, "", e, "World Position")   # the input's name; what it reads is any position
    return e


def _crystal_parameters(rel):
    """A crystal instance's own values ({scalar: value}, {vector: [r, g, b, a]}, {texture: asset path}) without the
    engine's parameters and those the estimate does without. dd_assets.instance_parameters does not take these: their
    textures are the engine's (T_ShapeNormal, DefaultTextureCube), which keep their /Engine paths."""
    props = dd_assets.main_export(dd_assets.export_json(rel, VERSION), rel)["props"]
    if props.get("StaticParameters"):
        raise NotImplementedError("%s has static parameters" % rel)
    skip = dd_assets.ENGINE_PARAMETERS + CRYSTAL_LEFT_OUT

    def own(key):
        return {v["ParameterInfo"]["Name"]: v["ParameterValue"] for v in props.get(key, [])
                if v["ParameterInfo"]["Name"] not in skip}

    textures = {}
    for name, value in own("TextureParameterValues").items():
        package = value.split(".", 1)[0]
        textures[name] = package if package.startswith("/Engine/") else dd_assets.asset_path(dd_assets.game_rel(value))
    return own("ScalarParameterValues"), own("VectorParameterValues"), textures


def make_crystal():
    """M_DD_Crystal, m_crystal (an instance of it with the original's defaults) and the orb's, the red shard's and the
    altar's orb's instances of that (saved; one that exists is put on it in place, without the base property overrides
    the stage gave it before). Returns the assets."""
    scalars, vectors = dd_assets.parameter_defaults(CRYSTAL, VERSION)
    defaults = dict(scalars, **vectors)
    master = dd_assets.material(CRYSTAL_MASTER, lambda mat: _build_crystal(mat, defaults))
    known = {str(n) for f in (MEL.get_scalar_parameter_names, MEL.get_vector_parameter_names,
                              MEL.get_texture_parameter_names) for n in f(master)}
    base = dd_assets.material_instance(dd_assets.asset_path(CRYSTAL), master,
                                       scalars={k: v for k, v in scalars.items() if k in known},
                                       vectors={k: v for k, v in vectors.items() if k in known})
    made = [master, base]
    for rel in CRYSTAL_INSTANCES:
        c_scalars, c_vectors, c_textures = _crystal_parameters(rel)
        unknown = (set(c_scalars) | set(c_vectors) | set(c_textures)) - known
        if unknown:
            raise RuntimeError("%s sets %s, which the estimate of m_crystal does not have" % (rel, sorted(unknown)))
        mic = dd_assets.material_instance(dd_assets.asset_path(rel), base, scalars=c_scalars, vectors=c_vectors,
                                          textures=c_textures)
        mic.set_editor_property("base_property_overrides", unreal.MaterialInstanceBasePropertyOverrides())
        MEL.update_material_instance(mic)
        made.append(mic)
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return made


def _build_masked_mark(mat):
    """A minimap mark cut to a shape (M_Bonus_Shard, M_Enemy): M_DD_MapMark's Color, with the opacity mask of Mask's R
    (the shaders' clip of T_EnemyTriangle's one channel at 0.3333)."""
    mat.set_editor_property("opacity_mask_clip_value", MASK_CLIP)
    g = dd_stage._Graph(mat, checked=True)
    colour = g.vector("Color", (1.0, 1.0, 1.0, 1.0), -600, 0)
    g.out(colour, "RGB", MP.MP_BASE_COLOR)
    g.out(colour, "RGB", MP.MP_EMISSIVE_COLOR)
    shape = g.texture("Mask", unreal.load_asset(dd_assets.asset_path(ENEMY_TRIANGLE)),
                      unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE, -600, 200)
    g.out(shape, "R", MP.MP_OPACITY_MASK)


def make_map_marks():
    """M_PowerOrb (an instance of dd_shards' M_DD_MapMark), M_DD_MapMarkMasked and M_Bonus_Shard and M_Enemy (instances
    of it), at the original's paths (saved). Returns the assets."""
    if not EAL.does_asset_exist(MAP_MARK_MASTER):
        raise RuntimeError("missing %s: run WasamiDDTools.import_dd_shards first" % MAP_MARK_MASTER)
    rel, colour = ORB_MARK
    made = [dd_assets.material_instance(dd_assets.asset_path(rel), unreal.load_asset(MAP_MARK_MASTER),
                                        vectors={"Color": colour})]
    masked = dd_assets.material(MASKED_MARK_MASTER, _build_masked_mark, blend_mode=unreal.BlendMode.BLEND_MASKED)
    made.append(masked)
    for rel, colour in MASKED_MARKS:
        made.append(dd_assets.material_instance(dd_assets.asset_path(rel), masked, vectors={"Color": colour}))
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return made


def _build_primitive_color(mat, d):
    """M_ky_primitiveColor, estimated. The cook kept its settings (translucent, unlit, for sprites and mesh
    particles), the parameters hilightPower and hilightColor, MF_ky_addHilight and the static switch useHilight on its
    emissive colour (A an Add, B the particle colour's RGB) and its opacity (A a Multiply, B the particle's alpha). Its
    compiled shaders read a highlight of T_ky_maskRGB3's R and B at UV × 0.2 panned by time × (0.1, −1) and (−0.2, −2):
    strength = n1 × n2 × (n1 + n2) × 2500 × hilightPower, and the emissive colour strength × hilightColor + the
    particle colour (the Add). The opacity is the switch through a DepthFade over 100 (both shaders end in
    saturate(depth × 0.01) × the switch), the switch's A saturate(strength ^ 0.5) × the particle's alpha (the
    max(x, 0), rsq/div and min(…, 1) of MI_ky_primitiveColor's shader, which turns the switch on) and its B the
    particle's alpha alone (the master's shader)."""
    dd_assets.particle_material(mat)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -700, 300)
    masks = unreal.load_asset(dd_assets.asset_path(KY + "Textures/T_ky_maskRGB3"))
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1900, -200)
    uv.set_editor_property("u_tiling", 0.2)
    uv.set_editor_property("v_tiling", 0.2)
    samples = []
    for speed, keep, y in (((0.1, -1.0), "R", -300), ((-0.2, -2.0), "B", -50)):
        pan = g.node(unreal.MaterialExpressionPanner, -1650, y)
        pan.set_editor_property("speed_x", speed[0])
        pan.set_editor_property("speed_y", speed[1])
        dd_assets.connect(uv, "", pan, "Coordinate")
        sample = g.node(unreal.MaterialExpressionTextureSample, -1400, y)
        sample.set_editor_property("texture", masks)
        dd_assets.connect(pan, "", sample, "UVs")
        samples.append((sample, keep))
    (a, a_pin), (b, b_pin) = samples
    together = dd_assets.add(g, a, a_pin, b, b_pin, -1150, -300)
    both = g.multiply(a, a_pin, b, b_pin, -1150, -150)
    shaped = g.multiply(g.multiply(together, "", both, "", -950, -250), "", dd_assets.constant(g, 2500.0, -950, -100),
                        "", -750, -200)
    strength = g.multiply(shaped, "", g.scalar("hilightPower", d["hilightPower"], -750, -50), "", -550, -150)
    hilight = g.multiply(strength, "", g.vector("hilightColor", d["hilightColor"], -550, 0), "RGB", -350, -100)
    added = dd_assets.add(g, particle, "RGB", hilight, "", -200, 0)
    g.out(g.switch("useHilight", added, "", particle, "RGB", 0, 50), "", MP.MP_EMISSIVE_COLOR)
    root = g.node(unreal.MaterialExpressionPower, -350, 200)   # PositiveClampedPow: the shader's max(x, 0) before sqrt
    root.set_editor_property("const_exponent", 0.5)
    dd_assets.connect(strength, "", root, "Base")
    lit = dd_assets.single(g, unreal.MaterialExpressionSaturate, root, "", -200, 200)
    fade = g.node(unreal.MaterialExpressionDepthFade, 250, 300)   # the expression's own distance, 100
    dd_assets.connect(g.switch("useHilight", g.multiply(lit, "", particle, "A", -50, 250), "", particle, "A", 100, 300),
                      "", fade, "Opacity")
    g.out(fade, "", MP.MP_OPACITY)


def _build_lens_flare(mat, d):
    """M_ky_lensFlare02, estimated. The cook kept its settings (translucent, unlit, for sprites), its emissive colour
    (the particle colour's RGB), the parameters alphaDensity, remap1, remap2, rotRemap1 and rotRemap2, two calls of
    Sine_Remapped and a sample of T_ky_lensFlare01 at a Rotator. Its compiled shaders read the opacity as
    lerp(remap1, remap2, s(time, 2)) × G^alphaDensity × the particle's alpha, G read at the texture coordinates turned
    about the middle by lerp(rotRemap1, rotRemap2, s(time, 0.5)) × 0.25, where s(time, p) = (sin(2π sin(2π time / p)) + 1)
    / 2 (a Sine of period p through Sine_Remapped): the flare pulses and rocks. The shaders' last saturate is the
    engine's own clamp of the opacity output, so no Saturate is built."""
    dd_assets.particle_material(mat)
    g = dd_stage._Graph(mat, checked=True)
    time = g.node(unreal.MaterialExpressionTime, -2000, 0)

    def pulse(period, low, high, y):
        wave = g.node(unreal.MaterialExpressionSine, -1800, y)
        wave.set_editor_property("period", period)
        dd_assets.connect(time, "", wave, "")
        again = dd_assets.single(g, unreal.MaterialExpressionSine, wave, "", -1650, y)
        half = g.multiply(dd_assets.add(g, again, "", dd_assets.constant(g, 1.0, -1650, y + 100), "", -1500, y), "",
                          dd_assets.constant(g, 0.5, -1500, y + 100), "", -1350, y)
        return g.lerp(g.scalar(low, d[low], -1350, y + 100), "", g.scalar(high, d[high], -1350, y + 200), "",
                      half, "", -1150, y)

    turn = pulse(0.5, "rotRemap1", "rotRemap2", -300)
    rotator = g.node(unreal.MaterialExpressionRotator, -950, -300)
    rotator.set_editor_property("center_x", 0.5)
    rotator.set_editor_property("center_y", 0.5)
    rotator.set_editor_property("speed", 0.25)
    dd_assets.connect(turn, "", rotator, "Time")
    flare = g.node(unreal.MaterialExpressionTextureSample, -750, -300)
    flare.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(KY + "Textures/T_ky_lensFlare01")))
    flare.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    dd_assets.connect(rotator, "", flare, "UVs")
    shaped = g.power(flare, "G", g.scalar("alphaDensity", d["alphaDensity"], -750, -100), "", -550, -250)
    bright = pulse(2.0, "remap1", "remap2", 200)
    particle = g.node(unreal.MaterialExpressionParticleColor, -400, 200)
    g.out(g.multiply(g.multiply(shaped, "", bright, "", -400, -100), "", particle, "A", -250, -100), "",
          MP.MP_OPACITY)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)


FLASH_MATERIALS = (
    ("M_ky_primitiveColor", "M_DD_KyPrimitiveColor", _build_primitive_color, ("MI_ky_primitiveColor",)),
    ("M_ky_lensFlare02", "M_DD_KyLensFlare02", _build_lens_flare, ()),
)


def make_flash_materials():
    """MI_ky_flare01b_primitiveG / R (instances of dd_shards' M_ky_flare01_primitive with their own values), and the
    estimated M_ky_primitiveColor with MI_ky_primitiveColor (its two-sided override) and M_ky_lensFlare02 (saved).
    Returns the assets."""
    parent = unreal.load_asset(dd_assets.asset_path(FLARE01))
    known = {str(n) for f in (MEL.get_scalar_parameter_names, MEL.get_texture_parameter_names)
             for n in f(parent)}
    made = []
    for rel in FLARE01B:
        scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(rel, VERSION)
        unknown = (set(scalars) | set(textures)) - known
        if unknown or vectors or switches:
            raise RuntimeError("%s sets %s, which the estimate of M_ky_flare01_primitive does not have"
                               % (rel, sorted(unknown) or (vectors, switches)))
        mic = dd_assets.material_instance(dd_assets.asset_path(rel), parent, scalars=scalars, textures=textures,
                                          static_masks=masks)
        dd_assets.base_property_overrides(mic, rel, VERSION)
        made.append(mic)
    made += dd_assets.estimated_materials(KY + "Materials/", FLASH_MATERIALS, VERSION)
    child = KY + "Materials/MI_ky_primitiveColor"
    dd_assets.base_property_overrides(unreal.load_asset(dd_assets.asset_path(child)), child, VERSION)
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return made


def import_meshes():
    """power_orb and soul_shard. power_orb keeps its own slot's material, m_crystal_Inst3 (made by make_crystal);
    soul_shard's (m_crystal_Inst1) is not made: the red shard puts m_crystal_Inst on it. Returns the package paths."""
    made = [dd_assets.static_mesh(rel, VERSION) for rel in MESHES]
    orb = unreal.load_asset(dd_assets.asset_path(ORB_MESH))
    orb.set_material(0, unreal.load_asset(dd_assets.asset_path(ORB_CRYSTAL)))
    EAL.save_loaded_asset(orb, only_if_is_dirty=False)
    return made


def import_all():
    """Imports and builds everything the special shards use, then saves /Game/DD and /Game/Pipeline. The shards
    (WasamiDDTools.import_dd_shards) and the zone barrier's burst (import_dd_gimmicks) have to have been imported: their
    map mark master and particle materials are shared. Returns how many of each kind."""
    missing = [rel for rel in PARTICLE_NEEDS if not EAL.does_asset_exist(dd_assets.asset_path(rel))]
    if missing:
        raise RuntimeError("missing %s: run WasamiDDTools.import_dd_shards and import_dd_gimmicks first"
                           % ", ".join(missing))
    result = {"sounds": len([dd_assets.sound(rel, VERSION) for rel in SOUNDS]),
              "textures": len([dd_assets.texture(rel, VERSION) for rel in TEXTURES])}
    result["crystal_materials"] = len(make_crystal())
    result["meshes"] = len(import_meshes())
    result["map_mark_materials"] = len(make_map_marks())
    result["flash_materials"] = len(make_flash_materials())
    result["particle_systems"] = len([dd_particles.particle_system(rel, VERSION) for rel in PARTICLE_SYSTEMS])
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
