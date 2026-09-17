"""The soul shard (AWasamiShard, after the latest version's BP_Shard): what it shows and plays.

  SM_WasamiMochi   this game's shard, the Wasami mochi (SourceArt/Wasami/wasami_mochi.glb, the WebGL version's 6000
                   triangles with its three 1024² JPEGs), under /Game/Wasami/Shard with its textures and MI_WasamiMochi
  M_DD_WasamiMochi the mochi's master: the glTF's metallic-roughness material, two-sided, glowing with its own base
                   colour × Glow (the WebGL version's game.shard.glow, which keeps it legible on a dark stage) and a
                   purple pulse of our own (the original's crystal does not pulse)
  M_DD_MapMark     the estimated master of the minimap's marks; M_Shard (the shard's plane) is an instance of it
  the pickup       Soul_Shard_Pickup_v2 and its cue (a modulator of pitch 0.9 – 1.1), OnlyFew, and
                   BP_CameraShake_ShardCollect
  the flash        P_ky_flash3 (AdvancedMagicFX13's Cascade system: a shockwave, glows, a star, converging lines and a
                   light) with its textures; its five materials' graphs are cooked away, so masters holding our
                   estimates (M_DD_Ky*) sit under /Game/Pipeline, and instances of them at the original's paths, the
                   original's instances being instances of those (MI_ky_flare01_primitiveG / R keep their channel);
                   the shards play P_WasamiShardFlash, our purple and weaker version of it under /Game/Wasami/Shard

Sources: pak_reference_2 (UE 4.24, the latest version), which the shards follow; the pickup's sound, cue and shake and
the flash are the same in both versions.
"""
import json
import os
import struct

import unreal

from wasami_tools.pipeline import dd_assets, dd_particles, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

# (pak_reference version, the original's path under /Game).
SOUNDS = ((2, "Audio/SharedGameplay/Soul_Shard_Pickup_v2"),)
SOUND_CONCURRENCIES = ((2, "Audio/OnlyFew"),)       # PlaySound2D's concurrency: one pickup sound at a time
SOUND_CUES = ((2, "Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue"),)  # after the waves it plays
CAMERA_SHAKES = ((2, "Blueprints/Shared/BP_CameraShake_ShardCollect"),)

MOCHI_SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "wasami_mochi.glb")
MOCHI_FOLDER = paths.WASAMI_ROOT + "/Shard"
MOCHI_MESH = MOCHI_FOLDER + "/SM_WasamiMochi"
MOCHI_MATERIAL = MOCHI_FOLDER + "/MI_WasamiMochi"
MOCHI_MASTER = "/Game/Pipeline/Materials/M_DD_WasamiMochi"
MOCHI_GLOW = 0.3
# The purple pulse, this game's own (the user's request; neither the original nor the WebGL version has it): the base
# colour × PulseColor × PulseStrength × (0.5 + 0.5 sin(2π (time / PulsePeriod + PulsePhase))) joins the glow. The
# colour is the shard light's (194, 0, 255) in linear. PulsePhase is the mochi's custom primitive data, a random
# fraction AWasamiShard gives each at BeginPlay (AWasamiShard::PulsePhaseData).
MOCHI_PULSE_COLOR = (0.539, 0.0, 1.0, 1.0)
MOCHI_PULSE_STRENGTH = 1.0  # TODO(仮)
MOCHI_PULSE_PERIOD = 2.0    # TODO(仮) seconds
MOCHI_PULSE_PHASE_DATA = 0
# The glb's embedded pictures, taken out to be imported: (the glTF material's texture, our parameter and texture name,
# sRGB, compression, LOD group). glTF's normal maps point Y up; UE's point it down, so the green channel is flipped.
MOCHI_TEXTURES = (
    ("baseColorTexture", "BaseColor", True, None, None),
    ("metallicRoughnessTexture", "MetallicRoughness", False, None, None),
    ("normalTexture", "Normal", False, "TC_Normalmap", "TEXTUREGROUP_WorldNormalMap"),
)
MOCHI_EXTRACTED = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "shard")

