"""Measures screen recordings (Tools/desktop.py record, ffmpeg gdigrab) of the reference game and of PIE, so both sides
are compared the same way (.claude/guides/observation.md).

    python Tools/video_probe.py frames <video>
    python Tools/video_probe.py sheet <video> <out.png> [--start T] [--end T] [--every N] [--cols C] [--width W]
                                                        [--crop L,T,R,B]
    python Tools/video_probe.py series <video> [--start T] [--end T] [--every N] [--box NAME=L,T,R,B ...]
                                               [--stat mean|median] [--dark V]
    python Tools/video_probe.py period <video> --box L,T,R,B [--start T] [--end T] [--min-lag S] [--max-lag S]

frames  prints the size, the number of frames, the capture rate and the time of every frame.
sheet   tiles frames into one PNG with each frame's time written on it (read the PNG, not the frames one by one).
series  prints one line per frame: the mean RGB of the whole frame, then each box (mean or median RGB), and with
        --dark V the share of pixels whose luma is below V in the lower half of the frame (the tablet crossing the
        view during a camera shake).
period  compares the box between every pair of frames and prints the mean difference per time lag, smallest first
        (how long a spinning or pulsing thing takes to look the same again).

Times are each frame's pts in seconds as ffprobe reports them, never frame numbers: gdigrab drops frames while the
screen does not change, and slow captures (the reference game in full screen gives about 10 frames a second) are
uneven. Boxes and crops are in the video's own pixels (left, top, right, bottom).
"""
import argparse
import os
import shutil
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw

FFMPEG = shutil.which("ffmpeg") or r"C:\ffmpeg\bin\ffmpeg.exe"
FFPROBE = shutil.which("ffprobe") or r"C:\ffmpeg\bin\ffprobe.exe"
SHEET_LIMIT = 120  # frames; more than this makes a PNG too large to read back
LUMA = np.array([0.299, 0.587, 0.114], np.float32)


def probe(video):
    """Returns (width, height, [pts...]) of the first video stream."""
    size = subprocess.run([FFPROBE, "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
                           "-of", "csv=p=0", video], capture_output=True, text=True, check=True).stdout.strip()
    width, height = (int(v) for v in size.split(",")[:2])
    out = subprocess.run([FFPROBE, "-v", "error", "-select_streams", "v:0", "-show_entries", "frame=pts_time",
                          "-of", "csv=p=0", video], capture_output=True, text=True, check=True).stdout
    pts = [float(v.strip().rstrip(",")) for v in out.split() if v.strip().rstrip(",")]
    return width, height, pts


def box_arg(text):
    """"L,T,R,B" or "NAME=L,T,R,B" -> (name, (l, t, r, b))."""
    name, _, coords = text.rpartition("=")
    values = [int(float(v)) for v in coords.split(",")]
    if len(values) != 4 or values[2] <= values[0] or values[3] <= values[1]:
        raise argparse.ArgumentTypeError("a box is L,T,R,B with R > L and B > T: %r" % text)
    return name or coords, tuple(values)


