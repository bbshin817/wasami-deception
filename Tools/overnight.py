"""Drives Claude Code unattended (the default way the work list moves on since 2026-09-18, by night or by day):
starts `claude -p "/continue" --permission-mode auto` again and again, each run being one step of the unattended
workflow (.claude/guides/autonomy.md). Claude finishing its reply (the process
exiting) is the signal to start the next run with a fresh context; a run has no timeout.

    python Tools/overnight.py --until 07:00 --usage-cmd "<command that prints the usage as JSON>"
    python Tools/overnight.py --until 07:00 --no-usage-check
    python Tools/overnight.py --max-iterations 3 --dry-run --no-usage-check

Budget (autonomy.md「予算」): before each run the driver asks --usage-cmd, a command that prints JSON of the form
{"seven_day": {"utilization": <0-100 percent>, "resets_at": "<ISO 8601>"}, "five_hour": {...}} (five_hour optional).
It stops when the weekly window has less than 50 % left with 3 or more days to the reset, or less than 25 % left with
1 or more days to the reset; when the five-hour window is used up it waits for that reset. Without --usage-cmd the
driver stops at once ("使用量が読めない") unless --no-usage-check is given; then only the "usage limit reached" reply
of Claude itself is watched, and the driver waits for the reset it names (or 30 minutes) and goes on.

After each run the driver reads Intermediate/Overnight/status.json (Claude writes {"result": "continue" | "stop",
"reason": ..., "step": ..., "commit": ..., "written": ..., "done": ..., "summary": [...], "learned": [...],
"pending": [...], "shots": {"caption": ..., "files": [...]}}) and compares HEAD before and after. It stops on "stop",
when the big goal in progress has been reached (every item of it 完了 in .claude/roadmap.md, read by
Tools/work_list.py; the unattended work never goes past a goal, the user's instruction of 2026-09-18) unless the next
goal's 始め方 is 自動 (then it goes on to that goal and posts so: goal 2, the user's instruction of 2026-09-19), when
HEAD did not move in two runs in a row, at --until, after --max-iterations, or when the budget says so. It does not
start when no goal is 進行中 or the goal in progress is already reached, unless the goal after the one reached starts
by itself (自動); otherwise only the user makes the next goal 進行中.
Everything Claude printed goes to Intermediate/Overnight/<YYYYMMDD-HHMM>.log with a header and footer per run, and a
summary for the morning (runs, why it stopped, the last status, how many 要確認 lines wait in the progress records) is
printed at the end and appended to the log.

Discord (Tools/discord_notify.py, which also says where the webhook URL comes from): the start, a report per run, the
waits and the summary are posted to the webhook. The report is the format the user gave on 2026-09-18 (run_report):
exit code, working time and 進捗率 (progress_line: the whole from the start of the project to the final goal, from
the 規模 in .claude/roadmap.md, and in brackets the big goal in progress), then 作業概要 / 分かったこと / 要検討事項
(only what came up in this run) from the status file, then the images Claude named in "shots" attached as one grid: only images meant for people (sequence
grids of effects, enemy motions, shard pickups), never shots taken to check the work. The reply of Claude itself goes
to the log only. The summary at the end is the other format the user gave on 2026-09-18 (final_report): 反復回数,
無人運転時間 and 終了理由 as a sentence, then やったこと ("done" of every run, one line each) and 要確認事項 (the
"pending" of every run put together). Without a URL, or with --no-discord, nothing is posted; a dry run prints both
posts built from the last status file instead. A post that fails is logged and the run goes on.

While it runs the driver keeps Intermediate/Overnight/driver.json ({"pid", "started", "log"}): a second driver refuses
to start, and the SessionStart hook tells an attended session not to change anything until the driver is stopped
(autonomy.md「作業の流れ」). A file left by a driver that died is ignored (its process is gone).

Exit codes: 0 ended as planned (--until, --max-iterations, Claude said stop, the goal was reached); 2 budget or usage
not readable; 3 no progress or Claude could not be started; 4 bad arguments; 5 another driver is running; 6 no big goal
to work on (none in progress, or it is already reached and the next one does not start by itself); 130 interrupted.
"""
import argparse
import atexit
import datetime
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import discord_notify  # noqa: E402  (same folder)
import work_list  # noqa: E402  (same folder)

