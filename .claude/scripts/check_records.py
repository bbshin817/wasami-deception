#!/usr/bin/env python
"""実装記録（.claude/implementation-records/*.md）とソースの同期チェック。

    python .claude/scripts/check_records.py            # 差分を報告（問題があれば exit 1）
    python .claude/scripts/check_records.py --update   # 記録を直した後、ハッシュを更新
    python .claude/scripts/check_records.py --update 02-player.md
    python .claude/scripts/check_records.py --json     # 機械可読出力

各記録の frontmatter `sources:` に列挙したファイルの git blob ハッシュを _hashes.json に保存し、ソースが変わった記録
（stale）、記録がないソース（uncovered）、記録が参照しているが存在しないソース（missing）、ハッシュ未登録（unhashed）を
検出する。WebGL 版の .claude/scripts/check-records.mjs の移植。
"""
import datetime
import hashlib
import json
import os
import re
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
RECORDS_DIR = os.path.join(ROOT, ".claude", "implementation-records")
HASHES_PATH = os.path.join(RECORDS_DIR, "_hashes.json")

# 記録の対象にするファイル（リポジトリ相対）。この範囲内で記録に載っていないものを uncovered とする。
SCOPE_PREFIXES = ("Source/", "Content/Python/", "Tools/", "Config/")
SCOPE_FILES = ("wasami_deception.uproject", ".mcp.json")
SCOPE_EXT = re.compile(r"\.(cpp|h|cs|py|ini|uproject|json)$")


def git_blob_hash(path):
    with open(path, "rb") as f:
        data = f.read()
    # 行末は LF で比べる。.gitattributes の eol=lf で git が入れるのは LF の中身なので、作業コピーが
    # CRLF（Windows の編集の道具や open(p, 'w') が書く）でも同じハッシュになる。NUL を含む中身は git と同じく変えない
    if b"\0" not in data:
        data = data.replace(b"\r\n", b"\n")
    h = hashlib.sha1()
    h.update(b"blob %d\0" % len(data))
    h.update(data)
    return h.hexdigest()


def in_scope(rel):
    if rel in SCOPE_FILES:
        return True
    return rel.startswith(SCOPE_PREFIXES) and bool(SCOPE_EXT.search(rel))


def scope_files():
    def ls(*args):
        out = subprocess.run(["git", "-C", ROOT, "ls-files", "-z", *args], capture_output=True)
        return out.stdout.decode("utf-8").split("\0")
    files = set(ls()) | set(ls("--others", "--exclude-standard"))
    return sorted(f for f in files if f and in_scope(f))


def parse_sources(text):
    m = re.match(r"^---\n(.*?)\n---", text, re.S)
    if not m:
        return []
    sources, in_sources = [], False
    for line in m.group(1).split("\n"):
        if re.match(r"^sources:\s*$", line):
            in_sources = True
            continue
        if in_sources:
            item = re.match(r"^\s+-\s+(.+?)\s*$", line)
            if item:
                sources.append(item.group(1))
                continue
            in_sources = False
    return sources


def load_records():
    out = []
    for name in sorted(os.listdir(RECORDS_DIR)):
        if not name.endswith(".md") or name.startswith("_"):
            continue
        with open(os.path.join(RECORDS_DIR, name), encoding="utf-8") as f:
            text = f.read()
        out.append({"file": name, "text": text, "sources": parse_sources(text)})
    return out


def main():
    args = sys.argv[1:]
    update = "--update" in args
    as_json = "--json" in args
    only = [a for a in args if not a.startswith("--")]

    if not os.path.isdir(RECORDS_DIR):
        print("実装記録のフォルダがありません: " + RECORDS_DIR, file=sys.stderr)
        return 1
    hashes = {}
    if os.path.exists(HASHES_PATH):
        with open(HASHES_PATH, encoding="utf-8") as f:
            hashes = json.load(f)
    records = load_records()
    report = {"stale": [], "missing": [], "unhashed": [], "uncovered": [], "updated": []}
    covered = set()

    for rec in records:
        stored = hashes.get(rec["file"], {})
        fresh, changed = {}, False
        for src in rec["sources"]:
            covered.add(src)
            path = os.path.join(ROOT, src.replace("/", os.sep))
            if not os.path.exists(path):
                report["missing"].append((rec["file"], src))
                continue
            h = git_blob_hash(path)
            fresh[src] = h
            if src not in stored:
                report["unhashed"].append((rec["file"], src))
                changed = True
            elif stored[src] != h:
                report["stale"].append((rec["file"], src))
                changed = True
        if any(s not in fresh for s in stored):
            changed = True
        if update and changed and (not only or rec["file"] in only):
            hashes[rec["file"]] = fresh
            today = datetime.date.today().isoformat()
            text = re.sub(r"^(---\n.*?\nupdated:\s*)\S+", r"\g<1>" + today, rec["text"], count=1, flags=re.S)
            if text != rec["text"]:
                with open(os.path.join(RECORDS_DIR, rec["file"]), "w", encoding="utf-8", newline="") as f:
                    f.write(text)
            report["updated"].append(rec["file"])

    for rel in scope_files():
        if rel not in covered:
            report["uncovered"].append(rel)

    if update:
        for key in [k for k in hashes if not any(r["file"] == k for r in records)]:
            del hashes[key]
        with open(HASHES_PATH, "w", encoding="utf-8", newline="") as f:
            json.dump(hashes, f, ensure_ascii=False, indent=2)
            f.write("\n")
        report["stale"] = [s for s in report["stale"] if s[0] not in report["updated"]]
        report["unhashed"] = [s for s in report["unhashed"] if s[0] not in report["updated"]]

    problems = len(report["stale"]) + len(report["missing"]) + len(report["unhashed"]) + len(report["uncovered"])
    if as_json:
        print(json.dumps({"ok": problems == 0, **{k: [list(v) if isinstance(v, tuple) else v for v in vals] for k, vals in report.items()}}, ensure_ascii=False, indent=2))
    else:
        if report["updated"]:
            print("ハッシュを更新: " + ", ".join(report["updated"]))
        if report["stale"]:
            print("【要更新】ソースが変わったのに記録が更新されていません:")
            for rec, src in report["stale"]:
                print("  %s  <-  %s" % (rec, src))
        if report["unhashed"]:
            print("【未登録】記録に列挙されているがハッシュ未保存（記録を確認後 --update）:")
            for rec, src in report["unhashed"]:
                print("  %s  <-  %s" % (rec, src))
        if report["missing"]:
            print("【欠落】記録が参照しているソースが存在しません（frontmatter の sources を直す）:")
            for rec, src in report["missing"]:
                print("  %s  ->  %s" % (rec, src))
        if report["uncovered"]:
            print("【未記録】どの記録にも含まれていないソース（既存記録の sources に追加するか新規記録を作る）:")
            for rel in report["uncovered"]:
                print("  " + rel)
        if problems == 0:
            print("OK: %d 件の記録はソースと同期しています。" % len(records))
    return 0 if problems == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
