#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

class UAudioComponent;
class USoundBase;

/**
 * A Wasami voice (/Game/Wasami/Voices, implementation record 10): this game's guide speaks where the original has no
 * line of its own, as the WebGL version had her do. The ids are that version's clips (its public/voices/manifest.json):
 * the eleven it plays, and You, which it never played and this game cries over a capture.
 */
enum class EWasamiVoice : uint8
{
	/** A run begins: こんにちワサミ！ */
	Greeting,
	/** The first soul shard of the run: 非常にいい！ */
	Well,
	/** The speed boost goes off: はっや！ */
	Fast,
	/** A secret wall has opened: さいこう！ */
	Best,
	/** An enemy has seen the player: ここか！ */
	Found,
	/** An enemy on its rounds: おーい。 */
	Calling,
	/** An enemy on its rounds: ほかのみんなは？ */
	Others,
	/** An enemy on its rounds: 俺のことも想え。 */
	Think,
	/** An enemy on its rounds: 覚えてます。 */
	Remember,
	/** A death with lives left: まぁまぁ、そういう時だってあるか。 */
	Fine,
	/** A death with none: あっ、終わりです。 */
	Over,
	/** The enemy that has caught the player: オマエ・ジャ。 */
	You
};

/**
 * The voices, and how they are said. The waves are referenced softly and loaded when first said (WasamiAssets.h), and
 * their subtitles are put up for the WebGL version's time rather than the clip's — half a second is no time to read
 * (implementation record 10).
 */
namespace WasamiVoice
{
	/** How many voices there are (EWasamiVoice's values are 0 to this less one). */
	WASAMI_DECEPTION_API int32 Num();

	/** '/Game/Wasami/Voices/Wasami_Greeting.Wasami_Greeting' … */
	WASAMI_DECEPTION_API FSoftObjectPath Path(EWasamiVoice Id);

	/** How long the clip sounds (manifest.json's duration, which the imported wave matches). */
	WASAMI_DECEPTION_API float Seconds(EWasamiVoice Id);

	/**
	 * How long the subtitle stays up: the WebGL version's hud.subtitle time, max(2.2, the clip + 1.2), and the flat
	 * 2.4 its enemies gave Found.
	 */
	WASAMI_DECEPTION_API float SubtitleSeconds(EWasamiVoice Id);

	/** The wave, loaded now (null where it has not been imported). */
	WASAMI_DECEPTION_API USoundBase* Load(EWasamiVoice Id);

	/**
	 * Say: plays the voice as a 2D sound of its own (Game.say's voice bus is the wave's DD_SoundClass_Dialogue) and,
	 * where the wave carries a subtitle, shows it for SubtitleSeconds. Returns the component playing it, or null
	 * where the wave is missing or the world has no audio (a test's).
	 */
	WASAMI_DECEPTION_API UAudioComponent* Say(const UObject* WorldContextObject, EWasamiVoice Id, float Volume = 1.f);

	/**
	 * Shows the voice's subtitle for SubtitleSeconds, as Say does for what it plays: the sound playing it must have
	 * its own subtitles suppressed, or the wave puts up the same line for as long as it sounds.
	 */
	WASAMI_DECEPTION_API void ShowSubtitle(const UObject* WorldContextObject, EWasamiVoice Id);
}
