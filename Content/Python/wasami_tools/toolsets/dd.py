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
        UI textures and font, the woosh sounds, the minimap's render target, map images and materials, the map's
        arrow's material (M_Arrow_Inst, WasamiArrowPointer's plane) and the sentries' view cone marks' materials
        (map_enemy_search_Mat and 0_DotCircle_Mat, WasamiViewcone's planes).

        Returns:
            How many assets of each kind were made ('textures', 'materials', 'meshes', 'fonts', 'sounds', 'minimap',
            'arrow', 'viewcone').
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
        the lock giving (its key's font comes with import_dd_tablet). And the loading screen's portal sound and the
        hospital's emblem (WasamiLoadingWidget, which Zone 1 shows as it opens Zone 2; the emblem is composed first by
        python Tools/dd/prepare_loader.py outside the editor). And the hand's icon (WasamiInteractWidget, shown while
        the player looks at something it can use), and the ring piece's picture and pickup sound (WasamiRingPieceWidget,
        which Zone 2's altar puts up). And the shard streak's ten cards and four milestone sounds
        (WasamiShardStreakWidget and the game mode's Check Streak). And the level clear screen's (WasamiLevelClearWidget)
        You Escaped!, rules and hospital title, and its sounds (You Escaped!, the grade stamps, the counters' fill).
        And the title screen's (WasamiTitleScreenWidget) smoky mask, the strokes and their panning material, the hover
        smear, the music and NEW GAME's sound and voice, and this game's logo, its glow and Wasami's face (the glow and
        the face are baked first by python Tools/dd/prepare_title.py outside the editor).

        Returns:
            How many assets of each kind were made ('textures', 'fonts', 'sounds', 'door_break_textures',
            '_sounds', '_sound_cues', '_attenuations', '_materials', 'loading_sounds', 'loading_emblems',
            'interact_textures', 'ring_piece_textures', 'ring_piece_sounds', 'streak_textures', 'streak_sounds',
            'level_clear_textures', 'level_clear_sounds', 'title_textures', 'title_sounds', 'title_materials' and
            'title_wasami_textures').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_ui").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_gimmicks() -> dict[str, int]:
        """Imports (or re-imports) what the stage's moving parts play: the double doors' (WasamiDoubleDoors) swing
        sounds, the locked rattle's SoundCue and its waves, and their attenuations; the zone barrier's (WasamiZoneBarrier)
        hum and shatter, the sound it turns the player away with and its attenuation, its planes' materials (MM_SpeedBarrier and its two instances) and its burst (P_ky_impact3,
        after import_dd_shards, which makes the burst's other materials); the tunnel's doors broken in (the zone flow):
        their crash and the burst of concrete (Fracture_concrete_3, its textures and estimated materials); Zone 2's cell:
        the needles' stab as its spikes reach the player, and the particles its sequences fire (P_06_NurseSparks,
        Fracture_dark_slow, Concrete_impact_large, their textures and estimated materials); the parking lot's nurses
        stabbing at the tunnel's doors (WasamiEnemy06Chase's Hit FX): the slam's SoundCue and waves, and the dust
        (P_06_NurseDoorHit and its additive material); Zone 2's lifts: the clunk,
        the movement loop and the garage lifts' rising sound; the garage lifts' skinned mesh and its animation (after
        import_dd_stage_assets, which makes its materials); Zone 2's ring piece over the altar (WasamiRingPiece): its
        glow (P_08_RingPiece and its material, after import_dd_shards). The doors' and lifts' meshes and materials come with the
        stage's assets; the level build puts them and the barriers' materials on the placed actors, and the bursts'
        systems on their emitters (place_dd_sequences those the sequences fire).

        Returns:
            How many assets of each kind were made ('double_door_attenuations', '_sounds', '_sound_cues',
            'zone_barrier_attenuations', '_sounds', '_textures', '_materials', '_particle_systems',
            'doors_busted_sounds', '_textures', '_materials', '_particle_systems', 'cell_sounds', '_textures',
            '_materials', '_particle_systems', 'nurse_door_hit_sounds', '_sound_cues', '_materials',
            '_particle_systems', 'lift_attenuations', '_sounds', 'garage_lift_skeletal_meshes',
            '_animations', and 'ring_piece_materials', '_particle_systems').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("gltf")
        _module("dd_enemy")
        _module("dd_skeletal")
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