MAP_MARK_MASTER = "/Game/Pipeline/Materials/M_DD_MapMark"
SHARD_MARK = "Materials/Shared/M_Shard"
# M_Shard's export keeps one Constant3Vector as its emissive colour, without its value, and the default lit shading.
# The player's scene capture reads the base colour, so the estimate gives both the colour. The WebGL version measured the
# original's marks on screen as #d21ee6 (210, 30, 230); the tablet shows its map through the stage's tonemapping (the
# map's lines match the latest version's screen that way), so the colour was found by measuring our tablet in PIE: this
# one shows (211, 29, 217). Blue cannot go higher: a base colour stops at 1. (sRGB → linear of #d21ee6 would show
# (205, 9, 206).)
SHARD_MARK_COLOR = (0.70, 0.0071, 1.0, 1.0)

# The collect flash (AdvancedMagicFX13, pak_reference_2; the same in both versions).
KY = "ThirdParty/AdvancedMagicFX13/"
FLASH_TEXTURES = (
    (2, KY + "Textures/T_ky_flare01"),          # R a star, G a soft round glow, B thin rays (linear)
    (2, KY + "Textures/T_ky_flareVertical02"),  # M_ky_flare01_primitive's own texture (its instances swap it)
    (2, KY + "Textures/T_ky_decoLinesB_sml"),   # white, with streaks down V in its alpha
    (2, KY + "Textures/T_ky_deco_rainbow"),     # a rainbow down V
)
PARTICLE_SYSTEMS = ((2, KY + "Particles/P_ky_flash3"),)  # after the materials it uses
MP = unreal.MaterialProperty
# The collect flash the shards play, this game's own (the user's request: purple and a little weaker): P_ky_flash3 with
# each emitter's colours turned to the shard light's purple at (their brightest channel ^ FLASH_GAMMA) × FLASH_STRENGTH.
# The power keeps the faint parts seen: purple is about a fifth as bright as white, so a plain factor all but hid the
# translucent white shockwave (what shows when the player walks into a shard) while the bright core stayed. With these
# the core (5) gets 1.79 and the shockwave's white (1) 0.8 (a plain 0.6 gave 3.0 and 0.6; 1.34 after the power made
# the purple haze stronger than the original's). The light emitter's light is its particle's colour ×
# alpha × the light module's brightness (UE's ParticleSystemRender), so the same factor sets it and the brightness
# (2.5) stays. The materials are the original's, shared (M_ky_polarGlow02's rainbow × purple loses its green).
FLASH = MOCHI_FOLDER + "/P_WasamiShardFlash"
FLASH_SOURCE = PARTICLE_SYSTEMS[0]
FLASH_COLOR = MOCHI_PULSE_COLOR[:3]
FLASH_GAMMA = 0.5       # TODO(仮)
FLASH_STRENGTH = 0.8    # TODO(仮)
FLASH_COLOUR_MODULES = 7  # one per emitter


