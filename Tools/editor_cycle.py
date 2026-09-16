"""Closes the running Unreal Editor (saving every dirty package first), builds the C++ editor target, starts the editor
again and waits until it answers.

    python Tools/editor_cycle.py                  quit, build, launch, wait
    python Tools/editor_cycle.py --quit-only      save and quit
    python Tools/editor_cycle.py --no-quit        build, launch, wait (the editor is already closed)
    python Tools/editor_cycle.py --no-build       quit, launch, wait

Quitting goes through Python remote execution (Tools/ue_remote.py). Starting has to happen on the interactive desktop:
when this script runs in Windows' session 0 (where Claude Code lives), a directly launched editor dies at once with
DXGI_ERROR_NOT_CURRENTLY_AVAILABLE because that session has no display outputs — so the editor is started through a
one-off scheduled task that runs as the logged-on user with an interactive token. Exit code 0 on success; 1 when the
build fails (the editor is then left closed); 2 when the editor does not quit or does not come up in time.
"""
import argparse
import ctypes
import os
import subprocess
import sys
import time

# The build's output can hold characters this console's code page has no room for (cp932 on this PC): print what we
# can rather than dying with a UnicodeEncodeError on the way to reporting a build failure.
for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(errors="replace")

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
UPROJECT = os.path.join(ROOT, "wasami_deception.uproject")
ENGINE = os.environ.get("UE_ENGINE_DIR", r"C:\Program Files\Epic Games\UE_5.8\Engine")
EDITOR = os.path.join(ENGINE, "Binaries", "Win64", "UnrealEditor.exe")
BUILD = os.path.join(ENGINE, "Build", "BatchFiles", "Build.bat")
# The one-off scheduled task that starts the editor on the interactive desktop. Its principal is the logged-on user
# by SID (the name form fails on a PC with no domain) with an interactive token, so the editor lands in that user's
# session and can see the display. launch() removes the task again once the editor answers.
TASK = "WasamiLaunchEditor"
LAUNCH_TASK_PS = """
$ErrorActionPreference = "Stop"
$sid = ([Security.Principal.WindowsIdentity]::GetCurrent()).User.Value
$action = New-ScheduledTaskAction -Execute "{editor}" -Argument '"{uproject}"'
$principal = New-ScheduledTaskPrincipal -UserId $sid -LogonType Interactive -RunLevel Limited
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit ([TimeSpan]::Zero)
Register-ScheduledTask -TaskName "{task}" -Action $action -Principal $principal -Settings $settings -Force | Out-Null
Start-ScheduledTask -TaskName "{task}"
"""


def editor_running():
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq UnrealEditor.exe", "/FO", "CSV", "/NH"], capture_output=True, text=True).stdout
    return "UnrealEditor.exe" in out


def quit_editor(timeout=240):
    if not editor_running():
        print("editor: not running")
        return True
    code = "import unreal\nunreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)\nunreal.SystemLibrary.quit_editor()\n"
    subprocess.run([sys.executable, os.path.join(ROOT, "Tools", "ue_remote.py"), "-c", code])
    deadline = time.time() + timeout
    while time.time() < deadline:
        if not editor_running():
            print("editor: closed")
            return True
        time.sleep(2)
    print("editor: still running after %d s" % timeout, file=sys.stderr)
    return False


def build():
    cmd = [BUILD, "wasami_deceptionEditor", "Win64", "Development", "-Project=" + UPROJECT, "-WaitMutex"]
    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = (proc.stdout + proc.stderr).splitlines()
    problems = [l for l in lines if " error " in l or ": error" in l or "warning C" in l]
    for l in problems[:60]:
        print(l)
    print("\n".join(lines[-12:]))
    print("build: %s" % ("ok" if proc.returncode == 0 else "FAILED (%d)" % proc.returncode))
    return proc.returncode == 0


def session_ids():
    """(this process's Windows session, the session at the console) — they differ when we run as a service."""
    ours = ctypes.c_ulong()
    ctypes.windll.kernel32.ProcessIdToSessionId(ctypes.c_ulong(os.getpid()), ctypes.byref(ours))
    return ours.value, ctypes.windll.kernel32.WTSGetActiveConsoleSessionId()


def start_editor():
    """Starts the editor, on the interactive desktop when we are not on it ourselves."""
    ours, console = session_ids()
    if ours == console:
        flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
        subprocess.Popen([EDITOR, UPROJECT], creationflags=flags, close_fds=True)
        return True
    # Session 0 has no display outputs, so the editor has to be started as the logged-on user. A scheduled task with
    # an interactive principal does that; it is removed again once the editor is up (see wait_for_editor).
    script = LAUNCH_TASK_PS.format(editor=EDITOR, uproject=UPROJECT, task=TASK)
    done = run_powershell(script)
    print("editor: started through the scheduled task (session %d, console session %d)" % (ours, console))
    return done


def run_powershell(script):
    proc = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", script],
                          capture_output=True, text=True, encoding="utf-8", errors="replace")
    if proc.returncode != 0:
        print((proc.stdout + proc.stderr).strip(), file=sys.stderr)
    return proc.returncode == 0


def drop_task():
    run_powershell('Unregister-ScheduledTask -TaskName "%s" -Confirm:$false -ErrorAction SilentlyContinue' % TASK)


def editor_answers():
    """Whether the editor's Python remote execution answers (the MCP port cannot be used: Docker Desktop holds
    127.0.0.1:8000 on this PC, so it answers even with no editor running)."""
    proc = subprocess.run([sys.executable, os.path.join(ROOT, "Tools", "ue_remote.py"), "-c", "print('up')"],
                          capture_output=True, text=True)
    return proc.returncode == 0


def launch(timeout=1200):
    if not start_editor():
        print("editor: could not be started", file=sys.stderr)
        return False
    start = time.time()
    while time.time() - start < timeout:
        if editor_running() and editor_answers():
            print("editor: answering after %d s" % (time.time() - start))
            drop_task()
            return True
        time.sleep(5)
    drop_task()
    print("editor: not answering after %d s" % timeout, file=sys.stderr)
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quit-only", action="store_true")
    ap.add_argument("--no-quit", action="store_true")
    ap.add_argument("--no-build", action="store_true")
    args = ap.parse_args()
    if not args.no_quit and not quit_editor():
        return 2
    if args.quit_only:
        return 0
    if not args.no_build and not build():
        return 1
    return 0 if launch() else 2


if __name__ == "__main__":
    sys.exit(main())
