#include "WasamiBonusShardCollectEffect.h"

#include "Camera/CameraShakeBase.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiPrimalPower.h"

namespace
{
	// BP_BonusShardCollectEffect (pak_reference_2). PostProcess: saturation 0 under this gain, a red screen;
	// PostProcess1's fringe (its gain and saturation have no overrides, and are left out).
	const FVector4 BonusTintGain(1.6100000143051147, 0., 0.4471859931945801, 1.);
	constexpr float BonusFlashFringe = 50.f;
	// ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop, 25, CameraLocal).
	constexpr float BonusShakeScale = 25.f;
}

AWasamiBonusShardCollectEffect::AWasamiBonusShardCollectEffect()
{
	PostProcess->Settings.ColorGain = BonusTintGain;
	PostProcess1->Settings.SceneFringeIntensity = BonusFlashFringe;
	// Its float2 (CurveFloat_1) is BP_PrimalPower's, key for key.
	FadeCurve = AWasamiPrimalPower::PrimalFadeCurve();

	WaveSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04")));
	ShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop")));
}

void AWasamiBonusShardCollectEffect::LoadDefaultAssets(TArray<TObjectPtr<UObject>>& Out) const
{
	Out.Add(WaveSound.LoadSynchronous());
	Out.Add(ShakeClass.LoadSynchronous());
}

void AWasamiBonusShardCollectEffect::StartPower()
{
	// PlaySoundAtLocation at the origin (the wave has no attenuation, so it is not placed).
	UGameplayStatics::PlaySoundAtLocation(this, WaveSound.LoadSynchronous(), FVector::ZeroVector, WaveVolume, WavePitch);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientStartCameraShake(ShakeClass.LoadSynchronous(), BonusShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
}
