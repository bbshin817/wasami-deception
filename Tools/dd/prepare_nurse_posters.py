"""The hospital's three pictures of the original's nurse, redrawn with Wasami's head.

    python Tools/dd/prepare_nurse_posters.py [--only <name>] [--preview]

The original's Torment Therapy has three textures that draw the nurse herself (Textures/06_Hospital):
hospital_poster_nurse_01_D (the pop-art "TAKE YOUR MEDICINE!", 16 in Zone 1), hospital_poster_nurse_02
(the red "GET VACCINATED!" silhouettes, 7 in Zone 1) and hospital_decal_nurseambulance (the doodle on the
ambulance's roof, 1 in Zone 1 and 2 in Zone 2). This game draws no character of the original
(.claude/guides/original-fidelity.md), so each one keeps its composition and its words and only has the
nurse's head (the paper bag with the cross, the silhouette's head, the doodle's cap) replaced by Wasami's,
as the WebGL version did to Chaotic Customer 2's posters (references/webgl/implementation-records/13):
the head's box is filled with the colours around it and Wasami's head (SourceArt/Wasami/UI/pause_head.png,
white with the ink in its alpha) is drawn in its place, in each picture's own style.

It writes Intermediate/Pipeline/wasami/stage/<out>.png for Tools/dd/prepare_stage.py, which imports them as
/Game/Wasami/Stage/T_* in the place of the original's textures. --preview also writes _nurse_sheet.png,
the originals and the new ones side by side.

Env: PAK_REF2 — the export (default <repo>/pak_reference_2).
"""
import argparse
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))
SRC = os.path.join(REF, "DDeception", "Content", "Textures", "06_Hospital")
HEAD = os.path.join(ROOT, "SourceArt", "Wasami", "UI", "pause_head.png")
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "wasami", "stage")

SS = 4          # the head is drawn at this many times the size and shrunk, for smooth edges


# ------------------------------------------------------------------------------------------------ helpers
def _box(a, r, axis):
    """A box blur of radius `r` along one axis, the edge repeated (a running sum, so any radius costs the
    same)."""
    if r < 1:
        return a
    pad = [(0, 0)] * a.ndim
    pad[axis] = (r + 1, r)
    s = np.cumsum(np.pad(a, pad, mode="edge"), axis=axis)
    hi = np.take(s, range(2 * r + 1, s.shape[axis]), axis=axis)
    lo = np.take(s, range(0, s.shape[axis] - 2 * r - 1), axis=axis)
    return (hi - lo) / (2 * r + 1)


def _blur(a, sigma):
    """A float32 array (h, w) or (h, w, n) blurred like a Gaussian (three box blurs; this PC has no scipy and
    PIL cannot blur a float image)."""
    r = int(round((math.sqrt(4.0 * sigma * sigma + 1.0) - 1.0) / 2.0))
    out = a.astype(np.float32)
    for _ in range(3):
        out = _box(_box(out, r, 0), r, 1)
    return out


def fill_hole(rgb, hole, sigma):
    """`rgb` with the pixels of `hole` (a 0..1 mask) replaced by the colours around them.

    Normalised convolution from the kept pixels only, from a wide blur down to a narrow one, so that nothing
    of what is being erased leaks into the fill (the WebGL version blurred the box with sigma = its short
    side x 0.18; the same width, in several passes)."""
    img = rgb.astype(np.float32)
    keep = (1.0 - hole).astype(np.float32)[..., None]
    inside = hole.astype(np.float32)[..., None]
    num, den = _blur(img * keep, sigma), _blur(keep, sigma)
    comp = img * keep + (num / np.maximum(den, 1e-6)) * inside
    for s in (sigma / 2.0, sigma / 4.0):                # the fill follows what is next to it a little more
        comp = img * keep + _blur(comp, s) * inside
    return np.clip(comp, 0, 255)


