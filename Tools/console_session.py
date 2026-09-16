"""Starts a program on the interactive desktop (the console Windows session).

    python Tools/console_session.py <exe> [args ...]
    python Tools/console_session.py --wait steam.exe "C:\Program Files (x86)\Steam\steam.exe"

Claude Code runs in Windows' session 0, which has no display outputs: a GUI program started from there dies at once
(the editor fails with DXGI_ERROR_NOT_CURRENTLY_AVAILABLE). This starts the program through a one-off scheduled task
whose principal is the logged-on user by SID with an interactive token, so it lands in that user's session and can see
the display; the task is removed again afterwards. When we already are on the console session the program is started
directly. Exit code 0 on success, 1 when the task could not be created, 2 when --wait timed out.

Tools/editor_cycle.py has its own copy of this for the editor (it also needs to build and to wait for remote execution).
"""
import argparse
import ctypes
import os
import subprocess
import sys
import time

TASK = "WasamiConsoleSession"
TASK_PS = """
$ErrorActionPreference = "Stop"
$sid = ([Security.Principal.WindowsIdentity]::GetCurrent()).User.Value
$action = New-ScheduledTaskAction -Execute {execute} {argument}
$principal = New-ScheduledTaskPrincipal -UserId $sid -LogonType Interactive -RunLevel Limited
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit ([TimeSpan]::Zero)
Register-ScheduledTask -TaskName "{task}" -Action $action -Principal $principal -Settings $settings -Force | Out-Null
Start-ScheduledTask -TaskName "{task}"
"""


def session_ids():
    """(this process's Windows session, the session at the console) — they differ when we run as a service."""
    ours = ctypes.c_ulong()
    ctypes.windll.kernel32.ProcessIdToSessionId(ctypes.c_ulong(os.getpid()), ctypes.byref(ours))
    return ours.value, ctypes.windll.kernel32.WTSGetActiveConsoleSessionId()


def run_powershell(script):
    proc = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", script],
                          capture_output=True, text=True, encoding="utf-8", errors="replace")
    if proc.returncode != 0:
        print((proc.stdout + proc.stderr).strip(), file=sys.stderr)
    return proc.returncode == 0


def ps_quote(text):
    return "'" + text.replace("'", "''") + "'"


def running(image):
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq " + image, "/FO", "CSV", "/NH"],
                         capture_output=True, text=True).stdout
    return image.lower() in out.lower()


def start(exe, args, task=TASK):
    ours, console = session_ids()
    if ours == console:
        flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
        subprocess.Popen([exe] + list(args), creationflags=flags, close_fds=True)
        print("started directly (session %d)" % ours)
        return True
    argument = ""
    if args:
        argument = "-Argument " + ps_quote(subprocess.list2cmdline(list(args)))
    script = TASK_PS.format(execute=ps_quote(exe), argument=argument, task=task)
    if not run_powershell(script):
        return False
    print("started through the scheduled task '%s' (session %d, console session %d)" % (task, ours, console))
    return True


def drop_task(task=TASK):
    run_powershell('Unregister-ScheduledTask -TaskName "%s" -Confirm:$false -ErrorAction SilentlyContinue' % task)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("args", nargs=argparse.REMAINDER)
    ap.add_argument("--wait", metavar="IMAGE", help="wait until this image name shows up in tasklist")
    ap.add_argument("--timeout", type=int, default=120)
    ap.add_argument("--task-name", default=TASK)
    opts = ap.parse_args()

    if not start(opts.exe, opts.args, opts.task_name):
        print("could not start %s" % opts.exe, file=sys.stderr)
        return 1
    if not opts.wait:
        time.sleep(3)
        drop_task(opts.task_name)
        return 0
    start_time = time.time()
    while time.time() - start_time < opts.timeout:
        if running(opts.wait):
            print("%s is up after %d s" % (opts.wait, time.time() - start_time))
            drop_task(opts.task_name)
            return 0
        time.sleep(2)
    drop_task(opts.task_name)
    print("%s did not show up within %d s" % (opts.wait, opts.timeout), file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