def _glb(path):
    """A binary glTF's JSON and BIN chunks."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"glTF":
        raise ValueError("%s is not a binary glTF" % path)
    json_length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + json_length])
    bin_offset = 20 + json_length
    bin_length = struct.unpack_from("<I", data, bin_offset)[0]
    return gltf, data[bin_offset + 8:bin_offset + 8 + bin_length]


def _extract_textures():
    """Writes the mochi's embedded pictures next to the pipeline's other intermediates and imports them. Returns
    {parameter: texture}."""
    gltf, blob = _glb(MOCHI_SOURCE)
    material = gltf["materials"][0]
    slots = dict(material.get("pbrMetallicRoughness", {}))
    slots.update({k: v for k, v in material.items() if k.endswith("Texture")})
    os.makedirs(MOCHI_EXTRACTED, exist_ok=True)
    out = {}
    for slot, param, srgb, compression, lod_group in MOCHI_TEXTURES:
        image = gltf["images"][gltf["textures"][slots[slot]["index"]]["source"]]
        view = gltf["bufferViews"][image["bufferView"]]
        start = view.get("byteOffset", 0)
        extension = {"image/jpeg": ".jpg", "image/png": ".png"}[image["mimeType"]]
        file = os.path.join(MOCHI_EXTRACTED, "T_WasamiMochi_%s%s" % (param, extension))
        with open(file, "wb") as f:
            f.write(blob[start:start + view["byteLength"]])
        tex = dd_stage.import_texture({"file": file, "asset": "%s/T_WasamiMochi_%s" % (MOCHI_FOLDER, param),
                                       "srgb": srgb, "compression": compression, "lodGroup": lod_group})
        if param == "Normal":
            tex.set_editor_property("flip_green_channel", True)
        EAL.save_loaded_asset(tex, only_if_is_dirty=False)
        out[param] = tex
    return out


def _build_mochi(mat, textures):
    """glTF's metallic-roughness material with every factor at 1 (the glb's): the base colour, metallic from B and
    roughness from G of the metallic-roughness map, the normal map, two-sided; and the base colour × (Glow + the
    purple pulse) as emissive."""
    mat.set_editor_property("two_sided", True)
    g = dd_stage._Graph(mat, checked=True)
    tcs = unreal.MaterialSamplerType
    base = g.texture("BaseColor", textures["BaseColor"], tcs.SAMPLERTYPE_COLOR, -900, -300)
    g.out(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    # The pulse: a Sine's period of 1 makes it sin(2π x).
    time = g.node(unreal.MaterialExpressionTime, -1900, -700)
    cycles = g.binary(unreal.MaterialExpressionDivide, time, "",
                      g.scalar("PulsePeriod", MOCHI_PULSE_PERIOD, -1900, -600), "", -1700, -650)
    phase = g.scalar("PulsePhase", 0.0, -1700, -450)
    phase.set_editor_property("use_custom_primitive_data", True)
    phase.set_editor_property("primitive_data_index", MOCHI_PULSE_PHASE_DATA)
    wave = dd_assets.single(g, unreal.MaterialExpressionSine,
                            dd_assets.add(g, cycles, "", phase, "", -1350, -550), "", -1200, -550)
    lifted = dd_assets.add(g, wave, "", dd_assets.constant(g, 1.0, -1200, -450), "", -1050, -550)
    level = g.multiply(lifted, "", dd_assets.constant(g, 0.5, -1050, -450), "", -900, -550)
    strength = g.multiply(g.scalar("PulseStrength", MOCHI_PULSE_STRENGTH, -1100, -750), "", level, "", -750, -650)
    pulse = g.multiply(g.vector("PulseColor", MOCHI_PULSE_COLOR, -900, -850), "RGB", strength, "", -600, -700)

    lit = dd_assets.add(g, g.scalar("Glow", MOCHI_GLOW, -900, -100), "", pulse, "", -450, -400)
    glow = g.multiply(base, "RGB", lit, "", -300, -250)
    g.out(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    packed = g.texture("MetallicRoughness", textures["MetallicRoughness"], tcs.SAMPLERTYPE_LINEAR_COLOR, -900, 100)
    g.out(packed, "B", unreal.MaterialProperty.MP_METALLIC)
    g.out(packed, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    normal = g.texture("Normal", textures["Normal"], tcs.SAMPLERTYPE_NORMAL, -900, 400)
    g.out(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)


def import_mochi():
    """The mochi's textures, master, instance and mesh (Nanite, its slot on the instance). Returns the package paths."""
    if not os.path.exists(MOCHI_SOURCE):
        raise FileNotFoundError("%s is missing (git lfs pull)" % MOCHI_SOURCE)
    textures = _extract_textures()
    master = dd_assets.material(MOCHI_MASTER, lambda mat: _build_mochi(mat, textures))
    instance = dd_assets.material_instance(MOCHI_MATERIAL, master,
                                           scalars={"Glow": MOCHI_GLOW, "PulseStrength": MOCHI_PULSE_STRENGTH,
                                                    "PulsePeriod": MOCHI_PULSE_PERIOD},
                                           vectors={"PulseColor": MOCHI_PULSE_COLOR},
                                           textures={p: t.get_path_name().split(".")[0] for p, t in textures.items()})
    mesh = dd_stage.import_mesh({"file": MOCHI_SOURCE, "asset": MOCHI_MESH, "slots": [None],
                                 "lightmapResolution": 4, "lightmapUv": 0}, nanite=True)
    mesh.set_material(0, instance)
    made = list(textures.values()) + [master, instance, mesh]
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def _build_map_mark(mat):
    """A minimap mark (M_Shard and its kind), estimated: Color as the base colour, which the map's scene capture reads,
    and as the emissive colour, the one input the cook kept (a constant)."""
    g = dd_stage._Graph(mat)
    colour = g.vector("Color", (1.0, 1.0, 1.0, 1.0), -600, 0)
    g.out(colour, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    g.out(colour, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def make_map_mark():
    """The marks' estimated master and M_Shard, an instance of it at the original's path."""
    master = dd_assets.material(MAP_MARK_MASTER, _build_map_mark)
    mark = dd_assets.material_instance(dd_assets.asset_path(SHARD_MARK), master,
                                       vectors={"Color": SHARD_MARK_COLOR})
    for asset in (master, mark):
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [master.get_path_name(), mark.get_path_name()]


def _build_flare01(mat, d):
    """M_ky_flare01_primitive, estimated. The cook kept its settings (translucent, unlit, for sprites, beam trails and
    mesh particles), its emissive colour (the particle colour's RGB), the parameters alphaDensity, depthFade, fresPower
    and fresDensity, a SubUV sample of baseTex whose RGB goes through the static mask selectCh (R by default), and the
    static switch useFresnel (A a Multiply_4, B a Multiply_1), of 15 expressions. The estimate is useFresnel's off
    side, which every instance keeps: an opacity of the selected channel × alphaDensity × the particle's alpha, faded
    into the depth over depthFade (fresPower and fresDensity belong to the side not made)."""
    dd_assets.particle_material(mat, beam_trails=True)
    g = dd_stage._Graph(mat, checked=True)
    tex = g.node(unreal.MaterialExpressionTextureSampleParameterSubUV, -1300, 0)
    tex.set_editor_property("parameter_name", "baseTex")
    tex.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(KY + "Textures/T_ky_flareVertical02")))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    channel = g.node(unreal.MaterialExpressionStaticComponentMaskParameter, -1050, 0)
    channel.set_editor_property("parameter_name", "selectCh")
    channel.set_editor_property("default_r", True)
    dd_assets.connect(tex, "RGB", channel, "")
    particle = g.node(unreal.MaterialExpressionParticleColor, -1050, 300)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)
    density = g.multiply(channel, "", g.scalar("alphaDensity", d["alphaDensity"], -1050, 150), "", -850, 50)
    alpha = g.multiply(density, "", particle, "A", -650, 100)
    dd_assets.depth_faded_opacity(g, alpha, g.scalar("depthFade", d["depthFade"], -650, 250), -450, 150)


