"""Dark Deception's hospital: assembles one zone's level from the imported assets (dd_stage) and stage_ue.json — the
placed meshes (the teleport's zones among them, with their own collision), the lights, the reflection captures, the
fog, the sky light, the post process volumes, the player starts and the minimap's map plane. Every actor it places
carries the tag 'dd', which a rebuild removes first."""
import unreal

from wasami_tools.pipeline import paths, ue_props

EAL = unreal.EditorAssetLibrary
TAG = "dd"

LIGHT_CLASS = {
    "PointLightComponent": unreal.PointLight,
    "SpotLightComponent": unreal.SpotLight,
    "RectLightComponent": unreal.RectLight,
    "DirectionalLightComponent": unreal.DirectionalLight,
}
# UE 4.24's default for a local light's IntensityUnits is Unitless, UE 5's is Candelas, and the export leaves out any
# property that is at its default — so a light the export says nothing about has to be set to Unitless explicitly.
DEFAULT_LIGHT_UNITS = "ELightUnits::Unitless"
# IESTexture: the original's light profiles are not in the export (the hospital's 6 lights using one are the
# ambulance's spot lights, at intensity 0).
SKIP_LIGHT_PROPS = ("LightGuid", "MapBuildDataId", "IESTexture")
# Every component's Mobility comes from the preprocessing, which reads a left-out one as its archetype's
# (Tools/dd/prepare_stage.py NATIVE_MOBILITY): most of the hospital's lights are Stationary (Zone 1: 669, the rest
# Movable), so their direct light is drawn every frame over baked indirect light, and their
# VolumetricScatteringIntensity (20 on the 294 ceiling lights) lights the volumetric fog as in the original.
# A component the preprocessing says nothing about keeps what the spawned actor gave it, which is the same rule.
# The minimap's plane: the original's BP_MapTexture (Zone 1) and BP_MapTexture_MultiFloor (Zone 2) put
# /Engine/BasicShapes/Plane under the level with the zone's baked map on it, and the player's scene capture draws it
# into T_NewMap. The material is each actor's OverrideMaterials in the export.
MAP_PLANE_MESH = "/Engine/BasicShapes/Plane"
MAP_PLANE_CLASSES = ("BP_MapTexture_C", "BP_MapTexture_MultiFloor_C")
MAP_PLANE_MATERIAL = {"Zone1": "/Game/DD/UI/Minimap/MM_Map_06_Zone01", "Zone2": "/Game/DD/UI/Minimap/MM_Map_06_Zone2"}
# WasamiPlayerCharacter's scene capture shows only the actors with this tag and the shards.
MINIMAP_TAG = "dd_minimap"

# The original's custom collision channels by slot, as Config/DefaultEngine.ini names them.
CUSTOM_CHANNELS = {"ECC_GameTraceChannel1": "ECC_Teleport"}

# Component properties the placement handles itself.
SKIP_COMPONENT_PROPS = ("Mobility", "CollisionProfileName", "bVisible", "bHiddenInGame")


def _vec(v):
    return unreal.Vector(float(v[0]), float(v[1]), float(v[2]))


def _rot(q):
    return unreal.Quat(float(q[0]), float(q[1]), float(q[2]), float(q[3])).rotator()


def _set_mobility(comp, props):
    """Sets the component's Mobility as the preprocessing gives it; without one, the spawned actor's stays."""
    value = props.get("Mobility")
    if value:
        comp.set_mobility(ue_props.enum_member(unreal.ComponentMobility, value.split("::")[-1]))


def _channel(name):
    """An exported channel ('ECC_WorldStatic', 'ECC_GameTraceChannel1') as the Python enum member; Python names a custom
    channel by its name in the project's collision settings (Config/DefaultEngine.ini), not by its slot."""
    return ue_props.enum_member(unreal.CollisionChannel, CUSTOM_CHANNELS.get(name, name))


def _set_collision(comp, collision):
    """A custom collision from the preprocessing (the teleport zones'): the object type, what the body takes part in
    and each channel's response. Setting them makes the profile 'Custom', as in the original. A StaticMeshActor's
    component takes the mesh's own collision (bUseDefaultCollision, StaticMeshActor.cpp) until that is turned off."""
    comp.set_editor_property("use_default_collision", False)
    comp.set_collision_object_type(_channel(collision["objectType"]))
    comp.set_collision_enabled(ue_props.enum_member(unreal.CollisionEnabled, collision["enabled"].split("::")[-1]))
    for channel, response in collision["responses"].items():
        comp.set_collision_response_to_channel(_channel("ECC_" + channel),
                                               ue_props.enum_member(unreal.CollisionResponseType, response))


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


