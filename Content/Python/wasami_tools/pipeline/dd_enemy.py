"""The enemy Wasami (AWasamiEnemy): its skeletal mesh, material and animations, from this game's own model.

  sources   SourceArt/Wasami/enemy_wasami_v3.glb, the user's model (28 bones rooted at pelvis, no root bone; 16
            animations), and SourceArt/Wasami/enemy_wasami_capture.glb, the capture's three animations of the user's
            older model with the run_fast_2 both models have (its bones and those animations only: make_capture_source
            took them out of the user's tmp/enemy_wasami.glb)
  prepared  Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb: the model with the animations the code asks for by
            role (ROLES, .claude/references/enemy-wasami-motions.md), each resampled on a 30 fps grid from 0 — the
            sources' keys mix 24 and 30 fps and start at 1/24 s, and Interchange refuses an animation that does not end
            on a frame. Loops are closed (their last key is their first), the chase variants and the Nightmare run are
            made in place, the stun's falls start where Idle stands and each has a get-up (a roll onto the front, then
            push_up_to_idle) that ends where Idle stands (_stun_get_up), the vault off a ledge is made a vault from the
            floor (VAULT_FRAMES), and the capture's are carried onto v3's bones (_Retarget).
  imported  /Game/Wasami/Enemy: SK_WasamiEnemy with SK_WasamiEnemy_Skeleton and SK_WasamiEnemy_PhysicsAsset,
            A_WasamiEnemy_<role>, T_WasamiEnemy_* and MI_WasamiEnemy (of M_DD_WasamiGltf, glTF's metallic-roughness
            material). The mesh faces +Y, as UE's mannequins do.

The capture's sounds come from here too (CAPTURE_SOUNDS): they are the original's own waves, not this game's model, but
they belong to what the enemy does when it catches the player (AWasamiCapture).
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

# What the capture plays (AWasamiCapture, which names them by their paths under /Game/DD): the scream every one of
# 01_Hotel's jumpscare Matinees opens with, and the laugh and the axe hit of 03_Manor's Gold Watcher kill the face
# follows. All three are the old version's (pak_reference).
CAPTURE_SOUNDS = (
    "Audio/01_Hotel/Evil_Monkey_Scream",
    "Audio/03_Manor/LIVING_STATUE_Laughter_05",
    "Audio/03_Manor/Axe_Hit_03",
)
CAPTURE_SOUND_VERSION = 1
PIPELINE_VERSION = "1"  # bump when ensure_skeletal_pipeline's settings change

ROOT_BONE = "pelvis"
BONES = 28
RATE = 30
# Every bone's first key is at 2/30 s (the pelvis has one at 1/24 s as well, from the 24 fps scene the 30 fps motion was
# baked into); a clip's content runs from there to the last key of its other bones.
CONTENT_START_FRAME = 2

# The stun (the user's answers, 2026-09-18): BeHit_FlyUp or Knock_Down at random, lying, then push_up_to_idle. Both
# falls end on the back with the head where the body's back was; push_up_to_idle starts face down with the head where
# its front is. So the get-up rolls the fallen body over about its length (pelvis to neck) in ROLL_FRAMES and lays the
# push-up along it, turned end for end. It ends on its feet turned away from where the fall started (and Knock_Down's
# some 0.7 m behind), so the whole get-up is turned and moved to end where Idle stands, as Idle is turned; the enemy is
# moved by as much, while it lies still, as the get-up starts (UWasamiEnemyAnimInstance).
GET_UP = "push_up_to_idle"
# 0.8 s, Claude's (the user's answer: a roll Claude makes, looked at in PIE); in PIE neither it nor the limbs' shares
# went into the floor or jumped (2026-09-18), so they stay.
ROLL_FRAMES = 24
# The share of the roll in which the limbs reach GET_UP's first pose (the candidates), and how far (m) a joint may go
# below the floor before the body is lifted (the joints are the bones' heads; the fingertips' reach the skin's end).
ROLL_LIMB_SHARES = (1.0, 0.75, 0.5, 0.35)
ROLL_FLOOR_SLACK = 0.03
NECK = "neck_01"
# The v3 stun the user first chose (a bent-over sway, then straightening up), unused since 2026-09-18.
OLD_STUN = "01a0a88f-0db6-7251-85e6-1a97b799ee52"

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
#   stun_fall      moved to start where Idle stands (_stun_fall)
#   stun_get_up    the fall's get-up (_stun_get_up)
#   vault          see _vault
V3, CAPTURE = "v3", "capture"
ROLES = (
    ("Idle", V3, "Idle_11", "loop"),
    ("Idle_Alert", V3, "Idle_5", "loop"),
    ("Walk", V3, "Walking", "loop"),
    ("Run", V3, "Running", "loop"),
    ("Run_Nightmare", V3, "run_fast_2", "loop_in_place"),
    ("Stun_FlyUp", V3, "BeHit_FlyUp", "stun_fall"),
    ("Stun_KnockDown", V3, "Knock_Down", "stun_fall"),
    ("Stun_GetUp_FlyUp", V3, "BeHit_FlyUp", "stun_get_up"),
    ("Stun_GetUp_KnockDown", V3, "Knock_Down", "stun_get_up"),
    ("Capture_1", CAPTURE, "Backflip", "once"),
    ("Capture_2", CAPTURE, "sliding_rool", "once"),
    ("Capture_3", CAPTURE, "Stylish_Walk", "once"),
    ("Chase_PickUp", V3, "Female_Run_Forward_Pick_Up_Right", "in_place"),
    ("Chase_Charge", V3, "Male_Head_Down_Charge", "in_place"),
    ("Chase_VaultRoll", V3, "Parkour_Vault_with_Roll", "in_place"),
    ("Chase_VaultLand", V3, VAULT, "vault"),
    ("Chase_RunFast", V3, "run_fast_5", "in_place"),
    ("Chase_Slide", V3, "slide_right", "in_place"),
)
# restpose is the arms-out bind pose, of no use as a motion; OLD_STUN is not used. An animation the user adds later is
# imported as it is, under its own name.
SKIPPED = ("restpose", OLD_STUN)
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


def _pelvis_offset(tracks, offsets):
    """Adds a horizontal (glTF x, z) offset per key to the pelvis's translation."""
    key = (ROOT_BONE, "translation")
    tracks[key] = [(x + dx, y, z + dz) for (x, y, z), (dx, dz) in zip(tracks[key], offsets)]


