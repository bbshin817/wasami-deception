"""Drives Claude Code through the night: starts `claude -p "/continue" --permission-mode auto` again and again, each
run being one step of the unattended workflow (.claude/guides/autonomy.md). Claude finishing its reply (the process
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
"reason": ..., "step": ..., "commit": ..., "written": ...}) and compares HEAD before and after. It stops on "stop",
when HEAD did not move in two runs in a row, at --until, after --max-iterations, or when the budget says so.
Everything Claude printed goes to Intermediate/Overnight/<YYYYMMDD-HHMM>.log with a header and footer per run, and a
summary for the morning (runs, why it stopped, the last status, how many 要確認 lines wait in the progress records) is
printed at the end and appended to the log.

Discord (Tools/discord_notify.py, which also says where the webhook URL comes from): the start, the reply of Claude
after each run (with the run's footer line) followed by the HighResShot images the run left in Saved/Screenshots/, the
waits and the summary are posted to the webhook. Without a URL, or with --no-discord, nothing is posted; a dry run only
says whether it would post. A post that fails is logged and the night goes on.

Exit codes: 0 ended as planned (--until, --max-iterations, Claude said stop); 2 budget or usage not readable;
3 no progress or Claude could not be started; 4 bad arguments; 130 interrupted.
"""
import argparse
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

# The replies of Claude and the records hold characters this console's code page (cp932) cannot show: print what we
# can rather than dying with a UnicodeEncodeError in the middle of the night. The log file is written as UTF-8.
for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, "reconfigure"):
        stream.reconfigure(errors="replace")

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
OVERNIGHT_DIR = os.path.join(ROOT, "Intermediate", "Overnight")
STATUS_FILE = os.path.join(OVERNIGHT_DIR, "status.json")
PROGRESS_DIR = os.path.join(ROOT, ".claude", "progress")
# What Claude Code prints in -p mode when the subscription window is used up (推定: the classic form is
# "Claude AI usage limit reached|<unix epoch of the reset>"; the newer wording says "hit your limit").
LIMIT_PATTERN = re.compile(r"usage limit reached|hit your limit|rate limit", re.I)
LIMIT_EPOCH = re.compile(r"limit reached\|(\d{9,})")
LIMIT_BACKOFF = datetime.timedelta(minutes=30)
STALL_LIMIT = 2
# PIE's HighResShot writes here (Saved/Screenshots/WindowsEditor/); only this game ever lands in it.
SCREENSHOT_DIR = os.path.join(ROOT, "Saved", "Screenshots")
SCREENSHOTS_PER_RUN = 20


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


def new_screenshots(since):
    """Images under Saved/Screenshots/ written at or after `since` (epoch seconds), oldest first."""
    found = []
    for folder, _, names in os.walk(SCREENSHOT_DIR):
        for name in names:
            if os.path.splitext(name)[1].lower() not in discord_notify.IMAGE_TYPES:
                continue
            path = os.path.join(folder, name)
            try:
                written = os.path.getmtime(path)
            except OSError:
                continue
            if written >= since:
                found.append((written, path))
    return [path for _, path in sorted(found)]


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


def pending_lines():
    """The 要確認（ユーザー） lines of the unfinished progress records: [(record name, line)]."""
    found = []
    if not os.path.isdir(PROGRESS_DIR):
        return found
    for name in sorted(os.listdir(PROGRESS_DIR)):
        if not name.endswith(".md") or name.startswith("_"):
            continue
        try:
            with open(os.path.join(PROGRESS_DIR, name), encoding="utf-8") as f:
                text = f.read()
        except OSError:
            continue
        m = re.search(r"^## 要確認（ユーザー）\s*$(.*?)(?=^## |\Z)", text, re.M | re.S)
        if not m:
            continue
        for line in m.group(1).splitlines():
            line = line.strip()
            if line.startswith("- ") and "（なし）" not in line:
                found.append((name, line))
    return found


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
    text = "待機: %s まで（%s）" % (stamp(target), why)
    log.say(text)
    discord.post(text)
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


def summarize(log, discord, runs, reason, status, code):
    pending = pending_lines()
    lines = ["反復: %d 回、終了の理由: %s" % (runs, reason)]
    if status:
        lines.append("最後の状態ファイル: result=%s step=%s commit=%s reason=%s" % (
            status.get("result", "?"), status.get("step", ""), status.get("commit", ""), status.get("reason", "")))
    else:
        lines.append("最後の状態ファイル: （この夜は書かれていない）")
    lines.append("HEAD: %s (%s)" % (head(), branch()))
    lines.append("要確認（ユーザー）: %d 件" % len(pending))
    for name, line in pending:
        lines.append("  %s: %s" % (name, line))
    lines.append("ログ: %s" % log.path)
    log.say("--- まとめ ---")
    for line in lines:
        log.say(line)
    discord.post("**無人運転を終えた**（終了コード %d）\n%s" % (code, "\n".join(lines)))
    return code


