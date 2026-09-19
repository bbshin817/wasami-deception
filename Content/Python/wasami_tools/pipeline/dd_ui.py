"""Dark Deception's game-flow screens: what the death screen (UWasamiDeathScreenWidget, after the original's
Blueprints/UMG/UMG_DeathScreen) shows and plays — the life icon, YOU ARE DEAD, the menu's font, the life-lost sound and
the game-over music — and its YES / NO question (UWasamiPopUpWidget, after UI/Main/TitleScreen/UMG_PopUp): the pause
menu's three window frames and the pop-up sound. The vignette, the heading's font and the UI select come with the
tablet (dd_tablet).

The door breaks' lock (UWasamiSwitchboxWidget, after Blueprints/07_FunPlace/Boss/UMG_07_Boss_Switchbox) and its sounds
(AWasamiDoorBreak, after Blueprints/06_Hospital/BP_06_Hospital_DoorBreak): the filling ring's material and its black
rings', the two sparks', their textures, the lockpicking clicks (a SoundCue) and the two sounds of the lock giving. The
key's font comes with the tablet; the clicks' attenuation comes with the level sequences (dd_sequence) and is made
again here.

The loading screen (UWasamiLoadingWidget, after UI/Main/UMG_Loading) as Zone 1 goes on to Zone 2: the portal sound the
level plays with it, and the hospital's emblem — the original's magic circle with this game's Wasami symbol in place of
the Reaper Nurse's mark (the user's answer, 2026-09-19) — which Tools/dd/prepare_loader.py composes into
Intermediate/Pipeline/wasami/ui/loader_wasami.png and this imports under /Game/Wasami/UI with the original emblems'
settings.

The hand in the middle of the screen (UWasamiInteractWidget, after Blueprints/UMG/UMG_Interact) while the player looks
at something it can use: its icon.

The ring piece's screen (UWasamiRingPieceWidget, after UI/01/UMG_01_RingPieceCollect) as Zone 2's altar gives the piece:
its picture and the sound the level plays with it. CLOSE's font comes with the tablet.

The shard streak's milestones (UWasamiShardStreakWidget, after UI/Menu/Streaks/UMG_ShardStreak, and the game mode's
Check Streak): the ten cards and the four milestone sounds. The vignette and EXTRA LIFE !'s font come with the tablet,
the life icon with the death screen, and the camera shake with the powers (dd_powers).

The level clear screen (UWasamiLevelClearWidget, after UI/Menu/UMG_LevelClear) as the player escapes: You Escaped!,
the rules above and under the rows, You Escaped!'s sound, the rows' and FINAL RANK's grade stamps and the counters'
fill, and in place of the hospital's title (Torment Therapy, which the widget tints red) this game's (the WebGL
version's "Stinky Gachimi", SourceArt/Wasami/UI/level_title_stinky_gachimi.png drawn as the original's title card by
Tools/dd/prepare_level_title.py) at /Game/Wasami/UI/T_LevelTitle. The white vignette and the font come with the tablet.

The title screen (UWasamiTitleScreenWidget, after the old version's UI/Main/TitleScreen/UMG_TitleScreen): the smoky
black over the left (title_screen_video_mask), the brush strokes panning over it (MM_TitleScreen_Mask_Grey over
title_screen_chapters_background, its graph read from its compiled shader), the hover smear behind a menu item
(title_screen_selection_marker), the menu's music (Pause_Sound_v1) and NEW GAME's sound and voice (Start_New_Game,
Bierce_Title_Modified_03). The select and pop-up sounds, the quit frame and the font come with the tablet and the death
screen. In place of the original's logo and monster face, this game's (SourceArt/Wasami/UI/title_logo.png, and the
face and the logo's glow that Tools/dd/prepare_title.py bakes the WebGL version's CSS into) under /Game/Wasami/UI/Title.

The options screen (UWasamiOptionsWidget, after the old version's UI/Menu/UMG_Options; the latest version has none):
its frame, the value boxes, the arrows (normal and hovered), the sliders' thumb and the two check boxes, the same in
both versions. Its font comes with the tablet.

The pause menu (UWasamiPauseWidget, after UI/Menu/Pause/UMG_Pause, the same tree in both versions): the black brush
stroke down the middle (pause_screen_bg), RESTART?'s frame (restart_window_frame; GIVING UP?'s comes with the death
screen) and the sound it opens with (UI_Pause). Its music (Pause_Sound_v1) comes with the title, the select with the
tablet. In place of the original's heads (the level's monster above the menu and peeking over the two windows), this
game's Wasami, white to be tinted red by the widget (SourceArt/Wasami/UI/pause_head.png and pause_peek.png, the WebGL
version's pause-head.webp and pause-peek.webp as they were) under /Game/Wasami/UI/Pause.

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24), whose death screen
the widget follows.
"""
import os

