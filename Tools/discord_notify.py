"""Posts to the Discord webhook of the unattended night (.claude/guides/autonomy.md): text, and images of this game.

    python Tools/discord_notify.py check                          is a URL set, and where from (never prints it)
    python Tools/discord_notify.py text "message"
    python Tools/discord_notify.py image <file>... [--caption "text"]

The URL is read from the environment variable WASAMI_DISCORD_WEBHOOK, else from "discord_webhook" in
Tools/overnight.local.json (ignored by git: whoever knows the URL can post). Tools/overnight.py posts the start, a
report per run (with the images of this game attached as one grid), the waits and the summary. Posts go out under the
name "Claude". A post that fails is reported in one line and never raises.
Exit codes: 0 posted, 1 not posted, 2 no URL.
"""
import argparse
import io
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
import uuid

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
LOCAL_CONFIG = os.path.join(ROOT, "Tools", "overnight.local.json")
ENV = "WASAMI_DISCORD_WEBHOOK"
NAME = "Claude"
# Discord refuses the default "Python-urllib" agent (Cloudflare error 1010).
AGENT = "DiscordBot (wasami_deception Tools/discord_notify.py, 1.0)"
# A message holds at most 2000 characters; keep clear of it (and of emoji counted twice).
TEXT_LIMIT = 1900
# Room for the ``` lines added when a piece is cut inside a code block.
FENCE_MARGIN = 32
# Attachments per message: a server without boosts takes 10 MB and 10 files.
BYTES_LIMIT = 8 * 1024 * 1024
FILES_LIMIT = 10
IMAGE_TYPES = {".png": "image/png", ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".gif": "image/gif",
               ".webp": "image/webp"}


def webhook_url():
    """The webhook URL and where it came from (never put the URL itself in a log line): WASAMI_DISCORD_WEBHOOK, else
    "discord_webhook" of Tools/overnight.local.json. (None, None) when neither has one."""
    url = os.environ.get(ENV, "").strip()
    if url:
        return url, "環境変数 " + ENV
    try:
        with open(LOCAL_CONFIG, encoding="utf-8") as f:
            url = str(json.load(f).get("discord_webhook") or "").strip()
    except (OSError, ValueError, AttributeError):
        url = ""
    if url:
        return url, os.path.relpath(LOCAL_CONFIG, ROOT)
    return None, None


def split_message(text, limit=TEXT_LIMIT):
    """Cuts text into pieces of at most `limit` characters, at line ends where it can (a longer line is cut where it
    must). A piece that ends inside a ``` block closes it and the next piece opens it again."""
    width = limit - FENCE_MARGIN
    parts = []
    for line in text.split("\n"):
        while len(line) > width:
            parts.append(line[:width])
            line = line[width:]
        parts.append(line)
    pieces = []
    lines, used, carried, fence = [], 0, 0, None
    for part in parts:
        if len(lines) > carried and used + len(part) + 1 > width:
            pieces.append("\n".join(lines) + ("\n```" if fence else ""))
            lines, used, carried = ([fence], len(fence) + 1, 1) if fence else ([], 0, 0)
        lines.append(part)
        used += len(part) + 1
        if part.lstrip().startswith("```"):
            fence = None if fence else part.strip()[:20]
    pieces.append("\n".join(lines))
    return [p for p in pieces if p.strip()]


def image_file(path, limit=BYTES_LIMIT):
    """(file name, bytes, media type) of an image, ready to attach. A file over `limit` or of a type Discord does not
    show is written again as JPEG (quality 90), halved in area until it fits."""
    base, ext = os.path.splitext(os.path.basename(path))
    safe = re.sub(r"[^A-Za-z0-9._-]", "_", base) or "image"
    kind = IMAGE_TYPES.get(ext.lower())
    if kind and os.path.getsize(path) <= limit:
        with open(path, "rb") as f:
            return safe + ext.lower(), f.read(), kind
    from PIL import Image
    with Image.open(path) as source:
        image = source.convert("RGB")
    while True:
        out = io.BytesIO()
        image.save(out, "JPEG", quality=90)
        if out.tell() <= limit or min(image.size) < 64:
            return safe + ".jpg", out.getvalue(), "image/jpeg"
        image = image.resize((int(image.width * 0.7071), int(image.height * 0.7071)), Image.LANCZOS)


def multipart(payload, files):
    """The body and Content-Type of a webhook post with attachments (payload_json and files[n])."""
    boundary = uuid.uuid4().hex
    out = io.BytesIO()

    def part(headers, data):
        out.write(("--%s\r\n%s\r\n\r\n" % (boundary, headers)).encode("utf-8"))
        out.write(data)
        out.write(b"\r\n")

    part('Content-Disposition: form-data; name="payload_json"\r\nContent-Type: application/json',
         json.dumps(payload, ensure_ascii=False).encode("utf-8"))
    for index, (name, data, kind) in enumerate(files):
        part('Content-Disposition: form-data; name="files[%d]"; filename="%s"\r\nContent-Type: %s'
             % (index, name, kind), data)
    out.write(("--%s--\r\n" % boundary).encode("utf-8"))
    return out.getvalue(), "multipart/form-data; boundary=" + boundary


