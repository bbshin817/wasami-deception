"""Reads the work list .claude/roadmap.md for the unattended driver (Tools/overnight.py) and the SessionStart hook
(.claude/scripts/session_start_hook.py): the big goals, the items and the 進捗率.

The work list (roadmap.md「大目標」「進捗率」, since 2026-09-18) is split into big goals, `## 大目標 N: <name>` sections
whose head has a `- 状態:` line (未着手 / 進行中 / 達成（日付）); the items are `### N. <title>` blocks with `- 規模:` and
`- 状態:` lines, under a goal section or under another section (the items finished before the goals, the items called
off). Only one goal is 進行中 at a time and only the user makes a goal 進行中; the unattended work takes items of that
goal only and stops when all of them are 完了 (.claude/guides/autonomy.md「何を作業するか」).

A progress record (.claude/progress/, not _template.md) belongs to item N when its "# " title names 「項目 N」; its
share is the checked top-level steps (`- [x]`) of its 計画 over all of them.
"""
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ROADMAP = os.path.join(ROOT, ".claude", "roadmap.md")
PROGRESS_DIR = os.path.join(ROOT, ".claude", "progress")

NUMBER = r"(\d+(?:\.\d+)?)"
GOAL_HEADING = re.compile(r"^## 大目標\s*(\d+)[:：]\s*(.+?)\s*$", re.M)  # matched at the start of a section
ITEM = re.compile(r"^### (\d+)\. ([^\n]*)$(.*?)(?=^##|\Z)", re.M | re.S)


class Item(object):
    def __init__(self, number, title, size, state, goal):
        self.number = number
        self.title = title
        self.size = size    # None when the item gives no 規模 (it is not counted)
        self.state = state  # the first word of 状態: 完了（…）, 未着手。, 保留…, 取りやめ（…）, …
        self.goal = goal    # the number of its big goal, None outside the goal sections

    @property
    def done(self):
        return self.state.startswith("完了")

    @property
    def counted(self):
        return self.size is not None and not self.state.startswith("取りやめ")


class Goal(object):
    def __init__(self, number, name, state):
        self.number = number
        self.name = name
        self.state = state  # 未着手 / 進行中 / 達成
        self.items = []

    @property
    def label(self):
        return "大目標 %d「%s」" % (self.number, self.name)

    @property
    def complete(self):
        """Every counted item of the goal is 完了 (the goal is reached even if its 状態 was not changed yet)."""
        counted = [item for item in self.items if item.counted]
        return bool(counted) and all(item.done for item in counted)


class WorkList(object):
    def __init__(self, base, goals, items):
        self.base = base
        self.goals = goals
        self.items = items

    def current_goal(self):
        """The goal whose 状態 is 進行中 (the first one if the list is wrong), else None."""
        for goal in self.goals:
            if goal.state == "進行中":
                return goal
        return None

    def goal(self, number):
        for goal in self.goals:
            if goal.number == number:
                return goal
        return None

    def progress(self, shares, goal=None):
        """進捗率 in percent: the whole (the groundwork plus every counted item) or, with `goal`, that goal's items only
        (no groundwork). An unfinished item counts its 規模 times the share of its progress record. None when nothing
        is counted."""
        items = goal.items if goal is not None else self.items
        total = done = 0.0 if goal is not None else self.base
        for item in items:
            if not item.counted:
                continue
            total += item.size
            done += item.size if item.done else item.size * shares.get(item.number, 0.0)
        if total <= (0.0 if goal is not None else self.base):
            return None
        return 100.0 * done / total


def _goal_state(text):
    if text.startswith("進行中"):
        return "進行中"
    if text.startswith("達成"):
        return "達成"
    return "未着手"


def parse(text):
    base = re.search(r"^- 作業一覧の前に済んだ土台の規模[:：]\s*" + NUMBER, text, re.M)
    base = float(base.group(1)) if base else 0.0
    goals, items = [], []
    # Split into the "## " sections; the items of a goal section belong to that goal.
    for section in re.split(r"(?m)^(?=## )", text):
        heading = GOAL_HEADING.match(section)
        goal = None
        if heading:
            head = re.split(r"(?m)^### ", section, 1)[0]
            state = re.search(r"^- 状態[:：]\s*\**(\S*)", head, re.M)
            goal = Goal(int(heading.group(1)), heading.group(2), _goal_state(state.group(1) if state else ""))
            goals.append(goal)
        for number, title, body in ITEM.findall(section):
            size = re.search(r"^- 規模[:：]\s*" + NUMBER, body, re.M)
            state = re.search(r"^- 状態[:：]\s*\**(\S*)", body, re.M)
            item = Item(int(number), title.strip(), float(size.group(1)) if size else None,
                        state.group(1) if state else "", goal.number if goal else None)
            items.append(item)
            if goal:
                goal.items.append(item)
    return WorkList(base, goals, items)


def load(path=ROADMAP):
    """The parsed work list, or None when it cannot be read."""
    try:
        with open(path, encoding="utf-8") as f:
            return parse(f.read())
    except OSError:
        return None


def record_shares(progress_dir=PROGRESS_DIR):
    """{item number: share of the checked top-level steps in the 計画 of its unfinished progress record}."""
    shares = {}
    try:
        names = sorted(os.listdir(progress_dir))
    except OSError:
        return shares
    for name in names:
        if not name.endswith(".md") or name.startswith("_"):
            continue
        try:
            with open(os.path.join(progress_dir, name), encoding="utf-8") as f:
                text = f.read()
        except OSError:
            continue
        title = re.search(r"^# .*$", text, re.M)
        item = re.search(r"項目\s*(\d+)", title.group(0)) if title else None
        plan = re.search(r"^## 計画\s*$(.*?)(?=^## |\Z)", text, re.M | re.S)
        if not item or not plan:
            continue
        marks = re.findall(r"^- \[([ xX])\]", plan.group(1), re.M)
        if marks:
            shares[int(item.group(1))] = sum(1 for mark in marks if mark != " ") / float(len(marks))
    return shares


def percent_text(percent):
    return "%d%%" % int(round(percent)) if percent is not None else "不明"


def goals_line(work, shares):
    """One line on the big goals for the SessionStart hook and the driver's log: each goal with its 状態, and the
    items done and the progress of the goal in progress."""
    parts = []
    for goal in work.goals:
        counted = [item for item in goal.items if item.counted]
        text = "%s %s" % (goal.label, goal.state)
        if goal.state == "進行中":
            text += "（項目 %d/%d 完了、%s）" % (sum(1 for item in counted if item.done), len(counted),
                                            percent_text(work.progress(shares, goal)))
            if goal.complete:
                text += " — 項目はすべて完了（達成として止まる）"
        parts.append(text)
    return " / ".join(parts) if parts else "（作業一覧に大目標の節が無い）"
