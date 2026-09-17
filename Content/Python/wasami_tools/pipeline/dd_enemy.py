"""The enemy Wasami (AWasamiEnemy): its skeletal mesh, material and animations, from this game's own model.

  sources   SourceArt/Wasami/enemy_wasami_v3.glb, the user's model (28 bones rooted at pelvis, no root bone; 16
            animations), and SourceArt/Wasami/enemy_wasami_capture.glb, the capture's three animations of the user's
            older model with the run_fast_2 both models have (its bones and those animations only: make_capture_source
            took them out of the user's tmp/enemy_wasami.glb)
  prepared  Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb: the model with the animations the code asks for by
            role (ROLES, .claude/references/enemy-wasami-motions.md), each resampled on a 30 fps grid from 0 — the
            sources' keys mix 24 and 30 fps and start at 1/24 s, and Interchange refuses an animation that does not end
            on a frame. Loops are closed (their last key is their first), the chase variants and the Nightmare run are
            made in place, the stun is cut into a loop and a recovery, the vault off a ledge is made a vault from the
            floor (VAULT_FRAMES), and the capture's are carried onto v3's bones (_Retarget).
  imported  /Game/Wasami/Enemy: SK_WasamiEnemy with SK_WasamiEnemy_Skeleton and SK_WasamiEnemy_PhysicsAsset,
            A_WasamiEnemy_<role>, T_WasamiEnemy_* and MI_WasamiEnemy (of M_DD_WasamiGltf, glTF's metallic-roughness
            material). The mesh faces +Y, as UE's mannequins do.
"""
import copy
import math
import os

import unreal

from wasami_tools.pipeline import dd_assets, dd_stage, gltf, paths

EAL = unreal.EditorAssetLibrary

SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "enemy_wasami_v3.glb")
CAPTURE_SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "enemy_wasami_capture.glb")
PREPARED_DIR = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "enemy")
FOLDER = paths.WASAMI_ROOT + "/Enemy"
MESH_NAME = "SK_WasamiEnemy"
MESH = FOLDER + "/" + MESH_NAME
SKELETON = MESH + "_Skeleton"
PHYSICS_ASSET = MESH + "_PhysicsAsset"
ANIM_PREFIX = "A_WasamiEnemy_"
MATERIAL = FOLDER + "/MI_WasamiEnemy"
MASTER = "/Game/Pipeline/Materials/M_DD_WasamiGltf"
PIPELINE_VERSION = "1"  # bump when ensure_skeletal_pipeline's settings change

ROOT_BONE = "pelvis"
BONES = 28
RATE = 30
# Every bone's first key is at 2/30 s (the pelvis has one at 1/24 s as well, from the 24 fps scene the 30 fps motion was
# baked into); a clip's content runs from there to the last key of its other bones.
CONTENT_START_FRAME = 2

STUN = "01a0a88f-0db6-7251-85e6-1a97b799ee52"
# The stun's loop, in the source's 30 fps frames: its bent-over sway from 0.967 s to 2.467 s, the pair of poses that
# match best in the bent part (a mean joint distance of 3.3 cm) — found by comparing every pair at least 1.5 s apart.
# The recovery is what follows the loop in the source (still bent until 4.8 s, upright by 8.0 s, settling to 10 s).
STUN_LOOP_FRAMES = (29, 74)
# The loop's last STUN_BLEND_FRAMES cross into the poses before its start, so its last key is its first; the recovery's
# first ones cross from the loop's continuation, so it starts on the loop's first key.
STUN_BLEND_FRAMES = 15

# Vault_and_Land vaults off a ledge (its feet are 77 cm higher at the start than at the end), in the source's 30 fps
# frames: the left foot leaves the ledge after 28 (0.933 s; the right one swings up from 0.567 s), the feet reach the
# floor at 50 (1.667 s), and the pelvis stops rising at 74 (2.467 s), where the clip is cut (then it only stands).
VAULT = "Vault_and_Land"
VAULT_FRAMES = (28, 50, 74)
FEET = ("ball_l", "ball_r")

