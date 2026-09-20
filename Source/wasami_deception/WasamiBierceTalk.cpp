#include "WasamiBierceTalk.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"

namespace
{
	// The original's AudioComponent0 plays through DialogueAttenuation (its Audio/Misc), under /Game/DD as it is
	// imported. Attenuate? decides whether a line is heard through it at all (bAllowSpatialization).
	const TCHAR* BierceAttenuationPath = TEXT("/Game/DD/Audio/Misc/DialogueAttenuation");
}

EWasamiTalkStep WasamiTalkStep(bool bHalt, bool bPlaying, bool bWaiting)
{
	if (bHalt)
	{
		return EWasamiTalkStep::Nothing;
	}
	if (!bPlaying)
	{
		return EWasamiTalkStep::Play;
	}
	return bWaiting ? EWasamiTalkStep::AlreadyWaiting : EWasamiTalkStep::Wait;
}

AWasamiBierceTalk::AWasamiBierceTalk()
{
	PrimaryActorTick.bCanEverTick = false;

	// AudioComponent0 as the original's AmbientSound parent makes it, with the Blueprint's bAutoActivate false on top:
	// nothing is heard until a Talk.
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent0"));
	AudioComponent->bAutoActivate = false;
	AudioComponent->bStopWhenOwnerDestroyed = true;
	AudioComponent->bShouldRemainActiveIfDropped = true;
	AudioComponent->Mobility = EComponentMobility::Movable;
	RootComponent = AudioComponent;

	bReplicates = false;
	SetHidden(true);
	SetCanBeDamaged(false);

	Attenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(BierceAttenuationPath));
}

void AWasamiBierceTalk::BeginPlay()
{
	Super::BeginPlay();
	if (USoundAttenuation* Settings = Attenuation.LoadSynchronous())
	{
		AudioComponent->AttenuationSettings = Settings;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no %s (run WasamiDDTools.import_dd_gimmicks)"), *GetClass()->GetName(),
			BierceAttenuationPath);
	}
}

void AWasamiBierceTalk::Talk(USoundBase* WhatToSay, bool bAttenuate)
{
	// Talk writes both onto the ubergraph's frame before anything is looked at, so even a halted call leaves them
	// there; then the graph puts Attenuate? on the component and only after that asks about Halt.
	PendingSound = WhatToSay;
	if (AudioComponent)
	{
		AudioComponent->bAllowSpatialization = bAttenuate;
	}
	Step(true, IsWaiting());
}

void AWasamiBierceTalk::StopTalking()
{
	// Stop Talking is the component's Stop alone: a Delay already running is left to play what Talk asked for last.
	if (AudioComponent)
	{
		AudioComponent->Stop();
	}
}

void AWasamiBierceTalk::Step(bool bLookAtHalt, bool bWaiting)
{
	LastStep = WasamiTalkStep(bLookAtHalt && bHalt, IsSpeaking(), bWaiting);
	switch (LastStep)
	{
	case EWasamiTalkStep::Play:
		if (AudioComponent)
		{
			AudioComponent->SetSound(PendingSound);
			AudioComponent->Play(0.f);
		}
		break;
	case EWasamiTalkStep::Wait:
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(WaitTimer, this, &AWasamiBierceTalk::Resume, WaitInterval, false);
		}
		break;
	default:
		break;
	}
}

void AWasamiBierceTalk::Resume()
{
	// The Delay comes back inside the loop, past the Halt check, and the Delay that woke this has run out — nothing is
	// waiting any more. (IsWaiting() cannot say so here: the engine calls a timer active while its own callback runs.)
	Step(false, false);
}

bool AWasamiBierceTalk::IsSpeaking() const
{
	return AudioComponent && AudioComponent->IsPlaying();
}

bool AWasamiBierceTalk::IsWaiting() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(WaitTimer);
}

AWasamiBierceTalk* AWasamiBierceTalk::Find(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull)
		: nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AWasamiBierceTalk> It(World); It; ++It)
	{
		return *It;
	}
	UE_LOG(LogTemp, Warning, TEXT("AWasamiBierceTalk::Find: no talker in %s"), *World->GetName());
	return nullptr;
}
