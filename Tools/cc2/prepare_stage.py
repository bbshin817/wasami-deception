"""Chaotic Customer 2 Zone_1 → UE-native stage data for this project's editor pipeline.

    python Tools/cc2/prepare_stage.py

Reads the WebGL version's derived stage (its assets-src/cc2/layout.json and assets-src/level/stage.json, written by its
scripts/cc2/layout.py from the fan game's CUE4Parse export: which meshes are drawn, their roles, the train moved to
the platform, the cars' start, the resolved materials, lights, volumes, the gameplay markers) and writes, under
Intermediate/Pipeline/cc2/:
  stage_ue.json  meshes, textures, materials, placements (UE transforms), lights, captures, post process volumes, fog,
                 sky and the gameplay markers, all in UE units (cm, left-handed, z up) and /Game asset paths
  meshes/        each placed glb with one material per section (S0, S1, …) and no images, so the editor import makes
                 one material slot per section (the export's glb gives sections that share a default material one slot)

The export's glb meshes import into UE in their original mesh space (checked on ATM and Blocker_Cube_001: bounds equal
the cooked StaticMesh's). The layout's placement matrices are Blender's (x, −y, z − floorZ) × 0.01 frame over that
mesh space: Tz · K · M_ue · K⁻¹ with K = diag(0.01, −0.01, 0.01), so M_ue = K⁻¹ · Tz⁻¹ · M · K.

Env: CC2_REF — the fan game's export (default <repo>/cc2_reference); WASAMI_WEBGL — the WebGL project (default
C:/Users/User/Downloads/wasami-deseption), whose derived stage (layout.json, stage.json, assets-src/cc2/tex) this reads.
"""
import datetime
import hashlib
import json
import os
import re
import struct
import sys

import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
WEBGL = os.environ.get("WASAMI_WEBGL", r"C:\Users\User\Downloads\wasami-deseption")
REF = os.environ.get("CC2_REF", os.path.join(ROOT, "cc2_reference"))
LAYOUT = os.path.join(WEBGL, "assets-src", "cc2", "layout.json")
STAGE = os.path.join(WEBGL, "assets-src", "level", "stage.json")
GEN_TEX = os.path.join(WEBGL, "assets-src", "cc2", "tex")
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "cc2")
OUT_MESHES = os.path.join(OUT, "meshes")

CONTENT_PREFIX = "Chaotic_Customer_2/Content/"
GAME_ROOT = "/Game/CC2"

# Pictures of the fan game's characters are not used: their figures were replaced with Wasami pictures by the WebGL
# version's scripts/prepare-cc2-textures.mjs (REPLACE, the user's choice of 2026-09-14), written to assets-src/cc2/tex/
# replace/<path under Content_game/models/Zone_1/ with / → __>.png.
ZONE1 = "Chaotic_Customer_2/Content/Content_game/models/Zone_1/"
REPLACE = (
    "Modules_Enhanced_zone/Graffiti__zone_1/MonkeAd2-4",
    "Modules_Enhanced_zone/Graffiti__zone_1/Portal_Decal_D",
    "Modules_Enhanced_zone/Graffiti__zone_1/Portal_Decal_H",
    "Modules_Enhanced_zone/Graffiti__zone_1/Portal_Decal_P",
    "Poster/1794_20220805025437",
    "Poster/1917_20220829022219",
    "Poster/ConeAd",
    "Poster/JJG_build_board",
    "Poster/OriginalPopuskIManda",
    "Poster/circus_build_board",
)
# Only the first two UV sets are kept (UV0: the materials; UV1: the lightmap set of most meshes). The export writes eight.
KEEP_ATTRIBUTES = {"POSITION", "NORMAL", "TANGENT", "COLOR_0", "TEXCOORD_0", "TEXCOORD_1"}


# ------------------------------------------------------------------------------------------------ names and paths
def safe_segment(s):
    """A package path segment UE accepts: [A-Za-z0-9_-], anything else '_', with a short hash when changed."""
    t = re.sub(r"[^A-Za-z0-9_-]", "_", s)
    if t != s:
        t = "%s_%s" % (t.strip("_") or "x", hashlib.md5(s.encode("utf-8")).hexdigest()[:6])
    return t


