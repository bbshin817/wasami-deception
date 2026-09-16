"""Drives the interactive desktop (screenshots and input) from Claude's session through Tools/desktop_agent.py.

    python Tools/desktop.py start                     start the agent on the interactive desktop (idempotent)
    python Tools/desktop.py ping                      session, screen size, the window in front
    python Tools/desktop.py shot --scale 0.5           capture the screen, save a PNG, print its path
    python Tools/desktop.py shot --region 0 0 800 600  capture a box (left top right bottom)
    python Tools/desktop.py click 960 540              move there and click
    python Tools/desktop.py key esc enter              press keys one after another
    python Tools/desktop.py combo ctrl s               press together, release in reverse
    python Tools/desktop.py hold w --ms 1500           hold keys down (walking, sprinting)
    python Tools/desktop.py look --dx 300 --dy 0       relative mouse movement (mouse look)
    python Tools/desktop.py type "some text"
    python Tools/desktop.py record --seconds 8 --name x.mkv   record the screen at 60 fps in the background
    python Tools/desktop.py record_status              are the recordings still running? (exit code when done)
    python Tools/desktop.py status                     is the agent running?
    python Tools/desktop.py stop                       stop the agent

Input is only delivered while the foreground window belongs to an allowed process — the reference game by default.
Pass --allow <image.exe> (repeatable) for anything else, and ask the user first: the editor and PIE are their app
(`.claude/guides/verification.md`). Exit code 0 when the agent answered ok, 1 otherwise.
"""
import argparse
import json
import os
import subprocess
import sys
import time
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import console_session  # noqa: E402  (same folder)

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SPOOL = os.path.join(ROOT, "Intermediate", "DesktopAgent")
IN_DIR, OUT_DIR = os.path.join(SPOOL, "in"), os.path.join(SPOOL, "out")
PID = os.path.join(SPOOL, "agent.pid")
AGENT = os.path.join(ROOT, "Tools", "desktop_agent.py")
PYTHONW = os.path.join(os.path.dirname(sys.executable), "pythonw.exe")
TASK = "WasamiDesktopAgent"


def request(cmd, timeout=30, **payload):
    """Puts one request in the spool and waits for the agent's answer."""
    for path in (IN_DIR, OUT_DIR):
        os.makedirs(path, exist_ok=True)
    request_id = "%d-%s" % (time.time() * 1000, uuid.uuid4().hex[:6])
    payload["cmd"] = cmd
    tmp = os.path.join(IN_DIR, request_id + ".tmp")
    with open(tmp, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, ensure_ascii=False)
    os.replace(tmp, os.path.join(IN_DIR, request_id + ".json"))
    answer_path = os.path.join(OUT_DIR, request_id + ".json")
    deadline = time.time() + timeout
    while time.time() < deadline:
        if os.path.exists(answer_path):
            try:
                with open(answer_path, encoding="utf-8") as handle:
                    answer = json.load(handle)
            except (PermissionError, ValueError):
                time.sleep(0.05)  # the agent is replacing the file right now
                continue
            os.remove(answer_path)
            return answer
        time.sleep(0.05)
    return {"ok": False, "error": "the agent did not answer within %d s (is it running? try 'start')" % timeout}


def agent_running():
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq pythonw.exe", "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    return "pythonw.exe" in out and os.path.exists(PID)


def start(timeout=90):
    if agent_running() and request("ping", timeout=5).get("ok"):
        print("agent: already running")
        return True
    for path in (IN_DIR, OUT_DIR):
        os.makedirs(path, exist_ok=True)
    if not os.path.exists(PYTHONW):
        print("pythonw.exe was not found next to %s" % sys.executable, file=sys.stderr)
        return False
    if not console_session.start(PYTHONW, [AGENT], task=TASK):
        return False
    deadline = time.time() + timeout
    while time.time() < deadline:
        answer = request("ping", timeout=5)
        if answer.get("ok"):
            console_session.drop_task(TASK)
            print(json.dumps(answer["result"], ensure_ascii=False, indent=2))
            return True
        time.sleep(2)
    console_session.drop_task(TASK)
    print("agent: did not answer within %d s" % timeout, file=sys.stderr)
    return False


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", help="start / status / stop / ping / shot / click / key / combo / hold / look / type / "
                               "scroll / wait / record / record_status")
    ap.add_argument("args", nargs="*", help="keys for key/combo/hold, X Y for click, the text for type")
    ap.add_argument("--allow", action="append", help="image name of a process whose window may receive the input")
    ap.add_argument("--scale", type=float, default=1.0)
    ap.add_argument("--name")
    ap.add_argument("--region", nargs=4, type=int, metavar=("LEFT", "TOP", "RIGHT", "BOTTOM"))
    ap.add_argument("--all-screens", action="store_true")
    ap.add_argument("--button", default="left")
    ap.add_argument("--count", type=int, default=1)
    ap.add_argument("--ms", type=int, default=500)
    ap.add_argument("--hold-ms", type=int, default=40)
    ap.add_argument("--gap-ms", type=int, default=80)
    ap.add_argument("--dx", type=int, default=0)
    ap.add_argument("--dy", type=int, default=0)
    ap.add_argument("--steps", type=int, default=10)
    ap.add_argument("--timeout", type=int, default=30)
    ap.add_argument("--seconds", type=float, default=10.0)
    ap.add_argument("--fps", type=int, default=60)
    opts = ap.parse_args()

    if opts.cmd == "start":
        return 0 if start() else 1
    if opts.cmd == "status":
        print(json.dumps({"pid_file": os.path.exists(PID), "pythonw_running": agent_running()}, ensure_ascii=False))
        answer = request("ping", timeout=5)
        print(json.dumps(answer, ensure_ascii=False, indent=2))
        return 0 if answer.get("ok") else 1

    payload = {}
    if opts.allow:
        payload["allow"] = opts.allow
    if opts.cmd == "shot":
        payload.update(scale=opts.scale, all_screens=opts.all_screens)
        if opts.name:
            payload["name"] = opts.name
        if opts.region:
            payload["region"] = opts.region
    elif opts.cmd == "click":
        if len(opts.args) >= 2:
            payload.update(x=int(opts.args[0]), y=int(opts.args[1]))
        payload.update(button=opts.button, count=opts.count)
    elif opts.cmd in ("key", "combo"):
        payload.update(keys=opts.args, hold_ms=opts.hold_ms, gap_ms=opts.gap_ms)
    elif opts.cmd == "hold":
        payload.update(keys=opts.args, ms=opts.ms)
    elif opts.cmd == "look":
        payload.update(dx=opts.dx, dy=opts.dy, steps=opts.steps)
    elif opts.cmd == "type":
        payload["text"] = " ".join(opts.args)
    elif opts.cmd == "scroll":
        payload["delta"] = opts.dx or 120
    elif opts.cmd == "wait":
        payload["ms"] = opts.ms
    elif opts.cmd == "record":
        payload.update(seconds=opts.seconds, fps=opts.fps)
        if opts.name:
            payload["name"] = opts.name

    answer = request(opts.cmd, timeout=opts.timeout, **payload)
    print(json.dumps(answer, ensure_ascii=False, indent=2))
    return 0 if answer.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
