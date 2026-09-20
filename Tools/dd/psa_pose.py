"""原作のスケルタルメッシュを、アニメの姿勢のままスキニングして測る（`pak_reference*/_anims_psa` の psk・psa）。

本家の見た目の高さは、基準姿勢（psk そのまま）ではなく**流しているアニメの姿勢**で決まる。
この道具は psk の頂点を psa の骨の姿勢でスキニングし、UE の座標（cm・Z up）で外接の範囲と骨の位置を出す。

  python Tools/dd/psa_pose.py <mesh.psk>                            # 基準姿勢の範囲
  python Tools/dd/psa_pose.py <mesh.psk> <clip.psa>                 # そのアニメの間の範囲（5 コマおき）
  python Tools/dd/psa_pose.py <mesh.psk> <clip.psa> --step 1 --bone head
  python Tools/dd/psa_pose.py <mesh.psk> <clip.psa> --scale 5 --origin -107.2184   # 置かれたとおりの高さ

`--material` は数える材質のスロット（既定 0。本家は見せない部品を別スロットの透明な材質で消すことがある。
例: Matron の `BP_06_Matron_MiniBoss` はスロット 1 を `M_Transparent` で上書きして、のこぎりを消す）。
`--material all` で全部。

psk・psa は CUE4Parse の ActorX（`pak_reference_2/README.md` の 2）。根以外の骨の四元数は共役で入っている。
"""

import argparse
import os
import struct
import sys


def chunks(path):
    """ActorX のチャンク {ID: [データ]}。"""
    with open(path, "rb") as f:
        data = f.read()
    off, out = 0, {}
    while off < len(data):
        cid, _flag, dsize, dcount = struct.unpack_from("<20siii", data, off)
        cid = cid.split(b"\0")[0].decode()
        off += 32
        out.setdefault(cid, []).append(data[off:off + dsize * dcount])
        off += dsize * dcount
    return out


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def qconj(q):
    return (-q[0], -q[1], -q[2], q[3])


def qrot(q, v):
    x, y, z, w = q
    vx, vy, vz = v
    tx = 2 * (y * vz - z * vy)
    ty = 2 * (z * vx - x * vz)
    tz = 2 * (x * vy - y * vx)
    return (vx + w * tx + (y * tz - z * ty), vy + w * ty + (z * tx - x * tz), vz + w * tz + (x * ty - y * tx))


