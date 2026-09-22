"""Dark Deception's level sequences: the original's LevelSequence packages (pak_reference_2/_assets, which keep what
the _sequences summaries leave out — each section's completion mode, an audio section's start offset, attenuation and
row) rebuilt as UE LevelSequence assets under /Game/DD/<the original's path>, bound to the actors the level assembly
placed (tag 'src:<the original's name>'), and the LevelSequenceActors that play them where the original places them.
The original's level Blueprints play those actors (GetSequencePlayer → Play); AWasamiZoneFlow finds them by the same
tag. A binding points at the level's actor by its path in the map, so rebuilding the level (new actor names) has to
rebuild the sequences too, which dd_level.build does last."""
import math

import unreal

from wasami_tools.pipeline import dd_assets, paths, ue_props

EAL = unreal.EditorAssetLibrary
VERSION = 2   # the hospital is only in the latest version

# The zones' level sequence actors the flow, the cut scenes (item 25) and the secret elevators
# (AWasamiFakeUseSequencePlayer, after BP_FakeUseActor_SequencePlayer) play. The levels' other ones are the ambulance
# taking off again at the escape, which nothing plays yet.
SEQUENCE_ACTORS = {
    "Zone1": ("06_Hospital_Zone01_ElevatorArrive", "06_Hospital_Zone1_AmbulanceTakeOff",
              "06_Hospital_Zone1_SecretElevator", "06_Hospital_Zone1_SecretElevator1_2",
              "06_Hospital_Zone1_06Event"),
    "Zone2": ("06_Hospital_Zone2_Spikes", "06_Hospital_Zone2_Cell_DoorPicked",
              "06_Hospital_Zone2_AmbulanceArrive1_2", "06_Hospital_Zone2_Capture", "06_Hospital_Zone2_Cell"),
}
# Sequences without bindings, which the game plays through a player it makes (BP_DD_Functions' Basic DD Fade Out plays
# Ballroom_Event_Fade at twice the speed).
FREE_SEQUENCES = ("Animation/00_Ballroom/Ballroom_Event_Fade",)
# The camera shakes the zones' flow plays with them (the level Blueprints' ClientPlayCameraShake, PlayWorldCameraShake
# and PlayCameraShake): the lift's arrival and its end, the ambulance's take-off, the doors broken in, the cell door.
CAMERA_SHAKES = {
    "Zone1": ("Animation/01_Hotel/01_Hotel_Lobby_ElevatorShake", "Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop",
              "Animation/06_Hospital/06_CameraShake_Zone1_AmbulanceTakeOff",
              "Blueprints/07_FunPlace/Boss/BP_07_CameraShake_Jump"),
    "Zone2": ("Blueprints/04_Sewer/Bossfight/BP_04_BossFight_CameraShake_Initial",),
}

TAG = "dd"   # dd_level.TAG: a level rebuild removes it
SEQUENCE_TAG = "dd_sequence"
SEQUENCE_FOLDER = "Hospital/Gameplay/Sequences"
# Bound actors the level assembly does not place, placed here where the original has them: a TargetPoint an audio
# track follows or a camera looks at, the particle emitters a particle track fires, and the cine cameras the cut
# scenes look through (the flow makes one the view target, so what it sees is what the player sees). A cine camera
# carries the original's own settings (prepare_stage's camera_settings), which are what the scenes are framed for:
# the sequences animate only the focal length, the aperture and the focus distance.
HELPER_CLASSES = {"TargetPoint": unreal.TargetPoint, "Emitter": unreal.Emitter,
                  "CineCameraActor": unreal.CineCameraActor}

# The cut scenes' nurses, which the level assembly leaves out too: Zone 1's two BP_06_Nurse_Cutscene (a Character, so
# its SkeletalMesh hangs under the capsule's centre) and Zone 2's two SkeletalMeshActors (their mesh at the actor).
# Each is placed as an AWasamiCutsceneNurse, which carries the enemy Wasami's mesh under a scene root the transform
# track moves; the value says whether its mesh hangs under a capsule's centre, as the original's Character has it.
NURSE_CLASSES = {"BP_06_Nurse_Cutscene_C": True, "SkeletalMeshActor": False}
NURSE_TAG = "dd_nurse"
WASAMI_CLIP = "/Game/Wasami/Enemy/A_WasamiEnemy_%s"

# The nurse animations the cut scenes play → the enemy Wasami's clip that stands in for it, and whether it is played
# backwards (.claude/references/enemy-wasami-motions.md の「場面の代用」, 2026-09-18 のユーザーの回答「場面は残し、v3 の
# 動きで代用」). The dialogue animations (Nurse_Hospital_Zone01_Event_40..47) are the cell scene's acting, which the
# idle stands in for.
NURSE_ANIMS = {
    "ReaperNurse_Boss_Idle_01": ("Idle_Alert", False),      # the low stance the two take before they leap
    "ReaperNurse_Fast_Jump_Up": ("Chase_VaultRoll", False),
    "ReaperNurse_Fast_Jump_Up_Air": ("Run", False),
    "ReaperNurse_Flip_Up": ("Chase_VaultRoll", False),
    "ReaperNurse_Idle_Alert": ("Idle_Alert", False),
    # The punch that takes the player. The original swings: it sinks, rises winding the right arm and the
    # syringe a trunk's length above the hips, then brings them down in the last 0.13 s (measured off
    # Nurse_Hospital_Zone01_Event_39.psa). Chase_PickUp is the only clip of the v3 set whose right hand does
    # the same shape late in the take, and the camera is 0.4 m from her as it lands.
    "Nurse_Hospital_Zone01_Event_39": ("Chase_PickUp", False),
    "nurse_idle_01": ("Idle", False),
    "ReaperNurse_Walk_Back": ("Walk", True),                # backing away
    # She turns invisible where she stands (the cell scene never moves her: its transform track holds one key of
    # zeroes). The Wasami has no cloaking of its own, so the idle stands in while the material takes her away
    # (dd_enemy's cloak, item 28's step 14b); until that cloak existed she walked off instead.
    "nurse_cloak": ("Idle", False),
}
NURSE_ANIMS.update({"Nurse_Hospital_Zone01_Event_%d" % n: ("Idle", False) for n in range(40, 48)})
# The stand-in the gaps in a nurse's animation track are filled with (fill_rest_pose).
FILL_CLIP = "Idle"
# The stand-ins that are one action rather than a cycle. A section the original holds a single take in slows these to
# fill it once instead of repeating the action (play_rate below); the cycles loop, as the original's own cycles do.
ONE_SHOT_CLIPS = {"Chase_Charge", "Chase_PickUp", "Chase_VaultRoll"}

# UE 4.24's UMovieScene defaults for what the export leaves out (60000 ticks a second, 30 frames).
DEFAULT_TICK_RESOLUTION = (60000, 1)
DEFAULT_DISPLAY_RATE = (30, 1)

