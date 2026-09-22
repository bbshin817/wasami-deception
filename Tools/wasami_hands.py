"""Which way the Wasami models' palms face, and how far the enemy's hands must be twisted to sit as the boss's do.

The palm's side is taken from the mesh itself: of the hand's three principal axes the thinnest one runs through the
palm, and the skin of the palm is on its +Y side in the bind pose of both models (looked at in renders, 2026-09-22).

  twist   `python Tools/wasami_hands.py twist` -- the enemy v3 model's palm against the boss's, read in the lowerarm's
          own frame (where the forearm's length is a fixed axis), in the bind pose and in the animations both models
          have. The turn about that axis that carries one onto the other is what dd_enemy.HAND_TWIST holds.
  palms   `python Tools/wasami_hands.py palms <glb> [up|down]` -- for each animation in a glb, which way each palm
          faces in the body's own frame at the clip's start (inwards, front, up; inwards positive is right).
          up/down says which side of the thin axis the palm's skin is on (the default is up, as the enemy's source is;
          the prepared glb keeps that mesh, so it is up as well, and the boss's is down).

Why: the user's enemy_wasami_v3.glb is modelled with its forearms turned over -- its bind pose has the palms up --
while its animations are of a body whose palms face down, so in play both palms face outwards (the review's finding,
the work list's item 37). dd_enemy.prepare turns each hand back; this tool measured by how much and tells whether it
worked.
"""
import importlib.util
import os
import sys

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("gltf", os.path.join(ROOT, "Content", "Python", "wasami_tools",
                                                                   "pipeline", "gltf.py"))
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)

DTYPE = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
V3 = os.path.join(ROOT, "SourceArt", "Wasami", "enemy_wasami_v3.glb")
BOSS = os.path.join(ROOT, "SourceArt", "Wasami", "boss_wasami.glb")
# (the side, the hand's bone, the bone past its fingers' roots, the forearm's bone)
SIDES = (("left", "hand_l", "LeftHand_End", "lowerarm_l"), ("right", "hand_r", "RightHand_End", "lowerarm_r"))
START = 2.0 / 30.0          # dd_enemy.CONTENT_START_FRAME: where every bone of a clip is first keyed


def array(m, b, index):
    """An accessor as a numpy array (the editor's Python has no numpy, so gltf.py cannot do this)."""
    acc = m["accessors"][index]
    view = m["bufferViews"][acc["bufferView"]]
    dt = DTYPE[acc["componentType"]]
    width = g.TYPE_WIDTHS[acc["type"]]
    size = np.dtype(dt).itemsize * width
    stride = view.get("byteStride", size)
    start = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    raw = np.frombuffer(bytes(b), dtype=np.uint8, count=stride * (acc["count"] - 1) + size, offset=start)
    rows = np.lib.stride_tricks.as_strided(raw, shape=(acc["count"], size), strides=(stride, 1)).copy()
    return rows.view(dt).reshape(acc["count"], width)


def matrix(q):
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