def retry_after(error, body):
    """Seconds Discord asks us to wait (the JSON body of a 429, else the Retry-After header, else 5)."""
    try:
        return float(json.loads(body)["retry_after"])
    except (ValueError, KeyError, TypeError):
        pass
    try:
        return float(error.headers.get("Retry-After"))
    except (AttributeError, TypeError, ValueError):
        return 5.0


class Webhook:
    """Posts to the webhook. Without a URL it does nothing. `say` gets one line per failure; nothing raises."""

    def __init__(self, url, say):
        self.url = url
        self.say = say

    def payload(self, content, files=()):
        data = {"content": content, "username": NAME, "allowed_mentions": {"parse": []}}
        if files:
            data["attachments"] = [{"id": i, "filename": name} for i, (name, _, _) in enumerate(files)]
        return data

    def post(self, text):
        """Posts text, cut into as many messages as it needs. True when every piece went out."""
        if not self.url:
            return False
        for piece in split_message(text.strip() or "（空）"):
            body = json.dumps(self.payload(piece), ensure_ascii=False).encode("utf-8")
            if not self.send(body, "application/json"):
                return False
            time.sleep(1)  # a webhook takes about 5 posts per 2 seconds
        return True

    def post_images(self, paths, caption=""):
        """Posts images, at most 10 files and 8 MB a message, the caption with the first. True when all went out."""
        if not self.url:
            return False
        files = []
        for path in paths:
            try:
                files.append(image_file(path))
            except Exception as e:  # an unreadable or broken image is skipped, the rest still go
                self.say("Discord に送る画像を読めない: %s (%s)" % (os.path.basename(path), e))
        pieces = split_message(caption) if caption.strip() else [""]
        if len(pieces) > 1 and not self.post("\n".join(pieces[:-1])):
            return False
        content = pieces[-1]
        if not files:  # nothing readable: the caption alone, and False when there were paths
            posted = self.post(content) if content else True
            return posted and not paths
        batches, batch, size = [], [], 0
        for item in files:
            if batch and (len(batch) >= FILES_LIMIT or size + len(item[1]) > BYTES_LIMIT):
                batches.append(batch)
                batch, size = [], 0
            batch.append(item)
            size += len(item[1])
        batches.append(batch)
        for batch in batches:
            body, kind = multipart(self.payload(content, batch), batch)
            if not self.send(body, kind):
                return False
            content = ""
            time.sleep(1)
        return len(files) == len(paths)

    def send(self, body, content_type):
        for attempt in range(4):
            try:
                request = urllib.request.Request(self.url, data=body, method="POST", headers={
                    "Content-Type": content_type, "User-Agent": AGENT})
                with urllib.request.urlopen(request, timeout=60):
                    return True
            except urllib.error.HTTPError as e:
                detail = e.read().decode("utf-8", errors="replace")
                if (e.code == 429 or e.code >= 500) and attempt < 3:
                    time.sleep(min(retry_after(e, detail), 60))
                    continue
                self.say("Discord に送れない: HTTP %d %s" % (e.code, detail.strip()[:200]))
                return False
            except ValueError:
                # The message of urllib would show the URL, which holds the token.
                self.say("Discord に送れない: webhook の URL の形が正しくない")
                return False
            except (urllib.error.URLError, OSError) as e:
                if attempt < 3:
                    time.sleep(5)
                    continue
                self.say("Discord に送れない: %s" % getattr(e, "reason", e))
                return False
        return False


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("cmd", choices=("check", "text", "image"))
    ap.add_argument("args", nargs="*", help="the message for text, the files for image")
    ap.add_argument("--caption", default="", help="image: the text that goes with the images")
    opts = ap.parse_args()
    url, source = webhook_url()
    if not url:
        print("webhook の URL が無い（環境変数 %s も %s も無い）" % (ENV, os.path.relpath(LOCAL_CONFIG, ROOT)),
              file=sys.stderr)
        return 2
    if opts.cmd == "check":
        print("webhook: %s" % source)
        return 0
    if not opts.args:
        ap.error("%s needs %s" % (opts.cmd, "a message" if opts.cmd == "text" else "one or more files"))
    webhook = Webhook(url, lambda line: print(line, file=sys.stderr))
    if opts.cmd == "text":
        ok = webhook.post(" ".join(opts.args))
    else:
        ok = webhook.post_images(opts.args, opts.caption)
    print("送った" if ok else "送れなかった")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
