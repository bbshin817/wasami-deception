"""原作のブループリントのバイトコード（pak_reference*/_bytecode の .txt）を、入口から制御の流れで読む。

逆アセンブルの .txt は番地の順に並ぶので、ユーバーグラフは処理があちこちへ飛ぶ。
この道具は入口の番地から Jump / JumpIfNot / PushExecutionFlow / Delay の再開先をたどり、
届く基本ブロックだけを 1 文 1 行に縮めて出す。

  python Tools/dd/bp_flow.py <file.txt> --list                 # 関数とユーバーグラフの入口の一覧
  python Tools/dd/bp_flow.py <file.txt> DeathEvent             # イベント・関数の名前から
  python Tools/dd/bp_flow.py <file.txt> 34486                  # ユーバーグラフの番地から
  python Tools/dd/bp_flow.py <file.txt> "Get Current Progress"  # ユーバーグラフでない関数は本体を先頭から

番地の無い文（呼び出しだけの文は逆アセンブルに @ が付かない）の番地は、前後の番地から推して
いちばん近いものを当てる（L~ と印を付ける）。
"""

import argparse
import re
import sys

STMT_RE = re.compile(r"^@(\d+) (.*)$")
FUNC_RE = re.compile(r"^=== (.+?)  \(flags")


class Node:
    def __init__(self, text, indent):
        self.text = text
        self.indent = indent
        self.children = []


def parse_tree(lines):
    """インデントの木にする。lines は (indent, text)。"""
    root = Node("", -1)
    stack = [root]
    for indent, text in lines:
        if text == ")":
            continue
        node = Node(text, indent)
        while stack[-1].indent >= indent:
            stack.pop()
        stack[-1].children.append(node)
        stack.append(node)
    return root.children


def short_obj(path):
    # /Game/Blueprints/Main/BP_Shard.BP_Shard_C -> BP_Shard_C
    tail = path.rsplit("/", 1)[-1]
    if "." in tail:
        tail = tail.split(".", 1)[1]
    return tail


def fmt_float(s):
    try:
        v = float(s)
    except ValueError:
        return s
    r = round(v, 4)
    return ("%g" % r)


