"""The enemy Wasami (AWasamiEnemy): its skeletal mesh, material and animations, from this game's own model.

  sources   SourceArt/Wasami/enemy_wasami_v3.glb, the user's model (28 bones rooted at pelvis, no root bone; 16
            animations), SourceArt/Wasami/enemy_wasami_capture.glb, the capture's three animations of the user's
            older model with the run_fast_2 both models have (its bones and those animations only: make_capture_source
            took them out of the user's tmp/enemy_wasami.glb), and the original's nurse's own animations as ActorX
            files under pak_reference_2/_anims_psa (the cut scenes' acting; NURSE_BONES, _NurseRetarget)
  prepared  Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb: the model with the animations the code asks for by
            role (ROLES, .claude/references/enemy-wasami-motions.md), each resampled on a 30 fps grid from 0 — the
            sources' keys mix 24 and 30 fps and start at 1/24 s, and Interchange refuses an animation that does not end
            on a frame. Loops are closed (their last key is their first), the chase variants and the Nightmare run are
            made in place, the stun's falls start where Idle stands and each has a get-up (a roll onto the front, then
            push_up_to_idle) that ends where Idle stands (_stun_get_up), the vault off a ledge is made a vault from the
            floor (VAULT_FRAMES), and the capture's and the nurse's are carried onto v3's bones
            (_Retarget, _NurseRetarget).
  imported  /Game/Wasami/Enemy: SK_WasamiEnemy with SK_WasamiEnemy_Skeleton and SK_WasamiEnemy_PhysicsAsset,
            A_WasamiEnemy_<role> (the cut scenes' are A_WasamiEnemy_Cut_<the original's animation>),
            T_WasamiEnemy_* and MI_WasamiEnemy (of M_DD_WasamiGltf, glTF's metallic-roughness
            material), and M_WasamiCaptureBlack, the black walls of the capture's room. The mesh faces +Y, as UE's
            mannequins do.

The enemy's own sound comes from here too: the loop it moves to (MOVE_SOUND). It is the original's own wave, not
this game's model, but it belongs to what the enemy does (AWasamiEnemy). What the capture cries is one of the
Wasami's voices, imported with the rest of them (dd_voices).
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
# The black room the capture plays in (AWasamiCapture's walls). The original's jumpscareblock uses
# /Engine/EngineDebugMaterials/BlackUnlitMaterial, which is an editor-only debug material the cook leaves out:
# in a packaged build the walls fall back to the default checkerboard, and the capture plays over a grid instead
# of the dark. This is the same material -- unlit, emissive 0 -- in this game's own content, so it is cooked.
CAPTURE_WALL_MATERIAL = FOLDER + "/M_WasamiCaptureBlack"

# The loop the enemy moves to (AWasamiEnemy's Skate Audio, the nurse's), through the attenuation the original gives it
# (dd_gimmicks makes the same one for the doors; making it again is harmless). The hospital is only in the latest
# version, so this one is pak_reference_2's.
MOVE_SOUND = "Audio/06_Hospital/DD_Rollerskating_Fast_V1_LOOP"
MOVE_ATTENUATION = "Audio/Misc/MonkeyAttenuation"
# What the enemy's lines play through (AWasamiEnemy's Talk Audio, the nurse's, which holds no sound of its own: Talk
# gives it one as the enemy speaks, this game's Wasami voices).
TALK_ATTENUATION = "Audio/Misc/AgathaAttenuation"
ENEMY_ATTENUATIONS = (MOVE_ATTENUATION, TALK_ATTENUATION)
MOVE_SOUND_VERSION = 2
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

# The hands' twist (the review's finding, the work list's item 37). SOURCE is modelled with its forearms turned over:
# its bind pose has the palms up, while its animations are of a body whose palms face down, and they hardly twist the
# arms away from the bind pose, so in play both palms face outwards (the least inwards any palm faces in a clip is
# -0.96, where +1 is straight in to the body). The boss's model, the same skeleton by the same hand, has the palms down
# in its bind pose and faces them inwards in every clip. The user's answer (2026-09-23) was to correct it here rather
# than in their model, so each hand is turned back about the forearm's length by as much as separates the two models'
# rest poses: `python Tools/wasami_hands.py twist` reads each palm in the lowerarm's own frame and measures the turn
# from v3's onto the boss's (+170.1 deg left, -172.2 deg right; the clips both models have agree within 9 deg).
# The forearm's length is the lowerarm's own +X -- every hand node's rest offset lies along it -- so the turn goes in
# the lowerarm's frame, ahead of the hand's own rotation: the wrist does not move, it only twists, and the fingers
# come with it. It is applied to every clip once, after the capture's have been carried onto v3's bones (_Retarget
# measures that mapping on the untwisted reference, so the two do not stack).
# Half a turn at one joint is what linear blend skinning cannot do: where the two bones' weights meet, the average of
# rotations 180 deg apart has no width left at all, and the wrist pinches to a thread (looked at in renders). So the
# turn is shared with the forearm, which carries the hand with it: ELBOW_SHARE of it goes to the lowerarm, about its
# own length, and the rest to the hand, and each seam only narrows by cos(half its share). Half and half leaves each
# 0.74 of its width, and the elbow's seam is under the coat's sleeve where the wrist's is bare skin.
HAND_TWIST = {"hand_l": 170.1, "hand_r": -172.2}
ELBOW = {"hand_l": "lowerarm_l", "hand_r": "lowerarm_r"}
ELBOW_SHARE = 0.5
FOREARM_AXIS = (1.0, 0.0, 0.0)

# (role, source, glTF animation, how):
#   loop           closed: the first key is repeated after the last (the sources' loops stop a frame short)
#   once           as it is
#   in_place       the pelvis's horizontal motion (glTF x and z) held at its first key; its height stays
#   loop_in_place  both
#   stun_fall      moved to start where Idle stands (_stun_fall)
#   stun_get_up    the fall's get-up (_stun_get_up)
#   vault          see _vault
V3, CAPTURE, NURSE = "v3", "capture", "nurse"
# Where the original's nurse's animations live (pak_reference_2): her own, and the hospital's dialogue takes.
NURSE_REAPER = "Animation/Enemies/Nurse/Reaper/"
NURSE_STANDING = "Animation/06_Hospital/NurseIntro/Anims/StandingAnims/"
ROLES = (
    ("Idle", V3, "Idle_11", "loop"),
    # The sentries' idle, which only they play (bAggressiveIdle), and the waiting the capture scene opens on: the
    # original's own alert idle, carried onto v3 (the work list's item 54 (a)). v3's Idle_5 stood here, and it holds
    # the arms out to the sides where the original's holds the syringe low, which read as six Wasami with their hands
    # up over the lobby.
    ("Idle_Alert", NURSE, NURSE_REAPER + "ReaperNurse_Idle_Alert", "loop"),
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
    # The cell scene's acting, the original's nurse's own (the work list's item 54): she waits, speaks eight takes
    # over the dialogue, backs away and cloaks. A nurse clip is named by the original's animation, and its path says
    # where the psa is read from and which AnimSequence gives its frame rate. What is left out is the acting the
    # user's decision of 2026-09-26 keeps as it is: the punch (Event_39, the syringe brought down) and the Matron's.
    # Event_43 is here because its raise is a gesture, not a swing: its right wrist reaches 7 cm over her head once,
    # at 4.9 m/s, where the punch's reaches 4 cm at 11.6 m/s and comes down at 4.9 m/s (nurse_ev43_probe).
    ("Cut_nurse_idle_01", NURSE, NURSE_REAPER + "nurse_idle_01", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_40", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_40", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_41", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_41", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_42", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_42", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_43", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_43", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_44", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_44", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_45", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_45", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_46", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_46", "once"),
    ("Cut_Nurse_Hospital_Zone01_Event_47", NURSE, NURSE_STANDING + "Nurse_Hospital_Zone01_Event_47", "once"),
    ("Cut_ReaperNurse_Walk_Back", NURSE, NURSE_REAPER + "ReaperNurse_Walk_Back", "once"),
    ("Cut_nurse_cloak", NURSE, NURSE_REAPER + "nurse_cloak", "once"),
)
# restpose is the arms-out bind pose, of no use as a motion; OLD_STUN is not used; Idle_5 was the sentries' idle until
# the original's own took its place (item 54 (a)), and nothing else plays it. An animation the user adds later is
# imported as it is, under its own name.
SKIPPED = ("restpose", OLD_STUN, "Idle_5")
CAPTURE_ANIMATIONS = ("Backflip", "sliding_rool", "Stylish_Walk")
# An animation the user's tool made for both models, from which _Retarget measures how one's bones map onto the other's
# (this one's pelvis goes 2.6 m, which fixes the scale; Running's hardly moves).
REFERENCE_ANIMATION = "run_fast_2"
RETARGET_TOLERANCE = (0.5, 0.002)  # degrees and metres the reference may stray from the measured mapping

# The original's nurse onto v3, bone by bone: (v3's bone, hers, and the tips the two point at). Her rig has 92 bones
# where v3 has 28, so a pair's tips say which of her segments is which of v3's: a tip is the next bone down the mapped
# chain, or the rig's own end bone where the chain stops. Her five spine bones are taken as three (Spine_01, Spine_03,
# Spine_Top), which splits her six spine segments as 1, 2, 2, 1 over v3's four -- Spine_01/02/Top would make it 1, 1,
# 3, 1 -- and her clavicles hang off Spine_Top as v3's do off spine_03. Her Spine_04, Neck_02, the twenty fingers, the
# six skirt and four wheel bones, the four at the waist, the weapon's, the trajectory's and the mesh's nodes are
# dropped: what they do reaches v3 through the bones above them, or not at all.
NURSE_BONES = (
    ("pelvis", "Nurse_ROOTSHJnt", "spine_01", "Nurse_Spine_01SHJnt"),
    ("spine_01", "Nurse_Spine_01SHJnt", "spine_02", "Nurse_Spine_03SHJnt"),
    ("spine_02", "Nurse_Spine_03SHJnt", "spine_03", "Nurse_Spine_TopSHJnt"),
    ("spine_03", "Nurse_Spine_TopSHJnt", "neck_01", "Nurse_Neck_01SHJnt"),
    ("neck_01", "Nurse_Neck_01SHJnt", "head", "Nurse_Neck_TopSHJnt"),
    ("head", "Nurse_Neck_TopSHJnt", "head_end", "Nurse_TopOfHead_AuxSHJnt"),
    ("clavicle_l", "Nurse_l_Arm_ClavicleSHJnt", "upperarm_l", "Nurse_l_Arm_ShoulderSHJnt"),
    ("upperarm_l", "Nurse_l_Arm_ShoulderSHJnt", "lowerarm_l", "Nurse_l_Arm_ElbowSHJnt"),
    ("lowerarm_l", "Nurse_l_Arm_ElbowSHJnt", "hand_l", "Nurse_l_Arm_WristSHJnt"),
    ("hand_l", "Nurse_l_Arm_WristSHJnt", "LeftHand_End", "Nurse_l_Finger_03_01SHJnt"),
    ("clavicle_r", "Nurse_r_Arm_ClavicleSHJnt", "upperarm_r", "Nurse_r_Arm_ShoulderSHJnt"),
    ("upperarm_r", "Nurse_r_Arm_ShoulderSHJnt", "lowerarm_r", "Nurse_r_Arm_ElbowSHJnt"),
    ("lowerarm_r", "Nurse_r_Arm_ElbowSHJnt", "hand_r", "Nurse_r_Arm_WristSHJnt"),
    ("hand_r", "Nurse_r_Arm_WristSHJnt", "RightHand_End", "Nurse_r_Finger_03_01SHJnt"),
    ("thigh_l", "Nurse_l_Leg_HipSHJnt", "calf_l", "Nurse_l_Leg_KneeSHJnt"),
    ("calf_l", "Nurse_l_Leg_KneeSHJnt", "foot_l", "Nurse_l_Leg_AnkleSHJnt"),
    ("foot_l", "Nurse_l_Leg_AnkleSHJnt", "ball_l", "Nurse_l_Leg_BallSHJnt"),
    ("ball_l", "Nurse_l_Leg_BallSHJnt", "LeftToe_end", "Nurse_l_Leg_ToeSHJnt"),
    ("thigh_r", "Nurse_r_Leg_HipSHJnt", "calf_r", "Nurse_r_Leg_KneeSHJnt"),
    ("calf_r", "Nurse_r_Leg_KneeSHJnt", "foot_r", "Nurse_r_Leg_AnkleSHJnt"),
    ("foot_r", "Nurse_r_Leg_AnkleSHJnt", "ball_r", "Nurse_r_Leg_BallSHJnt"),
    ("ball_r", "Nurse_r_Leg_BallSHJnt", "RightToe_end", "Nurse_r_Leg_ToeSHJnt"),
)
NURSE_VERSION = 2       # the hospital, and this nurse, are only in the latest version

# The cloak the original's nurse dissolves into: M_06_Nurse_Body over the Basic Stealth System's M_BSS_Character1,
# whose graph (and its material function MF_BSS_Energy1's) the cook took away, read back from its compiled shaders
# (_build_cloak). The two noises are the original's own textures, of the latest version (the hospital is only there).
# The cut scenes' material track drives Efficiency (dd_sequence); nothing else does, so the enemy is solid in play.
CLOAK_NOISE = {"Fast": "ThirdParty/BasicStealthSystem/Textures/BSS_Noise2",
               "Slow": "ThirdParty/BasicStealthSystem/Textures/BSS_Noise1"}
CLOAK_NOISE_SPEED = {"Fast": (0.08, 0.07), "Slow": (0.02, -0.05)}   # how far each noise pans a second
CLOAK_NOISE_VERSION = 2
# M_06_Nurse_Body's own values. (The original splits them: M_BSS_Character1 holds a master's defaults, which are
# another game's, and the nurse's instance overrides all of them. This game has the one Wasami material and the nurse
# is the only thing that cloaks, so the values that are used are the defaults.)
CLOAK_SCALARS = {"Efficiency": 0.0, "BestEfficiency": 1.0, "WorstEfficiency": 0.0,
                 "FringeSize": 0.1, "GlowIntensity": 25.0}
CLOAK_GLOW_COLOR = (1.0, 0.0, 0.0, 1.0)
CLOAK_MASK_CLIP = 0.3333          # the nurse's OpacityMaskClipValue (its BlendMode is Masked as well)

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


def _twist_hands(tracks):
    """Turns each hand about its forearm's length by HAND_TWIST, ELBOW_SHARE of it taken by the forearm (see them).
    In place.

    The forearm's length is the lowerarm's own +X, so the forearm's share is a turn in the lowerarm's own frame (after
    its rotation) and the hand's is one in the lowerarm's frame (before the hand's). Neither moves a joint: both the
    elbow's and the wrist's offsets from their parent lie along the axis they turn about. The hand hangs off the
    forearm, so it is already carried by the forearm's share and only needs what is left."""
    for bone, degrees in HAND_TWIST.items():
        elbow = (ELBOW[bone], "rotation")
        for key in (elbow, (bone, "rotation")):
            if key not in tracks:
                raise RuntimeError("%s is not keyed: the hands cannot be twisted" % key[0])
        at_elbow = _axis_angle(FOREARM_AXIS, math.radians(degrees * ELBOW_SHARE))
        at_wrist = _axis_angle(FOREARM_AXIS, math.radians(degrees * (1.0 - ELBOW_SHARE)))
        tracks[elbow] = [gltf.qmul(r, at_elbow) for r in tracks[elbow]]
        tracks[(bone, "rotation")] = [gltf.qmul(at_wrist, r) for r in tracks[(bone, "rotation")]]


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


def _dd_skeletal():
    """dd_skeletal, which reads the ActorX files. It is imported as it is used because it imports this module for the
    glTF import pipeline, and the two cannot import each other as they load."""
    from wasami_tools.pipeline import dd_skeletal
    return dd_skeletal


def _nurse_psa(rel):
    """The original's nurse's ActorX animation of that path."""
    return _dd_skeletal().read_psa(os.path.join(dd_assets.pak(NURSE_VERSION), "_anims_psa", *rel.split("/")) + ".psa")


def _nurse_world(psa, frame=None):
    """{her bone: (rotation, position) in the scene} in glTF's axes: her reference pose, or one frame of the psa's keys.
    The positions are in her rig's own units, 30 x the mesh's (_NurseRetarget takes the scale out).

    Her reference pose is the imported mesh's glTF rest pose, bone for bone, to 0.08 deg and one ratio. Her keys are
    written in ActorX's own convention, UE's mirrored across its Y: every key's position has its y negated, and every
    rotation but the root's is (x, -y, z, w) of UE's (the mirror, then the conjugate ActorX keeps for the bones under
    the root); the root's is the mirror alone, (-x, y, -z, w). Read so, the bones nothing animates (the waist's, the
    top of the head's) sit on the reference pose exactly, and her other bones are 6 deg off it at the median through
    ReaperNurse_Idle_Alert. (Conjugating the rotations alone, as this read until 2026-09-26, also stands her up, but
    leaves those bones half a turn off and twists her body -- the user's finding: Wasami twisted at the belly, the
    shoulders and hands wrong.)

    Her rotations are made unit as they are read: the file's are up to 1.7 % short (its numbers are float), which
    would skew every blend between two keys and leave the written keys out of glTF's rotations.
    """
    skeletal = _dd_skeletal()
    names = [bone["name"] for bone in psa["bones"]]
    out = {}
    for i, bone in enumerate(psa["bones"]):
        if frame is None:
            position, rotation = bone["position"], bone["rotation"]
        else:
            (x, y, z), (qx, qy, qz, qw) = psa["keys"][frame][i]
            position = (x, -y, z)
            rotation = (qx, -qy, qz, qw) if i else (-qx, qy, -qz, qw)
        r = gltf.normalized(skeletal.to_gltf_rotation(rotation))
        t = skeletal.to_gltf_position(position)
        if bone["parent"] < 0:
            out[bone["name"]] = (r, t)
        else:
            up_r, up_t = out[names[bone["parent"]]]
            out[bone["name"]] = (gltf.qmul(up_r, r), tuple(a + b for a, b in zip(up_t, gltf.rotate(up_r, t))))
    return out


def _unit(v):
    length = math.sqrt(sum(x * x for x in v))
    if length < 1e-9:
        raise ValueError("a bone of no length has no direction")
    return tuple(x / length for x in v)


def _swing(a, b):
    """The shortest rotation that takes the unit vector a onto the unit vector b."""
    dot = sum(x * y for x, y in zip(a, b))
    if dot > 1.0 - 1e-12:
        return (0.0, 0.0, 0.0, 1.0)
    if dot < -1.0 + 1e-12:   # opposite ways: half a turn about any axis across a
        axis = _unit((-a[1], a[0], 0.0) if abs(a[2]) < 0.9 else (0.0, -a[2], a[1]))
        return (axis[0], axis[1], axis[2], 0.0)
    q = (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0], 1.0 + dot)
    length = math.sqrt(sum(x * x for x in q))
    return tuple(x / length for x in q)