import unreal

from wasami_tools.pipeline import dd_assets, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
VERSION = 2

TEXTURES = (
    "UI/Main/life_icon_02",
    "UI/Main/you_are_dead",
    "UI/Menu/Pause/restart_window_frame_2",
    "UI/Menu/Pause/quit_window_frame",
    "UI/Menu/Pause/blank_window_frame",
)
FONTS = (
    "UI/Fonts/helvetica-normal",
)
SOUNDS = (
    "Audio/UI/Life_Lost",
    "Audio/SharedGameplay/66_-_Game_Over",
    "Audio/UI/UI_Window_PopUp_V3",
)

# ------------------------------------------------------------------------------------------------ the door break
RADIAL_FILL = "Textures/04_Sewer/T_04_RadialFill"
STAR_01 = "ThirdParty/ParticleAlphaTextures/BlackAndWhite/Star_01"
STAR_12 = "ThirdParty/ParticleAlphaTextures/BlackAndWhite/Star_12"
DOOR_BREAK_TEXTURES = (RADIAL_FILL, STAR_01, STAR_12)
DOOR_BREAK_SOUNDS = tuple("Audio/06_Hospital/Lockpicking/SFX_06_Lockpicking_%d" % n for n in range(1, 5)) + (
    "Audio/06_Hospital/Lockpicking/SFX_06_Lockpicked",
    "Audio/07_FunPlace/Press_Slam_02",
)
DOOR_BREAK_CUES = ("Audio/06_Hospital/Lockpicking/SFX_06_Lockpicking",)
DOOR_BREAK_ATTENUATIONS = ("Audio/01_Hotel/01_Lobby_Attenuation",)

# ------------------------------------------------------------------------------------------------ the loading screen
LOADING_SOUNDS = ("Audio/00_Ballroom/21-Ballroom_portal_V2",)
LOADING_EMBLEM_FILE = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "ui", "loader_wasami.png")
LOADING_EMBLEM = paths.WASAMI_ROOT + "/UI/loader_wasami"
# ------------------------------------------------------------------------------------------------ the interact hand
INTERACT_TEXTURES = ("UI/Main/interact_icon_03",)
# ------------------------------------------------------------------------------------------------ the ring piece's screen
RING_PIECE_TEXTURES = ("Textures/Ring_Assets/T_RingPiece_1",)
RING_PIECE_SOUNDS = ("Audio/RingStatue/Ring_Piece_Pickup_v1",)
# ------------------------------------------------------------------------------------------------ the shard streak
STREAK_TEXTURES = tuple("UI/Menu/Streaks/shard_streak_%d" % n for n in (20, 50, 100, 150, 200, 250, 350, 500, 700, 1000))
STREAK_SOUNDS = tuple("Audio/UI/Shard_Streak_Milestone_%s" % v for v in ("V1A", "V2", "V3A", "V4"))
# ------------------------------------------------------------------------------------------------ the level clear screen
LEVEL_CLEAR_TEXTURES = (
    "UI/Menu/you_escaped",
    "UI/Menu/results_window",
)
# The level's title: the WebGL version's "Stinky Gachimi" in place of the hospital's chapter_ui_title_tormenttherapy (the
# user's answer of 2026-09-20), drawn by Tools/dd/prepare_level_title.py; imported as the original's card is (UI).
LEVEL_TITLE_FILE = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "ui", "level_title.png")
LEVEL_TITLE = paths.WASAMI_ROOT + "/UI/T_LevelTitle"
LEVEL_CLEAR_SOUNDS = (
    "Audio/UI/UI_YouEscaped",
    "Audio/UI/Level_Clear_Grade_Stamp_v1",
    "Audio/UI/Level_Clear_Grade_Stamp_v2",
    "Audio/UI/UI_XP_Bar_Fill_V2A_0617",
)