def main():
    ap = argparse.ArgumentParser(description='Runs `claude -p "/continue"` again and again for the unattended night.')
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
    command = resolve_command(args.claude) + ["-p", args.prompt, "--permission-mode", "auto",
                                              "--permission-prompts", "none"]

    log = Log(os.path.join(OVERNIGHT_DIR, now().strftime("%Y%m%d-%H%M") + ".log"), enabled=not args.dry_run)
    log.say("無人運転の駆動役%s" % ("（dry run）" if args.dry_run else ""))
    log.say("プロジェクト: %s、ブランチ %s、HEAD %s" % (ROOT, branch(), head()))
    log.say("コマンド: %s" % subprocess.list2cmdline(command))
    log.say("環境: WASAMI_UNATTENDED=1、終了の時刻 %s、反復の上限 %s、使用量 %s" % (
        stamp(deadline) if deadline else "なし", args.max_iterations if args.max_iterations is not None else "なし",
        ("`%s`" % args.usage_cmd) if args.usage_cmd else "読まない（--no-usage-check）"))
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
                log.say("予算の決まりでは今夜は走らない")
                return 2
        log.say("dry run なので走らせない")
        return 0 if probe.returncode == 0 else 3

    discord.post("**無人運転を始めた**\nブランチ %s、HEAD %s、終了の時刻 %s、反復の上限 %s" % (
        branch(), head(), stamp(deadline) if deadline else "なし",
        args.max_iterations if args.max_iterations is not None else "なし"))
    runs = 0
    stalled = 0
    last_status = None
    reason = None
    code = 0
    try:
        while True:
            if args.max_iterations is not None and runs >= args.max_iterations:
                reason = "反復の上限 %d 回" % args.max_iterations
                break
            if deadline and now() >= deadline:
                reason = "終了の時刻 %s" % stamp(deadline)
                break
            if args.usage_cmd:
                usage, error = read_usage(args.usage_cmd)
                if usage is None:
                    reason, code = "使用量が読めない: %s" % error, 2
                    break
                action, text, wait_until = budget_verdict(usage)
                log.say("使用量: %s" % text)
                if action == "stop":
                    reason, code = "予算: %s" % text, 2
                    break
                if action == "wait":
                    if not sleep_until(wait_until, deadline, log, discord, "5 時間の枠が尽きた"):
                        reason = "終了の時刻 %s（5 時間の枠の待ちの途中）" % stamp(deadline)
                        break
                    continue
            runs += 1
            before = head()
            stamp_before = status_stamp()
            run_started = time.time()
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
            discord.post("**%s**\n%s" % (footer, output.strip() or "（出力なし）"))
            shots = new_screenshots(run_started)
            if shots:
                discord.post_images(shots[:SCREENSHOTS_PER_RUN], "反復 %d の HighResShot（%d 枚%s）" % (
                    runs, len(shots),
                    "、先頭の %d 枚を送る" % SCREENSHOTS_PER_RUN if len(shots) > SCREENSHOTS_PER_RUN else ""))
            if exit_code is None:
                reason, code = "Claude を起動できない", 3
                break
            reset = limit_reset(output, exit_code)
            if reset is not None and after == before:
                log.say("使用量の上限に達した返事（5 時間の枠とみなす）")
                if not sleep_until(reset, deadline, log, discord, "使用量の上限に達した返事"):
                    reason = "終了の時刻 %s（上限の待ちの途中）" % stamp(deadline)
                    break
                continue
            if status and status.get("result") == "stop":
                reason = "Claude が stop: %s" % status.get("reason", "（理由なし）")
                break
            if after == before:
                stalled += 1
                if stalled >= STALL_LIMIT:
                    reason, code = "進捗なし（HEAD が %d 回続けて動かない）" % STALL_LIMIT, 3
                    break
            else:
                stalled = 0
            if args.pause > 0:
                time.sleep(args.pause)
    except KeyboardInterrupt:
        reason, code = "中断（Ctrl+C）", 130
    result = summarize(log, discord, runs, reason, last_status, code)
    log.close()
    return result


if __name__ == "__main__":
    sys.exit(main())
