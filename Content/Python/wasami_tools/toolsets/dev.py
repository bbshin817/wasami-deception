import os

import unreal

import toolset_registry


@unreal.uclass()
class WasamiDevTools(unreal.ToolsetDefinition):
    """Editor utilities for developing this project: running console commands in the editor world and
    rendering the level from a given pose to a PNG."""

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

    @toolset_registry.tool_call
    @staticmethod
    def capture_pose(out_path: str, x: float, y: float, z: float, yaw: float, pitch: float = 0.0,
                     fov: float = 90.0, width: int = 1280, height: int = 720) -> str:
        """Renders the current level from one pose to a PNG, with the game's post-processing applied.

        Unlike 'HighResShot' and AutomationLibrary.take_high_res_screenshot, this does not go through the
        editor's viewport, so it also works while the editor window is not the focused window (those two
        drop the request silently and never write a file). It renders through a temporary SceneCapture2D
        with SCS_FINAL_COLOR_LDR, so exposure, bloom and the tonemapper are the ones the game will use.

        Args:
            out_path: Where to write the PNG, e.g. 'C:/tmp/shot.png'. The folder must already exist.
            x, y, z: The camera position in world space (cm). The player's eye is 95 cm above the
                capsule's centre, so a PlayerStart at z 92 corresponds to z 187 here.
            yaw: The camera's yaw in degrees (0 looks down +X).
            pitch: The camera's pitch in degrees (0 is horizontal).
            fov: Horizontal field of view in degrees. The player's resting FOV is 90.
            width, height: The rendered size in pixels.

        Returns:
            The path that was written.
        """
        folder, name = os.path.split(out_path.replace("\\", "/"))
        if not folder or not name.lower().endswith(".png"):
            raise ValueError("out_path must be an absolute path to a .png file.")
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        render_target = unreal.RenderingLibrary.create_render_target2d(
            world, width, height, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
        actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        capture = actors.spawn_actor_from_class(
            unreal.SceneCapture2D, unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
        try:
            component = capture.capture_component2d
            component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
            component.set_editor_property("texture_target", render_target)
            component.set_editor_property("fov_angle", fov)
            component.set_editor_property("capture_every_frame", False)
            component.set_editor_property("capture_on_movement", False)
            component.set_editor_property("always_persist_rendering_state", True)
            component.capture_scene()
            component.capture_scene()  # the second pass lets eye adaptation settle on the new view
            unreal.RenderingLibrary.export_render_target(world, render_target, folder, name)
        finally:
            actors.destroy_actor(capture)
        return out_path