# ------------------------------------------------------------------------------------------------ the options screen
OPTIONS_TEXTURES = tuple("UI/Menu/Settings/" + name for name in (
    "options_window_frame",
    "selection_bar",
    "selection_bar_arrow_normal",
    "selection_bar_arrow_hover",
    "slider_bar_tab",
    "checkbox_icon",
    "checkbox_icon_checked",
))
OPTIONS_SOUNDS = ("Audio/UI/UI_Select_V2",)  # SAVE & EXIT

# ------------------------------------------------------------------------------------------------ the pause menu
PAUSE_TEXTURES = ("UI/Menu/Pause/pause_screen_bg", "UI/Menu/Pause/restart_window_frame")
PAUSE_SOUNDS = ("Audio/UI/UI_Pause",)
PAUSE_WASAMI_ROOT = paths.WASAMI_ROOT + "/UI/Pause"
PAUSE_WASAMI = (
    (os.path.join(paths.PROJECT, "SourceArt", "Wasami", "UI", "pause_head.png"), PAUSE_WASAMI_ROOT + "/T_PauseHead"),
    (os.path.join(paths.PROJECT, "SourceArt", "Wasami", "UI", "pause_peek.png"), PAUSE_WASAMI_ROOT + "/T_PausePeek"),
)

# ------------------------------------------------------------------------------------------------ the title screen
TITLE_MASK = "UI/Main/TitleScreen/title_screen_video_mask"
TITLE_STROKES = "UI/Main/TitleScreen/title_screen_chapters_background"
TITLE_TEXTURES = (TITLE_MASK, TITLE_STROKES, "UI/Main/TitleScreen/title_screen_selection_marker")
TITLE_SOUNDS = (
    "Audio/UI/Pause_Sound_v1",
    "Audio/UI/Start_New_Game",
    "Audio/Titlescreen/Bierce_Title_Modified_03",
)
# MM_TitleScreen_Mask_Grey: a UI material whose graph the cook took away but for its samples (the strokes through a
# Panner, the mask at the UVs), a Time Multiplier and a Desaturation as the emissive colour. Its compiled Slate pixel
# shader (Tools/dd/cooked_shaders.py "TitleScreen/MM_TitleScreen_Mask_Grey." --show 4) gives the rest: the strokes at
# the UVs × (0.35, 1) panned 0.02 a second along U (Time × Time Multiplier), fully desaturated (UE's luminance factors)
# as the colour; the opacity is the mask's alpha × the strokes' alpha.
TITLE_STROKES_MATERIAL = "UI/Main/TitleScreen/MM_TitleScreen_Mask_Grey"
TITLE_STROKES_TILING = (0.35, 1.0)
TITLE_STROKES_SPEED = 0.02
TITLE_UI_DIR = os.path.join(paths.PROJECT, "Intermediate", "Pipeline", "wasami", "ui")
TITLE_WASAMI_ROOT = paths.WASAMI_ROOT + "/UI/Title"
TITLE_WASAMI = (
    (os.path.join(paths.PROJECT, "SourceArt", "Wasami", "UI", "title_logo.png"), TITLE_WASAMI_ROOT + "/T_TitleLogo"),
    (os.path.join(TITLE_UI_DIR, "title_logo_glow.png"), TITLE_WASAMI_ROOT + "/T_TitleLogoGlow"),
    (os.path.join(TITLE_UI_DIR, "title_face.png"), TITLE_WASAMI_ROOT + "/T_TitleFace"),
)

# The original emblems' settings (UI/Main/Loaders/loader_reapernurse in _textures.json: sRGB, default compression, UI).
LOADING_EMBLEM_SETTINGS = {"srgb": True, "compression": None, "lodGroup": "TEXTUREGROUP_UI"}

