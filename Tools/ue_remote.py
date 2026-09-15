"""Runs Python in the running Unreal Editor through PythonScriptPlugin's remote execution.

    python Tools/ue_remote.py <script.py>
    python Tools/ue_remote.py -c "<code>"

Needs Project Settings > Plugins > Python > Enable Remote Execution (bRemoteExecution, on in this project). The code
runs as a file with separate globals and locals, so top-level names are not visible inside comprehensions or
functions: put the work in a function and call it. Exit code 0 on success, 1 on a Python error, 2 when no editor answers.
"""
import os
import sys
import time

ENGINE = os.environ.get("UE_ENGINE_DIR", r"C:\Program Files\Epic Games\UE_5.8\Engine")
sys.path.insert(0, os.path.join(ENGINE, "Plugins", "Experimental", "PythonScriptPlugin", "Content", "Python"))
import remote_execution as rx  # noqa: E402


def main() -> int:
    if len(sys.argv) >= 3 and sys.argv[1] == "-c":
        code = sys.argv[2]
    elif len(sys.argv) == 2:
        with open(sys.argv[1], encoding="utf-8") as f:
            code = f.read()
    else:
        print(__doc__, file=sys.stderr)
        return 2
    ex = rx.RemoteExecution()
    ex.start()
    try:
        deadline = time.time() + 8
        while time.time() < deadline and not ex.remote_nodes:
            time.sleep(0.2)
        if not ex.remote_nodes:
            print("no editor answered (is it running with remote execution on?)", file=sys.stderr)
            return 2
        ex.open_command_connection(ex.remote_nodes[0]["node_id"])
        res = ex.run_command(code, unattended=True, exec_mode=rx.MODE_EXEC_FILE)
        for entry in res.get("output", []):
            sys.stdout.write(entry.get("output", ""))
        if not res.get("success"):
            print("\nFAILED: %s" % res.get("result"), file=sys.stderr)
            return 1
        return 0
    finally:
        ex.stop()


if __name__ == "__main__":
    sys.exit(main())
