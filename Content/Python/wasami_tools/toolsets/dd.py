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
