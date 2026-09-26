"""The hospital's ambulance screen video, Wasami's version, built from flat vector parts.

    python Tools/wasami_art/ambulance_video.py [--preview] [--seconds 8]

The original (Steam: DDeception/Content/Movies/ambulance_tutorial2.mp4, 1920x1280, 29.97 fps, 3.9 s) is a white card
with grey corner brackets, "NEED A RIDE?" above and a red "HOP ON TOP!" below a flat vector ambulance that bobs while
its roof beacon flashes. This one keeps that card (same layout, colours and words) and animates more on it: the
ambulance drives over a scrolling road with spinning wheels, speed lines and exhaust puffs, hits a pothole once per
loop, and Wasami rides on the roof — waving, then clinging on after the bump, then cheering — while the beacon
flashes and the bottom line bounces letter by letter.

Parts (tools/wasami_art gen/adopt, briefs/hospital_ambulance_video.json): SourceArt/Wasami/Movies/parts/amb_body.png
(the ambulance without wheels or beacon) and amb_wasami_{wave,hold,cheer}.png. Wheels, beacon, road, text and effects
are drawn here. Every motion is periodic in the loop, so the video repeats without a seam. Writes
SourceArt/Wasami/Movies/ambulance_tutorial2.mp4 (H.264, yuv420p, no audio) — or with --preview a contact sheet at
Intermediate/WasamiArt/ambulance_preview.png.
"""
import argparse
import math
import os
import subprocess

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PARTS = os.path.join(ROOT, "SourceArt", "Wasami", "Movies", "parts")
OUT = os.path.join(ROOT, "SourceArt", "Wasami", "Movies", "ambulance_tutorial2.mp4")
PREVIEW = os.path.join(ROOT, "Intermediate", "WasamiArt", "ambulance_preview.png")

W, H = 1920, 1280                       # the original's size and rate
FPS_NUM, FPS_DEN = 30000, 1001
WHITE, INK, RED, GREY = (255, 255, 255), (34, 34, 34), (232, 0, 1), (163, 163, 163)   # measured on the original
SLATE, ROAD = (84, 98, 117), (205, 208, 214)
FONT = "C:/Windows/Fonts/seguibl.ttf"

# The ambulance part: its size in the source PNG, the wheel-arch centres measured on it, and where it sits.
BODY_SRC_W = 1536
ARCHES_SRC = (381, 1287)
ARCH_Y_SRC = 745
WHEEL_R_SRC = 92
BODY_SCALE = 0.72
BODY_X, BODY_Y = 402, 382               # top-left of the scaled part on the card at rest (wheels on y 985)
ROOF_Y_SRC = 226                        # the box's roof line on the part
BEACON_X_SRC = 1050                     # the cab roof, where the beacon sits

GROUND_Y = BODY_Y + round((ARCH_Y_SRC + WHEEL_R_SRC) * BODY_SCALE)

# Wasami's poses: target height on the card, and x of his centre on the roof (in part pixels).
POSES = {"wave": (250, 560), "hold": (165, 600), "cheer": (290, 560)}


def load_part(name, height=None):
    im = Image.open(os.path.join(PARTS, name)).convert("RGBA")
    im = im.crop(im.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox())
    if height:
        im = im.resize((round(im.width * height / im.height), height), Image.LANCZOS)
    return im