# (role, source, glTF animation, how):
#   loop           closed: the first key is repeated after the last (the sources' loops stop a frame short)
#   once           as it is
#   in_place       the pelvis's horizontal motion (glTF x and z) held at its first key; its height stays
#   loop_in_place  both
#   stun_loop / stun_recover   see STUN_LOOP_FRAMES
#   vault          see _vault
V3, CAPTURE = "v3", "capture"
ROLES = (
    ("Idle", V3, "Idle_11", "loop"),
    ("Idle_Alert", V3, "Idle_5", "loop"),
    ("Walk", V3, "Walking", "loop"),
    ("Run", V3, "Running", "loop"),
    ("Run_Nightmare", V3, "run_fast_2", "loop_in_place"),
    ("Stun_Loop", V3, STUN, "stun_loop"),
    ("Stun_Recover", V3, STUN, "stun_recover"),
    ("Capture_1", CAPTURE, "Backflip", "once"),
    ("Capture_2", CAPTURE, "sliding_rool", "once"),
    ("Capture_3", CAPTURE, "Stylish_Walk", "once"),
    ("Chase_PickUp", V3, "Female_Run_Forward_Pick_Up_Right", "in_place"),
    ("Chase_Charge", V3, "Male_Head_Down_Charge", "in_place"),
    ("Chase_VaultRoll", V3, "Parkour_Vault_with_Roll", "in_place"),
    ("Chase_VaultLand", V3, VAULT, "vault"),
    ("Chase_RunFast", V3, "run_fast_5", "in_place"),
    ("Chase_Slide", V3, "slide_right", "in_place"),
    # not given a role yet: candidates for the scenes (the list's 場面の代用)
    ("BeHit_FlyUp", V3, "BeHit_FlyUp", "once"),
    ("Knock_Down", V3, "Knock_Down", "once"),
    ("Push_Up_To_Idle", V3, "push_up_to_idle", "once"),
)
# restpose is the arms-out bind pose, of no use as a motion. An animation the user adds later is imported as it is,
# under its own name.
SKIPPED = ("restpose",)
CAPTURE_ANIMATIONS = ("Backflip", "sliding_rool", "Stylish_Walk")
# An animation the user's tool made for both models, from which _Retarget measures how one's bones map onto the other's
# (this one's pelvis goes 2.6 m, which fixes the scale; Running's hardly moves).
REFERENCE_ANIMATION = "run_fast_2"
RETARGET_TOLERANCE = (0.5, 0.002)  # degrees and metres the reference may stray from the measured mapping

# The glb's embedded pictures: (the glTF material's texture, our parameter and texture name, sRGB, compression, LOD
# group). The metallic-roughness map is 4096² (the others 2048²) and stays so: the streaming loads the mips drawn.
TEXTURES = (
    ("baseColorTexture", "BaseColor", True, None, "TEXTUREGROUP_Character"),
    ("metallicRoughnessTexture", "MetallicRoughness", False, None, "TEXTUREGROUP_CharacterSpecular"),
    ("normalTexture", "Normal", False, "TC_Normalmap", "TEXTUREGROUP_CharacterNormalMap"),
)


# ------------------------------------------------------------------------------------------------ sources
def make_capture_source(old_glb):
    """Writes CAPTURE_SOURCE from the user's older model: its nodes (the mesh node left empty) and the keys of
    CAPTURE_ANIMATIONS and REFERENCE_ANIMATION as they are, without the mesh, skin or pictures."""
    old, old_blob = gltf.read(old_glb)
    nodes = [{k: v for k, v in n.items() if k not in ("mesh", "skin")} for n in old["nodes"]]
    out = {"asset": {"version": "2.0",
                     "generator": "wasami_tools dd_enemy.make_capture_source, from %s (%s)"
                                  % (os.path.basename(old_glb), old["asset"].get("generator", ""))},
           "scene": old.get("scene", 0), "scenes": old["scenes"], "nodes": nodes, "animations": []}
    blob = bytearray()
    copied = {}

    def copy_of(index):
        if index not in copied:
            copied[index] = gltf.copy_accessor(old, old_blob, index, out, blob)
        return copied[index]

    found = {a["name"]: a for a in old["animations"]}
    for name in CAPTURE_ANIMATIONS + (REFERENCE_ANIMATION,):
        anim = found[name]
        samplers = [{"input": copy_of(s["input"]), "output": copy_of(s["output"]),
                     "interpolation": s.get("interpolation", "LINEAR")} for s in anim["samplers"]]
        out["animations"].append({"name": name, "channels": copy.deepcopy(anim["channels"]), "samplers": samplers})
    gltf.write(CAPTURE_SOURCE, out, blob)
    return CAPTURE_SOURCE


def _content_frames(chans):
    """The frames (from CONTENT_START_FRAME) a clip's bones other than the pelvis are keyed on."""
    end = max(times[-1] for (node, _), (times, _) in chans.items() if node != ROOT_BONE)
    return range(CONTENT_START_FRAME, int(round(end * RATE)) + 1)


