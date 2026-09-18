#!/usr/bin/env python
"""SessionStart hook: 中断からの再開のために、未完了の進捗記録・main 以外のブランチ・直前のコミット・
未コミットの変更を知らせる（`.claude/guides/progress-tracking.md` の「再開の手順」）。

無人モード（環境変数 WASAMI_UNATTENDED=1。駆動役 Tools/overnight.py が付ける）ならそれを先頭で知らせ、
未完了の記録の「要確認（ユーザー）」と前回の無人運転の状態ファイル（Intermediate/Overnight/status.json）も出す
（`.claude/guides/autonomy.md`）。

有人セッション（合図が無い）では、作業一覧を進めるのは無人運転なので（2026-09-18 から）`/continue` を勧めず、
無人運転の結果を報告して指示を待つよう知らせ、最新の駆動役のログのまとめと、駆動役が今も動いているか
（Intermediate/Overnight/driver.json）を出す（autonomy.md の「有人セッション」）。

どちらのモードでも、作業一覧の大目標の状態（Tools/work_list.py。進行中の大目標の項目だけを取り、達成したら止まる。
2026-09-18 から）を出し、`status: 保留` の記録（進行中でない大目標の項目）に印を付ける。
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
OVERNIGHT_DIR = os.path.join(ROOT, "Intermediate", "Overnight")
STATUS_FILE = os.path.join(OVERNIGHT_DIR, "status.json")
DRIVER_FILE = os.path.join(OVERNIGHT_DIR, "driver.json")  # written by Tools/overnight.py while it runs
SUMMARY_MARK = "--- まとめ ---"  # Tools/overnight.py's summarize()
sys.path.insert(0, os.path.join(ROOT, "Tools"))


def goals_lines(unattended):
    """The big goals of the work list (Tools/work_list.py), or [] when it cannot be read."""
    try:
        import work_list
        work = work_list.load()
        if work is None or not work.goals:
            return []
        lines = ["作業一覧の大目標: " + work_list.goals_line(work, work_list.record_shares())]
        goal = work.current_goal()
        if goal is None:
            lines.append("  進行中の大目標がありません（駆動役は起動を断る）。次の大目標を進行中にするのはユーザーの指示のときだけ"
                         "（`.claude/guides/autonomy.md` の「有人セッション」の表）。" + ("何も始めず `stop` を書いて終える。" if unattended else ""))
        elif unattended:
            lines.append("  進行中の%sの節の項目だけを取る。ほかの大目標の項目は始めない。その節の項目がすべて完了したら、大目標を「達成」にして "
                         "`stop` を書く（`.claude/guides/autonomy.md` の「何を作業するか」）。" % goal.label)
        return lines
    except Exception:
        return []


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


def running_driver():
    """The driver's claim file when its process is still alive, else None. tasklist, because os.kill(pid, 0)
    terminates the process on Windows."""
    try:
        with open(DRIVER_FILE, encoding="utf-8") as f:
            data = json.load(f)
        pid = int(data.get("pid", 0))
        out = subprocess.run(["tasklist", "/FI", "PID eq %d" % pid, "/FO", "CSV", "/NH"], capture_output=True,
                             text=True, errors="replace", timeout=20).stdout
    except Exception:
        return None
    return data if pid and '"%d"' % pid in out and "python" in out.lower() else None


def latest_log_summary():
    """(path relative to ROOT, the head lines of its summary without timestamps) of the newest driver log, or None.
    The summary is empty while the driver runs or when it died before writing one."""
    try:
        logs = [os.path.join(OVERNIGHT_DIR, f) for f in os.listdir(OVERNIGHT_DIR) if f.endswith(".log")]
        path = max(logs, key=os.path.getmtime)
        with open(path, "rb") as f:
            f.seek(max(0, os.path.getsize(path) - 16384))
            tail = f.read().decode("utf-8", errors="replace")
    except Exception:
        return None
    summary = []
    if SUMMARY_MARK in tail:
        for line in tail.split(SUMMARY_MARK, 1)[1].splitlines()[1:]:
            text = re.sub(r"^\[[^\]]*\] ", "", line)
            if not text.strip() or text.startswith((" ", "ログ:", "最後の状態ファイル:")):
                continue  # the status file and the indented 要確認 lines are printed below anyway
            summary.append(text.strip())
    return os.path.relpath(path, ROOT), summary


def main():
    lines = []
    unattended = os.environ.get("WASAMI_UNATTENDED") == "1"
    if unattended:
        lines.append("**無人モード**（駆動役 Tools/overnight.py が起動。`.claude/guides/autonomy.md` に従う）: ユーザーに質問せず、"
                     "本家のコード → 実機 → WebGL 版 → 仮の値の順に決めて記録の「要確認（ユーザー）」に書く。"
                     "`status: ユーザー待ち`・`保留` の記録は飛ばす。ステップを終えてコミットしたら、`/clear` を頼まずに "
                     "Intermediate/Overnight/status.json を書いて応答を終える（Discord の報告になる done〈終わりのまとめのやったことの 1 行〉・summary・learned・"
                     "pending〈この反復で新しく出た要確認だけ〉・shots〈人に見せる連番のグリッドだけ。確かめるための撮影は付けない〉"
                     "も書く。autonomy.md の「Discord への通知」）。"
                     "変更を捨てる操作・配布・本家のセーブの中身の手での書き換えは行わない"
                     "（本家のセーブが遊んで書き換わる・控えから戻す・入れ替えるのはしてよい）。")
    else:
        lines.append("**有人セッション**（`.claude/guides/autonomy.md` の「有人セッション」）: 作業一覧は基本、無人運転が進める"
                     "（2026-09-18 から）。ここは無人運転の方向性・実装方針の修正と不具合の改善の場。まず無人運転で何が進んだか"
                     "（下のまとめと、その間のコミット）と要確認の一覧を短く報告し、ユーザーの指示を待つ。"
                     "作業一覧の続き（`/continue`）は頼まれたときだけ。指摘は記録・作業一覧・ガイドへその場で書く。")
        driver = running_driver()
        if driver:
            lines.append("**駆動役が動いています**（PID %s、%s から、ログ %s）。同じ作業ツリーとエディタを取り合うので、"
                         "ファイル・エディタ・git を変える前にユーザーに止めてもらう（端末で Ctrl+C）。読むだけならよい。" % (
                             driver.get("pid", "?"), driver.get("started", "?"), driver.get("log", "?")))
        latest = latest_log_summary()
        if latest:
            path, summary = latest
            lines.append("最新の駆動役のログ: %s — %s" % (
                path, " / ".join(summary) if summary else "（まとめなし: 駆動役が動いているか、途中で止まった）"))
    lines.extend(goals_lines(unattended))
    status = read_status()
    if status:
        lines.append("前回の無人運転: %s %s — %s（ステップ: %s、コミット: %s）" % (
            status.get("written", "?"), status.get("result", "?"), status.get("reason", ""),
            status.get("step", "?"), status.get("commit", "?")))
    records = []
    if os.path.isdir(PROGRESS_DIR):
        records = sorted(f for f in os.listdir(PROGRESS_DIR) if f.endswith(".md") and f != "_template.md")
    if records:
        lines.append("未完了の進捗記録があります。**`/continue` スキルで再開してください**（照合の手順は `.claude/skills/continue/SKILL.md`）:"
                     if unattended else
                     "未完了の進捗記録（続きは無人運転が進める。有人セッションで `/continue` を使うのは頼まれたときだけ）:")
        for name in records:
            with open(os.path.join(PROGRESS_DIR, name), encoding="utf-8") as f:
                text = f.read()
            title = re.search(r"^title:\s*(.+)$", text, re.M)
            status_line = re.search(r"^status:\s*(.+)$", text, re.M)
            waiting = bool(status_line and "ユーザー待ち" in status_line.group(1))
            on_hold = bool(status_line and "保留" in status_line.group(1))
            nxt = section(text, "次にやること")
            lines.append("- .claude/progress/%s — %s%s" % (name, title.group(1).strip() if title else "",
                                                     "（ユーザー待ち）" if waiting else "（保留: 進行中でない大目標の項目。飛ばす）" if on_hold else ""))
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
