"""Sets up a Blender file for hand-made camera work on the Wasami's capture scenes (AWasamiCapture).

    python Tools/blender/capture_camera_setup.py            # builds SourceArt/Wasami/CaptureCamera/capture_camera.blend
    python Tools/blender/capture_camera_setup.py --open     # builds it if missing, then opens it on the desktop
    python Tools/blender/capture_camera_setup.py --force    # builds it again over the existing file (loses its camera)

One scene per capture (Capture_1 to Capture_3, and the face's Run), laid out as the game's black room lays it out:
the Wasami's body and clips from the game's own import source (Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb,
dd_enemy.prepare()), grown and turned as AWasamiCapture's Body, facing +X, its feet at the origin; the camera where
the game's starts (CameraOffset, or FaceCameraOffset), FOV as the game's (horizontal). Blender's frames are the
game's real time at 30 fps: the scene's early speed-up (SceneTime), the clip's start (ClipStarts), the body's place
(BodyStart, the face's rush) are baked in, so what plays is what the player sees. Before the capture's t = 0 and after
the screen is black there are PAD seconds more (the Wasami held at t = 0 before, going on as the game would after);
they are only room for recording the camera and mean nothing to the game. Markers mark t = 0, the fade and the black.

UE to Blender: x = X / 100, y = -Y / 100, z = Z / 100 (cm to m, Y flipped).
"""

import math
import os
import subprocess
import sys

PROJECT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SOURCE = os.path.join(PROJECT, "Intermediate", "Pipeline", "wasami", "enemy", "WasamiEnemy.glb")
OUT = os.path.join(PROJECT, "SourceArt", "Wasami", "CaptureCamera", "capture_camera.blend")
BLENDER = r"C:\Program Files\Blender Foundation\Blender 4.0\blender.exe"

FPS = 30
PAD = 3.0
PAD_FRAMES = int(round(PAD * FPS))

# WasamiEnemy.h / WasamiCapture.h / WasamiCapture.cpp
MESH_SCALE = 229.05135 / 168.52719
HOTEL_MONKEY = (4737.9833984375, 1072.7213134765625, 6917.716796875)
HOTEL_CAMERA = (4832.646484375, 1075.6009521484375, 7107.3759765625)
HOTEL_LIGHT = (4777.72900390625, 1072.5234375, 7202.3505859375)
MONKEY_TOP = 65.78780364990234 * 4.0
MONKEY_HEAD_BASE = 34.261745931581764 * 4.0
MONKEY_HEAD_TOP = 58.8946292245342 * 4.0
WASAMI_TOP = 170.0 * MESH_SCALE
SCENE_SCALE = WASAMI_TOP / MONKEY_TOP
FRAME_SCALE = WASAMI_TOP / 2.0 / (MONKEY_HEAD_TOP - MONKEY_HEAD_BASE)
LIGHT_COLOR = (255 / 255.0, 236 / 255.0, 142 / 255.0)
FIELD_OF_VIEW = 90.0
WATCHER_FIELD_OF_VIEW = 75.0
FACE_CAMERA_OFFSET = (121.46516418457031, 0.0, 186.0)
FACE_RUSH_DISTANCE = 250.0
FACE_RUSH_TIME = 0.1
WATCHER_ANIM_DELAY = 0.2
FACE_DEATH_DELAY = 0.2 + 0.85 + 0.1
DEATH_DELAY = 3.5
START_RATE = 2.5
RATE_EASE_TIME = 0.4
CLIP_STARTS = (0.0, 0.45, 1.2)
MATINEE_FOR = (0, 2, 1)
# (length, fade start, fade end) of MonkeyJumpscare, 2, 3
MATINEE_FADES = (
    (2.121222972869873, 1.7005259990692139, 1.769968032836914),
    (2.464282989501953, 1.920689582824707, 2.051270008087158),
    (3.068389654159546, 2.6005260944366455, 2.769968032836914),
)
AIM_BONE = "neck_01"

