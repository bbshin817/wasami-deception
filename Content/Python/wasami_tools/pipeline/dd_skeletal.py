"""Dark Deception's skinned meshes with their animations, from pak_reference_2 (UE 4.24): the garage lifts'
(hospital_garage_lift_anim and hospital_garage_lift_anim_Anim, for AWasamiGarageLift) and Zone 2's saw traps'
(hospital_sawTrap_short_01_anim, _short_02_, _medium_01_ and _long_01_, each with its _Anim: a blade that spins as it
slides along its track).

  sources   _meshes_gltf/<mesh>.gltf, the SkeletalMesh in its reference pose (glTF's metres and Y up: UE's (x, y, z) cm
            is (x, z, y) / 100), and _anims_psa/<anim>.psa, its AnimSequence as ActorX keys (UE's centimetres and axes,
            one key per bone per frame, the frames 1 / rate apart with rate = (NumFrames - 1) / SequenceLength)
  prepared  Intermediate/Pipeline/dd/skeletal/<mesh>_prepared.glb: the mesh with the psa as a glTF animation named as
            the AnimSequence. Its translations go through the same axes as the reference pose (checked against the
            glTF's own joints). A bone's rotations go through the axes too: the swap of Y and Z mirrors, so (x, y, z, w)
            is (-x, -z, -y, w) in the glTF (checked on every bone's reference pose; the other bones' keys are written as
            their reference pose, as the garage lift's shows: its bones that do not turn key their reference rotations,
            a quarter turn among them). The root's keys are written in another convention than its reference pose's,
            so a root that turns is refused and the root keeps the glTF's rotation (none of the meshes' roots turn).
            A joint's parent that is not a joint and does nothing (the glTF's '<mesh>.ao' node) is taken out, so the
            skeleton is the original's bones alone, and the mesh's node gets the mesh's name (the import names the
            SkeletalMesh, its _Skeleton and _PhysicsAsset after the node).
  imported  the original's folder under /Game/DD: the SkeletalMesh with <mesh>_Skeleton and <mesh>_PhysicsAsset (the
            import's own), and the AnimSequence baked at the original's frame rate. The slots get the stage's materials
            (Tools/dd/prepare_stage.py's CLASS_MATERIALS, imported with the stage's assets) by name, and their master
            is made usable on skinned meshes.
"""
import json
import math
import os
import struct

import unreal

from wasami_tools.pipeline import dd_assets, dd_enemy, gltf, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
VERSION = 2   # the hospital is only in the latest version
PREPARED_DIR = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "dd", "skeletal")
GARAGE_LIFT_MESH = "Meshes/06_Hospital/hospital_garage_lift_anim"
GARAGE_LIFT_ANIM = "Meshes/06_Hospital/hospital_garage_lift_anim_Anim"
# The saw traps' meshes (BP_06_sawTrap_short01's, _short02's, _medium's and _long01's); each one's animation is <mesh>_Anim.
SAW_TRAP_MESHES = tuple("Meshes/06_Hospital/hospital_sawTrap_%s_anim" % n
                        for n in ("short_01", "short_02", "medium_01", "long_01"))
CM = 100.0
TOLERANCE = 1e-4   # cm, and quaternion components