def _heading(v):
    """The degrees a horizontal direction (glTF x, z) is turned from +Z towards +X (gltf.yaw's sense)."""
    return math.degrees(math.atan2(v[0], v[2]))


def _turn_pelvis(tracks, q):
    """Turns the whole pose about the up axis through the origin (the pelvis is the root)."""
    rot, pos = (ROOT_BONE, "rotation"), (ROOT_BONE, "translation")
    tracks[rot] = [gltf.qmul(q, r) for r in tracks[rot]]
    tracks[pos] = [gltf.rotate(q, p) for p in tracks[pos]]


def _move_pelvis_to(tracks, k, target):
    """Moves the whole pose horizontally so that the pelvis is over target (x, z) at key k."""
    x, _, z = tracks[(ROOT_BONE, "translation")][k]
    _pelvis_offset(tracks, [(target[0] - x, target[1] - z)] * len(tracks[(ROOT_BONE, "translation")]))


def _key(tracks, k):
    return {key: values[k] for key, values in tracks.items()}


def _body_axis(model, pose):
    """The horizontal direction (glTF x, 0, z, of length 1) from the pelvis to the neck in a pose {track: value}."""
    world = gltf.world_transforms(model, {n: v for (n, p), v in pose.items() if p == "rotation"},
                                  {n: v for (n, p), v in pose.items() if p == "translation"})
    (px, _, pz), (nx, _, nz) = world[ROOT_BONE][1], world[NECK][1]
    length = math.hypot(nx - px, nz - pz)
    return ((nx - px) / length, 0.0, (nz - pz) / length)


def _axis_angle(axis, radians):
    s = math.sin(radians / 2.0)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(radians / 2.0))


def _stun_fall(chans, target):
    """A fall, moved to start with the pelvis over target (Idle's (x, z)). Returns the tracks and how far the pelvis
    goes (x, z)."""
    tracks = _sample(chans, _content_frames(chans))
    _move_pelvis_to(tracks, 0, target)
    (x0, _, z0), (x1, _, z1) = tracks[(ROOT_BONE, "translation")][0], tracks[(ROOT_BONE, "translation")][-1]
    return tracks, (x1 - x0, z1 - z0)


