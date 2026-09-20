"""Wasami's own voice clips, from the WebGL version's mp3 to the wav this project keeps and imports.

    python Tools/dd/prepare_voices.py

The WebGL version ships fifteen clips of the user's Wasami (<WEBGL>/public/voices/*.mp3 with manifest.json: each
clip's id, the scene it belongs to, its subtitle and its length, all loudness-matched to -23 LUFS / -6 dBTP, which is
what it actually played; the wav under <WEBGL>/voices are the takes they were cut from and do not correspond one to
one). Unreal cannot import mp3, so this decodes each of them with ffmpeg into the 16-bit PCM wav Unreal wants, keeping
the mono 44.1 kHz they are in — a decode only, so the loudness the WebGL version matched is kept as it is.

It writes SourceArt/Wasami/Voices/<id>.wav and a copy of manifest.json beside them; those are this game's own source
art and are kept in the repository (Git LFS for the wav). dd_voices imports them into /Game/Wasami/Voices with the
manifest's subtitles (WasamiDDTools.import_wasami_voices).

Env: WEBGL - the WebGL version's project (default C:/Users/User/Downloads/wasami-deseption).
"""
import argparse
import json
import os
import shutil
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
WEBGL = os.environ.get("WEBGL", r"C:\Users\User\Downloads\wasami-deseption")
SRC = os.path.join(WEBGL, "public", "voices")
OUT = os.path.join(ROOT, "SourceArt", "Wasami", "Voices")
MANIFEST = "manifest.json"


def convert(clip_id, src=SRC, out=OUT):
    """Decodes <src>/<clip_id>.mp3 into <out>/<clip_id>.wav (16-bit PCM, the mp3's own rate and channels). Returns the
    wav's length in seconds."""
    mp3 = os.path.join(src, clip_id + ".mp3")
    if not os.path.exists(mp3):
        raise FileNotFoundError(mp3)
    wav = os.path.join(out, clip_id + ".wav")
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", mp3, "-c:a", "pcm_s16le", wav], check=True)
    probe = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", wav],
                           check=True, capture_output=True, text=True)
    return float(probe.stdout.strip())


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--src", default=SRC, help="where the WebGL version's clips are")
    parser.add_argument("--out", default=OUT, help="where the wav are written")
    args = parser.parse_args()
    manifest = os.path.join(args.src, MANIFEST)
    if not os.path.exists(manifest):
        sys.exit("missing %s" % manifest)
    with open(manifest, encoding="utf-8") as f:
        clips = json.load(f)["clips"]
    os.makedirs(args.out, exist_ok=True)
    for clip in clips:
        length = convert(clip["id"], args.src, args.out)
        # The decode is gapless, so a length off the manifest's by more than a frame would mean a different source.
        drift = length - clip["duration"]
        print("%-9s %6.3f s (%+.3f s)%s" % (clip["id"], length, drift, "  <- check" if abs(drift) > 0.03 else ""))
    shutil.copyfile(manifest, os.path.join(args.out, MANIFEST))
    print("wrote %d wav and %s to %s" % (len(clips), MANIFEST, args.out))


if __name__ == "__main__":
    main()