def add_pattern(filled, rgb, hole, band, sigma):
    """`filled` with the fine pattern of the background put back inside the hole.

    A hole filled with the colours around it is a smooth patch, which shows up against a background of rays
    and halftone dots. `band` is (x0, x1) of a column of the picture that is background from top to bottom;
    it is mirrored side by side to cover the picture, and everything in it finer than `sigma` is added to the
    fill, so the fill keeps the colours around the hole but gets the pattern back."""
    x0, x1 = band
    w = x1 - x0
    t = np.arange(rgb.shape[1]) - x0
    src = rgb[:, x0 + np.abs(((t + w) % (2 * w)) - w)]
    detail = src - _blur(src, sigma)
    return np.clip(filled + detail * hole[..., None], 0, 255)


def poly_mask(size, points, grow=0.0):
    """A 0..1 mask of a polygon, grown by `grow` pixels (a soft edge of one pixel)."""
    w, h = size
    img = Image.new("L", (w * SS, h * SS), 0)
    d = ImageDraw.Draw(img)
    d.polygon([(x * SS, y * SS) for x, y in points], fill=255)
    if grow:
        d.line([(x * SS, y * SS) for x, y in points] + [(points[0][0] * SS, points[0][1] * SS)],
               fill=255, width=int(grow * 2 * SS), joint="curve")
    return np.array(img.resize((w, h), Image.LANCZOS)).astype(np.float32) / 255.0


def head_ink():
    """Wasami's ink (a 0..1 mask cropped to its box) from pause_head.png."""
    a = np.array(Image.open(HEAD).convert("RGBA"))[..., 3]
    ys, xs = np.nonzero(a > 8)
    return a[ys.min():ys.max() + 1, xs.min():xs.max() + 1].astype(np.float32) / 255.0