# (scene, clip, is the face)
CAPTURES = (
    ("Capture_1", "A_WasamiEnemy_Capture_1", False),
    ("Capture_2", "A_WasamiEnemy_Capture_2", False),
    ("Capture_3", "A_WasamiEnemy_Capture_3", False),
    ("Face", "A_WasamiEnemy_Run", True),
)

README = """ワサミの捕獲のカメラ（Tools/blender/capture_camera_setup.py が作った）

■ シーン（画面上端のシーンの欄で切り替える）
  Capture_1 … バク転（ホテル型 1）
  Capture_2 … スライディング（ホテル型 2）
  Capture_3 … 歩いて来る（ホテル型 3）
  Face      … 顔型（走って来る。ゲームでは FOV 75）

■ 時間
  30 fps。フレームはゲームの実時間（場面の最初の早回しは焼き込み済み）。
  マーカー「t=0」が捕まった瞬間、「真っ黒」でゲームの画面は終わる。
  その前後 3 秒（90 フレーム）は収録用の余白で、ゲームには入らない。
  前の余白ではワサミは t=0 の姿勢のまま、後ろの余白ではゲームの続きのまま動く。

■ 置き方（ゲームの別室と同じ）
  ワサミの足元が原点、ワサミは +X を向く。1 Blender 単位 = 1 m（UE の 100 cm）。
  カメラ「CaptureCam」はゲームのいまの始まりの位置（t=0 にキー 1 つ）。
  水平の画角はゲームと同じ（ホテル型 90°、顔型 75°。センサーは横合わせ）。

■ カメラの付け方
  ・自動キー挿入（タイムラインの ● ）は入れてある。
  ・3D ビューはカメラ視点で「カメラをビューに固定」が入っている。
    マウスで視点を動かすとカメラが動き、そのフレームにキーが入る。
  ・再生しながら収録するなら Shift+` でウォーク／フライ移動（再生の同期はフレーム落ちで実時間）。
  ・Capture_1 のカメラだけ付ける、なども可。付けたシーンを教えてください。
"""


# ------------------------------------------------------------------------------------------------ outside Blender
def outside():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--open", action="store_true", help="open the file in Blender on the interactive desktop")
    ap.add_argument("--force", action="store_true", help="build again over the existing file (its camera is lost)")
    opts = ap.parse_args()
    if opts.force or not os.path.exists(OUT):
        if not os.path.exists(SOURCE):
            sys.exit("%s is missing: run the enemy's import (dd_enemy.prepare) first" % SOURCE)
        proc = subprocess.run([BLENDER, "-b", "--factory-startup", "--python", os.path.abspath(__file__)])
        if proc.returncode != 0 or not os.path.exists(OUT):
            sys.exit("Blender did not write %s" % OUT)
    else:
        print("%s exists; not built again (--force builds it over)" % OUT)
    if opts.open:
        subprocess.run([sys.executable, os.path.join(PROJECT, "Tools", "console_session.py"), BLENDER, OUT],
                       check=True)


# ------------------------------------------------------------------------------------------------ inside Blender
def ue(v):
    return (v[0] / 100.0, -v[1] / 100.0, v[2] / 100.0)


def scene_time(t):
    return t + (START_RATE - 1.0) * RATE_EASE_TIME * (1.0 - math.exp(-t / RATE_EASE_TIME))


def real_time(scene):
    low, high = 0.0, max(scene, 0.0) / max(min(START_RATE, 1.0), 1e-6)
    for _ in range(60):
        mid = 0.5 * (low + high)
        if scene_time(mid) < scene:
            low = mid
        else:
            high = mid
    return 0.5 * (low + high)