def _build_primitive(mat, d):
    """M_ky_primitive, estimated. The cook kept its settings (translucent, unlit, responsive AA, for sprites, beam
    trails and mesh particles), eleven scalar parameters, the static switches useFresnel (B a DepthFade), useTexColor
    (the emissive colour; B a Multiply_3) and useDistanceSize (the world position offset; B a constant), the static
    bool fresnelInv, a sample of T_ky_noise6 and the functions RadialGradientExponential and Fresnel_Function, of 41
    expressions. Its one instance here (MI_ky_primitive2_trs) keeps every switch off, which is all the estimate makes:
    the particle colour's RGB as the emissive colour, and an opacity of RadialGradientExponential(radius,
    radiusDensity) × alphaValue × the particle's alpha, faded into the depth over depthFade. The on sides (fresnelPower,
    threshold, minValue, texPower, texDensity, texUVcorrect, the noise) are not made, and the world position offset is
    left unconnected."""
    dd_assets.particle_material(mat, beam_trails=True, responsive_aa=True)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -1050, 300)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)
    radius = g.scalar("radius", d["radius"], -1300, 0)
    shape = dd_assets.radial_gradient(g, radius, g.scalar("radiusDensity", d["radiusDensity"], -1300, 100), -1050, 0)
    strength = g.scalar("alphaValue", d["alphaValue"], -1050, 150)
    alpha = g.multiply(shape, "RadialGradientExponential", strength, "", -850, 50)
    faded = g.multiply(alpha, "", particle, "A", -650, 100)
    dd_assets.depth_faded_opacity(g, faded, g.scalar("depthFade", d["depthFade"], -650, 250), -450, 150)


