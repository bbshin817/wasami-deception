"""Makes new 2D assets for Wasami Deception with GPT Image, using photos of Wasami as reference images.

    python Tools/wasami_art/wasami_art.py refs                              # the reference sets and whether each file is there
    python Tools/wasami_art/wasami_art.py catalog                           # contact sheets of the screenshot folder, to pick new refs
    python Tools/wasami_art/wasami_art.py gen Tools/wasami_art/briefs/example_poster.json [--count 2] [--dry-run]
    python Tools/wasami_art/wasami_art.py gen --name ward_sign --style poster --prompt "..."      # without a brief file
    python Tools/wasami_art/wasami_art.py adopt Intermediate/WasamiArt/<name>/<run>/out-01.png [--dest ...] [--size WxH]

The image calls go through the codex-gpt-image CLI (tmp/codex-gpt-image, outside git; WASAMI_GPT_IMAGE_CLI overrides the
path), which signs in with the local Codex OAuth session (~/.codex/auth.json), not an API key.

gen builds one prompt from styles.json (the common rules, then the style, then the brief), copies the chosen reference
images (refs.json; photos from the user's screenshot folder, cropped and shrunk) into the run folder, asks for the
images, and writes Intermediate/WasamiArt/<name>/<YYYYMMDD-HHMMSS>/: refs/, prompt.txt, meta.json, out-NN.png and
sheet.png (references on top, candidates below, numbered) for picking one. Everything there is outside git.

adopt copies one candidate to its destination under SourceArt/ (tracked by Git LFS; optionally resized) and appends
where it came from (brief, prompt, references, requested model) to SourceArt/Wasami/generated.json.
Exit code 0 on success, 1 on a failure.
"""
import argparse
import datetime
import glob
import json
import os
import subprocess
import sys

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT_ROOT = os.path.join(ROOT, "Intermediate", "WasamiArt")
PROVENANCE = os.path.join(ROOT, "SourceArt", "Wasami", "generated.json")
DEFAULT_CLI = os.path.join(ROOT, "tmp", "codex-gpt-image", "skills", "codex-gpt-image", "scripts", "codex_gpt_image.py")
REF_MAX_EDGE = 1536  # references are shrunk to this; the webcam shots are smaller anyway
MAX_REFS = 16  # the edits endpoint takes at most 16 input images
IMAGE_EXTS = (".png", ".jpg", ".jpeg", ".webp")


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def rel(path):
    return os.path.relpath(path, ROOT).replace("\\", "/")


def source_dirs(refs):
    dirs = dict(refs["source_dirs"])
    if os.environ.get("WASAMI_ART_SCREENSHOTS"):
        dirs["screenshots"] = os.environ["WASAMI_ART_SCREENSHOTS"]
    return {k: v if os.path.isabs(v) else os.path.join(ROOT, v) for k, v in dirs.items()}


def resolve_src(src, dirs):
    """'screenshots:<file>' → the file in that source folder; anything else is a path from the project root."""
    key, sep, name = src.partition(":")
    if sep and key in dirs:
        return os.path.join(dirs[key], name)
    return src if os.path.isabs(src) else os.path.join(ROOT, src)


def cmd_refs(args):
    refs = load_json(os.path.join(HERE, "refs.json"))
    dirs = source_dirs(refs)
    missing = 0
    for name, entry in refs["sets"].items():
        print(f"{name}: {entry.get('note', '')}")
        for item in entry["images"]:
            path = resolve_src(item["src"], dirs)
            ok = os.path.isfile(path)
            missing += not ok
            size = "x".join(map(str, Image.open(path).size)) if ok else "MISSING"
            print(f"  [{size}] {item['src']}  {item.get('note', '')}")
    return 1 if missing else 0


