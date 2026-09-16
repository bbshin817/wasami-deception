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
        _module("dd_assets")
        _module("dd_stage")
        return _module("dd_tablet").import_all()

    @toolset_registry.tool_call
    @staticmethod
    def import_dd_powers() -> dict[str, int]:
        """Imports (or re-imports) what the tablet's powers play: their sounds (with the original SoundWave settings
        and sound concurrency) and camera shakes. The power icons come with import_dd_tablet.

        Returns:
            How many assets of each kind were made ('sounds', 'camera_shakes').
        """
        _module("dd_assets")
        return _module("dd_powers").import_all()