def game_path(ref_rel):
    """'Chaotic_Customer_2/Content/Content_game/models/ATM.glb' → '/Game/CC2/Content_game/models/ATM' (the fan game's
    own /Game tree, mirrored under /Game/CC2, so names stay unique per folder as they were there)."""
    rel = ref_rel.replace("\\", "/")
    if rel.startswith(CONTENT_PREFIX):
        rel = rel[len(CONTENT_PREFIX):]
    rel = re.sub(r"\.[A-Za-z0-9]+$", "", rel)
    return GAME_ROOT + "/" + "/".join(safe_segment(p) for p in rel.split("/"))


def generated_path(file_name):
    return GAME_ROOT + "/Generated/" + safe_segment(os.path.splitext(file_name)[0])


def key_file(key):
    """The WebGL texture script's file stem for a material key (its `key.replace(/[^A-Za-z0-9_-]+/g, '__')`)."""
    return re.sub(r"[^A-Za-z0-9_-]+", "__", key)


# ------------------------------------------------------------------------------------------------ frames
FLOOR_Z = None  # set from the layout
K = np.diag([0.01, -0.01, 0.01, 1.0])
K_INV = np.diag([100.0, -100.0, 100.0, 1.0])


def blender_point(p):
    return [round(p[0] * 100.0, 3), round(-p[1] * 100.0, 3), round(p[2] * 100.0 + FLOOR_Z, 3)]


def blender_dir(d):
    v = np.array([d[0], -d[1], d[2]], float)
    n = np.linalg.norm(v)
    return v / n if n > 0 else v


def blender_box(b):
    """A Blender AABB {min, max} → UE {center, extent} (cm)."""
    a, c = np.array(blender_point(b["min"])), np.array(blender_point(b["max"]))
    lo, hi = np.minimum(a, c), np.maximum(a, c)
    return {"center": [round(float(v), 3) for v in (lo + hi) / 2], "extent": [round(float(v), 3) for v in (hi - lo) / 2]}


def matrix_quat(r):
    """Rotation matrix (column vectors) → quaternion (x, y, z, w): UE's FQuat for UE's rotation (ue.py quat_matrix)."""
    t = np.trace(r)
    if t > 0:
        s = 0.5 / np.sqrt(t + 1.0)
        w, x, y, z = 0.25 / s, (r[2, 1] - r[1, 2]) * s, (r[0, 2] - r[2, 0]) * s, (r[1, 0] - r[0, 1]) * s
    elif r[0, 0] > r[1, 1] and r[0, 0] > r[2, 2]:
        s = 2.0 * np.sqrt(1.0 + r[0, 0] - r[1, 1] - r[2, 2])
        w, x, y, z = (r[2, 1] - r[1, 2]) / s, 0.25 * s, (r[0, 1] + r[1, 0]) / s, (r[0, 2] + r[2, 0]) / s
    elif r[1, 1] > r[2, 2]:
        s = 2.0 * np.sqrt(1.0 + r[1, 1] - r[0, 0] - r[2, 2])
        w, x, y, z = (r[0, 2] - r[2, 0]) / s, (r[0, 1] + r[1, 0]) / s, 0.25 * s, (r[1, 2] + r[2, 1]) / s
    else:
        s = 2.0 * np.sqrt(1.0 + r[2, 2] - r[0, 0] - r[1, 1])
        w, x, y, z = (r[1, 0] - r[0, 1]) / s, (r[0, 2] + r[2, 0]) / s, (r[1, 2] + r[2, 1]) / s, 0.25 * s
    q = np.array([x, y, z, w])
    return q / np.linalg.norm(q)


def quat_matrix(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ])


def decompose(m):
    """UE matrix (column vectors, cm) → (location, quat, scale, fit error in cm per metre of mesh). A mirror goes to −x
    scale; shear, which a UE transform cannot hold, shows as the fit error."""
    a = m[:3, :3]
    s = np.linalg.norm(a, axis=0)
    if np.linalg.det(a) < 0:
        s[0] = -s[0]
    r = a / s
    u, _, vt = np.linalg.svd(r)
    r = u @ vt
    q = matrix_quat(r)
    err = float(np.abs(quat_matrix(q) @ np.diag(s) - a).max()) * 100.0
    loc = m[:3, 3]
    return [round(float(v), 4) for v in loc], [round(float(v), 8) for v in q], [round(float(v), 6) for v in s], round(err, 4)