def frames(video, vf=None, size=None):
    """Yields (index, pts, HxWx3 uint8 array) for every frame, decoded one at a time (full-size frames of the
    3440x1440 screen are 15 MB each). vf is an ffmpeg filter chain and size the (w, h) it produces."""
    width, height, pts = probe(video)
    if size:
        width, height = size
    cmd = [FFMPEG, "-hide_banner", "-loglevel", "error", "-i", video, "-fps_mode", "passthrough"]
    if vf:
        cmd += ["-vf", vf]
    cmd += ["-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.DEVNULL)
    step = width * height * 3
    try:
        index = 0
        while True:
            data = proc.stdout.read(step)
            if len(data) < step:
                break
            t = pts[index] if index < len(pts) else float("nan")
            yield index, t, np.frombuffer(data, np.uint8).reshape(height, width, 3)
            index += 1
    finally:
        proc.stdout.close()
        proc.wait()


def selected(args, t, index):
    return args.start <= t <= args.end and index % args.every == 0


def cmd_frames(args):
    width, height, pts = probe(args.video)
    span = pts[-1] - pts[0] if len(pts) > 1 else 0.0
    gaps = np.diff(pts) if len(pts) > 1 else np.array([0.0])
    print("%s: %dx%d, %d frames over %.3f s (%.1f per second; gaps median %.3f s, max %.3f s)" % (
        os.path.basename(args.video), width, height, len(pts), span, (len(pts) - 1) / span if span else 0.0,
        float(np.median(gaps)), float(gaps.max())))
    print(" ".join("%.3f" % t for t in pts))


def cmd_sheet(args):
    width, height, _ = probe(args.video)
    filters = []
    if args.crop:
        left, top, right, bottom = args.crop[1]
        filters.append("crop=%d:%d:%d:%d" % (right - left, bottom - top, left, top))
        width, height = right - left, bottom - top
    out_w = min(args.width, width)
    out_h = int(round(height * out_w / width / 2)) * 2
    filters.append("scale=%d:%d" % (out_w, out_h))
    picked = []
    for index, t, frame in frames(args.video, ",".join(filters), (out_w, out_h)):
        if selected(args, t, index):
            picked.append((t, frame.copy()))
            if len(picked) >= SHEET_LIMIT:
                print("stopped at %d frames (narrow --start/--end or raise --every)" % SHEET_LIMIT, file=sys.stderr)
                break
    if not picked:
        print("no frame between %.3f and %.3f s" % (args.start, args.end), file=sys.stderr)
        return 1
    rows = (len(picked) + args.cols - 1) // args.cols
    sheet = Image.new("RGB", (out_w * min(args.cols, len(picked)), out_h * rows))
    draw = ImageDraw.Draw(sheet)
    for i, (t, frame) in enumerate(picked):
        x, y = (i % args.cols) * out_w, (i // args.cols) * out_h
        sheet.paste(Image.fromarray(frame), (x, y))
        draw.rectangle((x, y, x + 52, y + 13), fill=(0, 0, 0))
        draw.text((x + 3, y + 1), "%.3f" % t, fill=(255, 255, 0))
    sheet.save(args.out)
    print("%s: %d frames (%.3f-%.3f s), %dx%d each, %dx%d" % (
        args.out, len(picked), picked[0][0], picked[-1][0], out_w, out_h, sheet.width, sheet.height))
    return 0


def cmd_series(args):
    boxes = args.box or []
    head = "time      whole (R G B)"
    for name, _ in boxes:
        head += "   %s %s" % (name, args.stat)
    if args.dark is not None:
        head += "   dark<%d lower half" % args.dark
    print(head)
    reduce = np.median if args.stat == "median" else np.mean
    for index, t, frame in frames(args.video):
        if not selected(args, t, index):
            continue
        whole = frame[::4, ::4].reshape(-1, 3).mean(0)
        line = "%8.3f  %5.1f %5.1f %5.1f" % (t, *whole)
        for _, (left, top, right, bottom) in boxes:
            value = reduce(frame[top:bottom, left:right].reshape(-1, 3), axis=0)
            line += "   %5.1f %5.1f %5.1f" % tuple(value)
        if args.dark is not None:
            lower = frame[frame.shape[0] // 2::2, ::2].astype(np.float32)
            luma = lower @ LUMA  # ffmpeg's gray, as in the step 11a measurements
            line += "   %.3f" % float((luma < args.dark).mean())
        print(line)
    return 0


def cmd_period(args):
    left, top, right, bottom = args.box[1]
    stride = max(1, max(right - left, bottom - top) // 64)
    times, samples = [], []
    for index, t, frame in frames(args.video):
        if args.start <= t <= args.end:
            times.append(t)
            samples.append(frame[top:bottom:stride, left:right:stride].astype(np.float32).ravel())
    if len(samples) < 3:
        print("too few frames in the range", file=sys.stderr)
        return 1
    times = np.array(times)
    stack = np.stack(samples)
    step = float(np.median(np.diff(times)))
    sums, counts = {}, {}
    for i in range(len(times) - 1):
        diff = np.abs(stack[i + 1:] - stack[i]).mean(axis=1)
        lags = np.round((times[i + 1:] - times[i]) / step) * step
        for lag, value in zip(lags, diff):
            if args.min_lag <= lag <= args.max_lag:
                key = round(float(lag), 3)
                sums[key] = sums.get(key, 0.0) + float(value)
                counts[key] = counts.get(key, 0) + 1
    table = sorted(((sums[k] / counts[k], k, counts[k]) for k in sums), key=lambda row: row[0])
    print("%d frames, step %.3f s, box sampled every %d px; lags %.2f-%.2f s, smallest difference first:" % (
        len(times), step, stride, args.min_lag, args.max_lag))
    for value, lag, count in table[:args.top]:
        print("  lag %7.3f s  diff %6.2f  (%d pairs)" % (lag, value, count))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    def ranged(p, every=True):
        p.add_argument("video")
        p.add_argument("--start", type=float, default=0.0)
        p.add_argument("--end", type=float, default=1e9)
        if every:
            p.add_argument("--every", type=int, default=1, help="keep every Nth frame")

    p = sub.add_parser("frames")
    p.add_argument("video")
    p = sub.add_parser("sheet")
    ranged(p)
    p.add_argument("out")
    p.add_argument("--cols", type=int, default=6)
    p.add_argument("--width", type=int, default=320, help="width of one tile")
    p.add_argument("--crop", type=box_arg)
    p = sub.add_parser("series")
    ranged(p)
    p.add_argument("--box", type=box_arg, action="append")
    p.add_argument("--stat", choices=("mean", "median"), default="mean")
    p.add_argument("--dark", type=int)
    p = sub.add_parser("period")
    ranged(p, every=False)
    p.add_argument("--box", type=box_arg, required=True)
    p.add_argument("--min-lag", type=float, default=1.0)
    p.add_argument("--max-lag", type=float, default=30.0)
    p.add_argument("--top", type=int, default=8)
    args = ap.parse_args()
    return {"frames": cmd_frames, "sheet": cmd_sheet, "series": cmd_series, "period": cmd_period}[args.cmd](args) or 0


if __name__ == "__main__":
    sys.exit(main())