def _build_primitive_dyn2(mat, d):
    """M_ky_primitive_dyn2, estimated. The cook kept its settings (translucent, unlit, responsive AA, for sprites, beam
    trails and mesh particles), its emissive colour (the particle colour's RGB), the parameters depthFade, outDensity,
    inDensity and inR, a DynamicParameter (dynOutDen, dynInR, dynInDen; defaults 0) and two RadialGradientExponential
    calls, of 14 expressions. The estimate: a ring of an outer gradient (the function's radius, density outDensity +
    dynOutDen) less an inner one (radius inR + dynInR, density inDensity + dynInDen), × the particle's alpha, faded
    over depthFade. The dynamic values are taken as added to the parameters: at their defaults of 0 they leave them as
    they are (multiplied, the outer density of 0 would draw nothing)."""
    dd_assets.particle_material(mat, beam_trails=True, responsive_aa=True)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -700, 400)
    g.out(particle, "RGB", MP.MP_EMISSIVE_COLOR)
    dynamic = dd_assets.dynamic_parameter(g, ("dynOutDen", "dynInR", "dynInDen", "Param4"), -1500, 150)
    out_density = dd_assets.add(g, g.scalar("outDensity", d["outDensity"], -1500, -100), "", dynamic, "dynOutDen",
                                -1250, -50)
    outer = dd_assets.radial_gradient(g, None, out_density, -1050, -50)
    in_radius = dd_assets.add(g, g.scalar("inR", d["inR"], -1500, 350), "", dynamic, "dynInR", -1250, 200)
    in_density = dd_assets.add(g, g.scalar("inDensity", d["inDensity"], -1500, 450), "", dynamic, "dynInDen",
                               -1250, 300)
    inner = dd_assets.radial_gradient(g, in_radius, in_density, -1050, 200)
    ring = g.binary(unreal.MaterialExpressionSubtract, outer, "RadialGradientExponential",
                    inner, "RadialGradientExponential", -850, 50)
    clamped = dd_assets.single(g, unreal.MaterialExpressionSaturate, ring, "", -700, 50)
    faded = g.multiply(clamped, "", particle, "A", -550, 150)
    dd_assets.depth_faded_opacity(g, faded, g.scalar("depthFade", d["depthFade"], -550, 300), -350, 200)


