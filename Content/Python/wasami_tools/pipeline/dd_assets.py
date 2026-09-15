"""Dark Deception (the original, pak_reference): Blueprints of its classes rebuilt from the exported class defaults,
under /Game/DD/<the original's path under /Game>."""
import json
import os

import unreal

from wasami_tools.pipeline import paths, ue_props

EAL = unreal.EditorAssetLibrary


def export_json(rel):
    """The exported package of the original's /Game/<rel> ('Blueprints/Main/BP_DD_PlayerCharacter_WalkShake')."""
    path = os.path.join(paths.DD_PAK, "_assets", "DDeception", "Content", *rel.split("/")) + ".json"
    if not os.path.exists(path):
        raise FileNotFoundError("no export of /Game/%s in %s" % (rel, paths.DD_PAK))
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def class_defaults(pkg):
    return next(e for e in pkg["exports"] if e["name"].startswith("Default__"))["props"]


def camera_shake(rel):
    """A LegacyCameraShake Blueprint (UE4's UCameraShake, as the original's) with the original's defaults. Returns its
    package path; an existing one gets its defaults written again."""
    target = paths.DD_ROOT + "/" + rel
    if EAL.does_asset_exist(target):
        bp = unreal.load_asset(target)
    else:
        folder, name = paths.split(target)
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.LegacyCameraShake)
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    failures = ue_props.apply(cdo, class_defaults(export_json(rel)), skip=("UberGraphFrame",))
    for f in failures:
        unreal.log_warning("camera_shake %s: %s" % (rel, f))
    if failures:
        raise RuntimeError("%d defaults of %s could not be set (see the log)" % (len(failures), rel))
    EAL.save_asset(target, only_if_is_dirty=False)
    return target
