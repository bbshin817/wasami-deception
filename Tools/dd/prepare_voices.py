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

It also cuts the lines this game speaks in place of the original's Bierce and nurses (2026-09-27, the user's
instruction: every voice but theirs is Wasami's) from the takes under <WEBGL>/voices, as LINES lists them: each is
loudness-matched the way the WebGL version matched its clips (two-pass loudnorm to -23 LUFS / -6 dBTP, mono 44.1 kHz)
and written to SourceArt/Wasami/Voices/Lines/<id>.wav with lines.json (id, subtitle, whether it shows one, length).
A line spoken with a subtitle is padded with silence to the WebGL version's subtitle time, max(2.2, the clip + 1.2):
UE takes a wave's subtitle down when the wave ends, and these are also played by level sequences and the Bierce
talker, which cannot hold it up longer (WasamiVoice does that for the twelve clips it says itself).

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
TAKES = os.path.join(WEBGL, "voices")
LINES_OUT = os.path.join(OUT, "Lines")
LINES_JSON = "lines.json"

# The lines (id, the take under <WEBGL>/voices, its subtitle, whether it is shown). What each stands in for is
# dd_voices.REPLACES. The guide's (Bierce's) lines and the intercom's announcement carry a subtitle, as the original's
# do; the nurses' lines in the scenes carry none, as theirs do not.
LINES = (
    # Bierce, the guide
    ("alert", "大丈夫かコレ.wav", "大丈夫かコレ", True),
    ("huh", "えっ (意表を突かれる).wav", "えっ", True),
    ("wow", "わぁ...(唖然).mp3", "わぁ…", True),
    ("eh", "え.wav", "え", True),
    ("va", "ヴァ！！.mp3", "ヴァ！！", True),
    ("acho", "あ～ちょ....wav", "あ～ちょ…", True),
    ("dead", "死んだわ.wav", "死んだわ", True),
    ("ketcha", "ケチャくんっ!.wav", "ケチャくんっ！", True),
    ("hunch", "今なんかそういう予感しました!.wav", "今なんかそういう予感しました！", True),
    ("safe", "無事でしたか!.wav", "無事でしたか！", True),
    ("sasuga", "さすがに.wav", "さすがに", True),
    ("nowwhile", "ま今のうちにね.wav", "ま、今のうちにね", True),
    ("naruhodo", "なるほどね!.wav", "なるほどね！", True),
    ("gone", "これ居なくなるよね？.wav", "これ居なくなるよね？", True),
    ("korenanka", "これなんかさぁ.wav", "これなんかさぁ", True),
    ("iya", "いや、こういうのはさ.wav", "いや、こういうのはさ", True),
    ("kimo", "キモ・チワリ.wav", "キモ・チワリ", True),
    ("kimochi", "ちょっと気持ち悪かったカ!.wav", "ちょっと気持ち悪かったカ！", True),
    ("help", "あ～助けてくれ.wav", "あ～助けてくれ", True),
    ("muimi", "これもしかして意味ない？.wav", "これもしかして意味ない？", True),
    ("oomou", "おぉもうこれ.wav", "おぉもうこれ", True),
    # the nurses (this game's enemy Wasami)
    ("dekokode_bright", "でココで(明るめ).wav", "でココで", True),
    ("wait", "ちょっ　と待っ　てね.wav", "ちょっと待ってね", False),
    ("vaa", "ヴァ！！.mp3", "", False),
    ("vooo", "ヴォォォ!!!.wav", "", False),
    ("uooo", "うぉぉぉ.wav", "", False),
    ("gero", "ゲロ.mp3", "", False),
    ("dufu", "デゥフッ.wav", "", False),
    ("kodomo", "お前こどもやな.wav", "", False),
    ("calling", "おーい(暗め).wav", "", False),
    ("aanannka", "あぁなんか.wav", "", False),
    ("greeting", "こんにちワサミ！.mp3", "", False),
    ("haihai", "あぃ～はいはいはい.wav", "", False),
    ("follow", "私と一緒に行きましょう.wav", "", False),
    ("fire", "火がつきませんに？.wav", "", False),
    ("dekokode", "でココで.wav", "", False),
    ("okay", "んあぁオーケーオーケー.wav", "", False),
)
LOUDNESS = "I=-23:TP=-6:LRA=11"


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


