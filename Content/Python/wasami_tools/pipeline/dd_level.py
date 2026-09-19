"""Dark Deception's hospital: assembles one zone's level from the imported assets (dd_stage) and stage_ue.json — the
placed meshes (the teleport's zones among them, with their own collision), the lights, the reflection captures, the
fog, the sky light, the post process volumes, the player starts, the minimap's map plane, the soul shards, what the
zones' flow names (trigger boxes, blocking and trigger volumes, door breaks, double doors, emitters, zone barriers,
Zone 2's altar and ring piece), Zone 2's lifts, the traps (defibrillators, speed barriers, saw traps), the special shards and their spawn points and the level sequences the flow plays (dd_sequence). Every actor it places carries the tag 'dd', which
a rebuild removes first."""
import json
import math
import os

import unreal

from wasami_tools.pipeline import dd_assets, paths, ue_props

EAL = unreal.EditorAssetLibrary
TAG = "dd"

# The title's level (the original's TitleScreen) and the mode its World Settings set.
TITLE_LEVEL = "/Game/Stage/Maps/L_Title"
TITLE_GAME_MODE = "/Script/wasami_deception.WasamiTitleGameMode"

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
# Zone 2's plane (BP_MapTexture_MultiFloor → AWasamiMapTextureMultiFloor) puts on itself the map of the floor the player
# is in: its Map takes each BP_MapArea (→ AWasamiMapArea, a box over a floor, its root's scale its size) to a texture.
MULTI_FLOOR_CLASS = "BP_MapTexture_MultiFloor_C"
MAP_AREA_CLASS = "BP_MapArea_C"
MAP_AREA_TAG = "dd_map_area"
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
# The double doors (BP_06_DoubleDoors → AWasamiDoubleDoors), all of them: Zone 1's 62 and Zone 2's one. The flow names
# three by their tags: Zone 1's lift doors and the tunnel's, and Zone 2's, locked until the ring piece is taken (the way
# to the garage).
DOUBLE_DOORS_CLASS = "BP_06_DoubleDoors_C"
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
# The defibrillators (BP_06_Defib → AWasamiDefib): Zone 1's 23 and Zone 2's 13, which the flow does not name. The class
# leaves its two stands' mesh unset (hospital_defibrillator_01 on both, as BP_06_Defib's SCS templates, each with the
# mesh's own material); it loads its sparks, sounds and shake when play begins, and no placed one has values of its own.
DEFIB_CLASS = "BP_06_Defib_C"
DEFIB_MESH = "/Game/Meshes/06_Hospital/hospital_defibrillator_01"
DEFIB_STANDS = ("defibrillator01", "defibrillator02")
# The speed barriers (BP_SpeedBarrier → AWasamiSpeedBarrier): Zone 1's four, which the flow does not name. The class
# leaves its planes' materials unset (BP_SpeedBarrier's SCS templates' OverrideMaterials, as the zone barrier's). Three
# of the four move, turn or scale their planes (SPEED_BARRIER_PLANES' RelativeLocation, RelativeRotation and
# RelativeScale3D in the level export; the root's scale is the actor's). The light is a component of it, so the lights
# the preprocessing lists under a speed barrier are not placed on their own (builds before 2026-09-19 did, into
# SPEED_BARRIER_LIGHT_FOLDER; place_flow takes those out).
SPEED_BARRIER_CLASS = "BP_SpeedBarrier_C"
SPEED_BARRIER_MATERIALS = {"static_mesh1": "/Game/DD/Materials/Shared/MM_SpeedBarrier_Inst",
                           "static_mesh": "/Game/DD/Materials/Shared/MM_SpeedBarrier_Inst2"}
SPEED_BARRIER_PLANES = {"StaticMesh": "static_mesh", "StaticMesh1": "static_mesh1"}
SPEED_BARRIER_LIGHT_FOLDER = "Hospital/Lights/" + SPEED_BARRIER_CLASS
# The saw traps (BP_06_sawTrap_medium, _short01, _short02 and _long01 → AWasamiSawTrap and its subclasses): Zone 2's 74,
# which the flow does not name. The classes load their meshes, animations and sound, and their construction puts the
# box on the blade and picks the whine's pitch; the placed ones' only values of their own are five short01s' lights
# turned down (SAW_TRAP_LIGHT's Intensity in the level export). The short01's light is a component of it, so the lights
# the preprocessing lists under one are not placed on their own (builds before 2026-09-19 did, into
# SAW_TRAP_LIGHT_FOLDER; place_flow takes those out).
SAW_TRAP_CLASSES = {"BP_06_sawTrap_medium_C": "WasamiSawTrap", "BP_06_sawTrap_short01_C": "WasamiSawTrapShort01",
                    "BP_06_sawTrap_short02_C": "WasamiSawTrapShort02", "BP_06_sawTrap_long01_C": "WasamiSawTrapLong01"}
