"""Dark Deception (the original, pak_reference / pak_reference_2): its assets rebuilt from the exported data under
/Game/DD/<the original's path under /Game> — Blueprints of its classes from their class defaults, sounds with the
SoundWave's own settings, textures with the original's texture settings, and CameraAnims as UWasamiCameraAnim — and the
master materials we write ourselves (materials whose graphs are cooked away)."""
import json
import os

import unreal

from wasami_tools.pipeline import dd_stage, paths, ue_props

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

# SoundWave properties of the export that are written onto the imported wave, with UE's default for when the export
# leaves them out (it keeps only what differs from the defaults). Channels, rate and duration come with the file; the
# original's sound classes are not made (yet), so SoundClassObject is not written.
SOUND_DEFAULTS = {"Volume": 1.0, "Pitch": 1.0}
SOUND_FLAGS = {"bLooping": False}

# The engine's own content the original uses ('/Engine/VREditor/Sounds/UI/Teleport_Committed') is exported under
# Engine/Content and lives under /Game/DD/_Engine here (UE 5.8's copies are not known to be the same).
ENGINE_REL = "/Engine/"
ENGINE_FOLDER = "_Engine"

# UCameraAnim's defaults for what its export leaves out (UE4's UCameraAnim constructor; the blend weight is zeroed).
CAMERA_ANIM_DEFAULTS = {"AnimLength": 3.0, "BaseFOV": 90.0, "BasePostProcessBlendWeight": 0.0}
# UE4's legacy tonemapper settings, which UE 5 dropped; the CameraAnims only switch them on at their neutral defaults.
UE4_ONLY_POSTPROCESS = ("bOverride_FilmWhitePoint",)


def pak(version):
    """The export root of pak_reference (1, UE 4.21) or pak_reference_2 (2, UE 4.24)."""
    return paths.DD_PAK if version == 1 else paths.DD_PAK2


def _content(rel):
    """The project folder of the export ('DDeception' or 'Engine') and the path under its Content: a rel is the
    original's /Game/<rel>, or an engine asset's whole path ('/Engine/VREditor/...')."""
    if rel.startswith(ENGINE_REL):
        return "Engine", rel[len(ENGINE_REL):]
    return "DDeception", rel


def content_file(rel, version, extension):
    """The exported file of /Game/<rel> (or an engine asset) with that extension ('.ogg', '.png')."""
    project, sub = _content(rel)
    return os.path.join(pak(version), project, "Content", *sub.split("/")) + extension


def asset_path(rel):
    """The original's /Game/<rel> under our /Game/DD (an engine asset under /Game/DD/_Engine)."""
    project, sub = _content(rel)
    return paths.DD_ROOT + "/" + (sub if project == "DDeception" else ENGINE_FOLDER + "/" + sub)


def export_json(rel, version=1):
    """The exported package of the original's /Game/<rel> ('Blueprints/Main/BP_DD_PlayerCharacter_WalkShake'), or of an
    engine asset ('/Engine/VREditor/Sounds/UI/Teleport_Committed')."""
    project, sub = _content(rel)
    path = os.path.join(pak(version), "_assets", project, "Content", *sub.split("/")) + ".json"
    if not os.path.exists(path):
        raise FileNotFoundError("no export of %s in %s" % (rel, pak(version)))
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def class_defaults(pkg):
    return next(e for e in pkg["exports"] if e["name"].startswith("Default__"))["props"]


def main_export(pkg, rel):
    """The package's asset itself (the export named like the package)."""
    name = rel.rsplit("/", 1)[-1]
    return next(e for e in pkg["exports"] if e["name"] == name)


def game_rel(object_path):
    """'/Game/Audio/NewSoundConcurrency.NewSoundConcurrency' → 'Audio/NewSoundConcurrency'."""
    package = object_path.split(".", 1)[0]
    if not package.startswith("/Game/"):
        raise ValueError("not an asset of the original's /Game: %s" % object_path)
    return package[len("/Game/"):]


