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
    """Builds this game's stage, Chaotic Customer 2's Zone_1, from the pipeline data written by
    Tools/cc2/prepare_stage.py: imports its meshes, textures and materials under /Game/CC2 and assembles the level."""

    @toolset_registry.tool_call
    @staticmethod
    def import_cc2_assets(max_items: int = 40) -> dict[str, int]:
        """Imports the stage's next missing meshes, then textures, then material instances, and saves them.

        Args:
            max_items: Most assets to create in this call; call again until 'remaining' is 0.

        Returns:
            'imported' in this call, 'remaining' overall, and '<kind>_done' / '<kind>_total' for meshes, textures and
            materials.
        """
        return _module("cc2_assets").import_batch(max_items)

    @toolset_registry.tool_call
    @staticmethod
    def refresh_cc2_asset_settings() -> dict[str, int]:
        """Brings the already imported stage assets up to the current pipeline: rebuilds the master material in place
        when its graph changed and re-applies each texture's compression, sRGB and LOD group for its use.

        Returns:
            'master_rebuilt' (1 or 0) and 'textures_updated'.
        """
        return _module("cc2_assets").refresh_settings()

    @toolset_registry.tool_call
    @staticmethod
    def build_cc2_level(map_path: str = "/Game/Stage/Maps/L_Zone1") -> dict[str, int]:
        """Builds the stage level from the imported assets (meshes, lights, reflection captures, post process volumes,
        fog, sky light, player start), replacing what an earlier build placed, and saves it. The level is left open.

        Args:
            map_path: Package path of the level; created when missing.

        Returns:
            Placed actors per kind, and 'failed_settings' for volume, fog or sky settings that could not be applied
            (listed in the output log under LogPython).
        """
        return _module("cc2_level").build(map_path)
