"""Chaotic Customer 2 Zone_1: assembles the stage level from the imported assets (cc2_assets) and stage_ue.json —
the placed meshes, the lights, reflection captures, post process volumes, fog, sky light and the player start. Every
actor it places carries the tag 'cc2', which a rebuild removes first."""
import unreal

from wasami_tools.pipeline import paths, ue_props

EAL = unreal.EditorAssetLibrary
TAG = "cc2"
MOVABLE = unreal.ComponentMobility.MOVABLE
UNITS = {
    "Candelas": unreal.LightUnits.CANDELAS,
    "Unitless": unreal.LightUnits.UNITLESS,
    "Lumens": unreal.LightUnits.LUMENS,
    "EV": unreal.LightUnits.EV,
}
LIGHT_CLASS = {"point": unreal.PointLight, "spot": unreal.SpotLight, "rect": unreal.RectLight}
# The player is Dark Deception's, whose motion blur is UE's default 0.5: the stage volumes' MotionBlurAmount 0 (the fan
# game turned it off for its own player) is not taken (.claude/guides/original-fidelity.md).
SKIP_VOLUME_SETTINGS = ("MotionBlur",)
# The capsule's half height over the floor, for the player start.
START_LIFT = 100.0


def _vec(v):
    return unreal.Vector(float(v[0]), float(v[1]), float(v[2]))


def _rot(q):
    return unreal.Quat(float(q[0]), float(q[1]), float(q[2]), float(q[3])).rotator()


def _tag(actor, label, folder, *tags):
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.tags = [unreal.Name(TAG)] + [unreal.Name(t) for t in tags if t]


def _open_level(map_path):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(map_path):
        if not les.load_level(map_path):
            raise RuntimeError("could not open " + map_path)
    elif not les.new_level(map_path):
        raise RuntimeError("could not create " + map_path)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    old = [a for a in eas.get_all_level_actors() if a.actor_has_tag(TAG)]
    if old:
        eas.destroy_actors(old)
    return les, eas


def _meshes(eas, stage, counts):
    cache = {}

    def asset(path):
        if path not in cache:
            cache[path] = unreal.load_asset(path)
            if cache[path] is None:
                raise RuntimeError("missing asset %s: run WasamiStageTools.import_cc2_assets until nothing remains" % path)
        return cache[path]

    with unreal.ScopedSlowTask(len(stage["placements"]), "Placing the stage's meshes") as task:
        for p in stage["placements"]:
            task.enter_progress_frame(1)
            mesh_info = stage["meshes"][p["mesh"]]
            actor = eas.spawn_actor_from_object(asset(mesh_info["asset"]), _vec(p["location"]), _rot(p["rotation"]))
            actor.set_actor_scale3d(_vec(p["scale"]))
            comp = actor.static_mesh_component
            if p["role"] != "static":
                comp.set_mobility(MOVABLE)
            for i, key in enumerate(p["materials"]):
                if key:
                    comp.set_material(i, asset(stage["materials"][key]["asset"]))
            if not p["collision"] or mesh_info["collision"] == "none":
                comp.set_collision_profile_name("NoCollision")
            if p["hidden"]:
                actor.set_actor_hidden_in_game(True)
            _tag(actor, "%s.%s" % (p["actor"], p["comp"]), "CC2/Meshes/" + p["role"], "role:" + p["role"], "src:" + p["actor"])
            counts["meshes"] += 1


def _lights(eas, stage, counts):
    for lt in stage["lights"]:
        actor = eas.spawn_actor_from_class(LIGHT_CLASS[lt["kind"]], _vec(lt["location"]), _rot(lt["rotation"]))
        c = actor.get_editor_property("light_component")
        c.set_mobility(MOVABLE)
        c.set_editor_property("intensity_units", UNITS.get(lt["units"], unreal.LightUnits.CANDELAS))
        c.set_editor_property("intensity", float(lt["intensity"]))
        r, g, b = (int(round(v * 255.0)) for v in lt["color"][:3])
        c.set_editor_property("light_color", unreal.Color(r=r, g=g, b=b, a=255))
        c.set_editor_property("attenuation_radius", float(lt["attenuationRadius"]))
        c.set_editor_property("cast_shadows", bool(lt["castShadows"]))
        c.set_editor_property("volumetric_scattering_intensity", float(lt["volumetric"]))
        if lt["kind"] in ("point", "spot"):
            c.set_editor_property("source_radius", float(lt.get("sourceRadius", 0.0)))
            if "sourceLength" in lt:
                c.set_editor_property("source_length", float(lt["sourceLength"]))
        if lt["kind"] == "spot":
            c.set_editor_property("inner_cone_angle", float(lt["innerCone"]))
            c.set_editor_property("outer_cone_angle", float(lt["outerCone"]))
        if lt["kind"] == "rect":
            c.set_editor_property("source_width", float(lt.get("sourceWidth", 64.0)))
            c.set_editor_property("source_height", float(lt.get("sourceHeight", 64.0)))
            if "barnDoorAngle" in lt:
                c.set_editor_property("barn_door_angle", float(lt["barnDoorAngle"]))
                c.set_editor_property("barn_door_length", float(lt["barnDoorLength"]))
        _tag(actor, lt["name"], "CC2/Lights/" + (lt.get("owner") or lt["kind"]), "owner:" + (lt.get("owner") or ""), "src:" + lt["actor"])
        counts["lights"] += 1