# M_UI_Radial: a UI material whose graph the cook took away; its compiled Slate pixel shader
# (Tools/dd/cooked_shaders.py "Materials/04_Sewer/M_UI_Radial." --show 4) is what the graph below follows. With v the
# UV's offset from the middle, doubled (1 - 2 UV), the angle frac(atan2(v.x, v.y) / 2π) plus Percentage, floored, picks
# between the texture's alpha times a dark grey and times Color; the opacity is the alpha.
RADIAL = "Materials/04_Sewer/M_UI_Radial"
RADIAL_INSTANCES = ("Materials/04_Sewer/M_04_UI_Radial_Red",)
RADIAL_UNFILLED = (0.078187, 0.074214, 0.076185)
INVERSE_TWO_PI = 0.159155
# M_07_Spark and M_07_Spark2: white, with the texture's red as the opacity (their compiled shaders); the widget's tint
# colours them.
SPARKS = (("Materials/07_FunPlace/M_07_Spark", STAR_01), ("Materials/07_FunPlace/M_07_Spark2", STAR_12))


def _build_radial(mat):
    g = dd_stage._Graph(mat, checked=True)
    scalars, vectors = dd_assets.parameter_defaults(RADIAL, VERSION)
    sample = g.node(unreal.MaterialExpressionTextureSample, -900, 300)
    sample.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(RADIAL_FILL)))

    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1500, -200)
    doubled = g.node(unreal.MaterialExpressionMultiply, -1350, -200)
    doubled.set_editor_property("const_b", -2.0)
    g.link(uv, "", doubled, "A")
    centred = g.node(unreal.MaterialExpressionAdd, -1200, -200)
    centred.set_editor_property("const_b", 1.0)
    g.link(doubled, "", centred, "A")
    angle = g.node(unreal.MaterialExpressionArctangent2, -900, -200)
    g.link(dd_assets.channel(g, centred, "", "R", -1050, -250), "", angle, "Y")
    g.link(dd_assets.channel(g, centred, "", "G", -1050, -150), "", angle, "X")
    turns = g.node(unreal.MaterialExpressionMultiply, -750, -200)
    turns.set_editor_property("const_b", INVERSE_TWO_PI)
    g.link(angle, "", turns, "A")
    radial = dd_assets.single(g, unreal.MaterialExpressionFrac, turns, "", -600, -200)
    percentage = g.scalar("Percentage", scalars.get("Percentage", 0.0), -600, -100)
    filled = dd_assets.add(g, radial, "", percentage, "", -450, -200)
    step = dd_assets.single(g, unreal.MaterialExpressionFloor, filled, "", -300, -200)

    colour = g.vector("Color", vectors.get("Color", [0.0, 0.0, 0.0, 1.0]), -900, 100)
    unfilled = g.multiply(sample, "A", g.const3(RADIAL_UNFILLED, -900, 0), "", -600, 0)
    lit = g.multiply(sample, "A", colour, "RGB", -600, 100)
    g.out(g.lerp(unfilled, "", lit, "", step, "", -150, 0), "", MP.MP_EMISSIVE_COLOR)
    g.out(dd_assets.single(g, unreal.MaterialExpressionSaturate, sample, "A", -300, 300), "", MP.MP_OPACITY)


def _spark_builder(texture_rel):
    def build(mat):
        g = dd_stage._Graph(mat, checked=True)
        sample = g.node(unreal.MaterialExpressionTextureSample, -500, 100)
        sample.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(texture_rel)))
        g.out(g.const3((1.0, 1.0, 1.0), -300, -100), "", MP.MP_EMISSIVE_COLOR)
        g.out(dd_assets.single(g, unreal.MaterialExpressionSaturate, sample, "R", -300, 100), "", MP.MP_OPACITY)
    return build