def render(node):
    """木の 1 節を 1 行の式にする。"""
    t = re.sub(r"^@\d+ ", "", node.text)
    kids = node.children
    labeled = {}
    attrs = {}
    plain = []
    for k in kids:
        kt = re.sub(r"^@\d+ ", "", k.text)
        if kt.startswith("case ") and kt.endswith(":"):
            plain.append(k)
        elif kt.endswith(":") and " " not in kt:
            labeled[kt[:-1]] = k.children
        elif " = " in kt and not kt.endswith("("):
            a, b = kt.split(" = ", 1)
            attrs[a] = b
        elif kt == ")":
            continue
        else:
            plain.append(k)

    def one(key):
        vs = labeled.get(key, [])
        return ", ".join(render(v) for v in vs) if vs else "?"

    if t.startswith("LocalVariable ") or t.startswith("LocalOutVariable "):
        return t.split(" ", 1)[1]
    if t.startswith("InstanceVariable "):
        return "this." + t.split(" ", 1)[1]
    if t.startswith("DefaultVariable "):
        return "default." + t.split(" ", 1)[1]
    if t.startswith("FloatConst "):
        return fmt_float(t.split(" ", 1)[1])
    if t.startswith(("IntConst ", "ByteConst ", "Int64Const ", "UInt64Const ", "SkipOffsetConst ")):
        return t.split(" ", 1)[1]
    if t.startswith(("StringConst ", "NameConst ", "TextConst ", "UnicodeStringConst ")):
        return t.split(" ", 1)[1]
    if t.startswith("ObjectConst "):
        return short_obj(t.split(" ", 1)[1])
    if t in ("Self", "True", "False", "NoObject", "Nothing", "NoInterface"):
        return {"NoObject": "None", "Nothing": "-"}.get(t, t)
    if t.startswith(("VectorConst", "RotationConst", "TransformConst")):
        return t
    if t == "StructConst":
        st = attrs.get("struct", "")
        vals = labeled.get("values", [])
        if st.endswith("LatentActionInfo"):
            return "resume@L%s" % render(vals[0]) if vals else "latent"
        return "%s{%s}" % (st.rsplit(".", 1)[-1], ", ".join(render(v) for v in vals))
    if t in ("Let", "LetObj", "LetBool", "LetValueOnPersistentFrame", "LetWeakObjPtr",
             "LetDelegate", "LetMulticastDelegate", "LetInterface"):
        tgt = one("target") if "target" in labeled else attrs.get("property", "?").rsplit(".", 1)[-1]
        return "%s = %s" % (tgt, one("value"))
    if t == "Context" or t == "ContextFailSilent" or t == "ClassContext":
        return "%s.%s" % (one("object"), one("member"))
    if t == "InterfaceContext":
        return one("value") if "value" in labeled else "iface(%s)" % ", ".join(render(p) for p in plain)
    if t in ("DynamicCast", "MetaCast", "ObjToInterfaceCast", "CrossInterfaceCast", "InterfaceToObjCast"):
        return "cast<%s>(%s)" % (short_obj(attrs.get("class", "?")), one("value"))
    if t == "Jump":
        return "goto L%s" % attrs.get("offset_to")
    if t == "JumpIfNot":
        return "if not (%s) goto L%s" % (one("condition"), attrs.get("offset_to"))
    if t == "PushExecutionFlow":
        return "push L%s" % attrs.get("offset_to")
    if t == "PopExecutionFlowIfNot":
        return "if not (%s) pop" % one("value")
    if t == "ComputedJump":
        return "computed goto %s" % one("value")
    if t == "Return":
        return "return %s" % one("value")
    if t == "SetArray":
        vals = labeled.get("values", [])
        return "%s = [%s]" % (one("target"), ", ".join(render(v) for v in vals))
    if t in ("CallMulticastDelegate",):
        return "broadcast %s" % one("delegate")
    if t in ("AddMulticastDelegate", "BindDelegate", "RemoveMulticastDelegate"):
        body = ", ".join("%s=%s" % (k, ", ".join(render(v) for v in vs)) for k, vs in labeled.items())
        extra = ", ".join("%s=%s" % (k, v) for k, v in attrs.items())
        return "%s(%s)" % (t, ", ".join(x for x in (body, extra) if x))
    if t == "StructMemberContext":
        member = attrs.get("property", "?").rsplit(".", 1)[-1]
        member = re.sub(r"_\d+_[0-9A-F]{32}$", "", member)
        return "%s.%s" % (one("target"), member)
    if t in ("ArrayGetByRef",):
        return "%s[%s]" % (one("target"), one("value"))
    if t == "SwitchValue":
        cases = []
        for p in plain:
            if p.text.startswith("case ") and len(p.children) >= 2:
                cases.append("%s:%s" % (render(p.children[0]), render(p.children[1])))
        if "default" in labeled:
            cases.append("else:%s" % one("default"))
        return "switch(%s){%s}" % (one("index"), ", ".join(cases))
    if t == "PrimitiveCast":
        return "bool(%s)" % one("value") if attrs.get("conversion") == "71" else "pcast(%s)" % one("value")
    if t.endswith("("):
        head = t[:-1]
        name = head.split(" ", 1)[1] if " " in head else head
        return "%s(%s)" % (name, ", ".join(render(p) for p in plain))
    # 不明な形はそのまま
    inner = []
    for k, vs in labeled.items():
        inner.append("%s: %s" % (k, ", ".join(render(v) for v in vs)))
    for k, v in attrs.items():
        inner.append("%s=%s" % (k, v))
    for p in plain:
        inner.append(render(p))
    return "%s(%s)" % (t, "; ".join(inner)) if inner else t