def _captures(eas, stage, counts):
    for cap in stage["captures"]:
        actor = eas.spawn_actor_from_class(unreal.BoxReflectionCapture, _vec(cap["location"]), unreal.Rotator())
        actor.set_actor_scale3d(unreal.Vector(*[e / 100.0 for e in cap["extent"]]))
        _tag(actor, cap["source"], "CC2/Environment")
        counts["captures"] += 1


def _volumes(eas, stage, counts, failures):
    for v in stage["volumes"]:
        keys = [k for k in v["overrides"] if not k.startswith(SKIP_VOLUME_SETTINGS)]
        if not keys:
            continue
        actor = eas.spawn_actor_from_class(unreal.PostProcessVolume, _vec(v["centreUe"]), unreal.Rotator())
        actor.set_editor_property("unbound", bool(v["unbound"]))
        if not v["unbound"]:
            actor.set_actor_scale3d(unreal.Vector(*[h / 100.0 for h in v["halfUe"]]))
            _origin, extent = actor.get_actor_bounds(False)
            if extent.x < 1.0:
                counts["volumes_without_brush"] += 1
        actor.set_editor_property("priority", float(v["priority"]))
        actor.set_editor_property("blend_weight", float(v["blendWeight"]))
        ps = actor.get_editor_property("settings")
        for key in keys:
            name = ue_props.snake(key)
            try:
                if key == "ColorGradingLUT":
                    value = unreal.load_asset(stage["textures"][v["lut"]]["asset"]) if v.get("lut") else None
                    if value is None:
                        raise ValueError("no LUT texture")
                    ps.set_editor_property(name, value)
                elif key in v["settings"]:
                    value = ue_props.value(v["settings"][key], ps.get_editor_property(name))
                    if value is None:
                        raise ValueError("unsupported value %r" % (v["settings"][key],))
                    ps.set_editor_property(name, value)
                # else: overridden at the engine's default, which the cooked export leaves out
                ps.set_editor_property("override_" + name, True)
            except Exception as e:  # noqa: BLE001
                failures.append("PostProcessSettings.%s (%s): %s" % (key, v["source"], e))
        actor.set_editor_property("settings", ps)
        _tag(actor, v["source"], "CC2/Environment")
        counts["volumes"] += 1


def _fog_and_sky(eas, stage, counts, failures):
    placement = {"RelativeLocation", "RelativeRotation", "RelativeScale3D"}
    fog = stage["fog"]
    actor = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, _vec(fog["positionUe"]), unreal.Rotator())
    ue_props.apply(actor.get_editor_property("component"), fog["props"], placement, failures)
    _tag(actor, "ExponentialHeightFog", "CC2/Environment")
    counts["fog"] += 1

    sky = stage["sky"]
    actor = eas.spawn_actor_from_class(unreal.SkyLight, _vec(sky["positionUe"]), unreal.Rotator())
    c = actor.get_editor_property("light_component")
    c.set_mobility(MOVABLE)
    ue_props.apply(c, sky["props"], placement | {"Cubemap", "Mobility", "CastRaytracedShadow"}, failures)
    if sky.get("cubemap"):
        c.set_editor_property("cubemap", unreal.load_asset(stage["textures"][sky["cubemap"]]["asset"]))
    c.recapture_sky()
    _tag(actor, "SkyLight", "CC2/Environment")
    counts["sky"] += 1


def _player_start(eas, stage, counts):
    start = stage["gameplay"]["start"]
    loc = _vec(start["location"])
    loc.z += START_LIFT
    # unreal.Rotator's positional order is (roll, pitch, yaw): name them
    actor = eas.spawn_actor_from_class(unreal.PlayerStart, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(start["yaw"])))
    _tag(actor, "PlayerStart_Checkpoint1", "Gameplay")
    counts["player_start"] += 1


def build(map_path):
    stage = paths.load_cc2_stage()
    les, eas = _open_level(map_path)
    counts = {k: 0 for k in ("meshes", "lights", "captures", "volumes", "volumes_without_brush", "fog", "sky", "player_start")}
    failures = []
    _meshes(eas, stage, counts)
    _lights(eas, stage, counts)
    _captures(eas, stage, counts)
    _volumes(eas, stage, counts, failures)
    _fog_and_sky(eas, stage, counts, failures)
    _player_start(eas, stage, counts)
    for f in failures:
        unreal.log_warning("build_cc2_level: " + f)
    counts["failed_settings"] = len(failures)
    if not les.save_current_level():
        raise RuntimeError("could not save " + map_path)
    return counts
