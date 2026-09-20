"""Measures frame time and memory at one spot of a running PIE session, through the engine's CSV profiler
(.claude/guides/performance.md, "測り方"). No number is read off the screen: "csvprofile start/stop" writes
Saved/Profiling/CSV/Profile(*).csv with a row per frame, and its columns hold what stat unit, stat RHI and
stat memory show.

    python Tools/perf_probe.py measure --label z1_cp4_arrive
    python Tools/perf_probe.py measure --label z1_cp5_maze --checkpoint 5 --level L_Hospital_Zone1
    python Tools/perf_probe.py measure --label x --seconds 20 --target 1280x720 --capped

The PIE viewport of the editor is about 1040x654, a third of 1080p, so the probe sets r.ScreenPercentage so that
the rendering resolution has as many pixels as --target (1920x1080 by default) and puts it back to 100 after.
The engine smooths the frame rate to 62 fps, which hides every number above 60, so t.MaxFPS 500 lifts the cap
while measuring (--capped leaves it as it is, to see whether 60 fps holds with the cap on).

Needs PIE up (python Tools/pie.py start) and nothing else talking to the editor at the same time. Writes
Intermediate/Perf/<label>.json next to the printed table. Exit code 0 on success, 1 on a failure, 2 when no
editor answers.
"""
import argparse
import csv
import glob
import json
import os
import statistics
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pie  # noqa: E402  (same folder; its remote() runs one body in the editor)

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CSV_DIR = os.path.join(ROOT, "Saved", "Profiling", "CSV")
OUT_DIR = os.path.join(ROOT, "Intermediate", "Perf")

# Columns kept out of the CSV, in the order they are printed. stat unit's four times come first.
COLUMNS = [
    "FrameTime", "GameThreadTime", "RenderThreadTime", "GPUTime", "RHIThreadTime",
    "RHI/DrawCalls", "RHI/PrimitivesDrawn",
    "GPUMem/LocalUsedMB", "GPUMem/LocalBudgetMB", "RenderTargetPoolUsed", "Shaders/ShaderMemoryMB",
    "PhysicalUsedMB", "MemoryFreeMB",
    "TextureStreaming/StreamingPool", "TextureStreaming/CachedMips", "TextureStreaming/WantedMips",
]

VIEWPORT_SIZE = """
w = _need_game()
size = unreal.WidgetLayoutLibrary.get_viewport_size(w)
print('SIZE %d %d' % (size.x, size.y))
"""

BRIEF = """
w = _need_game()
player = unreal.GameplayStatics.get_player_character(w, 0)
print('BRIEF %.2f %s' % (unreal.GameplayStatics.get_time_seconds(w), player.get_name() if player else 'none'))
"""


def editor(body, what):
    code, out = pie.remote(body)
    if code != 0:
        print("%s failed:\n%s" % (what, out), file=sys.stderr)
        return None
    return out


def console(*commands):
    return editor(pie.COMMANDS.format(commands=list(commands)), "console %s" % " ".join(commands))


def editor_ram_mb():
    """The editor process's working set, which is what .claude/guides/performance.md caps at 20 GB."""
    try:
        out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq UnrealEditor.exe", "/FO", "CSV", "/NH"],
                             capture_output=True, text=True, timeout=30)
        total = 0
        for line in out.stdout.splitlines():
            parts = [p.strip('" ') for p in line.split('","')]
            if len(parts) >= 5 and parts[0].lower().startswith("unrealeditor"):
                total += int(parts[4].replace(",", "").replace(" K", "").replace("K", ""))
        return round(total / 1024) if total else None
    except Exception:
        return None


def vram_used_mb():
    """The whole card's VRAM in use (nvidia-smi gives no per-process figure on a GeForce)."""
    try:
        out = subprocess.run(["nvidia-smi", "--query-gpu=memory.used,memory.total", "--format=csv,noheader,nounits"],
                             capture_output=True, text=True, timeout=30)
        used, total = out.stdout.strip().splitlines()[0].split(",")
        return int(used), int(total)
    except Exception:
        return None, None


def open_at_checkpoint(level, checkpoint, warmup):
    """Puts the game where checkpoint N starts, as Tools/playthrough.py's setup does."""
    if console("Wasami.Checkpoint %d" % checkpoint, "Wasami.Lives 3") is None:
        return False
    if console("open " + level) is None:
        return False
    deadline = time.time() + 60
    while time.time() < deadline:
        time.sleep(1.0)
        out = editor(BRIEF, "the new world")
        if out and "BRIEF" in out:
            for line in out.splitlines():
                if line.startswith("BRIEF"):
                    elapsed, player = line.split()[1:3]
                    if player != "none" and float(elapsed) < 30.0:
                        time.sleep(warmup)  # streaming and shaders settle
                        return True
    print("%s did not open at checkpoint %d" % (level, checkpoint), file=sys.stderr)
    return False


def newest_csv():
    files = glob.glob(os.path.join(CSV_DIR, "*.csv"))
    return max(files, key=os.path.getmtime) if files else None


