"""This game's own voices: the lines of the user's Wasami, as SoundWaves under /Game/Wasami/Voices.

The sources are SourceArt/Wasami/Voices/<id>.wav and the manifest.json beside them (this game's own source art, which
python Tools/dd/prepare_voices.py decodes from the WebGL version's mp3 and copies). The waves keep UE's defaults
(volume 1, no looping) and play through the original's Dialogue sound class, which is the bus the WebGL version played
them on (voice, at volume 1.0; its implementation record 06). The manifest's subtitle goes on the ones spoken with
one, so UE's own Subtitles put them up and take them down as they do the original's dialogue (dd_dialogue); the rest
carry none. Where and how loud each is played is the scene that uses it.

The lines (2026-09-27, the user's instruction: every voice but the original's nurses' and Bierce's is Wasami's, and
theirs become hers): SourceArt/Wasami/Voices/Lines/<id>.wav with lines.json, cut from the WebGL version's takes by
the same script, imported as /Game/Wasami/Voices/Lines/Wasami_Line_<Id>. REPLACES says which of the original's
voices each stands in for; the level sequences take it from there (dd_sequence), and the zones' flows and the title
name the same waves in C++ (WasamiZone1Flow, WasamiZone2Flow, WasamiTitleScreenWidget).
"""
import json
import os

from wasami_tools.pipeline import dd_assets, paths

SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "Voices")
MANIFEST = "manifest.json"
VOICES_ROOT = paths.WASAMI_ROOT + "/Voices"
# The bus the WebGL version played them on (voice) is the original's Dialogue sound class, as its dialogue's is.
SOUND_CLASS = "Audio/SoundMix/DD_SoundClass_Dialogue"

# What this game speaks, by the manifest's id (the WebGL version's records 06 and 15). Those with a subtitle up: the
# guide's greeting, the first shard, a boost made and the secret door opening (Game.say), and the enemies' call as one
# spots the player (which that version subtitles from game.ts).
SUBTITLED = ("greeting", "well", "fast", "best", "found")
# ... and those spoken with none: what an enemy calls out on patrol, the death screen's two, and the cry of the enemy
# that has caught the player (you, which the WebGL version never played and this game says over the capture, where a
# subtitle would only sit on top of a scene that fills the screen; WasamiCapture).
SILENT = ("calling", "others", "think", "remember", "fine", "over", "you")
CLIPS = SUBTITLED + SILENT
# The manifest's other three (follow, safe and wait) are not imported as clips: their takes come in with the lines
# below (lines.json), where they stand in for the original's nurses and Bierce.


LINES_SOURCE = os.path.join(SOURCE, "Lines")
LINES_JSON = "lines.json"
LINES_ROOT = VOICES_ROOT + "/Lines"


def line_asset(line_id):
    """'dekokode_bright' -> '/Game/Wasami/Voices/Lines/Wasami_Line_DekokodeBright'."""
    return "%s/Wasami_Line_%s" % (LINES_ROOT, "".join(part.capitalize() for part in line_id.split("_")))


