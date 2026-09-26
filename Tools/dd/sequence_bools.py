"""Re-exports the level sequences the zones import with their bool keys read (work list item 61).

pak_reference_2's export reads a BoolProperty inside an array from its tag's bool value, but an array's elements carry
no tag: the byte on disk is never read and every such bool comes out False (pak_reference_2/_tools/scripts/ue4.py,
read_prop_value). A visibility section's keys are such an array (FMovieSceneBoolChannel's Values), so every
visibility key of the export is False — Zone 2's capture scene nurse, hidden until 17.33 s in the original, was shown
from the start. This pulls the same packages from the installed game's pak, runs the export's own reader over them with
that one read fixed (the element's byte), and writes the documents in the export's format to
Intermediate/Pipeline/dd/_assets/<the same path>.json, which dd_assets.export_json reads before pak_reference_2's.
pak_reference_2 itself is left as it is (read only).

    python Tools/dd/sequence_bools.py            re-export, and list every value that differs from pak_reference_2's

Every difference must be a bool in an array; anything else stops it (the reader would have drifted).
"""
import json
import os
import sys
import tempfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.path.join(ROOT, "pak_reference_2")
PAK = r"C:\Program Files (x86)\Steam\steamapps\common\Dark Deception\DDeception\Content\Paks\DDeception-WindowsNoEditor.pak"
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "dd", "_assets")

# The packages dd_sequence imports (its SEQUENCE_ACTORS play these; FREE_SEQUENCES), under DDeception/Content.
SEQUENCES = [
    "Animation/06_Hospital/" + name for name in (
        "06_Hospital_Zone01_ElevatorArrive", "06_Hospital_Zone1_AmbulanceTakeOff", "06_Hospital_Zone1_SecretElevator",
        "06_Hospital_Zone1_SecretElevator1", "06_Hospital_Zone1_06Event", "06_Hospital_Zone2_Spikes",
        "06_Hospital_Zone2_Cell_DoorPicked", "06_Hospital_Zone2_AmbulanceArrive1", "06_Hospital_Zone2_AmbulanceTakeOff",
        "06_Hospital_Zone2_Capture", "06_Hospital_Zone2_Cell")
] + ["Animation/00_Ballroom/Ballroom_Event_Fade"]

sys.path.insert(0, os.path.join(REF, "_tools", "scripts"))
import ue4       # noqa: E402
import unpak     # noqa: E402

_read = ue4.read_prop_value


def _read_fixed(ar, typ, tag, size, pkg, depth=0, owner=None):
    # An array's element comes with an empty tag: its bool is the next byte, not the tag's.
    if typ == "BoolProperty" and "bool_val" not in tag:
        return bool(ar.u8())
    return _read(ar, typ, tag, size, pkg, depth, owner)


ue4.read_prop_value = _read_fixed


def export(path, rel):
    """The export_data.py document of one package."""
    pkg = ue4.Package(path)
    doc = {"package": rel, "exports": []}
    for e in pkg.exports:
        a = pkg.export_reader(e)
        ue4.set_budget(1.5)
        try:
            props = ue4.read_properties(a, pkg)
        except ue4.BudgetExceeded:
            props = {"_partial": "parse budget exceeded (native-serialised struct)"}
        except Exception as ex:
            props = {"_error": repr(ex)}
        doc["exports"].append({"name": e["name"], "class": str(e["class_name"]),
                               "outer": pkg.resolve_full(e["outer_index"]),
                               "super": pkg.resolve_full(e["super_index"]),
                               "template": pkg.resolve_full(e["template_index"]),
                               "is_asset": bool(e["is_asset"]), "props": props})
    doc["imports"] = sorted({im["name"] for im in pkg.imports})
    ue4.set_budget(None)
    return doc


def differences(old, new, where=""):
    """(path, old, new) of every value that differs."""
    if isinstance(old, dict) and isinstance(new, dict):
        for k in sorted(set(old) | set(new)):
            yield from differences(old.get(k), new.get(k), where + "/" + str(k))
    elif isinstance(old, list) and isinstance(new, list) and len(old) == len(new):
        for i, (a, b) in enumerate(zip(old, new)):
            yield from differences(a, b, "%s[%d]" % (where, i))
    elif old != new:
        yield where, old, new


def main():
    info, mount, entries = unpak.read_index(PAK)
    wanted = {"DDeception/Content/" + s + ext: s for s in SEQUENCES for ext in (".uasset", ".uexp")}
    chosen = [e for e in entries if ((mount + e["name"]) if mount else e["name"]).lstrip("/").replace("../../../", "") in wanted
              or any(e["name"].endswith(w) for w in wanted)]
    with tempfile.TemporaryDirectory() as tmp:
        count, written, bad, errors = unpak.extract((PAK, tmp, info, mount, chosen))
        if bad or errors:
            raise SystemExit("could not extract: %r" % (errors or "sha1 mismatch",))
        for rel in SEQUENCES:
            found = [os.path.join(d, f) for d, _, fs in os.walk(tmp) for f in fs
                     if os.path.join(d, f).replace("\\", "/").endswith("DDeception/Content/" + rel + ".uasset")]
            if len(found) != 1:
                raise SystemExit("%s: %d packages in the pak" % (rel, len(found)))
            package = "DDeception/Content/" + rel + ".uasset"
            doc = export(found[0], package)
            with open(os.path.join(REF, "_assets", "DDeception", "Content", *rel.split("/")) + ".json",
                      encoding="utf-8") as f:
                old = json.load(f)
            changed = list(differences(old, doc))
            odd = [c for c in changed if not (c[1] is False and isinstance(c[2], bool))]
            if odd:
                raise SystemExit("%s: differences that are not a bool read: %r" % (rel, odd[:5]))
            target = os.path.join(OUT, "DDeception", "Content", *rel.split("/")) + ".json"
            os.makedirs(os.path.dirname(target), exist_ok=True)
            with open(target, "w", encoding="utf-8") as f:
                json.dump(doc, f, ensure_ascii=False)
            print("%s: %d bools read" % (rel, len(changed)))
            for where, _, value in changed:
                print("   %s = %s" % (where[-110:], value))


if __name__ == "__main__":
    main()
