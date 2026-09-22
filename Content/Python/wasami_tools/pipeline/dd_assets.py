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
# sound class (SoundClassObject) is written by _write_sound_class.
SOUND_DEFAULTS = {"Volume": 1.0, "Pitch": 1.0}
SOUND_FLAGS = {"bLooping": False}
# What the export names only when it is not UE's default, so it is written only when it is there: the enemy's move loop
# is PlayWhenSilent (it starts at volume 0 and rises, and a restarting virtualization would never let it be heard).
SOUND_ENUMS = ("VirtualizationMode",)

# The engine's own content the original uses ('/Engine/VREditor/Sounds/UI/Teleport_Committed') is exported under
# Engine/Content and lives under /Game/DD/_Engine here (UE 5.8's copies are not known to be the same).
ENGINE_REL = "/Engine/"
ENGINE_FOLDER = "_Engine"

# The fallback face every Font the original made carries. UE 5.8's copy is the original's file (UE 4.24's
# Engine/Content/EngineFonts/Faces/DroidSansFallback.ttf), so the engine's own asset is used, not an import.
FALLBACK_FACE = "/Engine/EngineFonts/Faces/DroidSansFallback.DroidSansFallback"

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
    """The package's asset itself: the export named like the package, or its only export without an outer where none is
    (Textures/FX_Textures/dust holds dust_0)."""
    name = rel.rsplit("/", 1)[-1]
    named = [e for e in pkg["exports"] if e["name"] == name]
    if not named:
        named = [e for e in pkg["exports"] if not e.get("outer")]
        if len(named) != 1:
            raise KeyError("%s: no export named %s, and %d without an outer" % (rel, name, len(named)))
    return named[0]


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


def reverb_effect(rel, version=1):
    """A ReverbEffect with the original's settings ('/Engine/EngineSounds/ReverbSettings/BunkerHall'), saved; the
    presets the hospital's AudioVolumes name are the engine's, so they are rebuilt under /Game/DD/_Engine from the
    export as the rest of the engine's content is. Returns the asset."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        asset = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        asset = _tools().create_asset(name, folder, unreal.ReverbEffect, unreal.ReverbEffectFactory())
    failures = ue_props.apply(asset, main_export(export_json(rel, version), rel)["props"])
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    EAL.save_asset(target, only_if_is_dirty=False)
    return asset


def sound_attenuation(rel, version=1):
    """A SoundAttenuation asset with the original's settings ('Audio/01_Hotel/01_Lobby_Attenuation'); UE 5.8's defaults
    for what the export leaves out are UE 4.24's (FSoundAttenuationSettings, FBaseAttenuationSettings). Returns the
    asset."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        asset = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        asset = _tools().create_asset(name, folder, unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
    failures = ue_props.apply(asset, main_export(export_json(rel, version), rel)["props"])
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    EAL.save_asset(target, only_if_is_dirty=False)
    return asset


def _import_wave(file, target):
    """Imports the sound file at file as the SoundWave target (the import task does not save, and a wave left unsaved
    is gone when the editor opens again, so every caller saves). Returns the wave."""
    if not os.path.exists(file):
        raise FileNotFoundError(file)
    folder, name = paths.split(target)
    task = unreal.AssetImportTask()
    task.filename = file
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
    return wave


def _write_subtitles(wave, subtitles):
    """Puts [(time in seconds, text)] on the wave as UE's own Subtitles, which the options' SUBTITLES turns on and off.
    None leaves what the wave came with."""
    if subtitles is None:
        return
    wave.set_editor_property("subtitles", [unreal.SubtitleCue(text=unreal.Text(text), time=float(time))
                                           for time, text in subtitles])