def _build_title_strokes(mat):
    g = dd_stage._Graph(mat, checked=True)
    scalars, _ = dd_assets.parameter_defaults(TITLE_STROKES_MATERIAL, VERSION)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate, -1300, -100)
    uv.set_editor_property("u_tiling", TITLE_STROKES_TILING[0])
    uv.set_editor_property("v_tiling", TITLE_STROKES_TILING[1])
    time = g.node(unreal.MaterialExpressionTime, -1300, 50)
    multiplier = g.scalar("Time Multiplier", scalars.get("Time Multiplier", 1.0), -1300, 150)
    scaled = g.multiply(time, "", multiplier, "", -1100, 100)
    panner = g.node(unreal.MaterialExpressionPanner, -900, -50)
    panner.set_editor_property("speed_x", TITLE_STROKES_SPEED)
    panner.set_editor_property("speed_y", 0.0)
    g.link(uv, "", panner, "Coordinate")
    g.link(scaled, "", panner, "Time")
    strokes = g.node(unreal.MaterialExpressionTextureSample, -650, -50)
    strokes.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(TITLE_STROKES)))
    g.link(panner, "", strokes, "UVs")
    mask = g.node(unreal.MaterialExpressionTextureSample, -650, 250)
    mask.set_editor_property("texture", unreal.load_asset(dd_assets.asset_path(TITLE_MASK)))
    g.out(dd_assets.single(g, unreal.MaterialExpressionDesaturation, strokes, "RGB", -350, -50), "",
          MP.MP_EMISSIVE_COLOR)
    g.out(g.multiply(mask, "A", strokes, "A", -350, 200), "", MP.MP_OPACITY)


def make_door_break_materials():
    """M_UI_Radial and its instances with their own values, and the two sparks, at the original's paths (saved)."""
    ui = {"domain": unreal.MaterialDomain.MD_UI, "blend_mode": unreal.BlendMode.BLEND_TRANSLUCENT}
    radial = dd_assets.material(dd_assets.asset_path(RADIAL), _build_radial, **ui)
    made = [radial]
    for rel in RADIAL_INSTANCES:
        scalars, vectors, textures, masks, switches = dd_assets.instance_parameters(rel, VERSION)
        made.append(dd_assets.material_instance(dd_assets.asset_path(rel), radial, scalars=scalars, vectors=vectors,
                                                textures=textures, static_masks=masks, static_switches=switches))
    for rel, texture_rel in SPARKS:
        made.append(dd_assets.material(dd_assets.asset_path(rel), _spark_builder(texture_rel), **ui))
    for asset in made:
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return [a.get_path_name() for a in made]


def import_door_break():
    """The door break's textures, sounds, SoundCue, attenuation and materials. Returns how many of each."""
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in DOOR_BREAK_TEXTURES]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in DOOR_BREAK_SOUNDS])}
    result["sound_cues"] = len([dd_assets.sound_cue(rel, VERSION) for rel in DOOR_BREAK_CUES])
    result["attenuations"] = len([dd_assets.sound_attenuation(rel, VERSION) for rel in DOOR_BREAK_ATTENUATIONS])
    result["materials"] = len(make_door_break_materials())
    return result


def import_loading():
    """The loading screen's sound and the hospital's emblem (saved). Returns how many."""
    if not os.path.exists(LOADING_EMBLEM_FILE):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_loader.py first." % LOADING_EMBLEM_FILE)
    emblem = dd_stage.import_texture(dict(LOADING_EMBLEM_SETTINGS, file=LOADING_EMBLEM_FILE, asset=LOADING_EMBLEM))
    EAL.save_loaded_asset(emblem, only_if_is_dirty=False)
    return {"sounds": len([dd_assets.sound(rel, VERSION) for rel in LOADING_SOUNDS]), "emblems": 1}


def import_interact():
    """The hand's icon. Returns how many."""
    return {"textures": len([dd_assets.texture(rel, VERSION) for rel in INTERACT_TEXTURES])}


def import_ring_piece():
    """The ring piece's picture and its sound. Returns how many of each."""
    return {"textures": len([dd_assets.texture(rel, VERSION) for rel in RING_PIECE_TEXTURES]),
            "sounds": len([dd_assets.sound(rel, VERSION) for rel in RING_PIECE_SOUNDS])}