def _build_polar_glow(mat, d):
    """M_ky_polarGlow02, estimated. The cook kept its settings (translucent, unlit, for sprites and mesh particles), 20
    scalar parameters, a DynamicParameter (polarUV_den, baseOffsetY; defaults 0), samples of baseTex
    (T_ky_deco_rainbow) and noiseTex (T_ky_decoLinesB_sml; its RGB through the static mask noiseCh), two calls of
    MF_ky_VectorToRadialValue (the pack's copy of UE's VectorToRadialValue, whose description it carries), two
    RadialGradientExponential calls, a LinearGradient call, the static bools polarPattern and noisePolarPattern, and
    the static switches useBaseTexColor (the emissive colour: A a Multiply_5, B the particle colour's RGB), useNoise,
    useNoisePolar and useNoiseMaskAlpha (A the noise's alpha), of 64 expressions. Every static one is on by default and
    the particle system uses the material itself, so the estimate makes the on sides, with UE's VectorToRadialValue
    (the angle, and the distance from the middle × 2):
      emissive  useBaseTexColor: baseTex^texPower × texDensity × the particle's RGB, baseTex read at (angle × polarUV,
                distance × (polarUV_density + polarUV_den) + baseOffsetY + the dynamic baseOffsetY): rainbow rings
      opacity   rays: the noise's alpha read at (angle + noiseU + time × noiseXspd, distance × noisePolarUV_density +
                noiseV + time × noiseYspd), × noiseDensity, ^noisePower; × a ring,
                RadialGradientExponential(maskRadiusOut, maskRadiusOutDensity) × (1 − RadialGradientExponential(
                maskRadiusIn, maskRadiusInDensity)) (their difference is next to nothing at these values); × a fade at
                the top and the bottom, saturate((1 − |2V − 1|) × topAndUnderMask); × the particle's alpha, faded over
                depthFade
    The dynamic values are taken as added (their defaults of 0 leave the parameters as they are). noisePolarUV and
    noisePolarUV_val are not used: what they did is not known."""
    dd_assets.particle_material(mat)
    g = dd_stage._Graph(mat, checked=True)
    particle = g.node(unreal.MaterialExpressionParticleColor, -500, 600)
    dynamic = dd_assets.dynamic_parameter(g, ("polarUV_den", "baseOffsetY", "Param3", "Param4"), -2300, 300)
    polar = dd_assets.function_call(g, "Utility/VectorToRadialValue", -2300, 0, dd_assets.FUNCTIONS_02)
    angle = dd_assets.channel(g, polar, "Radial Coordinates", "R", -2050, -100)
    distance = dd_assets.channel(g, polar, "Radial Coordinates", "G", -2050, 50)

    # The rainbow.
    base_u = g.multiply(angle, "", g.scalar("polarUV", d["polarUV"], -2050, -250), "", -1800, -200)
    tiling = dd_assets.add(g, g.scalar("polarUV_density", d["polarUV_density"], -2050, 150), "", dynamic, "polarUV_den",
                           -1800, 100)
    offset = dd_assets.add(g, g.scalar("baseOffsetY", d["baseOffsetY"], -2050, 250), "", dynamic, "baseOffsetY",
                           -1800, 250)
    base_v = dd_assets.add(g, g.multiply(distance, "", tiling, "", -1600, 50), "", offset, "", -1400, 100)
    base_uv = g.binary(unreal.MaterialExpressionAppendVector, base_u, "", base_v, "", -1200, -50)
    rainbow = g.texture("baseTex", unreal.load_asset(dd_assets.asset_path(KY + "Textures/T_ky_deco_rainbow")),
                        unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -1000, -50)
    dd_assets.connect(base_uv, "", rainbow, "UVs")
    shaped = g.power(rainbow, "RGB", g.scalar("texPower", d["texPower"], -1000, 150), "", -750, -50)
    bright = g.multiply(shaped, "", g.scalar("texDensity", d["texDensity"], -750, 100), "", -550, 0)
    coloured = g.multiply(bright, "", particle, "RGB", -350, 50)
    use_base = g.switch("useBaseTexColor", coloured, "", particle, "RGB", -150, 100)
    use_base.set_editor_property("default_value", True)
    g.out(use_base, "", MP.MP_EMISSIVE_COLOR)

    # The rays.
    time = g.node(unreal.MaterialExpressionTime, -2050, 450)
    drift_u = g.multiply(time, "", g.scalar("noiseXspd", d["noiseXspd"], -2050, 550), "", -1800, 450)
    drift_v = g.multiply(time, "", g.scalar("noiseYspd", d["noiseYspd"], -2050, 650), "", -1800, 600)
    shifted_u = dd_assets.add(g, angle, "", g.scalar("noiseU", d["noiseU"], -1800, 350), "", -1600, 350)
    noise_u = dd_assets.add(g, shifted_u, "", drift_u, "", -1400, 400)
    noise_tiling = g.scalar("noisePolarUV_density", d["noisePolarUV_density"], -1800, 750)
    shifted_v = dd_assets.add(g, g.multiply(distance, "", noise_tiling, "", -1600, 650), "",
                              g.scalar("noiseV", d["noiseV"], -1600, 800), "", -1400, 700)
    noise_v = dd_assets.add(g, shifted_v, "", drift_v, "", -1200, 650)
    noise_uv = g.binary(unreal.MaterialExpressionAppendVector, noise_u, "", noise_v, "", -1000, 500)
    noise = g.texture("noiseTex", unreal.load_asset(dd_assets.asset_path(KY + "Textures/T_ky_decoLinesB_sml")),
                      unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -800, 500)
    dd_assets.connect(noise_uv, "", noise, "UVs")
    dense = g.multiply(noise, "A", g.scalar("noiseDensity", d["noiseDensity"], -800, 700), "", -600, 550)
    rays = g.power(dense, "", g.scalar("noisePower", d["noisePower"], -600, 700), "", -400, 550)

    # The ring and the top and bottom edges.
    ring_out = dd_assets.radial_gradient(
        g, g.scalar("maskRadiusOut", d["maskRadiusOut"], -1000, 900),
        g.scalar("maskRadiusOutDensity", d["maskRadiusOutDensity"], -1000, 1000), -800, 900)
    ring_in = dd_assets.radial_gradient(
        g, g.scalar("maskRadiusIn", d["maskRadiusIn"], -1000, 1100),
        g.scalar("maskRadiusInDensity", d["maskRadiusInDensity"], -1000, 1200), -800, 1100)
    hollow = dd_assets.single(g, unreal.MaterialExpressionOneMinus, ring_in, "RadialGradientExponential", -600, 1100)
    ring = g.multiply(ring_out, "RadialGradientExponential", hollow, "", -400, 950)
    linear = dd_assets.function_call(g, "Gradient/LinearGradient", -1000, 1350)
    doubled = g.multiply(linear, "VGradient", dd_assets.constant(g, 2.0, -1000, 1450), "", -800, 1350)
    centred = g.binary(unreal.MaterialExpressionSubtract, doubled, "", dd_assets.constant(g, 1.0, -800, 1450), "",
                       -650, 1350)
    folded = dd_assets.single(g, unreal.MaterialExpressionAbs, centred, "", -500, 1350)
    inside = dd_assets.single(g, unreal.MaterialExpressionOneMinus, folded, "", -400, 1350)
    steep = g.multiply(inside, "", g.scalar("topAndUnderMask", d["topAndUnderMask"], -400, 1450), "", -250, 1350)
    edges = dd_assets.single(g, unreal.MaterialExpressionSaturate, steep, "", -100, 1350)

    cover = g.multiply(g.multiply(rays, "", ring, "", -200, 700), "", edges, "", 0, 800)
    faded = g.multiply(cover, "", particle, "A", 150, 700)
    dd_assets.depth_faded_opacity(g, faded, g.scalar("depthFade", d["depthFade"], 150, 850), 350, 750)