def _tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def camera_shake(rel, version=1):
    """A LegacyCameraShake Blueprint (UE4's UCameraShake, as the original's) with the original's defaults. Returns its
    package path; an existing one gets its defaults written again."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        bp = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.LegacyCameraShake)
        bp = _tools().create_asset(name, folder, unreal.Blueprint, factory)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    failures = ue_props.apply(cdo, class_defaults(export_json(rel, version)), skip=("UberGraphFrame",))
    for f in failures:
        unreal.log_warning("camera_shake %s: %s" % (rel, f))
    if failures:
        raise RuntimeError("%d defaults of %s could not be set (see the log)" % (len(failures), rel))
    EAL.save_asset(target, only_if_is_dirty=False)
    return target


def sound_concurrency(rel, version=1):
    """A SoundConcurrency asset with the original's settings ('Audio/NewSoundConcurrency'). Returns the asset."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        asset = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        asset = _tools().create_asset(name, folder, unreal.SoundConcurrency, unreal.SoundConcurrencyFactory())
    pkg = export_json(rel, version)
    failures = ue_props.apply(asset, main_export(pkg, rel)["props"])
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    return asset


def sound(rel, version=1):
    """Imports the original's /Game/<rel>.ogg (or an engine sound's) as a SoundWave under /Game/DD and writes the
    export's Volume, Pitch, looping and ConcurrencySet onto it (making the concurrency assets it names). Returns the
    package path."""
    ogg = content_file(rel, version, ".ogg")
    if not os.path.exists(ogg):
        raise FileNotFoundError(ogg)
    target = asset_path(rel)
    folder, name = paths.split(target)
    task = unreal.AssetImportTask()
    task.filename = ogg
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    task.factory = unreal.SoundFactory()
    _tools().import_asset_tasks([task])
    wave = unreal.load_asset(target)
    if wave is None:
        raise RuntimeError("the sound did not import to %s" % target)

    props = main_export(export_json(rel, version), rel)["props"]
    for key, default in SOUND_DEFAULTS.items():
        wave.set_editor_property(ue_props.snake(key), float(props.get(key, default)))
    for key, default in SOUND_FLAGS.items():
        wave.set_editor_property(ue_props.snake(key), bool(props.get(key, default)))
    concurrency = [sound_concurrency(game_rel(p), version) for p in props.get("ConcurrencySet", [])]
    wave.set_editor_property("concurrency_set", concurrency)
    return target


def texture(rel, version=1):
    """Imports the original's /Game/<rel>.png under /Game/DD with its sRGB, compression and LOD group (_textures.json).
    Returns the package path."""
    with open(os.path.join(pak(version), "_textures.json"), encoding="utf-8") as f:
        table = json.load(f)
    key = "DDeception/Content/%s.uasset" % rel
    entry = table.get(key)
    if entry is None:
        raise KeyError("no texture %s in %s/_textures.json" % (key, pak(version)))
    png = content_file(rel, version, ".png")
    if not os.path.exists(png):
        raise FileNotFoundError(png)
    target = asset_path(rel)
    dd_stage.import_texture({"file": png, "asset": target, "srgb": entry["srgb"], "compression": entry["compression"],
                             "lodGroup": entry["lod_group"]})
    return target


def material(asset_path, build, domain=None, blend_mode=None):
    """Loads or creates a material, clears its graph, sets its domain and blend mode, and has build(mat) make the graph.
    Returns the material, recompiled."""
    if EAL.does_asset_exist(asset_path):
        mat = unreal.load_asset(asset_path)
        MEL.delete_all_material_expressions(mat)
    else:
        folder, name = paths.split(asset_path)
        mat = _tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    # The blend mode first: each change recompiles, and a decal domain over an opaque blend fails to compile.
    if blend_mode is not None:
        mat.set_editor_property("blend_mode", blend_mode)
    if domain is not None:
        mat.set_editor_property("material_domain", domain)
    build(mat)
    MEL.recompile_material(mat)
    return mat


def material_instance(asset_path, parent, scalars=None, vectors=None, textures=None):
    """Loads or creates a MaterialInstanceConstant of parent and sets its parameters ({name: value}; a vector is 4
    numbers, a texture an asset path); the parameters it had before are cleared first. Returns the instance."""
    if EAL.does_asset_exist(asset_path):
        mic = unreal.load_asset(asset_path)
    else:
        folder, name = paths.split(asset_path)
        mic = _tools().create_asset(name, folder, unreal.MaterialInstanceConstant,
                                    unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mic, parent)
    MEL.clear_all_material_instance_parameters(mic)
    for key, value in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mic, key, float(value))
    for key, value in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mic, key, unreal.LinearColor(*[float(v) for v in value]))
    for key, value in (textures or {}).items():
        MEL.set_material_instance_texture_parameter_value(mic, key, unreal.load_asset(value))
    MEL.update_material_instance(mic)
    return mic


