"""Dark Deception's hospital: assembles one zone's level from the imported assets (dd_stage) and stage_ue.json — the
placed meshes (the teleport's zones among them, with their own collision), the lights, the reflection captures, the
fog, the sky light, the post process volumes, the player starts, the minimap's map plane, the soul shards, what the
zones' flow names (trigger boxes, blocking and trigger volumes, door breaks, double doors, emitters, zone barriers),
Zone 2's lifts and the level sequences the flow plays (dd_sequence). Every actor it places carries the tag 'dd', which
a rebuild removes first."""
import json
import os

import unreal

from wasami_tools.pipeline import dd_assets, paths, ue_props

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
# The soul shards (BP_Shard): an AWasamiShard where the original places each. A shard's light is a component of it, so
# the lights the preprocessing lists under a shard are not placed on their own (builds before 2026-09-17 did, into
# SHARD_LIGHT_FOLDER). Its components are movable, so placing shards leaves the baked lighting as it is.
SHARD_CLASS = "BP_Shard_C"
SHARD_TAG = "dd_shard"
SHARD_FOLDER = "Hospital/Gameplay/Shards"
SHARD_LIGHT_FOLDER = "Hospital/Lights/" + SHARD_CLASS
# What the zones' flow (the original's level Blueprints, AWasamiZoneFlow here) names: the trigger boxes
# (BP_TriggerBox_Base → AWasamiTriggerBox) and the brush volumes it switches or listens to. The flow finds each by the
# tag 'src:<the original's name>'. Every brush in the hospital is the default 200 cm cube, which is what UE's box volume
# factory makes, so the actor's scale is the whole of its size.
TRIGGER_CLASS = "BP_TriggerBox_Base_C"
# The door breaks (BP_06_Hospital_DoorBreak → AWasamiDoorBreak), with their Progress Speed.
DOOR_BREAK_CLASS = "BP_06_Hospital_DoorBreak_C"
# The double doors (BP_06_DoubleDoors → AWasamiDoubleDoors) the flow names: Zone 1's lift doors and the tunnel's. The
# zones' others (Zone 1's 60, Zone 2's one) come with the work list's items 8 and 13.
DOUBLE_DOORS_CLASS = "BP_06_DoubleDoors_C"
FLOW_DOUBLE_DOORS = ("BP_06_DoubleDoors11", "BP_06_DoubleDoors33_36")
# The class's door components and their meshes (BP_06_DoubleDoors' SCS templates), which the C++ class leaves unset (it
# loads nothing from /Game/DD in its constructor); each takes its mesh's own materials, as in the original.
DOUBLE_DOOR_MESHES = {"static_mesh": "/Game/Meshes/06_Hospital/hospital_entrance_walkway_doubledoor2",
                      "static_mesh1": "/Game/Meshes/06_Hospital/hospital_entrance_walkway_doubledoor1"}
# A placed door's own values in the export → the class's properties.
DOUBLE_DOOR_PROPS = {"bLocked": "locked", "Open Amount": "open_amount"}
# The zone barriers (BP_ZoneBarrier → AWasamiZoneBarrier), with their planes' materials (the SCS templates'
# OverrideMaterials; the class leaves them unset, as the doors' meshes). The barrier's light is a component of it, so the
# lights the preprocessing lists under a barrier are not placed on their own (builds before 2026-09-18 did, into
# BARRIER_LIGHT_FOLDER; place_flow takes those out).
BARRIER_CLASS = "BP_ZoneBarrier_C"
BARRIER_MATERIALS = {"static_mesh1": "/Game/DD/Materials/Shared/MM_ZoneBarrier_Inst1",
                     "static_mesh": "/Game/DD/Materials/Shared/MM_ZoneBarrier_Inst2"}