def cmd_catalog(args):
    refs = load_json(os.path.join(HERE, "refs.json"))
    folder = args.dir or source_dirs(refs)["screenshots"]
    files = sorted(f for f in glob.glob(os.path.join(folder, "*")) if f.lower().endswith(IMAGE_EXTS))
    out = os.path.join(OUT_ROOT, "catalog")
    os.makedirs(out, exist_ok=True)
    cols, rows, cw, ch = 8, 5, 240, 170
    with open(os.path.join(out, "index.txt"), "w", encoding="utf-8") as f:
        for i, path in enumerate(files):
            f.write(f"{i}\t{os.path.basename(path)}\n")
    for page in range(0, len(files), cols * rows):
        sheet = Image.new("RGB", (cols * cw, rows * ch), (40, 40, 40))
        draw = ImageDraw.Draw(sheet)
        for k, path in enumerate(files[page:page + cols * rows]):
            try:
                im = Image.open(path).convert("RGB")
            except OSError:
                continue
            im.thumbnail((cw - 4, ch - 20))
            x, y = (k % cols) * cw, (k // cols) * ch
            sheet.paste(im, (x + 2, y + 18))
            draw.text((x + 3, y + 2), str(page + k), fill=(255, 255, 0))
        sheet.save(os.path.join(out, f"sheet_{page // (cols * rows):02d}.png"))
    print(f"{len(files)} images; sheets and index.txt (number → file name) in {rel(out)}")
    return 0


def prepare_refs(set_names, extra, run_dir):
    """Copies the references into run_dir/refs as PNG (cropped, shrunk) and returns their paths."""
    refs = load_json(os.path.join(HERE, "refs.json"))
    dirs = source_dirs(refs)
    items = []
    for name in set_names:
        if name not in refs["sets"]:
            raise SystemExit(f"unknown reference set: {name} (refs.json has {', '.join(refs['sets'])})")
        items += refs["sets"][name]["images"]
    items += [{"src": p} for p in extra]
    if len(items) > MAX_REFS:
        raise SystemExit(f"{len(items)} reference images; the endpoint takes at most {MAX_REFS}")
    out_dir = os.path.join(run_dir, "refs")
    os.makedirs(out_dir, exist_ok=True)
    written = []
    for i, item in enumerate(items, start=1):
        path = resolve_src(item["src"], dirs)
        if not os.path.isfile(path):
            raise SystemExit(f"reference not found: {item['src']} ({path})")
        im = Image.open(path)
        im = im.convert("RGBA" if im.mode in ("RGBA", "LA", "P") else "RGB")
        if "crop" in item:
            l, t, r, b = item["crop"]
            im = im.crop((round(l * im.width), round(t * im.height), round(r * im.width), round(b * im.height)))
        im.thumbnail((REF_MAX_EDGE, REF_MAX_EDGE))
        dst = os.path.join(out_dir, f"ref-{i:02d}.png")
        im.save(dst)
        written.append({"src": item["src"], "file": rel(dst), "note": item.get("note", "")})
    return written


def build_prompt(styles, style, brief):
    parts = [styles["common"], styles["styles"][style]["prompt"], "Asset: " + brief["prompt"].strip()]
    if brief.get("text"):
        parts.append(f"Exact visible text, spelled exactly: \"{brief['text']}\". No other text.")
    return "\n\n".join(parts)


def contact_sheet(ref_files, out_files, path):
    """References in the top row (small), numbered candidates below, on a mid grey so transparency shows."""
    rw, cw = 160, 480
    refs = [Image.open(os.path.join(ROOT, f)).convert("RGBA") for f in ref_files]
    outs = [Image.open(f).convert("RGBA") for f in out_files]
    for im in refs:
        im.thumbnail((rw, rw))
    for im in outs:
        im.thumbnail((cw, cw))
    width = max(len(refs) * (rw + 6), len(outs) * (cw + 6), 400)
    height = rw + 30 + max((im.height for im in outs), default=0) + 30
    sheet = Image.new("RGBA", (width, height), (96, 96, 96, 255))
    draw = ImageDraw.Draw(sheet)
    for i, im in enumerate(refs):
        sheet.alpha_composite(im, (i * (rw + 6), 0))
    for i, im in enumerate(outs):
        x = i * (cw + 6)
        sheet.alpha_composite(im, (x, rw + 30))
        draw.text((x + 4, rw + 12), os.path.basename(out_files[i]), fill=(255, 255, 0))
    sheet.convert("RGB").save(path)


def cmd_gen(args):
    styles = load_json(os.path.join(HERE, "styles.json"))
    if args.brief:
        brief = load_json(args.brief)
        brief.setdefault("name", os.path.splitext(os.path.basename(args.brief))[0])
    else:
        if not (args.name and args.prompt):
            raise SystemExit("give a brief file, or --name and --prompt")
        brief = {"name": args.name, "prompt": args.prompt}
    for key in ("style", "text", "size", "background", "quality", "count"):
        if getattr(args, key, None) is not None:
            brief[key] = getattr(args, key)
    if args.refs:
        brief["refs"] = args.refs.split(",")
    style = brief.get("style", "portrait")
    if style not in styles["styles"]:
        raise SystemExit(f"unknown style: {style} (styles.json has {', '.join(styles['styles'])})")
    preset = styles["styles"][style]
    ref_sets = brief.get("refs", preset["refs"])

    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = os.path.join(OUT_ROOT, brief["name"], stamp)
    os.makedirs(run_dir, exist_ok=True)
    references = prepare_refs(ref_sets, brief.get("extra_refs", []) + (args.extra_ref or []), run_dir)
    prompt = build_prompt(styles, style, brief)
    prompt_file = os.path.join(run_dir, "prompt.txt")
    with open(prompt_file, "w", encoding="utf-8") as f:
        f.write(prompt)

    cli = os.environ.get("WASAMI_GPT_IMAGE_CLI", DEFAULT_CLI)
    if not os.path.isfile(cli):
        raise SystemExit(f"codex-gpt-image CLI not found: {cli} (set WASAMI_GPT_IMAGE_CLI)")
    request = {
        "model": brief.get("model"),
        "size": brief.get("size", preset["size"]),
        "background": brief.get("background", preset["background"]),
        "quality": brief.get("quality", preset["quality"]),
        "count": int(brief.get("count", 2)),
    }
    meta = {"brief": brief, "style": style, "ref_sets": ref_sets, "references": references, "request": request,
            "started": stamp, "cli": rel(cli) if cli.startswith(ROOT) else cli, "cli_log": []}
    print(f"run folder: {rel(run_dir)}  ({len(references)} references, {request['count']} image(s))", flush=True)
    # One call per image: the Codex backend returned one image for --count 2 (2026-09-26).
    returncode = 0
    for n in range(1, (1 if args.dry_run else request["count"]) + 1):
        cmd = [sys.executable, cli, "generate", "--prompt-file", prompt_file,
               "--out", os.path.join(run_dir, f"out-{n:02d}.png"), "--size", request["size"],
               "--background", request["background"], "--quality", request["quality"]]
        if request["model"]:
            cmd += ["--model", request["model"]]
        for r in references:
            cmd += ["--image", os.path.join(ROOT, r["file"])]
        if args.dry_run:
            cmd.append("--dry-run")
        proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        sys.stdout.flush()
        meta["cli_log"] += proc.stderr.strip().splitlines()[-2:]
        if proc.returncode != 0:
            meta["error"] = returncode = proc.returncode
            break
    outputs = [] if args.dry_run else sorted(glob.glob(os.path.join(run_dir, "out-*.png")))
    for path in outputs:
        with Image.open(path) as im:
            meta.setdefault("outputs", []).append({"file": rel(path), "size": list(im.size), "mode": im.mode})
    with open(os.path.join(run_dir, "meta.json"), "w", encoding="utf-8") as f:
        json.dump(meta, f, ensure_ascii=False, indent=2)
    if outputs:
        contact_sheet([r["file"] for r in references], outputs, os.path.join(run_dir, "sheet.png"))
        print(f"sheet: {rel(os.path.join(run_dir, 'sheet.png'))}")
    return 1 if returncode else 0


def cmd_adopt(args):
    src = os.path.abspath(args.image)
    meta_path = os.path.join(os.path.dirname(src), "meta.json")
    if not os.path.isfile(meta_path):
        raise SystemExit(f"no meta.json next to {args.image}; adopt takes a candidate from a gen run folder")
    meta = load_json(meta_path)
    dest = args.dest or meta["brief"].get("dest")
    if not dest:
        raise SystemExit("no destination: give --dest, or put \"dest\" in the brief")
    dest_abs = os.path.normpath(os.path.join(ROOT, dest))
    if not rel(dest_abs).startswith("SourceArt/"):
        raise SystemExit(f"destination must be under SourceArt/: {dest}")
    size = args.size or meta["brief"].get("final_size")
    im = Image.open(src)
    if size:
        w, h = (int(v) for v in size.lower().split("x"))
        im = im.resize((w, h), Image.LANCZOS)
    os.makedirs(os.path.dirname(dest_abs), exist_ok=True)
    im.save(dest_abs)
    record = load_json(PROVENANCE) if os.path.isfile(PROVENANCE) else []
    record = [r for r in record if r["dest"] != rel(dest_abs)]
    with open(os.path.join(os.path.dirname(src), "prompt.txt"), encoding="utf-8") as f:
        prompt = f.read()
    record.append({
        "dest": rel(dest_abs),
        "adopted": datetime.date.today().isoformat(),
        "run": rel(os.path.dirname(src)),
        "candidate": os.path.basename(src),
        "size": list(im.size),
        "brief": meta["brief"],
        "style": meta["style"],
        "references": [r["src"] for r in meta["references"]],
        "requested": meta["request"],
        "prompt": prompt,
        "note": args.note or "",
    })
    with open(PROVENANCE, "w", encoding="utf-8") as f:
        json.dump(record, f, ensure_ascii=False, indent=2)
        f.write("\n")
    print(f"{rel(dest_abs)} {im.size[0]}x{im.size[1]}; provenance in {rel(PROVENANCE)}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("refs")
    p = sub.add_parser("catalog")
    p.add_argument("--dir", help="folder to list (default: refs.json's screenshots folder)")
    p = sub.add_parser("gen")
    p.add_argument("brief", nargs="?", help="brief JSON (Tools/wasami_art/briefs/)")
    p.add_argument("--name")
    p.add_argument("--prompt")
    p.add_argument("--style")
    p.add_argument("--text", help="exact visible text")
    p.add_argument("--refs", help="comma-separated reference sets (overrides the brief and the style)")
    p.add_argument("--extra-ref", action="append", help="one more reference image (repeatable)")
    p.add_argument("--size")
    p.add_argument("--background", choices=["auto", "opaque", "transparent"])
    p.add_argument("--quality", choices=["auto", "low", "medium", "high", "xhigh", "max"])
    p.add_argument("--count", type=int)
    p.add_argument("--dry-run", action="store_true", help="build the run folder and prompt, send nothing")
    p = sub.add_parser("adopt")
    p.add_argument("image")
    p.add_argument("--dest", help="path under SourceArt/ (default: the brief's dest)")
    p.add_argument("--size", help="WxH to resize to (default: the brief's final_size)")
    p.add_argument("--note")
    args = parser.parse_args()
    return {"refs": cmd_refs, "catalog": cmd_catalog, "gen": cmd_gen, "adopt": cmd_adopt}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
