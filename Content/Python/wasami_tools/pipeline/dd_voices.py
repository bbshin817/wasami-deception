"""This game's own voices: the lines of the user's Wasami, as SoundWaves under /Game/Wasami/Voices.

The sources are SourceArt/Wasami/Voices/<id>.wav and the manifest.json beside them (this game's own source art, which
python Tools/dd/prepare_voices.py decodes from the WebGL version's mp3 and copies). The waves keep UE's defaults
(volume 1, no looping) and play through the original's Dialogue sound class, which is the bus the WebGL version played
them on (voice, at volume 1.0; its implementation record 06). The manifest's subtitle goes on the ones spoken with
one, so UE's own Subtitles put them up and take them down as they do the original's dialogue (dd_dialogue); the rest
carry none. Where and how loud each is played is the scene that uses it.
"""
import json
import os

from wasami_tools.pipeline import dd_assets, paths

SOURCE = os.path.join(paths.SOURCE_ART, "Wasami", "Voices")
MANIFEST = "manifest.json"
VOICES_ROOT = paths.WASAMI_ROOT + "/Voices"
# The bus the WebGL version played them on (voice) is the original's Dialogue sound class, as its dialogue's is.
SOUND_CLASS = "Audio/SoundMix/DD_SoundClass_Dialogue"

# What this game speaks, by the manifest's id (the WebGL version's records 06 and 15). Those with a subtitle up: the
# guide's greeting, the first shard, a boost made and the secret door opening (Game.say), and the enemies' call as one
# spots the player (which that version subtitles from game.ts).
SUBTITLED = ("greeting", "well", "fast", "best", "found")
# ... and those spoken with none: what an enemy calls out on patrol, the death screen's two, and the cry of the enemy
# that has caught the player (you, which the WebGL version never played and this game says over the capture, where a
# subtitle would only sit on top of a scene that fills the screen; WasamiCapture).
SILENT = ("calling", "others", "think", "remember", "fine", "over", "you")
CLIPS = SUBTITLED + SILENT
# The manifest's other three (follow, safe and wait) are never played, so they are not imported, as the dialogue
# leaves out the original's lines this game's levels do not speak (dd_dialogue).


def asset_name(clip_id):
    """'greeting' -> 'Wasami_Greeting'."""
    return "Wasami_" + clip_id.capitalize()


def manifest():
    """The manifest's clips by id ({'greeting': {'subtitle': ..., 'duration': ..., ...}, ...})."""
    path = os.path.join(SOURCE, MANIFEST)
    if not os.path.exists(path):
        raise FileNotFoundError("%s is missing: run python Tools/dd/prepare_voices.py first." % path)
    with open(path, encoding="utf-8") as f:
        return {clip["id"]: clip for clip in json.load(f)["clips"]}


def voice(clip, sound_class, subtitled):
    """Imports the clip's wav as the SoundWave /Game/Wasami/Voices/Wasami_<Id>, with its subtitle at 0 s where it
    shows one. Returns its package path."""
    subtitles = [(0.0, clip["subtitle"])] if subtitled else []
    return dd_assets.sound_file(os.path.join(SOURCE, clip["id"] + ".wav"),
                                "%s/%s" % (VOICES_ROOT, asset_name(clip["id"])),
                                subtitles=subtitles, sound_class=sound_class)


def import_all():
    """Every voice this game speaks. Returns how many were made with a subtitle and how many without."""
    clips = manifest()
    missing = [clip_id for clip_id in CLIPS if clip_id not in clips]
    if missing:
        raise KeyError("%s has no %s" % (MANIFEST, ", ".join(missing)))
    sound_class = dd_assets.sound_class(SOUND_CLASS)
    for clip_id in CLIPS:
        voice(clips[clip_id], sound_class, clip_id in SUBTITLED)
    return {"subtitled": len(SUBTITLED), "silent": len(SILENT)}