# ------------------------------------------------------------------------------------------------ psa
def read_psa(path):
    """An ActorX animation file: {"bones": [{"name", "parent", "rotation" (x, y, z, w), "position"}], "frames",
    "keys": [[(position, rotation) per bone] per frame]}. One sequence per file (the export writes one)."""
    with open(path, "rb") as f:
        data = f.read()
    chunks, offset = {}, 0
    while offset < len(data):
        name = data[offset:offset + 20].split(b"\0")[0].decode("ascii")
        size, count = struct.unpack_from("<ii", data, offset + 24)
        offset += 32
        chunks[name] = (size, count, data[offset:offset + size * count])
        offset += size * count
    size, count, raw = chunks["BONENAMES"]
    bones = []
    for i in range(count):
        row = raw[i * size:(i + 1) * size]
        _flags, _children, parent = struct.unpack_from("<iii", row, 64)
        bones.append({"name": row[:64].split(b"\0")[0].decode("ascii"), "parent": parent,
                      "rotation": struct.unpack_from("<4f", row, 76), "position": struct.unpack_from("<3f", row, 92)})
    size, count, raw = chunks["ANIMINFO"]
    if count != 1:
        raise ValueError("%s: %d sequences, the reading takes one" % (path, count))
    total_bones, _root, _style, _quotum, _reduction, _track_time, _rate, _start, first, frames = \
        struct.unpack_from("<iiiifffiii", raw, 128)
    if total_bones != len(bones) or first != 0:
        raise ValueError("%s: %d bones from frame %d, not the file's %d from 0" % (path, total_bones, first, len(bones)))
    size, count, raw = chunks["ANIMKEYS"]
    if count != frames * len(bones):
        raise ValueError("%s: %d keys, not %d frames x %d bones" % (path, count, frames, len(bones)))
    keys = []
    for frame in range(frames):
        row = []
        for bone in range(len(bones)):
            values = struct.unpack_from("<3f4f", raw, (frame * len(bones) + bone) * size)
            row.append((values[:3], values[3:]))
        keys.append(row)
    return {"bones": bones, "frames": frames, "keys": keys}


def to_gltf_position(p):
    """UE's (x, y, z) cm → glTF's metres, Y up (the export's glTF swaps UE's Y and Z)."""
    return (p[0] / CM, p[2] / CM, p[1] / CM)


def to_gltf_rotation(q):
    """UE's (x, y, z, w) → the glTF's: the axes' swap is a mirror, which turns the other way about the swapped axes."""
    return (-q[0], -q[2], -q[1], q[3])


def _same_rotation(a, b):
    return abs(abs(sum(x * y for x, y in zip(a, b))) - 1.0) < TOLERANCE


def _continuous(quats):
    """The quaternions each on the side of the one before (the same rotations, so a blend between keys takes the short
    way)."""
    out = []
    for q in quats:
        if out and sum(x * y for x, y in zip(out[-1], q)) < 0.0:
            q = tuple(-x for x in q)
        out.append(tuple(q))
    return out


# ------------------------------------------------------------------------------------------------ prepare
def _gltf_file(mesh_rel):
    with open(os.path.join(dd_assets.pak(VERSION), "_meshes.json"), encoding="utf-8") as f:
        entry = json.load(f).get("/Game/" + mesh_rel)
    if entry is None or entry["class"] != "SkeletalMesh":
        raise KeyError("no skeletal mesh /Game/%s in _meshes.json" % mesh_rel)
    return os.path.join(dd_assets.pak(VERSION), *entry["gltf"].split("/"))


def frame_rate(anim_rel):
    """The AnimSequence's keys per second: its NumFrames keys span its SequenceLength."""
    props = dd_assets.main_export(dd_assets.export_json(anim_rel, VERSION), anim_rel)["props"]
    rate = (props["NumFrames"] - 1) / props["SequenceLength"]
    if abs(rate - round(rate)) > 1e-3:
        raise ValueError("%s: %d frames over %.6f s is %.4f fps, not a whole rate"
                         % (anim_rel, props["NumFrames"], props["SequenceLength"], rate))
    return int(round(rate)), props["NumFrames"], props["SequenceLength"]


def _drop_idle_joint_parents(model):
    """Takes out the scene root nodes that are only a joint's parent and move nothing, so the joint is a root."""
    joints = set(model["skins"][0]["joints"])
    idle = [i for i in model["scenes"][model.get("scene", 0)]["nodes"]
            if i not in joints and "mesh" not in model["nodes"][i]
            and not any(k in model["nodes"][i] for k in ("rotation", "translation", "scale", "matrix"))
            and model["nodes"][i].get("children") and set(model["nodes"][i]["children"]) <= joints]
    if not idle:
        return []
    keep = [i for i in range(len(model["nodes"])) if i not in idle]
    index = {old: new for new, old in enumerate(keep)}
    scene = model["scenes"][model.get("scene", 0)]
    roots = []
    for i in scene["nodes"]:
        roots.extend(model["nodes"][i]["children"] if i in idle else [i])
    names = [model["nodes"][i].get("name") for i in idle]
    model["nodes"] = [model["nodes"][i] for i in keep]
    for node in model["nodes"]:
        if "children" in node:
            node["children"] = [index[c] for c in node["children"]]
    scene["nodes"] = [index[i] for i in roots]
    for skin in model["skins"]:
        skin["joints"] = [index[j] for j in skin["joints"]]
        if "skeleton" in skin:
            skin["skeleton"] = index.get(skin["skeleton"], skin["joints"][0])
    return names


