import unreal

import toolset_registry


@unreal.uclass()
class WasamiDevTools(unreal.ToolsetDefinition):
    """Editor utilities for developing this project: running console commands in the editor world."""

    @toolset_registry.tool_call
    @staticmethod
    def execute_console_command(command: str) -> None:
        """Runs a console command in the editor world, as if typed into the editor's console.

        Args:
            command: The command line, e.g. 'stat fps' or 'r.Lumen.Reflections.Allow 0'.
        """
        if not command.strip():
            raise ValueError("command must not be empty.")
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        unreal.SystemLibrary.execute_console_command(world, command)
