"""Drives PIE in the running editor for recordings that are compared with the reference game
(.claude/guides/observation.md), through Python remote execution (Tools/ue_remote.py).

    python Tools/pie.py state                                  level, PIE or not, unsaved packages, game time, player
    python Tools/pie.py start [--timeout 30] [--warmup 3]      begin PIE in the level open in the editor and wait
    python Tools/pie.py place X Y [Z] [--yaw -90] [--pitch 0]  move the player (Z defaults to 90.15, the capsule
                                                               centre on the hospital floor) and turn the view
    python Tools/pie.py cmd "slomo 0.25" ["..."]               console commands in the game world
    python Tools/pie.py stop [--timeout 15]                    end PIE and report unsaved packages

Always finish with "stop": a PIE left running makes asset operations fail. Keys still go through Tools/desktop.py
(click the viewport once with --allow UnrealEditor.exe first). Exit code 0 on success, 1 on a failure, 2 when no editor
answers.
"""
import argparse
import os
import subprocess
import sys
import textwrap
import time

UE_REMOTE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ue_remote.py")

# ue_remote.py runs the code with separate globals and locals, so top-level names are invisible inside functions:
# every request becomes the body of one function, with these helpers nested in it.
PRELUDE = """
import unreal
def _les():
    return unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def _game():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def _dirty():
    return ([p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
            [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
def _need_game():
    w = _game()
    if w is None:
        raise RuntimeError('not in PIE')
    return w
"""

STATE = """
editor_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
w = _game()
# during PIE the editor world reads as None; the game world is UEDPIE_<n>_<level>
print('level:', editor_world.get_name() if editor_world else (w.get_name() if w else '?'))
print('pie:', _les().is_in_play_in_editor() and w is not None)
maps, content = _dirty()
print('dirty maps:', maps)
print('dirty content:', content)
if w is not None:
    print('game time: %.3f' % unreal.GameplayStatics.get_time_seconds(w))
    print('time dilation: %.3f' % unreal.GameplayStatics.get_global_time_dilation(w))
    player = unreal.GameplayStatics.get_player_character(w, 0)
    if player is not None:
        loc = player.get_actor_location()
        rot = unreal.GameplayStatics.get_player_controller(w, 0).get_control_rotation()
        print('player: (%.2f, %.2f, %.2f) yaw %.2f pitch %.2f' % (loc.x, loc.y, loc.z, rot.yaw, rot.pitch))
"""

PLACE = """
w = _need_game()
player = unreal.GameplayStatics.get_player_character(w, 0)
player.set_actor_location(unreal.Vector({x!r}, {y!r}, {z!r}), False, True)
unreal.GameplayStatics.get_player_controller(w, 0).set_control_rotation(
    unreal.Rotator(roll=0.0, pitch={pitch!r}, yaw={yaw!r}))
loc = player.get_actor_location()
print('placed: (%.2f, %.2f, %.2f) yaw {yaw} pitch {pitch}' % (loc.x, loc.y, loc.z))
"""

COMMANDS = """
w = _need_game()
for command in {commands!r}:
    unreal.SystemLibrary.execute_console_command(w, command)
    print('ran:', command)
print('time dilation: %.3f' % unreal.GameplayStatics.get_global_time_dilation(w))
"""


def remote(body):
    """Runs body (statements) in the editor inside one function; returns (exit code, output)."""
    code = "def _pie_main():\n" + textwrap.indent(PRELUDE + body, "    ") + "\n_pie_main()\n"
    proc = subprocess.run([sys.executable, UE_REMOTE, "-c", code], capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    lines = (proc.stdout + proc.stderr).splitlines()
    return proc.returncode, "\n".join(line for line in lines if line.strip())


def in_pie():
    code, out = remote("print('PIE', _les().is_in_play_in_editor() and _game() is not None)")
    if code != 0:
        return None, out
    return "PIE True" in out, out


def wait_for(want, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        state, out = in_pie()
        if state is None:
            print(out, file=sys.stderr)
            return False
        if state == want:
            return True
        time.sleep(0.5)
    return False


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("state")
    p = sub.add_parser("start")
    p.add_argument("--timeout", type=float, default=30.0)
    p.add_argument("--warmup", type=float, default=3.0, help="seconds to wait after PIE is up (shaders, streaming)")
    p = sub.add_parser("place")
    p.add_argument("x", type=float)
    p.add_argument("y", type=float)
    p.add_argument("z", type=float, nargs="?", default=90.15)
    p.add_argument("--yaw", type=float, default=-90.0)
    p.add_argument("--pitch", type=float, default=0.0)
    p = sub.add_parser("cmd")
    p.add_argument("commands", nargs="+")
    p = sub.add_parser("stop")
    p.add_argument("--timeout", type=float, default=15.0)
    args = ap.parse_args()

    if args.cmd == "state":
        code, out = remote(STATE)
        print(out)
        return code

    if args.cmd == "start":
        state, out = in_pie()
        if state is None:
            print(out, file=sys.stderr)
            return 2
        if not state:
            code, out = remote("_les().editor_request_begin_play()")
            if code != 0:
                print(out, file=sys.stderr)
                return 1
            if not wait_for(True, args.timeout):
                print("PIE did not start within %.0f s" % args.timeout, file=sys.stderr)
                return 1
        time.sleep(args.warmup)
        code, out = remote(STATE)
        print(out)
        return code

    if args.cmd == "place":
        code, out = remote(PLACE.format(x=args.x, y=args.y, z=args.z, yaw=args.yaw, pitch=args.pitch))
        print(out)
        return code

    if args.cmd == "cmd":
        code, out = remote(COMMANDS.format(commands=args.commands))
        print(out)
        return code

    if args.cmd == "stop":
        state, out = in_pie()
        if state is None:
            print(out, file=sys.stderr)
            return 2
        if state:
            code, out = remote("_les().editor_request_end_play()")
            if code != 0:
                print(out, file=sys.stderr)
                return 1
            if not wait_for(False, args.timeout):
                print("PIE did not end within %.0f s" % args.timeout, file=sys.stderr)
                return 1
        code, out = remote(STATE)
        print(out)
        return code
    return 1


if __name__ == "__main__":
    sys.exit(main())
