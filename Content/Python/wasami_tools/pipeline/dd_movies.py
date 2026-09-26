"""The hospital's ambulance screen: Wasami's version of the original's looping video on its monitors.

The original plays /Game/Movies/ambulance_tutorial (a FileMediaSource of ./Movies/ambulance_tutorial2.mp4) on
Ambulance_Tutorial_MediaPlayer (Loop), whose MediaTexture an unlit material (Ambulance_Tutorial_MediaPlayer_Video_Mat:
TextureSample, SAMPLERTYPE_External → Emissive; Color here) shows on Zone 1's 6 and Zone 2's 12 screens; the zone's level Blueprint
opens the source in Setup (BeginPlay). Here the video is this game's own (SourceArt/Wasami/Movies/ambulance_tutorial2.mp4,
Tools/wasami_art/ambulance_video.py), copied to Content/Movies (staged as a loose file: Config/DefaultGame.ini's
DirectoriesToAlwaysStageAsNonUFS), and the same four assets are made under /Game/Wasami/Movies. The stage's material
instance at the original's path (dd_stage.make_material) is parented to M_AmbulanceScreen, so the placed screens keep
their reference; AWasamiZoneFlow::BeginPlay opens the source.
"""
import os
import shutil

import unreal

from wasami_tools.pipeline import paths

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

VIDEO_NAME = "ambulance_tutorial2.mp4"
VIDEO_SRC = os.path.join(paths.SOURCE_ART, "Wasami", "Movies", VIDEO_NAME)
VIDEO_DST = os.path.join(paths.PROJECT, "Content", "Movies", VIDEO_NAME)
FOLDER = "/Game/Wasami/Movies"
SOURCE = FOLDER + "/FMS_AmbulanceTutorial"
PLAYER = FOLDER + "/MP_AmbulanceTutorial"
TEXTURE = FOLDER + "/MT_AmbulanceTutorial"
MATERIAL = FOLDER + "/M_AmbulanceScreen"


def _tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def _asset(path, cls, factory):
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    folder, name = paths.split(path)
    return _tools().create_asset(name, folder, cls, factory)


def copy_video():
    """The video into Content/Movies, when it is missing or differs from SourceArt's."""
    if not os.path.exists(VIDEO_SRC):
        raise FileNotFoundError("%s is missing: python Tools/wasami_art/ambulance_video.py" % VIDEO_SRC)
    if not os.path.exists(VIDEO_DST) or os.path.getsize(VIDEO_DST) != os.path.getsize(VIDEO_SRC) \
            or os.path.getmtime(VIDEO_DST) < os.path.getmtime(VIDEO_SRC):
        os.makedirs(os.path.dirname(VIDEO_DST), exist_ok=True)
        shutil.copy2(VIDEO_SRC, VIDEO_DST)
        return True
    return False


def ensure_ambulance_screen():
    """Makes (or brings up to date) the source, player, texture and material; returns the material."""
    copy_video()
    source = _asset(SOURCE, unreal.FileMediaSource, unreal.FileMediaSourceFactoryNew())
    source.set_editor_property("file_path", "./Movies/" + VIDEO_NAME)   # the original's path, relative to Content
    player = _asset(PLAYER, unreal.MediaPlayer, unreal.MediaPlayerFactoryNew())
    player.set_editor_property("loop", True)
    player.set_editor_property("play_on_open", True)
    texture = _asset(TEXTURE, unreal.MediaTexture, unreal.MediaTextureFactoryNew())
    texture.set_editor_property("media_player", player)
    texture.update_resource()

    mat = _asset(MATERIAL, unreal.Material, unreal.MaterialFactoryNew())
    while MEL.get_num_material_expressions(mat):
        MEL.delete_all_material_expressions(mat)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    sample = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, 0)
    sample.set_editor_property("texture", texture)
    # UE 5.8's MediaTexture is sampled as Color (the original's UE 4.24 one was External: that fails to compile here)
    sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    if not MEL.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("could not connect the media texture of %s" % MATERIAL)
    for usage in ("used_with_nanite", "used_with_static_lighting"):   # as the stage masters (dd_stage.MASTER_USAGE)
        mat.set_editor_property(usage, True)
    MEL.recompile_material(mat)
    for path in (SOURCE, PLAYER, TEXTURE, MATERIAL):
        EAL.save_asset(path, only_if_is_dirty=False)
    return mat
