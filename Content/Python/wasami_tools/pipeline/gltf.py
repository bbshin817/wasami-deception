"""Binary glTF (.glb) with the standard library alone (the editor's Python has no numpy): reading and writing the file,
reading and appending accessors, and sampling animation channels.

A file is (gltf, blob): the JSON as a dict and its one binary buffer as a bytearray. Appended data goes at the end of
the blob, 4-byte aligned, each accessor in a bufferView of its own.
"""
import json
import math
import struct

COMPONENT_FORMATS = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}
TYPE_WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT2": 4, "MAT3": 9, "MAT4": 16}
FLOAT = 5126
GLB_MAGIC = b"glTF"
JSON_CHUNK = 0x4E4F534A
BIN_CHUNK = 0x004E4942


def read(path):
    """A .glb's JSON and binary buffer."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != GLB_MAGIC:
        raise ValueError("%s is not a binary glTF" % path)
    json_length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + json_length])
    offset = 20 + json_length
    if offset >= len(data):
        return gltf, bytearray()
    bin_length = struct.unpack_from("<I", data, offset)[0]
    return gltf, bytearray(data[offset + 8:offset + 8 + bin_length])


def write(path, gltf, blob):
    """Writes a .glb of one buffer (the blob)."""
    blob = bytes(blob) + b"\0" * (-len(blob) % 4)
    gltf["buffers"] = [{"byteLength": len(blob)}]
    text = json.dumps(gltf, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    text += b" " * (-len(text) % 4)
    total = 12 + 8 + len(text) + 8 + len(blob)
    with open(path, "wb") as f:
        f.write(struct.pack("<4sII", GLB_MAGIC, 2, total))
        f.write(struct.pack("<II", len(text), JSON_CHUNK) + text)
        f.write(struct.pack("<II", len(blob), BIN_CHUNK) + blob)


def accessor(gltf, blob, index):
    """An accessor's elements as tuples (normalization is not applied; sparse accessors are not read)."""
    acc = gltf["accessors"][index]
    if "sparse" in acc or "bufferView" not in acc:
        raise ValueError("accessor %d is sparse or has no buffer view" % index)
    view = gltf["bufferViews"][acc["bufferView"]]
    fmt = COMPONENT_FORMATS[acc["componentType"]]
    width = TYPE_WIDTHS[acc["type"]]
    size = struct.calcsize("<" + fmt * width)
    stride = view.get("byteStride", size)
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    return [struct.unpack_from("<" + fmt * width, blob, start + i * stride) for i in range(acc["count"])]


def add_accessor(gltf, blob, rows, kind, bounds=False):
    """Appends float elements (tuples of TYPE_WIDTHS[kind] numbers) as an accessor. Returns its index. Animation inputs
    need their bounds."""
    width = TYPE_WIDTHS[kind]
    blob.extend(b"\0" * (-len(blob) % 4))
    offset = len(blob)
    for row in rows:
        if len(row) != width:
            raise ValueError("a %s element has %d numbers" % (kind, len(row)))
        blob.extend(struct.pack("<" + "f" * width, *row))
    gltf.setdefault("bufferViews", []).append({"buffer": 0, "byteOffset": offset, "byteLength": len(blob) - offset})
    acc = {"bufferView": len(gltf["bufferViews"]) - 1, "componentType": FLOAT, "count": len(rows), "type": kind}
    if bounds:
        acc["min"] = [min(r[i] for r in rows) for i in range(width)]
        acc["max"] = [max(r[i] for r in rows) for i in range(width)]
    gltf.setdefault("accessors", []).append(acc)
    return len(gltf["accessors"]) - 1


def copy_accessor(src_gltf, src_blob, index, gltf, blob):
    """Copies a float accessor of another file into this one. Returns its index here."""
    acc = src_gltf["accessors"][index]
    if acc["componentType"] != FLOAT:
        raise ValueError("accessor %d is not of floats" % index)
    return add_accessor(gltf, blob, accessor(src_gltf, src_blob, index), acc["type"], bounds="min" in acc)


# ------------------------------------------------------------------------------------------------ animation
def node_names(gltf):
    return [n.get("name", "") for n in gltf["nodes"]]


def parents(gltf):
    """{node name: its parent's name} of the nodes that have a parent."""
    names = node_names(gltf)
    return {names[c]: names[i] for i, n in enumerate(gltf["nodes"]) for c in n.get("children", ())}