def draw_head(box, skin, ink, outline, tilt=0.0, neck=None, ink_only=False):
    """Wasami's head drawn as a flat comic head (RGBA, the size of the picture it goes on).

    `box` is (left, top, width, height) of the head in the picture, `tilt` its angle in degrees (the top to
    the left when positive), `neck` (half width, the y it reaches, how much it flares) a neck drawn behind
    the head.
    `ink_only` draws the ink alone in `ink` (no head, no outline), for a picture drawn in silhouettes."""
    left, top, w, h = box
    pad = int(max(w, h) * 0.5)                         # room for the tilt
    cw, ch = (w + 2 * pad) * SS, (h + 2 * pad) * SS
    canvas = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
    d = ImageDraw.Draw(canvas)
    x0, y0 = pad * SS, pad * SS
    lw = max(3, int(round(min(w, h) * 0.022))) * SS     # the pictures' thick outlines
    if neck and not ink_only:
        half, bottom, flare = neck[0] * SS, (neck[1] - top + pad) * SS, neck[2]
        cx, chin = x0 + w * SS / 2, y0 + h * SS
        d.polygon([(cx - half, chin - h * SS * 0.22), (cx + half, chin - h * SS * 0.22),
                   (cx + half * flare, bottom), (cx - half * flare, bottom)], fill=skin, outline=outline,
                  width=lw)
    if not ink_only:
        d.ellipse([x0, y0, x0 + w * SS, y0 + h * SS], fill=skin, outline=outline, width=lw)
    ink_mask = head_ink()
    iw, ih = int(w * SS), int(h * SS)
    m = np.array(Image.fromarray((ink_mask * 255).astype(np.uint8)).resize((iw, ih), Image.LANCZOS))
    patch = np.zeros((ih, iw, 4), np.uint8)
    patch[..., :3] = ink
    patch[..., 3] = m
    canvas.alpha_composite(Image.fromarray(patch), (x0, y0))
    if tilt:
        canvas = canvas.rotate(tilt, Image.BICUBIC, center=(x0 + w * SS / 2, y0 + h * SS / 2))
    return canvas.resize((cw // SS, ch // SS), Image.LANCZOS), (left - pad, top - pad)


def place(base, head, at):
    """`head` composited onto the RGBA picture `base`, and the 0..1 mask of where it landed."""
    over = Image.new("RGBA", base.size, (0, 0, 0, 0))
    over.alpha_composite(head, (int(at[0]), int(at[1])))
    return Image.alpha_composite(base, over), np.array(over)[..., 3].astype(np.float32) / 255.0


# ------------------------------------------------------------------------------------------------ 01
def poster_01(src):
    """"TAKE YOUR MEDICINE!": the paper bag over the nurse's face becomes Wasami's head under the bandana.

    The bandana, the fist, the uniform, the speech bubble and the rays behind stay as they are."""
    rgb = np.array(src.convert("RGB")).astype(np.float32)
    ink = (32, 28, 28)                                 # the picture's outlines
    skin = (225, 167, 156)                             # the arm's skin
    band = (148, 64, 71)                               # the bandana
    # The bag's quad, read off the picture (the black outline is eaten by the growth; the collar below stays).
    bag = [(204, 146), (258, 130), (478, 98), (506, 550), (490, 584), (292, 588), (206, 580)]
    hole = poly_mask(src.size, bag, grow=7)
    filled = add_pattern(fill_hole(rgb, hole, 58.0), rgb, hole, (0, 190), 14.0)
    out = Image.fromarray(filled.astype(np.uint8)).convert("RGBA")
    # Where the bag was, the bandana carries on behind the head (its edge runs low on the left, high on the
    # right, following the two ends that are visible beside the bag).
    over = Image.new("RGBA", src.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(over)
    d.polygon([(150, 70), (540, 62), (508, 160), (214, 330)], fill=band)
    d.line([(214, 330), (508, 160)], fill=ink, width=11)
    a = np.array(over)
    a[..., 3] = (a[..., 3] * hole).astype(np.uint8)
    out = Image.alpha_composite(out, Image.fromarray(a))
    head, at = draw_head((180, 90, 350, 450), skin, ink, ink, tilt=4.0, neck=(104, 604, 1.32))
    out, _ = place(out, head, at)
    return out.convert("RGB")


# ------------------------------------------------------------------------------------------------ 02
RED = (255, 42, 41)                                    # the poster's flat ground
BLACK = (0, 0, 0)                                      # its silhouettes


def poster_02(src):
    """"GET VACCINATED!": the two silhouettes keep their bodies and get Wasami's face.

    The poster is drawn in two flat colours, so each head's box simply goes back to the red and Wasami's ink
    is drawn on it in the silhouettes' black: the face reads as red skin with black hair, eyes and mouth,
    which is the same two colours the rest of the poster uses. The syringe, the heart, the words and the
    bodies below the neck stay as they are."""
    out = src.convert("RGBA")
    # Each skull is cut off along the line where the silhouette turns into the shoulders, and the head is
    # drawn a little below that line, so that its jaw covers the cut. The cut is not level: it follows the
    # slope of the shoulders it leaves behind, so that what is left of the silhouette reads as a back or a
    # collar the head sits on instead of a ruler line across it.
    for box, skull, tilt in (
            # the tall one bent over the small one: its skull, down to where the back branches off (the back
            # falls away to the left, so the cut goes down that way too)
            ((361, 31, 72, 98), [(354, 22), (436, 22), (436, 112), (354, 152)], 3.0),
            # the small one at the left: its skull, down to where the hair widens into the shoulders (the cut
            # dips under the chin, which leaves the hair standing on both sides of it like a collar)
            ((40, 496, 100, 128), [(30, 484), (146, 484), (146, 628), (88, 640), (30, 596)], -4.0)):
        hole = poly_mask(src.size, skull)
        a = np.array(out).astype(np.float32)
        a[..., :3] = a[..., :3] * (1 - hole)[..., None] + np.float32(RED) * hole[..., None]
        out = Image.fromarray(a.astype(np.uint8))
        head, at = draw_head(box, None, BLACK, None, tilt=tilt, ink_only=True)
        out, _ = place(out, head, at)
    return out.convert("RGB")


# ------------------------------------------------------------------------------------------------ decal
def decal_ambulance(src):
    """The doodle on the ambulance's roof: the nurse's cap and the paper bag over her head become Wasami's.

    The doodle is drawn on a fully clear white and every stroke of it is at the same alpha (166/255), so the
    cap and the head are erased back to that clear white and Wasami's head is drawn in the doodle's own
    colours (brown outlines, the arms' skin) and put back at the same alpha. The syringe, the arms, the
    uniform and the ambulance stay as they are."""
    rgba = np.array(src.convert("RGBA")).astype(np.float32)
    ink = (49, 21, 21)                                 # the doodle's brown outlines
    skin = (255, 205, 173)                             # her arms
    opacity = 166.0                                    # every stroke of the doodle
    # The cap (a tall quad up to y 130) and the paper bag below it, stopping short of the collar and of the
    # forearm that crosses under her chin.
    cap = [(489, 129), (577, 129), (577, 238), (572, 252), (569, 282), (548, 292),
           (520, 290), (503, 277), (500, 250), (489, 238)]
    hole = poly_mask(src.size, cap, grow=2)
    rgba[..., :3] = rgba[..., :3] * (1 - hole)[..., None] + 255.0 * hole[..., None]
    rgba[..., 3] = rgba[..., 3] * (1 - hole)
    head, at = draw_head((501, 206, 72, 86), skin, ink, ink, tilt=-7.0)
    over = Image.new("RGBA", src.size, (0, 0, 0, 0))
    over.alpha_composite(head, (int(at[0]), int(at[1])))
    o = np.array(over).astype(np.float32)
    cover = o[..., 3:4] / 255.0                        # the head's own shape, its alpha put back by hand so
    rgba[..., :3] = rgba[..., :3] * (1 - cover) + o[..., :3] * cover      # that it matches the doodle's
    rgba[..., 3] = np.maximum(rgba[..., 3], cover[..., 0] * opacity)
    return Image.fromarray(np.clip(rgba, 0, 255).astype(np.uint8))


POSTERS = {
    "hospital_poster_nurse_01_D": ("wasami_poster_nurse_01.png", poster_01),
    "hospital_poster_nurse_02": ("wasami_poster_nurse_02.png", poster_02),
    "hospital_decal_nurseambulance": ("wasami_decal_nurseambulance.png", decal_ambulance),
}


# ------------------------------------------------------------------------------------------------ main
def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--only", help="one texture's name")
    parser.add_argument("--preview", action="store_true", help="also write the sheet of before and after")
    parser.add_argument("--out", default=OUT, help="where the pictures are written")
    args = parser.parse_args()
    names = [args.only] if args.only else list(POSTERS)
    for name in names:
        if name not in POSTERS:
            sys.exit("unknown texture %s (have %s)" % (name, ", ".join(POSTERS)))
    os.makedirs(args.out, exist_ok=True)
    sheet = []
    for name in names:
        out_name, fn = POSTERS[name]
        path = os.path.join(SRC, name + ".png")
        if not os.path.exists(path):
            sys.exit("missing %s" % path)
        src = Image.open(path)
        new = fn(src)
        dest = os.path.join(args.out, out_name)
        new.save(dest)
        print("wrote %s (%s, %d x %d)" % (dest, new.mode, new.width, new.height))
        if args.preview:
            sheet.append((src, new))
    if sheet:
        cell = 512
        page = Image.new("RGB", (cell * 2, cell * len(sheet)), (24, 24, 24))
        for i, (a, b) in enumerate(sheet):
            page.paste(a.convert("RGB").resize((cell, cell), Image.LANCZOS), (0, i * cell))
            page.paste(b.convert("RGB").resize((cell, cell), Image.LANCZOS), (cell, i * cell))
        dest = os.path.join(args.out, "_nurse_sheet.png")
        page.save(dest)
        print("wrote %s" % dest)


if __name__ == "__main__":
    main()
