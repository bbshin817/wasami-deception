"""Dark Deception (the original, pak_reference / pak_reference_2): its assets rebuilt from the exported data under
/Game/DD/<the original's path under /Game> — Blueprints of its classes from their class defaults, sounds with the
SoundWave's own settings, SoundCues with their node trees, textures with the original's texture settings, and
CameraAnims as UWasamiCameraAnim — and the master materials we write ourselves (materials whose graphs are cooked
away)."""
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
    # A Blueprint class copies only the defaults its last compile saw differ from its parent's into new instances
    # (UBlueprintGeneratedClass' custom property list); a compile after the writes makes the shakes carry them in this
    # session too (a load from disk would rebuild the list anyway).
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
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


# SoundCue properties the builder sets itself (the tree) or UE works out from it, and the sound classes, which are not
# made yet (as for the waves).
SOUND_CUE_SKIP = ("FirstNode", "SoundClassObject", "Duration", "MaxDistance")


def sound_cue(rel, version=1):
    """A SoundCue with the original's node tree ('Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue'): each node of the export
    made with UWasamiSoundCueLibrary, its numbers written by name and its inputs linked, the wave players pointing at the
    waves under /Game/DD (which have to be made first). Returns the package path."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        cue = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        cue = _tools().create_asset(name, folder, unreal.SoundCue, unreal.SoundCueFactoryNew())
    pkg = export_json(rel, version)
    exports = {("%s.%s" % (e["outer"], e["name"])) if e.get("outer") else e["name"]: e for e in pkg["exports"]}
    lib = unreal.WasamiSoundCueLibrary
    lib.reset_sound_cue(cue)
    made = {}

    def node(key):
        if key in made:
            return made[key]
        e = exports[key]
        obj = lib.add_sound_node(cue, e["class"])
        if obj is None:
            raise RuntimeError("%s: could not make %s (%s)" % (rel, key, e["class"]))
        made[key] = obj
        props = dict(e["props"])
        wave = props.pop("SoundWaveAssetPtr", None)
        if wave is not None:
            loaded = unreal.load_asset(asset_path(game_rel(wave)))
            if not isinstance(loaded, unreal.SoundWave) or not lib.set_wave(obj, loaded):
                raise RuntimeError("%s: %s plays %s, which is not made yet" % (rel, key, wave))
        children = [node(child) for child in props.pop("ChildNodes", [])]
        for name, value in props.items():
            if isinstance(value, bool) or not isinstance(value, (int, float)):
                raise ValueError("%s: %s.%s has a value this does not write (%r)" % (rel, key, name, value))
            error = unreal.WasamiCascadeLibrary.set_property_text(obj, name, repr(value))
            if error is None or error:
                raise RuntimeError("%s: %s.%s was not written: %s" % (rel, key, name, error))
        error = lib.set_child_nodes(obj, children)
        if error is None or error:
            raise RuntimeError("%s: the inputs of %s were not linked: %s" % (rel, key, error))
        return obj

    props = main_export(pkg, rel)["props"]
    lib.finish_sound_cue(cue, node(props["FirstNode"]))
    failures = ue_props.apply(cue, props, skip=SOUND_CUE_SKIP)
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    EAL.save_asset(target, only_if_is_dirty=False)
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


def static_mesh(rel, version=1):
    """Imports the original's static mesh /Game/<rel> under /Game/DD from its glTF (_meshes.json), without Nanite (a
    particle's mesh, drawn with translucent materials), with the StaticMesh's lightmap settings (UStaticMesh's defaults,
    4 texels on UV 0, where the export has none). Its slots keep the original's engine materials where this engine has
    them (a mesh emitter that overrides the material never draws them). Returns the package path."""
    with open(os.path.join(pak(version), "_meshes.json"), encoding="utf-8") as f:
        entry = json.load(f).get("/Game/" + rel)
    if entry is None or entry["class"] != "StaticMesh":
        raise KeyError("no static mesh /Game/%s in %s/_meshes.json" % (rel, pak(version)))
    gltf = os.path.join(pak(version), *entry["gltf"].split("/"))
    if not os.path.exists(gltf):
        raise FileNotFoundError(gltf)
    props = main_export(export_json(rel, version), rel)["props"]
    target = asset_path(rel)
    slots = entry["material_slots"]
    mesh = dd_stage.import_mesh({"file": gltf, "asset": target, "slots": [s["slot"] for s in slots],
                                 "lightmapResolution": props.get("LightMapResolution", 4),
                                 "lightmapUv": props.get("LightMapCoordinateIndex", 0)}, nanite=False)
    for index, slot in enumerate(slots):
        material = slot["material"] or ""
        if material.startswith(ENGINE_REL) and index < len(mesh.get_editor_property("static_materials")):
            engine_material = unreal.load_asset(material.split(".")[0])
            if engine_material is not None:
                mesh.set_material(index, engine_material)
    EAL.save_loaded_asset(mesh, only_if_is_dirty=False)
    return target


def material(asset_path, build, domain=None, blend_mode=None):
    """Loads or creates a material, clears its graph, sets its domain and blend mode, and has build(mat) make the graph.
    Returns the material, recompiled."""
    if EAL.does_asset_exist(asset_path):
        mat = unreal.load_asset(asset_path)
        dd_stage.clear_expressions(mat)
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


def material_instance(asset_path, parent, scalars=None, vectors=None, textures=None, static_masks=None):
    """Loads or creates a MaterialInstanceConstant of parent and sets its parameters ({name: value}; a vector is 4
    numbers, a texture an asset path, a static component mask the channels it keeps, 'G'); the parameters it had before
    (static ones too) are cleared first. Returns the instance."""
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
    # Python has no setter for a static component mask (UWasamiMaterialLibrary); the update rebuilds the permutation.
    for key, channels in (static_masks or {}).items():
        unreal.WasamiMaterialLibrary.set_static_component_mask(mic, key, *[c in channels for c in "RGBA"])
    MEL.update_material_instance(mic)
    return mic


# The engine's material function libraries.
FUNCTIONS_01 = "/Engine/Functions/Engine_MaterialFunctions01/"
FUNCTIONS_02 = "/Engine/Functions/Engine_MaterialFunctions02/"
# Parameters an instance's export lists that are the engine's own (every material has RefractionDepthBias; the
# original's particle and UI materials do not use it).
ENGINE_PARAMETERS = ("RefractionDepthBias",)


def connect(a, a_pin, b, b_pin):
    """Connects a's output a_pin to b's input b_pin, or raises (MaterialEditingLibrary only returns False)."""
    if not MEL.connect_material_expressions(a, a_pin, b, b_pin):
        raise RuntimeError("could not connect %s.%s to %s.%s" % (a.get_name(), a_pin, b.get_name(), b_pin))


def function_call(g, name, x, y, library=FUNCTIONS_01):
    """A call of the engine's material function library + name ('Gradient/RadialGradientExponential') in graph g."""
    call = g.node(unreal.MaterialExpressionMaterialFunctionCall, x, y)
    call.set_editor_property("material_function", unreal.load_asset(library + name))
    return call