def sound(rel, version=1, subtitles=None):
    """Imports the original's /Game/<rel>.ogg (or an engine sound's) as a SoundWave under /Game/DD and writes the
    export's Volume, Pitch, looping and ConcurrencySet onto it (making the concurrency assets it names), saved.
    Returns the package path.

    subtitles: [(time in seconds, text)] to show while it plays; the dialogue passes the original's lines, which its
    waves do not carry themselves (dd_dialogue)."""
    target = asset_path(rel)
    wave = _import_wave(content_file(rel, version, ".ogg"), target)
    props = main_export(export_json(rel, version), rel)["props"]
    for key, default in SOUND_DEFAULTS.items():
        wave.set_editor_property(ue_props.snake(key), float(props.get(key, default)))
    for key, default in SOUND_FLAGS.items():
        wave.set_editor_property(ue_props.snake(key), bool(props.get(key, default)))
    for key in SOUND_ENUMS:
        if key in props:
            wave.set_editor_property(ue_props.snake(key), ue_props.value(props[key], wave.get_editor_property(ue_props.snake(key))))
    concurrency = [sound_concurrency(game_rel(p), version) for p in props.get("ConcurrencySet", [])]
    wave.set_editor_property("concurrency_set", concurrency)
    _write_subtitles(wave, subtitles)
    _write_sound_class(wave, props)
    EAL.save_asset(target, only_if_is_dirty=False)
    return target


def sound_file(file, target, subtitles=None, sound_class=None):
    """Imports this game's own sound file (a wav under SourceArt) as the SoundWave target, at UE's defaults (volume 1,
    no looping), saved. Returns the package path.

    subtitles: as in sound; sound_class: the SoundClass to play it through (none for the project's default)."""
    wave = _import_wave(file, target)
    _write_subtitles(wave, subtitles)
    wave.set_editor_property("sound_class_object", sound_class)
    EAL.save_asset(target, only_if_is_dirty=False)
    return target


# The original's sound mix (BP_DD_GameMode's SetBaseSoundMix; the options' volumes are its class overrides) and the
# sound classes it adjusts, which the sounds name as their SoundClassObject.
SOUND_MIX = "Audio/SoundMix/DD_SoundMix"


