#include "WasamiMusicPlayer.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemyInterface.h"

namespace
{
	// The hospital's three tracks (pak_reference_2's Audio/06_Hospital/Music), under /Game/DD as they are imported.
	const TCHAR* MusicFolder = TEXT("/Game/DD/Audio/06_Hospital/Music/");
	const TCHAR* Zone1RegularTrack = TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING");
	const TCHAR* Zone2RegularTrack = TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING");
	const TCHAR* PanicTrack = TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_-_Panic_Track_v1_2_-_LOOPING");

	TSoftObjectPtr<USoundBase> MusicTrack(const TCHAR* Name)
	{
		return TSoftObjectPtr<USoundBase>(WasamiAssets::Path(*(FString(MusicFolder) + Name)));
	}
}

bool FWasamiMusicDoOnce::Enter()
{
	// The node's first entry initialises it, and a Start Closed one closes there and then.
	if (!bInitialised)
	{
		bInitialised = true;
		bClosed = bStartClosed;
	}
	if (bClosed)
	{
		return false;
	}
	bClosed = true;
	return true;
}

FWasamiMusicFades FWasamiMusicState::Update(bool bFadeOut, bool bOverrideMusic, bool bIntense)
{
	FWasamiMusicFades Fades;
	// 1. bFadeOut: everything out, and the Update ends there (the chase is not looked at while it is on).
	if (bFadeOut)
	{
		if (FadeOutOnce.Enter())
		{
			Fades.Regular = EWasamiMusicFade::OutIfPlaying;
			Fades.Panic = EWasamiMusicFade::OutIfPlaying;
			Fades.Override = EWasamiMusicFade::OutIfPlaying;
		}
		return Fades;
	}
	FadeOutOnce.Reset();

	// 2. bOverrideMusic: the override track alone, and the Update ends there as well.
	if (bOverrideMusic)
	{
		if (OverrideOnOnce.Enter())
		{
			Fades.Regular = EWasamiMusicFade::Out;
			Fades.Panic = EWasamiMusicFade::Out;
			Fades.Override = EWasamiMusicFade::In;
			OverrideOffOnce.Reset();
		}
		return Fades;
	}
	if (OverrideOffOnce.Enter())
	{
		Fades.Override = EWasamiMusicFade::Out;
		OverrideOnOnce.Reset();
	}

	// 3. Intense Music ?: the panic track while an enemy chases, the regular one otherwise.
	if (bIntense)
	{
		if (IntenseOnce.Enter())
		{
			Fades.Regular = EWasamiMusicFade::Out;
			Fades.Panic = EWasamiMusicFade::In;
			CalmOnce.Reset();
		}
	}
	else if (CalmOnce.Enter())
	{
		Fades.Panic = EWasamiMusicFade::Out;
		Fades.Regular = EWasamiMusicFade::In;
		IntenseOnce.Reset();
	}
	return Fades;
}

AWasamiMusicPlayer::AWasamiMusicPlayer()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// The original's 'Regular Music', 'Panic Music' and 'Override Music': plain audio components that do not start
	// themselves, so nothing sounds before the first fade in.
	RegularMusic = CreateDefaultSubobject<UAudioComponent>(TEXT("RegularMusic"));
	RegularMusic->SetupAttachment(DefaultSceneRoot);
	RegularMusic->bAutoActivate = false;
	PanicMusic = CreateDefaultSubobject<UAudioComponent>(TEXT("PanicMusic"));
	PanicMusic->SetupAttachment(DefaultSceneRoot);
	PanicMusic->bAutoActivate = false;
	OverrideMusic = CreateDefaultSubobject<UAudioComponent>(TEXT("OverrideMusic"));
	OverrideMusic->SetupAttachment(DefaultSceneRoot);
	OverrideMusic->bAutoActivate = false;

	RegularMusicSound = MusicTrack(Zone1RegularTrack);
	PanicMusicSound = MusicTrack(PanicTrack);
	// The hospital's override component has no sound in either class, so it is left empty here too.
}

void AWasamiMusicPlayer::BeginPlay()
{
	Super::BeginPlay();
	RegularMusic->SetSound(RegularMusicSound.LoadSynchronous());
	PanicMusic->SetSound(PanicMusicSound.LoadSynchronous());
	OverrideMusic->SetSound(OverrideMusicSound.LoadSynchronous());
	GetWorldTimerManager().SetTimer(UpdateTimer, this, &AWasamiMusicPlayer::Update, UpdateInterval, true);
}

void AWasamiMusicPlayer::Update()
{
	LastFades = State.Update(bFadeOut, bOverrideMusic, IsIntenseMusic());
	ApplyFade(RegularMusic, LastFades.Regular);
	ApplyFade(PanicMusic, LastFades.Panic);
	ApplyFade(OverrideMusic, LastFades.Override);
}

bool AWasamiMusicPlayer::IsIntenseMusic() const
{
	// The original's Get All Actors With Interface(DD_EnemyInterface), answering at the first one that is chasing.
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->Implements<UWasamiEnemyInterface>() && IWasamiEnemyInterface::Execute_Chasing(Actor))
		{
			return true;
		}
	}
	return false;
}

void AWasamiMusicPlayer::ApplyFade(UAudioComponent* Component, EWasamiMusicFade Fade)
{
	switch (Fade)
	{
	case EWasamiMusicFade::In:
		Component->FadeIn(FadeDuration, FadeInVolume, 0.f, EAudioFaderCurve::Linear);
		break;
	case EWasamiMusicFade::Out:
		Component->FadeOut(FadeDuration, 0.f, EAudioFaderCurve::Linear);
		break;
	case EWasamiMusicFade::OutIfPlaying:
		// The original's If Playing Fade Out.
		if (Component->IsPlaying())
		{
			Component->FadeOut(FadeDuration, 0.f, EAudioFaderCurve::Linear);
		}
		break;
	default:
		break;
	}
}

AWasamiMusicPlayerZone2::AWasamiMusicPlayerZone2()
{
	RegularMusicSound = MusicTrack(Zone2RegularTrack);
}