def basis_quat(x_axis, up=None):
    """Quaternion of the rotation whose X is x_axis and whose Z is as near `up` as it can be (UE MakeFromXZ)."""
    x = np.array(x_axis, float)
    x /= np.linalg.norm(x)
    z = np.array(up if up is not None else [0.0, 0.0, 1.0], float)
    if abs(float(np.dot(x, z / np.linalg.norm(z)))) > 0.999:
        z = np.array([1.0, 0.0, 0.0]) if abs(x[2]) > 0.9 else np.array([0.0, 0.0, 1.0])
    y = np.cross(z, x)
    y /= np.linalg.norm(y)
    z = np.cross(x, y)
    return [round(float(v), 8) for v in matrix_quat(np.column_stack([x, y, z]))]


# ------------------------------------------------------------------------------------------------ glb
def read_glb(path):
    with open(path, "rb") as f:
        data = f.read()
    magic, _version, _length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF":
        raise ValueError("not a glb: " + path)
    jlen, jtype = struct.unpack_from("<I4s", data, 12)
    if jtype != b"JSON":
        raise ValueError("glb without a JSON chunk: " + path)
    gltf = json.loads(data[20:20 + jlen])
    rest = data[20 + jlen:]
    return gltf, rest


def write_glb(path, gltf, rest):
    js = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    js += b" " * ((4 - len(js) % 4) % 4)
    total = 12 + 8 + len(js) + len(rest)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(struct.pack("<4sII", b"glTF", 2, total))
        f.write(struct.pack("<I4s", len(js), b"JSON"))
        f.write(js)
        f.write(rest)


def sectioned_glb(src, dst):
    """Writes src with one material S<i> per primitive (in order) and no images; returns the primitive count."""
    gltf, rest = read_glb(src)
    n = 0
    for mesh in gltf.get("meshes", []):
        for prim in mesh.get("primitives", []):
            prim["material"] = n
            prim["attributes"] = {k: v for k, v in prim["attributes"].items() if k in KEEP_ATTRIBUTES}
            n += 1
    gltf["materials"] = [{"name": "S%d" % i} for i in range(n)]
    for k in ("images", "textures", "samplers"):
        gltf.pop(k, None)
    for k in ("extensionsUsed", "extensionsRequired"):
        if k in gltf:
            gltf[k] = [e for e in gltf[k] if not e.startswith(("KHR_materials", "KHR_texture"))]
            if not gltf[k]:
                del gltf[k]
    write_glb(dst, gltf, rest)
    return n


