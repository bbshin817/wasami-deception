#include "WasamiPowerOrb.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemyInterface.h"
#include "WasamiSpecialSpawnPoint.h"
#include "WasamiStunCollectEffect.h"
#include "WasamiVignetteSidesWidget.h"

namespace
{
	// BP_PowerOrb's own values (pak_reference_2): the crystal, the capsule under it (840.6 at 0.1 under the crystal's
	// 0.54: a ball of 45.46 cm), the light. The export's FColor is B, G, R, A (0, 146, 255, 255).
	const FVector OrbCrystalLocation(0., 2.288818359375e-05, 125.25003051757812);
	constexpr double OrbCrystalScale = 0.5407835245132446;
	constexpr float OrbCapsuleSize = 840.6161499023438f;
	const FColor OrbLightColour(255, 146, 0, 255);

	// SpawnEmitterAtLocation(P_ky_flash_PowerOrb_Disappear / _Appear, …, 0.5); P_ky_impact at 1.
	constexpr float OrbMoveFlashScale = 0.5f;
	constexpr float OrbImpactScale = 1.f;
	// PlaySound2D(Soul_Shard_Pickup_v2_Cue, 0.8, 0.75); PlaySoundAtLocation(8-Dark_power_ball_countdown_, the origin, 1,
	// 1) (the wave has no attenuation).
	constexpr float OrbPickupVolume = 0.8f;
	constexpr float OrbPickupPitch = 0.75f;
	constexpr float OrbCountdownVolume = 1.f;
	constexpr float OrbCountdownPitch = 1.f;

	// What the pickup stuns (the enemies carry it, WasamiEnemy.cpp).
	const FName OrbEnemyTag(TEXT("Enemy"));

	FAutoConsoleCommandWithWorldAndArgs PowerOrbCommand(TEXT("Wasami.PowerOrb"),
		TEXT("Wasami.PowerOrb [N]: the level's power orbs start their 5 s flicker now (Spawn Power Orb); with N, they move to their spawn point N at once instead (the flashes, the next 150 s)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AWasamiPowerOrb> It(World); It; ++It)
			{
				if (Args.Num() > 0)
				{
					It->MoveToSpawnPoint(FCString::Atoi(*Args[0]));
				}
				else
				{
					It->SpawnSpecialShard();
				}
			}
		}));
}

AWasamiPowerOrb::AWasamiPowerOrb()
{
	SoulShard->SetRelativeLocation(OrbCrystalLocation);
	SoulShard->SetRelativeScale3D(FVector(OrbCrystalScale));
	Capsule->InitCapsuleSize(OrbCapsuleSize, OrbCapsuleSize);
	PointLight->LightColor = OrbLightColour;
	SpawnPointClass = AWasamiPowerOrbSpawnPoint::StaticClass();
	MoveFlashScale = OrbMoveFlashScale;

	CrystalMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/Shared/power_orb")));
	CrystalMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Fords_Materials/m_crystal_Inst3")));
	MapMarkMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Shared/M_PowerOrb")));
	DisappearFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Disappear")));
	AppearFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Appear")));
	CollectImpact = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_impact")));
	PickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue")));
	CountdownSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/8-Dark_power_ball_countdown_")));
	CollectEffectClass = AWasamiStunCollectEffect::StaticClass();
}

void AWasamiPowerOrb::LoadPickupAssets(TArray<TObjectPtr<UObject>>& Out) const
{
	Out.Add(CountdownSound.LoadSynchronous());
	if (CollectEffectClass)
	{
		CollectEffectClass->GetDefaultObject<AWasamiStunCollectEffect>()->LoadDefaultAssets(Out);
	}
}

void AWasamiPowerOrb::CollectShard()
{
	// The orb clears its flicker's timer too; it is destroyed below, which ends the flicker's Delay.
	GetWorldTimerManager().ClearTimer(FlickerTimer);
	SpawnFlash(CollectImpact.LoadSynchronous(), OrbImpactScale);

	// The original destroys the orb here and goes on; it goes last here. Its Used Stun Orbs? on the game instance is
	// for an achievement only and is not copied.
	UGameplayStatics::PlaySound2D(this, PickupSound.LoadSynchronous(), OrbPickupVolume, OrbPickupPitch);
	UWasamiVignetteSidesWidget::Show(this, UWasamiVignetteSidesWidget::StunnedColor, true, UWasamiVignetteSidesWidget::StunnedText());
	PlayCollectShake();
	UGameplayStatics::PlaySoundAtLocation(this, CountdownSound.LoadSynchronous(), FVector::ZeroVector, OrbCountdownVolume, OrbCountdownPitch);
	// BP_StunCollectEffect at the origin, unrotated; it moves itself onto the player.
	if (CollectEffectClass)
	{
		GetWorld()->SpawnActor<AWasamiStunCollectEffect>(CollectEffectClass, FTransform::Identity);
	}
	StunAllEnemies(this);
	Destroy();
}

int32 AWasamiPowerOrb::StunAllEnemies(const UObject* WorldContextObject)
{
	// GetAllActorsWithTag(Enemy), then a cast to DD_EnemyInterface: only the implementers are stunned.
	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, OrbEnemyTag, Tagged);
	int32 Stunned = 0;
	for (AActor* Each : Tagged)
	{
		if (Each && Each->Implements<UWasamiEnemyInterface>())
		{
			IWasamiEnemyInterface::Execute_SetState(Each, EWasamiEnemyState::Stun, true);
			++Stunned;
		}
	}
	return Stunned;
}