def _sound_class_asset(rel, version):
    """The SoundClass /Game/DD/<rel> with the export's Properties (made when missing), and the export's props."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        asset = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        asset = _tools().create_asset(name, folder, unreal.SoundClass, unreal.SoundClassFactory())
    props = main_export(export_json(rel, version), rel)["props"]
    # ParentClass is read-only to Python; adding the class to its parent's ChildClasses sets it (_sound_class_tree).
    failures = ue_props.apply(asset, props, skip=("ParentClass", "ChildClasses"))
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    return asset, props


def _sound_class_tree(rel, version):
    """The SoundClass /Game/DD/<rel> and the ChildClasses under it, as the exports have them (saved)."""
    asset, props = _sound_class_asset(rel, version)
    for child_path in props.get("ChildClasses", []):
        child = _sound_class_tree(game_rel(child_path), version)
        children = list(asset.get_editor_property("child_classes"))
        if child not in children:
            # One at a time: USoundClass::PostEditChangeProperty makes the first new entry's ParentClass this class and
            # stops there.
            asset.set_editor_property("child_classes", children + [child])
        if child.get_editor_property("parent_class") != asset:
            raise RuntimeError("%s did not become the parent of %s" % (rel, child_path))
    EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


# The class trees made since this module was loaded (the toolsets load it again for each call), by root and version.
_made_class_trees = set()


def sound_class(rel, version=1):
    """The SoundClass of the original's /Game/<rel> ('Audio/SoundMix/DD_SoundClass_SFX_UI') with the export's
    Properties, made with the whole tree it is in (from the root through the exports' ParentClass, then each
    ChildClasses) and saved. A parent in the engine's content is left out: the latest version hangs Music under the
    engine's Master for its master volume, which the old version's options (the ones this game has) do not have, and
    /Engine cannot be saved. Returns the asset."""
    root = rel
    while True:
        parent = main_export(export_json(root, version), root)["props"].get("ParentClass")
        if not parent or parent.startswith(ENGINE_REL):
            break
        root = game_rel(parent)
    if (root, version) not in _made_class_trees:
        _sound_class_tree(root, version)
        _made_class_trees.add((root, version))
    return unreal.load_asset(asset_path(rel))


def sound_mix(rel=SOUND_MIX, version=1):
    """The original's SoundMix ('Audio/SoundMix/DD_SoundMix') with its SoundClassEffects (the sound classes they adjust
    made first) and settings, saved. Returns the asset."""
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        mix = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        mix = _tools().create_asset(name, folder, unreal.SoundMix, unreal.SoundMixFactory())
    props = main_export(export_json(rel, version), rel)["props"]
    effects = []
    failures = []
    for entry in props.get("SoundClassEffects", []):
        adjuster = unreal.SoundClassAdjuster()
        adjuster.set_editor_property("sound_class_object", sound_class(game_rel(entry["SoundClassObject"]), version))
        ue_props.apply(adjuster, entry, skip=("SoundClassObject",), failures=failures)
        effects.append(adjuster)
    mix.set_editor_property("sound_class_effects", effects)
    ue_props.apply(mix, props, skip=("SoundClassEffects",), failures=failures)
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    EAL.save_loaded_asset(mix, only_if_is_dirty=False)
    return mix


def _write_sound_class(sound, props):
    """Sets the sound's SoundClassObject to the export's (made when missing), or to none where the export has none (the
    project's default class, as in the original). The classes are the old version's (the options this game has are
    its; the latest's differ only in Music's parent, see sound_class). Returns whether it changed."""
    path = props.get("SoundClassObject")
    cls = sound_class(game_rel(path), 1) if path else None
    if sound.get_editor_property("sound_class_object") == cls:
        return False
    sound.set_editor_property("sound_class_object", cls)
    return True


def _sound_rel(package_path):
    """'/Game/DD/Audio/UI/Life_Lost' → 'Audio/UI/Life_Lost'; an engine sound's → '/Engine/VREditor/...'."""
    sub = package_path[len(paths.DD_ROOT) + 1:]
    if sub.startswith(ENGINE_FOLDER + "/"):
        return ENGINE_REL + sub[len(ENGINE_FOLDER) + 1:]
    return sub


def sound_classes():
    """Gives every SoundWave and SoundCue under /Game/DD the sound class its export names (SoundClassObject) without
    importing it again, and makes the sound mix. A sound in both versions of the original takes the latest's (only
    Audio/UI/Pause_Sound_v1 differs: SFX in the old, Music in the latest, which the title imports it from). Saves the
    ones it changes. Returns how many sounds went to each class ('None' for none) and how many changed."""
    sound_mix()
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    result = {"changed": 0}
    for cls in ("SoundWave", "SoundCue"):
        found = registry.get_assets(unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", cls)],
                                                    package_paths=[paths.DD_ROOT], recursive_paths=True))
        for data in found:
            package = str(data.package_name)
            rel = _sound_rel(package)
            try:
                pkg, version = export_json(rel, 2), 2
            except FileNotFoundError:
                pkg, version = export_json(rel, 1), 1
            props = main_export(pkg, rel)["props"]
            sound = unreal.load_asset(package)
            if _write_sound_class(sound, props):
                EAL.save_loaded_asset(sound, only_if_is_dirty=False)
                result["changed"] += 1
            name = props.get("SoundClassObject", "None").rsplit(".", 1)[-1]
            result[name] = result.get(name, 0) + 1
    return result


# SoundCue properties the builder sets itself (the tree) or UE works out from it, and the sound class, which
# _write_sound_class writes.
SOUND_CUE_SKIP = ("FirstNode", "SoundClassObject", "Duration", "MaxDistance")


def _is_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)


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
        # A None child is an input left empty (SFX_06_Lockpicking's random node has a fifth, silent one).
        children = [node(child) if child else None for child in props.pop("ChildNodes", [])]
        # The inputs first: adding one to a random node adds a weight of 1, which its Weights then overwrite.
        error = lib.set_child_nodes(obj, children)
        if error is None or error:
            raise RuntimeError("%s: the inputs of %s were not linked: %s" % (rel, key, error))
        for name, value in props.items():
            if _is_number(value):
                text = repr(value)
            elif isinstance(value, list) and all(_is_number(v) for v in value):
                text = "(%s)" % ",".join(repr(v) for v in value)
            else:
                raise ValueError("%s: %s.%s has a value this does not write (%r)" % (rel, key, name, value))
            error = unreal.WasamiCascadeLibrary.set_property_text(obj, name, text)
            if error is None or error:
                raise RuntimeError("%s: %s.%s was not written: %s" % (rel, key, name, error))
        return obj

    props = main_export(pkg, rel)["props"]
    lib.finish_sound_cue(cue, node(props["FirstNode"]))
    attenuation = props.get("AttenuationSettings")
    if attenuation:   # Locked_Door's MonkeyAttenuation, made here when missing
        cue.set_editor_property("attenuation_settings", sound_attenuation(game_rel(attenuation), version))
    failures = ue_props.apply(cue, props, skip=SOUND_CUE_SKIP + ("AttenuationSettings",))
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    _write_sound_class(cue, props)
    EAL.save_asset(target, only_if_is_dirty=False)
    return target