BARRIER_LIGHT_FOLDER = "Hospital/Lights/" + BARRIER_CLASS
# The zone shard checkers (BP_ZoneShardChecker → AWasamiZoneShardChecker): the box over each zone that the tablet's arrow
# points at the shards of. Their root's scale is the box's size.
SHARD_CHECKER_CLASS = "BP_ZoneShardChecker_C"
# Zone 2's lifts: BP_06_Lift_03 and _04 (BP_06_Lift, a BP_06_LiftBase → AWasamiLift) and BP_06_LiftBase_Corner
# (→ AWasamiCornerLift). The C++ classes hold BP_06_LiftBase's box sizes; each class's own mesh (LiftMesh) and, where the
# class changes them, LiftCollision's and LiftCollision1's scale (the children's ICH templates) are written here. The
# mesh takes its own material, as in the original.
LIFT_MESHES = "/Game/Meshes/06_Hospital/hospital_zone_02_lifts_lift_"
LIFT_CLASSES = {
    "BP_06_Lift_03_C": ("WasamiLift", LIFT_MESHES + "03", (4.552351474761963, 9.153595924377441, 1.1901235580444336),
                        (4.552350997924805, 10.505670547485352, 0.6493702530860901)),
    "BP_06_Lift_04_C": ("WasamiLift", LIFT_MESHES + "04", (8.068293571472168, 4.54220724105835, 1.1901235580444336),
                        (8.60695743560791, 4.586122035980225, 1.1901240348815918)),
    "BP_06_LiftBase_Corner_C": ("WasamiCornerLift", LIFT_MESHES + "01", None, None),
}
LIFT_FOLDER = "Hospital/Gameplay/Lifts"
# The level's emitters the flow wakes (their ParticleSystemComponent's Activate): Zone 1's burst of concrete as the
# tunnel's doors break in. They keep the original's bAutoActivate (false) and template.
FLOW_EMITTERS = ("Fracture_concrete_5",)
VOLUME_CLASSES = {"BlockingVolume": unreal.BlockingVolume, "TriggerVolume": unreal.TriggerVolume}
DEFAULT_BRUSH_BOX = [-100.0, -100.0, -100.0, 100.0, 100.0, 100.0]
FLOW_TAG = "dd_flow"
FLOW_FOLDER = "Hospital/Gameplay/Flow"

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
    """The label, the folder, and the tags: 'dd', the class's own (the double doors' 'interact'), then these."""
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    own = [t for t in actor.tags if str(t) != TAG]
    actor.tags = [unreal.Name(TAG)] + own + [unreal.Name(t) for t in tags if t]


def _open_level(map_path, clear=True):
    """Opens the level (made when missing) and, with clear, removes what an earlier build placed."""
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if EAL.does_asset_exist(map_path):
        if not les.load_level(map_path):
            raise RuntimeError("could not open " + map_path)
    elif not les.new_level(map_path):
        raise RuntimeError("could not create " + map_path)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    old = [a for a in eas.get_all_level_actors() if a.actor_has_tag(TAG)] if clear else []
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
            if lt["actorClass"] in (SHARD_CLASS, BARRIER_CLASS):
                continue
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
        # The game mode finds the checkpoint's start by the original's name (labels are editor-only).
        actor.set_editor_property("player_start_tag", a["name"])
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


# ------------------------------------------------------------------------------------------------ shards
def _shards(eas, zone, counts):
    for a in zone["actors"]:
        if a["class"] != SHARD_CLASS or not a["world"]:
            continue
        actor = eas.spawn_actor_from_class(unreal.WasamiShard, _vec(a["world"]["location"]), _rot(a["world"]["quat_xyzw"]))
        actor.set_actor_scale3d(_vec(a["world"]["scale"]))
        _tag(actor, a["name"], SHARD_FOLDER, SHARD_TAG)
        counts["shards"] += 1


