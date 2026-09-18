"""A cooked material's compiled shaders, disassembled: the graph the cook took away, read back as code.

    python Tools/dd/cooked_shaders.py <asset> [--pak PAK] [--out DIR] [--show N ...]

<asset> is part of the pak path, e.g. "AdvancedMagicFX09/Materials/MI_ky_starDust_sq." (the trailing dot keeps the
.uasset/.uexp pair and nothing longer). The latest build (UE 4.24, the one pak_reference_2 was made from) cooks each
material's shader map inline: its .uexp holds one zlib stream per shader, each wrapping a DXBC blob. This inflates
them, disassembles each with the system's d3dcompiler_47 (D3DDisassemble) into <out>/NN_<model>.txt and prints a
table: the model, the interpolators it reads, the resources and how many samples it takes, after the names the
shader map's uniform expressions use. `--show N` prints shader N's code.

Which one to read: the translucent base pass pixel shader is the ps_5_0 that binds a texture3d (the translucency
lighting / fog volume) and a texture2d it reads with sample_l (scene depth, for the depth fade) besides the
material's own textures; its first half is the material, its end the fog and the output (o0.w = the opacity after
the engine's saturate). One is compiled per vertex factory (sprite, mesh, GPU sprite) and fog variant; the sprite
one reads the DynamicParameter as an interpolator (TEXCOORD1) where the GPU sprite one has the defaults folded in.
Uniform parameters sit in the last constant buffer (cb3): one float4 per vector expression, then the scalar
expressions packed four to a float4. The table ends with the slots - `cb3[4].y = hilightPower (10)` - read off the
shader map's uniform expression set (each expression the index of its class in the shader map's name table, then its
own fields; `uniforms:` is that name table, each name once). If the set holds a class this does not know, only the
names are printed; read the slots off how the code uses them then (SelectionColor, the editor's highlight, is the
lerp at the end of the emissive, black at run time). A
material instance with a static switch of its own (bHasStaticPermutationResource) carries its own shader map; one
without (MI_ky_aura7c) has none and uses its parent's.

Needs pak_reference_2/_tools/scripts/unpak.py (the pak reader the export was made with) and Windows.
Env: PAK_REF2 — the export (default <repo>/pak_reference_2).
"""
import argparse
import ctypes
import os
import re
import struct
import sys
import zlib

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))
PAK = r"C:\Program Files (x86)\Steam\steamapps\common\Dark Deception\DDeception\Content\Paks\DDeception-WindowsNoEditor.pak"
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "dd", "shaders")


def pull(pak, asset, out):
    """The pak entries whose path contains asset, written under out/raw. Returns their paths."""
    sys.path.insert(0, os.path.join(REF, "_tools", "scripts"))
    import unpak
    info, mount, entries = unpak.read_index(pak)
    chosen = [e for e in entries if asset.lower() in e["name"].lower()]
    if not chosen:
        raise SystemExit("no entry of %s contains %r" % (pak, asset))
    raw = os.path.join(out, "raw")
    count, written, bad, errors = unpak.extract((pak, raw, info, mount, chosen))
    if bad or errors:
        raise SystemExit("could not extract: %r" % (errors or "sha1 mismatch",))
    return [os.path.join(raw, *((mount + "/" + e["name"]) if mount else e["name"]).lstrip("/").split("/"))
            for e in chosen]


