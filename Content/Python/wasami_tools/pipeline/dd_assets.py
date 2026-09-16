"""Dark Deception (the original, pak_reference / pak_reference_2): its assets rebuilt from the exported data under
/Game/DD/<the original's path under /Game> — Blueprints of its classes from their class defaults, and sounds with the
SoundWave's own settings."""
import json
import os

import unreal

from wasami_tools.pipeline import paths, ue_props

EAL = unreal.EditorAssetLibrary

# SoundWave properties of the export that are written onto the imported wave, with UE's default for when the export
# leaves them out (it keeps only what differs from the defaults). Channels, rate and duration come with the file; the
# original's sound classes are not made (yet), so SoundClassObject is not written.
SOUND_DEFAULTS = {"Volume": 1.0, "Pitch": 1.0}


def pak(version):
    """The export root of pak_reference (1, UE 4.21) or pak_reference_2 (2, UE 4.24)."""
    return paths.DD_PAK if version == 1 else paths.DD_PAK2


def export_json(rel, version=1):
    """The exported package of the original's /Game/<rel> ('Blueprints/Main/BP_DD_PlayerCharacter_WalkShake')."""
    path = os.path.join(pak(version), "_assets", "DDeception", "Content", *rel.split("/")) + ".json"
    if not os.path.exists(path):
        raise FileNotFoundError("no export of /Game/%s in %s" % (rel, pak(version)))
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
    target = paths.DD_ROOT + "/" + rel
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
    target = paths.DD_ROOT + "/" + rel
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
    """Imports the original's /Game/<rel>.ogg as a SoundWave under /Game/DD and writes the export's Volume, Pitch and
    ConcurrencySet onto it (making the concurrency assets it names). Returns the package path."""
    ogg = os.path.join(pak(version), "DDeception", "Content", *rel.split("/")) + ".ogg"
    if not os.path.exists(ogg):
        raise FileNotFoundError(ogg)
    target = paths.DD_ROOT + "/" + rel
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
    concurrency = [sound_concurrency(game_rel(p), version) for p in props.get("ConcurrencySet", [])]
    wave.set_editor_property("concurrency_set", concurrency)
    return target