def _stun_get_up(model, fall, get_up_chans, target, forward):
    """The get-up after the fall's tracks: the fallen body rolls onto its front about its length (pelvis to neck) over
    ROLL_FRAMES, into GET_UP's first pose laid along it (turned so that its pelvis-to-neck runs the same way, its pelvis
    where the fall's lies), then GET_UP. The whole is turned and moved so that it ends with the pelvis over target and
    turned to forward (Idle's: the pelvis's own axis that points ahead at Idle's first key). Returns the tracks and the
    turn (degrees) and move (x, z) that carry the fall's last pose to the get-up's first."""
    last = _key(fall, -1)
    push = _sample(get_up_chans, _content_frames(get_up_chans))
    _turn_pelvis(push, gltf.yaw(_heading(_body_axis(model, last)) - _heading(_body_axis(model, _key(push, 0)))))
    _move_pelvis_to(push, 0, (last[(ROOT_BONE, "translation")][0], last[(ROOT_BONE, "translation")][2]))
    first = _key(push, 0)

    # The roll: the pelvis turns half round about the body's length, and what is left of the turn to GET_UP's first pose
    # comes in along with it; the other bones go over to theirs sooner, so that the arms flung out on the back are drawn
    # in before the body rolls onto one. Of the two ways round and ROLL_LIMB_SHARES, the one whose joints go least
    # below the floor; what still goes lower than ROLL_FLOOR_SLACK lifts the body.
    axis = _body_axis(model, last)
    joints = _joint_names(model)
    best = None
    for sign in (1.0, -1.0):
        for share in ROLL_LIMB_SHARES:
            frames = _roll(last, first, axis, sign, share)
            low = [_lowest_joint(model, joints, pose) for pose in frames]
            if best is None or min(low) > min(best[1]):
                best = (frames, low, sign, share)
    frames, low, sign, share = best
    # The ends are the fall's and GET_UP's own poses: they are allowed their own lows and are not lifted.
    last_k = len(low) - 1
    floors = [min(-ROLL_FLOOR_SLACK, low[0] + (low[-1] - low[0]) * k / last_k) for k in range(len(low))]
    lifts = [max(0.0, f - h) for f, h in zip(floors, low)]
    lifts = [max(lifts[max(0, k - 3):k + 4]) * min(1.0, k / 3.0, (last_k - k) / 3.0)  # held a little either side
             for k in range(len(lifts))]
    for pose, lift in zip(frames, lifts):
        x, y, z = pose[(ROOT_BONE, "translation")]
        pose[(ROOT_BONE, "translation")] = (x, y + lift, z)
    tracks = {key: [pose[key] for pose in frames] + push[key][1:] for key in push}

    # Where Idle stands, as Idle is turned.
    end = gltf.rotate(tracks[(ROOT_BONE, "rotation")][-1], forward)
    turn = -_heading(end)
    _turn_pelvis(tracks, gltf.yaw(turn))
    _move_pelvis_to(tracks, -1, target)
    lx, _, lz = gltf.rotate(gltf.yaw(turn), last[(ROOT_BONE, "translation")])
    fx, _, fz = tracks[(ROOT_BONE, "translation")][0]
    report = {"turn": turn, "move": (fx - lx, fz - lz), "way": sign, "limbs": share, "lowest": min(low),
              "lift": max(lifts)}
    return tracks, report


def _roll(last, first, axis, sign, share):
    """The roll's poses {track: value}, ROLL_FRAMES + 1 of them from last to first (see _stun_get_up)."""
    identity = (0.0, 0.0, 0.0, 1.0)
    r0, r1 = last[(ROOT_BONE, "rotation")], first[(ROOT_BONE, "rotation")]
    rest = gltf.qmul(r1, gltf.qinv(gltf.qmul(_axis_angle(axis, sign * math.pi), r0)))
    frames = []
    for k in range(ROLL_FRAMES + 1):
        s = _smooth(k / ROLL_FRAMES)
        limbs = _smooth(k / (ROLL_FRAMES * share))
        pose = {}
        for (node, path), value in last.items():
            if (node, path) == (ROOT_BONE, "rotation"):
                pose[(node, path)] = gltf.qmul(gltf.slerp(identity, rest, s),
                                               gltf.qmul(_axis_angle(axis, sign * math.pi * s), r0))
            elif node == ROOT_BONE:
                pose[(node, path)] = gltf.blend(path, value, first[(node, path)], s)
            else:
                pose[(node, path)] = gltf.blend(path, value, first[(node, path)], limbs)
        frames.append(pose)
    return frames


def _joint_names(model):
    names = gltf.node_names(model)
    return {names[j] for j in model["skins"][0]["joints"]}