def prepare(mesh_rel, anim_rel):
    """Writes the mesh's glTF with the psa as its animation. Returns (file, rate, report)."""
    model, blob = gltf.read(_gltf_file(mesh_rel))
    psa = read_psa(os.path.join(dd_assets.pak(VERSION), "_anims_psa", *anim_rel.split("/")) + ".psa")
    rate, frames, length = frame_rate(anim_rel)
    if psa["frames"] != frames:
        raise ValueError("%s: the psa has %d frames, the AnimSequence %d" % (anim_rel, psa["frames"], frames))
    if len(model.get("skins") or []) != 1:
        raise ValueError("%s: the reading takes one skin" % mesh_rel)
    joints = [model["nodes"][j] for j in model["skins"][0]["joints"]]
    names = [j["name"] for j in joints]
    if names != [b["name"] for b in psa["bones"]]:
        raise ValueError("%s: the psa's bones %s are not the glTF's joints %s"
                         % (anim_rel, [b["name"] for b in psa["bones"]], names))
    tracks = {}
    for i, (joint, bone) in enumerate(zip(joints, psa["bones"])):
        bind = joint.get("translation", [0.0, 0.0, 0.0])
        if any(abs(a - b) * CM > TOLERANCE for a, b in zip(to_gltf_position(bone["position"]), bind)):
            raise ValueError("%s: %s's reference position %s is not the glTF's %s through the axes"
                             % (anim_rel, bone["name"], bone["position"], bind))
        bind_rotation = tuple(joint.get("rotation", [0.0, 0.0, 0.0, 1.0]))
        if not _same_rotation(to_gltf_rotation(bone["rotation"]), bind_rotation):
            raise ValueError("%s: %s's reference rotation %s is not the glTF's %s through the axes"
                             % (anim_rel, bone["name"], bone["rotation"], bind_rotation))
        tracks[(bone["name"], "translation")] = [to_gltf_position(psa["keys"][f][i][0]) for f in range(frames)]
        if bone["parent"] < 0:
            first = psa["keys"][0][i][1]
            if any(not _same_rotation(first, psa["keys"][f][i][1]) for f in range(frames)):
                raise ValueError("%s: the root %s turns; the reading carries a still root only" % (anim_rel, bone["name"]))
            tracks[(bone["name"], "rotation")] = [bind_rotation] * frames
        else:
            tracks[(bone["name"], "rotation")] = _continuous(
                [to_gltf_rotation(psa["keys"][f][i][1]) for f in range(frames)])
    dropped = _drop_idle_joint_parents(model)
    for node in model["nodes"]:
        if "mesh" in node and not node.get("name"):
            node["name"] = model["meshes"][node["mesh"]]["name"]   # the import names the SkeletalMesh after its node
    model.pop("animations", None)
    gltf.add_animation(model, blob, anim_rel.split("/")[-1], tracks, rate)
    os.makedirs(PREPARED_DIR, exist_ok=True)
    # Not named after an asset: Interchange makes the import of a file named as an asset in the folder a reimport of it.
    out = os.path.join(PREPARED_DIR, mesh_rel.split("/")[-1] + "_prepared.glb")
    gltf.write(out, model, blob)
    moved = {b["name"]: round(max(abs(a - z) for k in psa["keys"] for a, z in zip(k[i][0], psa["keys"][0][i][0])), 3)
             for i, b in enumerate(psa["bones"])}
    turned = {b["name"]: round(max(_angle(k[i][1], psa["keys"][0][i][1]) for k in psa["keys"]), 1)
              for i, b in enumerate(psa["bones"])}
    return out, rate, {"frames": frames, "seconds": length, "rate": rate, "dropped_nodes": dropped, "slide_cm": moved,
                       "turn_deg": turned}