# The export's channel names → UE 5.8's, with the default each class's constructor gives the channel (the export keeps
# a channel's DefaultValue and bHasDefaultValue only where they differ from those).
TRANSFORM_CHANNELS = {
    "Translation": ("Location.X", 0.0), "Translation[1]": ("Location.Y", 0.0), "Translation[2]": ("Location.Z", 0.0),
    "Rotation": ("Rotation.X", 0.0), "Rotation[1]": ("Rotation.Y", 0.0), "Rotation[2]": ("Rotation.Z", 0.0),
    "Scale": ("Scale.X", 1.0), "Scale[1]": ("Scale.Y", 1.0), "Scale[2]": ("Scale.Z", 1.0),
}
# How far a transform channel's keys have to spread for the binding to count as moved (movable).
MOVED_EPSILON = 0.01
AUDIO_CHANNELS = {"SoundVolume": ("Volume", 1.0), "PitchMultiplier": ("Pitch", 1.0)}
CURVE_CHANNELS = {"FloatCurve": ("None", None)}   # a float property's and a fade's, without a default
PARTICLE_CHANNELS = {"ParticleKeys": ("None", None)}

# The transform channels a camera anim is added to (see bake_location), and the six axes UInterpTrackMove splits a
# Matinee move track into, in the order it makes them. The export leaves the first axis' MoveAxis out, as its default.
LOCATION_CHANNELS = ("Translation", "Translation[1]", "Translation[2]")
ROTATION_CHANNELS = ("Rotation", "Rotation[1]", "Rotation[2]")
MOVE_AXES = ("AXIS_TranslationX", "AXIS_TranslationY", "AXIS_TranslationZ",
             "AXIS_RotationX", "AXIS_RotationY", "AXIS_RotationZ")
CAMERA_ANIM_KEYS = {"CameraAnim", "PlayRate", "PlayScale", "BlendInTime", "BlendOutTime", "bLooping",
                    "bRandomStartTime", "Duration"}
# A key's interpolation -> how the curve between it and the next one is evaluated; anything else is a cubic of the
# tangents saved with the keys (a Matinee curve and a section's channel are evaluated the same way).
MATINEE_INTERP = {"CIM_Constant": "constant", "CIM_Linear": "linear"}
CHANNEL_INTERP = {"RCIM_Constant": "constant", "RCIM_Linear": "linear"}

INTERP = {"RCIM_Linear": "LINEAR", "RCIM_Constant": "CONSTANT"}
CUBIC_TANGENT = {"RCTM_Auto": "AUTO", "RCTM_User": "USER", "RCTM_Break": "BREAK", "RCTM_SmartAuto": "SMART_AUTO"}

# Section properties handled below; anything else in a section's export is an error, so a sequence added later that
# uses more of the Sequencer does not lose it silently.
SECTION_KEYS = {"SectionRange", "EvalOptions", "Easing", "RowIndex", "OverlapPriority", "Signature"}
AUDIO_KEYS = {"Sound", "StartFrameOffset", "bOverrideAttenuation", "AttenuationSettings"}
ANIMATION_KEYS = {"Animation", "SlotName", "StartFrameOffset"}
SHAKE_KEYS = {"ShakeClass", "PlayScale"}
# The easing an export keeps: the functions (a built-in one at its default, Linear, in every one of these), the
# durations the original's editor worked out from the overlaps (UE works those out again) and the author's own.
EASING_KEYS = {"EaseIn", "EaseOut", "AutoEaseInDuration", "AutoEaseOutDuration",
               "bManualEaseIn", "bManualEaseOut", "ManualEaseInDuration", "ManualEaseOutDuration"}


def _cue_waves(pkg):
    """The waves a SoundCue's export plays (its wave players' SoundWaveAssetPtr), which it has to be made after."""
    return [e["props"]["SoundWaveAssetPtr"] for e in pkg["exports"] if e["props"].get("SoundWaveAssetPtr")]


def _frame(n):
    return unreal.FrameNumber(int(n))


def _rate(struct, default):
    if not struct:
        return unreal.FrameRate(*default)
    return unreal.FrameRate(int(struct.get("Numerator", default[0])), int(struct.get("Denominator", 1)))


def _enum(cls, member):
    return getattr(cls, member)


def _matinee_keys(points):
    """A Matinee curve's points (FInterpCurvePoint<float>, keyed in seconds) as (x, value, arrive, leave, how)."""
    return [(float(p["InVal"]), float(p["OutVal"]), float(p.get("ArriveTangent", 0.0)),
             float(p.get("LeaveTangent", 0.0)), MATINEE_INTERP.get(p.get("InterpMode", ""), "cubic"))
            for p in points]


def _channel_keys(data):
    """A section channel's keys (a rich curve's Times and Values, keyed in ticks) in the same form."""
    keys = []
    for t, v in zip((data or {}).get("Times", []), (data or {}).get("Values", [])):
        tangent = v.get("Tangent") or {}
        keys.append((float(t), float(v["Value"]), float(tangent.get("ArriveTangent", 0.0)),
                     float(tangent.get("LeaveTangent", 0.0)), CHANNEL_INTERP.get(v.get("InterpMode", ""), "cubic")))
    return keys


def _eval(keys, x, default=0.0):
    """A curve's value at x and its slope there, in the keys' own unit. UE evaluates FInterpCurve<float> and a rich
    curve channel alike: outside the keys it holds the end value, the key before the segment says how the segment is
    read, and a cubic one is a Hermite of the tangents saved with the keys, scaled by the gap between them
    (FInterpCurve::Eval, FMath::CubicInterp and FMath::CubicInterpDerivative)."""
    if not keys:
        return default, 0.0
    if x <= keys[0][0]:
        return keys[0][1], 0.0
    if x >= keys[-1][0]:
        return keys[-1][1], 0.0
    index = max(i for i in range(len(keys) - 1) if keys[i][0] <= x)
    lo, hi = keys[index], keys[index + 1]
    diff = hi[0] - lo[0]
    if diff <= 0.0 or lo[4] == "constant":
        return lo[1], 0.0
    alpha = (x - lo[0]) / diff
    if lo[4] == "linear":
        return lo[1] + (hi[1] - lo[1]) * alpha, (hi[1] - lo[1]) / diff
    p0, p1, t0, t1 = lo[1], hi[1], lo[3] * diff, hi[2] * diff
    a2 = alpha * alpha
    a3 = a2 * alpha
    value = (2 * a3 - 3 * a2 + 1) * p0 + (a3 - 2 * a2 + alpha) * t0 + (a3 - a2) * t1 + (3 * a2 - 2 * a3) * p1
    slope = ((6 * a2 - 6 * alpha) * p0 + (3 * a2 - 4 * alpha + 1) * t0 + (3 * a2 - 2 * alpha) * t1
             + (6 * alpha - 6 * a2) * p1) / diff
    return value, slope


def _rotate(rotation, vector):
    """A vector turned by an FRotator (roll, pitch, yaw in degrees), as FRotationMatrix::TransformVector turns it."""
    roll, pitch, yaw = (math.radians(a) for a in rotation)
    sp, cp = math.sin(pitch), math.cos(pitch)
    sy, cy = math.sin(yaw), math.cos(yaw)
    sr, cr = math.sin(roll), math.cos(roll)
    axes = ((cp * cy, cp * sy, sp),
            (sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp),
            (-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp))
    return tuple(sum(vector[i] * axes[i][j] for i in range(3)) for j in range(3))


