"""Plays the packaged build through, from the title to the escape, with the command line alone (work list item 36).

    python Tools/game_flow.py run                  one launch, about four minutes, then the timeline
    python Tools/game_flow.py run --dry-run        the command line it would use
    python Tools/game_flow.py timeline             read the last run's log again

No key or click is sent: Windows' firewall prompt holds the foreground, so nothing typed reaches the game
(.claude/references/troubleshooting.md). A packaged build's only way in is -ExecCmds, which runs on the first tick, so
the run is written out as a schedule of Wasami.Delay commands: each milestone of Tools/playthrough.py's sections gets
its own moment, reached with the flow's own triggers and events (Wasami.Trigger, Wasami.Flow) and, where the player
has to be somewhere else, the checkpoints' player starts (BugItGo). At every milestone Wasami.Status writes one line
to the log and Shot showui takes the picture.

The waits are real time from the title's first tick and cover the two level loads, so the timeline is read afterwards
to see that each milestone landed where it should: the log's Wasami.Status lines are matched, in order, against the
route's expectations (the level, the checkpoint, the objective, the widgets on screen). The pictures are copied out of
the build to Intermediate/GameFlow/. Exit code 0 when every expectation held and nothing crashed, 1 otherwise.

What this cannot try is the mouse and the keys themselves: the title's NEW GAME, the ring piece screen's CLOSE and the
score screen's NEXT are clicks, so the run goes round them (Wasami.ResetSave and open for the title, Wasami.Flow
OnRingPieceCollect for the ring piece) and stops at the score screen. Those three were pressed by hand on 2026-09-23,
once with the mouse and once with the keyboard (Tab, then Enter), after the firewall prompt went away: work list item
52, implementation record 00. Note the ring piece screen itself never comes up in this run — Wasami.Flow
OnRingPieceCollect is what happens after it closes; Wasami.Flow OnCollectedRingPiece is what puts it up.
"""
import argparse
import glob
import os
import re
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import game_perf  # noqa: E402  (same folder: the launcher, the paths and the lock)

ROOT = game_perf.ROOT
OUT_DIR = os.path.join(ROOT, "Intermediate", "GameFlow")
SHOT_DIR = os.path.join(game_perf.GAME_SAVED, "Screenshots", "Windows")
CRASH_DIR = os.path.join(game_perf.GAME_SAVED, "Crashes")

# The checkpoints' player starts (Intermediate/Pipeline/dd/stage_ue.json), as BugItGo takes them.
CELL_OUT = "BugItGo -10501.43 -2516.37 92 0 30 0"       # PlayerStart_MiniBoss, out of the cell before the spikes
MAZE_AT = "BugItGo -3373 0 103 0 0 0"                    # PlayerStart_Maze
POSTMAZE_AT = "BugItGo -1680 0 103 0 180 0"              # PlayerStart_PostMaze, by the altar


class Step:
    """One milestone: <after> seconds after the one before it, its console commands, and what the status should say.

    status=True adds Wasami.Status (a line in the log), shot=True adds Shot showui (a picture). expect holds the fields
    of that status line that matter: level and checkpoint exactly, objective and widgets as a part of the text."""

    def __init__(self, name, after, commands=(), status=True, shot=False, note="", **expect):
        self.name, self.after, self.commands = name, after, list(commands)
        self.status, self.shot, self.note, self.expect = status, shot, note, expect


