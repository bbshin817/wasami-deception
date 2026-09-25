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
import concurrent.futures
import datetime
import glob
import json
import os
import subprocess
import sys

import numpy as np
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
    text = brief.get("text")
    if isinstance(text, list):
        pieces = "; ".join(f"\"{t}\"" for t in text)
        parts.append(f"Exact visible text, as {len(text)} separate pieces placed as described, spelled exactly: "
                     f"{pieces}. No other text.")
    elif text:
        parts.append(f"Exact visible text, spelled exactly: \"{text}\". No other text.")
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


def load_briefs(args):
    """Each brief file holds one brief or a list of them; --only keeps the named ones."""
    briefs = []
    for path in args.brief:
        data = load_json(path)
        for brief in data if isinstance(data, list) else [data]:
            brief.setdefault("name", os.path.splitext(os.path.basename(path))[0])
            briefs.append(brief)
    if not args.brief:
        if not (args.name and args.prompt):
            raise SystemExit("give brief files, or --name and --prompt")
        briefs.append({"name": args.name, "prompt": args.prompt})
    if args.only:
        keep = set(args.only.split(","))
        briefs = [b for b in briefs if b["name"] in keep]
        missing = keep - {b["name"] for b in briefs}
        if missing:
            raise SystemExit(f"not in the briefs: {', '.join(sorted(missing))}")
    for brief in briefs:
        for key in ("style", "text", "size", "background", "quality", "count"):
            if getattr(args, key, None) is not None:
                brief[key] = getattr(args, key)
        if args.refs:
            brief["refs"] = args.refs.split(",")
    return briefs


def cmd_gen(args):
    briefs = load_briefs(args)
    if len(briefs) == 1 or args.jobs <= 1:
        return max(gen_one(brief, args) for brief in briefs)
    # Several briefs side by side; each brief still asks for its images one after another.
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        return max(pool.map(lambda brief: gen_one(brief, args), briefs))


def gen_one(brief, args):
    styles = load_json(os.path.join(HERE, "styles.json"))
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
    print(f"{brief['name']}: run folder {rel(run_dir)}  ({len(references)} references, {request['count']} image(s))",
          flush=True)
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
        print(f"{brief['name']}: sheet {rel(os.path.join(run_dir, 'sheet.png'))}", flush=True)
    return 1 if returncode else 0


