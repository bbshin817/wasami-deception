"""Dark Deception's tablet powers: the sounds, camera shakes, camera anims, textures and materials the power system
(UWasamiPowerComponent) and the player's FX (UWasamiChameleonComponent) use. The icons on the tablet's sockets are the
tablet's own (dd_tablet).

  M_Speedlines                  the original's graph, node for node (FlipBook at its defaults → T_Speedlines)
  M_DD_ChameleonCameraShake     the Chameleon pack's M_CameraShake, estimated (its graph is cooked away)

Sources: pak_reference_2 (UE 4.24, the latest version), which the powers follow except the teleport (pak_reference).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

# (pak_reference version, the original's path under /Game).
SOUNDS = (
    (2, "Audio/UI/power_refilled"),             # a power is ready again
    (2, "Audio/UI/Shard_Streak_Milestone_V5"),  # the speed boost starts
)
CAMERA_SHAKES = (
    (2, "UI/Menu/Streaks/BP_CameraShake_Streak"),  # the speed boost starts
)
CAMERA_ANIMS = (
    (2, "Animation/Camera/CameraAnim_SpeedBoost"),  # the view turns red while the speed boost lasts
)
TEXTURES = (
    (2, "UI/Main/Powers/T_Speedlines"),    # UMG_SpeedBoost's lines (a 2 × 5 sheet; M_Speedlines reads it as 2 × 2)
    (2, "UI/Menu/Streaks/T_VignetteNew"),  # UMG_SpeedBoost's vignette
)

SPEEDLINES = "UI/Main/Powers/M_Speedlines"
FLIPBOOK = "/Engine/Functions/Engine_MaterialFunctions02/Texturing/FlipBook"
# The FlipBook call's output M_Speedlines takes its UVs from (its expression input's OutputIndex).
FLIPBOOK_UV_OUTPUT = 2
CAMERA_SHAKE_MASTER = "/Game/Pipeline/Materials/M_DD_ChameleonCameraShake"


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


def make_materials():
    speedlines = dd_assets.material(dd_assets.asset_path(SPEEDLINES), _build_speedlines,
                                    domain=unreal.MaterialDomain.MD_UI, blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
    shake = dd_assets.material(CAMERA_SHAKE_MASTER, _build_camera_shake, domain=unreal.MaterialDomain.MD_POST_PROCESS)
    return [speedlines.get_path_name(), shake.get_path_name()]


def import_all():
    """Imports and builds every asset the powers use, then saves /Game/DD and /Game/Pipeline. Returns how many of each
    kind."""
    result = {"sounds": len([dd_assets.sound(rel, version) for version, rel in SOUNDS])}
    result["camera_shakes"] = len([dd_assets.camera_shake(rel, version) for version, rel in CAMERA_SHAKES])
    result["camera_anims"] = len([dd_assets.camera_anim(rel, version) for version, rel in CAMERA_ANIMS])
    result["textures"] = len([dd_assets.texture(rel, version) for version, rel in TEXTURES])
    result["materials"] = len(make_materials())
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