def _sample(chans, frames):
    """{track: [value at each source frame]}."""
    return {key: [gltf.sample(key[1], times, values, f / RATE) for f in frames]
            for key, (times, values) in chans.items()}


def _smooth(x):
    x = min(max(x, 0.0), 1.0)
    return x * x * (3.0 - 2.0 * x)


def _crossed(chans, frames_a, frames_b, weights):
    """Keys blended from the source frames frames_a to frames_b by weights (0 keeps a)."""
    a, b = _sample(chans, frames_a), _sample(chans, frames_b)
    return {key: [gltf.blend(key[1], va, vb, w) for va, vb, w in zip(a[key], b[key], weights)] for key in a}


def _pelvis_offset(tracks, offsets):
    """Adds a horizontal (glTF x, z) offset per key to the pelvis's translation."""
    key = (ROOT_BONE, "translation")
    tracks[key] = [(x + dx, y, z + dz) for (x, y, z), (dx, dz) in zip(tracks[key], offsets)]


def _stun_loop(chans):
    a, b = STUN_LOOP_FRAMES
    span, fade = b - a, STUN_BLEND_FRAMES
    own = range(a, b + 1)
    # at the last key, the loop's first (the frames before the fade are not blended in)
    before = [max(f - span, CONTENT_START_FRAME) for f in own]
    weights = [_smooth((k - (span - fade)) / fade) for k in range(span + 1)]
    return _crossed(chans, own, before, weights)


def _stun_recover(chans, target):
    """What follows the loop, crossing in from the loop's continuation, with the pelvis brought over its length to end
    over target (Idle's (x, z)) instead of the source's 0.2 – 0.3 m aside."""
    a, b = STUN_LOOP_FRAMES
    own = range(b, _content_frames(chans)[-1] + 1)
    weights = [_smooth(k / STUN_BLEND_FRAMES) for k in range(len(own))]
    tracks = _crossed(chans, [a + k for k in range(len(own))], own, weights)
    x, _, z = tracks[(ROOT_BONE, "translation")][-1]
    last = len(own) - 1
    _pelvis_offset(tracks, [((target[0] - x) * _smooth(k / last), (target[1] - z) * _smooth(k / last))
                            for k in range(len(own))])
    return tracks, (target[0] - x, target[1] - z)


def _lowest_foot(model, tracks, k):
    """How high (glTF y, metres) the lower of FEET is at key k."""
    rotations = {node: v[k] for (node, path), v in tracks.items() if path == "rotation"}
    translations = {node: v[k] for (node, path), v in tracks.items() if path == "translation"}
    world = gltf.world_transforms(model, rotations, translations)
    return min(world[foot][1][1] for foot in FEET)


def _vault(model, chans):
    """VAULT as a vault from the floor, as the chase has no ledge: cut at VAULT_FRAMES' end, the pelvis lowered by the
    ledge's height (the lower foot's at the first key less at the last) until the take-off and brought back to its own
    by the landing, and all of it turned about the up axis so that it goes straight ahead (+Z) as the other chase
    variants do (the source goes 31° aside). Returns the tracks, the height and the turn in degrees."""
    takeoff, landing, end = VAULT_FRAMES
    frames = range(CONTENT_START_FRAME, end + 1)
    tracks = _sample(chans, frames)
    height = _lowest_foot(model, tracks, 0) - _lowest_foot(model, tracks, -1)
    key = (ROOT_BONE, "translation")
    lowered = [height * (1.0 - _smooth((f - takeoff) / (landing - takeoff))) for f in frames]
    tracks[key] = [(x, y - dy, z) for (x, y, z), dy in zip(tracks[key], lowered)]
    (x0, _, z0), (x1, _, z1) = tracks[key][0], tracks[key][-1]
    turn = -math.degrees(math.atan2(x1 - x0, z1 - z0))
    q = gltf.yaw(turn)
    tracks[(ROOT_BONE, "rotation")] = [gltf.qmul(q, r) for r in tracks[(ROOT_BONE, "rotation")]]
    tracks[key] = [gltf.rotate(q, p) for p in tracks[key]]
    return tracks, height, turn


def _in_place(tracks):
    """Holds the pelvis's horizontal position at its first key. Returns how far it went (x, z) in the source."""
    key = (ROOT_BONE, "translation")
    x0, _, z0 = tracks[key][0]
    x1, _, z1 = tracks[key][-1]
    tracks[key] = [(x0, y, z0) for _, y, _ in tracks[key]]
    return x1 - x0, z1 - z0