# The replies of Claude and the records hold characters this console's code page (cp932) cannot show: print what we
# can rather than dying with a UnicodeEncodeError in the middle of the night. The log file is written as UTF-8.
for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(errors="replace")

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
OVERNIGHT_DIR = os.path.join(ROOT, "Intermediate", "Overnight")
STATUS_FILE = os.path.join(OVERNIGHT_DIR, "status.json")
DRIVER_FILE = os.path.join(OVERNIGHT_DIR, "driver.json")
PROGRESS_DIR = os.path.join(ROOT, ".claude", "progress")
# What Claude Code prints in -p mode when the subscription window is used up (推定: the classic form is
# "Claude AI usage limit reached|<unix epoch of the reset>"; the newer wording says "hit your limit").
LIMIT_PATTERN = re.compile(r"usage limit reached|hit your limit|rate limit", re.I)
LIMIT_EPOCH = re.compile(r"limit reached\|(\d{9,})")
LIMIT_BACKOFF = datetime.timedelta(minutes=30)
# What Claude Code prints in -p mode when the reply ended with background tasks (agents, background commands) still
# running and they did not finish within the wait ceiling (600 s, CLAUDE_CODE_PRINT_BG_WAIT_CEILING_MS): it
# terminates them and exits 0 (2026-09-18, troubleshooting.md).
BG_CUTOFF_PATTERN = re.compile(r"Background tasks still running after \d+s; terminating")
STALL_LIMIT = 2
# The images of a report go in one message, which Discord shows as one grid (at most 10 attachments).
REPORT_IMAGES = discord_notify.FILES_LIMIT
# The user's sample has #### headings, but Discord renders headings only down to ### (#### shows as text).
SECTION = "###"


def now():
    return datetime.datetime.now()


def stamp(when=None):
    return (when or now()).strftime("%Y-%m-%d %H:%M:%S")


class Log:
    """Timestamped lines go to the console and, unless disabled (dry run), to the log file; raw text (the output of
    Claude) goes to both as is."""

    def __init__(self, path, enabled=True):
        self.path = path
        self.file = None
        if enabled:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            self.file = open(path, "a", encoding="utf-8", newline="\n")

    def say(self, text):
        line = "[%s] %s" % (stamp(), text)
        print(line, flush=True)
        if self.file:
            self.file.write(line + "\n")
            self.file.flush()

    def raw(self, text):
        sys.stdout.write(text)
        sys.stdout.flush()
        if self.file:
            self.file.write(text)
            self.file.flush()

    def close(self):
        if self.file:
            self.file.close()
            self.file = None


def git(*args):
    out = subprocess.run(["git", "-C", ROOT] + list(args), capture_output=True, text=True, encoding="utf-8",
                         errors="replace")
    return out.stdout.strip()


def head():
    return git("rev-parse", "--short", "HEAD") or "?"


def branch():
    return git("rev-parse", "--abbrev-ref", "HEAD") or "?"


def status_stamp():
    try:
        return os.path.getmtime(STATUS_FILE)
    except OSError:
        return None


def read_status():
    try:
        with open(STATUS_FILE, encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, dict) else None
    except (OSError, ValueError):
        return None


def running_driver():
    """The claim file of another driver whose process is still alive, else None. tasklist, because os.kill(pid, 0)
    terminates the process on Windows."""
    try:
        with open(DRIVER_FILE, encoding="utf-8") as f:
            data = json.load(f)
        pid = int(data.get("pid", 0))
        out = subprocess.run(["tasklist", "/FI", "PID eq %d" % pid, "/FO", "CSV", "/NH"], capture_output=True,
                             text=True, errors="replace", timeout=20).stdout
    except (OSError, ValueError, TypeError, AttributeError, subprocess.SubprocessError):
        return None
    if pid and pid != os.getpid() and '"%d"' % pid in out and "python" in out.lower():
        return data
    return None


def claim_driver(log_path):
    """Writes driver.json for this process and removes it again at exit (only if it is still ours)."""
    os.makedirs(OVERNIGHT_DIR, exist_ok=True)
    with open(DRIVER_FILE, "w", encoding="utf-8") as f:
        json.dump({"pid": os.getpid(), "started": now().isoformat(timespec="seconds"),
                   "log": os.path.relpath(log_path, ROOT)}, f, ensure_ascii=False)

    def release():
        try:
            with open(DRIVER_FILE, encoding="utf-8") as f:
                if json.load(f).get("pid") != os.getpid():
                    return
            os.remove(DRIVER_FILE)
        except (OSError, ValueError, AttributeError):
            pass
    atexit.register(release)


