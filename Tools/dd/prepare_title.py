"""The title screen's own pictures: Wasami's face as the close-up and the glow around this game's logo.

    python Tools/dd/prepare_title.py

Dark Deception's title (UI/Main/TitleScreen/UMG_TitleScreen, v1.6.1) shows a monster's face (Image_97, 1100 units
square at the middle of the right edge) and its logo (title_screen_logo, the letters wrapped in a soft red-orange glow).
This game shows neither (.claude/guides/original-fidelity.md): the WebGL version put the user's photo of Wasami in the
face's place and this game's logo in the logo's, and styled both with CSS. UMG draws a picture as it is, so the CSS is
baked here:

- The face (SourceArt/Wasami/UI/title_face.png, 512 px): the WebGL version's filter (grayscale(0.75) sepia(0.5)
  hue-rotate(38deg) saturate(0.85) brightness(0.46) contrast(1.45): the Filter Effects spec's matrices and transfers in
  sRGB, each clamped) and its mask (radial-gradient(ellipse 50% 50% at 50% 52%, opaque at 30 %, alpha 0.5 at 58 %,
  clear at 86 %)) as the alpha → Intermediate/Pipeline/wasami/ui/title_face.png.
- The logo's glow: the WebGL version's drop-shadow(0 0 116u rgba(255, 40, 0, 0.28)) with the logo drawn 848 units wide
  (a Gaussian of σ = 58 units over the logo's alpha, × 0.28, in that colour), at a quarter of the logo's resolution with
  GLOW_PAD pixels of room on each side → Intermediate/Pipeline/wasami/ui/title_logo_glow.png. The widget draws it under
  the logo, GLOW_PAD glow pixels (GLOW_SCALE logo pixels each) out from the logo's box on every side.

The logo itself (SourceArt/Wasami/UI/title_logo.png) is imported as it is. dd_ui.import_title imports all three under
/Game/Wasami/UI/Title.
"""
import argparse
import math
import os
import sys

import numpy as np
from PIL import Image, ImageFilter

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FACE = os.path.join(ROOT, "SourceArt", "Wasami", "UI", "title_face.png")
LOGO = os.path.join(ROOT, "SourceArt", "Wasami", "UI", "title_logo.png")
OUT_DIR = os.path.join(ROOT, "Intermediate", "Pipeline", "wasami", "ui")

# The WebGL version's styles.css (.title__monster, .title__logo img).
GRAYSCALE, SEPIA, HUE_ROTATE, SATURATE, BRIGHTNESS, CONTRAST = 0.75, 0.5, 38.0, 0.85, 0.46, 1.45
MASK_CENTRE = (0.5, 0.52)                         # at 50% 52% of the box; the ellipse's radii are half the box
MASK_STOPS = ((0.30, 1.0), (0.58, 0.5), (0.86, 0.0))
LOGO_WIDTH = 848.0                                # units the logo is drawn across
GLOW_BLUR = 116.0                                 # the drop-shadow's blur radius (units) = 2σ
GLOW_COLOUR = (255, 40, 0)
GLOW_ALPHA = 0.28
GLOW_SCALE = 4                                    # logo pixels per glow pixel
GLOW_PAD = 100                                    # glow pixels of room around the logo's box (> 3σ)


def _grayscale(a):
    k = 1 - a
    return np.array([[0.2126 + 0.7874 * k, 0.7152 - 0.7152 * k, 0.0722 - 0.0722 * k],
                     [0.2126 - 0.2126 * k, 0.7152 + 0.2848 * k, 0.0722 - 0.0722 * k],
                     [0.2126 - 0.2126 * k, 0.7152 - 0.7152 * k, 0.0722 + 0.9278 * k]])


def _sepia(a):
    k = 1 - a
    return np.array([[0.393 + 0.607 * k, 0.769 - 0.769 * k, 0.189 - 0.189 * k],
                     [0.349 - 0.349 * k, 0.686 + 0.314 * k, 0.168 - 0.168 * k],
                     [0.272 - 0.272 * k, 0.534 - 0.534 * k, 0.131 + 0.869 * k]])


