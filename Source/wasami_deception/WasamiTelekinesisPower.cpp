#include "WasamiTelekinesisPower.h"

#include "Camera/CameraShakeBase.h"
#include "Components/PostProcessComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiTelekinesisInterface.h"

namespace
{
	// BP_TelekinesisPower (pak_reference_2). PostProcess: saturation 0 under this gain, a blue screen; PostProcess1's
	// fringe (Primal Fear's).
	const FVector4 TelekinesisTintGain(0., 0.4217270016670227, 1.6100000143051147, 1.);
	constexpr float TelekinesisFlashFringe = 50.f;
	// ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop, Scale, CameraLocal).
	constexpr float TelekinesisShakeScale = 25.f;
	// PlaySoundAtLocation(Stun_Wave_Attack_New_04) at the origin (the wave has no attenuation, so it is not placed).
	constexpr float TelekinesisWaveVolume = 1.f;
	constexpr float TelekinesisWavePitch = 1.f;

	// The timeline's float2 track (CurveFloat_1), Primal Fear's keys. Its float, desaturation and opacity tracks
	// (Primal's keys too) drive nothing.
	const FWasamiCurveKey TelekinesisFadeKeys[] = {
		{-0.011600494384765625f, 0.f, RCIM_Cubic, -0.0950283631682396f, -0.09502881020307541f},
		{0.5f, 1.f, RCIM_Cubic, 4.5270514488220215f, 4.527058124542236f},
	};
}

const FRichCurve& AWasamiTelekinesisPower::TelekinesisFadeCurve()
{
	static const FRichCurve Curve = MakeCurve(TelekinesisFadeKeys);
	return Curve;
}

AWasamiTelekinesisPower::AWasamiTelekinesisPower()
{
	PostProcess->Settings.ColorGain = TelekinesisTintGain;
	PostProcess1->Settings.SceneFringeIntensity = TelekinesisFlashFringe;
	FadeCurve = TelekinesisFadeCurve();

	WaveSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04")));
	ShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop")));
	ForceFieldParticles = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/Wasami/Powers/P_WasamiForceField")));
}

void AWasamiTelekinesisPower::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const AWasamiTelekinesisPower* Defaults = GetDefault<AWasamiTelekinesisPower>();
	Out.Add(Defaults->WaveSound.LoadSynchronous());
	Out.Add(Defaults->ShakeClass.LoadSynchronous());
	Out.Add(Defaults->ForceFieldParticles.LoadSynchronous());
}

int32 AWasamiTelekinesisPower::PullShards(const UObject* WorldContextObject, FVector Center, float Radius)
{
	// Pawn, world-dynamic and world-static bodies (a shard's capsule is world-static), no class filter, nothing ignored,
	// and no line of sight. Only what implements the interface is told: the original checks that, then casts to it.
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_Pawn),
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic),
		UEngineTypes::ConvertToObjectType(ECC_WorldStatic),
	};
	TArray<AActor*> Found;
	UKismetSystemLibrary::SphereOverlapActors(WorldContextObject, Center, Radius, ObjectTypes, nullptr, TArray<AActor*>(), Found);
	int32 Pulled = 0;
	for (AActor* Each : Found)
	{
		if (Each && Each->Implements<UWasamiTelekinesisInterface>())
		{
			IWasamiTelekinesisInterface::Execute_Activate(Each);
			++Pulled;
		}
	}
	return Pulled;
}

int32 AWasamiTelekinesisPower::PullAllShards(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0;
	}
	// Gathered first: Activate may move or collect (destroy) what it is called on.
	TArray<AActor*> Found;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->Implements<UWasamiTelekinesisInterface>())
		{
			Found.Add(*It);
		}
	}
	for (AActor* Each : Found)
	{
		if (IsValid(Each))
		{
			IWasamiTelekinesisInterface::Execute_Activate(Each);
		}
	}
	return Found.Num();
}

void AWasamiTelekinesisPower::StartPower()
{
	// From the spawn 50 m down onto the player's capsule centre, where it stays.
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	const FVector Center = Player ? Player->GetActorLocation() : GetActorLocation();
	SetActorLocation(Center);
	UGameplayStatics::PlaySoundAtLocation(this, WaveSound.LoadSynchronous(), FVector::ZeroVector, TelekinesisWaveVolume, TelekinesisWavePitch);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientStartCameraShake(ShakeClass.LoadSynchronous(), TelekinesisShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
	if (bAllShards)
	{
		PullAllShards(this);
	}
	else
	{
		PullShards(this, Center, Range);
	}
	// The timeline plays next (the base's BeginPlay); the original's Delay(0.2) then brings the force field.
	GetWorldTimerManager().SetTimer(ForceFieldTimer, this, &AWasamiTelekinesisPower::SpawnForceField, ForceFieldDelay);
}

void AWasamiTelekinesisPower::SpawnForceField()
{
	// SpawnEmitterAtLocation where the actor is (the player's place at the start; it does not follow), unturned, at
	// twice the size, destroyed when done, not pooled, active at once.
	if (UParticleSystem* Template = ForceFieldParticles.LoadSynchronous())
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, Template, GetActorLocation(), FRotator::ZeroRotator, FVector(ForceFieldScale),
			true, EPSCPoolMethod::None, true);
	}
}