class _Retarget:
    """How the older model's motion maps onto v3's bones, measured on REFERENCE_ANIMATION (the user's tool made it for
    both). There each bone's rotation in the scene is the older one's followed by a turn of its own that does not
    change over time (a roll about the bone of up to 22° at the arms, and some swing at the hands: the two models'
    bones point the same way but are rolled differently), and the pelvis's position is v3's rest plus the older one's
    offset from its rest × one scale (0.993, x, y and z alike). The other bones keep v3's lengths."""

    def __init__(self, old, old_tracks, new, new_tracks):
        self.parents = gltf.parents(old)
        bones = [node for node, path in old_tracks if path == "rotation"]
        count = len(next(iter(old_tracks.values())))
        turns = []
        for k in range(count):
            w_old = gltf.world_rotations(old, {b: old_tracks[(b, "rotation")][k] for b in bones})
            w_new = gltf.world_rotations(new, {b: v[k] for (b, path), v in new_tracks.items() if path == "rotation"})
            turns.append({b: gltf.qmul(gltf.qinv(w_old[b]), w_new[b]) for b in bones})
        self.turn = turns[0]
        stray = max(gltf.angle(t[b], self.turn[b]) for t in turns for b in bones)

        # The pelvis: one least-squares scale over x, y and z, each axis with its own offset.
        key = (ROOT_BONE, "translation")
        olds, news = old_tracks[key], new_tracks[key]
        mean_o = [sum(p[a] for p in olds) / count for a in range(3)]
        mean_n = [sum(p[a] for p in news) / count for a in range(3)]
        cov = sum((po[a] - mean_o[a]) * (pn[a] - mean_n[a]) for po, pn in zip(olds, news) for a in range(3))
        var = sum((po[a] - mean_o[a]) ** 2 for po in olds for a in range(3))
        self.scale = cov / var
        self.offset = [mean_n[a] - self.scale * mean_o[a] for a in range(3)]
        miss = max(abs(self._position(po)[a] - pn[a]) for po, pn in zip(olds, news) for a in range(3))
        if stray > RETARGET_TOLERANCE[0] or miss > RETARGET_TOLERANCE[1]:
            raise RuntimeError("%s does not map onto v3 by constant turns and one scale (%.2f°, %.4f m off)"
                               % (REFERENCE_ANIMATION, stray, miss))
        largest = max(gltf.angle(t, (0.0, 0.0, 0.0, 1.0)) for t in self.turn.values())
        self.report = (largest, self.scale, stray, miss)

    def _position(self, p):
        return tuple(self.offset[a] + self.scale * p[a] for a in range(3))

    def apply(self, tracks):
        """The older model's sampled tracks on v3's bones: rotation r of bone b under parent p becomes
        inverse(p's turn) r (b's turn); the pelvis's position is mapped; the other positions are dropped."""
        identity = (0.0, 0.0, 0.0, 1.0)
        out = {}
        for (node, path), values in tracks.items():
            if path == "rotation":
                before = gltf.qinv(self.turn.get(self.parents.get(node), identity))
                after = self.turn[node]
                out[(node, path)] = [gltf.qmul(gltf.qmul(before, r), after) for r in values]
            elif path == "translation" and node == ROOT_BONE:
                out[(node, path)] = [self._position(p) for p in values]
        return out


