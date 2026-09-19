"""The secrets and collectables (work list item 12): what the secret files (AWasamiCollectable, after the latest
version's Blueprints/Main/BP_Collectable), Zone 2's mysterious room (AWasamiSecretRoomZone, after
Blueprints/Shared/BP_SecretRoomZone), its secret wall (AWasamiSecretWall, after Blueprints/02_School/BP_03_SecretWall1)
and its notes (AWasamiMysteryCollectable, after Blueprints/Main/BP_MysteryCollectable, read on UMG_MysteryNote) show and
play.

  the sounds     Bierce_Secret_Files_Pickup (a file taken), 67-Dark_Whispers_SFX_0704 (the room's whispers, faded in and
                 out as the player comes and goes), DD_LVL2_15_V1_Secret_Mystery_Room_120818 (UMG_Collectables_Secret's
                 sting), Sliding_Wall (the wall going up) and DD_LoreNote_01 (a lore note read; the hospital's are not)
  the pictures   UMG_Collectables' four icons (art, diary, sound, movie: one at random) and the frame of it and of
                 UMG_Collectables_Secret (extras_unlock_bg), and UMG_Collectables_Secret's T_MysteryRoom
  the file       secret_file's two slots given MM_Shared_Secret_Folder (the Blueprint leaves the mesh's own). The mesh,
                 its material and manor_fake_wall, and the notes' materials, come with the stage's assets
                 (prepare_stage's CLASS_MESHES and CLASS_MATERIALS)
  the glitch     the room's Chameleon glitch (M_GlitchHLSL, a post-process material whose graph the cook took away),
                 rebuilt off its compiled shader as M_DD_ChameleonGlitch (see _build_glitch)

Zone 1's secret elevators' sequences come with dd_sequence (SEQUENCE_ACTORS), UMG_MysteryNote's font and arrow with the
tablet and the options screen.

Everything lands under /Game/DD mirroring the original's /Game tree, from pak_reference_2 (UE 4.24; the hospital is only
in the latest version).
"""
import unreal

from wasami_tools.pipeline import dd_assets, dd_stage, paths

EAL = unreal.EditorAssetLibrary
MP = unreal.MaterialProperty
VERSION = 2

SOUNDS = ("Audio/SharedGameplay/Bierce_Secret_Files_Pickup", "Audio/SharedGameplay/67-Dark_Whispers_SFX_0704",
          "Audio/SharedGameplay/DD_LVL2_15_V1_Secret_Mystery_Room_120818", "Audio/02_School/Sliding_Wall",
          "Audio/Misc/DD_LoreNote_01")
TEXTURES = ("UI/Main/Collectables/art_icon", "UI/Main/Collectables/diary_icon", "UI/Main/Collectables/sound_icon",
            "UI/Main/Collectables/movie_icon", "UI/Main/Collectables/extras_unlock_bg", "UI/Main/T_MysteryRoom")
# UMG_MysteryNote's arrow (NextPage) and font, made by the options screen (dd_ui) and the tablet (dd_tablet).
NEEDS = ("UI/Menu/Settings/selection_bar_arrow_hover", "UI/Fonts/helvetica-neue-bold_Font")

SECRET_FILE = "Meshes/Shared/secret_file"
SECRET_FILE_MATERIAL = "Materials/Shared/MM_Shared_Secret_Folder"
STAGE_MADE = (SECRET_FILE, SECRET_FILE_MATERIAL, "Meshes/03_Manor/manor_fake_wall",
              "Materials/06_Hospital/M_06_Hospital_Brick_01", "Materials/06_Hospital/M_06_Hospital_MysteryRoom_Note_01",
              "Materials/06_Hospital/M_06_Hospital_MysteryRoom_Note_02",
              "Materials/06_Hospital/M_06_Hospital_MysteryRoom_Note_03")

GLITCH = "ThirdParty/Chameleon/Materials/M_GlitchHLSL"
GLITCH_MASTER = dd_assets.PIPELINE_MATERIALS + "M_DD_ChameleonGlitch"
# The parameters the estimate keeps, with the defaults the export holds (the Chameleon sets Speed, Density, Amount and
# the three GridDistortion ones from its own values, and BlendingOpacity as the player comes and goes).
GLITCH_SCALARS = ("Density", "Speed", "RandomSeed", "Amount", "Pow1", "Pow2", "Pow3", "Blockeffect",
                  "GridDistortionSpeed", "GridDistortionSize", "GridDistortionPower", "BlendingOpacity")
GLITCH_VECTORS = ("DotValue", "DotValue2")
BLENDING_OPACITY = 1.0   # the export's default (Chameleon's Glitch - Advanced has 1; BP_SecretRoomZone's template 0)