def texture(rel, version=1):
    """Imports the original's /Game/<rel>.png under /Game/DD with its sRGB, compression and LOD group (_textures.json)
    and its NeverStream (the export). An engine texture the original uses ('/Engine/Functions/...') comes from the same
    table, under Engine/Content, and lands under /Game/DD/_Engine. Returns the package path."""
    with open(os.path.join(pak(version), "_textures.json"), encoding="utf-8") as f:
        table = json.load(f)
    project, sub = _content(rel)
    key = "%s/Content/%s.uasset" % (project, sub)
    entry = table.get(key)
    if entry is None:
        raise KeyError("no texture %s in %s/_textures.json" % (key, pak(version)))
    png = content_file(rel, version, ".png")
    if not os.path.exists(png):
        raise FileNotFoundError(png)
    target = asset_path(rel)
    tex = dd_stage.import_texture({"file": png, "asset": target, "srgb": entry["srgb"],
                                   "compression": entry["compression"], "lodGroup": entry["lod_group"]})
    props = main_export(export_json(rel, version), rel)["props"]
    tex.set_editor_property("never_stream", bool(props.get("NeverStream")))
    # The addressing the table lists (left out: UE's Wrap, which the import leaves).
    for axis in ("address_x", "address_y"):
        if entry.get(axis):
            tex.set_editor_property(axis, ue_props.enum_member(unreal.TextureAddress, entry[axis]))
    return target


def _import_font_face(face_rel, version, props=None):
    """Imports the .ttf of /Game/<face_rel> (or of an engine face, '/Engine/EngineFonts/Faces/RobotoTiny') as a FontFace
    under /Game/DD, with the exported properties (the hinting) when they are given. Returns its package path."""
    ttf = content_file(face_rel, version, ".ttf")
    if not os.path.exists(ttf):
        raise FileNotFoundError(ttf)
    face_path = asset_path(face_rel)
    folder, name = paths.split(face_path)
    task = unreal.AssetImportTask()
    task.filename = ttf
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    task.factory = unreal.FontFileImportFactory()
    _tools().import_asset_tasks([task])
    face = unreal.load_asset(face_path)
    if face is None:
        raise RuntimeError("the font face did not import to %s" % face_path)
    if props:
        failures = ue_props.apply(face, props, skip=("SourceFilename",))
        if failures:
            raise RuntimeError("settings of %s could not be set: %s" % (face_rel, "; ".join(failures)))
    return face_path


def _write_composite_font(font_asset, face_path, default_name="Default", fallback_name="Fallback"):
    """Puts the face into the Font as its Default typeface under `default_name`, over the fallback typeface (the engine's
    DroidSansFallback, what a character the face has not falls back to) under `fallback_name`."""
    # FCompositeFont's members are not exposed to Python, so the typefaces go in as the struct's own text form.
    composite = unreal.CompositeFont()
    composite.import_text('(DefaultTypeface=(Fonts=((Name="%s",Font=(FontFaceAsset=FontFace\'"%s"\','
                          'LoadingPolicy=LazyLoad,SubFaceIndex=0)))),FallbackTypeface=(Typeface=(Fonts='
                          '((Name="%s",Font=(FontFaceAsset=FontFace\'"%s"\',LoadingPolicy=LazyLoad,'
                          'SubFaceIndex=0)))),ScalingFactor=1.000000),SubTypefaces=,'
                          'bEnableAscentDescentOverride=True)'
                          % (default_name, paths.object_path(face_path), fallback_name, FALLBACK_FACE))
    font_asset.set_editor_property("composite_font", composite)