SAW_TRAP_LIGHT_CLASS = "BP_06_sawTrap_short01_C"
SAW_TRAP_LIGHT = "PointLight"
SAW_TRAP_LIGHT_FOLDER = "Hospital/Lights/" + SAW_TRAP_LIGHT_CLASS
TRAP_FOLDER = "Hospital/Gameplay/Traps"
# The special shards (BP_PowerOrb and BP_BonusShard → AWasamiPowerOrb and AWasamiBonusShard): one of each in either zone,
# off the map until its first flicker, and the places they move to (BP_PowerOrbSpawnPoint and BP_BonusShardSpawnPoint →
# AWasamiPowerOrbSpawnPoint and AWasamiBonusShardSpawnPoint: Zone 1's 11 and 10, Zone 2's 10 and 10), which the flow
# does not name. The classes hold everything. A placed one's Spawn Points (which the preprocessing leaves out) holds every
# point of its kind in its zone, which is what the class takes when SpawnPoints is left empty; its only other value of
# its own is Zone 2's bonus shard's ID (SPECIAL_SHARD_PROPS). The light is a component of it, so the lights the
# preprocessing lists under one are not placed on their own (builds before 2026-09-19 did, into
# SPECIAL_SHARD_LIGHT_FOLDERS; place_flow takes those out). No MINIMAP_TAG: the player's MinimapActorClasses shows them.
SPECIAL_SHARD_CLASSES = {"BP_PowerOrb_C": "WasamiPowerOrb", "BP_BonusShard_C": "WasamiBonusShard"}
SPECIAL_SPAWN_POINT_CLASSES = {"BP_PowerOrbSpawnPoint_C": "WasamiPowerOrbSpawnPoint",
                               "BP_BonusShardSpawnPoint_C": "WasamiBonusShardSpawnPoint"}
SPECIAL_SHARD_PROPS = {"ID": "id"}
SPECIAL_SHARD_LIGHT_FOLDERS = tuple("Hospital/Lights/" + c for c in SPECIAL_SHARD_CLASSES)
SPECIAL_SHARD_FOLDER = "Hospital/Gameplay/SpecialShards"
# Zone 2's altar (BP_01_Statue → AWasamiRingStatue) and the ring piece over it (BP_08_RingPiece_NoPickup →
# AWasamiRingPiece), with what their classes leave unset: the altar's mesh with the materials the placed one puts on it
# (its StaticMeshComponent0's OverrideMaterials in the level export, else the class's), and the piece's mesh with
# BP_08_RingPiece's StaticMesh's OverrideMaterials and its glow (P_08_RingPiece, import_dd_gimmicks). The piece's light
# is a component of it, so the light the preprocessing lists under the piece is not placed on its own (builds before
# 2026-09-19 did, into RING_PIECE_LIGHT_FOLDER; place_flow takes it out). The orb on the altar is a mesh of the level.
STATUE_CLASS = "BP_01_Statue_C"
STATUE_MESH = "/Game/Meshes/00_Ballroom/ring_statue"
STATUE_MATERIALS = ("/Game/Materials/00_Ballroom/MM_00_Ballroom_Ring_Altar_Metal",)
STATUE_SKIPPED_PROPS = ("bCanBeInCluster",)
RING_PIECE_CLASS = "BP_08_RingPiece_NoPickup_C"
RING_PIECE_MESH = "/Game/Meshes/Ring_Assets/ring_pieces/ring_piece06"
RING_PIECE_MATERIALS = ("/Game/Meshes/Ring_Assets/ring_pieces/M_ring_metal", "/Game/Meshes/Ring_Assets/ring_pieces/M_ring_metal2")
RING_PIECE_PARTICLE = "/Game/DD/Particles/08_BearHouse/P_08_RingPiece"
RING_PIECE_LIGHT_FOLDER = "Hospital/Lights/" + RING_PIECE_CLASS
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
# The garage lifts: BP_06_GarageLift (Zone 2's two → AWasamiGarageLift) and BP_06_GarageLift_Zone1_Special (Zone 1's
# car park → AWasamiGarageLiftZone1Special). The C++ classes hold everything (the skeletal mesh is a soft reference
# they load), and no placed one has values of its own.
GARAGE_LIFT_CLASSES = {"BP_06_GarageLift_C": "WasamiGarageLift",
                       "BP_06_GarageLift_Zone1_Special_C": "WasamiGarageLiftZone1Special"}
