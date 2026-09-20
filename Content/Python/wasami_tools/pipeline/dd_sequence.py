"""Dark Deception's level sequences: the original's LevelSequence packages (pak_reference_2/_assets, which keep what
the _sequences summaries leave out — each section's completion mode, an audio section's start offset, attenuation and
row) rebuilt as UE LevelSequence assets under /Game/DD/<the original's path>, bound to the actors the level assembly
placed (tag 'src:<the original's name>'), and the LevelSequenceActors that play them where the original places them.
The original's level Blueprints play those actors (GetSequencePlayer → Play); AWasamiZoneFlow finds them by the same
tag. A binding points at the level's actor by its path in the map, so rebuilding the level (new actor names) has to
rebuild the sequences too, which dd_level.build does last."""
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
# scenes look through (the original's placed ones override nothing, so they keep the class's settings; their focal
# length, aperture and focus distance are what the sequences animate).
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
    "Nurse_Hospital_Zone01_Event_39": ("Chase_Charge", False),   # the punch that takes the player
    "nurse_idle_01": ("Idle", False),
    "ReaperNurse_Walk_Back": ("Walk", True),                # backing away
    "nurse_cloak": ("Walk", False),                         # she turns invisible; here she walks off
}
NURSE_ANIMS.update({"Nurse_Hospital_Zone01_Event_%d" % n: ("Idle", False) for n in range(40, 48)})

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
AUDIO_CHANNELS = {"SoundVolume": ("Volume", 1.0), "PitchMultiplier": ("Pitch", 1.0)}
CURVE_CHANNELS = {"FloatCurve": ("None", None)}   # a float property's and a fade's, without a default
PARTICLE_CHANNELS = {"ParticleKeys": ("None", None)}

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
        if clip not in self.clips:
            asset = unreal.load_asset(WASAMI_CLIP % clip)
            if asset is None:
                raise RuntimeError("%s is missing: run WasamiDDTools.import_wasami_enemy" % (WASAMI_CLIP % clip))
            self.clips[clip] = asset
        return self.clips[clip], reverse

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
                self.channels(section, sec["props"], TRANSFORM_CHANNELS)
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
            for s in props.get("AnimationSections", []):
                sec = pkg.get(s)
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
                section.set_editor_property("params", params)
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
            # UE 5.8 has no camera anim track (a CameraAnim is a UWasamiCameraAnim here), so the capture scene's
            # CameraAnim_Nurse_01 is played by the zone's flow while the scene runs (item 25's step 4).
            self.result["skipped_tracks"].append("%s: %s" % (pkg.rel, export["name"]))
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

        possessables = {p["Guid"]: p for p in ms.get("Possessables", [])}
        references = (pkg.asset.get("BindingReferences") or {}).get("BindingIdToReferences", {})
        order = sorted(possessables.values(), key=lambda p: p["ParentGuid"] != "0" * 32)   # parents first
        bindings = {}
        for p in order:
            target_object = resolve(p, references.get(p["Guid"], {}).get("References", [])) if resolve else None
            if target_object is None:
                self.result["missing"].append("%s: %s" % (pkg.rel.rsplit("/", 1)[-1], p["Name"]))
                continue
            proxy = seq.add_possessable(target_object)
            proxy.set_display_name(p["Name"])
            bindings[p["Guid"]] = proxy
            self.result["bindings"] += 1
        for ob in ms.get("ObjectBindings", []):
            proxy = bindings.get(ob["ObjectGuid"])
            if proxy is None:
                continue
            for path in ob.get("Tracks", []):
                self.track(pkg, proxy, path)
        for path in ms.get("MasterTracks", []):
            self.track(pkg, seq, path)
        seq.set_display_rate(display)
        EAL.save_asset(target, only_if_is_dirty=False)
        self.result["sequences"] += 1
        return seq


def _new_result():
    return {"sequences": 0, "bindings": 0, "tracks": 0, "sections": 0, "keys": 0, "sounds": 0, "sound_cues": 0,
            "attenuations": 0,
            "camera_shakes": 0, "sequence_actors": 0, "sequence_players": 0, "helpers": 0, "nurses": 0, "missing": [],
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


def _helpers(eas, zone, names, existing, result):
    """Places the bound actors the level assembly leaves out, as the original has them: the TargetPoints an audio track
    follows, the emitters a particle track fires, and the cut scenes' nurses."""
    from wasami_tools.pipeline import dd_level
    by_name = {a["name"]: a for a in zone["actors"]}
    level = {}
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