# The playthrough of Tools/playthrough.py, section by section, as one schedule. The waits hold the times the flow
# itself takes (implementation record 11): the level's 10 s hold and the lift's 14.1 s arrival, the parking lot's
# 10.53 s scene, the ambulance's 1 + 7 + 2.5 s to the level change, the yard's 26.23 s capture and 74.07 s cell.
ROUTE = [
    Step("title", 3.0, shot=True, level="L_Title", widgets="WasamiTitleScreenWidget",
         note="the title screen"),
    Step("new_game", 2.0, ["Wasami.ResetSave", "open L_Hospital_Zone1"], status=False,
         note="NEW GAME (the save started over and Zone 1 opened)"),
    Step("z1_load", 5.0, shot=True, level="L_Hospital_Zone1", checkpoint=4, lives=3, shards=337,
         note="Zone 1 opened at the lift with an empty save (the stage's title card, the lift rising)"),
    Step("z1_arrive", 14.0, shot=True, level="L_Hospital_Zone1", checkpoint=4,
         note="the lift is there and its doors are open"),
    Step("z1_intercom", 1.0, ["Wasami.Trigger 04_Intercom"], status=False,
         note="the intercom (the nurse's announcement)"),
    Step("z1_doorbreak", 4.0, ["Wasami.Flow On04DoorBreak"], shot=True, level="L_Hospital_Zone1", checkpoint=4,
         note="the lock picked: the doors in front swing open"),
    Step("z1_maze", 4.0, ["Wasami.Trigger BP_04_Trigger_Maze"], shot=True, checkpoint=5, shards=337,
         objective="SHARDS", note="into the maze (saved 5, COLLECT ALL SHARDS)"),
    Step("z1_collect", 3.0, ["Wasami.CollectShards"], status=False, note="every shard of the maze collected"),
    Step("z1_shards", 3.0, shot=True, checkpoint=5, objective="PARKING", shards=0,
         note="all 337 collected: REACH THE PARKING LOT, the barrier gone"),
    Step("z1_cutscene", 2.0, ["Wasami.Trigger 06_CutsceneStart"], status=False,
         note="the parking lot's scene begins (06_Hospital_Zone1_06Event, 10.53 s)"),
    Step("z1_cutscene_shot", 4.0, status=False, shot=True, note="the scene from its cine camera"),
    Step("z1_tunnel", 7.5, ["Wasami.Trigger 06_TunnelEnter"], objective="AMBULANCE",
         note="the car park (REACH THE TUNNEL), then GET ON TOP OF THE AMBULANCE"),
    Step("z1_ambulance", 1.2, ["Wasami.Trigger TriggerBox_06_AmbulanceTop"], status=False,
         note="on the ambulance's roof (the nurses are taken away here)"),
    Step("z1_takeoff", 1.5, shot=True, level="L_Hospital_Zone1", checkpoint=7, objective="GOOD LUCK",
         note="saved 7, GOOD LUCK: the ambulance drives off"),
    Step("z2_load", 16.0, shot=True, level="L_Hospital_Zone2", checkpoint=7, shards=342,
         note="Zone 2's yard: the ambulance drives in with the player on its roof"),
    Step("z2_capture", 4.0, ["Wasami.Trigger Trigger_Arrive_CaptureScene"], status=False,
         note="the capture scene begins (26.23 s)"),
    Step("z2_capture_shot", 6.0, shot=True, input=0, note="the capture, the player's input taken away"),
    Step("z2_cell_shot", 40.0, status=False, shot=True, note="the cell's scene (74.07 s)"),
    Step("z2_cell", 60.0, shot=True, level="L_Hospital_Zone2", checkpoint=7, input=1,
         note="the cell with the input back, the spikes coming down"),
    Step("z2_lock", 2.0, ["Wasami.Flow OnCellDoorBreak"], shot=True, note="the cell's lock picked"),
    Step("z2_out", 4.0, [CELL_OUT], status=False, note="out of the cell, before the spikes reach the head"),
    Step("z2_miniboss", 1.5, ["Wasami.Trigger BP_MiniBoss_Trigger"], shot=True, checkpoint=8, objective="NURSES",
         note="the corridor (saved 8, GET PAST THE NURSES): the six sentries wake"),
    Step("z2_maze_at", 4.0, [MAZE_AT], status=False, note="past the Matron, to the maze's way in"),
    Step("z2_maze", 1.5, ["Wasami.Trigger Trigger_MazeStart"], shot=True, checkpoint=9, objective="SHARDS",
         shards=342, note="the maze (saved 9, COLLECT ALL SHARDS)"),
    Step("z2_collect", 3.0, ["Wasami.CollectShards"], status=False, note="every shard of the maze collected"),
    Step("z2_shards", 3.0, shot=True, checkpoint=10, objective="RING", shards=0,
         note="all 342 collected (saved 10, COLLECT THE RING PIECE)"),
    Step("z2_altar_at", 2.0, [POSTMAZE_AT], status=False, note="back along the corridor to the altar"),
    Step("z2_ring", 1.5, ["Wasami.Flow OnRingPieceCollect"], shot=True, checkpoint=10, objective="GARAGE",
         note="the ring piece taken (HEAD TOWARDS THE GARAGE)"),
    Step("z2_garage", 3.0, ["Wasami.Trigger Postmaze_Trigger_Garage"], shot=True, objective="PORTAL",
         note="the garage (GET TO THE PORTAL): the portal opens"),
    Step("z2_escape", 3.0, ["Wasami.Trigger Wasami_EscapeTrigger"], status=False,
         note="into the portal: the screen goes black"),
    Step("z2_results", 8.0, shot=True, paused=1, deaths=0, checkpoint=0, widgets="WasamiLevelClearWidget",
         note="the score screen (You Escaped!, the results, FINAL RANK)"),
    Step("quit", 6.0, ["quit"], status=False, note="the game closes itself"),
]