def prepare():
    """Writes the prepared glb. Returns {role: (seconds, (x, z) the pelvis was moved by, in metres)}."""
    for path in (SOURCE, CAPTURE_SOURCE):
        if not os.path.exists(path):
            raise FileNotFoundError("%s is missing (git lfs pull)" % path)
    model, blob = gltf.read(SOURCE)
    capture, capture_blob = gltf.read(CAPTURE_SOURCE)
    out = copy.deepcopy(model)
    out["animations"] = []
    names = gltf.node_names(model)
    skinned = [n for n in out["nodes"] if "skin" in n]
    if len(skinned) != 1 or len(out["meshes"]) != 1 or len(model["skins"][0]["joints"]) != BONES:
        raise RuntimeError("%s is not one mesh skinned to %d bones" % (SOURCE, BONES))
    skinned[0]["name"] = MESH_NAME
    out["meshes"][0]["name"] = MESH_NAME

    animations = {V3: {a["name"]: a for a in model["animations"]},
                  CAPTURE: {a["name"]: a for a in capture["animations"]}}
    reference_new = gltf.channels(model, blob, animations[V3][REFERENCE_ANIMATION])
    reference_old = gltf.channels(capture, capture_blob, animations[CAPTURE][REFERENCE_ANIMATION])
    frames = _content_frames(reference_new)
    retarget = _Retarget(capture, _sample(reference_old, frames), model, _sample(reference_new, frames))
    unreal.log("enemy: the older model's bones turn by up to %.1f° onto v3's, its pelvis moves × %.4f (the reference "
               "strays %.3f°, %.4f m)" % retarget.report)
    idle = _sample(gltf.channels(model, blob, animations[V3]["Idle_11"]), [CONTENT_START_FRAME])
    target = idle[(ROOT_BONE, "translation")][0][0], idle[(ROOT_BONE, "translation")][0][2]

    roles = list(ROLES)
    used = {name for _, source, name, _ in ROLES if source == V3}
    for name in animations[V3]:
        if name not in used and name not in SKIPPED:
            unreal.log_warning("enemy: %s has no role; imported as it is" % name)
            roles.append((name, V3, name, "once"))

    report = {}
    for role, source, name, how in roles:
        chans = gltf.channels(*((model, blob) if source == V3 else (capture, capture_blob)), animations[source][name])
        missing = {node for node, _ in chans} - set(names)
        if missing:
            raise RuntimeError("%s: bones not in the model: %s" % (name, sorted(missing)))
        moved = (0.0, 0.0)
        if how == "stun_loop":
            tracks = _stun_loop(chans)
        elif how == "stun_recover":
            tracks, moved = _stun_recover(chans, target)
        elif how == "vault":
            tracks, height, turn = _vault(model, chans)
            unreal.log("enemy: %s is lowered by %.1f cm until its take-off and turned by %.1f°"
                       % (name, height * 100.0, turn))
            moved = _in_place(tracks)
        else:
            tracks = _sample(chans, _content_frames(chans))
            if source == CAPTURE:
                if how != "once":
                    raise ValueError("%s: the older model's animations are imported as they are" % role)
                tracks = retarget.apply(tracks)
            if how in ("in_place", "loop_in_place"):
                moved = _in_place(tracks)
            if how in ("loop", "loop_in_place"):
                for values in tracks.values():
                    values.append(values[0])
        gltf.add_animation(out, blob, ANIM_PREFIX + role, tracks, RATE)
        keys = len(next(iter(tracks.values())))
        report[role] = ((keys - 1) / RATE, moved)

    os.makedirs(PREPARED_DIR, exist_ok=True)
    gltf.write(prepared_file(), out, blob)
    return report


def prepared_file():
    # Not named after an asset: Interchange turns the import of a file named as an asset in the folder into a reimport
    # of that asset alone, and a reimport of the mesh leaves the animations as they were.
    return os.path.join(PREPARED_DIR, "WasamiEnemy.glb")


# ------------------------------------------------------------------------------------------------ assets
def ensure_skeletal_pipeline():
    """The glTF assets pipeline for one skinned model with its animations: no sub folders, no materials or textures, no
    static meshes, no Nanite, animations baked at 30 fps, assets named after the glTF's mesh and animations."""
    pipeline = paths.SKELETAL_PIPELINE
    if EAL.does_asset_exist(pipeline):
        pl = unreal.load_asset(pipeline)
        if EAL.get_metadata_tag(pl, dd_stage.VERSION_TAG) == PIPELINE_VERSION:
            return pl
    else:
        EAL.duplicate_asset("/Interchange/Pipelines/DefaultGLTFAssetsPipeline", pipeline)
        pl = unreal.load_asset(pipeline)
    for prop, value in (("asset_type_sub_folders", False), ("scene_name_sub_folder", False),
                        ("use_source_name_for_asset", False), ("asset_name", "")):
        pl.set_editor_property(prop, value)
    mp = pl.get_editor_property("material_pipeline")
    mp.set_editor_property("import_materials", False)
    mp.get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
    mesh = pl.get_editor_property("mesh_pipeline")
    for prop, value in (("import_static_meshes", False), ("import_skeletal_meshes", True), ("build_nanite", False),
                        ("create_physics_asset", True), ("import_morph_targets", False)):
        mesh.set_editor_property(prop, value)
    anim = pl.get_editor_property("animation_pipeline")
    anim.set_editor_property("import_animations", True)
    anim.set_editor_property("use30_hz_to_bake_bone_animation", True)
    EAL.set_metadata_tag(pl, dd_stage.VERSION_TAG, PIPELINE_VERSION)
    EAL.save_asset(pipeline, only_if_is_dirty=False)
    return pl