def _hue_rotate(degrees):
    c, s = math.cos(math.radians(degrees)), math.sin(math.radians(degrees))
    return (np.array([[0.213, 0.715, 0.072]] * 3)
            + c * np.array([[0.787, -0.715, -0.072], [-0.213, 0.285, -0.072], [-0.213, -0.715, 0.928]])
            + s * np.array([[-0.213, -0.715, 0.928], [0.143, 0.140, -0.283], [-0.787, 0.715, 0.072]]))


def _saturate(s):
    return np.array([[0.213 + 0.787 * s, 0.715 - 0.715 * s, 0.072 - 0.072 * s],
                     [0.213 - 0.213 * s, 0.715 + 0.285 * s, 0.072 - 0.072 * s],
                     [0.213 - 0.213 * s, 0.715 - 0.715 * s, 0.072 + 0.928 * s]])


def filtered(rgb):
    """The WebGL filter chain over sRGB values in [0, 1] (H x W x 3)."""
    for matrix in (_grayscale(GRAYSCALE), _sepia(SEPIA), _hue_rotate(HUE_ROTATE), _saturate(SATURATE)):
        rgb = np.clip(rgb @ matrix.T, 0.0, 1.0)
    rgb = np.clip(rgb * BRIGHTNESS, 0.0, 1.0)
    return np.clip(rgb * CONTRAST + (0.5 - 0.5 * CONTRAST), 0.0, 1.0)


def mask(width, height):
    """The radial gradient's alpha over a box of width x height pixels."""
    ys, xs = np.mgrid[0:height, 0:width]
    dx = (xs + 0.5 - MASK_CENTRE[0] * width) / (0.5 * width)
    dy = (ys + 0.5 - MASK_CENTRE[1] * height) / (0.5 * height)
    r = np.hypot(dx, dy)
    return np.interp(r, [s for s, _ in MASK_STOPS], [a for _, a in MASK_STOPS])


def face(path=FACE):
    """The face (RGBA, the source's size) with the filter baked in and the mask as its alpha."""
    rgb = np.array(Image.open(path).convert("RGB"), dtype=np.float64) / 255.0
    out = np.empty(rgb.shape[:2] + (4,), np.float64)
    out[..., :3] = filtered(rgb)
    out[..., 3] = mask(rgb.shape[1], rgb.shape[0])
    return (out * 255.0 + 0.5).astype(np.uint8)


def glow(path=LOGO):
    """The logo's glow (RGBA) and its σ in glow pixels."""
    alpha = Image.open(path).convert("RGBA").getchannel("A")
    small = alpha.resize((round(alpha.width / GLOW_SCALE), round(alpha.height / GLOW_SCALE)), Image.LANCZOS)
    canvas = Image.new("L", (small.width + 2 * GLOW_PAD, small.height + 2 * GLOW_PAD), 0)
    canvas.paste(small, (GLOW_PAD, GLOW_PAD))
    sigma = GLOW_BLUR / 2 / (LOGO_WIDTH / small.width)
    if 3 * sigma > GLOW_PAD:
        raise ValueError("GLOW_PAD %d is under 3σ (%.1f)" % (GLOW_PAD, 3 * sigma))
    blurred = np.array(canvas.filter(ImageFilter.GaussianBlur(sigma)), dtype=np.float64) / 255.0
    out = np.zeros(blurred.shape + (4,), np.uint8)
    out[..., :3] = GLOW_COLOUR
    out[..., 3] = (blurred * GLOW_ALPHA * 255.0 + 0.5).astype(np.uint8)
    return out, sigma


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=OUT_DIR, help="where the pictures are written")
    args = parser.parse_args()
    for path in (FACE, LOGO):
        if not os.path.exists(path):
            sys.exit("missing %s" % path)
    os.makedirs(args.out, exist_ok=True)
    face_out = os.path.join(args.out, "title_face.png")
    Image.fromarray(face()).save(face_out)
    print("wrote %s" % face_out)
    glow_image, sigma = glow()
    glow_out = os.path.join(args.out, "title_logo_glow.png")
    Image.fromarray(glow_image).save(glow_out)
    print("wrote %s (%d x %d, sigma %.1f px, %d px of room)" % (glow_out, glow_image.shape[1], glow_image.shape[0],
                                                           sigma, GLOW_PAD))


if __name__ == "__main__":
    main()