# ------------------------------------------------------------------------------------------------ meshes
def _meshes(eas, stage, zone, counts, failures):
    cache = {}

    def asset(path):
        if path not in cache:
            cache[path] = unreal.load_asset(path)
            if cache[path] is None:
                raise RuntimeError("missing asset %s: run WasamiStageTools.import_dd_stage_assets until nothing remains" % path)
        return cache[path]

    with unreal.ScopedSlowTask(len(zone["placements"]), "Placing the hospital's meshes") as task:
        for p in zone["placements"]:
            task.enter_progress_frame(1)
            info = stage["meshes"][p["mesh"]]
            world = p["world"]
            actor = eas.spawn_actor_from_object(asset(info["asset"] or info["source"]),
                                                _vec(world["location"]), _rot(world["quat_xyzw"]))
            actor.set_actor_scale3d(_vec(world["scale"]))
            comp = actor.static_mesh_component
            decal = False
            for i, key in enumerate(p["materials"]):
                if not key:
                    continue
                m = stage["materials"][key]
                comp.set_material(i, asset(m["asset"]))
                decal = decal or m["master"] == "decal"
            props = dict(p["props"])
            _set_mobility(comp, props)
            if decal:
                comp.set_collision_profile_name("NoCollision")   # the original's decals are planes, not colliders
            elif p.get("collision"):
                _set_collision(comp, p["collision"])
            elif props.get("CollisionProfileName"):
                comp.set_collision_profile_name(props["CollisionProfileName"])
            if props.get("bVisible") is False:
                comp.set_visibility(False)
            if props.get("bHiddenInGame"):
                actor.set_actor_hidden_in_game(True)
            ue_props.apply(comp, props, skip=SKIP_COMPONENT_PROPS, failures=failures)
            _tag(actor, p["actor"] + "." + p["path"].rsplit(".", 1)[-1],
                 "Hospital/Meshes/" + (p["actorClass"] or "StaticMeshActor"), "src:" + p["actor"])
            counts["meshes"] += 1
            counts["decals"] += 1 if decal else 0


# ------------------------------------------------------------------------------------------------ lights
def _lights(eas, zone, counts, failures):
    with unreal.ScopedSlowTask(len(zone["lights"]), "Placing the hospital's lights") as task:
        for lt in zone["lights"]:
            task.enter_progress_frame(1)
            cls = LIGHT_CLASS.get(lt["class"])
            if cls is None or not lt["world"]:
                failures.append("light %s: no class for %s" % (lt["path"], lt["class"]))
                continue
            actor = eas.spawn_actor_from_class(cls, _vec(lt["world"]["location"]), _rot(lt["world"]["quat_xyzw"]))
            c = actor.get_editor_property("light_component")
            _set_mobility(c, lt["props"])
            props = {k: v for k, v in lt["props"].items() if k not in SKIP_LIGHT_PROPS}
            units = props.pop("IntensityUnits", None if cls is unreal.DirectionalLight else DEFAULT_LIGHT_UNITS)
            if units:                                    # before Intensity: the units decide what the number means
                c.set_editor_property("intensity_units", ue_props.enum_member(unreal.LightUnits, units.split("::")[-1]))
            ue_props.apply(c, props, skip=("Mobility",), failures=failures)
            _tag(actor, lt["actor"] + "." + lt["path"].rsplit(".", 1)[-1],
                 "Hospital/Lights/" + (lt["actorClass"] or lt["class"]), "src:" + lt["actor"])
            counts["lights"] += 1


# ------------------------------------------------------------------------------------------------ environment
def _captures(eas, zone, stage, counts, failures):
    for cap in zone["captures"]:
        cls = unreal.SphereReflectionCapture if cap["kind"] == "sphere" else unreal.BoxReflectionCapture
        actor = eas.spawn_actor_from_class(cls, _vec(cap["world"]["location"]), unreal.Rotator())
        actor.set_actor_scale3d(_vec(cap["world"]["scale"]))
        comp = actor.get_editor_property("capture_component")
        if cap.get("brightness") is not None:
            comp.set_editor_property("brightness", float(cap["brightness"]))
        if cap.get("influenceRadius") is not None and cap["kind"] == "sphere":
            comp.set_editor_property("influence_radius", float(cap["influenceRadius"]))
        _cubemap(comp, cap.get("sourceType"), cap.get("cubemapFile"), stage, failures, cap["path"])
        _tag(actor, cap["path"].rsplit(".", 2)[-2], "Hospital/Environment")
        counts["captures"] += 1


def _cubemap(comp, source_type, cubemap_file, stage, failures, where):
    """Points a sky light or capture at the original's cubemap. The export writes the HDRI as a flat picture, so it
    imports as a Texture2D; UE wants a TextureCube here, and falls back to capturing the scene when it has none."""
    if not source_type or "SpecifiedCubemap" not in str(source_type):
        return
    entry = stage["textures"].get(cubemap_file) if cubemap_file else None
    tex = unreal.load_asset(entry["asset"]) if entry and EAL.does_asset_exist(entry["asset"]) else None
    if isinstance(tex, unreal.TextureCube):
        comp.set_editor_property("cubemap", tex)
        comp.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    else:
        failures.append("%s: %s is not a TextureCube, capturing the scene instead" % (where, cubemap_file))


