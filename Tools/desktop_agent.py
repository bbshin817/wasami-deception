"""Runs on the interactive desktop (Windows session 1) and performs screen captures and input for Claude.

Claude Code lives in session 0, which cannot see or touch the logged-on user's desktop (session isolation), so this
agent is started there by Tools/console_session.py and takes its orders through a spool directory:

    Intermediate/DesktopAgent/in/<id>.json     one request, written atomically by Tools/desktop.py
    Intermediate/DesktopAgent/out/<id>.json    the answer
    Intermediate/DesktopAgent/shots/*.png      screenshots
    Intermediate/DesktopAgent/agent.log        every action, with the foreground window it went to

Guard rails: input is only sent while the foreground window belongs to an allowed process (the reference game by
default), keys that act on the whole OS are refused, and the agent exits by itself once it has been idle for a while
so it is never left running. Start it with Tools/desktop.py, not by hand.
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import shutil
import subprocess
import sys
import time

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SPOOL = os.path.join(ROOT, "Intermediate", "DesktopAgent")
IN_DIR, OUT_DIR, SHOT_DIR = os.path.join(SPOOL, "in"), os.path.join(SPOOL, "out"), os.path.join(SPOOL, "shots")
LOG = os.path.join(SPOOL, "agent.log")
PID = os.path.join(SPOOL, "agent.pid")

DEFAULT_ALLOW = ["ddeception-win64-shipping.exe", "ddeception.exe"]
IDLE_EXIT_SECONDS = 1800

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

# Scan codes (set 1). 0xE0xx are the extended keys.
SCAN = {
    "esc": 0x01, "1": 0x02, "2": 0x03, "3": 0x04, "4": 0x05, "5": 0x06, "6": 0x07, "7": 0x08, "8": 0x09, "9": 0x0A,
    "0": 0x0B, "minus": 0x0C, "equals": 0x0D, "backspace": 0x0E, "tab": 0x0F,
    "q": 0x10, "w": 0x11, "e": 0x12, "r": 0x13, "t": 0x14, "y": 0x15, "u": 0x16, "i": 0x17, "o": 0x18, "p": 0x19,
    "enter": 0x1C, "ctrl": 0x1D, "a": 0x1E, "s": 0x1F, "d": 0x20, "f": 0x21, "g": 0x22, "h": 0x23, "j": 0x24,
    "k": 0x25, "l": 0x26, "shift": 0x2A, "z": 0x2C, "x": 0x2D, "c": 0x2E, "v": 0x2F, "b": 0x30, "n": 0x31, "m": 0x32,
    "alt": 0x38, "space": 0x39, "capslock": 0x3A,
    "f1": 0x3B, "f2": 0x3C, "f3": 0x3D, "f4": 0x3E, "f5": 0x3F, "f6": 0x40, "f7": 0x41, "f8": 0x42, "f9": 0x43,
    "f10": 0x44, "f11": 0x57, "f12": 0x58,
    "up": 0xE048, "left": 0xE04B, "right": 0xE04D, "down": 0xE050,
    "home": 0xE047, "end": 0xE04F, "pageup": 0xE049, "pagedown": 0xE051, "delete": 0xE053, "insert": 0xE052,
}
# Numpad digits go out as virtual keys (with their scan code), so they stay digits whatever the NumLock state is (a
# bare scan code turns into Home / End / … while NumLock is off). The reference game's mod menu toggles its cheats
# with them.
NUMPAD_VK = {"num%d" % d: 0x60 + d for d in range(10)}
NUMPAD_SCAN = {"num0": 0x52, "num1": 0x4F, "num2": 0x50, "num3": 0x51, "num4": 0x4B, "num5": 0x4C, "num6": 0x4D,
               "num7": 0x47, "num8": 0x48, "num9": 0x49}
BLOCKED_KEYS = {"win", "lwin", "rwin"}
BLOCKED_WITH_ALT = {"tab", "f4", "esc"}

INPUT_MOUSE, INPUT_KEYBOARD = 0, 1
KEYEVENTF_KEYUP, KEYEVENTF_SCANCODE, KEYEVENTF_EXTENDEDKEY, KEYEVENTF_UNICODE = 0x0002, 0x0008, 0x0001, 0x0004
MOUSEEVENTF = {
    "move": 0x0001, "absolute": 0x8000, "left_down": 0x0002, "left_up": 0x0004, "right_down": 0x0008,
    "right_up": 0x0010, "middle_down": 0x0020, "middle_up": 0x0040, "wheel": 0x0800,
}


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wt.LONG), ("dy", wt.LONG), ("mouseData", wt.DWORD), ("dwFlags", wt.DWORD),
                ("time", wt.DWORD), ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wt.WORD), ("wScan", wt.WORD), ("dwFlags", wt.DWORD), ("time", wt.DWORD),
                ("dwExtraInfo", ctypes.POINTER(wt.ULONG))]


class _INPUTunion(ctypes.Union):
    _fields_ = [("mi", MOUSEINPUT), ("ki", KEYBDINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wt.DWORD), ("union", _INPUTunion)]


def send(*inputs):
    array = (INPUT * len(inputs))(*inputs)
    sent = user32.SendInput(len(inputs), array, ctypes.sizeof(INPUT))
    if sent != len(inputs):
        raise OSError("SendInput sent %d of %d (error %d)" % (sent, len(inputs), ctypes.get_last_error()))


def key_input(name, up=False):
    if name in NUMPAD_VK:
        return INPUT(type=INPUT_KEYBOARD,
                     union=_INPUTunion(ki=KEYBDINPUT(wVk=NUMPAD_VK[name], wScan=NUMPAD_SCAN[name],
                                                     dwFlags=KEYEVENTF_KEYUP if up else 0, time=0, dwExtraInfo=None)))
    scan = SCAN[name]
    flags = KEYEVENTF_SCANCODE | (KEYEVENTF_KEYUP if up else 0)
    if scan > 0xFF:
        flags |= KEYEVENTF_EXTENDEDKEY
        scan &= 0xFF
    return INPUT(type=INPUT_KEYBOARD,
                 union=_INPUTunion(ki=KEYBDINPUT(wVk=0, wScan=scan, dwFlags=flags, time=0, dwExtraInfo=None)))


def unicode_input(char, up=False):
    flags = KEYEVENTF_UNICODE | (KEYEVENTF_KEYUP if up else 0)
    return INPUT(type=INPUT_KEYBOARD,
                 union=_INPUTunion(ki=KEYBDINPUT(wVk=0, wScan=ord(char), dwFlags=flags, time=0, dwExtraInfo=None)))


def mouse_input(dx=0, dy=0, data=0, flags=0):
    return INPUT(type=INPUT_MOUSE,
                 union=_INPUTunion(mi=MOUSEINPUT(dx=dx, dy=dy, mouseData=data, dwFlags=flags, time=0,
                                                 dwExtraInfo=None)))


def screen_size():
    return user32.GetSystemMetrics(0), user32.GetSystemMetrics(1)


def foreground():
    hwnd = user32.GetForegroundWindow()
    if not hwnd:
        return {"hwnd": 0, "title": "", "process": "", "rect": None}
    length = user32.GetWindowTextLengthW(hwnd)
    buf = ctypes.create_unicode_buffer(length + 1)
    user32.GetWindowTextW(hwnd, buf, length + 1)
    rect = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(rect))
    pid = wt.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    name = ""
    handle = kernel32.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
    if handle:
        path = ctypes.create_unicode_buffer(1024)
        size = wt.DWORD(1024)
        if kernel32.QueryFullProcessImageNameW(handle, 0, path, ctypes.byref(size)):
            name = os.path.basename(path.value)
        kernel32.CloseHandle(handle)
    return {"hwnd": int(hwnd), "title": buf.value, "process": name, "pid": int(pid.value),
            "rect": [rect.left, rect.top, rect.right, rect.bottom]}


def check_target(req):
    """Input only goes to a window we are allowed to drive, never to whatever else the user has in front."""
    allow = [a.lower() for a in req.get("allow", DEFAULT_ALLOW)]
    front = foreground()
    if front["process"].lower() not in allow:
        raise PermissionError("the foreground window is %s (%s), not one of %s"
                              % (front["process"] or "unknown", front["title"], ", ".join(allow)))
    return front


def check_keys(names):
    names = [n.lower() for n in names]
    for name in names:
        if name in BLOCKED_KEYS:
            raise PermissionError("%s acts on the whole OS and is not sent" % name)
        if name not in SCAN and name not in NUMPAD_VK:
            raise KeyError("unknown key %r" % name)
    bad = BLOCKED_WITH_ALT & set(names)
    if "alt" in names and bad:
        raise PermissionError("alt with %s acts on the whole OS and is not sent" % ", ".join(sorted(bad)))
    return names


def do_shot(req):
    from PIL import Image, ImageGrab
    region = req.get("region")
    image = ImageGrab.grab(bbox=tuple(region) if region else None, all_screens=bool(req.get("all_screens")))
    scale = float(req.get("scale", 1.0))
    if scale != 1.0:
        image = image.resize((max(1, int(image.width * scale)), max(1, int(image.height * scale))), Image.LANCZOS)
    name = req.get("name") or ("shot-%s.png" % time.strftime("%H%M%S"))
    path = os.path.join(SHOT_DIR, name)
    image.save(path)
    all_black = image.convert("L").getextrema() == (0, 0)
    return {"path": path, "size": [image.width, image.height], "all_black": all_black, "foreground": foreground()}


def do_click(req):
    front = check_target(req)
    width, height = screen_size()
    button = req.get("button", "left")
    if "x" in req and "y" in req:
        x = int(req["x"] * 65535 / max(1, width - 1))
        y = int(req["y"] * 65535 / max(1, height - 1))
        send(mouse_input(dx=x, dy=y, flags=MOUSEEVENTF["move"] | MOUSEEVENTF["absolute"]))
        time.sleep(0.05)
    for _ in range(int(req.get("count", 1))):
        send(mouse_input(flags=MOUSEEVENTF[button + "_down"]))
        time.sleep(0.04)
        send(mouse_input(flags=MOUSEEVENTF[button + "_up"]))
        time.sleep(0.08)
    return {"clicked": [req.get("x"), req.get("y")], "button": button, "window": front["title"]}


def do_look(req):
    """Relative mouse movement, in steps, which is what a game's mouse look reads."""
    check_target(req)
    steps = max(1, int(req.get("steps", 10)))
    dx, dy = int(req.get("dx", 0)), int(req.get("dy", 0))
    for _ in range(steps):
        send(mouse_input(dx=dx // steps, dy=dy // steps, flags=MOUSEEVENTF["move"]))
        time.sleep(float(req.get("step_ms", 16)) / 1000.0)
    return {"dx": dx, "dy": dy, "steps": steps}


def do_key(req):
    front = check_target(req)
    names = check_keys(req.get("keys", []))
    hold = float(req.get("hold_ms", 40)) / 1000.0
    for name in names:
        send(key_input(name))
        time.sleep(hold)
        send(key_input(name, up=True))
        time.sleep(float(req.get("gap_ms", 80)) / 1000.0)
    return {"keys": names, "window": front["title"]}


def do_combo(req):
    front = check_target(req)
    names = check_keys(req.get("keys", []))
    for name in names:
        send(key_input(name))
        time.sleep(0.03)
    time.sleep(float(req.get("hold_ms", 60)) / 1000.0)
    for name in reversed(names):
        send(key_input(name, up=True))
        time.sleep(0.03)
    return {"combo": names, "window": front["title"]}


def do_hold(req):
    front = check_target(req)
    names = check_keys(req.get("keys", []))
    for name in names:
        send(key_input(name))
    time.sleep(float(req.get("ms", 500)) / 1000.0)
    for name in reversed(names):
        send(key_input(name, up=True))
    return {"held": names, "ms": req.get("ms", 500), "window": front["title"]}


def do_type(req):
    front = check_target(req)
    text = str(req.get("text", ""))
    for char in text:
        send(unicode_input(char))
        time.sleep(0.02)
        send(unicode_input(char, up=True))
        time.sleep(0.02)
    return {"typed": len(text), "window": front["title"]}


def do_scroll(req):
    check_target(req)
    send(mouse_input(data=int(req.get("delta", 120)), flags=MOUSEEVENTF["wheel"]))
    return {"delta": req.get("delta", 120)}


def do_wait(req):
    time.sleep(float(req.get("ms", 100)) / 1000.0)
    return {"waited_ms": req.get("ms", 100)}


RECORDINGS = {}


def do_record(req):
    """Starts recording the screen to a video in the background (a single screenshot takes seconds, which is too slow
    for effects that last a fraction of a second). ffmpeg's Desktop Duplication grabber hands GPU frames straight to
    NVENC, so the game being watched keeps its frame rate. Returns at once; input commands can follow while it runs.

    grab "gdi" uses the CPU grabber instead (ddagrab sometimes never delivers its first frame); "region" (left, top,
    right, bottom) narrows it, which is what keeps it near 60 fps."""
    ffmpeg = shutil.which("ffmpeg") or r"C:\ffmpeg\bin\ffmpeg.exe"
    seconds, fps = float(req.get("seconds", 10)), int(req.get("fps", 60))
    name = req.get("name") or ("rec-%s.mkv" % time.strftime("%H%M%S"))
    path = os.path.join(SHOT_DIR, name)
    if req.get("grab", "dda") == "gdi":
        source = ["-f", "gdigrab", "-framerate", str(fps), "-draw_mouse", "0"]
        if req.get("region"):
            left, top, right, bottom = (int(v) for v in req["region"])
            source += ["-offset_x", str(left), "-offset_y", str(top),
                       "-video_size", "%dx%d" % (right - left, bottom - top)]
        source += ["-i", "desktop"]
    else:
        source = ["-f", "lavfi", "-i", "ddagrab=output_idx=%d:draw_mouse=0:framerate=%d"
                  % (int(req.get("output_idx", 0)), fps)]
    cmd = [ffmpeg, "-hide_banner", "-loglevel", req.get("loglevel", "warning"), "-y"] + source + [
        "-t", "%.3f" % seconds, "-c:v", "h264_nvenc", "-preset", "p1", "-rc", "constqp",
        "-qp", str(int(req.get("qp", 20))), path]
    errors = open(path + ".log", "w", encoding="utf-8")
    proc = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=errors, stderr=errors,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    RECORDINGS[proc.pid] = (proc, errors)
    return {"path": path, "pid": proc.pid, "seconds": seconds, "fps": fps, "started": time.time()}


def do_record_status(req):
    """Reports the recordings started by this agent: still running, or the exit code."""
    result = []
    for pid, (proc, errors) in list(RECORDINGS.items()):
        code = proc.poll()
        if code is not None:
            errors.close()
            del RECORDINGS[pid]
        result.append({"pid": pid, "running": code is None, "exit_code": code, "path": proc.args[-1]})
    return {"recordings": result}


def do_ping(req):
    ours = ctypes.c_ulong()
    kernel32.ProcessIdToSessionId(ctypes.c_ulong(os.getpid()), ctypes.byref(ours))
    return {"pid": os.getpid(), "session": ours.value, "console_session": kernel32.WTSGetActiveConsoleSessionId(),
            "screen": list(screen_size()), "foreground": foreground(), "allow_default": DEFAULT_ALLOW}


HANDLERS = {"ping": do_ping, "shot": do_shot, "click": do_click, "key": do_key, "combo": do_combo, "hold": do_hold,
            "look": do_look, "type": do_type, "scroll": do_scroll, "wait": do_wait, "record": do_record,
            "record_status": do_record_status}


def log(line):
    with open(LOG, "a", encoding="utf-8") as handle:
        handle.write("%s %s\n" % (time.strftime("%H:%M:%S"), line))


def answer(request_id, payload):
    tmp = os.path.join(OUT_DIR, request_id + ".tmp")
    with open(tmp, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, ensure_ascii=False)
    os.replace(tmp, os.path.join(OUT_DIR, request_id + ".json"))


def main():
    for path in (IN_DIR, OUT_DIR, SHOT_DIR):
        os.makedirs(path, exist_ok=True)
    user32.SetProcessDPIAware()
    with open(PID, "w", encoding="utf-8") as handle:
        handle.write(str(os.getpid()))
    log("agent started (pid %d, screen %dx%d)" % (os.getpid(), screen_size()[0], screen_size()[1]))
    last_seen = time.time()
    try:
        while True:
            names = sorted(name for name in os.listdir(IN_DIR) if name.endswith(".json"))
            if not names:
                if time.time() - last_seen > IDLE_EXIT_SECONDS:
                    log("idle for %d s, exiting" % IDLE_EXIT_SECONDS)
                    return 0
                time.sleep(0.05)
                continue
            last_seen = time.time()
            for name in names:
                path = os.path.join(IN_DIR, name)
                request_id = name[:-len(".json")]
                try:
                    with open(path, encoding="utf-8") as handle:
                        req = json.load(handle)
                except Exception as error:
                    os.remove(path)
                    answer(request_id, {"ok": False, "error": "unreadable request: %s" % error})
                    continue
                os.remove(path)
                cmd = req.get("cmd", "")
                if cmd == "stop":
                    log("stop requested")
                    answer(request_id, {"ok": True, "result": {"stopped": True}})
                    return 0
                handler = HANDLERS.get(cmd)
                if handler is None:
                    answer(request_id, {"ok": False, "error": "unknown command %r" % cmd})
                    continue
                try:
                    result = handler(req)
                    answer(request_id, {"ok": True, "result": result})
                    log("%s %s -> ok" % (cmd, json.dumps({k: v for k, v in req.items() if k != "cmd"},
                                                         ensure_ascii=False)))
                except Exception as error:
                    answer(request_id, {"ok": False, "error": "%s: %s" % (type(error).__name__, error)})
                    log("%s -> %s: %s" % (cmd, type(error).__name__, error))
    finally:
        try:
            os.remove(PID)
        except OSError:
            pass
        log("agent stopped")


if __name__ == "__main__":
    sys.exit(main())
