"""The level clear screen's title: this game's "Stinky Gachimi" drawn as Dark Deception draws a level's title card.

    python Tools/dd/prepare_level_title.py

The original's level clear screen (UI/Menu/UMG_LevelClear) shows the level's title card above RESULTS (LevelName,
648.72 x 129.6 units, tinted red); for the hospital that is UI/Menu/TitleCards/chapter_ui_title_tormenttherapy, 901 x
180 px of one grey (sRGB 202) with the letters in its alpha, their box 888 x 171 px from (8, 4). This game shows the
WebGL version's title instead (the user's answer of 2026-09-20; SourceArt/Wasami/UI/level_title_stinky_gachimi.png, the
user's red "Stinky Gachimi" on clear, the WebGL version's assets-src/stage/title.png): its alpha, cropped to its
letters (alpha > 8, as the WebGL version cropped it), scaled to fit the original's letters' box with its proportions
kept and centred in it, over the original's grey, at SCALE times the original's size (the widget draws it at the tree's
size whatever the texture's). It writes Intermediate/Pipeline/wasami/ui/level_title.png for dd_ui.import_level_clear
(/Game/Wasami/UI/T_LevelTitle).

Env: PAK_REF2 — the export (default <repo>/pak_reference_2).
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))
CARD = os.path.join(REF, "DDeception", "Content", "UI", "Menu", "TitleCards", "chapter_ui_title_tormenttherapy.png")
TITLE = os.path.join(ROOT, "SourceArt", "Wasami", "UI", "level_title_stinky_gachimi.png")
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "wasami", "ui", "level_title.png")

SCALE = 2
CROP_ALPHA = 8                                    # the WebGL version's crop (13-asset-pipeline: alpha > 8)


def _box(alpha, threshold):
    ys, xs = np.nonzero(alpha > threshold)
    if not len(ys):
        raise ValueError("the image is empty")
    return xs.min(), xs.max() + 1, ys.min(), ys.max() + 1


def compose(card_path=CARD, title_path=TITLE):
    """The title (RGBA) and where the letters went (left, top, width, height)."""
    card = np.array(Image.open(card_path).convert("RGBA"))
    c_left, c_right, c_top, c_bottom = _box(card[..., 3], CROP_ALPHA)
    grey = np.median(card[card[..., 3] > 128][:, :3], axis=0).round().astype(np.uint8)
    title = np.array(Image.open(title_path).convert("RGBA"))[..., 3]
    t_left, t_right, t_top, t_bottom = _box(title, CROP_ALPHA)
    crop = Image.fromarray(title[t_top:t_bottom, t_left:t_right])
    box_w, box_h = (c_right - c_left) * SCALE, (c_bottom - c_top) * SCALE
    fit = min(box_w / crop.width, box_h / crop.height)
    width, height = round(crop.width * fit), round(crop.height * fit)
    x = round(c_left * SCALE + (box_w - width) / 2)
    y = round(c_top * SCALE + (box_h - height) / 2)
    out = np.zeros((card.shape[0] * SCALE, card.shape[1] * SCALE, 4), np.uint8)
    out[..., :3] = grey
    out[y:y + height, x:x + width, 3] = np.array(crop.resize((width, height), Image.LANCZOS))
    return out, (x, y, width, height)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=OUT, help="where the title is written")
    args = parser.parse_args()
    for path in (CARD, TITLE):
        if not os.path.exists(path):
            sys.exit("missing %s" % path)
    title, box = compose()
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    Image.fromarray(title).save(args.out)
    print("wrote %s (%d x %d; the letters at %d, %d, %d x %d)" % ((args.out, title.shape[1], title.shape[0]) + box))


if __name__ == "__main__":
    main()
