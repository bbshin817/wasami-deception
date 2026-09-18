import importlib

import unreal

import toolset_registry


def _module(name):
    from wasami_tools.pipeline import paths, ue_props
    importlib.reload(paths)
    importlib.reload(ue_props)
    return importlib.reload(importlib.import_module("wasami_tools.pipeline." + name))


@unreal.uclass()
class WasamiDDTools(unreal.ToolsetDefinition):
    """Rebuilds assets of Dark Deception, the original game, from its exported data (pak_reference) under /Game/DD,
    mirroring the original's /Game paths."""

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_camera_shakes(asset_paths: list[str]) -> list[str]:
        """Creates (or rewrites) LegacyCameraShake Blueprints with the original camera shakes' settings.

        Args:
            asset_paths: The original's shake Blueprints, as paths under its /Game without the prefix
                (e.g. 'Blueprints/Main/BP_DD_PlayerCharacter_WalkShake').

        Returns:
            The package paths of the Blueprints under /Game/DD.
        """
        if not asset_paths:
            raise ValueError("asset_paths must not be empty.")
        _module("dd_stage")
        dd = _module("dd_assets")
        return [dd.camera_shake(p) for p in asset_paths]

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_tablet() -> dict[str, int]:
        """Imports (or re-imports) everything the player's tablet needs: its mesh, materials and textures, the screen's
        UI textures and font, the woosh sounds, and the minimap's render target, map images and materials.

        Returns:
            How many assets of each kind were made ('textures', 'materials', 'meshes', 'fonts', 'sounds', 'minimap').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_tablet").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_ui() -> dict[str, int]:
        """Imports (or re-imports) what the death screen (WasamiDeathScreenWidget) shows and plays: the life icon and
        YOU ARE DEAD, the menu's font (helvetica-normal), and the life-lost sound and the game-over music. Its vignette,
        heading font and select sound come with import_dd_tablet. Also the door breaks' lock (WasamiSwitchboxWidget,
        WasamiDoorBreak): the ring and spark materials and their textures, the lockpicking SoundCue and the sounds of
        the lock giving (its key's font comes with import_dd_tablet). And the loading screen's portal sound
        (WasamiLoadingWidget, which Zone 1 shows as it opens Zone 2).

        Returns:
            How many assets of each kind were made ('textures', 'fonts', 'sounds', 'door_break_textures',
            '_sounds', '_sound_cues', '_attenuations', '_materials', and 'loading_sounds').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_ui").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_gimmicks() -> dict[str, int]:
        """Imports (or re-imports) what the stage's moving parts play: the double doors' (WasamiDoubleDoors) swing
        sounds, the locked rattle's SoundCue and its waves, and their attenuations; the zone barrier's (WasamiZoneBarrier)
        hum and shatter, its planes' materials (MM_SpeedBarrier and its two instances) and its burst (P_ky_impact3,
        after import_dd_shards, which makes the burst's other materials); the tunnel's doors broken in (the zone flow):
        their crash and the burst of concrete (Fracture_concrete_3, its textures and estimated materials). The doors'
        meshes and materials come with the stage's assets; the level build puts them and the barriers' materials on the
        placed actors, and the burst's system on its emitter.

        Returns:
            How many assets of each kind were made ('double_door_attenuations', '_sounds', '_sound_cues',
            'zone_barrier_attenuations', '_sounds', '_textures', '_materials', '_particle_systems', and
            'doors_busted_sounds', '_textures', '_materials', '_particle_systems').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_gimmicks").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_powers() -> dict[str, int]:
        """Imports (or re-imports) what the tablet's powers show and play: their sounds (with the original SoundWave
        settings and sound concurrency), camera shakes, camera anims (as WasamiCameraAnim), the speed boost's
        textures and materials with the player's FX material, the teleport aim's materials and Cascade particle
        system (rebuilt from the original's exported package), Primal Fear's sphere texture and material, Vanish's
        puff (textures, material, Cascade particle system) and vignette material, the telepathy marker's material,
        and the telekinesis's force field (textures, meshes, estimated materials, Cascade particle system). The power
        icons come with import_dd_tablet.

        Returns:
            How many assets of each kind were made ('sounds', 'camera_shakes', 'camera_anims', 'textures', 'meshes',
            'materials', 'particle_systems').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("dd_particles")
        return _module("dd_powers").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_shards() -> dict[str, int]:
        """Imports (or re-imports) what the soul shards (WasamiShard) show and play: this game's mochi (mesh, textures,
        material; from SourceArt), the minimap mark's material (M_Shard), and the pickup's sound, cue, sound
        concurrency, camera shake and flash (P_ky_flash3: its textures, estimated materials and Cascade particle
        system) from the original.

        Returns:
            How many assets of each kind were made ('sounds', 'sound_concurrencies', 'sound_cues', 'camera_shakes',
            'materials', 'mochi', 'flash_textures', 'flash_materials', 'particle_systems').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("dd_particles")
        return _module("dd_shards").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_wasami_enemy() -> dict[str, int]:
        """Imports (or re-imports) the enemy Wasami (WasamiEnemy) from this game's model in SourceArt: its skeletal
        mesh, skeleton and physics asset, its textures and material, and its animations named A_WasamiEnemy_<role>
        (Idle, Walk, Run, Stun_KnockDown, Capture_1 …), after writing the prepared glb (animations resampled at 30 fps,
        loops closed, chase variants in place, the stun's falls with their get-ups) under
        Intermediate/Pipeline/wasami/enemy.

        Returns:
            How many assets of each kind were made ('textures', 'materials', 'meshes', 'animations').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("gltf")
        return _module("dd_enemy").import_all()
