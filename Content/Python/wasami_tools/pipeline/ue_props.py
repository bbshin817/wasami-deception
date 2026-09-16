"""Writing property values from the original games' exported data (UE property names and JSON values) onto UE objects
and structs through their Python names."""
import re

import unreal

# Properties the original's engine (UE 4.21 / 4.24) had under another name. UE 5 renamed the height fog's colours to
# luminance; the field is the same LinearColor, so the exported value carries over as it is.
RENAMED = {
    "FogInscatteringColor": "FogInscatteringLuminance",
    "DirectionalInscatteringColor": "DirectionalInscatteringLuminance",
}


def snake(name):
    """A UE property name → its Python name ('CameraISO' → 'camera_iso', 'bOverride_WhiteTemp' → 'override_white_temp')."""
    if len(name) > 1 and name[0] == "b" and name[1].isupper():
        name = name[1:]
    parts = [re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", p) for p in name.split("_") if p]
    return "_".join(parts).lower()


def enum_member(cls, member):
    """'EOO_OffsetZero' / 'SLS_SpecifiedCubemap' → the member of the Python enum class ('EOO_OFFSET_ZERO')."""
    return getattr(cls, re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", member).upper())


def value(raw, current):
    """An exported value → the Python value for a property whose current value is `current`, or None when the export's
    shape is not one this reads (object references, arrays)."""
    if isinstance(raw, dict):
        if {"X", "Y", "Z", "W"} <= raw.keys():
            return unreal.Vector4(raw["X"], raw["Y"], raw["Z"], raw["W"])
        if {"X", "Y", "Z"} <= raw.keys():
            return unreal.Vector(raw["X"], raw["Y"], raw["Z"])
        if {"R", "G", "B"} <= raw.keys():
            if isinstance(current, unreal.Color):
                return unreal.Color(r=int(raw["R"]), g=int(raw["G"]), b=int(raw["B"]), a=int(raw.get("A", 255)))
            return unreal.LinearColor(raw["R"], raw["G"], raw["B"], raw.get("A", 1.0))
        return None
    # pak_reference_2 writes colours and vectors as arrays ([183, 163, 145, 255], [0.27, 0.33, 0.44, 1.0])
    if isinstance(raw, (list, tuple)) and 2 <= len(raw) <= 4 and all(isinstance(v, (int, float)) for v in raw):
        n = [float(v) for v in raw]
        if isinstance(current, unreal.Color):
            return unreal.Color(r=int(n[0]), g=int(n[1]), b=int(n[2]), a=int(n[3]) if len(n) > 3 else 255)
        if isinstance(current, unreal.LinearColor):
            return unreal.LinearColor(n[0], n[1], n[2], n[3] if len(n) > 3 else 1.0)
        if isinstance(current, unreal.Vector) and len(n) >= 3:
            return unreal.Vector(n[0], n[1], n[2])
        if isinstance(current, unreal.Vector4) and len(n) >= 4:
            return unreal.Vector4(n[0], n[1], n[2], n[3])
        if isinstance(current, unreal.Vector2D) and len(n) >= 2:
            return unreal.Vector2D(n[0], n[1])
        return None
    if isinstance(current, unreal.EnumBase) and isinstance(raw, str):
        return enum_member(type(current), raw.split("::")[-1])
    if isinstance(current, float) and isinstance(raw, (int, float)):
        return float(raw)
    if isinstance(current, bool) and isinstance(raw, bool):
        return raw
    if isinstance(current, int) and isinstance(raw, int):
        return raw
    if isinstance(current, str) and isinstance(raw, str):
        return raw
    return None


def apply(obj, props, skip=(), failures=None):
    """Sets each exported property on obj (a UObject or struct). Nested dicts go into struct properties member by member
    (the export keeps only the members that differ from the defaults). Unreadable ones go to failures."""
    failures = failures if failures is not None else []
    for key, raw in props.items():
        if key in skip:
            continue
        name = snake(RENAMED.get(key, key))
        try:
            current = obj.get_editor_property(name)
            if isinstance(raw, dict) and isinstance(current, unreal.StructBase) and value(raw, current) is None:
                apply(current, raw, (), failures)
                obj.set_editor_property(name, current)
                continue
            v = value(raw, current)
            if v is None:
                raise ValueError("unsupported value %r" % (raw,))
            obj.set_editor_property(name, v)
        except Exception as e:  # noqa: BLE001
            failures.append("%s.%s: %s" % (type(obj).__name__, key, e))
    return failures