def font(face_rel, version=1):
    """Imports the original's /Game/<face_rel>.ttf as a font face under /Game/DD and makes the runtime Font asset its UMG
    texts use (<face_rel>_Font) with the face as its Default typeface, over the original's fallback typeface (the
    engine's DroidSansFallback, what a character the face has not falls back to). (The original's Font keeps the
    engine's Roboto as the default and the face as an en-US sub-typeface; the face is what an English game shows, and
    this game's texts are English whatever the machine's culture. Its helvetica-normal_Font lists a second fallback
    entry, RobotoRegular, which Slate never reaches: a typeface it cannot find the asked name in gives its first
    entry.) Returns the Font's package path."""
    face_path = _import_font_face(face_rel, version)
    folder, name = paths.split(face_path)
    font_path = face_path + "_Font"
    if EAL.does_asset_exist(font_path):
        font_asset = unreal.load_asset(font_path)
    else:
        font_asset = _tools().create_asset(name + "_Font", folder, unreal.Font, unreal.FontFactory())
    font_asset.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)
    _write_composite_font(font_asset, face_path)
    return font_path


def engine_font(rel, version=1):
    """The engine's own Font the original names ('/Engine/EngineFonts/RobotoTiny') rebuilt under /Game/DD/_Engine with
    its face, saved. The engine's own copies cannot be shipped: the cook leaves them out, because nothing but this
    game's C++ asks for them and a soft path a constructor sets is no package dependency of any asset, so a packaged
    build would draw the text in Slate's last resort font. The Font keeps the export's cache type, legacy size and
    legacy name and its typefaces' names — the widgets ask for the typeface by name ('Light') — and the face the
    export's hinting (RobotoTiny's Auto, where the engine's own Roboto Light face has AutoLight). Its fallback typeface
    stays the engine's DroidSansFallback, which the cook does take. Returns the Font's package path."""
    props = main_export(export_json(rel, version), rel)["props"]
    composite = props["CompositeFont"]
    default = composite["DefaultTypeface"]["Fonts"][0]
    fallback = composite["FallbackTypeface"]["Typeface"]["Fonts"][0]
    face_rel = default["Font"]["FontFaceAsset"].split(".", 1)[0]
    face_path = _import_font_face(face_rel, version, main_export(export_json(face_rel, version), face_rel)["props"])
    target = asset_path(rel)
    if EAL.does_asset_exist(target):
        font_asset = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        font_asset = _tools().create_asset(name, folder, unreal.Font, unreal.FontFactory())
    failures = ue_props.apply(font_asset, props, skip=("CompositeFont", "LegacyFontName"))
    if failures:
        raise RuntimeError("settings of %s could not be set: %s" % (rel, "; ".join(failures)))
    # LegacyFontName is an FName, which ue_props does not read a string into; it goes in as it is.
    if "LegacyFontName" in props:
        font_asset.set_editor_property("legacy_font_name", props["LegacyFontName"])
    _write_composite_font(font_asset, face_path, default["Name"], fallback["Name"])
    EAL.save_asset(face_path, only_if_is_dirty=False)
    EAL.save_asset(target, only_if_is_dirty=False)
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


def material_instance(asset_path, parent, scalars=None, vectors=None, textures=None, static_masks=None,
                      static_switches=None):
    """Loads or creates a MaterialInstanceConstant of parent and sets its parameters ({name: value}; a vector is 4
    numbers, a texture an asset path, a static component mask the channels it keeps, 'G', a static switch a bool); the
    parameters it had before (static ones too) are cleared first. Returns the instance."""
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
    # UE 5.8's static switch setter always returns False, so the values are read back after the update.
    for key, value in (static_switches or {}).items():
        MEL.set_material_instance_static_switch_parameter_value(mic, key, bool(value), update_material_instance=False)
    MEL.update_material_instance(mic)
    for key, value in (static_switches or {}).items():
        if MEL.get_material_instance_static_switch_parameter_value(mic, key) != bool(value):
            raise RuntimeError("%s: the static switch %s did not take %s" % (asset_path, key, value))
    return mic