def disassemble(code):
    """D3DDisassemble(code) as text, or None when it is not a shader."""
    d3d = ctypes.WinDLL("d3dcompiler_47.dll")
    blob = ctypes.c_void_p()
    if d3d.D3DDisassemble(ctypes.c_char_p(code), ctypes.c_size_t(len(code)), 0, None, ctypes.byref(blob)) != 0:
        return None
    vtable = ctypes.cast(ctypes.cast(blob, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
    pointer = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(vtable[3])(blob)       # ID3DBlob::GetBufferPointer
    size = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(vtable[4])(blob)          # ID3DBlob::GetBufferSize
    text = ctypes.string_at(pointer, size).decode("ascii", "replace").replace("\x00", "")
    ctypes.WINFUNCTYPE(ctypes.c_ulong, ctypes.c_void_p)(vtable[2])(blob)                  # IUnknown::Release
    return text


def shaders(data):
    """Each DXBC blob inside the zlib streams of data, in file order."""
    for i in range(len(data) - 1):
        if data[i] != 0x78 or data[i + 1] not in (0x01, 0x5E, 0x9C, 0xDA):
            continue
        try:
            inflated = zlib.decompressobj().decompress(data[i:i + (1 << 20)])
        except zlib.error:
            continue
        j = inflated.find(b"DXBC")
        if j >= 0:
            yield inflated[j:j + struct.unpack_from("<I", inflated, j + 24)[0]]


def uniform_names(data):
    """The uniform expression classes and parameter names of the shader map (printable runs after the first
    FMaterialUniformExpression, up to the first vertex factory's name). A name is written once, where it first
    appears, so this is the set of names in that order, not the expressions one by one."""
    runs = [m.group().decode() for m in re.finditer(rb"[\x20-\x7e]{4,}", data)]
    start = next((k for k, r in enumerate(runs) if r.startswith("FMaterialUniformExpression")), None)
    if start is None:
        return []
    names = []
    for r in runs[start:]:
        if r.endswith("VertexFactory"):
            break
        names.append(r.replace("FMaterialUniformExpression", "") if r.startswith("FMaterialUniformExpression")
                     else "'%s'" % r)
    return names


def name_table(data):
    """The shader map's name table ([name, ...], the offset after it): an int32 count, then each name as an int32
    length, its characters with a NUL and a 4-byte hash. It is the one holding the uniform expressions' classes."""
    first = data.find(b"FMaterialUniformExpression")
    if first < 8:
        return [], None
    at = first - 8
    count = struct.unpack_from("<i", data, at)[0]
    at += 4
    names = []
    for _ in range(count):
        size = struct.unpack_from("<i", data, at)[0]
        if not 0 < size < 256:
            return [], None
        names.append(data[at + 4:at + 3 + size].decode("ascii", "replace"))
        at += 4 + size + 4
    return names, at


# The fields each uniform expression class serializes after its class name (UE 4.24's MaterialUniformExpressions.h),
# as a format: e expression, n FName, i int32, b int8, f float, c FLinearColor, a an association (int8).
FIELDS = {
    "Constant": "cb", "VectorParameter": "nai" + "c", "ScalarParameter": "naif", "ComponentSwizzle": "ebbbbb",
    "AppendVector": "eei", "Max": "ee", "Min": "ee", "Clamp": "eee", "Saturate": "e", "Abs": "e", "Floor": "e",
    "Ceil": "e", "Frac": "e", "Periodic": "e", "SquareRoot": "e", "Logarithm2": "e", "Logarithm10": "e",
    "Fmod": "ee", "Sine": "ei", "Length": "ei", "FoldedMath": "eeib", "Time": "", "RealTime": "",
}
FOLDED = {0: "+", 1: "-", 2: "*", 3: "/", 4: "dot", 5: "cross"}


def uniform_slots(data):
    """["cb3[N].c = expression", ...] for the shader map's vector and scalar uniform expressions, or [] where the
    set is not found or holds a class FIELDS does not know."""
    names, start = name_table(data)
    if start is None:
        return []

    def expression(at):
        index = struct.unpack_from("<i", data, at)[0]
        if not 0 <= index < len(names) or not names[index].startswith("FMaterialUniformExpression"):
            raise ValueError
        kind = names[index][len("FMaterialUniformExpression"):]
        if kind not in FIELDS:
            raise ValueError
        at += 8
        parts = []
        for field in FIELDS[kind]:
            if field == "e":
                text, at = expression(at)
                parts.append(text)
            elif field == "n":
                index = struct.unpack_from("<i", data, at)[0]
                parts.append(names[index] if 0 <= index < len(names) else "?")
                at += 8
            elif field in "ab":
                parts.append(struct.unpack_from("<b", data, at)[0])
                at += 1
            elif field == "i":
                parts.append(struct.unpack_from("<i", data, at)[0])
                at += 4
            elif field == "f":
                parts.append(round(struct.unpack_from("<f", data, at)[0], 6))
                at += 4
            elif field == "c":
                parts.append(tuple(round(v, 6) for v in struct.unpack_from("<4f", data, at)))
                at += 16
        if kind in ("ScalarParameter", "VectorParameter"):
            return "%s (%s)" % (parts[0], ", ".join(map(str, parts[3])) if kind == "VectorParameter" else parts[3]), at
        if kind == "Constant":
            value = parts[0]
            return (str(value[0]) if value[1:] == (value[0],) * 3 else str(value)), at
        if kind == "ComponentSwizzle":
            return "%s.%s" % (parts[0], "".join("rgba"[k] for k in parts[1:1 + parts[5]])), at
        if kind == "FoldedMath":
            return "(%s %s %s)" % (parts[0], FOLDED.get(parts[3], "?"), parts[1]), at
        return "%s(%s)" % (kind, ", ".join(str(p) for p in parts if isinstance(p, str))), at

    def listing(at):
        count = struct.unpack_from("<i", data, at)[0]
        if not 0 <= count < 256:
            raise ValueError
        at += 4
        items = []
        for _ in range(count):
            text, at = expression(at)
            items.append(text)
        return items, at

    for at in range(start, min(len(data) - 12, start + 8192)):
        try:
            vectors, after = listing(at)
            if not vectors:
                continue
            scalars, _ = listing(after)
        except (ValueError, struct.error, IndexError):
            continue
        slots = ["cb3[%d] = %s" % (k, v) for k, v in enumerate(vectors)]
        slots += ["cb3[%d].%s = %s" % (len(vectors) + k // 4, "xyzw"[k % 4], v) for k, v in enumerate(scalars)]
        return slots
    return []


def summary(text):
    lines = text.splitlines()
    body = [l for l in lines if l and not l.startswith("//")]
    model = next((l.strip() for l in body if re.match(r"(vs|ps|gs|hs|ds|cs)_\d_\d", l)), "?")
    inputs = []
    section = None
    for l in lines:
        if l.startswith("// Input signature"):
            section = "in"
        elif l.startswith("// Output signature"):
            section = None
        elif section == "in" and re.match(r"// (TEXCOORD|ATTRIBUTE|SV_)", l):
            name, index = l[3:].split()[:2]
            inputs.append(name.replace("TEXCOORD", "T").replace("ATTRIBUTE", "A") + index)
    resources = [re.sub(r"^dcl_resource_(\w+)(?: \([\w,]+\))? (.*)$", r"\1 \2", l.strip()) for l in body
                 if l.startswith("dcl_resource")]
    samples = sum(1 for l in body if l.strip().startswith("sample"))
    return model, len(body), inputs, resources, samples


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("asset")
    ap.add_argument("--pak", default=PAK)
    ap.add_argument("--out", default=None, help="default Intermediate/Pipeline/dd/shaders/<asset's last part>")
    ap.add_argument("--show", type=int, nargs="*", default=[])
    args = ap.parse_args()
    name = re.sub(r"[^\w]+", "_", args.asset.rstrip(".").split("/")[-1])
    out = args.out or os.path.join(OUT, name)
    os.makedirs(out, exist_ok=True)
    uexp = [p for p in pull(args.pak, args.asset, out) if p.endswith(".uexp")]
    if len(uexp) != 1:
        raise SystemExit("%r matches %d .uexp files; make it narrower (end it with a dot)" % (args.asset, len(uexp)))
    data = open(uexp[0], "rb").read()
    print(os.path.relpath(uexp[0], ROOT), len(data), "bytes")
    print("uniforms:", " ".join(uniform_names(data)) or "(none: no shader map of its own)")
    for slot in uniform_slots(data):
        print("  " + slot)
    texts = []
    for code in shaders(data):
        text = disassemble(code)
        if text is None:
            continue
        model, count, inputs, resources, samples = summary(text)
        path = os.path.join(out, "%02d_%s.txt" % (len(texts), model))
        with open(path, "w", encoding="ascii") as f:
            f.write(text)
        print("%2d %-7s %4d lines  samples %d  in %s  res %s"
              % (len(texts), model, count, samples, ",".join(inputs), "; ".join(resources)))
        texts.append(text)
    print("%d shaders -> %s" % (len(texts), os.path.relpath(out, ROOT)))
    for n in args.show:
        print("\n== %d ==" % n)
        print("\n".join(l for l in texts[n].splitlines() if l and not l.startswith("//")))


if __name__ == "__main__":
    main()
