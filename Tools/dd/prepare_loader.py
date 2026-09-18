"""The loading screen's emblem for the hospital: Dark Deception's magic circle with this game's Wasami symbol inside.

    python Tools/dd/prepare_loader.py

The original's level emblems (pak_reference_2 UI/Main/Loaders/loader_*.png, 512 x 512) share one magic circle — the
outer ring, the band of runes and the three small circles — and differ only in the character's mark drawn in red on the
black disk inside the thin inner ring. The hospital's is loader_reapernurse. This takes that emblem, paints its mark
out with the disk's black, and draws the user's symbol (SourceArt/Wasami/UI/wasami_symbol.png) in its place, in the
marks' red and at their size, then writes Intermediate/Pipeline/wasami/ui/loader_wasami.png for dd_ui.import_loading
(UWasamiLoadingWidget's LevelEmblems[7]). The result holds the original's art, so it stays out of git like the rest of
Intermediate/Pipeline; run this again after either input changes.

Env: PAK_REF2 — the export (default <repo>/pak_reference_2).
"""
import argparse
import os
import sys
from collections import deque

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))
FRAME = os.path.join(REF, "DDeception", "Content", "UI", "Main", "Loaders", "loader_reapernurse.png")
SYMBOL = os.path.join(ROOT, "SourceArt", "Wasami", "UI", "wasami_symbol.png")
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "wasami", "ui", "loader_wasami.png")

SIZE = 512
# The magic circle's middle (a least-squares fit to the outer ring's edge) and the black disk inside its thin inner
# ring, whose red is at 182-183 px from the middle; the small circles reach in to 164 px.
CENTRE = (254.5, 255.5)
INNER_RADIUS = 179.5
# The original marks: red 192 (the commonest value of all nine), 222-289 px high, their boxes centred near (256, 262).
MARK_RED = 192
MARK_HEIGHT = 280
MARK_CENTRE = (256, 262)
# Where the small circles begin; the symbol must stay inside it.
CLEAR_RADIUS = 164.0


def _distances():
    ys, xs = np.mgrid[0:SIZE, 0:SIZE]
    return np.hypot(xs - CENTRE[0], ys - CENTRE[1])


def mark_mask(emblem, distances):
    """The pixels of the mark: red ones inside the inner disk that are not joined to the magic circle (the small circles
    reach in over the disk's edge; the marks keep clear of it)."""
    red = emblem[..., 0] > 0
    inside = distances < INNER_RADIUS
    circle = np.zeros((SIZE, SIZE), bool)
    queue = deque()
    for y, x in zip(*np.nonzero(red & inside & (distances >= INNER_RADIUS - 4))):
        circle[y, x] = True
        queue.append((y, x))
    while queue:
        y, x = queue.popleft()
        for ny in (y - 1, y, y + 1):
            for nx in (x - 1, x, x + 1):
                if 0 <= ny < SIZE and 0 <= nx < SIZE and not circle[ny, nx] and red[ny, nx] and inside[ny, nx]:
                    circle[ny, nx] = True
                    queue.append((ny, nx))
    return red & inside & ~circle


def symbol_alpha(path):
    """The symbol's coverage (its alpha, 0-255) scaled to the marks' height and placed where they are."""
    alpha = np.array(Image.open(path).convert("RGBA"))[..., 3]
    ys, xs = np.nonzero(alpha)
    if not len(ys):
        raise ValueError("%s is empty" % path)
    crop = Image.fromarray(alpha[ys.min():ys.max() + 1, xs.min():xs.max() + 1])
    scale = MARK_HEIGHT / crop.height
    width, height = round(crop.width * scale), round(crop.height * scale)
    placed = np.zeros((SIZE, SIZE), np.int32)
    left, top = round(MARK_CENTRE[0] - width / 2), round(MARK_CENTRE[1] - height / 2)
    placed[top:top + height, left:left + width] = np.array(crop.resize((width, height), Image.LANCZOS))
    return placed


def compose(frame_path=FRAME, symbol_path=SYMBOL):
    emblem = np.array(Image.open(frame_path).convert("RGBA")).astype(np.int32)
    if emblem.shape[:2] != (SIZE, SIZE):
        raise ValueError("%s is %s, not %d x %d" % (frame_path, emblem.shape[:2], SIZE, SIZE))
    distances = _distances()
    mark = mark_mask(emblem, distances)
    emblem[mark] = (0, 0, 0, 255)
    alpha = symbol_alpha(symbol_path)
    reach = distances[alpha > 0].max()
    if reach >= CLEAR_RADIUS:
        raise ValueError("the symbol reaches %.1f px from the middle, over the small circles (%.0f)" % (reach, CLEAR_RADIUS))
    # Over the disk's black: red in proportion to the coverage.
    emblem[..., 0] = (emblem[..., 0] * (255 - alpha) + MARK_RED * alpha + 127) // 255
    return emblem.astype(np.uint8), int(mark.sum()), float(reach)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=OUT, help="where the emblem is written")
    args = parser.parse_args()
    for path in (FRAME, SYMBOL):
        if not os.path.exists(path):
            sys.exit("missing %s" % path)
    emblem, painted, reach = compose()
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    Image.fromarray(emblem).save(args.out)
    print("wrote %s (the mark's %d px painted out; the symbol reaches %.1f px from the middle)" % (args.out, painted, reach))


if __name__ == "__main__":
    main()