def place_shards(zone="Zone1", map_path=""):
    """Puts the zone's shards in again (and takes out the shard lights an earlier build placed on their own), leaving
    the rest of the level and its baked lighting as they are, and saves the level. Returns what was removed and
    placed."""
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"], clear=False)
    old = [a for a in eas.get_all_level_actors()
           if a.actor_has_tag(SHARD_TAG) or (a.actor_has_tag(TAG) and str(a.get_folder_path()) == SHARD_LIGHT_FOLDER)]
    counts = {"removed_shards": sum(1 for a in old if a.actor_has_tag(SHARD_TAG)), "shards": 0}
    counts["removed_lights"] = len(old) - counts["removed_shards"]
    if old:
        eas.destroy_actors(old)
    _shards(eas, z, counts)
    if not les.save_current_level():
        raise RuntimeError("could not save " + (map_path or z["level"]))
    return counts


# ------------------------------------------------------------------------------------------------ flow
def _set_brush_collision(comp, collision):
    """A volume's brush collision as the level writes it, over the volume class's own profile (InvisibleWall for a
    blocking volume, Trigger for a trigger volume): the profile when named, then the object type, what the body takes
    part in and each listed channel's response (which makes the profile 'Custom')."""
    profile = collision.get("CollisionProfileName")
    if profile and profile != "Custom":
        comp.set_collision_profile_name(profile)
    if collision.get("ObjectType") and comp.get_collision_object_type() != _channel(collision["ObjectType"]):
        comp.set_collision_object_type(_channel(collision["ObjectType"]))   # a named profile's own type stays named
    if collision.get("CollisionEnabled"):
        comp.set_collision_enabled(ue_props.enum_member(unreal.CollisionEnabled, collision["CollisionEnabled"].split("::")[-1]))
    for channel, response in (collision.get("responses") or {}).items():
        comp.set_collision_response_to_channel(_channel("ECC_" + channel),
                                               ue_props.enum_member(unreal.CollisionResponseType, response))


def _set_mesh(comp, stage, source):
    """The stage's mesh made from the original's (a Blueprint component's, which the placements leave out) on comp,
    with the mesh's own materials."""
    info = stage["meshes"].get(source)
    mesh = unreal.load_asset(info["asset"]) if info else None
    if mesh is None:
        raise RuntimeError("missing mesh %s: run WasamiStageTools.import_dd_stage_assets until nothing remains" % source)
    comp.set_static_mesh(mesh)
    for i, slot in enumerate(info["slots"]):
        m = stage["materials"].get(slot.rsplit(".", 1)[0]) if slot else None
        material = unreal.load_asset(m["asset"]) if m else None
        if material is None:
            raise RuntimeError("missing material %s of %s" % (slot, source))
        comp.set_material(i, material)


def level_file(zone):
    """The zone's whole level export (pak_reference_2/_levels/<map>.full.json: every object's path and values)."""
    return os.path.join(paths.DD_PAK2, "_levels", zone["map"] + ".full.json")


def set_emitter(actor, zone, name, level):
    """An Emitter placed from the original's of that name, set up as its ParticleSystemComponent is: bAutoActivate and
    the template (imported under /Game/DD). level: the zone's level export by path (level_file), or {} to have it read.
    Returns the template's asset path when it is not imported yet, else None."""
    if not level:
        with open(level_file(zone), encoding="utf-8") as f:
            level.update({e["path"]: e for e in json.load(f)})
    props = level.get("%s.PersistentLevel.%s.ParticleSystemComponent0" % (zone["map"], name), {}).get("props", {})
    psc = actor.get_editor_property("particle_system_component")
    psc.set_editor_property("auto_activate", bool(props.get("bAutoActivate", True)))
    template = props.get("Template")
    if not template:
        return None
    target = dd_assets.asset_path(dd_assets.game_rel(template))
    if not EAL.does_asset_exist(target):
        return target
    psc.set_editor_property("template", unreal.load_asset(target))
    return None


