"""The boss Wasami (AWasamiMatron, Zone 2's Matron): its skeletal mesh, material and animations, from this game's own
model, with dd_enemy's tools (the two models have the same 28 bones, rooted at pelvis, and their keys the same timing).

  source    SourceArt/Wasami/boss_wasami.glb, the user's model (7 animations)
  prepared  Intermediate/Pipeline/wasami/boss/WasamiBoss.glb: the model with the animations the Matron plays, by role
            (ROLES, .claude/references/enemy-wasami-motions.md), each resampled on a 30 fps grid from 0 as the enemy's
            are (the keys start at 1/24 s, and Interchange refuses an animation that does not end on a frame)
  imported  /Game/Wasami/Boss: SK_WasamiBoss with SK_WasamiBoss_Skeleton and SK_WasamiBoss_PhysicsAsset,
            A_WasamiBoss_<role>, T_WasamiBoss_* and MI_WasamiBoss (of the enemy's M_DD_WasamiGltf). The mesh faces +Y,
            as the enemy's does.
"""
import copy
import os

import unreal

from wasami_tools.pipeline import dd_assets, dd_enemy, gltf, paths

EAL = unreal.EditorAssetLibrary

SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "boss_wasami.glb")
PREPARED_DIR = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "boss")
FOLDER = paths.WASAMI_ROOT + "/Boss"
MESH_NAME = "SK_WasamiBoss"
MESH = FOLDER + "/" + MESH_NAME
ANIM_PREFIX = "A_WasamiBoss_"
TEXTURE_PREFIX = "T_WasamiBoss_"
MATERIAL = FOLDER + "/MI_WasamiBoss"

HEAD = "head"

# (role, glTF animation, how), the roles the original's Matron_MiniBoss_AnimBP plays:
#   loop    closed: the first key is repeated after the last (Long_Breathe_and_Look_Around stops a frame short of its
#           first pose: its last key is 0.33° off it, its last step 0.29°)
#   closed  a loop whose last key already is its first pose, as it is (Alert's is 0.03° off it, its steps 4°)
#   once    as it is
ROLES = (
    ("Idle", "Long_Breathe_and_Look_Around", "loop"),
    ("Alert", "Alert", "closed"),
    ("Detected", "Lower_Weapon_Look_Raise", "once"),
)
# The Matron does not move (BP_06_Matron_MiniBoss has no movement), so the walks are not imported, nor the bind pose. An
# animation the user adds later is imported as it is, under its own name.
SKIPPED = ("Walking", "Running", "Walking_Scan_with_Sudden_Look_Back", "restpose")


def prepared_file():
    # Not named after an asset (dd_enemy.prepared_file).
    return os.path.join(PREPARED_DIR, "WasamiBoss.glb")


def prepare():
    """Writes the prepared glb. Returns ({role: seconds}, the head bone's height in metres at Idle's first key)."""
    if not os.path.exists(SOURCE):
        raise FileNotFoundError("%s is missing (git lfs pull)" % SOURCE)
    model, blob = gltf.read(SOURCE)
    out = copy.deepcopy(model)
    out["animations"] = []
    names = gltf.node_names(model)
    skinned = [n for n in out["nodes"] if "skin" in n]
    if len(skinned) != 1 or len(out["meshes"]) != 1 or len(model["skins"][0]["joints"]) != dd_enemy.BONES:
        raise RuntimeError("%s is not one mesh skinned to %d bones" % (SOURCE, dd_enemy.BONES))
    skinned[0]["name"] = MESH_NAME
    out["meshes"][0]["name"] = MESH_NAME

    animations = {a["name"]: a for a in model["animations"]}
    roles = list(ROLES)
    used = {name for _, name, _ in ROLES}
    for name in animations:
        if name not in used and name not in SKIPPED:
            unreal.log_warning("boss: %s has no role; imported as it is" % name)
            roles.append((name, name, "once"))

    report, head = {}, None
    for role, name, how in roles:
        chans = gltf.channels(model, blob, animations[name])
        missing = {node for node, _ in chans} - set(names)
        if missing:
            raise RuntimeError("%s: bones not in the model: %s" % (name, sorted(missing)))
        tracks = dd_enemy._sample(chans, dd_enemy._content_frames(chans))
        if how == "loop":
            for values in tracks.values():
                values.append(values[0])
        if role == "Idle":
            pose = dd_enemy._key(tracks, 0)
            world = gltf.world_transforms(model, {n: v for (n, p), v in pose.items() if p == "rotation"},
                                          {n: v for (n, p), v in pose.items() if p == "translation"})
            head = world[HEAD][1][1]
        gltf.add_animation(out, blob, ANIM_PREFIX + role, tracks, dd_enemy.RATE)
        report[role] = (len(next(iter(tracks.values()))) - 1) / dd_enemy.RATE

    os.makedirs(PREPARED_DIR, exist_ok=True)
    gltf.write(prepared_file(), out, blob)
    return report, head


def import_all():
    """Prepares and imports the boss Wasami, then saves /Game/Wasami/Boss. Returns how many of each kind and the head
    bone's height (cm, unscaled) at Idle's first key, and logs each animation's length."""
    report, head = prepare()
    for role, seconds in report.items():
        unreal.log("boss: %s%s %.3f s" % (ANIM_PREFIX, role, seconds))
    textures = dd_enemy._extract_textures(SOURCE, PREPARED_DIR, FOLDER, TEXTURE_PREFIX)
    # The enemy's master, whose parameters the instance sets; it is made here only when the enemy was not imported.
    if EAL.does_asset_exist(dd_enemy.MASTER):
        master = unreal.load_asset(dd_enemy.MASTER)
    else:
        master = dd_assets.material(dd_enemy.MASTER, lambda mat: dd_enemy._build_master(mat, textures))
        EAL.save_loaded_asset(master, only_if_is_dirty=False)
    instance = dd_assets.material_instance(MATERIAL, master,
                                           textures={p: t.get_path_name().split(".")[0] for p, t in textures.items()})
    mesh, anims = dd_enemy._import_model(instance, prepared_file(), FOLDER, MESH, ANIM_PREFIX, [r[0] for r in ROLES])
    for asset in [instance, mesh]:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    return {"textures": len(textures), "materials": 1, "meshes": 1, "animations": len(report),
            "idle_head_cm": round(head * 100.0, 1)}
