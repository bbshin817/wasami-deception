"""The game's Windows icon: Wasami's head the way the original's Steam icon shows its monkey.

    python Tools/wasami_art/game_icon.py [--preview]
    python Tools/wasami_art/game_icon.py --from-photo <photo> [--preview]   # remake the head from a photo first

The original (Steam app 332950's icon, 32x32: Steam/appcache/librarycache/332950/*.jpg and Steam/steam/games/*.ico)
is a dark, creepy toy monkey's head cut out on transparency with a thin red outline hugging its silhouette. This takes
the head (SourceArt/Wasami/Icon/game_icon_head.png), and for every
size Windows asks for draws it again with its own red outline (dilated silhouette, so the rim stays one or two pixels
wide at 16 px as it does in the original), then writes SourceArt/Wasami/Icon/Application.ico and copies it to
Build/Windows/Application.ico, which UnrealBuildTool embeds in the game's executables. --preview writes
Intermediate/WasamiArt/game_icon_preview.png (every size, on dark and light).

The head is the user's own photo, not a generated drawing (2026-09-26: a generated face "looks nothing like the face I
gave"): --from-photo cuts the head out of it (rembg's isnet-general-use), drops the neck and shirt under the jaw with
an ellipse, and grades it dark and sickly the way the original's monkey is lit (darker, harder, less saturated, the
rim of the head sunk into shadow), then writes game_icon_head.png. The photo itself is not kept in the repository.
"""
import argparse
import os
import shutil

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
HEAD = os.path.join(ROOT, "SourceArt", "Wasami", "Icon", "game_icon_head.png")
ICO = os.path.join(ROOT, "SourceArt", "Wasami", "Icon", "Application.ico")
BUILD_ICO = os.path.join(ROOT, "Build", "Windows", "Application.ico")
PREVIEW = os.path.join(ROOT, "Intermediate", "WasamiArt", "game_icon_preview.png")

SIZES = (16, 24, 32, 40, 48, 64, 256)          # the original executable's icon sizes
RED = (230, 0, 0)                               # the original's rim


# The head's ellipse on the photo, as fractions of its width and height (the 2026-09-26 photo: 397 x 397, the jaw at
# about 0.93 of the height) — only what is inside both it and the cut-out is kept.
HEAD_ELLIPSE = (0.12, -0.05, 0.88, 0.95)
GRADE = {"brightness": 0.78, "contrast": 1.3, "colour": 0.8, "rim_dark": 0.55}


def head_from_photo(photo):
    from rembg import new_session, remove   # pip install "rembg[cpu]"; the model downloads on first use
    src = Image.open(photo).convert("RGB")
    cut = remove(src, session=new_session("isnet-general-use"), post_process_mask=True)
    w, h = src.size
    ellipse = Image.new("L", src.size, 0)
    l, t, r, b = HEAD_ELLIPSE
    ImageDraw.Draw(ellipse).ellipse((l * w, t * h, r * w, b * h), fill=255)
    ellipse = ellipse.filter(ImageFilter.GaussianBlur(w / 200))
    alpha = Image.fromarray(__import__("numpy").minimum(__import__("numpy").asarray(cut.getchannel("A")),
                                                         __import__("numpy").asarray(ellipse)))
    rgb = src
    rgb = ImageEnhance.Brightness(rgb).enhance(GRADE["brightness"])
    rgb = ImageEnhance.Contrast(rgb).enhance(GRADE["contrast"])
    rgb = ImageEnhance.Color(rgb).enhance(GRADE["colour"])
    # the rim of the head sinks into shadow: darken by the distance from the ellipse's centre
    shade = Image.new("L", src.size, 0)
    d = ImageDraw.Draw(shade)
    cx, cy, rx, ry = (l + r) / 2 * w, (t + b) / 2 * h, (r - l) / 2 * w, (b - t) / 2 * h
    for k in range(40, 0, -1):
        f = k / 40
        d.ellipse((cx - rx * f, cy - ry * f, cx + rx * f, cy + ry * f), fill=int(255 * (1 - GRADE["rim_dark"] * f ** 3)))
    rgb = Image.composite(rgb, Image.new("RGB", src.size, (0, 0, 0)), shade.filter(ImageFilter.GaussianBlur(w / 30)))
    head = rgb.convert("RGBA")
    head.putalpha(alpha)
    head.save(HEAD)
    return head


def head_square():
    im = Image.open(HEAD).convert("RGBA")
    im = im.crop(im.getchannel("A").point(lambda a: 255 if a > 16 else 0).getbbox())
    side = max(im.size)
    sq = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    sq.alpha_composite(im, ((side - im.width) // 2, (side - im.height) // 2))
    return sq


def icon(head, size):
    """The head at one size with its red rim: drawn at 4x and shrunk, the rim a dilation of the silhouette."""
    s = 4 * size
    rim = min(max(1.0, size / 32 * 1.25), 4.0) * 4   # 1.25 px at 32, like the original; 1 to 4 px
    inner = round(s - 2 * rim - 2)
    face = head.resize((inner, inner), Image.LANCZOS)
    canvas = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    canvas.alpha_composite(face, ((s - inner) // 2, (s - inner) // 2))
    alpha = canvas.getchannel("A").point(lambda a: 255 if a > 96 else 0)
    k = int(rim) * 2 + 1
    grown = alpha.filter(ImageFilter.MaxFilter(k if k % 2 else k + 1))
    out = Image.new("RGBA", (s, s), RED + (0,))
    out.putalpha(grown)
    out.alpha_composite(canvas)
    return out.resize((size, size), Image.LANCZOS)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--preview", action="store_true")
    ap.add_argument("--from-photo", help="remake the head from this photo first")
    args = ap.parse_args()
    if args.from_photo:
        head_from_photo(args.from_photo)
    head = head_square()
    images = [icon(head, n) for n in SIZES]
    if args.preview:
        w = sum(max(n, 64) + 16 for n in SIZES)
        sheet = Image.new("RGBA", (w, 2 * 280), (0, 0, 0, 0))
        for row, bg in enumerate(((40, 40, 40, 255), (235, 235, 235, 255))):
            sheet.alpha_composite(Image.new("RGBA", (w, 280), bg), (0, row * 280))
            x = 8
            for n, im in zip(SIZES, images):
                sheet.alpha_composite(im, (x, row * 280 + 10))
                x += max(n, 64) + 16
        os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
        sheet.save(PREVIEW)
        print(PREVIEW)
        return 0
    os.makedirs(os.path.dirname(ICO), exist_ok=True)
    images[-1].save(ICO, format="ICO", sizes=[(n, n) for n in SIZES], append_images=images[:-1])
    os.makedirs(os.path.dirname(BUILD_ICO), exist_ok=True)
    shutil.copy2(ICO, BUILD_ICO)
    print("%s (%s) -> %s" % (os.path.relpath(ICO, ROOT), ", ".join(map(str, SIZES)), os.path.relpath(BUILD_ICO, ROOT)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