def _lowest_joint(model, joints, pose):
    """How high (glTF y) the lowest joint of a pose {track: value} is."""
    world = gltf.world_transforms(model, {n: v for (n, p), v in pose.items() if p == "rotation"},
                                  {n: v for (n, p), v in pose.items() if p == "translation"})
    return min(world[j][1][1] for j in joints)


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
    forward = gltf.rotate(gltf.qinv(idle[(ROOT_BONE, "rotation")][0]), (0.0, 0.0, 1.0))
    get_up = gltf.channels(model, blob, animations[V3][GET_UP])

    roles = list(ROLES)
    used = {name for _, source, name, _ in ROLES if source == V3} | {GET_UP}
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
        if how == "stun_fall":
            tracks, moved = _stun_fall(chans, target)
        elif how == "stun_get_up":
            tracks, got = _stun_get_up(model, _stun_fall(chans, target)[0], get_up, target, forward)
            moved = got["move"]
            unreal.log("enemy: %s's get-up turns the enemy by %.1f° about the origin, then moves it x %.2f m z %.2f m, "
                       "as it starts; it rolls %s with the limbs in over %.0f %% of it, its lowest joint %.3f m, lifted "
                       "%.3f m" % (name, got["turn"], *moved, "one way" if got["way"] > 0 else "the other way",
                                   got["limbs"] * 100.0, got["lowest"], got["lift"]))
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
def ensure_skeletal_pipeline(pipeline=paths.SKELETAL_PIPELINE, sample_rate=None):
    """The glTF assets pipeline for one skinned model with its animations: no sub folders, no materials or textures, no
    static meshes, no Nanite, animations baked at 30 fps (or at sample_rate), assets named after the glTF's mesh and
    animations."""
    version = PIPELINE_VERSION if sample_rate is None else "%s@%d" % (PIPELINE_VERSION, sample_rate)
    if EAL.does_asset_exist(pipeline):
        pl = unreal.load_asset(pipeline)
        if EAL.get_metadata_tag(pl, dd_stage.VERSION_TAG) == version:
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
    anim.set_editor_property("use30_hz_to_bake_bone_animation", sample_rate is None)
    anim.set_editor_property("custom_bone_animation_sample_rate", sample_rate or 0)
    EAL.set_metadata_tag(pl, dd_stage.VERSION_TAG, version)
    EAL.save_asset(pipeline, only_if_is_dirty=False)
    return pl


def _extract_textures(source=SOURCE, prepared_dir=PREPARED_DIR, folder=FOLDER, prefix="T_WasamiEnemy_"):
    """Writes a model's embedded pictures next to its prepared glb and imports them as <folder>/<prefix><parameter>
    (the enemy's by default; the boss's have the same slots). Returns {parameter: texture}."""
    model, blob = gltf.read(source)
    material = model["materials"][0]
    slots = dict(material.get("pbrMetallicRoughness", {}))
    slots.update({k: v for k, v in material.items() if k.endswith("Texture")})
    os.makedirs(prepared_dir, exist_ok=True)
    out = {}
    for slot, param, srgb, compression, lod_group in TEXTURES:
        image = model["images"][model["textures"][slots[slot]["index"]]["source"]]
        view = model["bufferViews"][image["bufferView"]]
        start = view.get("byteOffset", 0)
        extension = {"image/jpeg": ".jpg", "image/png": ".png"}[image["mimeType"]]
        name = prefix + param
        file = os.path.join(prepared_dir, name + extension)
        with open(file, "wb") as f:
            f.write(blob[start:start + view["byteLength"]])
        tex = dd_stage.import_texture({"file": file, "asset": "%s/%s" % (folder, name),
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


def _import_model(material, prepared=None, folder=FOLDER, mesh_path=MESH, anim_prefix=ANIM_PREFIX, roles=None):
    """Imports a prepared glb (the enemy's by default) into folder and gives the mesh at mesh_path its material. Returns
    (mesh, {role: animation}) of roles (the enemy's ROLES by default)."""
    prepared = prepared or prepared_file()
    ensure_skeletal_pipeline()
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines = [unreal.SoftObjectPath(paths.object_path(paths.SKELETAL_PIPELINE))]
    src = unreal.InterchangeManager.create_source_data(prepared)
    if not unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(folder, src, params):
        raise RuntimeError("the import of %s failed (see the output log)" % prepared)
    mesh = unreal.load_asset(mesh_path)
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("the import made no SkeletalMesh at %s" % mesh_path)
    materials = mesh.get_editor_property("materials")
    if len(materials) != 1:
        raise RuntimeError("%s has %d material slots, the glb one" % (mesh_path, len(materials)))
    slot = materials[0]
    slot.set_editor_property("material_interface", material)
    materials[0] = slot
    mesh.set_editor_property("materials", materials)
    anims = {}
    for role in roles or [r[0] for r in ROLES]:
        anim = unreal.load_asset(folder + "/" + anim_prefix + role)
        if not isinstance(anim, unreal.AnimSequence):
            raise RuntimeError("the import made no animation %s%s (see the output log)" % (anim_prefix, role))
        anims[role] = anim
    return mesh, anims


def import_capture_sounds():
    """The waves the capture plays, with the original's own settings (dd_assets.sound). Returns their package paths."""
    return [dd_assets.sound(rel, CAPTURE_SOUND_VERSION) for rel in CAPTURE_SOUNDS]


def import_all():
    """Prepares and imports the enemy Wasami and the capture's sounds, then saves /Game/Wasami/Enemy and the master.
    Returns how many of each kind, and logs each animation's length and how far its pelvis was moved."""
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
    sounds = import_capture_sounds()
    return {"textures": len(textures), "materials": 2, "meshes": 1, "animations": len(report), "sounds": len(sounds)}