# The shader's offsets and noise (M_GlitchHLSL's SM5 post-process pixel shader, `python Tools/dd/cooked_shaders.py
# "Chameleon/Materials/M_GlitchHLSL." --show 1`, its uniforms at the table's slots): returns the two viewport UVs the
# green and the blue are read at, and gives the UV of the row shift, the UV of the block shift and the block's weight.
GLITCH_OFFSETS = """
float ft = floor(T * Speed);
float2 p = UV * (Density * 0.0001) * ft;
float a = frac(sin(dot(p, Dot1.xy)));
float b = frac(sin(dot(float2(ft * RandomSeed, ft), Dot1.xy)));
float dx = (pow(a, Pow1) * pow(a, Pow2) - pow(b, Pow3) * Amount) * b * 0.05;

float tx = T * Speed / 20.0;
float ty = T * GridDistortionSpeed;
float s = tx * 0.03125;
float fr = s >= 0.0 ? frac(abs(s)) : -frac(abs(s));
float row = floor(UV.y * 32.0) * 0.03125 + 10.0;
float ftx = floor(tx);
float h = frac(sin(dot(float2(row * floor(fr * 256.0) * 0.125, 1.0) * ftx, Dot2.xy)));
h = h * floor(h * fr * 1024.0);
float2 c = floor(h * float2(3.4, 2.15) + UV) * ftx;
float n1 = frac(sin(dot(float2(c.x * 0.045956, ftx), Dot2.xy)));
float n2 = frac(sin(dot(float2(c.y * 0.072674, ftx), Dot2.xy)));
float n = (n1 * 0.5 + n2 * 0.5) * 2.0 - 1.0;
float bs = Blockeffect * 0.1;
float rdx = saturate((abs(n) - (1.0 - bs)) / bs) * sign(n) * Amount;
ShiftUV = saturate(UV + float2(rdx, 0.0));

float3 r[2];
float cells[2] = { GridDistortionSize, round(frac(sin(ty * 6.283185)) * GridDistortionSize * 0.5) };
for (int i = 0; i < 2; i++)
{
    uint3 v = uint3(int3(floor(float3(UV, ty) * cells[i]))) * 1664525u + 1013904223u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    r[i] = float3(v >> 16) * (1.0 / 65536.0);
}
float3 lum = float3(0.3, 0.59, 0.11);
float pick = round(dot(min(r[0], r[1]), lum));
BlockUV = saturate(lerp(r[0].xy, r[1].xy, pick) * GridDistortionPower + UV);
BlockWeight = dot(max(r[0], r[1]), lum);
return float4(UV + float2(dx, 0.0), UV - float2(dx, 0.0));
"""
GLITCH_OFFSET_INPUTS = ("UV", "T", "Density", "Speed", "RandomSeed", "Amount", "Pow1", "Pow2", "Pow3", "Blockeffect",
                        "GridDistortionSpeed", "GridDistortionSize", "GridDistortionPower", "Dot1", "Dot2")
GLITCH_OFFSET_OUTPUTS = (("ShiftUV", "CMOT_FLOAT2"), ("BlockUV", "CMOT_FLOAT2"), ("BlockWeight", "CMOT_FLOAT1"))
# The mix: the red of the scene with the green and the blue read aside, half over the row shift, over the block shift
# by its weight, and that much of the change laid over the scene (the blend mode 0, no mask, no distance, no stencil).
GLITCH_MIX = """
float3 split = float3(Base.r, Green.g, Blue.b);
float3 glitched = lerp(lerp(split, Shift.rgb, 0.5), Block.rgb, Weight);
return max(Base.rgb + (glitched - Base.rgb) * Opacity, 0.0);
"""
GLITCH_MIX_INPUTS = ("Base", "Green", "Blue", "Shift", "Block", "Weight", "Opacity")


def _custom(g, code, inputs, output_type, x, y, outputs=()):
    e = g.node(unreal.MaterialExpressionCustom, x, y)
    e.set_editor_property("code", code.strip())
    e.set_editor_property("output_type", getattr(unreal.CustomMaterialOutputType, output_type))
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    e.set_editor_property("inputs", pins)
    extra = []
    for name, kind in outputs:
        out = unreal.CustomOutput()
        out.set_editor_property("output_name", name)
        out.set_editor_property("output_type", getattr(unreal.CustomMaterialOutputType, kind))
        extra.append(out)
    if extra:
        e.set_editor_property("additional_outputs", extra)
    return e