class _NurseRetarget:
    """How the original's nurse's motion is carried onto v3's bones (NURSE_BONES), measured on the two reference poses.

    Each of v3's bones is turned so that it points where hers does: its rotation in the scene is hers followed by a
    constant of its own, which takes v3's rest bone onto hers (a swing) and keeps how the two rigs' bones are rolled
    against each other (the rest of their reference poses' difference, up to half a turn -- her bones run along their
    own +X, and so do v3's on the left, but hers run along -X on the right).

    The swing has to be dropped from the reference poses' difference rather than carried with it. Taking the motion as
    a difference from her rest pose -- what _Retarget does for the user's older model, whose bones point where v3's do
    -- would carry it: she rests in an A pose, her arms 32 deg lower than v3's, so each of her poses would come out
    with the arms 32 deg higher than she holds them. Her directions are the acting, so they are what is copied; only
    the roll, which no direction fixes, comes from the reference poses.

    The pelvis's position is hers about her own rest pose, scaled by the two pelvises' heights (x 1.081 of the mesh's,
    x 0.0360 of the psa's own units); the other bones keep v3's lengths.
    """

    def __init__(self, psa, model):
        rest_hers = _nurse_world(psa)
        rest_v3 = gltf.world_transforms(model, {}, {})
        missing = ([b for row in NURSE_BONES for b in row[1::2] if b not in rest_hers]
                   + [b for row in NURSE_BONES for b in row[0::2] if b not in rest_v3])
        if missing:
            raise RuntimeError("NURSE_BONES names bones no rig has: %s" % sorted(set(missing)))
        self.source = {v3_bone: hers for v3_bone, hers, _, _ in NURSE_BONES}
        self.parents = gltf.parents(model)
        self.rest_v3 = rest_v3
        self.constant, swings = {}, {}
        for v3_bone, hers, v3_tip, her_tip in NURSE_BONES:
            # The constant takes a direction in v3's bone to one in hers: the reference poses' difference, and then the
            # swing that is left between where the two bones point, both of them in her bone's frame.
            into_hers = gltf.qinv(rest_hers[hers][0])
            difference = gltf.qmul(into_hers, rest_v3[v3_bone][0])
            along_v3 = gltf.rotate(into_hers, _unit([a - b for a, b in zip(rest_v3[v3_tip][1], rest_v3[v3_bone][1])]))
            along_hers = gltf.rotate(into_hers, _unit([a - b for a, b in zip(rest_hers[her_tip][1],
                                                                            rest_hers[hers][1])]))
            swing = _swing(along_v3, along_hers)
            self.constant[v3_bone] = gltf.qmul(swing, difference)
            swings[v3_bone] = gltf.angle(swing, (0.0, 0.0, 0.0, 1.0))
        self.scale = rest_v3[ROOT_BONE][1][1] / rest_hers[self.source[ROOT_BONE]][1][1]
        self.pelvis_rest = rest_hers[self.source[ROOT_BONE]][1]
        names = gltf.node_names(model)
        self.pelvis_local = tuple(model["nodes"][names.index(ROOT_BONE)].get("translation", (0.0, 0.0, 0.0)))
        self.report = (max(swings.values()), max(swings, key=swings.get), self.scale)

    def apply(self, psa):
        """Her psa on v3's bones: {track: [value per frame of the psa]}, the rotations of NURSE_BONES and the pelvis's
        position."""
        tracks = {(v3_bone, "rotation"): [] for v3_bone, _, _, _ in NURSE_BONES}
        tracks[(ROOT_BONE, "translation")] = []
        for frame in range(psa["frames"]):
            hers = _nurse_world(psa, frame)
            scene = {v3_bone: gltf.qmul(hers[self.source[v3_bone]][0], self.constant[v3_bone])
                     for v3_bone, _, _, _ in NURSE_BONES}
            for v3_bone in scene:
                parent = self.parents.get(v3_bone)
                # The pelvis hangs off a node of the model's own, which no bone of hers drives; it keeps its rest.
                above = scene.get(parent) or self.rest_v3[parent][0]
                tracks[(v3_bone, "rotation")].append(gltf.qmul(gltf.qinv(above), scene[v3_bone]))
            moved = [self.scale * (a - b) for a, b in zip(hers[self.source[ROOT_BONE]][1], self.pelvis_rest)]
            above = self.rest_v3[self.parents[ROOT_BONE]][0]
            tracks[(ROOT_BONE, "translation")].append(
                tuple(a + b for a, b in zip(self.pelvis_local, gltf.rotate(gltf.qinv(above), moved))))
        return tracks