def _angle(a, b):
    """Degrees between two rotations."""
    return math.degrees(2.0 * math.acos(min(1.0, abs(sum(x * y for x, y in zip(a, b))))))


# ------------------------------------------------------------------------------------------------ import
def _skinnable(material):
    """Makes the material's base Material usable on skinned meshes. Returns whether it changed."""
    base = material.get_base_material() if isinstance(material, unreal.MaterialInstance) else material
    if base.get_editor_property("used_with_skeletal_mesh"):
        return False
    base.set_editor_property("used_with_skeletal_mesh", True)
    MEL.recompile_material(base)
    EAL.save_loaded_asset(base, only_if_is_dirty=False)
    return True


def import_skinned(mesh_rel, anim_rel):
    """Prepares and imports the mesh with its animation, and gives its slots the stage's materials of their names."""
    prepared, rate, report = prepare(mesh_rel, anim_rel)
    materials = {}
    for imp in dd_assets.export_json(mesh_rel, VERSION)["imports"]:
        if imp.startswith("/Game/Materials/"):
            path = dd_assets.asset_path(imp[len("/Game/"):])
            material = unreal.load_asset(path)
            if material is None:
                raise RuntimeError("missing %s: run python Tools/dd/prepare_stage.py and "
                                   "WasamiStageTools.import_dd_stage_assets" % path)
            materials[path.split("/")[-1]] = material
    folder = paths.split(dd_assets.asset_path(mesh_rel))[0]
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    dd_enemy.ensure_skeletal_pipeline(paths.DD_SKELETAL_PIPELINE, rate)
    params.override_pipelines = [unreal.SoftObjectPath(paths.object_path(paths.DD_SKELETAL_PIPELINE))]
    src = unreal.InterchangeManager.create_source_data(prepared)
    if not unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(folder, src, params):
        raise RuntimeError("the import of %s failed (see the output log)" % prepared)
    mesh = unreal.load_asset(dd_assets.asset_path(mesh_rel))
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("the import made no SkeletalMesh at %s" % dd_assets.asset_path(mesh_rel))
    anim = unreal.load_asset(dd_assets.asset_path(anim_rel))
    if not isinstance(anim, unreal.AnimSequence):
        raise RuntimeError("the import made no AnimSequence at %s" % dd_assets.asset_path(anim_rel))
    slots = mesh.get_editor_property("materials")
    unmatched = []
    for i, slot in enumerate(slots):
        name = str(slot.get_editor_property("material_slot_name"))
        if name not in materials:
            unmatched.append(name)
            continue
        slot.set_editor_property("material_interface", materials[name])
        slots[i] = slot
    if unmatched:
        raise RuntimeError("%s: slots %s have no material of the original's %s"
                           % (mesh.get_path_name(), unmatched, sorted(materials)))
    mesh.set_editor_property("materials", slots)
    report["skinnable_masters"] = sum(1 for m in materials.values() if _skinnable(m))
    report["imported_seconds"] = round(anim.get_play_length(), 4)
    report["imported_frames"] = unreal.AnimationLibrary.get_num_frames(anim)
    for asset in (mesh, anim):
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    EAL.save_directory(folder, only_if_is_dirty=True, recursive=False)
    unreal.log("dd_skeletal: %s %s" % (mesh_rel, report))
    return report


def import_garage_lift():
    """The garage lifts' mesh and animation. Returns the report."""
    return import_skinned(GARAGE_LIFT_MESH, GARAGE_LIFT_ANIM)


def import_saw_traps():
    """The saw traps' four meshes with their animations. Returns the reports by mesh."""
    return {mesh_rel.split("/")[-1]: import_skinned(mesh_rel, mesh_rel + "_Anim") for mesh_rel in SAW_TRAP_MESHES}