def bones(psk):
    """psk の参照骨格 [(名前, 親, 四元数, 位置)]（根以外は共役を戻した）。"""
    blob = psk["REFSKELT"][0]
    out = []
    for i in range(len(blob) // 120):
        rec = blob[i * 120:(i + 1) * 120]
        name = rec[:64].split(b"\0")[0].decode()
        parent = struct.unpack_from("<iii", rec, 64)[2]
        q = struct.unpack_from("<4f", rec, 76)
        t = struct.unpack_from("<3f", rec, 92)
        out.append((name, parent, q if i == 0 else qconj(q), t))
    return out


def fk(local):
    """局所の [(名前, 親, 四元数, 位置)] から部品の空間の [(四元数, 位置)]。"""
    out = []
    for _name, parent, q, t in local:
        if parent < 0:
            out.append((q, t))
        else:
            pq, pt = out[parent]
            r = qrot(pq, t)
            out.append((qmul(pq, q), (pt[0] + r[0], pt[1] + r[1], pt[2] + r[2])))
    return out


def mesh(psk, material):
    """(頂点, 数える頂点の番号, {頂点: [(骨, 重み)]})。"""
    blob = psk["PNTS0000"][0]
    points = [struct.unpack_from("<3f", blob, i * 12) for i in range(len(blob) // 12)]
    blob = psk["VTXW0000"][0]
    wedges = [struct.unpack_from("<H", blob, i * 16)[0] for i in range(len(blob) // 16)]
    blob = psk["FACE0000"][0]
    counted = set()
    for i in range(len(blob) // 12):
        w0, w1, w2, mat, _aux, _sg = struct.unpack_from("<HHHBBI", blob, i * 12)
        if material is None or mat == material:
            counted.update((wedges[w0], wedges[w1], wedges[w2]))
    blob = psk["RAWWEIGHTS"][0]
    influences = {}
    for i in range(len(blob) // 12):
        weight, point, bone = struct.unpack_from("<fii", blob, i * 12)
        influences.setdefault(point, []).append((bone, weight))
    return points, sorted(counted), influences


def clip(path):
    """(骨 [(名前, 親)], コマ数, コマ -> [(名前, 親, 四元数, 位置)], 1 秒のコマ数)。"""
    psa = chunks(path)
    blob = psa["BONENAMES"][0]
    names = [(blob[i * 120:i * 120 + 64].split(b"\0")[0].decode(),
              struct.unpack_from("<iii", blob, i * 120 + 64)[2]) for i in range(len(blob) // 120)]
    rate = struct.unpack_from("<f", psa["ANIMINFO"][0], 128 + 6 * 4)[0]
    keys = psa["ANIMKEYS"][0]
    frames = len(keys) // 32 // len(names)

    def pose(f):
        out = []
        for b, (name, parent) in enumerate(names):
            off = (f * len(names) + b) * 32
            t = struct.unpack_from("<3f", keys, off)
            q = struct.unpack_from("<4f", keys, off + 12)
            out.append((name, parent, q if b == 0 else qconj(q), t))
        return out

    return names, frames, pose, rate


def skin(ref, ref_world, world):
    """頂点を参照姿勢から今の姿勢へ運ぶ、骨ごとの (四元数, 位置)。"""
    out = []
    for i in range(len(ref)):
        rq, rt = ref_world[i]
        iq = qconj(rq)
        it = qrot(iq, (-rt[0], -rt[1], -rt[2]))
        aq, at = world[i]
        r = qrot(aq, it)
        out.append((qmul(aq, iq), (at[0] + r[0], at[1] + r[1], at[2] + r[2])))
    return out


def extent(points, counted, influences, moved, axis):
    lo, hi = 1e18, -1e18
    for p in counted:
        v = points[p]
        x = 0.0
        for bone, weight in influences.get(p, ()):
            q, t = moved[bone]
            x += weight * (qrot(q, v)[axis] + t[axis])
        lo, hi = min(lo, x), max(hi, x)
    return lo, hi


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("psk")
    ap.add_argument("psa", nargs="?")
    ap.add_argument("--material", default="0", help="数える材質のスロット（既定 0、all で全部）")
    ap.add_argument("--axis", default="z", choices=("x", "y", "z"), help="測る軸（既定 z = 高さ）")
    ap.add_argument("--step", type=int, default=5, help="何コマおきに測るか（既定 5）")
    ap.add_argument("--bone", action="append", default=[], help="一緒に出す骨（何度でも）")
    ap.add_argument("--scale", type=float, default=1.0, help="置かれたときの拡縮")
    ap.add_argument("--origin", type=float, default=0.0, help="置かれたときのメッシュの原点（cm）")
    ap.add_argument("--frames", action="store_true", help="コマごとに出す")
    args = ap.parse_args(argv)

    axis = "xyz".index(args.axis)
    material = None if args.material == "all" else int(args.material)
    psk = chunks(args.psk)
    ref = bones(psk)
    ref_world = fk(ref)
    points, counted, influences = mesh(psk, material)
    names = [b[0] for b in ref]
    placed = args.scale != 1.0 or args.origin

    def place(x):
        return x * args.scale + args.origin

    def report(label, world):
        moved = skin(ref, ref_world, world)
        lo, hi = extent(points, counted, influences, moved, axis)
        line = "%-12s %9.4f .. %9.4f cm" % (label, lo, hi)
        if placed:
            line += "   placed %8.2f .. %8.2f" % (place(lo), place(hi))
        for bone in args.bone:
            line += "   %s %.4f" % (bone, place(world[names.index(bone)][1][axis]))
        print(line)
        return lo, hi

    print("%s: %d bones, %d vertices counted (material %s)"
          % (os.path.basename(args.psk), len(ref), len(counted), args.material))
    if not args.psa:
        report("ref pose", ref_world)
        return 0

    _anim_bones, frames, pose, rate = clip(args.psa)
    print("%s: %d frames at %.3f fps (%.4f s)" % (os.path.basename(args.psa), frames, rate, (frames - 1) / rate))
    best = None
    for f in range(0, frames, args.step):
        keyed = {name: (q, t) for name, _parent, q, t in pose(f)}
        local = [(n, p, keyed[n][0] if n in keyed else q, keyed[n][1] if n in keyed else t) for n, p, q, t in ref]
        world = fk(local)
        if args.frames or f == 0:
            _lo, hi = report("frame %d" % f, world)
        else:
            _lo, hi = extent(points, counted, influences, skin(ref, ref_world, world), axis)
        if best is None or hi > best[1]:
            best = (f, hi)
    print("%-12s %9.4f cm at frame %d%s" % ("highest", best[1], best[0],
                                            "   placed %8.2f" % place(best[1]) if placed else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