LIFT_FOLDER = "Hospital/Gameplay/Lifts"
# The level's emitters the flow wakes (their ParticleSystemComponent's Activate): Zone 1's burst of concrete as the
# tunnel's doors break in. They keep the original's bAutoActivate (false) and template.
FLOW_EMITTERS = ("Fracture_concrete_5",)
# The level's target points: where the zones spawn their nurses (NurseSpawn_1, 06_NurseSpawn, ...), and the other
# places the level Blueprints name (DoorLocation, the sounds' and cameras' places). Every one is placed.
TARGET_POINT_CLASS = "TargetPoint"
VOLUME_CLASSES = {"BlockingVolume": unreal.BlockingVolume, "TriggerVolume": unreal.TriggerVolume,
                  "NavMeshBoundsVolume": unreal.NavMeshBoundsVolume, "NavModifierVolume": unreal.NavModifierVolume}
# The navigation's volumes: the original's bounds where the navmesh is made, and (Zone 2) its modifiers, each with the
# class's own area (NavArea_Null: the level writes no AreaClass). The navmesh's settings are the project's
# (Config/DefaultEngine.ini's RecastNavMesh and NavigationSystemV1, as the original's).
NAV_VOLUME_CLASSES = ("NavMeshBoundsVolume", "NavModifierVolume")
NAV_FOLDER = "Hospital/Navigation"
# Zone 2's sentries (BP_06_ReaperNurse_Sentry → AWasamiEnemySentry): the nurses up high in the miniboss's corridor, each
# with its own CanSpawn and Offset (when its view cone first looks) and the place it jumps down to (its Jump Down Spot's,
# relative to the capsule, from the level export). Initial Yaw is the construction script's, which nothing reads.
SENTRY_CLASS = "BP_06_ReaperNurse_Sentry_C"
SENTRY_PROPS = {"CanSpawn": "can_spawn", "Offset": "offset"}
SENTRY_SKIPPED_PROPS = ("Initial Yaw", "SpawnCollisionHandlingMethod")
# Zone 2's Matron (BP_06_Matron_MiniBoss → AWasamiMatron) over the same corridor, and her two view cones the level places
# (→ AWasamiViewconeMatronLong / _Short). The placed ones move their parts (from the level export): the Matron her
# SkeletalMesh (its location: the boss keeps AWasamiMatron's MeshScale) and her CloseArea, each cone its Plane (the map's
# fan). Her Long Cone and Short Cone are the level's references to the cones, set by the cones' names.
MATRON_CLASS = "BP_06_Matron_MiniBoss_C"
MATRON_PARTS = {"SkeletalMesh": "skeletal_mesh", "CloseArea": "close_area"}
MATRON_CONES = {"Long Cone": "long_cone", "Short Cone": "short_cone"}
MATRON_CONE_CLASSES = {"BP_06_Miniboss_viewcone_Matron_Long_C": "WasamiViewconeMatronLong",
                       "BP_06_Miniboss_viewcone_Matron_Short_C": "WasamiViewconeMatronShort"}
ENEMY_FOLDER = "Hospital/Gameplay/Enemies"
DEFAULT_BRUSH_BOX = [-100.0, -100.0, -100.0, 100.0, 100.0, 100.0]
FLOW_TAG = "dd_flow"
FLOW_FOLDER = "Hospital/Gameplay/Flow"
# This game's garage portal (AWasamiPortal, after BP_00_Teleport), which the original's hospital does not have: the zone
# leaves by it instead of the ambulance's ride to the boss fight (item 13). Placed as the hotel's exit is (locked,
# masked, at 2.5 times its size), turned so its front faces the way the player comes: its +X, the side the logo and the
# lock lie on over the disc and the hotel's exit's end trigger is on (its strobing light is behind it). It stands in the
# mouth of the tunnel the ambulance would drive into, 600 cm past the ambulance's lift, in the middle of the tunnel's
# 2000 cm (x -11338 to -9356; the ceiling at 1000). The zone's flow finds it by its name, and leaves by the trigger by
# it (its "trigger"): the hotel's EndTrigger by its exit, a TriggerVolume (the default brush, ±100 cm, scaled
# (1, 2.376, 1)) PORTAL_TRIGGER_OFFSET from the portal in the portal's frame (neither is turned), placed the same way from
# this portal as an AWasamiTriggerBox (its box's half extent TRIGGER_BOX_EXTENT, scaled to the same size).
PORTALS = {"06_Hospital_Zone_02": {"name": "Wasami_GaragePortal", "location": (-10347.0, -7700.0, 0.0), "yaw": 90.0,
                                   "scale": 2.5, "trigger": "Wasami_EscapeTrigger"}}
PORTAL_TRIGGER_OFFSET = (40.643310546875, 10.24072265625, 165.11831665039062)
PORTAL_TRIGGER_EXTENT = (100.0, 237.63628005981445, 100.0)
TRIGGER_BOX_EXTENT = 32.0

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


