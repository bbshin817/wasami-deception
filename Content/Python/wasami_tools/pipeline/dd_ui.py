"""Dark Deception's game-flow screens: what the death screen (UWasamiDeathScreenWidget, after the original's
Blueprints/UMG/UMG_DeathScreen) shows and plays — the life icon, YOU ARE DEAD, the menu's font, the life-lost sound and
the game-over music. The vignette, the heading's font and the UI select come with the tablet (dd_tablet).

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24), whose death screen
the widget follows.
"""
import unreal

from wasami_tools.pipeline import dd_assets, paths

EAL = unreal.EditorAssetLibrary
VERSION = 2

TEXTURES = (
    "UI/Main/life_icon_02",
    "UI/Main/you_are_dead",
)
FONTS = (
    "UI/Fonts/helvetica-normal",
)
SOUNDS = (
    "Audio/UI/Life_Lost",
    "Audio/SharedGameplay/66_-_Game_Over",
)


def import_all():
    """Imports the death screen's textures, font and sounds, then saves /Game/DD."""
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in TEXTURES]),
              "fonts": len([dd_assets.font(rel, VERSION) for rel in FONTS]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in SOUNDS])}
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
