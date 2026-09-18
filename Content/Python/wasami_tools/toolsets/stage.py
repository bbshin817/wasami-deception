import importlib

import unreal

import toolset_registry


def _module(name):
    """The pipeline module, read again so edits to it apply without restarting the editor."""
    from wasami_tools.pipeline import paths, ue_props
    importlib.reload(paths)
    importlib.reload(ue_props)
    module = importlib.import_module("wasami_tools.pipeline." + name)
    return importlib.reload(module)


@unreal.uclass()
class WasamiStageTools(unreal.ToolsetDefinition):
    """Builds this game's stage, Dark Deception's hospital (06_Hospital Zone 1 and Zone 2), from the pipeline data
    written by Tools/dd/prepare_stage.py: imports its meshes, textures and materials under /Game/DD."""

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_stage_assets(max_items: int = 40) -> dict[str, int]:
        """Imports the stage's next missing meshes, then textures, then material instances, and saves them.

        Args:
            max_items: Most assets to create in this call; call again until 'remaining' is 0.

        Returns:
            'imported' in this call, 'remaining' overall, and '<kind>_done' / '<kind>_total' for meshes, textures and
            materials.
        """
        return _module("dd_stage").import_batch(max_items)

    @toolset_registry.tool_call
    @staticmethod
    def refresh_dd_stage_assets() -> dict[str, int]:
        """Brings the already imported stage assets up to the current pipeline: rebuilds a master material whose graph
        changed, re-applies each texture's compression, sRGB and LOD group and each mesh's lightmap resolution and UV
        channel, and recompiles the material instances.

        Returns:
            'masters_rebuilt', 'textures_updated', 'lightmaps_updated' and 'materials_updated'.
        """
        return _module("dd_stage").refresh_settings()

    @toolset_registry.tool_call
    @staticmethod
    def build_dd_stage_level(zone: str = "Zone1", map_path: str = "") -> dict[str, int]:
        """Builds one zone's level from the imported assets (meshes, lights, reflection captures, fog, sky light, post
        process volumes, player starts, the minimap's plane, the soul shards, the flow's trigger boxes and volumes), replacing what an earlier build placed,
        and saves it. The level is left open. The meshes are made again, so the level's lighting has to be baked again.

        Args:
            zone: 'Zone1' (06_Hospital_Zone_01) or 'Zone2' (06_Hospital_Zone_02).
            map_path: Package path of the level; the zone's own is used when this is empty, and it is created when
                missing.

        Returns:
            Placed actors per kind, and 'failed_settings' for properties that could not be applied (listed in the
            output log under LogPython).
        """
        return _module("dd_level").build(zone, map_path)

    @toolset_registry.tool_call
    @staticmethod
    def place_dd_shards(zone: str = "Zone1", map_path: str = "") -> dict[str, int]:
        """Puts one zone's soul shards (WasamiShard) in again where the original places them, taking out the shards and
        the separate shard lights an earlier build placed, and saves the level. Nothing else changes, and the baked
        lighting stays valid (the shards are movable). The shard assets come from WasamiDDTools.import_dd_shards.

        Args:
            zone: 'Zone1' (337 shards) or 'Zone2' (342).
            map_path: Package path of the level; the zone's own is used when this is empty.

        Returns:
            'removed_shards', 'removed_lights' and 'shards' (placed).
        """
        return _module("dd_level").place_shards(zone, map_path)

    @toolset_registry.tool_call
    @staticmethod
    def place_dd_flow(zone: str = "Zone1", map_path: str = "") -> dict[str, int]:
        """Puts one zone's trigger boxes (WasamiTriggerBox, the original's BP_TriggerBox_Base) and blocking and trigger
        volumes in again where the original places them, each tagged 'src:<the original's name>' for the zone's flow
        and fixed to the ambulance or the spikes it moves with, taking out what an earlier call placed, and saves the
        level. Nothing else changes, and the baked lighting stays valid (none of them is drawn).

        Args:
            zone: 'Zone1' (6 trigger boxes, 9 volumes) or 'Zone2' (8 trigger boxes, 10 volumes).
            map_path: Package path of the level; the zone's own is used when this is empty.

        Returns:
            'removed', 'triggers', 'volumes', 'attached' and 'failed_settings' (listed in the output log).
        """
        return _module("dd_level").place_flow(zone, map_path)