def progress_records():
    """[(file name, text)] of the unfinished progress records (.claude/progress/, without _template.md)."""
    records = []
    if not os.path.isdir(PROGRESS_DIR):
        return records
    for name in sorted(os.listdir(PROGRESS_DIR)):
        if not name.endswith(".md") or name.startswith("_"):
            continue
        try:
            with open(os.path.join(PROGRESS_DIR, name), encoding="utf-8") as f:
                records.append((name, f.read()))
        except OSError:
            continue
    return records


def pending_lines():
    """The 要確認（ユーザー） lines of the unfinished progress records: [(record name, line)]."""
    found = []
    for name, text in progress_records():
        m = re.search(r"^## 要確認（ユーザー）\s*$(.*?)(?=^## |\Z)", text, re.M | re.S)
        if not m:
            continue
        for line in m.group(1).splitlines():
            line = line.strip()
            if line.startswith("- ") and "（なし）" not in line:
                found.append((name, line))
    return found


def overall_progress():
    """進捗率 in percent of the whole work, from the start of the project to the final goal (.claude/roadmap.md
    「進捗率」, computed by Tools/work_list.py). None when the work list gives no 規模."""
    work = work_list.load()
    return work.progress(work_list.record_shares()) if work else None


def progress_line():
    """The 進捗率 of the reports: the whole, and in brackets the goal in progress (roadmap.md「進捗率」)."""
    work = work_list.load()
    if not work:
        return progress_text(None)
    shares = work_list.record_shares()
    text = progress_text(work.progress(shares))
    goal = work.current_goal()
    if goal:
        text += "（%s: %s）" % (goal.label, work_list.percent_text(work.progress(shares, goal)))
    return text


def goal_in_progress():
    """(the goal to work on, None) when the driver may start, else (None, why not): the unattended work never goes
    past the big goal in progress, except on to a next goal whose 始め方 is 自動 (Claude makes it 進行中 in its first
    run); only the user makes any other goal 進行中 (autonomy.md「何を作業するか」)."""
    work = work_list.load()
    if work is None:
        return None, "作業一覧 .claude/roadmap.md が読めない。"
    goal, stopped = work.goal_to_work()
    if goal is not None:
        return goal, None
    if work.current_goal() is None:
        reached = [g.label for g in work.goals if g.state == "達成"]
        return None, ("進行中の大目標が無い（%s）。次の大目標を進行中にするのはユーザーの指示のときだけ"
                      "（有人セッションで .claude/roadmap.md の大目標の「状態」を変える）。" % (
                          "達成: " + "、".join(reached) if reached else "作業一覧に大目標の節が無い"))
    return None, ("%s の項目はすべて完了している（達成として止まる）。有人セッションで達成を確かめ、"
                  "ユーザーの指示で次の大目標を進行中にする。" % stopped.label)


def goal_reached(number):
    """True when the big goal `number` has been reached: every item of it is 完了, or its 状態 says 達成."""
    work = work_list.load()
    goal = work.goal(number) if work else None
    return bool(goal and work.reached(goal))


def next_goal(number):
    """The goal the unattended work goes on to once the goal `number` is reached (its 始め方 is 自動), else None."""
    work = work_list.load()
    goal = work.goal(number) if work else None
    return work.auto_successor(goal) if goal else None


def goal_plan(goal):
    """Where the driver stops, for the start post: the goal, and the goals after it that start by themselves."""
    chain = [goal]
    while True:
        successor = next_goal(chain[-1].number)
        if successor is None or successor in chain:
            break
        chain.append(successor)
    if len(chain) == 1:
        return "%s（この大目標を達成したら止まる）" % goal.label
    return "%s（達成したら%sへ続け、%sの達成で止まる）" % (
        goal.label, "・".join(g.label for g in chain[1:]), chain[-1].label)


def progress_text(percent):
    return "%d%%" % int(round(percent)) if percent is not None else "不明（作業一覧に規模が無い）"


def duration_text(seconds):
    """1時間15分 / 24分 / 1分未満."""
    minutes = int(round(seconds / 60.0))
    if minutes < 1:
        return "1分未満"
    hours, minutes = divmod(minutes, 60)
    if not hours:
        return "%d分" % minutes
    return "%d時間%d分" % (hours, minutes) if minutes else "%d時間" % hours


def status_list(status, key):
    """A list field of the status file as a list of strings (a lone string is one item); None when it is missing."""
    if not status or key not in status:
        return None
    value = status[key]
    if isinstance(value, str):
        value = [value]
    if not isinstance(value, list):
        return []
    return [str(item).strip() for item in value if str(item).strip()]