# A material instance's BasePropertyOverrides the export can have switched on (bOverride_<name>), by the Python names of
# FMaterialInstanceBasePropertyOverrides' switch and value, and how the export's value is read.
BASE_PROPERTY_OVERRIDES = {
    "TwoSided": ("override_two_sided", "two_sided", bool),
    "BlendMode": ("override_blend_mode", "blend_mode",
                  lambda v: ue_props.enum_member(unreal.BlendMode, v.split("::")[-1])),
    "ShadingModel": ("override_shading_model", "shading_model",
                     lambda v: ue_props.enum_member(unreal.MaterialShadingModel, v.split("::")[-1])),
    "OpacityMaskClipValue": ("override_opacity_mask_clip_value", "opacity_mask_clip_value", float),
}


def base_property_overrides(mic, rel, version):
    """Switches on the material instance's base property overrides its export switches on (bOverride_TwoSided with
    TwoSided, ...), with their values, and updates it. The export lists values it does not switch on (the parent's), which
    are left alone. Returns the names switched on."""
    props = main_export(export_json(rel, version), rel)["props"].get("BasePropertyOverrides") or {}
    on = [k[len("bOverride_"):] for k, v in props.items() if k.startswith("bOverride_") and v]
    unknown = [name for name in on if name not in BASE_PROPERTY_OVERRIDES]
    if unknown:
        raise NotImplementedError("%s overrides %s" % (rel, unknown))
    overrides = mic.get_editor_property("base_property_overrides")
    for name in on:
        switch, value, convert = BASE_PROPERTY_OVERRIDES[name]
        overrides.set_editor_property(switch, True)
        overrides.set_editor_property(value, convert(props[name]))
    mic.set_editor_property("base_property_overrides", overrides)
    MEL.update_material_instance(mic)
    return on


# The engine's material function libraries.
FUNCTIONS_01 = "/Engine/Functions/Engine_MaterialFunctions01/"
FUNCTIONS_02 = "/Engine/Functions/Engine_MaterialFunctions02/"
FUNCTIONS_03 = "/Engine/Functions/Engine_MaterialFunctions03/"
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
    path}, {static component mask: the channels it keeps, 'G'}, {static switch: bool}), without the engine's parameters
    (ENGINE_PARAMETERS). Static parameters of other kinds raise (nothing writes them yet)."""
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
    switches = {p["ParameterInfo"]["Name"]: bool(p.get("Value")) for p in static.pop("StaticSwitchParameters", [])
                if p.get("bOverride")}
    if any(static.values()):
        raise NotImplementedError("%s has static parameters %s" % (rel, sorted(k for k, v in static.items() if v)))
    return scalars, vectors, textures, masks, switches


# Helpers for the estimated masters of particle materials (graphs of dd_stage._Graph).
PIPELINE_MATERIALS = paths.PIPELINE_ROOT + "/Materials/"
MP = unreal.MaterialProperty


def particle_material(mat, beam_trails=False, responsive_aa=False, two_sided=False):
    """The settings a cook keeps on the particle packs' materials (their blend is translucent, set by material())."""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", two_sided)
    mat.set_editor_property("used_with_particle_sprites", True)
    mat.set_editor_property("used_with_mesh_particles", True)
    mat.set_editor_property("used_with_beam_trails", beam_trails)
    mat.set_editor_property("enable_responsive_aa", responsive_aa)


def depth_faded_opacity(g, opacity, distance, x, y):
    """opacity × how far the scene lies behind it (DepthFade over distance, None: the expression's own 100; UE clamps
    a distance of 0 to a tiny one, which fades nothing), as the material's opacity."""
    fade = g.node(unreal.MaterialExpressionDepthFade, x, y)
    connect(opacity, "", fade, "Opacity")
    if distance is not None:
        connect(distance, "", fade, "FadeDistance")
    g.out(fade, "", MP.MP_OPACITY)


