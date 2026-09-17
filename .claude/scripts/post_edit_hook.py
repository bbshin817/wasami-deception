#!/usr/bin/env python
"""PostToolUse(Edit|Write|MultiEdit) hook: 記録対象のソースを編集したら、対応する実装記録を思い出させる。

何があっても停止させない（例外は握りつぶして exit 0）。
"""
import json
import os
import re
import sys

# Claude Code は hook の出力を UTF-8 として読む。この PC のコンソールは cp932 で、「—」のような cp932 に無い文字が
# 1 つでもあると print が UnicodeEncodeError になり、外側の try が握りつぶして出力ごと消えていた（2026-09-17 に発見）。
for _stream in (sys.stdout, sys.stderr):
    if hasattr(_stream, "reconfigure"):
        _stream.reconfigure(encoding="utf-8", errors="replace")

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
RECORDS_DIR = os.path.join(ROOT, ".claude", "implementation-records")
SCOPE_PREFIXES = ("Source/", "Content/Python/", "Tools/", "Config/")
SCOPE_FILES = ("wasami_deception.uproject", ".mcp.json")
SCOPE_EXT = re.compile(r"\.(cpp|h|cs|py|ini|uproject|json)$")


def main():
    try:
        data = json.load(sys.stdin)
    except Exception:
        return 0
    path = (data.get("tool_input") or {}).get("file_path")
    if not path:
        return 0
    try:
        rel = os.path.relpath(os.path.abspath(path), ROOT).replace(os.sep, "/")
    except ValueError:
        return 0
    if rel.startswith("..") or rel.startswith(".claude/"):
        return 0
    if rel not in SCOPE_FILES and not (rel.startswith(SCOPE_PREFIXES) and SCOPE_EXT.search(rel)):
        return 0

    owners = []
    if os.path.isdir(RECORDS_DIR):
        for name in sorted(os.listdir(RECORDS_DIR)):
            if not name.endswith(".md") or name.startswith("_"):
                continue
            with open(os.path.join(RECORDS_DIR, name), encoding="utf-8") as f:
                text = f.read()
            m = re.match(r"^---\n(.*?)\n---", text, re.S)
            if m and any(line.strip() == "- " + rel for line in m.group(1).split("\n")):
                owners.append(name)

    if owners:
        note = ("編集した %s は実装記録 %s の対象です。作業の最後に記録本文を現行実装に合わせ、"
                "`python .claude/scripts/check_records.py --update` を実行してください。"
                % (rel, ", ".join(".claude/implementation-records/" + o for o in owners)))
    else:
        note = ("編集した %s はどの実装記録にも含まれていません。既存記録の frontmatter sources に追加するか、"
                "新しい記録を .claude/implementation-records/ に作り、_index.md にも載せてください。" % rel)
    print(json.dumps({"hookSpecificOutput": {"hookEventName": "PostToolUse", "additionalContext": note}}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception:
        sys.exit(0)
