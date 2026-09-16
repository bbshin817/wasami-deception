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
    def import_dd_powers() -> dict[str, int]:
        """Imports (or re-imports) what the tablet's powers show and play: their sounds (with the original SoundWave
        settings and sound concurrency), camera shakes, camera anims (as WasamiCameraAnim), the speed boost's
        textures and materials with the player's FX material, the teleport aim's materials and Cascade particle
        system (rebuilt from the original's exported package), Primal Fear's sphere texture and material, and
        Vanish's puff (textures, material, Cascade particle system) and vignette material. The power icons come with
        import_dd_tablet.

        Returns:
            How many assets of each kind were made ('sounds', 'camera_shakes', 'camera_anims', 'textures',
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
        concurrency and camera shake from the original.

        Returns:
            How many assets of each kind were made ('sounds', 'sound_concurrencies', 'sound_cues', 'camera_shakes',
            'materials', 'mochi').
        """
        _module("dd_stage")
        _module("dd_assets")
        return _module("dd_shards").import_all()