def import_shard_streak():
    """The shard streak's cards and milestone sounds. Returns how many of each."""
    return {"textures": len([dd_assets.texture(rel, VERSION) for rel in STREAK_TEXTURES]),
            "sounds": len([dd_assets.sound(rel, VERSION) for rel in STREAK_SOUNDS])}


def import_level_clear():
    """The level clear screen's pictures and sounds, and this game's level title (saved). The title's image has to have
    been drawn (python Tools/dd/prepare_level_title.py). Returns how many of each."""
    if not os.path.exists(LEVEL_TITLE_FILE):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_level_title.py first." % LEVEL_TITLE_FILE)
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in LEVEL_CLEAR_TEXTURES]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in LEVEL_CLEAR_SOUNDS])}
    title = dd_stage.import_texture(dict(LOADING_EMBLEM_SETTINGS, file=LEVEL_TITLE_FILE, asset=LEVEL_TITLE))
    EAL.save_loaded_asset(title, only_if_is_dirty=False)
    result["wasami_textures"] = 1
    return result


def import_title():
    """The title screen's textures, sounds and strokes material (saved), and this game's logo, its glow and the face
    (saved). Returns how many of each."""
    for path, _ in TITLE_WASAMI:
        if not os.path.exists(path):
            raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_title.py first." % path)
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in TITLE_TEXTURES]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in TITLE_SOUNDS])}
    ui = {"domain": unreal.MaterialDomain.MD_UI, "blend_mode": unreal.BlendMode.BLEND_TRANSLUCENT}
    strokes = dd_assets.material(dd_assets.asset_path(TITLE_STROKES_MATERIAL), _build_title_strokes, **ui)
    EAL.save_loaded_asset(strokes, only_if_is_dirty=False)
    result["materials"] = 1
    for path, asset in TITLE_WASAMI:
        tex = dd_stage.import_texture(dict(LOADING_EMBLEM_SETTINGS, file=path, asset=asset))
        EAL.save_loaded_asset(tex, only_if_is_dirty=False)
    result["wasami_textures"] = len(TITLE_WASAMI)
    return result


def import_options():
    """The options screen's textures and sound. Returns how many."""
    return {"textures": len([dd_assets.texture(rel, VERSION) for rel in OPTIONS_TEXTURES]),
            "sounds": len([dd_assets.sound(rel, VERSION) for rel in OPTIONS_SOUNDS])}


def import_pause():
    """The pause menu's textures and sound, and this game's heads (saved). Returns how many of each."""
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in PAUSE_TEXTURES]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in PAUSE_SOUNDS])}
    for path, asset in PAUSE_WASAMI:
        tex = dd_stage.import_texture(dict(LOADING_EMBLEM_SETTINGS, file=path, asset=asset))
        EAL.save_loaded_asset(tex, only_if_is_dirty=False)
    result["wasami_textures"] = len(PAUSE_WASAMI)
    return result


def import_all():
    """Imports the death screen's and the pop-up's textures, font and sounds, the door break's assets, the loading
    screen's, the hand's, the ring piece screen's, the shard streak's, the level clear screen's, the title screen's, the
    options screen's and the pause menu's, then saves /Game/DD."""
    result = {"textures": len([dd_assets.texture(rel, VERSION) for rel in TEXTURES]),
              "fonts": len([dd_assets.font(rel, VERSION) for rel in FONTS]),
              "sounds": len([dd_assets.sound(rel, VERSION) for rel in SOUNDS])}
    for key, count in import_door_break().items():
        result["door_break_" + key] = count
    for key, count in import_loading().items():
        result["loading_" + key] = count
    for key, count in import_interact().items():
        result["interact_" + key] = count
    for key, count in import_ring_piece().items():
        result["ring_piece_" + key] = count
    for key, count in import_shard_streak().items():
        result["streak_" + key] = count
    for key, count in import_level_clear().items():
        result["level_clear_" + key] = count
    for key, count in import_title().items():
        result["title_" + key] = count
    for key, count in import_options().items():
        result["options_" + key] = count
    for key, count in import_pause().items():
        result["pause_" + key] = count
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