STATUS_RE = re.compile(r"^\[(?P<stamp>[\d.\-:]+)\]\[\s*\d+\]LogWasamiDebug: Display: Wasami\.Status (?P<fields>.*)$")
FIELD_RE = re.compile(r"(\w+)=('[^']*'|\S+)")


def schedule(route):
    """The -ExecCmds list: the frame rate capped so the picture costs no more than it must, then every milestone as
    Wasami.Delay <seconds since the first tick> <command>."""
    commands = ["t.MaxFPS 60"]
    at = 0.0
    for step in route:
        at += step.after
        for command in step.commands + (["Wasami.Status"] if step.status else []) + (["Shot showui"] if step.shot else []):
            commands.append("Wasami.Delay %.1f %s" % (at, command))
    return commands, at


def parse_status(line):
    match = STATUS_RE.match(line.strip())
    if not match:
        return None
    fields = {"stamp": match.group("stamp")}
    for name, value in FIELD_RE.findall(match.group("fields")):
        fields[name] = value[1:-1] if value.startswith("'") else value
    return fields


def read_status(path):
    rows = []
    with open(path, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            fields = parse_status(line)
            if fields:
                rows.append(fields)
    return rows


def check(step, fields):
    """The expectations of one milestone against its status line; a list of what went wrong (empty when it held)."""
    wrong = []
    for name, want in step.expect.items():
        got = fields.get(name if name != "shards" else "shards")
        if name == "shards":                       # 'N/Total': only the N matters
            got = (got or "").split("/")[0]
            want = str(want)
            if got != want:
                wrong.append("shards %s, not %s" % (got or "?", want))
            continue
        if name in ("objective", "widgets"):
            if want.upper() not in (got or "").upper():
                wrong.append("%s '%s' holds no %s" % (name, got, want))
            continue
        if str(got) != str(want):
            wrong.append("%s %s, not %s" % (name, got, want))
    return wrong


def seconds_between(first, later):
    def when(stamp):
        return time.mktime(time.strptime(stamp.split(":")[0], "%Y.%m.%d-%H.%M.%S")) + int(stamp.split(":")[1]) / 1000.0
    try:
        return when(later) - when(first)
    except (ValueError, IndexError):
        return float("nan")


def timeline(route, rows, out=sys.stdout):
    """The route's milestones beside the status lines the log holds, in order; True when every expectation held."""
    wanted = [step for step in route if step.status]
    ok = True
    print("| s | milestone | level | cp | lives | shards | objective | widgets | |", file=out)
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- |", file=out)
    for index, step in enumerate(wanted):
        if index >= len(rows):
            print("| | %s | (no status line) | | | | | | NO |" % step.name, file=out)
            ok = False
            continue
        fields = rows[index]
        wrong = check(step, fields)
        ok = ok and not wrong
        print("| %.0f | %s | %s | %s | %s | %s | %s | %s | %s |" % (
            seconds_between(rows[0]["stamp"], fields["stamp"]), step.name, fields.get("level", "?"),
            fields.get("checkpoint", "?"), fields.get("lives", "?"), fields.get("shards", "?"),
            fields.get("objective", ""), fields.get("widgets", ""), "OK" if not wrong else "; ".join(wrong)), file=out)
    if len(rows) > len(wanted):
        print("(%d more status lines than the route has milestones)" % (len(rows) - len(wanted)), file=out)
        ok = False
    return ok


def keep_shots(route, since):
    """The pictures the run took, newest last, copied out of the build as Intermediate/GameFlow/NN-<milestone>.png."""
    shots = sorted((p for p in glob.glob(os.path.join(SHOT_DIR, "ScreenShot*.png")) if os.path.getmtime(p) > since),
                   key=os.path.getmtime)
    names = [step.name for step in route if step.shot]
    os.makedirs(OUT_DIR, exist_ok=True)
    for old in glob.glob(os.path.join(OUT_DIR, "*.png")):
        os.remove(old)
    kept = []
    for index, path in enumerate(shots):
        name = names[index] if index < len(names) else "extra%d" % index
        target = os.path.join(OUT_DIR, "%02d-%s.png" % (index + 1, name))
        shutil.copy2(path, target)
        kept.append(os.path.relpath(target, ROOT))
    return kept, len(names)


def crashes(since):
    if not os.path.isdir(CRASH_DIR):
        return []
    return [name for name in os.listdir(CRASH_DIR)
            if os.path.getmtime(os.path.join(CRASH_DIR, name)) > since]


def run(args):
    if not os.path.exists(game_perf.EXE):
        print("no packaged build at %s (.claude/guides/distribution.md)" % game_perf.EXE, file=sys.stderr)
        return 1
    commands, last = schedule(ROUTE)
    if args.dry_run:
        print('%s L_Title -ExecCmds="%s"' % (os.path.basename(game_perf.EXE), ", ".join(commands)))
        print("%d commands, the last at %.0f s" % (len(commands), last))
        return 0
    since = time.time() - 1.0
    print("playing through (%d commands, about %.0f s)..." % (len(commands), last))
    _, took = game_perf.launch("L_Title", commands, args.timeout)
    if took is None:
        return 1
    print("  the game ran %.0f s" % took)
    return report(since, quit_expected=took < args.timeout - 5)


def report(since, quit_expected=True):
    rows = read_status(game_perf.LOG)
    ok = timeline(ROUTE, rows)
    kept, wanted_shots = keep_shots(ROUTE, since)
    print("\n%d of %d pictures in %s" % (len(kept), wanted_shots, os.path.relpath(OUT_DIR, ROOT)))
    for path in kept:
        print("  " + path)
    if len(kept) != wanted_shots:
        ok = False
    left = crashes(since)
    if left:
        print("crashed: %s" % ", ".join(left), file=sys.stderr)
        ok = False
    else:
        print("no crash report")
    if not quit_expected:
        print("the game did not close itself", file=sys.stderr)
        ok = False
    print("\n%s" % ("the playthrough went through" if ok else "something did not land (see the table)"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("run")
    p.add_argument("--timeout", type=int, default=420, help="seconds to wait for the game to close itself")
    p.add_argument("--dry-run", action="store_true")
    p.set_defaults(func=run)
    t = sub.add_parser("timeline")
    t.set_defaults(func=lambda a: report(0, quit_expected=True))
    args = ap.parse_args()
    if args.cmd != "run" or args.dry_run:
        return args.func(args)
    if not game_perf.take_lock():
        return 1
    try:
        return args.func(args)
    finally:
        game_perf.drop_lock()


if __name__ == "__main__":
    sys.exit(main())