# ------------------------------------------------------------------------------------------------ main
def main():
    global FLOOR_Z
    for p in (LAYOUT, STAGE, REF):
        if not os.path.exists(p):
            print("missing: " + p + " (set WASAMI_WEBGL to the WebGL project)", file=sys.stderr)
            return 1
    with open(LAYOUT, encoding="utf-8") as f:
        layout = json.load(f)
    with open(STAGE, encoding="utf-8") as f:
        stage = json.load(f)
    FLOOR_Z = float(layout["floorZ"])
    k_tz_inv = np.eye(4)
    k_tz_inv[2, 3] = FLOOR_Z * 0.01  # Tz⁻¹: back down by the floor lift (Tz adds −floorZ · 0.01)

    # ---------------------------------------------------------------- meshes
    meshes = {}
    problems = []
    for rel in sorted({p["glb"] for p in layout["placements"]}):
        dst = os.path.join(OUT_MESHES, *rel.split("/"))
        n = sectioned_glb(os.path.join(REF, *rel.split("/")), dst)
        shape = (layout.get("collisionShapes") or {}).get(rel, {}).get("kind", "none")
        meshes[rel] = {"glb": dst, "asset": game_path(rel), "sections": n, "collision": shape}

    # ---------------------------------------------------------------- textures and materials
    textures = {}

    def tex(path, kind):
        path = os.path.normpath(path)
        if not os.path.exists(path):
            problems.append("texture missing: " + path)
            return None
        if path.startswith(os.path.normpath(REF)):
            asset = game_path(os.path.relpath(path, REF))
        else:
            asset = generated_path(os.path.basename(path))
        prev = textures.get(path)
        if prev and prev["kind"] != kind:
            problems.append("texture used as %s and %s: %s" % (prev["kind"], kind, path))
        textures.setdefault(path, {"asset": asset, "kind": kind})
        return path

    def ref_png(rel):
        return os.path.join(REF, *rel.split("/"))

    materials = {}
    for key, m in layout["materials"].items():
        t = m.get("textures") or {}
        e = {
            "asset": game_path(m["asset"]),
            "source": m["asset"],
            "blend": m.get("blend", "OPAQUE"),
            "clip": m.get("clip", 0.3333),
            "twoSided": bool(m.get("twoSided")),
            "alpha": bool(m.get("alpha")),
            "decal": bool(m.get("decal")),
            "constColor": m.get("constColor"),
            "roughness": m.get("roughness"),
            "metallic": m.get("metallic"),
            "emissiveColor": m.get("emissiveColor"),
            "opacity": m.get("opacity"),
            "scalars": m.get("scalars") or {},
            "textures": {},
            "replaced": False,
        }
        if e["decal"]:
            e["textures"]["diffuse"] = tex(os.path.join(GEN_TEX, key_file(key) + "_decal.png"), "albedo")
        else:
            if t.get("diffuse"):
                png = ref_png(t["diffuse"])
                if t["diffuse"].startswith(ZONE1) and t["diffuse"][len(ZONE1):-4] in REPLACE:
                    png = os.path.join(GEN_TEX, "replace", t["diffuse"][len(ZONE1):-4].replace("/", "__") + ".png")
                    e["replaced"] = True
                e["textures"]["diffuse"] = tex(png, "albedo")
            if t.get("normal"):
                e["textures"]["normal"] = tex(ref_png(t["normal"]), "normal")
            if t.get("orm"):
                e["textures"]["orm"] = tex(ref_png(t["orm"]), "orm")
            elif t.get("occlusion") or t.get("roughness") or t.get("metallic"):
                e["textures"]["orm"] = tex(os.path.join(GEN_TEX, key_file(key) + "_orm.png"), "orm")
            if t.get("emissive"):
                e["textures"]["emissive"] = tex(ref_png(t["emissive"]), "emissive")
        e["textures"] = {k: v for k, v in e["textures"].items() if v}
        materials[key] = e

    # ---------------------------------------------------------------- placements
    placements = []
    worst = 0.0
    for p in layout["placements"]:
        mb = np.array(p["matrix"], float).reshape(4, 4)
        m_ue = K_INV @ k_tz_inv @ mb @ K
        loc, quat, scale, err = decompose(m_ue)
        worst = max(worst, err)
        mesh = meshes[p["glb"]]
        sections = p.get("sections") or list(range(mesh["sections"]))
        if len(sections) != mesh["sections"]:
            problems.append("sections %d ≠ glb primitives %d: %s" % (len(sections), mesh["sections"], p["glb"]))
        mats = p["materials"]
        per_section = [mats[s] if s < len(mats) else None for s in sections[:mesh["sections"]]]
        placements.append({
            "id": p["id"], "actor": p["actor"], "comp": p["comp"], "class": p["class"], "role": p["role"],
            "zone": p["zone"], "mesh": p["glb"], "materials": per_section, "collision": bool(p["collision"]),
            "hidden": bool(p["hidden"]), "location": loc, "rotation": quat, "scale": scale, "fitError": err,
        })

    # ---------------------------------------------------------------- lights
    lights = []
    for lt in layout["lights"]:
        e = {
            "id": lt["id"], "name": lt["name"], "actor": lt["actor"], "class": lt["class"], "kind": lt["kind"],
            "owner": lt.get("owner"), "zone": lt["zone"], "location": blender_point(lt["position"]),
            "rotation": basis_quat(blender_dir(lt["direction"]), blender_dir(lt["up"]) if lt.get("up") else None),
            "color": lt["color"], "intensity": lt["intensity"], "units": lt["units"],
            "attenuationRadius": round(lt["radius"] * 100.0, 3), "castShadows": bool(lt.get("castShadows", True)),
            "volumetric": lt.get("volumetric", 1.0), "sourceRadius": round(lt.get("sourceRadius", 0.0) * 100.0, 3),
        }
        if lt.get("size"):
            e["sourceWidth"], e["sourceHeight"] = round(lt["size"][0] * 100.0, 3), round(lt["size"][1] * 100.0, 3)
        if lt.get("barnDoor"):
            e["barnDoorAngle"], e["barnDoorLength"] = lt["barnDoor"][0], round(lt["barnDoor"][1] * 100.0, 3)
        if lt.get("cone"):
            e["innerCone"], e["outerCone"] = lt["cone"]
        if lt.get("sourceLength") is not None:
            e["sourceLength"] = round(lt["sourceLength"] * 100.0, 3)
        lights.append(e)

    captures = [{"name": c["name"], "source": c["source"], "zone": c["zone"], "kind": c["kind"],
                 "location": blender_point(c["position"]), "extent": [round(v * 100.0, 3) for v in c["extent"]]}
                for c in layout["captures"]]

    # the post process volumes' grading LUTs are textures of the export
    volumes = []
    for v in layout["volumes"]:
        v = dict(v)
        lut = (v.get("settings") or {}).get("ColorGradingLUT")
        if isinstance(lut, dict) and lut.get("ObjectPath"):
            png = ref_png(re.sub(r"\.\d+$", "", lut["ObjectPath"]) + ".png")
            v["lut"] = tex(png, "lut")
        volumes.append(v)
    sky = dict(layout["sky"])
    cube = (sky.get("props") or {}).get("Cubemap") or {}
    if cube.get("ObjectPath"):
        hdr = ref_png(re.sub(r"\.\d+$", "", cube["ObjectPath"]) + ".hdr")
        sky["cubemap"] = tex(hdr, "cubemap")

    # ---------------------------------------------------------------- gameplay markers (UE cm)
    def start(s):
        pos, look = np.array(blender_point(s["position"])), np.array(blender_point(s["look"]))
        d = look - pos
        return {"zone": s.get("zone"), "location": [float(v) for v in pos], "yaw": round(float(np.degrees(np.arctan2(d[1], d[0]))), 3)}

    gameplay = {
        "floors": {k: round(v * 100.0 + FLOOR_Z, 3) for k, v in stage["floors"].items()},
        "start": start(stage["start"]),
        "checkpoints": {k: start(v) for k, v in stage["checkpoints"].items()},
        "shards": [{"id": s["id"], "source": s["source"], "location": blender_point(s["position"])} for s in stage["shards"]],
        "enemySpawns": [{"source": s["source"], "location": blender_point(s["position"]), "yaw": -s.get("yaw", 0.0)} for s in stage["enemySpawns"]],
        "specials": {k: {**{kk: vv for kk, vv in v.items() if kk != "points"}, "points": [blender_point(p) for p in v["points"]]} for k, v in stage["specials"].items()},
        "triggers": {k: blender_box(v) for k, v in stage["triggers"].items()},
        "panel": {"source": stage["panel"]["source"], "location": blender_point(stage["panel"]["position"])},
    }

    out = {
        "generated": datetime.datetime.now().isoformat(timespec="seconds"),
        "source": {"webgl": WEBGL, "layout": LAYOUT, "stage": STAGE, "ref": REF, "floorZ": FLOOR_Z},
        "meshes": meshes, "textures": textures, "materials": materials, "placements": placements,
        "lights": lights, "captures": captures, "volumes": volumes, "fog": layout["fog"], "sky": sky,
        "gameplay": gameplay, "problems": problems,
    }
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, "stage_ue.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    print("meshes %d, textures %d, materials %d, placements %d (worst fit %.4f cm/m), lights %d, problems %d"
          % (len(meshes), len(textures), len(materials), len(placements), worst, len(lights), len(problems)))
    for p in problems[:20]:
        print("  ! " + p)
    return 0


if __name__ == "__main__":
    sys.exit(main())
