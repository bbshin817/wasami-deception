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


def import_all():
    """Imports the death screen's and the pop-up's textures, font and sounds, the door break's assets, the loading
    screen's, the hand's, the ring piece screen's and the shard streak's, then saves /Game/DD."""
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
    EAL.save_directory(paths.DD_ROOT, only_if_is_dirty=True, recursive=True)
    return result