def _nurse_tracks(model, rel):
    """The nurse's animation of that path on v3's bones, resampled on the 30 fps grid from 0."""
    psa = _nurse_psa(rel)
    rate, frames, length = _dd_skeletal().frame_rate(rel, whole=False)
    if psa["frames"] != frames:
        raise ValueError("%s: the psa has %d frames, the AnimSequence %d" % (rel, psa["frames"], frames))
    retarget = _NurseRetarget(psa, model)
    unreal.log("enemy: %s onto v3: the reference poses' bones are up to %.1f deg apart (%s), her pelvis x %.4f"
               % (rel.split("/")[-1], *retarget.report))
    chans = {key: ([f / rate for f in range(frames)], values) for key, values in retarget.apply(psa).items()}
    return _sample(chans, range(int(round(length * RATE)) + 1))


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
        moved = (0.0, 0.0)
        if source == NURSE:
            if how not in ("once", "loop"):
                raise ValueError("%s: the nurse's animations are carried over as they are" % role)
            tracks = _nurse_tracks(model, name)
            if how == "loop":
                # Her cycles close themselves, unlike the v3 sources a loop repeats the first key after: repeating it
                # here would hold her still for a frame every time round.
                gap = max(gltf.angle(values[0], values[-1]) if what == "rotation"
                          else math.dist(values[0], values[-1]) for (_, what), values in tracks.items())
                if gap > 1e-3:
                    raise ValueError("%s: the original's cycle does not close (%.4f between its first and last key)"
                                     % (role, gap))
        else:
            chans = gltf.channels(*((model, blob) if source == V3 else (capture, capture_blob)),
                                  animations[source][name])
            missing = {node for node, _ in chans} - set(names)
            if missing:
                raise RuntimeError("%s: bones not in the model: %s" % (name, sorted(missing)))
            if how == "stun_fall":
                tracks, moved = _stun_fall(chans, target)
            elif how == "stun_get_up":
                tracks, got = _stun_get_up(model, _stun_fall(chans, target)[0], get_up, target, forward)
                moved = got["move"]
                unreal.log("enemy: %s's get-up turns the enemy by %.1f° about the origin, then moves it x %.2f m "
                           "z %.2f m, as it starts; it rolls %s with the limbs in over %.0f %% of it, its lowest joint "
                           "%.3f m, lifted %.3f m"
                           % (name, got["turn"], *moved, "one way" if got["way"] > 0 else "the other way",
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
        _twist_hands(tracks)
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


def _cloak_noise(g, which, noise, y):
    """One of the cloak's two noises, panned as the original pans it, sampled at the mesh's own UVs (the shader adds
    Time * speed to TEXCOORD0 and nothing else, so the Panner keeps UE's default coordinate)."""
    speed = CLOAK_NOISE_SPEED[which]
    panner = g.node(unreal.MaterialExpressionPanner, -1700, y)
    panner.set_editor_property("speed_x", speed[0])
    panner.set_editor_property("speed_y", speed[1])
    sample = g.node(unreal.MaterialExpressionTextureSample, -1450, y)
    sample.set_editor_property("texture", unreal.load_asset(noise[which]))
    sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    g.link(panner, "", sample, "UVs")
    return sample


def _build_cloak(g, noise):
    """The stealth dissolve the original's nurse vanishes into, read back from M_BSS_Character1's compiled shaders:

        n     = saturate(Fast.r * 3 - Slow.r)                      each noise panned by CLOAK_NOISE_SPEED
        keep  = n > lerp(WorstEfficiency, BestEfficiency, Efficiency)     the opacity mask, clipped at 0.3333
        glow  = n < (Efficiency > 0 ? Efficiency + FringeSize : 0)        the fringe over the edge that is going
        emissive = (glow ? GlowColor : black) * GlowIntensity

    So Efficiency 0 leaves the body whole and 1 takes it away, with a red rim over the edge in between. Which noise is
    tripled the shader does not say (a texture slot carries no name), but only this reading holds together: with both
    textures sRGB, as the original has them, the other one leaves half the body missing at Efficiency 0 and a good part
    of it standing at 1. The original multiplies the emissive texture where there is no glow; the Wasami has none, so
    that side is black (implementation record 07)."""
    fast = _cloak_noise(g, "Fast", noise, 700)
    slow = _cloak_noise(g, "Slow", noise, 900)
    three = g.node(unreal.MaterialExpressionConstant, -1450, 550)
    three.set_editor_property("r", 3.0)
    scaled = g.multiply(fast, "R", three, "", -1200, 600)
    n = g.node(unreal.MaterialExpressionSaturate, -1000, 700)
    g.link(g.binary(unreal.MaterialExpressionSubtract, scaled, "", slow, "R", -1100, 700), "", n, "")

    zero = g.node(unreal.MaterialExpressionConstant, -1450, 1100)
    zero.set_editor_property("r", 0.0)
    one = g.node(unreal.MaterialExpressionConstant, -1450, 1200)
    one.set_editor_property("r", 1.0)
    scalars = {name: g.scalar(name, value, -1700, 1100 + 120 * i)
               for i, (name, value) in enumerate(sorted(CLOAK_SCALARS.items()))}
    # The mask: gone where the noise has fallen to the efficiency (an If, whose equal side goes with the greater one).
    edge = g.lerp(scalars["WorstEfficiency"], "", scalars["BestEfficiency"], "", scalars["Efficiency"], "", -1000, 1100)
    mask = g.node(unreal.MaterialExpressionIf, -700, 1000)
    g.link(edge, "", mask, "A")
    g.link(n, "", mask, "B")
    for pin, value in (("A > B", zero), ("A == B", zero), ("A < B", one)):
        g.link(value, "", mask, pin)
    g.out(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    # The fringe: the band of width FringeSize just past the edge, and only while the body is going.
    band = g.binary(unreal.MaterialExpressionAdd, scalars["Efficiency"], "", scalars["FringeSize"], "", -1200, 1500)
    rim = g.node(unreal.MaterialExpressionIf, -1000, 1450)
    g.link(scalars["Efficiency"], "", rim, "A")
    g.link(zero, "", rim, "B")
    for pin, value in (("A > B", band), ("A == B", zero), ("A < B", zero)):
        g.link(value, "", rim, pin)
    black = g.const3((0.0, 0.0, 0.0, 1.0), -1000, 1750)
    glow = g.vector("GlowColor", CLOAK_GLOW_COLOR, -1000, 1650)
    colour = g.node(unreal.MaterialExpressionIf, -700, 1500)
    g.link(rim, "", colour, "A")
    g.link(n, "", colour, "B")
    for pin, value, value_pin in (("A > B", glow, "RGB"), ("A == B", black, ""), ("A < B", black, "")):
        g.link(value, value_pin, colour, pin)
    g.out(g.multiply(colour, "", scalars["GlowIntensity"], "", -450, 1500), "",
          unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def _build_master(mat, textures, noise):
    """glTF's metallic-roughness material with every factor at 1 (the glb's): the base colour, metallic from B and
    roughness from G of the metallic-roughness map, and the normal map, over the nurse's cloak (_build_cloak, which
    is why the material is masked)."""
    mat.set_editor_property("used_with_skeletal_mesh", True)
    mat.set_editor_property("opacity_mask_clip_value", CLOAK_MASK_CLIP)
    g = dd_stage._Graph(mat, checked=True)
    tcs = unreal.MaterialSamplerType
    base = g.texture("BaseColor", textures["BaseColor"], tcs.SAMPLERTYPE_COLOR, -600, -300)
    g.out(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    packed = g.texture("MetallicRoughness", textures["MetallicRoughness"], tcs.SAMPLERTYPE_LINEAR_COLOR, -600, 0)
    g.out(packed, "B", unreal.MaterialProperty.MP_METALLIC)
    g.out(packed, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    normal = g.texture("Normal", textures["Normal"], tcs.SAMPLERTYPE_NORMAL, -600, 300)
    g.out(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
    _build_cloak(g, noise)


def _build_capture_black(mat):
    """BlackUnlitMaterial, which the original's jumpscareblock gives its planes: unlit with an emissive of 0, one-sided
    (the capture's walls face in because the material shows one side only)."""
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    g = dd_stage._Graph(mat, checked=True)
    g.out(g.const3((0.0, 0.0, 0.0, 1.0), -300, 0), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


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


def import_cloak_noise():
    """The two noises the cloak dissolves through, as the original has them. Returns {which: package path}."""
    return {which: dd_assets.texture(rel, CLOAK_NOISE_VERSION) for which, rel in CLOAK_NOISE.items()}


def import_enemy_audio():
    """The loop the enemy moves to and the attenuations it moves and speaks through. Returns the wave's package
    path."""
    for rel in ENEMY_ATTENUATIONS:
        dd_assets.sound_attenuation(rel, MOVE_SOUND_VERSION)
    return dd_assets.sound(MOVE_SOUND, MOVE_SOUND_VERSION)


def import_all():
    """Prepares and imports the enemy Wasami and the loop it moves to, then saves /Game/Wasami/Enemy and the master.
    Returns how many of each kind, and logs each animation's length and how far its pelvis was moved."""
    report = prepare()
    for role, (seconds, (dx, dz)) in report.items():
        unreal.log("enemy: %s%s %.3f s, pelvis moved x %.2f m z %.2f m" % (ANIM_PREFIX, role, seconds, dx, dz))
    textures = _extract_textures()
    noise = import_cloak_noise()
    master = dd_assets.material(MASTER, lambda mat: _build_master(mat, textures, noise),
                                blend_mode=unreal.BlendMode.BLEND_MASKED)
    instance = dd_assets.material_instance(MATERIAL, master,
                                           textures={p: t.get_path_name().split(".")[0] for p, t in textures.items()})
    black = dd_assets.material(CAPTURE_WALL_MATERIAL, _build_capture_black)
    mesh, anims = _import_model(instance)
    for asset in [master, instance, black, mesh]:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    EAL.save_directory(FOLDER, only_if_is_dirty=True, recursive=True)
    sounds = [import_enemy_audio()]
    return {"textures": len(textures) + len(noise), "materials": 3, "meshes": 1, "animations": len(report),
            "sounds": len(sounds)}