def _curve_points(points, point_cls, convert):
    """The export's Matinee curve points ({InVal, OutVal, ArriveTangent, LeaveTangent, InterpMode}) as Python structs;
    convert makes a value of the curve's type out of an exported one."""
    out = []
    for p in points:
        point = point_cls()
        point.set_editor_property("val", float(p["InVal"]))  # InVal: Python strips its 'In'
        point.set_editor_property("out_val", convert(p["OutVal"]))
        point.set_editor_property("arrive_tangent", convert(p.get("ArriveTangent", 0.0)))
        point.set_editor_property("leave_tangent", convert(p.get("LeaveTangent", 0.0)))
        point.set_editor_property("interp_mode", ue_props.enum_member(unreal.InterpCurveMode, p.get("InterpMode", "CIM_Linear")))
        out.append(point)
    return out


def _linear_color(value):
    if not isinstance(value, (list, tuple)):
        value = [value] * 4
    return unreal.LinearColor(*[float(v) for v in value])


def camera_anim(rel, version=1):
    """The original CameraAnim /Game/<rel> as a UWasamiCameraAnim under /Game/DD: its length, base FOV, base
    post-process settings and weight, and its float and colour property tracks with their keys and tangents as saved.
    The Move track is left out (the CameraAnims the powers play keep it at the origin). Returns the package path."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        anim = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.WasamiCameraAnim)
        anim = _tools().create_asset(name, folder, unreal.WasamiCameraAnim, factory)
    pkg = export_json(rel, version)
    props = main_export(pkg, rel)["props"]
    anim.set_editor_property("anim_length", float(props.get("AnimLength", CAMERA_ANIM_DEFAULTS["AnimLength"])))
    anim.set_editor_property("base_fov", float(props.get("BaseFOV", CAMERA_ANIM_DEFAULTS["BaseFOV"])))
    anim.set_editor_property("base_post_process_blend_weight",
                             float(props.get("BasePostProcessBlendWeight", CAMERA_ANIM_DEFAULTS["BasePostProcessBlendWeight"])))
    settings = unreal.PostProcessSettings()
    failures = ue_props.apply(settings, props.get("BasePostProcessSettings", {}), skip=UE4_ONLY_POSTPROCESS)
    if failures:
        raise RuntimeError("base post-process settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    anim.set_editor_property("base_post_process_settings", settings)

    float_tracks, color_tracks = [], []
    for e in pkg["exports"]:
        p = e["props"]
        if e["class"] == "InterpTrackFloatProp":
            curve = unreal.InterpCurveFloat()
            curve.set_editor_property("points", _curve_points(p["FloatTrack"]["Points"], unreal.InterpCurvePointFloat, float))
            track = unreal.WasamiCameraAnimFloatTrack()
            track.set_editor_property("property_name", p["PropertyName"])
            track.set_editor_property("curve", curve)
            float_tracks.append(track)
        elif e["class"] == "InterpTrackLinearColorProp":
            curve = unreal.InterpCurveLinearColor()
            curve.set_editor_property("points", _curve_points(p["LinearColorTrack"]["Points"],
                                                              unreal.InterpCurvePointLinearColor, _linear_color))
            track = unreal.WasamiCameraAnimColorTrack()
            track.set_editor_property("property_name", p["PropertyName"])
            track.set_editor_property("curve", curve)
            color_tracks.append(track)
        elif e["class"].startswith("InterpTrack") and e["class"] != "InterpTrackMove":
            raise RuntimeError("%s has a %s track, which UWasamiCameraAnim does not hold" % (rel, e["class"]))
    anim.set_editor_property("float_tracks", float_tracks)
    anim.set_editor_property("color_tracks", color_tracks)
    EAL.save_asset(target, only_if_is_dirty=False)
    return target
