"""Dark Deception's hospital audio: what the zones play on their own (the music the level's music player crossfades,
AWasamiMusicPlayer). The sounds of the moving parts, the powers, the enemies and the UI come with the tool that makes
each of them (dd_gimmicks, dd_powers, dd_tablet, dd_ui); this module is for the level's own sound.

Every wave keeps the original's own settings (looping, volume, sound class) — dd_assets.sound writes them from the
export."""
from wasami_tools.pipeline import dd_assets

# The hospital's music (pak_reference_2's Audio/06_Hospital/Music, under the original's /Game): the two zones' regular
# tracks and the panic track they share. The level's music players name them by their paths under /Game/DD
# (WasamiMusicPlayer.cpp), so the names are kept as they are.
MUSIC = (
    "Audio/06_Hospital/Music/DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING",
    "Audio/06_Hospital/Music/DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING",
    "Audio/06_Hospital/Music/DD_-_Dark_Deception_-_Chapter_4_Hospital_-_Panic_Track_v1_2_-_LOOPING",
)
# The hospital is only in the latest version (pak_reference_2).
VERSION = 2


def import_music():
    """The three tracks the music players fade between. Returns their package paths."""
    return [dd_assets.sound(rel, VERSION) for rel in MUSIC]


def import_all():
    """Everything the level plays by itself. Returns how many of each kind were made."""
    return {"music": len(import_music())}
