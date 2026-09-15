#!/usr/bin/env python
"""Stop hook: 実装記録がソースと同期していなければ停止をブロックし、更新を促す。

stop_hook_active（既にこの hook で継続中）のときは無限ループ防止のため警告だけ出して通す。
チェック自体が失敗したときも通す（作業を止めない）。
"""
import json
import os
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
CHECK = os.path.join(ROOT, ".claude", "scripts", "check_records.py")


def main():
    data = {}
    try:
        data = json.load(sys.stdin)
    except Exception:
        pass
    try:
        res = subprocess.run([sys.executable, CHECK], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=45)
    except Exception:
        return 0
    if res.returncode == 0:
        return 0

    msg = ("実装記録(.claude/implementation-records/)がソースと同期していません。\n"
           "該当する記録の本文を現行実装に合わせて直し、最後に `python .claude/scripts/check_records.py --update` を実行してください。\n"
           "(運用ルール: .claude/guides/implementation-records.md)\n\n" + (res.stdout or "") + (res.stderr or ""))
    if data.get("stop_hook_active"):
        print(msg)
        return 0
    print(msg, file=sys.stderr)
    return 2


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception:
        sys.exit(0)