def _build_empty(mat, d):
    """M_ky_empty, estimated. The cook kept its settings (translucent, unlit, for sprites and mesh particles) and that
    it had one expression, not what it was; a cook keeps no material's opacity input. The system's emitter 'light'
    draws it only to carry a particle light, and with nothing connected it would be a black plate (an opacity of 1),
    so the estimate is an opacity of 0."""
    dd_assets.particle_material(mat)
    g = dd_stage._Graph(mat, checked=True)
    g.out(dd_assets.constant(g, 0.0, -300, 0), "", MP.MP_OPACITY)


# (the original's material, the master holding our estimate, its builder, the original's instances of it)
FLASH_MATERIALS = (
    ("M_ky_flare01_primitive", "M_DD_KyFlare01Primitive", _build_flare01,
     ("MI_ky_flare01_primitiveG", "MI_ky_flare01_primitiveR")),
    ("M_ky_primitive", "M_DD_KyPrimitive", _build_primitive, ("MI_ky_primitive2_trs",)),
    ("M_ky_primitive_dyn2", "M_DD_KyPrimitiveDyn2", _build_primitive_dyn2, ()),
    ("M_ky_polarGlow02", "M_DD_KyPolarGlow02", _build_polar_glow, ()),
    ("M_ky_empty", "M_DD_KyEmpty", _build_empty, ()),
)


