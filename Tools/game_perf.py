"""Measures the packaged game's frame time at one spot, the same way Tools/perf_probe.py measures PIE.

    python Tools/game_perf.py measure --label pkg_z1_cp4 --checkpoint 4 --level L_Hospital_Zone1
    python Tools/game_perf.py measure --label pkg_z2_cp7 --checkpoint 7 --level L_Hospital_Zone2         --pre "Wasami.Flow OnCellCutsceneFinished" --pre "BugItGo -14573.65 1694.18 92 0 -67.86 0"
    python Tools/game_perf.py table                      # every Intermediate/Perf/pkg_*.json as one table

No key is sent to the game: the spot is reached through the command line, which needs no window in front (the firewall
prompt that used to hold the foreground is gone since 2026-09-23; .claude/references/troubleshooting.md).
Two launches per spot:

  1. <exe> <map> -ExecCmds="Wasami.Lives 3, Wasami.Checkpoint N, quit"   writes the checkpoint into the save and quits
  2. <exe> <map> -ExecCmds="t.MaxFPS 500, slomo S, CsvProfile exitoncompletion, CsvProfile frames=F"

UEngine::Init queues -ExecCmds as deferred commands and UGameEngine::Init loads the map before the first tick, so the
commands of the second launch run inside the loaded zone: the checkpoint's player start decides where the player
stands (AWasamiGameMode::ChoosePlayerStart), slomo all but stops the enemies, and the CSV profiler writes one row per
frame to <archive>/wasami_deception/Saved/Profiling/CSV and exits the game when it has its frames.

Only the last --seconds of the capture are summarised; everything before is the warm-up (streaming and shaders). The
columns are Tools/perf_probe.py's, so the numbers go next to the PIE ones in implementation record 00. Writes
Intermediate/Perf/<label>.json. Exit code 0 on success, 1 on a failure.
"""
import argparse
import glob
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import console_session  # noqa: E402  (same folder)
import perf_probe  # noqa: E402  (COLUMNS and summarise, so both tables hold the same numbers)

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ARCHIVE = os.path.join(ROOT, "Saved", "Archive", "Windows")
EXE = os.path.join(ARCHIVE, "wasami_deception.exe")
GAME_SAVED = os.path.join(ARCHIVE, "wasami_deception", "Saved")
CSV_DIR = os.path.join(GAME_SAVED, "Profiling", "CSV")
LOG = os.path.join(GAME_SAVED, "Logs", "wasami_deception.log")
OUT_DIR = os.path.join(ROOT, "Intermediate", "Perf")
IMAGE = "wasami_deception.exe"


def running():
    return console_session.running(IMAGE)


def kill():
    subprocess.run(["taskkill", "/F", "/IM", IMAGE], capture_output=True, text=True)
    for _ in range(20):
        if not running():
            return
        time.sleep(0.5)


def vram_used_mb():
    try:
        out = subprocess.run(["nvidia-smi", "--query-gpu=memory.used,memory.total", "--format=csv,noheader,nounits"],
                             capture_output=True, text=True, timeout=20)
        used, total = out.stdout.strip().splitlines()[0].split(",")
        return int(used), int(total)
    except Exception:
        return None, None


def start_game(argument):
    """console_session.start for one raw argument string: -ExecCmds="a, b" has to keep its own quotes, which
    subprocess.list2cmdline would escape away (UE parses FCommandLine::Get(), not argv)."""
    ours, console = console_session.session_ids()
    if ours == console:
        flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
        subprocess.Popen('"%s" %s' % (EXE, argument), creationflags=flags, close_fds=True)
        return True
    script = console_session.TASK_PS.format(execute=console_session.ps_quote(EXE),
                                            argument="-Argument " + console_session.ps_quote(argument),
                                            task=console_session.TASK)
    return console_session.run_powershell(script)


def launch(map_name, commands, timeout, poll_vram=False):
    """Starts the game on the interactive desktop and waits for it to quit by itself. (peak VRAM MB, seconds)."""
    if running():
        kill()
    argument = '%s -ExecCmds="%s"' % (map_name, ", ".join(commands))
    print("  %s %s" % (os.path.basename(EXE), argument))
    if not start_game(argument):
        print("could not start %s" % EXE, file=sys.stderr)
        return None, None
    started = time.time()
    for _ in range(60):  # the process appears within half a minute
        if running():
            break
        time.sleep(0.5)
    peak = None
    while time.time() - started < timeout:
        if not running():
            return peak, time.time() - started
        if poll_vram:
            used, _ = vram_used_mb()
            if used is not None:
                peak = used if peak is None else max(peak, used)
        time.sleep(1.0)
    print("the game did not quit within %d s; killing it" % timeout, file=sys.stderr)
    kill()
    return peak, time.time() - started