def bullets(items):
    return "\n".join("- " + item for item in items) if items else "なし"


def run_done(status):
    """The lines of a run for やったこと of the summary: "done" of its status file, else the first item of "summary"."""
    return status_list(status, "done") or (status_list(status, "summary") or [])[:1]


def run_pending(status, new_pending):
    """要検討事項 of a run: "pending" of its status file, else the 要確認 lines that appeared in the progress records
    during the run (`new_pending`)."""
    pending = status_list(status, "pending")
    if pending is None:
        pending = [re.sub(r"^- ", "", line) for line in new_pending]
    return pending


def report_images(status):
    """(caption, image paths) for the 📷 section: only the files Claude named in "shots" of the status file, the images
    meant for people (sequence grids of effects, enemy motions, shard pickups; the user's instruction of 2026-09-18).
    Shots taken to check the work, of the editor or of the whole desktop are never attached, so nothing else is looked
    for: without named files the section says なし."""
    shots = status.get("shots") if status else None
    if not isinstance(shots, dict):
        return None, []
    named = status_list(shots, "files") or []
    paths = [os.path.normpath(p if os.path.isabs(p) else os.path.join(ROOT, p)) for p in named]
    found = [p for p in paths if os.path.isfile(p)]
    if not found:
        return None, []
    caption = str(shots.get("caption") or "").strip()
    missing = len(paths) - len(found)
    if missing:
        caption += "（見つからない画像が %d 枚）" % missing
    if len(found) > REPORT_IMAGES:
        caption += "（%d 枚のうち先頭の %d 枚）" % (len(found), REPORT_IMAGES)
    return caption.strip(), found[:REPORT_IMAGES]


def run_problems(exit_code, status, moved, output):
    """What went wrong in a run that may still have exited 0, as (no_result, cut_off). no_result: it wrote no status
    file and HEAD did not move, so nothing of it is left. cut_off: `claude -p` terminated background tasks the reply
    left running. Added on 2026-09-18, when such a run (its research agents cut off after 600 s) showed up as an
    ordinary report with exit 0."""
    no_result = exit_code is not None and status is None and not moved
    cut_off = bool(BG_CUTOFF_PATTERN.search(output or ""))
    return no_result, cut_off


def run_report(number, exit_code, seconds, status, output, new_pending, progress, moved=True):
    """The Discord post of a finished run in the format the user gave on 2026-09-18, and the images to attach.
    作業概要 / 分かったこと / 要検討事項 come from the status file Claude wrote in this run (None when it wrote none):
    without "summary" the reply of Claude stands in, without "pending" the 要確認 lines that appeared in the progress
    records during the run (`new_pending`) do. `progress` is the 進捗率 as text (progress_line). `moved`: HEAD moved
    during the run. A run that left nothing (run_problems) gets ⚠️ in the heading and a 結果 line, so that exit 0 does
    not read as an ordinary run."""
    no_result, cut_off = run_problems(exit_code, status, moved, output)
    summary = status_list(status, "summary")
    if summary is None:
        summary_text = "（状態ファイルに作業概要が無いので、Claude の最後の応答を載せます）\n\n" + (output.strip() or "（出力なし）")
    else:
        summary_text = bullets(summary)
    pending = run_pending(status, new_pending)
    caption, images = report_images(status)
    lines = [
        ("## ⚠️ 反復 #%d 終了（成果なし）" if no_result else "## 📌 反復 #%d 終了") % number,
        "",
        "%s ステータス" % SECTION,
        "- exit: %s" % (exit_code if exit_code is not None else "起動できない"),
        "- 作業時間: %s" % duration_text(seconds),
        "- 進捗率: %s" % progress,
    ]
    if no_result:
        lines.append("- 結果: 状態ファイルが書かれず、コミットも増えていない（この反復の作業は残っていない）")
    if cut_off:
        lines.append("- 原因: 応答を終えた後もバックグラウンドの作業が残り、`claude -p` が打ち切った"
                     "（症状索引の「Background tasks still running after 600s」）")
    lines += [
        "",
        "%s 🔧 作業概要" % SECTION,
        summary_text,
        "",
        "%s 💡 分かったこと" % SECTION,
        bullets(status_list(status, "learned") or []),
        "",
        "%s 🚨 要検討事項" % SECTION,
        bullets(pending),
        "",
        "%s 📷 スクショ" % SECTION,
        (caption or "画像を添付します。") if images else "なし",
    ]
    return "\n".join(lines), images


