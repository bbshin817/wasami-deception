import importlib
import json

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
        process volumes, player starts, the minimap's plane, the soul shards, the flow's trigger boxes, volumes, door
        breaks and double doors, Zone 2's lifts), replacing what an earlier build placed, and saves it. The level is left open. The
        meshes are made again, so the level's lighting has to be baked again.

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
    def place_dd_minimap(zone: str = "Zone2", map_path: str = "") -> dict[str, int]:
        """Puts one zone's minimap plane (the zone's map under the level, which the player's minimap capture draws) in
        again, and Zone 2's floor boxes (WasamiMapArea, the original's BP_MapArea) with its plane that shows the map of
        the floor the player is in (WasamiMapTextureMultiFloor, BP_MapTexture_MultiFloor, with its Map), taking out
        what an earlier build placed, and saves the level. Zone 2's baked lighting stays valid (its plane is movable);
        Zone 1's plane is static, so its level counts as unbuilt until the lighting is built again. The map materials
        and textures come from WasamiDDTools.import_dd_tablet.

        Args:
            zone: 'Zone2' (1 plane, 2 floor boxes) or 'Zone1' (1 plane).
            map_path: Package path of the level; the zone's own is used when this is empty.

        Returns:
            'removed', 'mapPlane', 'mapAreas' and 'failed_settings' (listed in the output log).
        """
        return _module("dd_level").place_minimap(zone, map_path)

    @toolset_registry.tool_call
    @staticmethod
    def place_dd_flow(zone: str = "Zone1", map_path: str = "") -> dict[str, int]:
        """Puts one zone's trigger boxes (WasamiTriggerBox, the original's BP_TriggerBox_Base), blocking and trigger
        volumes, door breaks (WasamiDoorBreak, BP_06_Hospital_DoorBreak, with their Progress Speed), the double doors
        and emitters the flow names (WasamiDoubleDoors, BP_06_DoubleDoors, with the original's meshes and materials;
        the emitters asleep, with their particle systems), the zone barriers (WasamiZoneBarrier, with their planes'
        materials), the zone shard checkers (WasamiZoneShardChecker, the tablet's arrow's box over the zone) and Zone 2's
        lifts (WasamiLift, BP_06_Lift_03 / _04, and WasamiCornerLift, BP_06_LiftBase_Corner, with their meshes and box
        sizes; after import_dd_stage_assets) in again where the original places them, each tagged
        'src:<the original's name>' for the zone's flow and fixed to the ambulance or the spikes it moves with, taking
        out what an earlier call placed, and saves the level. Nothing else changes, and the baked lighting stays valid
        (none of them is in it).

        Args:
            zone: 'Zone1' (6 trigger boxes, 9 volumes, 1 door break, 2 double doors, 1 emitter, 1 barrier, 1 shard
                checker) or 'Zone2' (8 trigger boxes, 10 volumes, 1 door break, 1 barrier, 1 shard checker, 15 lifts).
            map_path: Package path of the level; the zone's own is used when this is empty.

        Returns:
            'removed', 'removed_lights', 'triggers', 'volumes', 'doorBreaks', 'doubleDoors', 'emitters',
            'zoneBarriers', 'shardCheckers', 'lifts', 'attached' and 'failed_settings' (listed in the output log).
        """
        return _module("dd_level").place_flow(zone, map_path)

    @toolset_registry.tool_call
    @staticmethod
    def place_dd_sequences(zone: str = "Zone1", map_path: str = "") -> str:
        """Rebuilds one zone's level sequences from the original's (the ones its flow plays, under
        /Game/DD/Animation/06_Hospital, and the fade /Game/DD/Animation/00_Ballroom/Ballroom_Event_Fade) bound to the
        level's actors, importing the sounds and attenuations they use and the camera shakes the flow plays with them,
        and puts their LevelSequenceActors and the bound
        TargetPoints and emitters in again where the original places them (tag 'src:<the original's name>'), taking out
        what an earlier call placed, and saves the level. build_dd_stage_level does this last; call this after changing
        the sequences' pipeline.

        Args:
            zone: 'Zone1' (the elevator's arrival, the ambulance's take-off) or 'Zone2' (the spikes, the cell door).
            map_path: Package path of the level; the zone's own is used when this is empty.

        Returns:
            JSON of the counts ('sequences', 'bindings', 'tracks', 'sections', 'keys', 'sounds', 'attenuations',
            'camera_shakes', 'sequence_actors', 'helpers', 'removed'), 'missing' (bindings whose actor is not in the
            level), 'skipped_tracks' and 'missing_particles' (emitters placed without their particle system). (A
            toolset's dict has to hold one type; this one mixes counts and lists.)
        """
        return json.dumps(_module("dd_sequence").place(zone, map_path))