def _camera_anim_move(object_path):
    """The translation of the original's CameraAnim: the three translation axes of its Matinee move track, as curves
    keyed in seconds. The rotation axes are read only to be sure the anim is one of these (bake_location leaves them
    out)."""
    rel = dd_assets.game_rel(object_path)
    pkg = dd_assets.export_json(rel, VERSION)
    by_path = {(e["outer"] + "." if e["outer"] else "") + e["name"]: e for e in pkg["exports"]}
    group = by_path[dd_assets.main_export(pkg, rel)["props"]["CameraInterpGroup"]]
    axes = {}
    for track_path in group["props"].get("InterpTracks", []):
        track = by_path[track_path]
        if track["class"] != "InterpTrackMove":
            raise ValueError("%s: camera anim track %s not handled" % (rel, track["class"]))
        for sub_path in track["props"].get("SubTracks", []):
            sub = by_path[sub_path]
            axes[sub["props"].get("MoveAxis", MOVE_AXES[0])] = _matinee_keys(
                (sub["props"].get("FloatTrack") or {}).get("Points", []))
    missing = [a for a in MOVE_AXES if a not in axes]
    if missing:
        raise ValueError("%s: the camera anim's move track has no %s" % (rel, ", ".join(missing)))
    return [axes[a] for a in MOVE_AXES[:3]]


class _Package:
    """The original's sequence package: its exports by path ('<asset>.MovieScene_0.<track>.<section>')."""

    def __init__(self, rel):
        self.rel = rel
        pkg = dd_assets.export_json(rel, VERSION)
        self.by_path = {}
        for e in pkg["exports"]:
            self.by_path[(e["outer"] + "." if e["outer"] else "") + e["name"]] = e
        self.asset = dd_assets.main_export(pkg, rel)["props"]
        self.movie_scene = self.by_path[self.asset["MovieScene"]]["props"]
        rate = _rate(self.movie_scene.get("TickResolution"), DEFAULT_TICK_RESOLUTION)
        self.ticks_per_second = rate.numerator / float(rate.denominator)

    def get(self, path):
        return self.by_path[path]


