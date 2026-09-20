#include "WasamiVoice.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "SubtitleManager.h"
#include "WasamiAssets.h"

namespace
{
	struct FWasamiVoiceClip
	{
		/** The package the pipeline imports the clip into (dd_voices.asset_name). */
		const TCHAR* Package;
		/** manifest.json's duration. */
		float Seconds;
	};

	// In EWasamiVoice's order. The lengths are the WebGL version's manifest (implementation record 10); the subtitles
	// are in the waves themselves, put there by the import.
	const FWasamiVoiceClip Clips[] = {
		{TEXT("/Game/Wasami/Voices/Wasami_Greeting"), 3.878f},
		{TEXT("/Game/Wasami/Voices/Wasami_Well"), 0.705f},
		{TEXT("/Game/Wasami/Voices/Wasami_Fast"), 0.637f},
		{TEXT("/Game/Wasami/Voices/Wasami_Best"), 0.517f},
		{TEXT("/Game/Wasami/Voices/Wasami_Found"), 0.622f},
		{TEXT("/Game/Wasami/Voices/Wasami_Calling"), 1.027f},
		{TEXT("/Game/Wasami/Voices/Wasami_Others"), 0.690f},
		{TEXT("/Game/Wasami/Voices/Wasami_Think"), 0.862f},
		{TEXT("/Game/Wasami/Voices/Wasami_Remember"), 0.937f},
		{TEXT("/Game/Wasami/Voices/Wasami_Fine"), 1.784f},
		{TEXT("/Game/Wasami/Voices/Wasami_Over"), 0.727f}
	};
	static_assert(UE_ARRAY_COUNT(Clips) == static_cast<uint8>(EWasamiVoice::Over) + 1, "a clip for every voice");

	/** hud.subtitle's time in the WebGL version (its record 06), and the flat one its enemies gave Found (record 15). */
	constexpr float SubtitleLeast = 2.2f;
	constexpr float SubtitleTail = 1.2f;
	constexpr float FoundSubtitleSeconds = 2.4f;

	const FWasamiVoiceClip& Clip(EWasamiVoice Id)
	{
		const int32 Index = static_cast<int32>(Id);
		check(Index >= 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(Clips)));
		return Clips[Index];
	}
}

int32 WasamiVoice::Num()
{
	return static_cast<int32>(UE_ARRAY_COUNT(Clips));
}

FSoftObjectPath WasamiVoice::Path(EWasamiVoice Id)
{
	return WasamiAssets::Path(Clip(Id).Package);
}

float WasamiVoice::Seconds(EWasamiVoice Id)
{
	return Clip(Id).Seconds;
}

float WasamiVoice::SubtitleSeconds(EWasamiVoice Id)
{
	return Id == EWasamiVoice::Found ? FoundSubtitleSeconds : FMath::Max(SubtitleLeast, Seconds(Id) + SubtitleTail);
}

USoundBase* WasamiVoice::Load(EWasamiVoice Id)
{
	return TSoftObjectPtr<USoundBase>(Path(Id)).LoadSynchronous();
}

UAudioComponent* WasamiVoice::Say(const UObject* WorldContextObject, EWasamiVoice Id, float Volume)
{
	USoundBase* Sound = Load(Id);
	UAudioComponent* Component = Sound
		? UGameplayStatics::CreateSound2D(WorldContextObject, Sound, Volume, 1.f, 0.f, nullptr, false, true)
		: nullptr;
	if (!Component)
	{
		return nullptr;
	}
	// The wave's own subtitle would go only while the clip sounds, which for most of these is under a second, so it is
	// suppressed and put up here for the time the WebGL version gave it.
	Component->bSuppressSubtitles = true;
	Component->Play();
	ShowSubtitle(WorldContextObject, Id);
	return Component;
}

void WasamiVoice::ShowSubtitle(const UObject* WorldContextObject, EWasamiVoice Id)
{
	const USoundWave* Wave = Cast<USoundWave>(Load(Id));
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!Wave || Wave->Subtitles.Num() == 0 || !World)
	{
		return;
	}
	// USoundWave::HandleStart's call, with the clip's Duration put aside for SubtitleSeconds. The subtitles are hung on
	// the wave itself rather than a wave instance, so saying the same voice again replaces the line it is showing.
	FSubtitleManager::GetSubtitleManager()->QueueSubtitles(reinterpret_cast<PTRINT>(Wave), Wave->GetSubtitlePriority(),
		Wave->bManualWordWrap, Wave->bSingleLine, SubtitleSeconds(Id), Wave->Subtitles, 0.f, World->GetAudioTimeSeconds());
}
