"""Dark Deception's dialogue of the hospital: the lines Bierce speaks over the two zones this game builds (Torment
Therapy's Zone 1 and Zone 2) and the announcement Zone 1's flow plays over the intercom, with subtitles.

The waves are the original's own, with its settings (volume 2.0 and the Dialogue sound class, which dd_assets.sound
writes from the export). What plays them is AWasamiBierceTalk and the zones' flows (implementation record 11-zone-flow),
as the original's BierceTalk_Blueprint and its levels do; the quips the nurses near the player set off go through the
SoundCue Bierce_TormentTherapy_Gameplay, which picks one of five waves at random.

Subtitles: the original's hospital dialogue carries none of its own (only the entrance level this game does not build
has them on the nurses' lines), so the text comes from the original's string table Strings by the names'
correspondence — Bierce_TormentTherapy_Event_NN is 06_Cutscene_Zone_01_Bierce_NN and ..._Gameplay_NN is
06_Gameplay_Zone_01_Bierce_NN, as the 4th chapter's Bierce waves, which do carry theirs, are keyed. They are written as
plain text (unreal.Text), not as entries of a string table asset, as the mystery room's notes are (dd_level).
"""
from wasami_tools.pipeline import dd_assets

# The hospital is only in the latest version (pak_reference_2).
VERSION = 2

TT = "Audio/Dialogue/Bierce/Ch06/TT/"
# What the zones speak directly (AWasamiBierceTalk.Talk, always unattenuated):
#   Zone 1: Event_10 as a nurse breaks a door in (04_DoorBreak), Event_09 13 s after the intercom (04_Intercom).
#   Zone 2: Event_17 after the cell cutscene, Gameplay_07 by the lift, Gameplay_08 as the maze starts, Event_20 once
#           every shard of the maze is taken, Event_21 after the ring piece, Event_22 in the garage, Event_19 at the
#           Matron.
# The entrance level's (Event_01..08) and the boss fight's (Event_11..16, 18B, 23, 24) are not built (CLAUDE.md).
LINES = (
    TT + "Bierce_TormentTherapy_Event_09",
    TT + "Bierce_TormentTherapy_Event_10",
    TT + "Bierce_TormentTherapy_Event_17",
    TT + "Bierce_TormentTherapy_Event_19",
    TT + "Bierce_TormentTherapy_Event_20",
    TT + "Bierce_TormentTherapy_Event_21",
    TT + "Bierce_TormentTherapy_Event_22",
    TT + "Bierce_TormentTherapy_Gameplay_07",
    TT + "Bierce_TormentTherapy_Gameplay_08",
)
# The quip a nurse coming close sets off: the SoundCue's SoundNodeRandom picks one of these five at even weights, and
# the cue itself plays through DialogueAttenuation (dd_assets.sound_cue makes it). Gameplay_06 is not in the original.
GAMEPLAY_CUE = TT + "Bierce_TormentTherapy_Gameplay"
GAMEPLAY_WAVES = tuple(TT + "Bierce_TormentTherapy_Gameplay_0%d" % n for n in (1, 2, 3, 4, 5))
# What Zone 1's flow plays over the intercom before Event_09 (PlaySound2D at volume 0.6). This wave has no sound class
# of its own, as the original's other intercom announcement (dd_audio's AMBIENCE) has none.
INTERCOM = "Audio/06_Hospital/Nurse_Hospital_Zone01_Event_37_Intercom"

# The original's string table, and how a wave's name becomes the key of its line in it.
STRINGS = "Blueprints/Main/Strings/Strings"
SUBTITLE_PREFIXES = (
    ("Bierce_TormentTherapy_Event_", "06_Cutscene_Zone_01_Bierce_"),
    ("Bierce_TormentTherapy_Gameplay_", "06_Gameplay_Zone_01_Bierce_"),
)
SUBTITLE_KEYS = {"Nurse_Hospital_Zone01_Event_37_Intercom": "06_Cutscene_Zone_01_Nurse_01"}


def strings():
    """The entries of the original's string table Strings ({key: text})."""
    pkg = dd_assets.export_json(STRINGS, VERSION)
    return next(e for e in pkg["exports"] if e.get("string_table"))["string_table"]["entries"]


def subtitle_key(name):
    """The string table's key of the wave named name ('Bierce_TormentTherapy_Event_09')."""
    if name in SUBTITLE_KEYS:
        return SUBTITLE_KEYS[name]
    for prefix, key_prefix in SUBTITLE_PREFIXES:
        if name.startswith(prefix):
            return key_prefix + name[len(prefix):]
    raise KeyError("no subtitle key for %s" % name)


def line(rel, entries=None):
    """Imports the wave of /Game/<rel> with its line of the string table as its only subtitle (at 0 s). Returns its
    package path."""
    entries = strings() if entries is None else entries
    text = entries[subtitle_key(rel.rsplit("/", 1)[-1])]
    return dd_assets.sound(rel, VERSION, subtitles=[(0.0, text)])


def import_lines():
    """The lines the zones speak directly. Returns their package paths."""
    entries = strings()
    return [line(rel, entries) for rel in LINES]


def import_quips():
    """The nurse quips' five waves and the SoundCue that picks between them. Returns their package paths."""
    entries = strings()
    return [line(rel, entries) for rel in GAMEPLAY_WAVES] + [dd_assets.sound_cue(GAMEPLAY_CUE, VERSION)]


def import_intercom():
    """The intercom announcement Zone 1's flow plays. Returns its package path."""
    return line(INTERCOM)


def import_all():
    """Everything the zones speak. Returns how many of each kind were made."""
    return {"lines": len(import_lines()), "quips": len(import_quips()) - 1, "cues": 1,
            "intercom": len([import_intercom()])}
