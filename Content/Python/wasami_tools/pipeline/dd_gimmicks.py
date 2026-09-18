"""Dark Deception's hospital gimmicks: what the stage's moving parts play. The double doors (AWasamiDoubleDoors, after
the original's Blueprints/06_Hospital/BP_06_DoubleDoors): the swing's two sounds and their attenuation (MonkeyAttenuation),
and the locked rattle (Locked_Door, a SoundCue of two waves) with the attenuation the doors play it through
(01_Lobby_Attenuation). The doors' meshes and materials come with the stage's assets (dd_stage), and the level build puts
them on the placed doors (dd_level).

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24).
"""
import unreal

from wasami_tools.pipeline import dd_assets, paths

EAL = unreal.EditorAssetLibrary
VERSION = 2   # the hospital is only in the latest version

DOUBLE_DOOR_SOUNDS = (
    "Audio/06_Hospital/SFX_06_DoubleDoor_Open",
    "Audio/06_Hospital/SFX_06_DoubleDoor_Close",
    "Audio/01_Hotel/Locked_Door_v1",
    "Audio/01_Hotel/Locked_Door_v2",
)
DOUBLE_DOOR_CUES = (
    "Audio/01_Hotel/Locked_Door",
)
DOUBLE_DOOR_ATTENUATIONS = (
    "Audio/Misc/MonkeyAttenuation",
    "Audio/01_Hotel/01_Lobby_Attenuation",
)


def import_double_doors():
    """The double doors' sounds, SoundCue and attenuations. Returns how many of each."""
    result = {"attenuations": len([dd_assets.sound_attenuation(rel, VERSION) for rel in DOUBLE_DOOR_ATTENUATIONS]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in DOUBLE_DOOR_SOUNDS])}
    result["sound_cues"] = len([dd_assets.sound_cue(rel, VERSION) for rel in DOUBLE_DOOR_CUES])
    return result


def import_all():
    """Imports the gimmicks' assets (the double doors'), then saves /Game/DD."""
    result = {"double_door_" + key: count for key, count in import_double_doors().items()}
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