def final_report(runs, seconds, ending, done, pending):
    """The Discord post at the end of the driver in the format the user gave on 2026-09-18. `ending` is why it ended as
    a sentence (「スケジュール時刻を迎えたため」), `done` and `pending` the lines of every run in order (the same
    line twice is shown once)."""
    return "\n".join([
        "# 🏁 無人運転終了",
        "",
        "- 反復回数: %d回" % runs,
        "- 無人運転時間: %s" % duration_text(seconds),
        "- 終了理由: %s" % ending,
        "",
        "%s 🔧 やったこと" % SECTION,
        bullets(list(dict.fromkeys(done))),
        "",
        "%s 🚨 要確認事項" % SECTION,
        bullets(list(dict.fromkeys(pending))),
    ])


def parse_when(text):
    """An ISO 8601 time (a trailing Z is accepted; a naive time is taken as local) or a unix epoch → naive local
    datetime."""
    if isinstance(text, (int, float)):
        return datetime.datetime.fromtimestamp(text)
    dt = datetime.datetime.fromisoformat(str(text).strip().replace("Z", "+00:00"))
    if dt.tzinfo is not None:
        dt = dt.astimezone().replace(tzinfo=None)
    return dt


def parse_until(text):
    if not text:
        return None
    hours, minutes = text.split(":")
    when = now().replace(hour=int(hours), minute=int(minutes), second=0, microsecond=0)
    if when <= now():
        when += datetime.timedelta(days=1)
    return when


def read_usage(command):
    """Runs --usage-cmd. Returns (usage dict, None) or (None, why it could not be read)."""
    try:
        out = subprocess.run(command, shell=True, cwd=ROOT, capture_output=True, text=True, encoding="utf-8",
                             errors="replace", timeout=120)
    except (OSError, subprocess.TimeoutExpired) as e:
        return None, "実行できない: %s" % e
    if out.returncode != 0:
        return None, "終了コード %d: %s" % (out.returncode, (out.stderr or out.stdout).strip()[:300])
    try:
        data = json.loads(out.stdout)
    except ValueError as e:
        return None, "JSON ではない: %s (%s)" % (e, out.stdout.strip()[:200])
    if not isinstance(data, dict) or not isinstance(data.get("seven_day"), dict):
        return None, "seven_day が無い: %s" % json.dumps(data, ensure_ascii=False)[:200]
    for key in ("seven_day", "five_hour"):
        window = data.get(key)
        if window is None:
            continue
        try:
            float(window["utilization"])
            parse_when(window["resets_at"])
        except (KeyError, TypeError, ValueError) as e:
            return None, "%s の utilization / resets_at が読めない: %s" % (key, e)
    return data, None


def budget_verdict(usage):
    """The budget rule (autonomy.md「予算」). Returns (action, text, wait_until): action is "go", "wait" (the
    five-hour window is used up; wait_until is its reset) or "stop"."""
    week = usage["seven_day"]
    remaining = 100.0 - float(week["utilization"])
    resets = parse_when(week["resets_at"])
    days = (resets - now()).total_seconds() / 86400.0
    text = "週間の枠は残り %.0f %%（リセット %s、あと %.1f 日）" % (remaining, stamp(resets), days)
    five = usage.get("five_hour")
    if five is not None:
        text += "、5 時間の枠は残り %.0f %%（リセット %s）" % (100.0 - float(five["utilization"]),
                                                            stamp(parse_when(five["resets_at"])))
    if days >= 3 and remaining < 50:
        return "stop", text + " — リセットまで 3 日以上で残り 50 % 未満", None
    if days >= 1 and remaining < 25:
        return "stop", text + " — リセットまで 1 日以上で残り 25 % 未満", None
    if five is not None and float(five["utilization"]) >= 100:
        return "wait", text + " — 5 時間の枠が尽きた", parse_when(five["resets_at"])
    return "go", text, None


def limit_reset(output, exit_code):
    """When the reply of Claude says the usage limit is reached: when to try again (the epoch after "|" if the reply
    has one, else 30 minutes from now). Otherwise None. A reply that merely mentions limits while exiting with 0 and
    without the "|epoch" form is not taken as the limit (Claude may write about limits in an ordinary reply)."""
    m = LIMIT_EPOCH.search(output)
    if m:
        return datetime.datetime.fromtimestamp(int(m.group(1)))
    if exit_code != 0 and LIMIT_PATTERN.search(output):
        return now() + LIMIT_BACKOFF
    return None