def _purple(raw):
    """A colour distribution's lookup table (RGB triples) turned purple: each colour → FLASH_COLOR × (its brightest
    channel ^ FLASH_GAMMA) × FLASH_STRENGTH, with the ranges the table now spans (the minimum and maximum of each channel, and of
    them all, as the cook saved them)."""
    table = raw["Table"]
    values = table["Values"]
    if raw.get("Distribution") or table.get("EntryStride", 0) % 3 or len(values) % 3:
        raise ValueError("not a baked table of colours: %r" % (raw,))
    out = []
    for i in range(0, len(values), 3):
        peak = max(max(values[i:i + 3]), 0.0) ** FLASH_GAMMA * FLASH_STRENGTH
        out.extend(c * peak for c in FLASH_COLOR)
    table["Values"] = out
    raw["MinValueVec"] = [min(out[k::3]) for k in range(3)]
    raw["MaxValueVec"] = [max(out[k::3]) for k in range(3)]
    raw["MinValue"] = min(raw["MinValueVec"])
    raw["MaxValue"] = max(raw["MaxValueVec"])


def _purple_flash(exports):
    """dd_particles' adjust for P_WasamiShardFlash: every colour over life of P_ky_flash3 turned purple."""
    colours = [e for e in exports.values() if e["class"] == "ParticleModuleColorOverLife"]
    others = {e["class"] for e in exports.values() if e["class"].startswith("ParticleModuleColor")}
    if len(colours) != FLASH_COLOUR_MODULES or others != {"ParticleModuleColorOverLife"}:
        raise RuntimeError("P_ky_flash3's colours are not the ones known: %d, %s" % (len(colours), sorted(others)))
    for e in colours:
        _purple(e["props"]["ColorOverLife"])


def make_flash():
    """P_WasamiShardFlash (after P_ky_flash3's materials). Returns the package path."""
    version, rel = FLASH_SOURCE
    return dd_particles.particle_system(rel, version, FLASH, _purple_flash)


def make_flash_materials():
    """The flash's materials (dd_assets.estimated_materials). Returns the package paths."""
    return [a.get_path_name() for a in dd_assets.estimated_materials(KY + "Materials/", FLASH_MATERIALS, 2)]


def import_all():
    """Imports and builds everything the shards use, then saves /Game/DD, /Game/Pipeline and /Game/Wasami. Returns how
    many of each kind."""
    result = {"sounds": len([dd_assets.sound(rel, version) for version, rel in SOUNDS])}
    result["sound_concurrencies"] = len([dd_assets.sound_concurrency(rel, version) for version, rel in SOUND_CONCURRENCIES])
    result["sound_cues"] = len([dd_assets.sound_cue(rel, version) for version, rel in SOUND_CUES])
    result["camera_shakes"] = len([dd_assets.camera_shake(rel, version) for version, rel in CAMERA_SHAKES])
    result["materials"] = len(make_map_mark())
    result["mochi"] = len(import_mochi())
    result["flash_textures"] = len([dd_assets.texture(rel, version) for version, rel in FLASH_TEXTURES])
    result["flash_materials"] = len(make_flash_materials())
    systems = [dd_particles.particle_system(rel, version) for version, rel in PARTICLE_SYSTEMS] + [make_flash()]
    result["particle_systems"] = len(systems)
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT, paths.WASAMI_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