def timing(index, length, face):
    """What the game does over real time t (s): the clip's time, the body's x (m, None: held), the markers."""
    if face:
        def clip(t):
            return math.fmod(scene_time(max(t, 0.0)), length)

        def body_x(t):
            return -FACE_RUSH_DISTANCE / 100.0 * math.exp(-max(t, 0.0) / FACE_RUSH_TIME)

        markers = [(0.0, "t=0 捕まった"), (WATCHER_ANIM_DELAY, "声・カメラの揺れ"),
                   (FACE_DEATH_DELAY, "真っ黒・死亡画面")]
        return clip, body_x, markers, FACE_DEATH_DELAY, 0.0
    matinee_length, fade_start, fade_end = MATINEE_FADES[MATINEE_FOR[index]]
    clip_fade = length * fade_start / matinee_length
    clip_start = min(max(CLIP_STARTS[index], 0.0), clip_fade * 0.9)
    rate = fade_start / (clip_fade - clip_start)
    fade_at = real_time(clip_fade - clip_start)
    black = real_time(fade_end / rate)

    def clip(t):
        return min(clip_start + scene_time(max(t, 0.0)), length)

    markers = [(0.0, "t=0 捕まった・声"), (fade_at, "暗転の始まり"), (black, "真っ黒")]
    if DEATH_DELAY - black < PAD:
        markers.append((DEATH_DELAY, "死亡画面"))
    return clip, None, markers, black, clip_fade