def _flow(eas, stage, zone, counts, failures):
    """The trigger boxes, brush volumes, door breaks, the double doors and emitters the flow names, the zone barriers,
    the zone shard checkers and the lifts, each where the original has it, and fixed to what it moves with (an
    ambulance, the spikes) when that is in the level."""
    placed = []
    level = {}
    for a in zone["actors"]:
        doors = a["class"] == DOUBLE_DOORS_CLASS and a["name"] in FLOW_DOUBLE_DOORS
        emitter = a["class"] == "Emitter" and a["name"] in FLOW_EMITTERS
        if not a["world"] or (a["class"] not in (TRIGGER_CLASS, DOOR_BREAK_CLASS, BARRIER_CLASS, SHARD_CHECKER_CLASS)
                              and a["class"] not in VOLUME_CLASSES and a["class"] not in LIFT_CLASSES
                              and not doors and not emitter):
            continue
        world = a["world"]
        if a["class"] == TRIGGER_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiTriggerBox, _vec(world["location"]), _rot(world["quat_xyzw"]))
            actor.set_editor_property("end_overlap", bool(a["props"].get("EndOverlap")))
            counts["triggers"] += 1
        elif a["class"] == DOOR_BREAK_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiDoorBreak, _vec(world["location"]), _rot(world["quat_xyzw"]))
            if "Progress Speed" in a["props"]:
                actor.set_editor_property("progress_speed", float(a["props"]["Progress Speed"]))
            counts["doorBreaks"] += 1
        elif doors:
            actor = eas.spawn_actor_from_class(unreal.WasamiDoubleDoors, _vec(world["location"]), _rot(world["quat_xyzw"]))
            for prop, source in DOUBLE_DOOR_MESHES.items():
                _set_mesh(actor.get_editor_property(prop), stage, source)
            for key, name in DOUBLE_DOOR_PROPS.items():
                if key in a["props"]:
                    actor.set_editor_property(name, a["props"][key])
            counts["doubleDoors"] += 1
        elif a["class"] == BARRIER_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiZoneBarrier, _vec(world["location"]), _rot(world["quat_xyzw"]))
            for prop, path in BARRIER_MATERIALS.items():
                material = unreal.load_asset(path)
                if material is None:
                    raise RuntimeError("missing %s: run WasamiDDTools.import_dd_gimmicks" % path)
                actor.get_editor_property(prop).set_material(0, material)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["zoneBarriers"] += 1
        elif a["class"] == SHARD_CHECKER_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiZoneShardChecker, _vec(world["location"]),
                                               _rot(world["quat_xyzw"]))
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["shardCheckers"] += 1
        elif a["class"] in LIFT_CLASSES:
            cls, mesh, collision_scale, top_scale = LIFT_CLASSES[a["class"]]
            actor = eas.spawn_actor_from_class(getattr(unreal, cls), _vec(world["location"]), _rot(world["quat_xyzw"]))
            _set_mesh(actor.get_editor_property("lift_mesh"), stage, mesh)
            if collision_scale:
                actor.get_editor_property("lift_collision").set_relative_scale3d(_vec(collision_scale))
            if top_scale:
                actor.get_editor_property("lift_collision1").set_relative_scale3d(_vec(top_scale))
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["lifts"] += 1
        elif emitter:
            actor = eas.spawn_actor_from_class(unreal.Emitter, _vec(world["location"]), _rot(world["quat_xyzw"]))
            missing = set_emitter(actor, zone, a["name"], level)
            if missing:
                failures.append("%s: no particle system %s (run WasamiDDTools.import_dd_gimmicks)" % (a["name"], missing))
            counts["emitters"] += 1
        else:
            actor = eas.spawn_actor_from_class(VOLUME_CLASSES[a["class"]], _vec(world["location"]), _rot(world["quat_xyzw"]))
            if a.get("brushBox") != DEFAULT_BRUSH_BOX:
                failures.append("%s: brush %s is not the default cube" % (a["name"], a.get("brushBox")))
            comp = actor.get_editor_property("brush_component")
            _set_mobility(comp, {"Mobility": a.get("brushMobility")})
            _set_brush_collision(comp, a.get("brushCollision") or {})
            counts["volumes"] += 1
        actor.set_actor_scale3d(_vec(world["scale"]))
        _tag(actor, a["name"], LIFT_FOLDER if a["class"] in LIFT_CLASSES else FLOW_FOLDER, FLOW_TAG, "src:" + a["name"])
        placed.append((actor, a))
    by_source = {}
    for actor in eas.get_all_level_actors():
        for t in actor.tags:
            if str(t).startswith("src:"):
                by_source.setdefault(str(t)[4:], actor)
    for actor, a in placed:
        parent = by_source.get(a.get("attachParent") or "")
        if parent is not None and parent != actor:
            actor.attach_to_actor(parent, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD,
                                  unreal.AttachmentRule.KEEP_WORLD, False)
            counts["attached"] += 1