def sleep_until(when, deadline, log, discord, why):
    """Sleeps until `when` (but never past the deadline). Returns False when the deadline came first."""
    target = when if deadline is None or when <= deadline else deadline
    log.say("待機: %s まで（%s）" % (stamp(target), why))
    discord.post("## ⏸️ 待機\n\n- 再開: %s\n- 理由: %s" % (stamp(target), why))
    while True:
        rest = (target - now()).total_seconds()
        if rest <= 0:
            break
        time.sleep(min(rest, 60))
    return deadline is None or when <= deadline


def resolve_command(text):
    """--claude as a command line → argv, with the program resolved through PATH (claude.CMD on this PC)."""
    parts = [p.strip('"') for p in shlex.split(text, posix=False)]
    found = shutil.which(parts[0])
    if found:
        parts[0] = found
    return parts


def run_claude(command, log):
    """One run. Returns (exit code, everything it printed, seconds). None as the exit code means it could not start."""
    env = dict(os.environ, WASAMI_UNATTENDED="1")
    started = time.time()
    try:
        proc = subprocess.Popen(command, cwd=ROOT, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
    except OSError as e:
        log.say("起動できない: %s" % e)
        return None, "", 0.0
    chunks = []
    try:
        for raw in iter(proc.stdout.readline, b""):
            text = raw.decode("utf-8", errors="replace")
            chunks.append(text)
            log.raw(text)
        proc.wait()
    except KeyboardInterrupt:
        proc.terminate()
        raise
    if chunks and not chunks[-1].endswith("\n"):
        log.raw("\n")
    return proc.returncode, "".join(chunks), time.time() - started


def summarize(log, discord, runs, reason, ending, status, code, started, done, night_pending):
    """The summary for the morning: the log gets the detail (the SessionStart hook reads its head lines), Discord the
    user's format (final_report)."""
    pending = pending_lines()
    lines = ["反復: %d 回、終了の理由: %s" % (runs, reason)]
    if status:
        lines.append("最後の状態ファイル: result=%s step=%s commit=%s reason=%s" % (
            status.get("result", "?"), status.get("step", ""), status.get("commit", ""), status.get("reason", "")))
    else:
        lines.append("最後の状態ファイル: （この運転では書かれていない）")
    lines.append("HEAD: %s (%s)" % (head(), branch()))
    lines.append("要確認（ユーザー）: %d 件" % len(pending))
    for name, line in pending:
        lines.append("  %s: %s" % (name, line))
    lines.append("ログ: %s" % log.path)
    log.say("--- まとめ ---")
    for line in lines:
        log.say(line)
    discord.post(final_report(runs, time.time() - started, ending, done, night_pending))
    return code


def main():
    ap = argparse.ArgumentParser(description='Runs `claude -p "/continue"` again and again for unattended work.')
    ap.add_argument("--until", metavar="HH:MM", help="stop before starting a run at or after this time of day")
    ap.add_argument("--max-iterations", type=int, metavar="N", help="stop after N runs")
    ap.add_argument("--usage-cmd", metavar="CMD", help="command that prints the usage as JSON (see the module doc)")
    ap.add_argument("--no-usage-check", action="store_true",
                    help="run without reading the usage (only the limit reply of Claude itself is watched)")
    ap.add_argument("--dry-run", action="store_true", help="show the plan, check the launcher and the usage, run nothing")
    ap.add_argument("--claude", default="claude", metavar="CMD",
                    help="how to start Claude Code (a command line; the prompt and modes are appended)")
    ap.add_argument("--prompt", default="/continue", help="what each run is asked (default /continue)")
    ap.add_argument("--pause", type=float, default=5.0, metavar="SEC", help="pause between runs (default 5)")
    ap.add_argument("--no-discord", action="store_true", help="do not post to the Discord webhook")
    args = ap.parse_args()

    if args.claude == "claude" and os.environ.get("CLAUDECODE") and not args.dry_run:
        print("Claude Code のセッションの中からは動かせない（入れ子になる）。ユーザーの端末から起動する。", file=sys.stderr)
        return 4
    if not args.usage_cmd and not args.no_usage_check:
        print("使用量が読めない: --usage-cmd <使用量を JSON で出すコマンド> か --no-usage-check を付ける"
              "（.claude/guides/autonomy.md の「予算」）。", file=sys.stderr)
        return 2
    try:
        deadline = parse_until(args.until)
    except ValueError:
        print("--until は HH:MM の形", file=sys.stderr)
        return 4
    other = running_driver()
    if other:
        print("別の駆動役が動いている（PID %s、%s から、ログ %s）。止めてから起動する。" % (
            other.get("pid", "?"), other.get("started", "?"), other.get("log", "?")), file=sys.stderr)
        return 5
    goal, refusal = goal_in_progress()
    if refusal:
        print(refusal, file=sys.stderr)
        return 6
    command = resolve_command(args.claude) + ["-p", args.prompt, "--permission-mode", "auto",
                                              "--permission-prompts", "none"]

    log = Log(os.path.join(OVERNIGHT_DIR, now().strftime("%Y%m%d-%H%M") + ".log"), enabled=not args.dry_run)
    log.say("無人運転の駆動役%s" % ("（dry run）" if args.dry_run else ""))
    log.say("プロジェクト: %s、ブランチ %s、HEAD %s" % (ROOT, branch(), head()))
    log.say("コマンド: %s" % subprocess.list2cmdline(command))
    log.say("環境: WASAMI_UNATTENDED=1、終了の時刻 %s、反復の上限 %s、使用量 %s" % (
        stamp(deadline) if deadline else "なし", args.max_iterations if args.max_iterations is not None else "なし",
        ("`%s`" % args.usage_cmd) if args.usage_cmd else "読まない（--no-usage-check）"))
    log.say("大目標: %s（進行中の大目標の項目がすべて完了したら止まる）" % work_list.goals_line(
        work_list.load(), work_list.record_shares()))
    url, source = discord_notify.webhook_url()
    if args.no_discord:
        url = None
        log.say("Discord: 送らない（--no-discord）")
    elif url:
        log.say("Discord: %s の webhook へ送る%s" % (source, "（dry run なので送らない）" if args.dry_run else ""))
    else:
        log.say("Discord: 送らない（環境変数 %s も %s も無い）" % (
            discord_notify.ENV, os.path.relpath(discord_notify.LOCAL_CONFIG, ROOT)))
    discord = discord_notify.Webhook(None if args.dry_run else url, log.say)
    previous = read_status()
    if previous:
        log.say("前回の状態ファイル: result=%s written=%s reason=%s" % (
            previous.get("result", "?"), previous.get("written", "?"), previous.get("reason", "")))

    if args.dry_run:
        probe = subprocess.run(command[:1] + ["--version"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8",
                               errors="replace")
        log.say("起動の確認: %s → exit %d %s" % (command[0], probe.returncode, (probe.stdout or probe.stderr).strip()))
        if args.usage_cmd:
            usage, error = read_usage(args.usage_cmd)
            if usage is None:
                log.say("使用量が読めない: %s" % error)
                return 2
            action, text, _ = budget_verdict(usage)
            log.say("使用量: %s → %s" % (text, action))
            if action == "stop":
                log.say("予算の決まりでは今は走らない")
                return 2
        if previous:
            report, images = run_report(0, 0, 0, previous, "", [], progress_line())
            log.say("前回の状態ファイルから組んだ反復の報告の見本（添付 %d 枚）:\n%s" % (len(images), report))
            log.say("同じ状態ファイルの 1 反復で終えたときの終わりのまとめの見本:\n%s" % final_report(
                1, 0, "スケジュール時刻を迎えたため", run_done(previous), run_pending(previous, [])))
        log.say("dry run なので走らせない")
        return 0 if probe.returncode == 0 else 3

    claim_driver(log.path)
    driver_started = time.time()
    discord.post("\n".join([
        "## 🚀 無人運転 開始",
        "",
        "%s ステータス" % SECTION,
        "- ブランチ: %s（HEAD %s）" % (branch(), head()),
        "- 終了の時刻: %s" % (stamp(deadline) if deadline else "なし"),
        "- 反復の上限: %s" % (args.max_iterations if args.max_iterations is not None else "なし"),
        "- 大目標: %s" % goal_plan(goal),
        "- 進捗率: %s" % progress_line(),
    ]))
    runs = 0
    stalled = 0
    last_status = None
    # Why the driver ended: `reason` for the log (with the detail), `ending` as the sentence of the Discord summary.
    reason = ending = None
    code = 0
    done, night_pending = [], []
    try:
        while True:
            if args.max_iterations is not None and runs >= args.max_iterations:
                reason = "反復の上限 %d 回" % args.max_iterations
                ending = "反復の上限（%d回）に達したため" % args.max_iterations
                break
            if deadline and now() >= deadline:
                reason = "終了の時刻 %s" % stamp(deadline)
                ending = "スケジュール時刻を迎えたため"
                break
            if args.usage_cmd:
                usage, error = read_usage(args.usage_cmd)
                if usage is None:
                    reason, code = "使用量が読めない: %s" % error, 2
                    ending = "使用量を読めなかったため"
                    break
                action, text, wait_until = budget_verdict(usage)
                log.say("使用量: %s" % text)
                if action == "stop":
                    reason, code = "予算: %s" % text, 2
                    ending = "週間の使用量の残りが予算の決まりを下回ったため"
                    break
                if action == "wait":
                    if not sleep_until(wait_until, deadline, log, discord, "5 時間の枠が尽きた"):
                        reason = "終了の時刻 %s（5 時間の枠の待ちの途中）" % stamp(deadline)
                        ending = "5時間の使用量の枠の回復を待つ間にスケジュール時刻を迎えたため"
                        break
                    continue
            runs += 1
            before = head()
            stamp_before = status_stamp()
            pending_before = set(line for _, line in pending_lines())
            log.say("=== 反復 %d 開始: HEAD %s (%s)" % (runs, before, branch()))
            exit_code, output, seconds = run_claude(command, log)
            after = head()
            status = read_status() if status_stamp() != stamp_before else None
            if status:
                last_status = status
            footer = "反復 %d 終了: exit %s、%.0f 秒、HEAD %s → %s、状態ファイル %s" % (
                runs, exit_code, seconds, before, after,
                ("result=%s step=%s reason=%s" % (status.get("result", "?"), status.get("step", ""),
                                                  status.get("reason", ""))) if status else "書かれていない")
            log.say("=== " + footer)
            no_result, cut_off = run_problems(exit_code, status, after != before, output)
            if cut_off:
                log.say("バックグラウンドの作業が残ったまま応答が終わり、打ち切られた（症状索引）")
            new_pending = [line for _, line in pending_lines() if line not in pending_before]
            done += run_done(status)
            if no_result:
                done.append("反復 %d: 成果なし（%s）" % (
                    runs, "バックグラウンドの作業の打ち切り" if cut_off else "状態ファイルもコミットも無い"))
            night_pending += run_pending(status, new_pending)
            report, images = run_report(runs, exit_code, seconds, status, output, new_pending, progress_line(),
                                        after != before)
            if images:
                discord.post_images(images, report)
            else:
                discord.post(report)
            if exit_code is None:
                reason, code = "Claude を起動できない", 3
                ending = "Claude を起動できなかったため"
                break
            reset = limit_reset(output, exit_code)
            if reset is not None and after == before:
                log.say("使用量の上限に達した返事（5 時間の枠とみなす）")
                if not sleep_until(reset, deadline, log, discord, "使用量の上限に達した返事"):
                    reason = "終了の時刻 %s（上限の待ちの途中）" % stamp(deadline)
                    ending = "使用量の上限の回復を待つ間にスケジュール時刻を迎えたため"
                    break
                continue
            if goal_reached(goal.number):
                successor = next_goal(goal.number)
                if successor is None:
                    reason = "%s を達成（項目がすべて完了）" % goal.label
                    ending = "%sを達成したため" % goal.label
                    break
                # The next goal starts by itself (始め方: 自動): go on; Claude makes it 進行中 if it has not yet.
                log.say("%s を達成。%s は始め方が自動なので続ける" % (goal.label, successor.label))
                discord.post("\n".join([
                    "## 🎯 %s を達成" % goal.label,
                    "",
                    "%s ステータス" % SECTION,
                    "- 次: %s（始め方が自動なので、無人運転がそのまま続ける）" % successor.label,
                    "- 進捗率: %s" % progress_line(),
                ]))
                goal = successor
            if status and status.get("result") == "stop":
                reason = "Claude が stop: %s" % status.get("reason", "（理由なし）")
                ending = "Claude が作業を止めたため（%s）" % (status.get("reason") or "理由なし")
                break
            if after == before:
                stalled += 1
                if stalled >= STALL_LIMIT:
                    reason, code = "進捗なし（HEAD が %d 回続けて動かない）" % STALL_LIMIT, 3
                    ending = "%d回続けてコミットが増えなかったため" % STALL_LIMIT
                    break
            else:
                stalled = 0
            if args.pause > 0:
                time.sleep(args.pause)
    except KeyboardInterrupt:
        reason, code = "中断（Ctrl+C）", 130
        ending = "Ctrl+C で中断したため"
    result = summarize(log, discord, runs, reason, ending, last_status, code, driver_started, done, night_pending)
    log.close()
    return result


if __name__ == "__main__":
    sys.exit(main())