def _navigable_volumes():
    """(bounds volumes with navmesh in them, bounds volumes) in the level open in the editor."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    volumes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.NavMeshBoundsVolume)
    found = 0
    for volume in volumes:
        origin, extent = volume.get_actor_bounds(False)
        found += unreal.NavigationSystemV1.get_random_location_in_navigable_radius(
            world, origin, max(extent.x, extent.y)) is not None
    return found, len(volumes)


def _save_level(les, map_path):
    """Saves the level open in the editor. Opening a level empties its navmesh and builds it again over the next ticks,
    and a game plays the saved navmesh as it is (DynamicModifiersOnly), so a level opened and saved in the same call
    has no paths in the game: warns to call build_navigation afterwards."""
    if not les.save_current_level():
        raise RuntimeError("could not save " + map_path)
    found, volumes = _navigable_volumes()
    if found < volumes:
        unreal.log_warning("%s is saved with navigation in %d of %d bounds volumes: call WasamiStageTools.build_navigation"
                           " in a call of its own" % (map_path, found, volumes))


def build_navigation(map_path=""):
    """Builds the navigation of the level open in the editor (RebuildNavigation: the whole navmesh, synchronously) and
    saves the level when every bounds volume has paths. A level opened in the same call is locked against building
    (ENavigationBuildLock::AsyncLoadLock, released some seconds later once the level's assets are ready), so a map_path
    other than the open level is only opened: call again a few seconds later."""
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    open_path = ues.get_editor_world().get_outermost().get_name()
    if map_path and map_path != open_path:
        _open_level(map_path, clear=False)
        return {"opened": 1, "built": 0, "saved": 0, "volumes": 0, "navigable": 0}
    unreal.SystemLibrary.execute_console_command(ues.get_editor_world(), "RebuildNavigation")
    found, volumes = _navigable_volumes()
    result = {"opened": 0, "built": int(found == volumes and volumes > 0), "saved": 0, "volumes": volumes,
              "navigable": found}
    if result["built"]:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if not les.save_current_level():
            raise RuntimeError("could not save " + open_path)
        result["saved"] = 1
    else:
        unreal.log_warning("build_navigation: paths in %d of %d bounds volumes of %s (a level opened within the last few"
                           " seconds cannot build yet: call again later); not saved" % (found, volumes, open_path))
    return result


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
            if lt["actorClass"] in (SHARD_CLASS, BARRIER_CLASS, RING_PIECE_CLASS, SPEED_BARRIER_CLASS,
                                    SAW_TRAP_LIGHT_CLASS) or lt["actorClass"] in SPECIAL_SHARD_CLASSES:
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
    areas = {}
    for a in zone["actors"]:
        if a["class"] != MAP_AREA_CLASS or not a["world"]:
            continue
        actor = eas.spawn_actor_from_class(unreal.WasamiMapArea, _vec(a["world"]["location"]), _rot(a["world"]["quat_xyzw"]))
        actor.set_actor_scale3d(_vec(a["world"]["scale"]))
        if a["props"]:
            failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
        _tag(actor, a["name"], "Hospital/Gameplay", MAP_AREA_TAG)
        areas[a["name"]] = actor
        counts["mapAreas"] += 1
    for a in zone["actors"]:
        if a["class"] not in MAP_PLANE_CLASSES or not a["world"]:
            continue
        if a["class"] == MULTI_FLOOR_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiMapTextureMultiFloor, _vec(a["world"]["location"]),
                                               _rot(a["world"]["quat_xyzw"]))
            actor.static_mesh_component.set_static_mesh(mesh)
            floors = {}
            for area, texture in (a["props"].get("Map") or {}).items():
                target = dd_assets.asset_path(dd_assets.game_rel(texture))
                if area not in areas or not EAL.does_asset_exist(target):
                    failures.append("%s: no %s or %s (run WasamiDDTools.import_dd_tablet)" % (a["name"], area, target))
                    continue
                floors[areas[area]] = unreal.load_asset(target)
            actor.set_editor_property("map", floors)
        else:
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


def place_minimap(zone="Zone1", map_path=""):
    """Puts the zone's map plane and its floor boxes in again, leaving the rest of the level as it is, and saves the
    level. Zone 2's plane is movable and its boxes have no light, so its baked lighting stays valid; Zone 1's plane is
    static, and a new one has no baked lighting until the level's is built again (the capture reads only its base colour,
    but the editor counts it as unbuilt)."""
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"], clear=False)
    old = [a for a in eas.get_all_level_actors() if a.actor_has_tag(MINIMAP_TAG) or a.actor_has_tag(MAP_AREA_TAG)]
    counts = {"removed": len(old), "mapPlane": 0, "mapAreas": 0}
    if old:
        eas.destroy_actors(old)
    failures = []
    _map_plane(eas, z, zone, counts, failures)
    for f in failures:
        unreal.log_warning("place_dd_minimap: " + f)
    counts["failed_settings"] = len(failures)
    _save_level(les, map_path or z["level"])
    return counts


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
    _save_level(les, map_path or z["level"])
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


def _set_mesh(comp, stage, source, materials=None):
    """The stage's mesh made from the original's (a Blueprint component's, which the placements leave out) on comp,
    with the mesh's own materials, or these (the component's OverrideMaterials, slot by slot)."""
    info = stage["meshes"].get(source)
    mesh = unreal.load_asset(info["asset"]) if info else None
    if mesh is None:
        raise RuntimeError("missing mesh %s: run WasamiStageTools.import_dd_stage_assets until nothing remains" % source)
    comp.set_static_mesh(mesh)
    for i, slot in enumerate(materials or info["slots"]):
        m = stage["materials"].get(slot.rsplit(".", 1)[0]) if slot else None
        material = unreal.load_asset(m["asset"]) if m else None
        if material is None:
            raise RuntimeError("missing material %s of %s" % (slot, source))
        comp.set_material(i, material)


def level_file(zone):
    """The zone's whole level export (pak_reference_2/_levels/<map>.full.json: every object's path and values)."""
    return os.path.join(paths.DD_PAK2, "_levels", zone["map"] + ".full.json")


def _level_props(zone, path, level):
    """The props of the zone's level export object <map>.PersistentLevel.<path> ({} when it has none). level: the export
    by path, or {} to have it read (level_file)."""
    if not level:
        with open(level_file(zone), encoding="utf-8") as f:
            level.update({e["path"]: e for e in json.load(f)})
    return level.get("%s.PersistentLevel.%s" % (zone["map"], path), {}).get("props", {})


def set_sentry(actor, zone, a, level):
    """A sentry placed from the original's (a: its stage entry): its own values (SENTRY_PROPS) and its Jump Down Spot's
    relative location. Returns the names of its own values that are not written."""
    for key, name in SENTRY_PROPS.items():
        if key in a["props"]:
            actor.set_editor_property(name, a["props"][key])
    spot = _level_props(zone, a["name"] + ".Jump Down Spot", level).get("RelativeLocation")
    if spot is not None:
        actor.get_editor_property("jump_down_spot").set_editor_property("relative_location", _vec(spot))
    return sorted(set(a["props"]) - set(SENTRY_PROPS) - set(SENTRY_SKIPPED_PROPS))


def _set_part(comp, props):
    """A part moved as the level export's props for it do: RelativeLocation, RelativeRotation, RelativeScale3D."""
    if "RelativeLocation" in props:
        comp.set_editor_property("relative_location", _vec(props["RelativeLocation"]))
    if "RelativeRotation" in props:
        pitch, yaw, roll = (float(v) for v in props["RelativeRotation"])
        comp.set_editor_property("relative_rotation", unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
    if "RelativeScale3D" in props:
        comp.set_editor_property("relative_scale3d", _vec(props["RelativeScale3D"]))


def set_matron(actor, zone, name, level):
    """The Matron placed from the original's of that name: her SkeletalMesh and CloseArea where the placed one has them
    (MATRON_PARTS). Returns the level's names of her cones by her property (MATRON_CONES)."""
    for part, prop in MATRON_PARTS.items():
        _set_part(actor.get_editor_property(prop), _level_props(zone, "%s.%s" % (name, part), level))
    own = _level_props(zone, name, level)
    return {prop: own[key].rsplit(".", 1)[-1] for key, prop in MATRON_CONES.items() if own.get(key)}


def set_matron_cone(actor, zone, name, level):
    """One of the Matron's view cones placed from the original's of that name: its Plane (the map's fan) as the placed
    one's."""
    _set_part(actor.get_editor_property("plane"), _level_props(zone, name + ".Plane", level))


def set_emitter(actor, zone, name, level):
    """An Emitter placed from the original's of that name, set up as its ParticleSystemComponent is: bAutoActivate and
    the template (imported under /Game/DD). level: the zone's level export by path (level_file), or {} to have it read.
    Returns the template's asset path when it is not imported yet, else None."""
    props = _level_props(zone, name + ".ParticleSystemComponent0", level)
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


def set_speed_barrier(actor, zone, name, level):
    """A speed barrier placed from the original's of that name: its planes' materials (SPEED_BARRIER_MATERIALS) and
    where the placed one moves, turns or scales them (SPEED_BARRIER_PLANES)."""
    for prop, path in SPEED_BARRIER_MATERIALS.items():
        material = unreal.load_asset(path)
        if material is None:
            raise RuntimeError("missing %s: run WasamiDDTools.import_dd_gimmicks" % path)
        actor.get_editor_property(prop).set_material(0, material)
    for part, prop in SPEED_BARRIER_PLANES.items():
        _set_part(actor.get_editor_property(prop), _level_props(zone, "%s.%s" % (name, part), level))


def set_saw_trap_light(actor, zone, name, level):
    """A short01 saw trap placed from the original's of that name: its light's Intensity where the placed one sets it."""
    intensity = _level_props(zone, "%s.%s" % (name, SAW_TRAP_LIGHT), level).get("Intensity")
    if intensity is not None:
        actor.get_editor_property("point_light").set_editor_property("intensity", float(intensity))


def _flow(eas, stage, zone, counts, failures):
    """The trigger boxes, brush volumes (the navigation's too), target points, door breaks, the double doors, the emitters
    the flow names, the zone barriers, the zone shard checkers, the lifts, the garage lifts, the sentries, the Matron
    with her view cones (her references to them set), the altar, the ring piece, the defibrillators, the speed barriers, the saw traps and the special shards with their spawn points, each where the original has it, and fixed to what it moves with (an ambulance, the spikes) when that
    is in the level; and this game's garage portal and the trigger by it (PORTALS)."""
    placed = []
    level = {}
    cones = {}
    for a in zone["actors"]:
        doors = a["class"] == DOUBLE_DOORS_CLASS
        emitter = a["class"] == "Emitter" and a["name"] in FLOW_EMITTERS
        special = a["class"] in SPECIAL_SHARD_CLASSES or a["class"] in SPECIAL_SPAWN_POINT_CLASSES
        enemy = a["class"] in (SENTRY_CLASS, MATRON_CLASS) or a["class"] in MATRON_CONE_CLASSES
        if not a["world"] or (a["class"] not in (TRIGGER_CLASS, DOOR_BREAK_CLASS, BARRIER_CLASS, SHARD_CHECKER_CLASS,
                                                 TARGET_POINT_CLASS, STATUE_CLASS, RING_PIECE_CLASS,
                                                 DEFIB_CLASS, SPEED_BARRIER_CLASS)
                              and a["class"] not in VOLUME_CLASSES and a["class"] not in LIFT_CLASSES
                              and a["class"] not in GARAGE_LIFT_CLASSES and a["class"] not in SAW_TRAP_CLASSES
                              and not doors and not emitter and not special and not enemy):
            continue
        world = a["world"]
        if a["class"] == TRIGGER_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiTriggerBox, _vec(world["location"]), _rot(world["quat_xyzw"]))
            actor.set_editor_property("end_overlap", bool(a["props"].get("EndOverlap")))
            counts["triggers"] += 1
        elif a["class"] == TARGET_POINT_CLASS:
            actor = eas.spawn_actor_from_class(unreal.TargetPoint, _vec(world["location"]), _rot(world["quat_xyzw"]))
            counts["targetPoints"] += 1
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
        elif a["class"] in GARAGE_LIFT_CLASSES:
            actor = eas.spawn_actor_from_class(getattr(unreal, GARAGE_LIFT_CLASSES[a["class"]]), _vec(world["location"]),
                                               _rot(world["quat_xyzw"]))
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["garageLifts"] += 1
        elif a["class"] == SENTRY_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiEnemySentry, _vec(world["location"]),
                                               _rot(world["quat_xyzw"]))
            unwritten = set_sentry(actor, zone, a, level)
            if unwritten:
                failures.append("%s: its own values %s are not written" % (a["name"], unwritten))
            counts["sentries"] += 1
        elif a["class"] == MATRON_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiMatron, _vec(world["location"]), _rot(world["quat_xyzw"]))
            cones[actor] = set_matron(actor, zone, a["name"], level)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["matrons"] += 1
        elif a["class"] in MATRON_CONE_CLASSES:
            actor = eas.spawn_actor_from_class(getattr(unreal, MATRON_CONE_CLASSES[a["class"]]), _vec(world["location"]),
                                               _rot(world["quat_xyzw"]))
            set_matron_cone(actor, zone, a["name"], level)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["viewcones"] += 1
        elif a["class"] == STATUE_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiRingStatue, _vec(world["location"]), _rot(world["quat_xyzw"]))
            over = _level_props(zone, a["name"] + ".StaticMeshComponent0", level).get("OverrideMaterials")
            _set_mesh(actor.static_mesh_component, stage, STATUE_MESH,
                      [m.rsplit(".", 1)[0] for m in over] if over else STATUE_MATERIALS)
            unwritten = sorted(set(a["props"]) - set(STATUE_SKIPPED_PROPS))
            if unwritten:
                failures.append("%s: its own values %s are not written" % (a["name"], unwritten))
            counts["ringStatues"] += 1
        elif a["class"] == RING_PIECE_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiRingPiece, _vec(world["location"]), _rot(world["quat_xyzw"]))
            _set_mesh(actor.get_editor_property("static_mesh"), stage, RING_PIECE_MESH, RING_PIECE_MATERIALS)
            glow = unreal.load_asset(RING_PIECE_PARTICLE)
            if glow is None:
                raise RuntimeError("missing %s: run WasamiDDTools.import_dd_gimmicks" % RING_PIECE_PARTICLE)
            actor.get_editor_property("particle_system").set_editor_property("template", glow)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["ringPieces"] += 1
        elif a["class"] == DEFIB_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiDefib, _vec(world["location"]), _rot(world["quat_xyzw"]))
            for prop in DEFIB_STANDS:
                _set_mesh(actor.get_editor_property(prop), stage, DEFIB_MESH)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["defibs"] += 1
        elif a["class"] == SPEED_BARRIER_CLASS:
            actor = eas.spawn_actor_from_class(unreal.WasamiSpeedBarrier, _vec(world["location"]),
                                               _rot(world["quat_xyzw"]))
            set_speed_barrier(actor, zone, a["name"], level)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["speedBarriers"] += 1
        elif a["class"] in SAW_TRAP_CLASSES:
            actor = eas.spawn_actor_from_class(getattr(unreal, SAW_TRAP_CLASSES[a["class"]]), _vec(world["location"]),
                                               _rot(world["quat_xyzw"]))
            if a["class"] == SAW_TRAP_LIGHT_CLASS:
                set_saw_trap_light(actor, zone, a["name"], level)
            if a["props"]:
                failures.append("%s: its own values %s are not written" % (a["name"], sorted(a["props"])))
            counts["sawTraps"] += 1
        elif special:
            cls = SPECIAL_SHARD_CLASSES.get(a["class"]) or SPECIAL_SPAWN_POINT_CLASSES[a["class"]]
            actor = eas.spawn_actor_from_class(getattr(unreal, cls), _vec(world["location"]), _rot(world["quat_xyzw"]))
            for key, name in SPECIAL_SHARD_PROPS.items():
                if key in a["props"]:
                    actor.set_editor_property(name, a["props"][key])
            unwritten = sorted(set(a["props"]) - set(SPECIAL_SHARD_PROPS))
            if unwritten:
                failures.append("%s: its own values %s are not written" % (a["name"], unwritten))
            counts["specialShards" if a["class"] in SPECIAL_SHARD_CLASSES else "specialSpawnPoints"] += 1
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
            counts["navVolumes" if a["class"] in NAV_VOLUME_CLASSES else "volumes"] += 1
        actor.set_actor_scale3d(_vec(world["scale"]))
        lift = a["class"] in LIFT_CLASSES or a["class"] in GARAGE_LIFT_CLASSES
        folder = (LIFT_FOLDER if lift else NAV_FOLDER if a["class"] in NAV_VOLUME_CLASSES
                  else ENEMY_FOLDER if enemy
                  else TRAP_FOLDER if a["class"] in (DEFIB_CLASS, SPEED_BARRIER_CLASS) or a["class"] in SAW_TRAP_CLASSES
                  else SPECIAL_SHARD_FOLDER if special
                  else FLOW_FOLDER)
        _tag(actor, a["name"], folder, FLOW_TAG, "src:" + a["name"])
        placed.append((actor, a))
    portal = PORTALS.get(zone["map"])
    if portal:
        actor = eas.spawn_actor_from_class(unreal.WasamiPortal, _vec(portal["location"]),
                                           unreal.Rotator(roll=0.0, pitch=0.0, yaw=portal["yaw"]))
        # Each set runs the actor's construction again (PostEditChangeProperty), which puts on the locked look.
        actor.set_editor_property("locked", True)
        actor.set_editor_property("masked_portal_material", True)
        actor.set_actor_scale3d(unreal.Vector(portal["scale"], portal["scale"], portal["scale"]))
        _tag(actor, portal["name"], FLOW_FOLDER, FLOW_TAG, "src:" + portal["name"])
        counts["portals"] += 1
        yaw = math.radians(portal["yaw"])
        (px, py, pz), (x, y, z) = portal["location"], PORTAL_TRIGGER_OFFSET
        trigger = eas.spawn_actor_from_class(
            unreal.WasamiTriggerBox,
            _vec((px + x * math.cos(yaw) - y * math.sin(yaw), py + x * math.sin(yaw) + y * math.cos(yaw), pz + z)),
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=portal["yaw"]))
        trigger.set_actor_scale3d(_vec([e / TRIGGER_BOX_EXTENT for e in PORTAL_TRIGGER_EXTENT]))
        _tag(trigger, portal["trigger"], FLOW_FOLDER, FLOW_TAG, "src:" + portal["trigger"])
        counts["triggers"] += 1
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
    for actor, refs in cones.items():
        for prop, name in refs.items():
            cone = by_source.get(name)
            if cone is None:
                failures.append("%s: no %s %s" % (actor.get_actor_label(), prop, name))
            else:
                actor.set_editor_property(prop, cone)