def place_flow(zone="Zone1", map_path=""):
    """Puts the zone's trigger boxes, brush volumes, door breaks, double doors, emitters, zone barriers, zone shard
    checkers and lifts in again (and takes out the barrier lights an earlier build placed on their own), leaving the
    rest of the level and its baked lighting as they are (none of them is in the baked lighting: the doors, the lifts
    and the barriers' lights are movable), and saves the level."""
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"], clear=False)
    old = [a for a in eas.get_all_level_actors() if a.actor_has_tag(FLOW_TAG)]
    lights = [a for a in eas.get_all_level_actors()
              if a.actor_has_tag(TAG) and str(a.get_folder_path()) == BARRIER_LIGHT_FOLDER]
    counts = {"removed": len(old), "removed_lights": len(lights), "triggers": 0, "volumes": 0, "doorBreaks": 0,
              "doubleDoors": 0, "emitters": 0, "zoneBarriers": 0, "shardCheckers": 0, "lifts": 0, "attached": 0}
    old += lights
    if old:
        eas.destroy_actors(old)
    failures = []
    _flow(eas, stage, z, counts, failures)
    for f in failures:
        unreal.log_warning("place_dd_flow: " + f)
    counts["failed_settings"] = len(failures)
    if not les.save_current_level():
        raise RuntimeError("could not save " + (map_path or z["level"]))
    return counts


# ------------------------------------------------------------------------------------------------ build
def build(zone="Zone1", map_path=""):
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"])
    counts = {k: 0 for k in ("meshes", "decals", "lights", "captures", "fog", "sky", "postProcess", "playerStarts",
                             "mapPlane", "shards", "triggers", "volumes", "doorBreaks", "doubleDoors", "emitters", "zoneBarriers",
                             "shardCheckers", "lifts", "attached")}
    failures = []
    _meshes(eas, stage, z, counts, failures)
    _lights(eas, z, counts, failures)
    _captures(eas, z, stage, counts, failures)
    _environment(eas, z, stage, counts, failures)
    _post_process(eas, z, counts, failures)
    _player_starts(eas, z, counts)
    _map_plane(eas, z, zone, counts, failures)
    _shards(eas, z, counts)
    _flow(eas, stage, z, counts, failures)
    # Last: a sequence binds the level's actors by their paths, which this build has just made anew.
    from wasami_tools.pipeline import dd_sequence
    sequences = dd_sequence.place_all(eas, zone, z)
    counts["sequences"] = sequences["sequences"]
    counts["sequenceActors"] = sequences["sequence_actors"]
    failures += ["sequence binding without its actor: " + m for m in sequences["missing"]]
    for f in failures[:50]:
        unreal.log_warning("build_dd_stage_level: " + f)
    counts["failed_settings"] = len(failures)
    if not les.save_current_level():
        raise RuntimeError("could not save " + (map_path or z["level"]))
    return counts
