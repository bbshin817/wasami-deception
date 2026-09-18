#!/usr/bin/env python
"""SessionStart hook: 中断からの再開のために、未完了の進捗記録・main 以外のブランチ・直前のコミット・
未コミットの変更を知らせる（`.claude/guides/progress-tracking.md` の「再開の手順」）。

無人モード（環境変数 WASAMI_UNATTENDED=1。駆動役 Tools/overnight.py が付ける）ならそれを先頭で知らせ、
未完了の記録の「要確認（ユーザー）」と前回の無人運転の状態ファイル（Intermediate/Overnight/status.json）も出す
（`.claude/guides/autonomy.md`）。
"""
import json
import os
import re
import subprocess
import sys

# Claude Code は hook の出力を UTF-8 として読む。この PC のコンソールは cp932 で、「—」のような cp932 に無い文字が
# 1 つでもあると print が UnicodeEncodeError になり、外側の try が握りつぶして出力ごと消えていた（2026-09-17 に発見）。
for _stream in (sys.stdout, sys.stderr):
    if hasattr(_stream, "reconfigure"):
        _stream.reconfigure(encoding="utf-8", errors="replace")

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PROGRESS_DIR = os.path.join(ROOT, ".claude", "progress")
PROGRESS_LIMIT = 30 * 1024  # bytes; above this a record is folded before the next step (progress-tracking.md)
STATUS_FILE = os.path.join(ROOT, "Intermediate", "Overnight", "status.json")


def git(*args):
    try:
        return subprocess.run(["git", "-C", ROOT, *args], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=20).stdout.strip()
    except Exception:
        return ""


def section(text, heading):
    m = re.search(r"^## %s\s*\n(.*?)(?=^## |\Z)" % re.escape(heading), text, re.S | re.M)
    return " ".join(m.group(1).split()) if m else ""


def read_status():
    """前回の無人運転の状態ファイル。無ければ None。"""
    try:
        with open(STATUS_FILE, encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, dict) else None
    except Exception:
        return None


def main():
    lines = []
    unattended = os.environ.get("WASAMI_UNATTENDED") == "1"
    if unattended:
        lines.append("**無人モード**（駆動役 Tools/overnight.py が起動。`.claude/guides/autonomy.md` に従う）: ユーザーに質問せず、"
                     "本家のコード → 実機 → WebGL 版 → 仮の値の順に決めて記録の「要確認（ユーザー）」に書く。"
                     "`status: ユーザー待ち` の記録は飛ばす。ステップを終えてコミットしたら、`/clear` を頼まずに "
                     "Intermediate/Overnight/status.json を書いて応答を終える。変更を捨てる操作・配布・本家のセーブの中身の手での書き換えは行わない"
                     "（本家のセーブが遊んで書き換わる・控えから戻す・入れ替えるのはしてよい）。")
    status = read_status()
    if status:
        lines.append("前回の無人運転: %s %s — %s（ステップ: %s、コミット: %s）" % (
            status.get("written", "?"), status.get("result", "?"), status.get("reason", ""),
            status.get("step", "?"), status.get("commit", "?")))
    records = []
    if os.path.isdir(PROGRESS_DIR):
        records = sorted(f for f in os.listdir(PROGRESS_DIR) if f.endswith(".md") and f != "_template.md")
    if records:
        lines.append("未完了の進捗記録があります。**`/continue` スキルで再開してください**（照合の手順は `.claude/skills/continue/SKILL.md`）:")
        for name in records:
            with open(os.path.join(PROGRESS_DIR, name), encoding="utf-8") as f:
                text = f.read()
            title = re.search(r"^title:\s*(.+)$", text, re.M)
            status_line = re.search(r"^status:\s*(.+)$", text, re.M)
            waiting = bool(status_line and "ユーザー待ち" in status_line.group(1))
            nxt = section(text, "次にやること")
            lines.append("- .claude/progress/%s — %s%s" % (name, title.group(1).strip() if title else "", "（ユーザー待ち）" if waiting else ""))
            size = len(text.encode("utf-8"))
            if size > PROGRESS_LIMIT:
                lines.append("  大きさ: %d KB（上限 30 KB を超えています。次のステップの前に、"
                             "`.claude/guides/progress-tracking.md` の「記録を畳む」で全体を畳んでコミットしてください）" % (size // 1024))
            if nxt:
                lines.append("  次にやること: " + nxt[:300])
            pending = section(text, "要確認（ユーザー）")
            if pending and "（なし）" not in pending and not pending.startswith("<"):
                lines.append("  要確認（ユーザー）: " + pending[:600])
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