def wheel_sprite(r):
    """A flat vector wheel like the original's (dark tyre, grey rim, five spokes), drawn 4x and shrunk."""
    s = 4
    im = Image.new("RGBA", (2 * r * s, 2 * r * s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    c = r * s
    d.ellipse((0, 0, 2 * c - 1, 2 * c - 1), fill=(38, 40, 46))
    rr = int(c * 0.62)
    d.ellipse((c - rr, c - rr, c + rr, c + rr), fill=(236, 238, 241))
    ri = int(c * 0.5)
    d.ellipse((c - ri, c - ri, c + ri, c + ri), fill=(120, 128, 140))
    for k in range(5):
        a = 2 * math.pi * k / 5
        d.line((c, c, c + math.cos(a) * ri, c + math.sin(a) * ri), fill=(236, 238, 241), width=int(c * 0.12))
    hub = int(c * 0.16)
    d.ellipse((c - hub, c - hub, c + hub, c + hub), fill=(38, 40, 46))
    return im.resize((2 * r, 2 * r), Image.LANCZOS)


def ease(x):
    return 0.5 - 0.5 * math.cos(math.pi * max(0.0, min(1.0, x)))


class Video:
    def __init__(self, frames):
        self.n = frames
        self.body = Image.open(os.path.join(PARTS, "amb_body.png")).convert("RGBA")   # uncropped: the arches are in its pixels
        self.body_scaled = self.body.resize((round(self.body.width * BODY_SCALE), round(self.body.height * BODY_SCALE)),
                                            Image.LANCZOS)
        self.wheel = wheel_sprite(round(WHEEL_R_SRC * BODY_SCALE))
        self.poses = {k: load_part(f"amb_wasami_{k}.png", h) for k, (h, _x) in POSES.items()}
        self.font_top = ImageFont.truetype(FONT, 118)
        self.font_bottom = ImageFont.truetype(FONT, 128)
        self.card = self.make_card()
        self.cycles = 10                      # road-dash and wheel cycles per loop (whole numbers keep the seam)

    def make_card(self):
        im = Image.new("RGBA", (W, H), WHITE + (255,))
        d = ImageDraw.Draw(im)
        t = 7                                 # the corner brackets, where the original has them
        for x0, dx in ((136, 1), (1780 - t, -1)):
            d.rectangle((x0, 172, x0 + t, 1110), fill=GREY)
            for y in (172, 1110 - t):
                x1 = x0 + dx * 150
                d.rectangle((min(x0, x1), y, max(x0, x1) + t, y + t), fill=GREY)
        return im

    # ---------------------------------------------------------------------------------------------- the timeline
    def bump(self, p):
        """Vertical offset of the body: small road bobs, plus one pothole hop at p = 0.36."""
        bob = 6 * abs(math.sin(2 * math.pi * p * 16))
        hop = 0.0
        q = (p - 0.36) / 0.07
        if 0 <= q <= 1:
            hop = 70 * math.sin(math.pi * q) * (1 - 0.3 * q)
        return -(bob + hop)

    def tilt(self, p):
        q = (p - 0.36) / 0.09
        return 3.0 * math.sin(2 * math.pi * q) * (1 - q) if 0 <= q <= 1 else 0.6 * math.sin(2 * math.pi * p * 8)

    def pose(self, p):
        """(pose, pop) — wave until the pothole, clinging after it, cheering from 0.66; pop scales the switch."""
        for name, start in (("cheer", 0.66), ("hold", 0.37), ("wave", 0.0)):
            if p >= start:
                dt = p - start
                pop = 1 + 0.12 * math.sin(math.pi * min(1.0, dt / 0.025)) if dt < 0.025 else 1.0
                return name, pop
        return "wave", 1.0

    # ---------------------------------------------------------------------------------------------- one frame
    def frame(self, i):
        p = i / self.n
        im = self.card.copy()
        d = ImageDraw.Draw(im, "RGBA")
        scroll = p * self.cycles

        # road: a grey band with dashes running left, speed lines behind the ambulance
        d.rectangle((150, GROUND_Y, 1770, GROUND_Y + 10), fill=ROAD)
        period = 1620 / 5
        for k in range(-1, 7):
            x = 150 + ((k - scroll * 1.0) % 6) * period - period
            x0, x1 = max(150, x), min(1770, x + period * 0.45)
            if x1 > x0:
                d.rectangle((x0, GROUND_Y + 26, x1, GROUND_Y + 36), fill=ROAD)
        for k, (y, length) in enumerate(((430, 180), (540, 120), (650, 220), (760, 150))):
            ph = (p * self.cycles * 2 + k * 0.37) % 1.0
            x1 = BODY_X - 30 - ph * 120
            a = int(255 * math.sin(math.pi * ph))
            d.rounded_rectangle((x1 - length, y, x1, y + 12), radius=6, fill=GREY + (a,))

        dy = self.bump(p)
        ang = self.tilt(p)

        # exhaust puffs from the back, drifting left and fading
        for k in range(3):
            ph = (p * 6 + k / 3) % 1.0
            r = 18 + 40 * ph
            cx, cy = BODY_X + 20 - 170 * ph, GROUND_Y - 60 + dy * 0.5 - 40 * ph
            d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(190, 195, 202, int(200 * (1 - ph))))

        # the ambulance layer: wheels behind the body, the beacon and Wasami on top; bobbed and tilted together
        layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        body = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        wr = self.wheel.width // 2
        spin = -360 * scroll * 1.6
        for ax in ARCHES_SRC:
            wx = BODY_X + round(ax * BODY_SCALE) - wr
            wy = GROUND_Y - 2 * wr
            body.alpha_composite(self.wheel.rotate(spin, resample=Image.BICUBIC), (wx, wy))
        body.alpha_composite(self.body_scaled, (BODY_X, BODY_Y))
        self.draw_beacon(body, p)
        self.draw_wasami(body, p)
        cx, cy = BODY_X + self.body_scaled.width / 2, GROUND_Y
        body = body.rotate(ang, resample=Image.BICUBIC, center=(cx, cy), translate=(0, dy))
        layer.alpha_composite(body)
        im.alpha_composite(layer)

        self.draw_text(im, p)
        return im.convert("RGB")

    def draw_beacon(self, layer, p):
        d = ImageDraw.Draw(layer, "RGBA")
        bx = BODY_X + BEACON_X_SRC * BODY_SCALE
        by = BODY_Y + ROOF_Y_SRC * BODY_SCALE + 140 * BODY_SCALE   # the cab roof sits lower than the box
        on = int(p * self.n / 4) % 2 == 0                              # ~7.5 Hz, like the original's blink
        d.rectangle((bx - 34, by - 10, bx + 34, by + 2), fill=(236, 238, 241))
        d.rounded_rectangle((bx - 24, by - 44, bx + 24, by - 8), radius=16, fill=RED if on else (150, 20, 30))
        if on:
            glow = Image.new("RGBA", layer.size, (0, 0, 0, 0))
            gd = ImageDraw.Draw(glow)
            gd.ellipse((bx - 70, by - 95, bx + 70, by + 30), fill=(255, 60, 60, 110))
            layer.alpha_composite(glow.filter(ImageFilter.GaussianBlur(18)))
            for k in range(5):
                a = math.radians(-150 + k * 30)
                r0, r1 = 62, 100
                d.line((bx + math.cos(a) * r0, by - 26 + math.sin(a) * r0,
                        bx + math.cos(a) * r1, by - 26 + math.sin(a) * r1), fill=INK, width=7)

    def draw_wasami(self, layer, p):
        name, pop = self.pose(p)
        sprite = self.poses[name]
        h, x_src = POSES[name]
        wob = {"wave": 2.5 * math.sin(2 * math.pi * p * 12), "hold": 1.5 * math.sin(2 * math.pi * p * 40),
               "cheer": 4 * math.sin(2 * math.pi * p * 10)}[name]
        lift = {"wave": 0, "hold": 0, "cheer": 10 * abs(math.sin(2 * math.pi * p * 10))}[name]
        if pop != 1.0:
            sprite = sprite.resize((round(sprite.width * pop), round(sprite.height * pop)), Image.LANCZOS)
        sprite = sprite.rotate(wob, resample=Image.BICUBIC, expand=True)
        roof = BODY_Y + ROOF_Y_SRC * BODY_SCALE
        x = BODY_X + x_src * BODY_SCALE - sprite.width / 2
        y = roof - sprite.height + 18 - lift
        layer.alpha_composite(sprite, (round(x), round(y)))

    def draw_text(self, im, p):
        d = ImageDraw.Draw(im)
        top = "NEED A RIDE?"
        w = d.textlength(top, font=self.font_top)
        d.text(((W - w) / 2, 102), top, font=self.font_top, fill=INK)
        bottom = "HOP ON TOP!"
        widths = [d.textlength(c, font=self.font_bottom) for c in bottom]
        x = (W - sum(widths)) / 2
        beat = (p * 8) % 1.0                                              # a wave of hopping letters, 8 per loop
        for k, (c, cw) in enumerate(zip(bottom, widths)):
            q = beat * 1.6 - k * 0.06
            hop = 26 * math.sin(math.pi * q) if 0 <= q <= 1 else 0
            d.text((x, 1020 - hop), c, font=self.font_bottom, fill=RED)
            x += cw


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--seconds", type=float, default=8.0)
    ap.add_argument("--preview", action="store_true")
    args = ap.parse_args()
    n = round(args.seconds * FPS_NUM / FPS_DEN)
    v = Video(n)
    if args.preview:
        picks = [round(n * k / 12) for k in range(12)]
        sheet = Image.new("RGB", (4 * 480, 3 * 320), WHITE)
        for j, i in enumerate(picks):
            sheet.paste(v.frame(i).resize((480, 320), Image.LANCZOS), ((j % 4) * 480, (j // 4) * 320))
        os.makedirs(os.path.dirname(PREVIEW), exist_ok=True)
        sheet.save(PREVIEW)
        print(PREVIEW)
        return 0
    cmd = ["ffmpeg", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
           "-r", f"{FPS_NUM}/{FPS_DEN}", "-i", "-", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "18",
           "-movflags", "+faststart", OUT]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    for i in range(n):
        proc.stdin.write(v.frame(i).tobytes())
    proc.stdin.close()
    if proc.wait():
        raise SystemExit("ffmpeg failed")
    print(f"{os.path.relpath(OUT, ROOT)}: {n} frames, {n * FPS_DEN / FPS_NUM:.3f} s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