def inside():
    import bpy
    import mathutils

    scene = bpy.context.scene
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)
    scene.render.fps, scene.render.fps_base = FPS, 1.0
    bpy.ops.import_scene.gltf(filepath=SOURCE)
    # The armature and its skinned mesh; the importer's bone shape (a hidden icosphere) stays out of the scenes.
    arm = next(o for o in scene.collection.all_objects if o.type == "ARMATURE")
    imported = [arm] + [o for o in scene.collection.all_objects if o.parent is arm]
    for o in scene.collection.all_objects:
        if o not in imported:
            for collection in list(o.users_collection):
                collection.objects.unlink(o)
    actions = {a.name: a for a in bpy.data.actions}

    def action_of(clip):
        return actions[clip + "_" + arm.name]

    # The placement: the body grown as the enemy's, turned to face +X (headfront lies in front of head).
    placement = bpy.data.objects.new("Wasami", None)
    scene.collection.objects.link(placement)
    placement.empty_display_type = "ARROWS"
    for o in imported:
        if o.parent is None:
            o.parent = placement
    arm.data.pose_position = "REST"
    bpy.context.view_layer.update()
    forward = arm.pose.bones["headfront"].head - arm.pose.bones["head"].head
    placement.rotation_euler.z = -math.atan2(forward.y, forward.x)
    placement.scale = (MESH_SCALE,) * 3
    arm.data.pose_position = "POSE"
    if arm.animation_data:
        for track in list(arm.animation_data.nla_tracks):
            arm.animation_data.nla_tracks.remove(track)
    else:
        arm.animation_data_create()
    for bone in arm.pose.bones:
        bone.location = (0.0, 0.0, 0.0)
        bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        bone.scale = (1.0, 1.0, 1.0)

    def bone_at(action, seconds, bone):
        arm.animation_data.action = action
        frame = action.frame_range[0] + seconds * FPS
        scene.frame_set(int(math.floor(frame)), subframe=frame - math.floor(frame))
        return arm.matrix_world @ arm.pose.bones[bone].head

    # Everything measured on the first scene: the clips' lengths, the body's start, where the camera aims.
    plans = []
    for index, (name, clip_name, face) in enumerate(CAPTURES):
        action = action_of(clip_name)
        length = (action.frame_range[1] - action.frame_range[0]) / FPS
        clip, body_x, markers, black, clip_fade = timing(index, length, face)
        if face:
            body = mathutils.Vector((body_x(0.0), 0.0, 0.0))
        else:
            travel = bone_at(action, clip_fade, "pelvis") - bone_at(action, 0.0, "pelvis")
            body = mathutils.Vector((-travel.x, -travel.y, 0.0))
        aim = bone_at(action, clip(0.0), AIM_BONE) + body
        plans.append(dict(name=name, action=action, length=length, clip=clip, body_x=body_x, body=body,
                          markers=markers, black=black, aim=aim, face=face))
        print("capture %s: clip %.3f s, black at %.3f s, body at (%.3f, %.3f), aim at (%.3f, %.3f, %.3f)"
              % (name, length, black, body.x, body.y, *aim))

    world = bpy.data.worlds.new("CaptureRoom")
    world.color = (0.0, 0.0, 0.0)
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)
    floor_material = bpy.data.materials.new("CaptureFloor")
    floor_material.diffuse_color = (0.02, 0.02, 0.02, 1.0)
    floor_material.use_nodes = True
    floor_material.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.02, 0.02, 0.02, 1.0)
    floor_mesh = bpy.data.meshes.new("CaptureFloor")
    half = 10.0
    floor_mesh.from_pydata([(-half, -half, 0), (half, -half, 0), (half, half, 0), (-half, half, 0)], [],
                           [(0, 1, 2, 3)])
    floor_mesh.materials.append(floor_material)
    light_offset = [(l - m) * SCENE_SCALE for l, m in zip(HOTEL_LIGHT, HOTEL_MONKEY)]
    camera_offset = [h - m for h, m in zip(HOTEL_CAMERA, HOTEL_MONKEY)]
    camera_offset = (camera_offset[0] * FRAME_SCALE, camera_offset[1] * FRAME_SCALE,
                     WASAMI_TOP / 2.0 + (camera_offset[2] - MONKEY_HEAD_BASE) * FRAME_SCALE)

    base_objects = [placement] + imported
    base_names = {o: o.name for o in base_objects}
    for index, plan in enumerate(plans):
        if index == 0:
            target = scene
            target.name = plan["name"]
            objects = {o: o for o in base_objects}
        else:
            target = bpy.data.scenes.new(plan["name"])
            objects = {o: o.copy() for o in base_objects}
            for old, new in objects.items():
                target.collection.objects.link(new)
                if old.parent in objects:
                    new.parent = objects[old.parent]
                for modifier in new.modifiers:
                    if modifier.type == "ARMATURE" and modifier.object in objects:
                        modifier.object = objects[modifier.object]
        body_root, body_arm = objects[placement], objects[arm]
        for old, new in objects.items():
            new.name = "%s_%s" % (plan["name"], base_names[old])

        target.render.fps, target.render.fps_base = FPS, 1.0
        target.render.resolution_x, target.render.resolution_y = 1920, 1080
        target.world = world
        target.sync_mode = "FRAME_DROP"
        target.tool_settings.use_keyframe_insert_auto = True
        game_end = PAD_FRAMES + int(math.ceil(plan["black"] * FPS))
        target.frame_start, target.frame_end = 0, game_end + PAD_FRAMES
        target.frame_current = PAD_FRAMES
        target.timeline_markers.new("余白の終わり", frame=PAD_FRAMES - 1)
        for seconds, label in plan["markers"]:
            frame = PAD_FRAMES + int(round(seconds * FPS))
            if frame <= target.frame_end:
                target.timeline_markers.new("%s (%.2fs)" % (label, seconds), frame=frame)
        target.timeline_markers.new("余白の始まり", frame=game_end + 1)

        # The clip on the game's real time, one key per frame.
        frames = range(target.frame_start, target.frame_end + 1)
        source = plan["action"]
        baked = bpy.data.actions.new("%s_GameTime" % plan["name"])
        for curve in source.fcurves:
            group = curve.group.name if curve.group else ""
            out = baked.fcurves.new(curve.data_path, index=curve.array_index, action_group=group)
            points = []
            for f in frames:
                at = source.frame_range[0] + plan["clip"]((f - PAD_FRAMES) / FPS) * FPS
                points += [float(f), curve.evaluate(at)]
            out.keyframe_points.add(len(frames))
            out.keyframe_points.foreach_set("co", points)
            for point in out.keyframe_points:
                point.interpolation = "LINEAR"
            out.update()
        if body_arm.animation_data is None:
            body_arm.animation_data_create()
        for track in list(body_arm.animation_data.nla_tracks):
            body_arm.animation_data.nla_tracks.remove(track)
        body_arm.animation_data.action = baked
        body_root.location = plan["body"]
        if plan["body_x"]:
            for f in frames:
                body_root.location.x = plan["body_x"]((f - PAD_FRAMES) / FPS)
                body_root.keyframe_insert("location", index=0, frame=f)
            body_root.animation_data.action.name = "%s_Rush" % plan["name"]

        floor = bpy.data.objects.new("%s_Floor" % plan["name"], floor_mesh)
        target.collection.objects.link(floor)
        floor.hide_select = True
        light_data = bpy.data.lights.new("%s_Light" % plan["name"], "POINT")
        light_data.color = LIGHT_COLOR
        light_data.energy = 400.0
        light_data.shadow_soft_size = 0.05
        light = bpy.data.objects.new("%s_Light" % plan["name"], light_data)
        light.location = ue(light_offset)
        target.collection.objects.link(light)

        camera_data = bpy.data.cameras.new("%s_CaptureCam" % plan["name"])
        camera_data.sensor_fit = "HORIZONTAL"
        camera_data.angle = math.radians(WATCHER_FIELD_OF_VIEW if plan["face"] else FIELD_OF_VIEW)
        camera_data.clip_start = 0.01
        camera_data.passepartout_alpha = 0.8
        camera_data.display_size = 0.3
        camera = bpy.data.objects.new("%s_CaptureCam" % plan["name"], camera_data)
        target.collection.objects.link(camera)
        target.camera = camera
        camera.rotation_mode = "XYZ"
        if plan["face"]:
            camera.location = ue(FACE_CAMERA_OFFSET)
            camera.rotation_euler = (math.radians(90.0), 0.0, math.radians(90.0))
        else:
            camera.location = ue(camera_offset)
            camera.rotation_euler = (plan["aim"] - camera.location).to_track_quat("-Z", "Y").to_euler("XYZ")
        camera.keyframe_insert("location", frame=PAD_FRAMES)
        camera.keyframe_insert("rotation_euler", frame=PAD_FRAMES)
        camera.animation_data.action.name = "%s_Camera" % plan["name"]
        target.frame_set(PAD_FRAMES)

        # Only the camera selected and active; the bones (the importer's icosphere shapes) hidden, still moving the body.
        layer = target.view_layers[0]
        for o in target.objects:
            o.select_set(o is camera, view_layer=layer)
        layer.objects.active = camera
        body_arm.hide_set(True, view_layer=layer)
        body_root.hide_set(True, view_layer=layer)
        floor.hide_set(False, view_layer=layer)

    for image in bpy.data.images:
        if image.source == "FILE" and image.packed_file is None and image.has_data:
            image.pack()
    text = bpy.data.texts.new("README_カメラの付け方")
    text.write(README)

    # The viewports: through the scene's camera, the camera following the view, material preview.
    for screen in bpy.data.screens:
        for area in screen.areas:
            for space in area.spaces:
                if space.type == "VIEW_3D":
                    space.lock_camera = True
                    space.shading.type = "MATERIAL"
                    space.clip_start = 0.01
                    if space.region_3d:
                        space.region_3d.view_perspective = "CAMERA"
                elif space.type == "TEXT_EDITOR":
                    space.text = text

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=OUT, compress=True)
    print("wrote %s" % OUT)


if __name__ == "__main__":
    try:
        import bpy  # noqa: F401
    except ImportError:
        outside()
    else:
        inside()