def channels(gltf, blob, animation):
    """{(node name, 'rotation' | 'translation' | 'scale'): (times, values)} of a glTF animation. Only LINEAR samplers
    are read (what Blender writes for baked actions)."""
    names = node_names(gltf)
    out = {}
    for ch in animation["channels"]:
        sampler = animation["samplers"][ch["sampler"]]
        if sampler.get("interpolation", "LINEAR") != "LINEAR":
            raise ValueError("%s: %s interpolation is not read" % (animation.get("name"), sampler["interpolation"]))
        target = ch["target"]
        if "node" not in target:
            continue
        times = [t[0] for t in accessor(gltf, blob, sampler["input"])]
        out[(names[target["node"]], target["path"])] = (times, accessor(gltf, blob, sampler["output"]))
    return out


def qmul(a, b):
    """The quaternion product a b (x, y, z, w): b's rotation, then a's."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def qinv(q):
    """The inverse of a unit quaternion."""
    return (-q[0], -q[1], -q[2], q[3])


def angle(a, b):
    """The angle in degrees between the rotations of two unit quaternions."""
    return math.degrees(2.0 * math.acos(min(1.0, abs(sum(x * y for x, y in zip(a, b))))))


def world_rotations(gltf, local):
    """{node name: rotation in the scene} for local rotations {node name: quaternion}; a node without one keeps its
    own."""
    names = node_names(gltf)
    up = parents(gltf)
    out = {}

    def world(name):
        if name not in out:
            own = local.get(name) or tuple(gltf["nodes"][names.index(name)].get("rotation", (0.0, 0.0, 0.0, 1.0)))
            out[name] = qmul(world(up[name]), own) if name in up else own
        return out[name]

    for name in names:
        world(name)
    return out


def _normalized(q):
    length = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / length for c in q)


def slerp(a, b, f):
    """Spherical interpolation of unit quaternions (x, y, z, w), along the shorter arc."""
    if f <= 0.0:
        return tuple(a)
    if f >= 1.0:
        return tuple(b)
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0.0:
        b = tuple(-c for c in b)
        dot = -dot
    if dot > 0.9995:
        return _normalized(tuple(x + f * (y - x) for x, y in zip(a, b)))
    theta = math.acos(min(dot, 1.0))
    sa = math.sin((1.0 - f) * theta) / math.sin(theta)
    sb = math.sin(f * theta) / math.sin(theta)
    return tuple(sa * x + sb * y for x, y in zip(a, b))


def lerp(a, b, f):
    return tuple(x + f * (y - x) for x, y in zip(a, b))


def blend(path, a, b, f):
    """A channel's value between a and b (slerp for rotations)."""
    return slerp(a, b, f) if path == "rotation" else lerp(a, b, f)


def sample(path, times, values, t):
    """A LINEAR channel at time t, held at its ends. Keys at the same time take the later one."""
    if t <= times[0]:
        return values[0]
    if t >= times[-1]:
        return values[-1]
    lo, hi = 0, len(times) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if times[mid] <= t:
            lo = mid
        else:
            hi = mid
    span = times[hi] - times[lo]
    if span <= 1e-9:
        return values[hi]
    return blend(path, values[lo], values[hi], (t - times[lo]) / span)


def continuous(quaternions):
    """The quaternions with each on the side of the previous one (q and -q are the same rotation, but a key's
    neighbours are interpolated along the shorter arc only when they agree in sign)."""
    out = []
    for q in quaternions:
        if out and sum(x * y for x, y in zip(out[-1], q)) < 0.0:
            q = tuple(-c for c in q)
        out.append(tuple(q))
    return out


def add_animation(gltf, blob, name, tracks, rate):
    """Appends an animation of evenly spaced keys: tracks is {(node name, path): [value per key]}, every track as long,
    key k at k / rate seconds. Returns its index."""
    names = node_names(gltf)
    counts = {len(v) for v in tracks.values()}
    if len(counts) != 1:
        raise ValueError("%s: tracks of different lengths %s" % (name, sorted(counts)))
    count = counts.pop()
    kinds = {"rotation": "VEC4", "translation": "VEC3", "scale": "VEC3"}
    times = add_accessor(gltf, blob, [(k / rate,) for k in range(count)], "SCALAR", bounds=True)
    samplers, chans = [], []
    for (node, path), values in tracks.items():
        if path == "rotation":
            values = continuous(values)
        output = add_accessor(gltf, blob, values, kinds[path])
        chans.append({"sampler": len(samplers), "target": {"node": names.index(node), "path": path}})
        samplers.append({"input": times, "output": output, "interpolation": "LINEAR"})
    gltf.setdefault("animations", []).append({"name": name, "channels": chans, "samplers": samplers})
    return len(gltf["animations"]) - 1