def _build_glitch(mat, scalars, vectors):
    """The Chameleon pack's M_GlitchHLSL, off its compiled shader (the cook kept its parameters, which the shader's
    uniforms name): five reads of the scene (PostProcessInput0) at the viewport UV and at the UVs GLITCH_OFFSETS works
    out, mixed by GLITCH_MIX. The shader's other branches are left out: the blend modes other than 0, the mask texture
    (the Chameleon's is white), the distance blend (its BlendDistance is 0), the stencil and custom depth masks and the
    selection colour (off). The shader reads the offsets in the scene texture's UVs and the noise in the viewport's;
    the estimate reads both in the viewport's (the same where the view fills the buffer)."""
    g = dd_stage._Graph(mat, checked=True)
    screen = g.node(unreal.MaterialExpressionScreenPosition, -1800, -400)
    time = g.node(unreal.MaterialExpressionTime, -1800, -300)
    offsets = _custom(g, GLITCH_OFFSETS, GLITCH_OFFSET_INPUTS, "CMOT_FLOAT4", -1300, 0, GLITCH_OFFSET_OUTPUTS)
    g.link(screen, "ViewportUV", offsets, "UV")
    g.link(time, "", offsets, "T")
    y = -200
    for name in GLITCH_SCALARS[:-1]:
        g.link(g.scalar(name, scalars[name], -1800, y), "", offsets, name)
        y += 80
    for pin, name in (("Dot1", "DotValue"), ("Dot2", "DotValue2")):
        g.link(g.vector(name, vectors[name], -1800, y), "", offsets, pin)
        y += 150

    def scene(uv, uv_pin, y):
        e = g.node(unreal.MaterialExpressionSceneTexture, -900, y)
        e.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
        g.link(uv, uv_pin, e, "UVs")
        return e

    green_blue = offsets
    reads = {"Base": scene(screen, "ViewportUV", -400)}
    for name, pins, y in (("Green", "RG", -250), ("Blue", "BA", -100)):
        mask = g.node(unreal.MaterialExpressionComponentMask, -1050, y)
        for channel in "RGBA":
            mask.set_editor_property(channel.lower(), channel in pins)
        g.link(green_blue, "return", mask, "")
        reads[name] = scene(mask, "", y)
    reads["Shift"] = scene(offsets, "ShiftUV", 50)
    reads["Block"] = scene(offsets, "BlockUV", 200)
    mix = _custom(g, GLITCH_MIX, GLITCH_MIX_INPUTS, "CMOT_FLOAT3", -500, 0)
    for name, read in reads.items():
        g.link(read, "Color", mix, name)
    g.link(offsets, "BlockWeight", mix, "Weight")
    g.link(g.scalar("BlendingOpacity", scalars["BlendingOpacity"], -700, 350), "", mix, "Opacity")
    g.out(mix, "", MP.MP_EMISSIVE_COLOR)


def make_glitch():
    """M_DD_ChameleonGlitch, the estimate of M_GlitchHLSL with the export's parameter defaults (saved). Returns it."""
    scalars, vectors = dd_assets.parameter_defaults(GLITCH, VERSION)
    scalars.setdefault("BlendingOpacity", BLENDING_OPACITY)
    missing = [n for n in GLITCH_SCALARS if n not in scalars] + [n for n in GLITCH_VECTORS if n not in vectors]
    if missing:
        raise RuntimeError("%s's export has no defaults for %s" % (GLITCH, missing))
    mat = dd_assets.material(GLITCH_MASTER, lambda m: _build_glitch(m, scalars, vectors),
                             domain=unreal.MaterialDomain.MD_POST_PROCESS)
    EAL.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def dress_secret_file():
    """secret_file with MM_Shared_Secret_Folder on both of its slots, as the original's mesh has them (saved)."""
    mesh = unreal.load_asset(dd_assets.asset_path(SECRET_FILE))
    material = unreal.load_asset(dd_assets.asset_path(SECRET_FILE_MATERIAL))
    for index in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(index, material)
    EAL.save_loaded_asset(mesh, only_if_is_dirty=False)
    return mesh


def import_all():
    """Imports and builds everything the secrets use, then saves /Game/DD and /Game/Pipeline. The stage's assets
    (WasamiStageTools.import_dd_stage_assets until nothing remains), the tablet's and the UI's have to have been
    imported. Returns how many of each kind."""
    missing = [rel for rel in STAGE_MADE + NEEDS if not EAL.does_asset_exist(dd_assets.asset_path(rel))]
    if missing:
        raise RuntimeError("missing %s: run python Tools/dd/prepare_stage.py, WasamiStageTools.import_dd_stage_assets "
                           "until nothing remains, WasamiDDTools.import_dd_tablet and import_dd_ui first"
                           % ", ".join(missing))
    result = {"sounds": len([dd_assets.sound(rel, VERSION) for rel in SOUNDS]),
              "textures": len([dd_assets.texture(rel, VERSION) for rel in TEXTURES])}
    dress_secret_file()
    result["meshes"] = 1
    make_glitch()
    result["materials"] = 1
    for folder in (paths.DD_ROOT, paths.PIPELINE_ROOT):
        EAL.save_directory(folder, only_if_is_dirty=True, recursive=True)
    return result