def place_flow(zone="Zone1", map_path=""):
    """Puts the zone's trigger boxes, brush volumes (the navigation's too), target points, door breaks, double doors, emitters, zone barriers, zone shard
    checkers, lifts, garage lifts, sentries, the Matron with her view cones, altar, ring piece, defibrillators, speed barriers, saw traps and special shards with their spawn
    points in again (and takes out the barrier, ring piece, speed barrier, saw trap and special shard lights an earlier build placed on their
    own), leaving the rest of the level and its baked lighting as they are (none of them is in the baked lighting: the doors, the lifts, the
    altar, the defibrillators' stands, the saw traps, the special shards and the barriers', the piece's and the traps' lights are movable), and
    saves the level."""
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"], clear=False)
    old = [a for a in eas.get_all_level_actors() if a.actor_has_tag(FLOW_TAG)]
    lights = [a for a in eas.get_all_level_actors()
              if a.actor_has_tag(TAG) and str(a.get_folder_path()) in (BARRIER_LIGHT_FOLDER, RING_PIECE_LIGHT_FOLDER,
                                                                  SPEED_BARRIER_LIGHT_FOLDER, SAW_TRAP_LIGHT_FOLDER)
                                                                  + SPECIAL_SHARD_LIGHT_FOLDERS]
    counts = {"removed": len(old), "removed_lights": len(lights), "triggers": 0, "volumes": 0, "navVolumes": 0,
              "targetPoints": 0, "doorBreaks": 0,
              "doubleDoors": 0, "emitters": 0, "zoneBarriers": 0, "shardCheckers": 0, "lifts": 0, "garageLifts": 0, "sentries": 0,
              "matrons": 0, "viewcones": 0, "ringStatues": 0, "ringPieces": 0, "defibs": 0, "speedBarriers": 0, "sawTraps": 0, "specialShards": 0,
              "specialSpawnPoints": 0, "portals": 0, "attached": 0}
    old += lights
    if old:
        eas.destroy_actors(old)
    failures = []
    _flow(eas, stage, z, counts, failures)
    for f in failures:
        unreal.log_warning("place_dd_flow: " + f)
    counts["failed_settings"] = len(failures)
    _save_level(les, map_path or z["level"])
    return counts


