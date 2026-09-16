"""Where the pipeline's inputs and this project's generated assets live."""
import os

import unreal

PROJECT = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
# Dark Deception's exported data (git-ignored at the project root): pak_reference is the original UE 4.21 export,
# pak_reference_2 the Steam version's UE 4.24 one, which is where the hospital (06_Hospital) lives.
DD_PAK = os.environ.get("PAK_REF", os.path.join(PROJECT, "pak_reference"))
DD_PAK2 = os.environ.get("PAK_REF2", os.path.join(PROJECT, "pak_reference_2"))
DD_ROOT = "/Game/DD"


def split(asset_path):
    """'/Game/A/B/Name' → ('/Game/A/B', 'Name')."""
    folder, name = asset_path.rsplit("/", 1)
    return folder, name


def object_path(asset_path):
    """'/Game/A/Name' → '/Game/A/Name.Name'."""
    return "%s.%s" % (asset_path, split(asset_path)[1])