def _environment(eas, zone, stage, counts, failures):
    placement = {"RelativeLocation", "RelativeRotation", "RelativeScale3D"}
    fog = zone.get("fog")
    if fog:
        actor = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, _vec(fog["world"]["location"]), unreal.Rotator())
        ue_props.apply(actor.get_editor_property("component"), fog["props"], placement, failures)
        _tag(actor, "ExponentialHeightFog", "Hospital/Environment")
        counts["fog"] += 1
    sky = zone.get("sky")
    if sky:
        actor = eas.spawn_actor_from_class(unreal.SkyLight, _vec(sky["world"]["location"]), unreal.Rotator())
        c = actor.get_editor_property("light_component")
        _set_mobility(c, sky["props"])
        props = {k: v for k, v in sky["props"].items() if k not in ("Cubemap", "SourceType", "Mobility")}
        ue_props.apply(c, props, placement, failures)
        _cubemap(c, sky["props"].get("SourceType"), sky.get("cubemapFile"), stage, failures, sky["path"])
        c.recapture_sky()
        _tag(actor, "SkyLight", "Hospital/Environment")
        counts["sky"] += 1


def _post_process(eas, zone, counts, failures):
    for v in zone["postProcess"]:
        settings = v["settings"] or {}
        keys = [k[len("bOverride_"):] for k, on in settings.items() if k.startswith("bOverride_") and on]
        if not keys:
            continue
        actor = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(), unreal.Rotator())
        actor.set_editor_property("unbound", bool(v["unbound"]))
        if v.get("priority") is not None:
            actor.set_editor_property("priority", float(v["priority"]))
        if v.get("blendWeight") is not None:
            actor.set_editor_property("blend_weight", float(v["blendWeight"]))
        ps = actor.get_editor_property("settings")
        for key in keys:
            name = ue_props.snake(key)
            try:
                if key in settings:
                    value = ue_props.value(settings[key], ps.get_editor_property(name))
                    if value is None:
                        raise ValueError("unsupported value %r" % (settings[key],))
                    ps.set_editor_property(name, value)
                # else: overridden at the engine's default, which the cooked export leaves out
                ps.set_editor_property("override_" + name, True)
            except Exception as e:  # noqa: BLE001
                failures.append("PostProcessSettings.%s (%s): %s" % (key, v["path"], e))
        actor.set_editor_property("settings", ps)
        _tag(actor, v["path"].rsplit(".", 1)[-1], "Hospital/Environment")
        counts["postProcess"] += 1


def _player_starts(eas, zone, counts):
    for a in zone["actors"]:
        if a["class"] != "PlayerStart" or not a["world"]:
            continue
        actor = eas.spawn_actor_from_class(unreal.PlayerStart, _vec(a["world"]["location"]), _rot(a["world"]["quat_xyzw"]))
        _tag(actor, a["name"], "Hospital/Gameplay")
        counts["playerStarts"] += 1


# ------------------------------------------------------------------------------------------------ minimap
def _map_plane(eas, zone, zone_name, counts, failures):
    mesh = unreal.load_asset(MAP_PLANE_MESH)
    material = unreal.load_asset(MAP_PLANE_MATERIAL.get(zone_name, ""))
    for a in zone["actors"]:
        if a["class"] not in MAP_PLANE_CLASSES or not a["world"]:
            continue
        actor = eas.spawn_actor_from_object(mesh, _vec(a["world"]["location"]), _rot(a["world"]["quat_xyzw"]))
        actor.set_actor_scale3d(_vec(a["world"]["scale"]))
        comp = actor.static_mesh_component
        comp.set_editor_property("cast_shadow", False)
        comp.set_editor_property("can_ever_affect_navigation", False)
        # UE 5 only: the plane is for the capture, so it is kept out of the view and of Lumen (the original relies on
        # it being under the floor and on baked lighting, neither of which holds here).
        comp.set_editor_property("visible_in_scene_capture_only", True)
        comp.set_collision_profile_name("NoCollision")
        if material is not None:
            comp.set_material(0, material)
        else:
            failures.append("%s: no map material for %s" % (a["name"], zone_name))
        _tag(actor, a["name"], "Hospital/Gameplay", MINIMAP_TAG)
        counts["mapPlane"] += 1


# ------------------------------------------------------------------------------------------------ build
def build(zone="Zone1", map_path=""):
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"])
    counts = {k: 0 for k in ("meshes", "decals", "lights", "captures", "fog", "sky", "postProcess", "playerStarts",
                             "mapPlane")}
    failures = []
    _meshes(eas, stage, z, counts, failures)
    _lights(eas, z, counts, failures)
    _captures(eas, z, stage, counts, failures)
    _environment(eas, z, stage, counts, failures)
    _post_process(eas, z, counts, failures)
    _player_starts(eas, z, counts)
    _map_plane(eas, z, zone, counts, failures)
    for f in failures[:50]:
        unreal.log_warning("build_dd_stage_level: " + f)
    counts["failed_settings"] = len(failures)
    if not les.save_current_level():
        raise RuntimeError("could not save " + (map_path or z["level"]))
    return counts