def newest_csv(since):
    files = [p for p in glob.glob(os.path.join(CSV_DIR, "*.csv")) if os.path.getmtime(p) > since]
    return max(files, key=os.path.getmtime) if files else None


def tail_rows(path, seconds):
    """The rows of the last <seconds> of the capture (FrameTime is milliseconds), newest capture last."""
    import csv as csv_module
    with open(path, newline="", encoding="utf-8", errors="replace") as handle:
        rows = list(csv_module.DictReader(handle))
    kept, total = [], 0.0
    for row in reversed(rows):
        try:
            total += float(row["FrameTime"])
        except (TypeError, ValueError, KeyError):
            continue
        kept.append(row)
        if total >= seconds * 1000.0:
            break
    kept.reverse()
    return rows, kept


def summarise_rows(rows):
    import statistics
    stats = {}
    for name in perf_probe.COLUMNS:
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
    return stats


def quality_commands(quality):
    """The scalability the product's own OPTIONS quality means (UWasamiSettingsSaveGame::Apply): the level over all
    groups, the view distance always at 3 and the post-processing at 3 only for VERY HIGH, the resolution scale at
    100. Set here rather than through Wasami.Settings, which crashes a packaged build
    (.claude/references/troubleshooting.md)."""
    if quality is None:
        return []
    return ["scalability %d" % quality, "sg.ViewDistanceQuality 3",
            "sg.PostProcessQuality %d" % (3 if quality >= 3 else 2), "sg.ResolutionQuality 100"]


LOCK = os.path.join(OUT_DIR, ".game_perf.lock")


def take_lock():
    """Only one probe at a time: two runs would launch the game twice and read each other's CSV (2026-09-21)."""
    os.makedirs(OUT_DIR, exist_ok=True)
    if os.path.exists(LOCK):
        with open(LOCK, encoding="utf-8") as handle:
            pid = handle.read().strip()
        alive = subprocess.run(["tasklist", "/FI", "PID eq " + pid, "/FO", "CSV", "/NH"],
                               capture_output=True, text=True).stdout
        if pid and pid in alive:
            print("another Tools/game_perf.py (pid %s) is measuring; wait for it" % pid, file=sys.stderr)
            return False
    with open(LOCK, "w", encoding="utf-8") as handle:
        handle.write(str(os.getpid()))
    return True


def drop_lock():
    try:
        os.remove(LOCK)
    except OSError:
        pass


def measure(args):
    if not os.path.exists(EXE):
        print("no packaged build at %s (.claude/guides/distribution.md)" % EXE, file=sys.stderr)
        return 1
    prepare = []
    if args.checkpoint is not None:
        prepare += ["Wasami.Lives %d" % args.lives, "Wasami.Checkpoint %d" % args.checkpoint]
    if prepare:
        print("preparing the save (%s)..." % ", ".join(prepare))
        _, took = launch(args.level, prepare + ["quit"], args.prepare_timeout)
        if took is None:
            return 1
        print("  done in %.0f s" % took)

    since = time.time()
    commands = list(quality_commands(args.game_quality)) + ["t.MaxFPS 500"]
    if args.slomo:
        commands.append("slomo %g" % args.slomo)
    commands += list(args.pre) + ["CsvProfile exitoncompletion", "CsvProfile frames=%d" % args.frames]
    print("capturing %d frames at %s (checkpoint %s)..." % (args.frames, args.level, args.checkpoint))
    peak_vram, took = launch(args.level, commands, args.timeout, poll_vram=True)
    if took is None:
        return 1
    print("  the game ran %.0f s (peak VRAM %s MB)" % (took, peak_vram))

    for _ in range(20):  # the CSV is written by a background thread as the game exits
        path = newest_csv(since)
        if path and os.path.getsize(path) > 0:
            break
        time.sleep(1.0)
    path = newest_csv(since)
    if not path:
        print("no new CSV in %s" % CSV_DIR, file=sys.stderr)
        return 1
    all_rows, rows = tail_rows(path, args.seconds)
    if not rows:
        print("the CSV %s has no FrameTime" % path, file=sys.stderr)
        return 1
    stats = summarise_rows(rows)
    result = {
        "label": args.label,
        "note": args.note,
        "when": time.strftime("%Y-%m-%d %H:%M"),
        "build": "packaged Development Win64",
        "level": args.level,
        "checkpoint": args.checkpoint,
        "resolution": args.resolution,
        "quality": args.game_quality,
        "slomo": args.slomo,
        "seconds": args.seconds,
        "frames": len(rows),
        "captured_frames": len(all_rows),
        "warmup_frames": len(all_rows) - len(rows),
        "csv": os.path.relpath(path, ROOT),
        "vram_mb": {"peak": peak_vram, "total": vram_used_mb()[1]},
        "stats": stats,
    }
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, args.label + ".json"), "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=1, ensure_ascii=False)

    print("%s: %s cp %s, %d of %d frames kept (%.0f s)" % (args.label, args.level, args.checkpoint,
                                                           len(rows), len(all_rows), args.seconds))
    if "FrameTime" in stats:
        print("  fps: %.1f avg, %.1f at the p95 frame" % (1000.0 / stats["FrameTime"]["avg"],
                                                          1000.0 / stats["FrameTime"]["p95"]))
    for name in perf_probe.COLUMNS:
        if name in stats:
            s = stats[name]
            print("  %-34s avg %9.2f  med %9.2f  p95 %9.2f  max %9.2f" % (name, s["avg"], s["med"], s["p95"], s["max"]))
    return 0