class Model:
    def __init__(self, path, palm_up=True):
        self.name = os.path.basename(path)
        self.m, self.b = g.read(path)
        m, b = self.m, self.b
        names = g.node_names(m)
        self.node = {n: i for i, n in enumerate(names)}
        prim = m["meshes"][0]["primitives"][0]
        pos = array(m, b, prim["attributes"]["POSITION"]).astype(np.float64)
        joints = array(m, b, prim["attributes"]["JOINTS_0"]).astype(np.int32)
        weights = array(m, b, prim["attributes"]["WEIGHTS_0"]).astype(np.float64)
        skin = m["skins"][0]["joints"]
        dominant = np.array([skin[j[np.argmax(w)]] for j, w in zip(joints, weights)])
        bone = {names[n]: n for n in skin}
        self.bind = g.world_transforms(m, {}, {})
        self.palm_local = {}
        for side, hand, end, _ in SIDES:
            p = pos[np.isin(dominant, [bone[hand], bone[end]])]
            _, _, vt = np.linalg.svd(p - p.mean(axis=0), full_matrices=False)
            thin = vt[2] * (1.0 if vt[2][1] >= 0.0 else -1.0)           # the hand's thinnest axis, its +Y way round
            self.palm_local[side] = matrix(self.bind[hand][0]).T @ (thin if palm_up else -thin)
        # The forearm's length in the lowerarm's own frame: the hand node's rest offset from it.
        self.axis = {}
        for side, hand, _, _ in SIDES:
            a = np.array(m["nodes"][self.node[hand]]["translation"], dtype=np.float64)
            self.axis[side] = a / np.linalg.norm(a)
        self.anims = {a["name"]: a for a in m.get("animations", [])}

    def pose(self, anim=None, t=START):
        if anim is None:
            return self.bind
        chans = g.channels(self.m, self.b, self.anims[anim])
        rot = {n: g.sample("rotation", *chans[(n, "rotation")], t) for (n, p) in chans if p == "rotation"}
        tr = {n: g.sample("translation", *chans[(n, "translation")], t) for (n, p) in chans if p == "translation"}
        return g.world_transforms(self.m, rot, tr)

    def palm(self, side, world):
        hand = dict((s[0], s[1]) for s in SIDES)[side]
        return matrix(world[hand][0]) @ self.palm_local[side]

    def palm_in_forearm(self, side, world):
        """The palm's direction read in the lowerarm's frame, where the forearm's length does not move."""
        lower = dict((s[0], s[3]) for s in SIDES)[side]
        return matrix(world[lower][0]).T @ self.palm(side, world)

    def body_frame(self, world):
        """(the body's left, forwards, up) in the pose, from how far the pelvis has turned from the bind pose. glTF
        is right handed with +Y up and +Z the front, so the body's left is up x forwards."""
        up = np.array([0.0, 1.0, 0.0])
        fwd = matrix(world["pelvis"][0]) @ matrix(self.bind["pelvis"][0]).T @ np.array([0.0, 0.0, 1.0])
        fwd = fwd - up * (fwd @ up)
        fwd = fwd / np.linalg.norm(fwd)
        return np.cross(up, fwd), fwd, up


def turn_about(a, c, axis):
    """The degrees about axis that carry direction a onto direction c."""
    axis = axis / np.linalg.norm(axis)
    pa, pc = a - axis * (a @ axis), c - axis * (c @ axis)
    pa, pc = pa / np.linalg.norm(pa), pc / np.linalg.norm(pc)
    return float(np.degrees(np.arctan2(np.cross(pa, pc) @ axis, pa @ pc)))


def twist():
    v3, boss = Model(V3, palm_up=True), Model(BOSS, palm_up=False)
    print("how far v3's hands must turn about the forearm's length to sit as the boss's do")
    print("  %-13s %-6s %-26s %-26s %s" % ("pose", "side", "v3 palm (lowerarm frame)", "boss palm", "turn"))
    for name in [None] + sorted(set(v3.anims) & set(boss.anims)):
        for side, _, _, _ in SIDES:
            a, c = v3.palm_in_forearm(side, v3.pose(name)), boss.palm_in_forearm(side, boss.pose(name))
            print("  %-13s %-6s %-26s %-26s %+7.1f deg"
                  % (name or "(bind)", side, np.round(a, 3), np.round(c, 3), turn_about(a, c, v3.axis[side])))


def palms(path, palm_up=True):
    mo = Model(path, palm_up=palm_up)
    print("%s: which way each palm faces in the body's frame (inwards positive is right)" % mo.name)
    worst = 1.0
    for name in [None] + sorted(mo.anims):
        world = mo.pose(name)
        left, fwd, up = mo.body_frame(world)
        out = []
        for side, _, _, _ in SIDES:
            p = mo.palm(side, world)
            towards = p @ (-left if side == "left" else left)    # each palm's own way in to the body
            worst = min(worst, towards) if name is not None else worst
            out.append("%s inwards %+.2f front %+.2f up %+.2f" % (side, towards, p @ fwd, p @ up))
        print("  %-34s %s | %s" % (name or "(bind)", *out))
    print("  the least inwards any palm faces in an animation: %+.2f" % worst)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "twist":
        twist()
    elif len(sys.argv) > 2 and sys.argv[1] == "palms":
        palms(sys.argv[2], palm_up=(len(sys.argv) < 4 or sys.argv[3] == "up"))
    else:
        print(__doc__)
