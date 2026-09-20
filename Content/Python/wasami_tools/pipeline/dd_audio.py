"""Dark Deception's hospital audio: what the zones play on their own — the music the level's music player crossfades
(AWasamiMusicPlayer), the ambient sounds the level places (AmbientSound) and the reverb presets its AudioVolumes name.
The sounds of the moving parts, the powers, the enemies and the UI come with the tool that makes each of them
(dd_gimmicks, dd_powers, dd_tablet, dd_ui); this module is for the level's own sound.

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
# What the levels' AmbientSounds play (dd_level places them with the AudioComponent's own settings): the city outside
# Zone 1's windows and the parking lot, and the intercom Zone 2's flow speaks through.
AMBIENCE = (
    "Audio/06_Hospital/DD_City_Ambience_Creepy_Loop",
    "Audio/06_Hospital/Nurse_Hospital_Zone01_Event_48_Intercom",
)
# The reverb presets Zone 1's AudioVolumes name, which are the engine's own content (rebuilt under /Game/DD/_Engine,
# dd_assets.reverb_effect): the corridor by the lobby and the parking lot.
REVERBS = (
    "/Engine/EngineSounds/ReverbSettings/BunkerHall",
    "/Engine/EngineSounds/ReverbSettings/ParkingLot",
)
# The hospital is only in the latest version (pak_reference_2).
VERSION = 2


def import_music():
    """The three tracks the music players fade between. Returns their package paths."""
    return [dd_assets.sound(rel, VERSION) for rel in MUSIC]


def import_ambience():
    """The waves the levels' AmbientSounds play. Returns their package paths."""
    return [dd_assets.sound(rel, VERSION) for rel in AMBIENCE]


def import_reverbs():
    """The reverb presets the AudioVolumes name. Returns the assets."""
    return [dd_assets.reverb_effect(rel, VERSION) for rel in REVERBS]


def import_all():
    """Everything the level plays by itself. Returns how many of each kind were made."""
    return {"music": len(import_music()), "ambience": len(import_ambience()), "reverbs": len(import_reverbs())}