def summarise(path):
    with open(path, newline="", encoding="utf-8", errors="replace") as handle:
        rows = list(csv.DictReader(handle))
    stats = {}
    for name in COLUMNS:
        values = []
        for row in rows:
            try:
                values.append(float(row.get(name)))
            except (TypeError, ValueError):
                pass
        if not values:
            continue
        ordered = sorted(values)
        stats[name] = {
            "n": len(values),
            "avg": sum(values) / len(values),
            "med": statistics.median(values),
            "p95": ordered[max(0, int(len(ordered) * 0.95) - 1)],
            "max": ordered[-1],
        }
    return len(rows), stats


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("measure")
    p.add_argument("--label", required=True, help="name of the spot; the JSON is written under it")
    p.add_argument("--seconds", type=float, default=10.0)
    p.add_argument("--target", default="1920x1080", help="resolution to match in pixel count ('none' to leave)")
    p.add_argument("--capped", action="store_true", help="leave the engine's 62 fps smoothing on")
    p.add_argument("--slomo", type=float, default=0.0, help="time dilation while measuring (0.05 all but stops the "
                                                            "enemies, so a spot they patrol can be held for 10 s); "
                                                            "0 leaves the game at its speed")
    p.add_argument("--checkpoint", type=int, help="open the level at this checkpoint first")
    p.add_argument("--level", default="L_Hospital_Zone1")
    p.add_argument("--warmup", type=float, default=8.0, help="seconds after the level opens before measuring")
    p.add_argument("--note", default="", help="kept in the JSON (what the player is doing, where)")
    args = ap.parse_args()

    state, out = pie.in_pie()
    if state is None:
        print(out, file=sys.stderr)
        return 2
    if not state:
        print("PIE is not running (python Tools/pie.py start)", file=sys.stderr)
        return 1

    if args.checkpoint is not None and not open_at_checkpoint(args.level, args.checkpoint, args.warmup):
        return 1

    out = editor(VIEWPORT_SIZE, "the viewport size")
    if out is None:
        return 1
    width, height = [int(v) for v in [line for line in out.splitlines() if line.startswith("SIZE")][0].split()[1:3]]
    percentage = 100
    if args.target.lower() != "none":
        want = [int(v) for v in args.target.lower().split("x")]
        percentage = round(((want[0] * want[1]) / float(width * height)) ** 0.5 * 100)
    commands = ["r.ScreenPercentage %d" % percentage]
    if not args.capped:
        commands.append("t.MaxFPS 500")
    if args.slomo:
        commands.append("slomo %g" % args.slomo)
    if console(*commands) is None:
        return 1
    time.sleep(2.0)  # the new resolution settles (the render targets are made again)

    before_used, total = vram_used_mb()
    ram_before = editor_ram_mb()
    if console("csvprofile start") is None:
        return 1
    time.sleep(args.seconds)
    if console("csvprofile stop") is None:
        return 1
    after_used, _ = vram_used_mb()
    ram_after = editor_ram_mb()
    time.sleep(2.0)  # the CSV is written by a background thread
    console("r.ScreenPercentage 100", *(([] if args.capped else ["t.MaxFPS 0"]) + (["slomo 1"] if args.slomo else [])))

    path = newest_csv()
    if not path:
        print("no CSV in %s" % CSV_DIR, file=sys.stderr)
        return 1
    frames, stats = summarise(path)
    result = {
        "label": args.label,
        "note": args.note,
        "when": time.strftime("%Y-%m-%d %H:%M"),
        "level": args.level,
        "checkpoint": args.checkpoint,
        "viewport": [width, height],
        "screen_percentage": percentage,
        "rendered": [round(width * percentage / 100.0), round(height * percentage / 100.0)],
        "capped": args.capped,
        "slomo": args.slomo,
        "seconds": args.seconds,
        "frames": frames,
        "csv": os.path.relpath(path, ROOT),
        "vram_mb": {"before": before_used, "after": after_used, "total": total},
        "editor_ram_mb": {"before": ram_before, "after": ram_after},
        "stats": stats,
    }
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, args.label + ".json"), "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=1, ensure_ascii=False)

    print("%s: %s at %d%% (%dx%d -> %dx%d), %d frames in %.0f s%s" % (
        args.label, args.level, percentage, width, height, result["rendered"][0], result["rendered"][1],
        frames, args.seconds, " (62 fps cap on)" if args.capped else ""))
    if args.slomo:
        print("  time dilation while measuring: %g" % args.slomo)
    if "FrameTime" in stats:
        print("  fps: %.1f avg, %.1f at the p95 frame" % (1000.0 / stats["FrameTime"]["avg"],
                                                          1000.0 / stats["FrameTime"]["p95"]))
    for name in COLUMNS:
        if name in stats:
            s = stats[name]
            print("  %-32s avg %9.2f  med %9.2f  p95 %9.2f  max %9.2f" % (name, s["avg"], s["med"], s["p95"], s["max"]))
    if total:
        print("  VRAM (whole card): %d -> %d MB of %d MB" % (before_used, after_used, total))
    if ram_after:
        print("  editor RAM (working set): %d -> %d MB" % (ram_before, ram_after))
    return 0


if __name__ == "__main__":
    sys.exit(main())
