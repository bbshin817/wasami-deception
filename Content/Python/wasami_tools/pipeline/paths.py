"""Where the pipeline's inputs and this project's generated assets live."""
import json
import os

import unreal

PROJECT = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
CC2_STAGE = os.path.join(PROJECT, "Intermediate", "Pipeline", "cc2", "stage_ue.json")
# Dark Deception's exported data (git-ignored at the project root).
DD_PAK = os.environ.get("PAK_REF", os.path.join(PROJECT, "pak_reference"))
DD_ROOT = "/Game/DD"

# Pipeline assets: the Interchange pipeline for the stage's glb meshes and the master material of its materials.
MESH_PIPELINE = "/Game/Pipeline/Interchange/PL_CC2_StaticMesh"
MASTER_MATERIAL = "/Game/Pipeline/Materials/M_CC2_Standard"
DEFAULT_MASKS = "/Game/Pipeline/Textures/T_Default_Masks"
CC2_ROOT = "/Game/CC2"


def load_cc2_stage():
    if not os.path.exists(CC2_STAGE):
        raise FileNotFoundError("%s is missing: run python Tools/cc2/prepare_stage.py first." % CC2_STAGE)
    with open(CC2_STAGE, encoding="utf-8") as f:
        return json.load(f)


def split(asset_path):
    """'/Game/A/B/Name' → ('/Game/A/B', 'Name')."""
    folder, name = asset_path.rsplit("/", 1)
    return folder, name


def object_path(asset_path):
    """'/Game/A/Name' → '/Game/A/Name.Name'."""
    return "%s.%s" % (asset_path, split(asset_path)[1])