_TT = "Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_"
_NURSE = "Audio/06_Hospital/Nurse/Nurse_Hospital_Zone01_"
# The original's voices (their paths under /Game/DD) and the Wasami wave that is played in their place. Bierce's are
# the guide's, spoken with a subtitle; the nurses' are this game's enemy Wasami, spoken without, as theirs are.
REPLACES = {
    # Bierce, through the zones' flows (AWasamiBierceTalk)
    _TT + "Event_09": line_asset("alert"),          # They could be anywhere! Stay alert! (after the intercom)
    _TT + "Event_10": line_asset("huh"),            # ... they can't hide the sounds they make (a door broken in)
    _TT + "Event_17": line_asset("hunch"),          # ... they're trying to kill you again (after the cell)
    _TT + "Event_19": line_asset("sasuga"),         # You need to find a way to get past her (the Matron)
    _TT + "Event_20": line_asset("nowwhile"),       # No time to rest. Get to the ring piece!
    _TT + "Event_21": line_asset("naruhodo"),       # You got it! Now get out of this twisted hospital!
    _TT + "Event_22": line_asset("gone"),           # Malak's not around? (the garage)
    _TT + "Gameplay_07": line_asset("korenanka"),   # ... handicap accessible nightmare (by the lift)
    _TT + "Gameplay_08": line_asset("iya"),         # Some pretty twisted therapy (the maze)
    # ... the quip a nurse coming close sets off (the SoundCue Bierce_TormentTherapy_Gameplay's five)
    _TT + "Gameplay_01": line_asset("kimo"),
    _TT + "Gameplay_02": line_asset("kimochi"),
    _TT + "Gameplay_03": line_asset("help"),
    _TT + "Gameplay_04": line_asset("muimi"),
    _TT + "Gameplay_05": line_asset("oomou"),
    # Bierce, in the scenes (level sequences)
    _TT + "Event_11": line_asset("wow"),            # Now that's a HUGE woman! (the capture)
    _TT + "Event_12": line_asset("eh"),             # Wait, did you hear something?
    _TT + "Event_13": line_asset("va"),             # Look out!!
    _TT + "Event_14": line_asset("acho"),           # Ugh, you careless oaf! (the cell)
    _TT + "Event_15": line_asset("dead"),           # Wake up, you idiot!
    _TT + "Event_16": line_asset("ketcha"),         # Get up. Now!!
    _TT + "Event_18B": line_asset("safe"),          # That was close. (the cell's door picked)
    # Bierce on the title screen
    "Audio/Titlescreen/Bierce_Title_Modified_03": line_asset("follow"),  # NEW GAME (no subtitle over the title)
    # the nurses over the intercom (the flows)
    "Audio/06_Hospital/Nurse_Hospital_Zone01_Event_37_Intercom": line_asset("dekokode_bright"),
    "Audio/06_Hospital/Nurse_Hospital_Zone01_Event_48_Intercom": line_asset("wait"),
    # the nurses in the parking lot's scene (06_Hospital_Zone1_06Event)
    _NURSE + "Detected_01": VOICES_ROOT + "/Wasami_Found",
    _NURSE + "Attack_04": line_asset("gero"),
    _NURSE + "Attack_06": line_asset("vooo"),
    _NURSE + "Attack_07": line_asset("uooo"),
    _NURSE + "Attack_08": line_asset("vaa"),
    _NURSE + "Laugh_08": line_asset("dufu"),
    # ... in the capture (06_Hospital_Zone2_Capture) and the cell (06_Hospital_Zone2_Cell)
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_38": line_asset("kodomo"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_39": VOICES_ROOT + "/Wasami_You",
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_40": line_asset("calling"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_41": line_asset("aanannka"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_42": line_asset("greeting"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_43": line_asset("haihai"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_44": line_asset("follow"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_45": line_asset("fire"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_46": line_asset("dekokode"),
    "Audio/06_Hospital/Nurse/Zone2/Nurse_Hospital_Zone01_Event_47": line_asset("okay"),
}


def replacement(rel):
    """The Wasami wave played in place of the original's voice /Game/DD/<rel>, or None for any other sound."""
    return REPLACES.get(rel)


def lines():
    """lines.json's lines by id ({'alert': {'subtitle': ..., 'subtitled': ..., ...}, ...})."""
    path = os.path.join(LINES_SOURCE, LINES_JSON)
    if not os.path.exists(path):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_voices.py --lines-only first." % path)
    with open(path, encoding="utf-8") as f:
        return {line["id"]: line for line in json.load(f)["lines"]}


def import_lines(sound_class):
    """Every line of lines.json as /Game/Wasami/Voices/Lines/Wasami_Line_<Id>, its subtitle at 0 s where it shows one.
    Returns how many."""
    entries = lines()
    for line_id, line in entries.items():
        subtitles = [(0.0, line["subtitle"])] if line["subtitled"] else []
        dd_assets.sound_file(os.path.join(LINES_SOURCE, line_id + ".wav"), line_asset(line_id),
                             subtitles=subtitles, sound_class=sound_class)
    return len(entries)


def asset_name(clip_id):
    """'greeting' -> 'Wasami_Greeting'."""
    return "Wasami_" + clip_id.capitalize()


def manifest():
    """The manifest's clips by id ({'greeting': {'subtitle': ..., 'duration': ..., ...}, ...})."""
    path = os.path.join(SOURCE, MANIFEST)
    if not os.path.exists(path):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_voices.py first." % path)
    with open(path, encoding="utf-8") as f:
        return {clip["id"]: clip for clip in json.load(f)["clips"]}


def voice(clip, sound_class, subtitled):
    """Imports the clip's wav as the SoundWave /Game/Wasami/Voices/Wasami_<Id>, with its subtitle at 0 s where it
    shows one. Returns its package path."""
    subtitles = [(0.0, clip["subtitle"])] if subtitled else []
    return dd_assets.sound_file(os.path.join(SOURCE, clip["id"] + ".wav"),
                                "%s/%s" % (VOICES_ROOT, asset_name(clip["id"])),
                                subtitles=subtitles, sound_class=sound_class)


def import_all():
    """Every voice this game speaks. Returns how many were made with a subtitle and how many without."""
    clips = manifest()
    missing = [clip_id for clip_id in CLIPS if clip_id not in clips]
    if missing:
        raise KeyError("%s has no %s" % (MANIFEST, ", ".join(missing)))
    sound_class = dd_assets.sound_class(SOUND_CLASS)
    for clip_id in CLIPS:
        voice(clips[clip_id], sound_class, clip_id in SUBTITLED)
    return {"subtitled": len(SUBTITLED), "silent": len(SILENT), "lines": import_lines(sound_class)}