# ------------------------------------------------------------------------------------------------ build
def build(zone="Zone1", map_path=""):
    stage = paths.load_dd_stage()
    if zone not in stage["zones"]:
        raise ValueError("no zone %r in the stage data (have %s)" % (zone, ", ".join(stage["zones"])))
    z = stage["zones"][zone]
    les, eas = _open_level(map_path or z["level"])
    counts = {k: 0 for k in ("meshes", "decals", "lights", "captures", "fog", "sky", "postProcess", "playerStarts",
                             "mapPlane", "mapAreas", "shards", "triggers", "volumes", "navVolumes", "targetPoints", "doorBreaks",
                             "doubleDoors", "emitters", "zoneBarriers",
                             "shardCheckers", "lifts", "garageLifts", "sentries", "matrons", "viewcones", "ringStatues", "ringPieces", "defibs",
                             "speedBarriers", "sawTraps", "specialShards", "specialSpawnPoints", "portals", "attached")}
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
    _save_level(les, map_path or z["level"])
    return counts


# ------------------------------------------------------------------------------------------------ the title
def build_title(map_path=""):
    """The title's level: the original's TitleScreen holds nothing but its level Blueprint (which shows UMG_TitleScreen),
    so an empty level whose World Settings' GameMode Override is WasamiTitleGameMode, which does the Blueprint's part.
    Made when missing, and saved; the level open before is opened again."""
    path = map_path or TITLE_LEVEL
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    before = ues.get_editor_world().get_outermost().get_name()
    created = not EAL.does_asset_exist(path)
    les, eas = _open_level(path, clear=False)
    world = ues.get_editor_world()
    mode = unreal.load_class(None, TITLE_GAME_MODE)
    if mode is None:
        raise RuntimeError("no class " + TITLE_GAME_MODE + " (build the C++ module first)")
    settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)
    if not settings:
        raise RuntimeError("no World Settings in " + path)
    settings[0].set_editor_property("default_game_mode", mode)
    if not les.save_current_level():
        raise RuntimeError("could not save " + path)
    result = {"created": int(created), "actors": len(eas.get_all_level_actors()), "gameMode": 1}
    if before != path and EAL.does_asset_exist(before):
        les.load_level(before)
    return result
