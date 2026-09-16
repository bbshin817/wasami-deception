"""Closes the running Unreal Editor (saving every dirty package first), builds the C++ editor target, starts the editor
again and waits until its MCP server answers.

    python Tools/editor_cycle.py                  quit, build, launch, wait
    python Tools/editor_cycle.py --quit-only      save and quit
    python Tools/editor_cycle.py --no-quit        build, launch, wait (the editor is already closed)
    python Tools/editor_cycle.py --no-build       quit, launch, wait

Quitting goes through Python remote execution (Tools/ue_remote.py). Exit code 0 on success; 1 when the build fails
(the editor is then left closed); 2 when the editor does not quit or its MCP server does not come up in time.
"""
import argparse
import os
import socket
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
MCP = ("127.0.0.1", 8000)


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


def mcp_up():
    try:
        with socket.create_connection(MCP, timeout=2):
            return True
    except OSError:
        return False


def launch(timeout=1200):
    flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
    subprocess.Popen([EDITOR, UPROJECT], creationflags=flags, close_fds=True)
    start = time.time()
    while time.time() - start < timeout:
        if mcp_up():
            print("editor: MCP answering after %d s" % (time.time() - start))
            return True
        time.sleep(5)
    print("editor: MCP not answering after %d s" % timeout, file=sys.stderr)
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