def radial_gradient(g, radius, density, x, y, uvs=None, centre=None):
    """RadialGradientExponential with the given radius, density, UVs and centre (None leaves the function's default:
    TexCoord 0, the middle)."""
    call = function_call(g, "Gradient/RadialGradientExponential", x, y)
    for source, pin in ((radius, "Radius"), (density, "Density"), (uvs, "UVs"), (centre, "CenterPosition")):
        if source is not None:
            connect(source, "", call, pin)
    return call


def add(g, a, a_pin, b, b_pin, x, y):
    return g.binary(unreal.MaterialExpressionAdd, a, a_pin, b, b_pin, x, y)


def single(g, cls, source, pin, x, y):
    """A one-input expression (Saturate, OneMinus, Abs, ...) of source's output pin."""
    e = g.node(cls, x, y)
    connect(source, pin, e, "")
    return e


def channel(g, source, pin, keep, x, y):
    """One channel ('R', 'G', ...) of source's output pin (a ComponentMask starts with R and G on, so each is set)."""
    e = g.node(unreal.MaterialExpressionComponentMask, x, y)
    for c in "RGBA":
        e.set_editor_property(c.lower(), c == keep)
    connect(source, pin, e, "")
    return e


def dynamic_parameter(g, names, x, y, defaults=(0.0, 0.0, 0.0, 1.0)):
    """A DynamicParameter with the original's names (the particle system's values are named after them) and defaults."""
    e = g.node(unreal.MaterialExpressionDynamicParameter, x, y)
    e.set_editor_property("param_names", list(names))
    e.set_editor_property("default_value", unreal.LinearColor(*defaults))
    # The outputs take the names only when they are asked for (GetOutputs), and connecting looks them up as they are.
    MEL.get_material_expression_output_names(e)
    return e


def estimated_materials(folder, entries, version, left_out=()):
    """Particle materials whose graphs the cook took away. entries: (the original's material under folder, the master
    holding our estimate under /Game/Pipeline/Materials, its builder(mat, the original's scalar and vector defaults by
    name), the original's instances of that material). Makes each master (translucent), an instance of it at the
    original material's path with the original's defaults of the parameters the estimate has (those of the sides not
    made left out), and the original's instances as instances of that one with their own values (a value the estimate
    has no parameter for raises, unless its parameter is named in left_out: one the estimate knowingly does without,
    whose values are dropped) and the base property overrides they switch on. Returns the assets, saved."""
    made = []
    for name, master_name, build, children in entries:
        rel = folder + name
        scalars, vectors = parameter_defaults(rel, version)
        defaults = dict(scalars, **vectors)
        master = material(PIPELINE_MATERIALS + master_name, lambda mat, b=build, d=defaults: b(mat, d),
                          blend_mode=unreal.BlendMode.BLEND_TRANSLUCENT)
        known = {kind: {str(n) for n in names(master)} for kind, names in (
            ("scalars", MEL.get_scalar_parameter_names), ("vectors", MEL.get_vector_parameter_names),
            ("textures", MEL.get_texture_parameter_names), ("switches", MEL.get_static_switch_parameter_names))}
        base = material_instance(asset_path(rel), master,
                                 scalars={k: v for k, v in scalars.items() if k in known["scalars"]},
                                 vectors={k: v for k, v in vectors.items() if k in known["vectors"]})
        made += [master, base]
        for child in children:
            child_rel = folder + child
            c_scalars, c_vectors, c_textures, c_masks, c_switches = instance_parameters(child_rel, version)
            for kind, values in (("scalars", c_scalars), ("vectors", c_vectors), ("textures", c_textures),
                                 ("switches", c_switches)):
                for name in set(values) & set(left_out):
                    del values[name]
                unknown = set(values) - known[kind]
                if unknown:
                    raise RuntimeError("%s sets %s, which the estimate of %s does not have"
                                       % (child, sorted(unknown), name))
            child_mic = material_instance(asset_path(child_rel), base, scalars=c_scalars, vectors=c_vectors,
                                          textures=c_textures, static_masks=c_masks, static_switches=c_switches)
            base_property_overrides(child_mic, child_rel, version)
            made.append(child_mic)
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return made


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