def _extract_textures():
    """Writes the model's embedded pictures next to the prepared glb and imports them. Returns {parameter: texture}."""
    model, blob = gltf.read(SOURCE)
    material = model["materials"][0]
    slots = dict(material.get("pbrMetallicRoughness", {}))
    slots.update({k: v for k, v in material.items() if k.endswith("Texture")})
    os.makedirs(PREPARED_DIR, exist_ok=True)
    out = {}
    for slot, param, srgb, compression, lod_group in TEXTURES:
        image = model["images"][model["textures"][slots[slot]["index"]]["source"]]
        view = model["bufferViews"][image["bufferView"]]
        start = view.get("byteOffset", 0)
        extension = {"image/jpeg": ".jpg", "image/png": ".png"}[image["mimeType"]]
        name = "T_WasamiEnemy_" + param
        file = os.path.join(PREPARED_DIR, name + extension)
        with open(file, "wb") as f:
            f.write(blob[start:start + view["byteLength"]])
        tex = dd_stage.import_texture({"file": file, "asset": "%s/%s" % (FOLDER, name),
                                       "srgb": srgb, "compression": compression, "lodGroup": lod_group})
        if param == "Normal":
            tex.set_editor_property("flip_green_channel", True)  # glTF's normal maps point Y up, UE's down
        EAL.save_loaded_asset(tex, only_if_is_dirty=False)
        out[param] = tex
    return out


def _build_master(mat, textures):
    """glTF's metallic-roughness material with every factor at 1 (the glb's): the base colour, metallic from B and
    roughness from G of the metallic-roughness map, and the normal map."""
    mat.set_editor_property("used_with_skeletal_mesh", True)
    g = dd_stage._Graph(mat, checked=True)
    tcs = unreal.MaterialSamplerType
    base = g.texture("BaseColor", textures["BaseColor"], tcs.SAMPLERTYPE_COLOR, -600, -300)
    g.out(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    packed = g.texture("MetallicRoughness", textures["MetallicRoughness"], tcs.SAMPLERTYPE_LINEAR_COLOR, -600, 0)
    g.out(packed, "B", unreal.MaterialProperty.MP_METALLIC)
    g.out(packed, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    normal = g.texture("Normal", textures["Normal"], tcs.SAMPLERTYPE_NORMAL, -600, 300)
    g.out(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)


def _import_model(material):
    """Imports the prepared glb and gives the mesh its material. Returns (mesh, {role: animation})."""
    ensure_skeletal_pipeline()
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines = [unreal.SoftObjectPath(paths.object_path(paths.SKELETAL_PIPELINE))]
    src = unreal.InterchangeManager.create_source_data(prepared_file())
    if not unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(FOLDER, src, params):
        raise RuntimeError("the import of %s failed (see the output log)" % prepared_file())
    mesh = unreal.load_asset(MESH)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("the import made no SkeletalMesh at %s" % MESH)
    materials = mesh.get_editor_property("materials")
    if len(materials) != 1:
        raise RuntimeError("%s has %d material slots, the glb one" % (MESH, len(materials)))
    slot = materials[0]
    slot.set_editor_property("material_interface", material)
    materials[0] = slot
    mesh.set_editor_property("materials", materials)
    anims = {}
    for role in [r[0] for r in ROLES]:
        anim = unreal.load_asset(FOLDER + "/" + ANIM_PREFIX + role)
        if not isinstance(anim, unreal.AnimSequence):
            raise RuntimeError("the import made no animation %s%s (see the output log)" % (ANIM_PREFIX, role))
        anims[role] = anim
    return mesh, anims


def import_all():
    """Prepares and imports the enemy Wasami, then saves /Game/Wasami/Enemy and the master. Returns how many of each
    kind, and logs each animation's length and how far its pelvis was moved."""
    report = prepare()
    for role, (seconds, (dx, dz)) in report.items():
        unreal.log("enemy: %s%s %.3f s, pelvis moved x %.2f m z %.2f m" % (ANIM_PREFIX, role, seconds, dx, dz))
    textures = _extract_textures()
    master = dd_assets.material(MASTER, lambda mat: _build_master(mat, textures))
    instance = dd_assets.material_instance(MATERIAL, master,
                                           textures={p: t.get_path_name().split(".")[0] for p, t in textures.items()})
    mesh, anims = _import_model(instance)
    for asset in [master, instance, mesh]:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    return {"textures": len(textures), "materials": 2, "meshes": 1, "animations": len(report)}