class _Builder:
    """Writes one package into a LevelSequence asset; `resolve(guid, possessable, references)` gives the object a
    possessable binds to (an actor or a component of one) or None."""

    def __init__(self, result):
        self.result = result
        self.sounds = {}
        self.attenuations = {}
        self.shakes = {}
        self.clips = {}
        self.lengths = {}
        # What the binding being written adds to its transform track, from its camera anim track (camera_anim).
        self.offset = None
        # The sequence's playback range in ticks, which fill_rest_pose measures its gaps against.
        self.playback = None

    # ---------------------------------------------------------------------------------------------- assets
    def sound(self, object_path):
        """What an audio section plays, made under /Game/DD if it is not there yet: a wave, or a SoundCue with the
        waves its players play first (the cut scenes' footsteps are cues)."""
        rel = dd_assets.game_rel(object_path)
        if rel not in self.sounds:
            target = dd_assets.asset_path(rel)
            if not EAL.does_asset_exist(target):
                pkg = dd_assets.export_json(rel, VERSION)
                if dd_assets.main_export(pkg, rel)["class"] == "SoundCue":
                    for wave in _cue_waves(pkg):
                        self.sound(wave)
                    dd_assets.sound_cue(rel, VERSION)
                    self.result["sound_cues"] += 1
                else:
                    dd_assets.sound(rel, VERSION)
                    self.result["sounds"] += 1
                EAL.save_asset(target, only_if_is_dirty=False)
            self.sounds[rel] = unreal.load_asset(target)
        return self.sounds[rel]

    def attenuation(self, object_path):
        rel = dd_assets.game_rel(object_path)
        if rel not in self.attenuations:
            self.attenuations[rel] = dd_assets.sound_attenuation(rel, VERSION)
            self.result["attenuations"] += 1
        return self.attenuations[rel]

    def shake_class(self, object_path):
        """The camera shake class a shake section plays, as a LegacyCameraShake Blueprint of the original's defaults."""
        rel = dd_assets.game_rel(object_path)
        if rel not in self.shakes:
            self.shakes[rel] = unreal.load_asset(dd_assets.camera_shake(rel, VERSION)).generated_class()
            self.result["camera_shakes"] += 1
        return self.shakes[rel]

    def clip(self, object_path):
        """The enemy Wasami's clip that stands in for the nurse animation of an animation section, and whether it is
        played backwards (NURSE_ANIMS)."""
        name = object_path.split(".", 1)[0].rsplit("/", 1)[-1]
        if name not in NURSE_ANIMS:
            raise ValueError("no stand-in for the nurse animation %s (see NURSE_ANIMS and "
                             ".claude/references/enemy-wasami-motions.md)" % name)
        clip, reverse = NURSE_ANIMS[name]
        return self.wasami_clip(clip), reverse

    def wasami_clip(self, name):
        """One of the enemy Wasami's clips, by its name (WASAMI_CLIP)."""
        if name not in self.clips:
            asset = unreal.load_asset(WASAMI_CLIP % name)
            if asset is None:
                raise RuntimeError("%s is missing: run WasamiDDTools.import_wasami_enemy" % (WASAMI_CLIP % name))
            self.clips[name] = asset
        return self.clips[name]

    def nurse_length(self, object_path):
        """The nurse animation's own length (SequenceLength), which tells a section that loops a cycle from one that
        holds a single take."""
        rel = dd_assets.game_rel(object_path)
        if rel not in self.lengths:
            pkg = dd_assets.export_json(rel, VERSION)
            self.lengths[rel] = float(dd_assets.main_export(pkg, rel)["props"]["SequenceLength"])
        return self.lengths[rel]

    # ---------------------------------------------------------------------------------------------- channels
    def _defaults(self, channel, data, class_default):
        has_default = data.get("bHasDefaultValue", class_default is not None)
        if has_default:
            channel.set_default(float(data.get("DefaultValue", class_default or 0.0)) if not isinstance(
                channel, unreal.MovieSceneScriptingParticleChannel) else self._particle_key(data.get("DefaultValue", 0)))
        elif channel.has_default():
            channel.remove_default()

    @staticmethod
    def _particle_key(value):
        return next(getattr(unreal.ParticleKey, m) for m in ("ACTIVATE", "DEACTIVATE", "TRIGGER")
                    if getattr(unreal.ParticleKey, m).value == int(value))

    def curve(self, channel, data, class_default):
        """A rich curve channel (FMovieSceneFloatChannel / FMovieSceneDoubleChannel): keys at their ticks with their
        interpolation, tangent mode and tangents, the default, and the extrapolation. The tangents are written after
        every key is in (adding a key recomputes the automatic ones by UE 5's rule, which is not UE 4.24's)."""
        data = data or {}
        # TickResolution is the sequence's own rate, which UE 4.24's channel keeps beside its keys to work its tangents
        # out in seconds; UE 5.8's takes it from the sequence, so it is read and left out.
        unknown = set(data) - {"Times", "Values", "DefaultValue", "bHasDefaultValue", "PreInfinityExtrap",
                               "PostInfinityExtrap", "TickResolution"}
        if unknown:
            raise ValueError("curve fields not handled: %s" % sorted(unknown))
        keys = []
        for t, v in zip(data.get("Times", []), data.get("Values", [])):
            interp = INTERP.get(v["InterpMode"]) or CUBIC_TANGENT[v.get("TangentMode", "RCTM_Auto")]
            keys.append((channel.add_key(_frame(t), float(v["Value"]), 0.0, unreal.MovieSceneTimeUnit.TICK_RESOLUTION,
                                         _enum(unreal.MovieSceneKeyInterpolation, interp)), v))
        for key, v in keys:
            tangent = v.get("Tangent") or {}
            key.set_interpolation_mode(ue_props.enum_member(unreal.RichCurveInterpMode, v["InterpMode"]))
            key.set_tangent_mode(ue_props.enum_member(unreal.RichCurveTangentMode, v.get("TangentMode", "RCTM_Auto")))
            key.set_tangent_weight_mode(ue_props.enum_member(unreal.RichCurveTangentWeightMode,
                                                             tangent.get("TangentWeightMode", "RCTWM_WeightedNone")))
            key.set_arrive_tangent(float(tangent.get("ArriveTangent", 0.0)))
            key.set_leave_tangent(float(tangent.get("LeaveTangent", 0.0)))
            key.set_arrive_tangent_weight(float(tangent.get("ArriveTangentWeight", 0.0)))
            key.set_leave_tangent_weight(float(tangent.get("LeaveTangentWeight", 0.0)))
        for field, setter in (("PreInfinityExtrap", "set_pre_infinity_extrapolation"),
                              ("PostInfinityExtrap", "set_post_infinity_extrapolation")):
            if field in data:
                getattr(channel, setter)(ue_props.enum_member(unreal.RichCurveExtrapolation, data[field].split("::")[-1]))
        self._defaults(channel, data, class_default)
        self.result["keys"] += len(keys)

    def particle(self, channel, data):
        data = data or {}
        for t, v in zip(data.get("Times", []), data.get("Values", [])):
            channel.add_key(_frame(t), self._particle_key(v), 0.0, unreal.MovieSceneTimeUnit.TICK_RESOLUTION)
        self._defaults(channel, data, None)
        self.result["keys"] += len(data.get("Times", []))

    def boolean(self, channel, data):
        """A bool channel of a visibility section. The original's UE 4.24 track keys bHidden (its template inverts what
        it reads before it hides the actor); UE 5.8's UMovieSceneVisibilitySection keys visibility itself
        (MovieSceneVisibilitySystem: SetActorHiddenInGame(!value)), so the keys and the default go in inverted."""
        data = data or {}
        unknown = set(data) - {"Times", "Values", "DefaultValue", "bHasDefaultValue"}
        if unknown:
            raise ValueError("bool channel fields not handled: %s" % sorted(unknown))
        for t, v in zip(data.get("Times", []), data.get("Values", [])):
            channel.add_key(_frame(t), not bool(v), 0.0, unreal.MovieSceneTimeUnit.TICK_RESOLUTION)
        # The export keeps DefaultValue only where it differs from the channel's own (false), and the flag on its own
        # where it does not (Zone 2's nurses start not hidden, which is visible here).
        if data.get("bHasDefaultValue", "DefaultValue" in data):
            channel.set_default(not bool(data.get("DefaultValue", False)))
        elif channel.has_default():
            channel.remove_default()
        self.result["keys"] += len(data.get("Times", []))

    def channels(self, section, props, table, particle=False):
        by_name = {str(c.channel_name): c for c in section.get_all_channels()}
        for export_name, (ue_name, class_default) in table.items():
            channel = by_name[ue_name]
            if particle:
                self.particle(channel, props.get(export_name))
            else:
                self.curve(channel, props.get(export_name), class_default)

    # ------------------------------------------------------------------------------------------- camera anim
    def camera_anim(self, pkg, path):
        """What a binding's MovieSceneCameraAnimTrack adds to its transform track: the section's range in ticks and
        the CameraAnim's three translation curves (bake_location adds them)."""
        export = pkg.get(path)
        sections = export["props"].get("CameraAnimSections", [])
        if len(sections) != 1:
            raise ValueError("%s: %d camera anim sections" % (export["name"], len(sections)))
        sec = pkg.get(sections[0])
        props = sec["props"]
        unknown = set(props) - SECTION_KEYS - {"AnimData"}
        if unknown:
            raise ValueError("%s: properties not handled: %s" % (sec["name"], sorted(unknown)))
        data = props["AnimData"]
        unknown = set(data) - CAMERA_ANIM_KEYS
        if unknown:
            raise ValueError("%s: camera anim data not handled: %s" % (sec["name"], sorted(unknown)))
        # The original's one section names the anim and leaves the rest at its defaults, which is what the bake takes
        # it for: played once from the start of the section, at its own rate and scale, without a blend.
        left = {k: v for k, v in data.items() if k != "CameraAnim"}
        if left:
            raise ValueError("%s: camera anim data other than the defaults: %s" % (sec["name"], left))
        rng = props.get("SectionRange") or {}
        if (rng.get("lower") or {}).get("type") != "Inclusive" or (rng.get("upper") or {}).get("type") != "Exclusive":
            raise ValueError("%s: camera anim range %s" % (sec["name"], rng))
        return {"start": int(rng["lower"]["value"]), "end": int(rng["upper"]["value"]),
                "curves": _camera_anim_move(data["CameraAnim"])}

    def bake_location(self, pkg, props):
        """A copy of a transform section's properties with the camera anim's offset added to its location channels.

        UE 4.24 plays a camera anim on a bound camera as an additive animation: what the anim's move track has at the
        time is an offset in the camera's own space, which is turned by the rotation the transform track has just set
        and added to the location it has just set (FMovieSceneAdditiveCameraAnimationTrackExecutionToken ->
        FCameraAnimationHelper::ApplyOffset, which UE 5.8 still has). UE 5.8 has no camera anim track, so that sum is
        keyed here instead: the location channels are keyed again over the section's range, at the anim's own key
        times and wherever the transform track has a key of its own, and hold the anim's last key to the end of the
        section (the Sequencer's camera anim instance does not stop itself, and a Matinee curve holds its last key).
        Each key carries the slope of the sum there, so the easing the anim was keyed with survives.

        The anim's rotation is left out. This camera carries the original's look-at tracking (item 28's step 14c),
        which turns it towards the actor it follows every frame, over whatever the transform track or the anim sets -
        in the original as here, only the offset's translation is ever seen. The keys put in at the edges of the
        section carry the value and the slope the channel already has there, so the segments outside keep their shape.
        """
        start, end, curves = self.offset["start"], self.offset["end"], self.offset["curves"]
        ticks = pkg.ticks_per_second
        location = [_channel_keys(props.get(c)) for c in LOCATION_CHANNELS]
        rotation = [_channel_keys(props.get(c)) for c in ROTATION_CHANNELS]
        defaults = [float((props.get(c) or {}).get("DefaultValue", TRANSFORM_CHANNELS[c][1]))
                    for c in LOCATION_CHANNELS + ROTATION_CHANNELS]
        times = {start - 1, start, end - 1, end}
        times |= {start + int(round(key[0] * ticks)) for curve in curves for key in curve}
        times |= {int(key[0]) for keys in location + rotation for key in keys}
        baked = []
        for tick in sorted(t for t in times if start - 1 <= t <= end):
            base = [_eval(location[i], tick, defaults[i]) for i in range(3)]
            if start <= tick < end:
                anim = [_eval(curves[i], (tick - start) / ticks) for i in range(3)]
                turn = [_eval(rotation[i], tick, defaults[3 + i])[0] for i in range(3)]
                world = _rotate(turn, [value for value, _ in anim])
                # The offset's own speed, in the unit a channel's tangents are in (a tick).
                speed = _rotate(turn, [slope / ticks for _, slope in anim])
            else:
                world = speed = (0.0, 0.0, 0.0)
            baked.append((tick, [(base[i][0] + world[i], base[i][1] + speed[i]) for i in range(3)]))
        patched = dict(props)
        for i, name in enumerate(LOCATION_CHANNELS):
            data = dict(props.get(name) or {})
            keys = {int(t): v for t, v in zip(data.get("Times", []), data.get("Values", []))}
            for tick, values in baked:
                if tick in keys and not start <= tick < end:
                    continue      # a key of the original's just outside the section stands as it is
                value, slope = values[i]
                keys[tick] = {"Value": value, "InterpMode": "RCIM_Cubic", "TangentMode": "RCTM_User",
                              "Tangent": {"ArriveTangent": slope, "LeaveTangent": slope}}
            data["Times"] = sorted(keys)
            data["Values"] = [keys[t] for t in data["Times"]]
            patched[name] = data
        return patched

    # ---------------------------------------------------------------------------------------------- sections
    def section(self, track, export, handled):
        """A new section on the track with the export's range, completion mode, row and priority; `handled` lists the
        class's own properties, which the caller writes."""
        props = export["props"]
        unknown = set(props) - SECTION_KEYS - handled
        if unknown:
            raise ValueError("%s: properties not handled: %s" % (export["name"], sorted(unknown)))
        easing = props.get("Easing") or {}
        if set(easing) - EASING_KEYS:
            raise ValueError("%s: easing not handled: %s" % (export["name"], easing))
        section = track.add_section()
        rng = props.get("SectionRange") or {"lower": {"type": "Open"}, "upper": {"type": "Open"}}
        lower, upper = rng["lower"], rng["upper"]
        if lower["type"] not in ("Open", "Inclusive") or upper["type"] not in ("Open", "Exclusive", "Inclusive"):
            raise ValueError("%s: range %s" % (export["name"], rng))
        # The sequence's display rate is its tick resolution while this runs, so a frame here is a tick; a bounded
        # range goes in at once ([start, end), as the export's inclusive-exclusive one). UE's scripting writes an
        # exclusive upper bound only, so an inclusive one (the cell's material section) goes in as the tick after it,
        # which holds the same frames.
        end = None if upper["type"] == "Open" else int(upper["value"]) + (upper["type"] == "Inclusive")
        if lower["type"] == "Inclusive" and end is not None:
            section.set_range(int(lower["value"]), end)
        else:
            if lower["type"] == "Open":
                section.set_start_frame_bounded(False)
            else:
                section.set_start_frame(int(lower["value"]))
            if end is None:
                section.set_end_frame_bounded(False)
            else:
                section.set_end_frame(end)
        mode = (props.get("EvalOptions") or {}).get("CompletionMode", "EMovieSceneCompletionMode::ProjectDefault")
        section.set_completion_mode(_enum(unreal.MovieSceneCompletionMode,
                                          {"KeepState": "KEEP_STATE", "RestoreState": "RESTORE_STATE",
                                           "ProjectDefault": "PROJECT_DEFAULT"}[mode.split("::")[-1]]))
        section.set_row_index(int(props.get("RowIndex", 0)))
        section.set_overlap_priority(int(props.get("OverlapPriority", 0)))
        # A manual ease is the author's own, so it goes in (the automatic ones are worked out from the overlaps again);
        # set_ease_*_duration writes the duration and turns the manual flag on, as the original's has it.
        if easing.get("bManualEaseIn"):
            section.set_ease_in_duration(int(easing.get("ManualEaseInDuration", 0)))
        if easing.get("bManualEaseOut"):
            section.set_ease_out_duration(int(easing.get("ManualEaseOutDuration", 0)))
        self.result["sections"] += 1
        return section

    def play_rate(self, params, pkg, sec, p, clip):
        """The original plays every one of these at 1, and a section that outlasts its animation loops it (UE 4.24
        and 5.8 alike). So the original's sections come in two kinds: one loops a cycle (its animation is shorter than
        the section), the other holds a single take (the animation fills the section, sometimes cut short). A stand-in
        is a different length, so a single take would come out looped - the capture scene's punch (1.633 s, the nurse
        animation's own length) would swing twice over with the 1.233 s stand-in. Keep which of the two kinds a
        section is: slow a stand-in down to fill a single-take section once, and leave a looping one at 1. Only the
        stand-ins that are one action (ONE_SHOT_CLIPS) are slowed; a cycle standing in for a take of acting is left to
        loop, which reads as the idle it is (the cell's 11 s of dialogue would otherwise crawl at a sixth speed)."""
        if clip.get_name() not in {WASAMI_CLIP.rsplit("/", 1)[-1] % c for c in ONE_SHOT_CLIPS}:
            return
        rng = sec["props"].get("SectionRange") or {}
        bounds = (rng.get("lower") or {}), (rng.get("upper") or {})
        if bounds[0].get("type") != "Inclusive" or bounds[1].get("type") != "Exclusive":
            return
        seconds = (int(bounds[1]["value"]) - int(bounds[0]["value"])) / pkg.ticks_per_second
        offset = p.get("StartFrameOffset", 0) / pkg.ticks_per_second
        left = clip.get_play_length() - offset
        if seconds <= self.nurse_length(p["Animation"]) - offset and 0.0 < left < seconds:
            # The variant is a value too, so it is read, written and put back.
            rate = params.get_editor_property("play_rate")
            rate.set_fixed_play_rate(left / seconds)
            params.set_editor_property("play_rate", rate)

    # ------------------------------------------------------------------------------------------ the rest pose
    def span(self, props):
        """A section's [start, end) in ticks and whether it restores state when it ends. An open bound reaches the
        end of the playback range, which is as far as anything is evaluated. Every mode but KeepState restores:
        BaseEngine.ini gives MovieSceneSequence DefaultCompletionMode=RestoreState, which is what ProjectDefault
        (the mode of all but one of these sections) resolves to."""
        rng = props.get("SectionRange") or {}
        lower, upper = (rng.get("lower") or {"type": "Open"}), (rng.get("upper") or {"type": "Open"})
        start = self.playback[0] if lower.get("type") == "Open" else int(lower["value"])
        end = (self.playback[1] if upper.get("type") == "Open"
               else int(upper["value"]) + (upper.get("type") == "Inclusive"))
        mode = (props.get("EvalOptions") or {}).get("CompletionMode", "EMovieSceneCompletionMode::ProjectDefault")
        return start, end, mode.split("::")[-1] != "KeepState"

    def fill_rest_pose(self, track, spans):
        """Fills the stretches of a nurse's animation track that would show the stand-in's rest pose with its idle.

        The original's nurses stand naturally wherever nothing animates them: Zone 2's SkeletalMeshActors have no
        AnimClass and their rest pose is an upright stand, and the cell's nurse has an idle Anim Blueprint
        (...AnimBlueprint_lookat_cutscene). The enemy Wasami standing in for them rests with its arms out to the
        sides (restpose), which is what a reviewer saw on the capture scene's terrace over the 17.2 s before that
        track's first section (item 42).

        That rest pose shows where nothing has posed the mesh yet - before the first section - and after a section
        that restores state when it ends (span). After a KeepState section the last pose stays instead, which is the
        original's own look (measured in PIE: the capture nurse holds the punch's last pose to the end of the scene),
        so those gaps are left as they are. The fills themselves are KeepState, so one cannot end into a rest pose.
        """
        if not spans or self.playback is None:
            return
        start, end = self.playback
        gaps, cursor, restores = [], start, True
        for lower, upper, mode in sorted(spans):
            if lower > cursor and restores:
                gaps.append((cursor, lower))
            if upper > cursor:
                cursor, restores = upper, mode
        if cursor < end and restores:
            gaps.append((cursor, end))
        for lower, upper in gaps:
            section = track.add_section()
            section.set_range(lower, upper)
            params = section.get_editor_property("params")
            params.set_editor_property("animation", self.wasami_clip(FILL_CLIP))
            section.set_editor_property("params", params)
            section.set_completion_mode(unreal.MovieSceneCompletionMode.KEEP_STATE)
            self.result["sections"] += 1
            self.result["idle_fills"] += 1

    # ------------------------------------------------------------------------------------------- mobility
    def moves(self, pkg, paths_):
        """Whether the binding's transform tracks change its transform. Some bindings the original keys once, at the
        very transform the level places them at (Zone 2's holding cell), and those are not moved at all."""
        per_channel = {}
        for path in paths_:
            export = pkg.get(path)
            if export["class"] != "MovieScene3DTransformTrack":
                continue
            for s in export["props"].get("Sections", []):
                props = pkg.get(s)["props"]
                for name in TRANSFORM_CHANNELS:
                    per_channel.setdefault(name, []).extend(
                        float(v["Value"]) for v in (props.get(name) or {}).get("Values", []))
        return any(max(v) - min(v) > MOVED_EPSILON for v in per_channel.values() if v)

    def movable(self, obj):
        """What a sequence moves is set Movable. A sequence does move a Static component, but the renderer is told its
        transform never changes and caches it (baked lighting, its place in the static draw lists), which is the one
        lead left for Zone 1's ambulance looking doubled on Metal. The original's levels leave three moved ones Static,
        which UE 4.24 tolerated: Zone 1's taking-off ambulance, Zone 2's cell door and its wall switch. Whatever hangs
        under it goes Movable too, which UE requires of a Movable component's children."""
        comp = obj.get_editor_property("root_component") if isinstance(obj, unreal.Actor) else obj
        if not isinstance(comp, unreal.SceneComponent):
            return
        for i, c in enumerate([comp] + list(comp.get_children_components(True))):
            if c.get_editor_property("mobility") != unreal.ComponentMobility.MOVABLE:
                c.set_mobility(unreal.ComponentMobility.MOVABLE)
                owner = c.get_owner()
                label = owner.get_actor_label() if owner else c.get_name()
                self.result["made_movable"].append(label if i == 0 else "%s / %s" % (label, c.get_name()))

    def track(self, pkg, owner, path):
        """One track of the package onto `owner` (a binding proxy, or the sequence for a master track)."""
        export = pkg.get(path)
        cls, props = export["class"], export["props"]
        add = owner.add_track
        if cls == "MovieScene3DTransformTrack":
            track = add(unreal.MovieScene3DTransformTrack)
            for s in props.get("Sections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, set(TRANSFORM_CHANNELS))
                self.channels(section, self.bake_location(pkg, sec["props"]) if self.offset else sec["props"],
                              TRANSFORM_CHANNELS)
        elif cls == "MovieSceneFloatTrack":
            track = add(unreal.MovieSceneFloatTrack)
            track.set_property_name_and_path(props["PropertyName"], props["PropertyPath"])
            for s in props.get("Sections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, set(CURVE_CHANNELS))
                self.channels(section, sec["props"], CURVE_CHANNELS)
        elif cls == "MovieSceneFadeTrack":
            track = add(unreal.MovieSceneFadeTrack)
            for s in props.get("Sections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, set(CURVE_CHANNELS))
                self.channels(section, sec["props"], CURVE_CHANNELS)
        elif cls == "MovieSceneParticleTrack":
            track = add(unreal.MovieSceneParticleTrack)
            for s in props.get("ParticleSections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, set(PARTICLE_CHANNELS))
                self.channels(section, sec["props"], PARTICLE_CHANNELS, particle=True)
        elif cls == "MovieSceneAudioTrack":
            track = add(unreal.MovieSceneAudioTrack)
            for s in props.get("AudioSections", []):
                sec = pkg.get(s)
                sp = sec["props"]
                section = self.section(track, sec, set(AUDIO_CHANNELS) | AUDIO_KEYS)
                section.set_sound(self.sound(sp["Sound"]))
                # UE 4.24's bLooping (left at its default, true, in every one of these) repeats the sound when the
                # section outlasts it; UE 5.8 reads bRepeating for that (Sequencer.Audio.UseRepeating) and set_sound
                # derives bLooping from the sound, so both are set back.
                section.set_looping(True)
                section.set_repeating(True)
                section.set_start_offset(_frame(sp.get("StartFrameOffset", 0)))
                section.set_override_attenuation(bool(sp.get("bOverrideAttenuation", False)))
                if sp.get("AttenuationSettings"):
                    section.set_attenuation_settings(self.attenuation(sp["AttenuationSettings"]))
                self.channels(section, sp, AUDIO_CHANNELS)
        elif cls == "MovieSceneSkeletalAnimationTrack":
            track = add(unreal.MovieSceneSkeletalAnimationTrack)
            spans = []
            for s in props.get("AnimationSections", []):
                sec = pkg.get(s)
                spans.append(self.span(sec["props"]))
                section = self.section(track, sec, {"Params"})
                p = sec["props"]["Params"]
                unknown = set(p) - ANIMATION_KEYS
                if unknown:
                    raise ValueError("%s: animation params not handled: %s" % (sec["name"], sorted(unknown)))
                clip, reverse = self.clip(p["Animation"])
                # The params struct is a value, so it is read, written and put back.
                params = section.get_editor_property("params")
                params.set_editor_property("animation", clip)
                params.set_editor_property("slot_name", p.get("SlotName", "DefaultSlot"))
                # Ticks, and this sequence keeps the original's tick resolution, so the original's offset carries over
                # (it is an offset into another animation, but the stand-in is at least as long as the two we have).
                params.set_editor_property("start_frame_offset", _frame(p.get("StartFrameOffset", 0)))
                params.set_editor_property("reverse", reverse)
                self.play_rate(params, pkg, sec, p, clip)
                section.set_editor_property("params", params)
            self.fill_rest_pose(track, spans)
        elif cls == "MovieSceneVisibilityTrack":
            track = add(unreal.MovieSceneVisibilityTrack)
            track.set_property_name_and_path(props["PropertyName"], props["PropertyPath"])
            for s in props.get("Sections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, {"BoolCurve"})
                self.boolean(section.get_all_channels()[0], sec["props"].get("BoolCurve"))
        elif cls == "MovieSceneCameraShakeTrack":
            track = add(unreal.MovieSceneCameraShakeTrack)
            for s in props.get("CameraShakeSections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, {"ShakeData"})
                data = sec["props"]["ShakeData"]
                unknown = set(data) - SHAKE_KEYS
                if unknown:
                    raise ValueError("%s: shake data not handled: %s" % (sec["name"], sorted(unknown)))
                shake = section.get_editor_property("shake_data")
                shake.set_editor_property("shake_class", self.shake_class(data["ShakeClass"]))
                shake.set_editor_property("play_scale", float(data.get("PlayScale", 1.0)))
                section.set_editor_property("shake_data", shake)
        elif cls == "MovieSceneSlomoTrack":
            track = add(unreal.MovieSceneSlomoTrack)
            for s in props.get("Sections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, set(CURVE_CHANNELS))
                self.channels(section, sec["props"], CURVE_CHANNELS)
        elif cls == "MovieSceneComponentMaterialTrack":
            track = add(unreal.MovieSceneComponentMaterialTrack)
            # The export leaves MaterialIndex at 0 (the nurse's only slot); UE 5.8 wants that as a material info.
            track.set_material_info(unreal.ComponentMaterialInfo(
                material_slot_index=0, material_type=unreal.ComponentMaterialType.INDEXED_MATERIAL))
            for s in props.get("Sections", []):
                sec = pkg.get(s)
                section = self.section(track, sec, {"ScalarParameterNamesAndCurves"})
                for entry in sec["props"]["ScalarParameterNamesAndCurves"]:
                    name = entry["ParameterName"]
                    # A parameter's channel comes with the first key, which is then taken out again for the curve.
                    # UE 5.8 asks for the parameter's info; the original's is a global one (it has no layers).
                    info = unreal.MaterialParameterInfo(
                        name=name, association=unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, index=-1)
                    section.add_scalar_parameter_key(info, _frame(0), 0.0, "", "",
                                                     unreal.MovieSceneKeyInterpolation.AUTO)
                    by_name = {str(c.channel_name): c for c in section.get_all_channels()}
                    if name not in by_name:
                        raise ValueError("%s: no channel %s (have %s)" % (sec["name"], name, sorted(by_name)))
                    channel = by_name[name]
                    for key in channel.get_keys():
                        channel.remove_key(key)
                    self.curve(channel, entry["ParameterCurve"], None)
        elif cls == "MovieSceneCameraAnimTrack":
            # UE 5.8 has no camera anim track, and UWasamiCameraAnim (item 24) holds a CameraAnim's post-process and
            # field of view tracks, not a Matinee move track. The capture scene's CameraAnim_Nurse_01 is a move track
            # alone, which build reads before the binding's tracks go in and bake_location adds to its transform
            # track (item 28's step 14d).
            self.result["camera_anims"] += 1
            return
        elif cls == "MovieSceneEventTrack":
            # UE 4.24's legacy event track calls the level Blueprint's functions of the key's name; the hospital's
            # level Blueprints have none (DisablePlayerInput / EnablePlayerInput are 00_Ballroom's), so nothing happens.
            self.result["skipped_tracks"].append("%s: %s" % (pkg.rel, export["name"]))
            return
        else:
            raise ValueError("%s: track class %s not handled" % (pkg.rel, cls))
        self.result["tracks"] += 1

    # ---------------------------------------------------------------------------------------------- sequence
    def build(self, rel, resolve=None):
        """Makes (or empties and writes again) /Game/DD/<rel> from the original's package. Returns the asset."""
        pkg = _Package(rel)
        target = dd_assets.asset_path(rel)
        if EAL.does_asset_exist(target):
            seq = unreal.load_asset(target)
            for b in reversed(list(seq.get_bindings())):
                if b.is_valid():
                    b.remove()
            for t in list(seq.get_tracks()):
                seq.remove_track(t)
        else:
            folder, name = paths.split(target)
            seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.LevelSequence,
                                                                         unreal.LevelSequenceFactoryNew())
        ms = pkg.movie_scene
        tick = _rate(ms.get("TickResolution"), DEFAULT_TICK_RESOLUTION)
        display = _rate(ms.get("DisplayRate"), DEFAULT_DISPLAY_RATE)
        seq.set_tick_resolution_directly(tick)
        seq.set_display_rate(tick)             # frames are ticks while the ranges are written
        rng = ms["PlaybackRange"]
        if rng["lower"]["type"] != "Inclusive" or rng["upper"]["type"] != "Exclusive":
            raise ValueError("%s: playback range %s" % (rel, rng))
        seq.set_playback_start(int(rng["lower"]["value"]))
        seq.set_playback_end(int(rng["upper"]["value"]))
        self.playback = (int(rng["lower"]["value"]), int(rng["upper"]["value"]))

        possessables = {p["Guid"]: p for p in ms.get("Possessables", [])}
        references = (pkg.asset.get("BindingReferences") or {}).get("BindingIdToReferences", {})
        order = sorted(possessables.values(), key=lambda p: p["ParentGuid"] != "0" * 32)   # parents first
        bindings, targets = {}, {}
        for p in order:
            target_object = resolve(p, references.get(p["Guid"], {}).get("References", [])) if resolve else None
            if target_object is None:
                self.result["missing"].append("%s: %s" % (pkg.rel.rsplit("/", 1)[-1], p["Name"]))
                continue
            proxy = seq.add_possessable(target_object)
            proxy.set_display_name(p["Name"])
            bindings[p["Guid"]] = proxy
            targets[p["Guid"]] = target_object
            self.result["bindings"] += 1
        for ob in ms.get("ObjectBindings", []):
            proxy = bindings.get(ob["ObjectGuid"])
            if proxy is None:
                continue
            tracks = ob.get("Tracks", [])
            if self.moves(pkg, tracks):
                self.movable(targets[ob["ObjectGuid"]])
            # A camera anim of the binding's is added to its transform track, so it is read before the tracks go in.
            self.offset = next((self.camera_anim(pkg, p) for p in tracks
                                if pkg.get(p)["class"] == "MovieSceneCameraAnimTrack"), None)
            for path in tracks:
                self.track(pkg, proxy, path)
            self.offset = None
        for path in ms.get("MasterTracks", []):
            self.track(pkg, seq, path)
        seq.set_display_rate(display)
        EAL.save_asset(target, only_if_is_dirty=False)
        self.result["sequences"] += 1
        return seq


def _new_result():
    return {"sequences": 0, "bindings": 0, "tracks": 0, "sections": 0, "keys": 0, "made_movable": [],
            "sounds": 0, "sound_cues": 0, "attenuations": 0,
            "idle_fills": 0, "camera_shakes": 0, "camera_anims": 0, "sequence_actors": 0, "sequence_players": 0, "helpers": 0, "nurses": 0, "missing": [],
            "unlinked_players": [], "skipped_tracks": [], "missing_particles": []}


def _source_actors(eas):
    found = {}
    for actor in eas.get_all_level_actors():
        for t in actor.tags:
            if str(t).startswith("src:"):
                found.setdefault(str(t)[4:], actor)
    return found


def _placed_name(references):
    """The level actor a root binding points at ('/Game/06_Hospital_Zone_02.06_Hospital_Zone_02:PersistentLevel.spikes'
    → 'spikes')."""
    for r in references:
        path = r.get("ExternalObjectPath") or ""
        if ":PersistentLevel." in path:
            return path.split(":PersistentLevel.", 1)[1]
    return None


def _cine_camera(actor, camera, failures):
    """The original's own settings onto a placed cine camera: its component's, and its look-at tracking minus the actor
    it aims at, which is placed by this same pass and so is resolved after it. Returns that actor's name."""
    ue_props.apply(actor.get_cine_camera_component(), camera.get("component") or {}, failures=failures)
    look = dict(camera.get("lookAt") or {})
    tracked = look.pop("ActorToTrack", None)
    if look:
        settings = actor.get_editor_property("lookat_tracking_settings")
        ue_props.apply(settings, look, failures=failures)
        actor.set_editor_property("lookat_tracking_settings", settings)
    return tracked


def _look_at(actor, target):
    """The actor a placed cine camera turns to follow."""
    settings = actor.get_editor_property("lookat_tracking_settings")
    settings.set_editor_property("actor_to_track", target)
    actor.set_editor_property("lookat_tracking_settings", settings)


def _helpers(eas, zone, names, existing, result):
    """Places the bound actors the level assembly leaves out, as the original has them: the TargetPoints an audio track
    follows, the emitters a particle track fires, the cut scenes' cine cameras and their nurses."""
    from wasami_tools.pipeline import dd_level
    by_name = {a["name"]: a for a in zone["actors"]}
    level = {}
    tracking = []
    for name in names:
        a = by_name.get(name)
        if name in existing or a is None or not a["world"]:
            continue
        cls = a["class"]
        world = a["world"]
        if cls in HELPER_CLASSES:
            actor = eas.spawn_actor_from_class(HELPER_CLASSES[cls], dd_level._vec(world["location"]),
                                               dd_level._rot(world["quat_xyzw"]))
            actor.set_actor_scale3d(dd_level._vec(world["scale"]))
            if cls == "Emitter":
                missing = dd_level.set_emitter(actor, zone, name, level)
                if missing:
                    result["missing_particles"].append("%s: %s" % (name, missing))
            elif cls == "CineCameraActor":
                failures = []
                tracked = _cine_camera(actor, a.get("camera") or {}, failures)
                result["missing"] += ["%s: %s" % (name, f) for f in failures]
                if tracked:
                    tracking.append((actor, tracked))
            tags = ()
        elif cls in NURSE_CLASSES:
            actor = eas.spawn_actor_from_class(unreal.WasamiCutsceneNurse, dd_level._vec(world["location"]),
                                               dd_level._rot(world["quat_xyzw"]))
            actor.set_actor_scale3d(dd_level._vec(world["scale"]))
            actor.set_under_capsule(NURSE_CLASSES[cls])
            # The original's nurses wait hidden for the visibility track to show them.
            if a["props"].get("bHidden"):
                actor.set_actor_hidden_in_game(True)
            tags = (NURSE_TAG,)
            result["nurses"] += 1
        else:
            continue
        dd_level._tag(actor, name, SEQUENCE_FOLDER, SEQUENCE_TAG, "src:" + name, *tags)
        existing[name] = actor
        result["helpers"] += 1
    for actor, name in tracking:
        if name in existing:
            _look_at(actor, existing[name])
        else:
            result["missing"].append("%s looks at %s, which is not placed" % (actor.get_actor_label(), name))


def place_all(eas, zone_name, zone, result=None):
    """In the open level: the zone's bound helper actors, its sequences bound to the level's actors, and the
    LevelSequenceActors that play them (set as the secret elevators' Sequence); also the free sequences and the zone's
    camera shakes. The caller has taken out what an earlier call placed and saves the level."""
    from wasami_tools.pipeline import dd_level
    result = result if result is not None else _new_result()
    builder = _Builder(result)
    actors = {a["name"]: a for a in zone["actors"] if a["class"] == "LevelSequenceActor"}
    wanted = SEQUENCE_ACTORS.get(zone_name, ())
    packages = {}
    for name in wanted:
        if name not in actors:
            raise ValueError("%s has no level sequence actor %s" % (zone["map"], name))
        rel = dd_assets.game_rel(actors[name]["props"]["LevelSequence"])
        packages[name] = (rel, _Package(rel))
    existing = _source_actors(eas)
    helper_names = []
    for rel, pkg in packages.values():
        refs = (pkg.asset.get("BindingReferences") or {}).get("BindingIdToReferences", {})
        helper_names += [n for n in (_placed_name(r.get("References", [])) for r in refs.values()) if n]
    _helpers(eas, zone, helper_names, existing, result)

    def resolve(possessable, references):
        name = _placed_name(references)
        if name is not None:
            return existing.get(name)
        # a component of the parent binding's actor ('LightComponent0' of a spot light)
        parent_refs = []
        for rel, pkg in packages.values():
            parent_refs = (pkg.asset.get("BindingReferences") or {}).get("BindingIdToReferences", {}).get(
                possessable["ParentGuid"], {}).get("References", [])
            if parent_refs:
                break
        parent = existing.get(_placed_name(parent_refs) or "")
        component_name = next((r.get("ObjectPath") for r in references if r.get("ObjectPath")), possessable["Name"])
        if parent is None:
            return None
        found = next((c for c in parent.get_components_by_class(unreal.ActorComponent)
                      if c.get_name() == component_name), None)
        if found is None and parent.actor_has_tag(NURSE_TAG):
            # A nurse the original makes a SkeletalMeshActor binds its 'SkeletalMeshComponent0', which the stand-in
            # (whose only skinned mesh is named as the original's Character names it) does not have.
            found = next(iter(parent.get_components_by_class(unreal.SkeletalMeshComponent)), None)
        return found

    for name in wanted:
        rel, _ = packages[name]
        seq = builder.build(rel, resolve)
        world = actors[name]["world"]
        actor = eas.spawn_actor_from_class(unreal.LevelSequenceActor, dd_level._vec(world["location"]),
                                           dd_level._rot(world["quat_xyzw"]))
        actor.set_sequence(seq)
        dd_level._tag(actor, name, SEQUENCE_FOLDER, SEQUENCE_TAG, "src:" + name)
        result["sequence_actors"] += 1
    # The secret elevators play two of them: their Sequence is the actor just placed.
    linked, unlinked = dd_level.link_sequence_players(eas, zone)
    result["sequence_players"] += linked
    result["unlinked_players"] += unlinked
    for rel in FREE_SEQUENCES:
        builder.build(rel)
    for rel in CAMERA_SHAKES.get(zone_name, ()):
        dd_assets.camera_shake(rel, VERSION)
        result["camera_shakes"] += 1
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)   # the sounds' concurrency assets
    return result


def place(zone="Zone1", map_path=""):
    """Rebuilds the zone's sequences and puts their actors in again (taking out what an earlier call placed), and saves
    the level. Returns the counts, the bindings whose actor is not in the level ('missing'), the tracks left out and
    the emitters whose particle system is not imported yet."""
    from wasami_tools.pipeline import dd_level
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = dd_level._open_level(map_path or z["level"], clear=False)
    old = [a for a in eas.get_all_level_actors() if a.actor_has_tag(SEQUENCE_TAG)]
    result = _new_result()
    result["removed"] = len(old)
    if old:
        eas.destroy_actors(old)
    place_all(eas, zone, z, result)
    dd_level._save_level(les, map_path or z["level"])
    return result