def fit_size(im, size):
    """Matches the target's aspect ratio (the backend's sizes drift), then resizes. A transparent image gets
    transparent margins so nothing drawn is cut. An opaque one is cropped in the middle when that takes at most
    CROP_LIMIT of a side; otherwise it gets margins in its median border colour (a crop would cut titles off)."""
    w, h = (int(v) for v in size.lower().split("x"))
    if abs(im.width / im.height - w / h) > 0.01:
        pw, ph = max(im.width, round(im.height * w / h)), max(im.height, round(im.width * h / w))
        cut = 1 - min(im.width / pw * ph / im.height, im.height / ph * pw / im.width)
        if im.mode == "RGBA" or cut > CROP_LIMIT:
            if im.mode == "RGBA":
                fill = (255, 255, 255, 0)
            else:
                im = im.convert("RGB")
                a = np.asarray(im)
                border = np.concatenate([a[0], a[-1], a[:, 0], a[:, -1]])
                fill = tuple(int(v) for v in np.median(border, axis=0))
            padded = Image.new(im.mode, (pw, ph), fill)
            padded.paste(im, ((pw - im.width) // 2, (ph - im.height) // 2))
            im = padded
        elif im.width / im.height > w / h:
            cw = round(im.height * w / h)
            im = im.crop(((im.width - cw) // 2, 0, (im.width - cw) // 2 + cw, im.height))
        else:
            ch = round(im.width * h / w)
            im = im.crop((0, (im.height - ch) // 2, im.width, (im.height - ch) // 2 + ch))
    return im.resize((w, h), Image.LANCZOS)


CROP_LIMIT = 0.08


def flatten(im, colour):
    """An opaque image on the colour RRGGBB (the backend returns transparency even when asked for opaque)."""
    bg = tuple(int(colour[i:i + 2], 16) for i in (0, 2, 4))
    flat = Image.new("RGBA", im.size, bg + (255,))
    flat.alpha_composite(im.convert("RGBA"))
    return flat.convert("RGB")


def white_alpha(im):
    """White paint only, the way the original graffiti decals are made: RGB white, alpha = coverage x brightness."""
    a = np.asarray(im.convert("RGBA")).astype(np.float32)
    alpha = a[..., 3] * a[..., :3].mean(axis=2) / 255.0
    alpha[alpha < 8] = 0
    out = np.empty(a.shape, np.uint8)
    out[..., :3] = 255
    out[..., 3] = alpha.round().clip(0, 255).astype(np.uint8)
    return Image.fromarray(out)


POSTS = {"white_alpha": white_alpha}


def write_provenance(entry):
    record = load_json(PROVENANCE) if os.path.isfile(PROVENANCE) else []
    record = [r for r in record if r["dest"] != entry["dest"]]
    record.append(entry)
    record.sort(key=lambda r: r["dest"])
    with open(PROVENANCE, "w", encoding="utf-8") as f:
        json.dump(record, f, ensure_ascii=False, indent=2)
        f.write("\n")


def source_dest(path):
    dest_abs = os.path.normpath(os.path.join(ROOT, path))
    if not rel(dest_abs).startswith("SourceArt/"):
        raise SystemExit(f"destination must be under SourceArt/: {path}")
    os.makedirs(os.path.dirname(dest_abs), exist_ok=True)
    return dest_abs


def cmd_adopt(args):
    src = os.path.abspath(args.image)
    meta_path = os.path.join(os.path.dirname(src), "meta.json")
    if not os.path.isfile(meta_path):
        raise SystemExit(f"no meta.json next to {args.image}; adopt takes a candidate from a gen run folder")
    meta = load_json(meta_path)
    dest = args.dest or meta["brief"].get("dest")
    if not dest:
        raise SystemExit("no destination: give --dest, or put \"dest\" in the brief")
    dest_abs = source_dest(dest)
    size = args.size or meta["brief"].get("final_size")
    post = meta["brief"].get("post")
    im = Image.open(src)
    if post:
        im = POSTS[post](im)
    if meta["brief"].get("bg"):
        im = flatten(im, meta["brief"]["bg"])
    if size:
        im = fit_size(im, size)
    im.save(dest_abs)
    with open(os.path.join(os.path.dirname(src), "prompt.txt"), encoding="utf-8") as f:
        prompt = f.read()
    write_provenance({
        "dest": rel(dest_abs),
        "adopted": datetime.date.today().isoformat(),
        "run": rel(os.path.dirname(src)),
        "candidate": os.path.basename(src),
        "size": list(im.size),
        "post": post,
        "bg": meta["brief"].get("bg"),
        "brief": meta["brief"],
        "style": meta["style"],
        "references": [r["src"] for r in meta["references"]],
        "requested": meta["request"],
        "prompt": prompt,
        "note": args.note or "",
    })
    print(f"{rel(dest_abs)} {im.size[0]}x{im.size[1]}; provenance in {rel(PROVENANCE)}")
    return 0


def latest_candidates(name):
    runs = sorted(glob.glob(os.path.join(OUT_ROOT, name, "*", "out-*.png")))
    if not runs:
        return []
    last_run = os.path.dirname(runs[-1])
    return sorted(glob.glob(os.path.join(last_run, "out-*.png")))


def cmd_review(args):
    """One sheet with each brief's newest candidates as adopt would write them (post and final_size applied), on a
    dark wall colour so white paint and transparency show. Writes Intermediate/WasamiArt/review_<file>.png."""
    briefs = load_briefs(args)
    cell = 420
    tiles = []
    for brief in briefs:
        for path in latest_candidates(brief["name"]):
            im = Image.open(path)
            if brief.get("post"):
                im = POSTS[brief["post"]](im)
            if brief.get("bg"):
                im = flatten(im, brief["bg"])
            if brief.get("final_size"):
                im = fit_size(im, brief["final_size"])
            im = im.convert("RGBA")
            im.thumbnail((cell, cell - 24))
            tiles.append((f"{brief['name'].replace('hospital_', '')} {os.path.basename(path)}", im))
    if not tiles:
        raise SystemExit("no candidates yet")
    cols = min(args.cols, len(tiles))
    rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell, rows * cell), (58, 52, 48, 255))
    draw = ImageDraw.Draw(sheet)
    for i, (label, im) in enumerate(tiles):
        x, y = (i % cols) * cell, (i // cols) * cell
        sheet.alpha_composite(im, (x + (cell - im.width) // 2, y + 22))
        draw.text((x + 4, y + 4), label, fill=(255, 230, 0))
    out = os.path.join(OUT_ROOT, "review_" + "_".join(os.path.splitext(os.path.basename(b))[0] for b in args.brief) + ".png")
    sheet.convert("RGB").save(out)
    print(rel(out))
    return 0


def on_background(path, bg):
    """The image flattened onto the background colour (the backend sometimes returns transparency unasked)."""
    im = Image.open(path).convert("RGBA")
    flat = Image.new("RGBA", im.size, bg + (255,))
    flat.alpha_composite(im)
    return flat.convert("RGB")


def video_frames(args, bg):
    """The still images the video is made of: the candidates given, or the cells of one grid image."""
    if args.grid:
        cols, rows = (int(v) for v in args.grid.lower().split("x"))
        sheet = on_background(args.frames[0], bg)
        cw, ch = sheet.width / cols, sheet.height / rows
        return [sheet.crop((round(c * cw), round(r * ch), round((c + 1) * cw), round((r + 1) * ch)))
                for r in range(rows) for c in range(cols)]
    return [on_background(f, bg) for f in args.frames]


def letterbox(im, w, h, bg, dy=0.0):
    """im fitted inside w x h, centred and moved down by dy pixels, on the background colour."""
    k = min(w / im.width, h / im.height)
    fitted = im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)
    canvas = Image.new("RGB", (w, h), bg)
    canvas.paste(fitted, ((w - fitted.width) // 2, round((h - fitted.height) / 2 + dy)))
    return canvas


VIDEO_DEFAULTS = {"out": None, "grid": None, "size": "1280x720", "fps": 25.0, "duration": None, "mode": "sequence",
                  "xfade": 0.35, "zoom": 0.04, "blink": 0.0, "bob": 0.01, "bobs": 4, "bg": "000000"}


def cmd_video(args):
    """Builds a looping mp4 from generated stills, with the original movie's length, size and frame rate.

    sequence: the stills in turn, each held and then crossfaded into the next (the last back into the first), with
    a slow zoom that returns to its start, so the loop has no seam. bounce: the first still bobs up and down (the
    original ambulance clip); with several stills they alternate every --blink seconds (a flashing siren).
    """
    # Unset options come from the brief's "video" (next to the first frame), then the defaults.
    meta_path = os.path.join(os.path.dirname(os.path.abspath(args.frames[0])), "meta.json")
    preset = load_json(meta_path)["brief"].get("video", {}) if os.path.isfile(meta_path) else {}
    for key, default in VIDEO_DEFAULTS.items():
        if getattr(args, key) is None:
            setattr(args, key, preset.get(key, default))
    if not (args.out and args.duration):
        raise SystemExit("give --out and --duration, or a brief with \"video\": {\"out\", \"duration\"}")
    w, h = (int(v) for v in args.size.lower().split("x"))
    bg = tuple(int(args.bg[i:i + 2], 16) for i in (0, 2, 4))
    stills = video_frames(args, bg)
    n_out = round(args.duration * args.fps)
    dest_abs = source_dest(args.out)
    cmd = ["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{w}x{h}",
           "-r", str(args.fps), "-i", "-", "-an", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "18",
           "-movflags", "+faststart", dest_abs]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    base = [letterbox(im, w, h, bg) for im in stills]
    for i in range(n_out):
        t = i / n_out  # 0..1 over the loop
        if args.mode == "sequence":
            pos = t * len(base)
            k, frac = int(pos), pos - int(pos)
            cur, nxt = base[k], base[(k + 1) % len(base)]
            fade = max(0.0, (frac - (1 - args.xfade)) / args.xfade) if args.xfade > 0 else 0.0
            frame = Image.blend(cur, nxt, fade) if fade > 0 else cur
            if args.zoom:
                z = 1 + args.zoom * (0.5 - 0.5 * np.cos(2 * np.pi * t))
                zw, zh = w / z, h / z
                frame = frame.crop(((w - zw) / 2, (h - zh) / 2, (w + zw) / 2, (h + zh) / 2)).resize(
                    (w, h), Image.BILINEAR)
        else:
            k = int(i / args.fps / args.blink) % len(stills) if args.blink else 0
            dy = args.bob * h * np.sin(2 * np.pi * t * args.bobs)
            frame = letterbox(stills[k], w, h, bg, dy=dy)
        proc.stdin.write(frame.tobytes())
    proc.stdin.close()
    if proc.wait() != 0:
        return 1
    briefs = set()
    for f in args.frames:
        meta_path = os.path.join(os.path.dirname(os.path.abspath(f)), "meta.json")
        if os.path.isfile(meta_path):
            briefs.add(load_json(meta_path)["brief"]["name"])
    write_provenance({
        "dest": rel(dest_abs),
        "adopted": datetime.date.today().isoformat(),
        "kind": "video",
        "frames": [rel(os.path.abspath(f)) for f in args.frames],
        "video": {k: getattr(args, k) for k in ("grid", "size", "fps", "duration", "mode", "xfade", "zoom",
                                                "blink", "bob", "bobs", "bg")},
        "briefs": sorted(briefs),
        "note": args.note or "",
    })
    print(f"{rel(dest_abs)} {w}x{h} {args.fps} fps {n_out} frames; provenance in {rel(PROVENANCE)}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("refs")
    p = sub.add_parser("catalog")
    p.add_argument("--dir", help="folder to list (default: refs.json's screenshots folder)")
    p = sub.add_parser("gen")
    p.add_argument("brief", nargs="*", help="brief JSON files (Tools/wasami_art/briefs/); a file may hold a list")
    p.add_argument("--only", help="comma-separated brief names to run")
    p.add_argument("--jobs", type=int, default=1, help="briefs to run side by side")
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
    p = sub.add_parser("review")
    p.add_argument("brief", nargs="+")
    p.add_argument("--only")
    p.add_argument("--cols", type=int, default=4)
    p.set_defaults(name=None, prompt=None, refs=None, style=None, text=None, size=None, background=None,
                   quality=None, count=None)
    p = sub.add_parser("video")
    p.add_argument("frames", nargs="+", help="candidate images in order, or one grid image with --grid")
    p.add_argument("--out", help="mp4 path under SourceArt/")
    p.add_argument("--grid", help="CxR: cut the one image into C columns and R rows, read row by row")
    p.add_argument("--size", help="default 1280x720")
    p.add_argument("--fps", type=float, help="default 25")
    p.add_argument("--duration", type=float, help="seconds (the original movie's length)")
    p.add_argument("--mode", choices=["sequence", "bounce"], help="default sequence")
    p.add_argument("--xfade", type=float, help="sequence: share of each still spent crossfading (0.35)")
    p.add_argument("--zoom", type=float, help="sequence: slow zoom in and back out (0.04)")
    p.add_argument("--blink", type=float, help="bounce: seconds per still when there are several (0)")
    p.add_argument("--bob", type=float, help="bounce: height of the bob, share of the frame height (0.01)")
    p.add_argument("--bobs", type=int, help="bounce: bobs per loop (4)")
    p.add_argument("--bg", help="background colour RRGGBB (000000)")
    p.add_argument("--note")
    args = parser.parse_args()
    return {"refs": cmd_refs, "catalog": cmd_catalog, "gen": cmd_gen, "adopt": cmd_adopt,
            "video": cmd_video,
            "review": cmd_review}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