def load(path):
    funcs = {}
    order = []
    cur = None
    body = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            m = FUNC_RE.match(line)
            if m:
                if cur:
                    funcs[cur] = body
                cur = m.group(1)
                order.append(cur)
                body = []
                continue
            if cur is None or line.strip() == "" or line == "EndOfScript":
                continue
            indent = len(line) - len(line.lstrip(" "))
            body.append((indent, line.strip()))
    if cur:
        funcs[cur] = body
    return funcs, order


def statements(body):
    """トップレベルの文の一覧 [(offset or None, Node)]。"""
    out = []
    for node in parse_tree(body):
        m = STMT_RE.match(node.text)
        out.append((int(m.group(1)) if m else None, node))
    return out


def resolve(stmts, target):
    """番地 target の文の添字。無ければ番地の無い文から推す。(index, exact)"""
    for i, (off, _) in enumerate(stmts):
        if off == target:
            return i, True
    prev = -1
    for i, (off, _) in enumerate(stmts):
        if off is not None and off < target:
            prev = i
    j = prev + 1
    if j < len(stmts) and stmts[j][0] is None:
        return j, False
    return None, False


def entries(funcs):
    """ユーバーグラフの入口: 関数名 -> 番地。"""
    res = {}
    for name, body in funcs.items():
        if name.startswith("ExecuteUbergraph"):
            continue
        text = "\n".join(t for _, t in body)
        m = re.search(r"ExecuteUbergraph_\w+\(\n(?:IntConst )?(\d+)", text)
        if m:
            res[name] = int(m.group(1))
    return res


def trace(stmts, start_idx, max_blocks=400):
    order = []
    seen = set()
    queue = [start_idx]
    blocks = {}
    while queue and len(order) < max_blocks:
        i = queue.pop(0)
        if i is None or i in seen:
            continue
        lines = []
        j = i
        while j < len(stmts):
            if j != i and j in seen:
                lines.append(("", "(続きは L%s)" % label(stmts, j)))
                break
            seen.add(j)
            off, node = stmts[j]
            txt = render(node)
            lines.append((label(stmts, j), txt))
            succ = []
            for m in re.finditer(r"(?:goto |push |resume@)L(\d+)", txt):
                k, _ = resolve(stmts, int(m.group(1)))
                succ.append(k)
            queue.extend(succ)
            if txt.startswith(("goto ", "return", "computed goto")) or txt == "PopExecutionFlow":
                break
            j += 1
        blocks[i] = lines
        order.append(i)
    return order, blocks


def label(stmts, idx):
    off = stmts[idx][0]
    if off is not None:
        return str(off)
    prev = next((stmts[k][0] for k in range(idx - 1, -1, -1) if stmts[k][0] is not None), 0)
    return "~%d" % prev


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file")
    ap.add_argument("entry", nargs="?")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--max-blocks", type=int, default=400)
    a = ap.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")
    funcs, order = load(a.file)
    ub = next((n for n in order if n.startswith("ExecuteUbergraph")), None)
    ents = entries(funcs)
    if a.list or not a.entry:
        for n in order:
            print("%-60s %s" % (n, ("-> L%d" % ents[n]) if n in ents else ""))
        return
    if a.entry.isdigit():
        fname, start = ub, int(a.entry)
    elif a.entry in ents:
        fname, start = ub, ents[a.entry]
    elif a.entry in funcs:
        fname, start = a.entry, None
    else:
        sys.exit("見つからない: %s" % a.entry)
    stmts = statements(funcs[fname])
    if start is None:
        idx = 0
    else:
        idx, exact = resolve(stmts, start)
        if idx is None:
            sys.exit("番地 %d の文が無い" % start)
    print("# %s から (%s)" % (a.entry, fname))
    order_, blocks = trace(stmts, idx, a.max_blocks)
    for i in sorted(order_, key=lambda k: order_.index(k)):
        print("\nL%s:" % label(stmts, i))
        for lab, txt in blocks[i]:
            print("  %-7s %s" % (lab, txt))


if __name__ == "__main__":
    main()
