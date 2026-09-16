"""Dark Deception's tablet powers: the sounds and camera shakes the power system (UWasamiPowerComponent) plays.
The icons on the tablet's sockets are the tablet's own (dd_tablet).

Sources: pak_reference_2 (UE 4.24, the latest version), which the powers follow except the teleport (pak_reference).
"""
import unreal

from wasami_tools.pipeline import dd_assets, paths

EAL = unreal.EditorAssetLibrary

# (pak_reference version, the original's path under /Game).
SOUNDS = (
    (2, "Audio/UI/power_refilled"),             # a power is ready again
    (2, "Audio/UI/Shard_Streak_Milestone_V5"),  # the speed boost starts
)
CAMERA_SHAKES = (
    (2, "UI/Menu/Streaks/BP_CameraShake_Streak"),  # the speed boost starts
)


def import_all():
    """Imports and builds every asset the powers use, then saves /Game/DD. Returns how many of each kind."""
    result = {"sounds": len([dd_assets.sound(rel, version) for version, rel in SOUNDS])}
    result["camera_shakes"] = len([dd_assets.camera_shake(rel, version) for version, rel in CAMERA_SHAKES])
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