def constant(g, value, x, y):
    e = g.node(unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", value)
    return e


def parameter_defaults(rel, version):
    """A material's scalar and vector parameter defaults from its export ({name: value}, {name: [r, g, b, a]}); a
    default the export leaves out is the engine's (0, or black)."""
    scalars, vectors = {}, {}
    for e in export_json(rel, version)["exports"]:
        p = e["props"]
        if e["class"] == "MaterialExpressionScalarParameter":
            scalars[p["ParameterName"]] = p.get("DefaultValue", 0.0)
        elif e["class"] == "MaterialExpressionVectorParameter":
            vectors[p["ParameterName"]] = p.get("DefaultValue", [0.0, 0.0, 0.0, 1.0])
    return scalars, vectors


def instance_parameters(rel, version):
    """A material instance's own values from its export: ({scalar: value}, {vector: [r, g, b, a]}, {texture: our asset
    path}, {static component mask: the channels it keeps, 'G'}), without the engine's parameters (ENGINE_PARAMETERS).
    Static parameters of other kinds raise (nothing writes them yet)."""
    props = main_export(export_json(rel, version), rel)["props"]

    def own(key):
        return [v for v in props.get(key, []) if v["ParameterInfo"]["Name"] not in ENGINE_PARAMETERS]

    def colour(value):
        return [value[c] for c in "RGBA"] if isinstance(value, dict) else list(value)

    scalars = {v["ParameterInfo"]["Name"]: v["ParameterValue"] for v in own("ScalarParameterValues")}
    vectors = {v["ParameterInfo"]["Name"]: colour(v["ParameterValue"]) for v in own("VectorParameterValues")}
    textures = {v["ParameterInfo"]["Name"]: asset_path(game_rel(v["ParameterValue"])) for v in own("TextureParameterValues")}
    static = dict(props.get("StaticParameters") or {})
    masks = {p["ParameterInfo"]["Name"]: "".join(c for c in "RGBA" if p.get(c))
             for p in static.pop("StaticComponentMaskParameters", []) if p.get("bOverride")}
    if any(static.values()):
        raise NotImplementedError("%s has static parameters %s" % (rel, sorted(k for k, v in static.items() if v)))
    return scalars, vectors, textures, masks


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
