#!/usr/bin/env python
"""SessionStart hook: 中断からの再開のために、未完了の進捗記録・main 以外のブランチ・直前のコミット・
未コミットの変更を知らせる（`.claude/guides/progress-tracking.md` の「再開の手順」）。
"""
import json
import os
import re
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PROGRESS_DIR = os.path.join(ROOT, ".claude", "progress")


def git(*args):
    try:
        return subprocess.run(["git", "-C", ROOT, *args], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=20).stdout.strip()
    except Exception:
        return ""


def section(text, heading):
    m = re.search(r"^## %s\s*\n(.*?)(?=^## |\Z)" % re.escape(heading), text, re.S | re.M)
    return " ".join(m.group(1).split()) if m else ""


def main():
    lines = []
    records = []
    if os.path.isdir(PROGRESS_DIR):
        records = sorted(f for f in os.listdir(PROGRESS_DIR) if f.endswith(".md") and f != "_template.md")
    if records:
        lines.append("未完了の進捗記録があります。**`/continue` スキルで再開してください**（照合の手順は `.claude/skills/continue/SKILL.md`）:")
        for name in records:
            with open(os.path.join(PROGRESS_DIR, name), encoding="utf-8") as f:
                text = f.read()
            title = re.search(r"^title:\s*(.+)$", text, re.M)
            nxt = section(text, "次にやること")
            lines.append("- .claude/progress/%s — %s" % (name, title.group(1).strip() if title else ""))
            if nxt:
                lines.append("  次にやること: " + nxt[:300])
    else:
        lines.append("未完了の進捗記録はありません（続きを頼まれたら `/continue`: handover の「次の一歩」から始め、進捗記録を作る）。")

    branch = git("branch", "--show-current")
    others = [b.strip().lstrip("* ").strip() for b in git("branch", "--format=%(refname:short)").split("\n") if b.strip() and b.strip() != "main"]
    if others:
        lines.append("main 以外のローカルブランチ: " + ", ".join(others) + "（未完了の大規模改修かもしれません）")
    dirty = git("status", "--short")
    if dirty:
        lines.append("未コミットの変更: %d 件" % len(dirty.split("\n")))
    lines.append("いまのブランチ: %s / 直前のコミット: %s" % (branch or "?", git("log", "-1", "--format=%h %s") or "?"))

    print(json.dumps({"hookSpecificOutput": {"hookEventName": "SessionStart", "additionalContext": "\n".join(lines)}}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception:
        sys.exit(0)