def table(args):
    rows = []
    for path in sorted(glob.glob(os.path.join(OUT_DIR, "%s*.json" % args.prefix))):
        with open(path, encoding="utf-8") as handle:
            rows.append(json.load(handle))
    if not rows:
        print("no %s*.json in %s" % (args.prefix, OUT_DIR), file=sys.stderr)
        return 1
    print("| label | fps avg | p95 の fps | Frame ms | Game ms | GPU ms | DrawCalls | Prims | GPU メモリ |")
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    for r in rows:
        s = r["stats"]
        def avg(name, default=float("nan")):
            return s[name]["avg"] if name in s else default
        print("| %s | %.1f | %.1f | %.2f | %.2f | %.2f | %.0f | %.0fk | %.0f MB |" % (
            r["label"], 1000.0 / avg("FrameTime"), 1000.0 / s["FrameTime"]["p95"], avg("FrameTime"),
            avg("GameThreadTime"), avg("GPUTime"), avg("RHI/DrawCalls"), avg("RHI/PrimitivesDrawn") / 1000.0,
            avg("GPUMem/LocalUsedMB")))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("measure")
    p.add_argument("--label", required=True)
    p.add_argument("--level", default="L_Hospital_Zone1")
    p.add_argument("--checkpoint", type=int, help="written into the save by a first launch; none keeps the save as it is")
    p.add_argument("--lives", type=int, default=3)
    p.add_argument("--slomo", type=float, default=0.05,
                   help="time dilation while measuring (0.05 all but stops the enemies); 0 leaves the game at its speed")
    p.add_argument("--frames", type=int, default=1500, help="frames to capture; the last --seconds of them are kept")
    p.add_argument("--seconds", type=float, default=10.0)
    p.add_argument("--timeout", type=int, default=300, help="seconds to wait for the capture launch to quit")
    p.add_argument("--prepare-timeout", type=int, default=180)
    p.add_argument("--resolution", default="1920x1080", help="kept in the JSON; set in the build's GameUserSettings.ini")
    p.add_argument("--game-quality", type=int, default=3,
                   help="the game's own OPTIONS quality (0 LOW to 3 VERY HIGH); the same scalability the product's "
                        "SET SETTINGS applies (UWasamiSettingsSaveGame::Apply), set after the level has loaded")
    p.add_argument("--pre", action="append", default=[],
                   help="a console command to run after the level has loaded and before the capture (repeatable), "
                        "Zone 2's cell is --pre \"Wasami.Flow OnCellCutsceneFinished\" --pre \"BugItGo <x> <y> <z> "
                        "0 <yaw> 0\": checkpoint 7 starts at PlayerStart_1, and the scene that moves the player into "
                        "the cell does not end by itself when it is called from the console")
    p.add_argument("--note", default="")
    p.set_defaults(func=measure)
    t = sub.add_parser("table")
    t.add_argument("--prefix", default="pkg_")
    t.set_defaults(func=table)
    args = ap.parse_args()
    if args.func is not measure:
        return args.func(args)
    if not take_lock():
        return 1
    try:
        return measure(args)
    finally:
        drop_lock()


if __name__ == "__main__":
    sys.exit(main())
