"""Where the pipeline's inputs and this project's generated assets live."""
import json
import os

import unreal

PROJECT = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
# Dark Deception's exported data (git-ignored at the project root): pak_reference is the original UE 4.21 export,
# pak_reference_2 the Steam version's UE 4.24 one, which is where the hospital (06_Hospital) lives.
DD_PAK = os.environ.get("PAK_REF", os.path.join(PROJECT, "pak_reference"))
DD_PAK2 = os.environ.get("PAK_REF2", os.path.join(PROJECT, "pak_reference_2"))
DD_ROOT = "/Game/DD"
# This game's own assets: their sources are kept in the repository (SourceArt, Git LFS) and imported under /Game/Wasami.
SOURCE_ART = os.path.join(PROJECT, "SourceArt")
WASAMI_ROOT = "/Game/Wasami"
# The hospital stage, written by Tools/dd/prepare_stage.py.
DD_STAGE = os.path.join(PROJECT, "Intermediate", "Pipeline", "dd", "stage_ue.json")

# Pipeline assets: the Interchange pipeline for the stage's glTF meshes and the master materials its materials use.
MESH_PIPELINE = "/Game/Pipeline/Interchange/PL_DD_StaticMesh"
# ... and the one for this game's skinned models with their animations (dd_enemy).
SKELETAL_PIPELINE = "/Game/Pipeline/Interchange/PL_Wasami_Skeletal"
MASTER_SUBSTANCE = "/Game/Pipeline/Materials/M_DD_Substance"
MASTER_DECAL = "/Game/Pipeline/Materials/M_DD_Decal"
MASTER_UNLIT = "/Game/Pipeline/Materials/M_DD_Unlit"
DEFAULT_PACKED = "/Game/Pipeline/Textures/T_DD_DefaultPacked"
PIPELINE_ROOT = "/Game/Pipeline"


def load_dd_stage():
    if not os.path.exists(DD_STAGE):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_stage.py first." % DD_STAGE)
    with open(DD_STAGE, encoding="utf-8") as f:
        return json.load(f)


def split(asset_path):
    """'/Game/A/B/Name' → ('/Game/A/B', 'Name')."""
    folder, name = asset_path.rsplit("/", 1)
    return folder, name


def object_path(asset_path):
    """'/Game/A/Name' → '/Game/A/Name.Name'."""
    return "%s.%s" % (asset_path, split(asset_path)[1])
