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
    def import_dd_sound_classes() -> dict[str, int]:
        """Makes (or rewrites) the original's sound mix DD_SoundMix and its sound classes (Music, SFX with its children
        SFX_UI and SFX_Movies, Dialogue) under /Game/DD/Audio/SoundMix, and gives every SoundWave and SoundCue already
        under /Game/DD the class its export names (none where it names none), without importing the sounds again.
        Sounds imported after this get their class as they are imported.

        Returns:
            How many sounds went to each class ('DD_SoundClass_SFX' …, 'None') and how many changed ('changed').
        """
        _module("dd_stage")
        return _module("dd_assets").sound_classes()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_audio() -> dict[str, int]:
        """Imports (or re-imports) what the hospital's levels play by themselves: the music the zones' music players
        (WasamiMusicPlayer) crossfade — Zone 1's and Zone 2's regular tracks and the panic track they share — with the
        original SoundWaves' own settings (looping, volume and the Music sound class); what the levels' AmbientSounds
        play (Zone 1's city ambience, Zone 2's intercom); and the engine reverb presets Zone 1's AudioVolumes name
        (BunkerHall, ParkingLot), rebuilt under /Game/DD/_Engine from the original's exports. The moving parts', the
        powers', the enemies' and the UI's sounds come with import_dd_gimmicks, import_dd_powers, import_wasami_enemy
        and import_dd_ui.

        Returns:
            How many assets of each kind were made ('music', 'ambience', 'reverbs').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_audio").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_dialogue() -> dict[str, int]:
        """Imports (or re-imports) the hospital's dialogue: the nine lines Bierce speaks over Zone 1 and Zone 2
        (AWasamiBierceTalk), the five waves of the quips a nurse coming close sets off and the SoundCue that picks
        between them (Bierce_TormentTherapy_Gameplay, through DialogueAttenuation), and the announcement Zone 1's flow
        plays over the intercom. Every wave keeps the original's settings (volume 2.0, the Dialogue sound class) and
        gets the line of the original's string table Strings as its subtitle, which the original's hospital waves do
        not carry themselves. The entrance level's and the boss fight's lines are not imported.

        Returns:
            How many assets of each kind were made ('lines', 'quips', 'cues', 'intercom').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_dialogue").import_all()

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
        You Escaped!, rules and this game's level title (drawn first by python Tools/dd/prepare_level_title.py outside the
        editor), and its sounds (You Escaped!, the grade stamps, the counters' fill).
        And the title screen's (WasamiTitleScreenWidget) smoky mask, the strokes and their panning material, the hover
        smear, the music and NEW GAME's sound and voice, and this game's logo, its glow and Wasami's face (the glow and
        the face are baked first by python Tools/dd/prepare_title.py outside the editor). And the options screen's
        (WasamiOptionsWidget) frame, value boxes, arrows, slider thumb and check boxes. And the stage's title card's
        (WasamiChapterPortalWidget) portal ring, runes and two banners. And the extras screen's (after the latest
        version's UMG_Extras) play and pause icons, two locks, buttons' frame, opening sound and the material of the
        strokes behind it (MM_TitleScreen_Mask_); the original's art, music, diaries and movies are left out.

        Returns:
            How many assets of each kind were made ('textures', 'fonts', 'sounds', 'door_break_textures',
            '_sounds', '_sound_cues', '_attenuations', '_materials', 'loading_sounds', 'loading_emblems',
            'interact_textures', 'ring_piece_textures', 'ring_piece_sounds', 'streak_textures', 'streak_sounds',
            'level_clear_textures', 'level_clear_sounds', 'level_clear_wasami_textures', 'title_textures', 'title_sounds', 'title_materials',
            'title_wasami_textures', 'options_textures', 'chapter_portal_textures', 'extras_textures', 'extras_sounds'
            and 'extras_materials').
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
        after import_dd_shards, which makes the burst's other materials); the speed barriers' (WasamiSpeedBarrier) planes'
        instances of MM_SpeedBarrier, their burst (P_ky_impact2) and camera shake (BP_01_DoorExplode_CameraShake); the tunnel's doors broken in (the zone flow):
        their crash and the burst of concrete (Fracture_concrete_3, its textures and estimated materials); Zone 2's cell:
        the needles' stab as its spikes reach the player, and the particles its sequences fire (P_06_NurseSparks,
        Fracture_dark_slow, Concrete_impact_large, their textures and estimated materials); the parking lot's nurses
        stabbing at the tunnel's doors (WasamiEnemy06Chase's Hit FX): the slam's SoundCue and waves, and the dust
        (P_06_NurseDoorHit and its additive material); Zone 2's lifts: the clunk,
        the movement loop and the garage lifts' rising sound; the garage lifts' skinned mesh and its animation (after
        import_dd_stage_assets, which makes its materials); Zone 2's ring piece over the altar (WasamiRingPiece): its
        glow (P_08_RingPiece and its material, after import_dd_shards); the garage's portal (WasamiPortal); the
        defibrillators (WasamiDefib): the charge's hum, the crackle as the player is hit and the discharge (P_06_Defib,
        its lightning's textures and estimated material, and the meshes of its undrawn mesh emitters); Zone 2's saw
        traps: their whirring loop and their four skinned meshes with their animations (after import_dd_stage_assets,
        which makes their materials). The doors',
        lifts' and defibrillators' meshes and materials come with the stage's assets; the level build puts them and the barriers' materials on the placed actors, and the bursts'
        systems on their emitters (place_dd_sequences those the sequences fire).

        Returns:
            How many assets of each kind were made ('double_door_attenuations', '_sounds', '_sound_cues',
            'zone_barrier_attenuations', '_sounds', '_textures', '_materials', '_particle_systems',
            'speed_barrier_materials', '_particle_systems', '_camera_shakes',
            'doors_busted_sounds', '_textures', '_materials', '_particle_systems', 'cell_sounds', '_textures',
            '_materials', '_particle_systems', 'nurse_door_hit_sounds', '_sound_cues', '_materials',
            '_particle_systems', 'lift_attenuations', '_sounds', 'garage_lift_skeletal_meshes',
            '_animations', 'ring_piece_materials', '_particle_systems', the portal's ('portal_…'), and
            'defib_attenuations', '_sounds', '_textures', '_meshes', '_materials', '_particle_systems',
            'saw_trap_sounds', '_skeletal_meshes', '_animations').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("gltf")
        _module("dd_enemy")
        _module("dd_skeletal")
        _module("dd_powers")
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
    def import_dd_specials() -> dict[str, int]:
        """Imports (or re-imports) what the special shards (WasamiPowerOrb, the stun orb, and WasamiBonusShard, the
        red shard) show and play: their meshes (power_orb, soul_shard), the crystal material (m_crystal, estimated off
        its compiled shaders) with the orb's and the red shard's instances, the map marks' materials (M_PowerOrb, and
        M_Bonus_Shard and M_Enemy cut to T_EnemyTriangle), the flashes as they move and are taken (P_ky_flash_PowerOrb_
        and P_ky_flash_BonusOrb_Appear / _Disappear, P_ky_impact, P_ky_impact1, with their textures and the materials
        not made yet), and the pickup and wave sounds. After import_dd_shards and import_dd_gimmicks, whose map mark
        master and particle materials these share.

        Returns:
            How many assets of each kind were made ('sounds', 'textures', 'crystal_materials', 'meshes',
            'map_mark_materials', 'flash_materials', 'particle_systems').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("dd_particles")
        return _module("dd_specials").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_secrets() -> dict[str, int]:
        """Imports (or re-imports) what the secrets show and play: the secret files' (WasamiCollectable) pickup sound
        and the secret file's material on its mesh, UMG_Collectables' icons and frame, Zone 2's mysterious room's
        (WasamiSecretRoomZone) whispers, sting and picture and its glitch (M_GlitchHLSL, estimated off its compiled
        shader as M_DD_ChameleonGlitch), the secret wall's (WasamiSecretWall) sliding sound and the lore note's sound
        (WasamiMysteryNoteWidget). After prepare_stage and import_dd_stage_assets (the secret file's and the wall's
        meshes and the notes' materials come with the stage's assets), import_dd_tablet and import_dd_ui.

        Returns:
            How many assets of each kind were made ('sounds', 'textures', 'meshes', 'materials').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_secrets").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_wasami_voices() -> dict[str, int]:
        """Imports (or re-imports) this game's own voices from SourceArt/Wasami/Voices (the WebGL version's clips of
        the user's Wasami, decoded to wav by python Tools/dd/prepare_voices.py): the eleven lines the WebGL version
        speaks, as SoundWaves /Game/Wasami/Voices/Wasami_<Id> through the original's Dialogue sound class. The five it
        puts a subtitle up for (the greeting, the first shard, a boost made, the secret door, and an enemy spotting the
        player) get the manifest's line as their subtitle; the enemies' patrol calls and the death screen's two get
        none, as it speaks them with none. Where each is played is the scene that uses it.

        Returns:
            How many waves were made with a subtitle ('subtitled') and how many without ('silent').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_voices").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_wasami_enemy() -> dict[str, int]:
        """Imports (or re-imports) the enemy Wasami (WasamiEnemy) from this game's model in SourceArt: its skeletal
        mesh, skeleton and physics asset, its textures and material, and its animations named A_WasamiEnemy_<role>
        (Idle, Walk, Run, Stun_KnockDown, Capture_1 …), after writing the prepared glb (animations resampled at 30 fps,
        loops closed, chase variants in place, the stun's falls with their get-ups) under
        Intermediate/Pipeline/wasami/enemy. With them come the original's own sounds the capture plays
        (WasamiCapture): 01_Hotel's Evil_Monkey_Scream, and 03_Manor's LIVING_STATUE_Laughter_05 and Axe_Hit_03.

        Returns:
            How many assets of each kind were made ('textures', 'materials', 'meshes', 'animations', 'sounds').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("gltf")
        return _module("dd_enemy").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_wasami_boss() -> dict[str, float]:
        """Imports (or re-imports) the boss Wasami (WasamiBoss, Zone 2's Matron) from this game's model in SourceArt:
        its skeletal mesh, skeleton and physics asset, its textures and material instance (of the enemy Wasami's
        master), and its animations named A_WasamiBoss_<role> (Idle, Alert, Detected), after writing the prepared glb
        (animations resampled at 30 fps, Idle's loop closed) under Intermediate/Pipeline/wasami/boss.

        Returns:
            How many assets of each kind were made ('textures', 'materials', 'meshes', 'animations'), and the head
            bone's height in cm at Idle's first key, unscaled ('idle_head_cm').
        """
        _module("dd_stage")
        _module("dd_assets")
        _module("gltf")
        _module("dd_enemy")
        return _module("dd_boss").import_all()