def subtitle_seconds(length):
    """The WebGL version's hud.subtitle time for a clip of that length (WasamiVoice::SubtitleSeconds)."""
    return max(2.2, length + 1.2)


def duration(path):
    probe = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", path],
                           check=True, capture_output=True, text=True)
    return float(probe.stdout.strip())


def measure(path, loops=0):
    """loudnorm's first pass over the take (said loops more times over when given): its measured values."""
    first = subprocess.run(["ffmpeg", "-hide_banner", "-stream_loop", str(loops), "-i", path, "-af",
                            "loudnorm=%s:print_format=json" % LOUDNESS, "-f", "null", "-"],
                           check=True, capture_output=True, text=True, encoding="utf-8")
    return json.loads(first.stderr[first.stderr.rindex("{"):first.stderr.rindex("}") + 1])


def cut_line(line_id, take, subtitled, src=TAKES, out=LINES_OUT):
    """Loudness-matches <src>/<take> into <out>/<line_id>.wav (mono 44.1 kHz 16-bit), padded with silence to its
    subtitle time when it shows one. The take is made mono first and measured as such (a mix down after the match
    comes out 2 to 3 dB quieter). Returns (the take's length, the wav's length) in seconds."""
    path = os.path.join(src, take)
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    wav = os.path.join(out, line_id + ".wav")
    mono = wav + ".mono.wav"
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", path, "-ac", "1", "-ar", "44100", "-c:a", "pcm_s16le",
                    mono], check=True)
    try:
        measured = measure(mono)
        if measured["input_i"] == "-inf":
            # Shorter than the 0.4 s blocks integrated loudness is measured over: measured on the take said five times.
            measured = measure(mono, loops=4)
        length = duration(mono)
        chain = ("loudnorm=%s:measured_I=%s:measured_TP=%s:measured_LRA=%s:measured_thresh=%s:offset=%s:linear=true"
                 % (LOUDNESS, measured["input_i"], measured["input_tp"], measured["input_lra"],
                    measured["input_thresh"], measured["target_offset"]))
        if subtitled:
            chain += ",apad=whole_dur=%.3f" % subtitle_seconds(length)
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", mono, "-af", chain, "-ar", "44100",
                        "-c:a", "pcm_s16le", wav], check=True)
    finally:
        os.remove(mono)
    return length, duration(wav)


def cut_lines(src=TAKES, out=LINES_OUT):
    """Every line of LINES, and lines.json beside them."""
    os.makedirs(out, exist_ok=True)
    entries = []
    for line_id, take, subtitle, subtitled in LINES:
        length, padded = cut_line(line_id, take, subtitled, src, out)
        entries.append({"id": line_id, "source": take, "subtitle": subtitle, "subtitled": subtitled,
                        "duration": round(length, 6), "padded": round(padded, 6)})
        print("%-16s %6.3f s -> %6.3f s%s" % (line_id, length, padded, "  (subtitled)" if subtitled else ""))
    with open(os.path.join(out, LINES_JSON), "w", encoding="utf-8") as f:
        json.dump({"normalization": {"integratedLUFS": -23, "truePeakDB": -6}, "lines": entries}, f,
                  ensure_ascii=False, indent=2)
    print("wrote %d lines and %s to %s" % (len(entries), LINES_JSON, out))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--src", default=SRC, help="where the WebGL version's clips are")
    parser.add_argument("--out", default=OUT, help="where the wav are written")
    parser.add_argument("--lines-only", action="store_true", help="only cut the lines (LINES)")
    args = parser.parse_args()
    if args.lines_only:
        cut_lines()
        return
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
    cut_lines()


if __name__ == "__main__":
    main()
