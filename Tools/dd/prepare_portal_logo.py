"""The garage portal's logo: this game's Wasami symbol drawn as Dark Deception draws a character's mark on a portal.

    python Tools/dd/prepare_portal_logo.py

The original's portals (Blueprints/00_Ballroom/BP_00_Teleport) show a character's face on the plane Logo, a texture of
1024 x 1024 that is all the marks' red (192, 0, 0) with the face in its alpha (Textures/00_Ballroom/portal_monkey and
the others). This game shows no character of the original (.claude/guides/original-fidelity.md), so the garage's portal
shows the user's symbol (SourceArt/Wasami/UI/wasami_symbol.png) instead, drawn the same way: its alpha scaled to the
monkey's height and centred where the monkey's box is, over the same red. It writes
Intermediate/Pipeline/wasami/fx/portal_wasami.png for dd_gimmicks.import_portal (/Game/Wasami/Portal/T_Portal_Wasami).

Env: PAK_REF2 — the export (default <repo>/pak_reference_2).
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
REF = os.environ.get("PAK_REF2", os.path.join(ROOT, "pak_reference_2"))
MONKEY = os.path.join(REF, "DDeception", "Content", "Textures", "00_Ballroom", "portal_monkey.png")
SYMBOL = os.path.join(ROOT, "SourceArt", "Wasami", "UI", "wasami_symbol.png")
OUT = os.path.join(ROOT, "Intermediate", "Pipeline", "wasami", "fx", "portal_wasami.png")

SIZE = 1024
RED = (192, 0, 0)


def _box(alpha):
    ys, xs = np.nonzero(alpha)
    if not len(ys):
        raise ValueError("the image is empty")
    return xs.min(), xs.max() + 1, ys.min(), ys.max() + 1


def compose(monkey_path=MONKEY, symbol_path=SYMBOL):
    """The logo (RGBA, SIZE x SIZE) and where the symbol went (left, top, width, height)."""
    monkey = np.array(Image.open(monkey_path).convert("RGBA"))
    if monkey.shape[:2] != (SIZE, SIZE):
        raise ValueError("%s is %s, not %d x %d" % (monkey_path, monkey.shape[:2], SIZE, SIZE))
    left, right, top, bottom = _box(monkey[..., 3])
    centre = ((left + right) / 2, (top + bottom) / 2)
    alpha = np.array(Image.open(symbol_path).convert("RGBA"))[..., 3]
    s_left, s_right, s_top, s_bottom = _box(alpha)
    crop = Image.fromarray(alpha[s_top:s_bottom, s_left:s_right])
    scale = (bottom - top) / crop.height
    width, height = round(crop.width * scale), round(crop.height * scale)
    x, y = round(centre[0] - width / 2), round(centre[1] - height / 2)
    logo = np.zeros((SIZE, SIZE, 4), np.uint8)
    logo[..., :3] = RED
    logo[y:y + height, x:x + width, 3] = np.array(crop.resize((width, height), Image.LANCZOS))
    return logo, (x, y, width, height)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=OUT, help="where the logo is written")
    args = parser.parse_args()
    for path in (MONKEY, SYMBOL):
        if not os.path.exists(path):
            sys.exit("missing %s" % path)
    logo, box = compose()
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    Image.fromarray(logo).save(args.out)
    print("wrote %s (the symbol at %d, %d, %d x %d)" % ((args.out,) + box))


if __name__ == "__main__":
    main()
