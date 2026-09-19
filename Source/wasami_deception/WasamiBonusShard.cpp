#include "WasamiBonusShard.h"

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
#include "WasamiBonusShardCollectEffect.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSaveGame.h"
#include "WasamiSpecialSpawnPoint.h"
#include "WasamiVignetteSidesWidget.h"

namespace
{
	// BP_BonusShard's own values (pak_reference_2): the crystal, the capsule under it (49.57 at 0.1 under the crystal's
	// 20: a ball of 99.14 cm), the light. The export's FColor is B, G, R, A (0, 31, 255, 255).
	const FVector BonusCrystalLocation(0., 2.288818359375e-05, 97.08537292480469);
	constexpr double BonusCrystalScale = 20.;
	constexpr float BonusCapsuleSize = 49.57180404663086f;
	const FColor BonusLightColour(255, 31, 0, 255);

	// SpawnEmitterAtLocation(P_ky_flash_BonusOrb_Disappear / _Appear, …, 1); P_ky_impact1 at 1.
	constexpr float BonusMoveFlashScale = 1.f;
	constexpr float BonusImpactScale = 1.f;
	// PlaySound2D(Bonus_Shard_Pickup_v1, 0.8, 1).
	constexpr float BonusPickupVolume = 0.8f;
	constexpr float BonusPickupPitch = 1.f;

	// What the reveal puts on the map (the enemies carry it, WasamiEnemy.cpp).
	const FName BonusEnemyTag(TEXT("Enemy"));

	FAutoConsoleCommandWithWorldAndArgs BonusShardCommand(TEXT("Wasami.BonusShard"),
		TEXT("Wasami.BonusShard [N]: the level's bonus shards start their 5 s flicker now (Spawn Special Shard); with N, they move to their spawn point N at once instead (the flashes, the next 150 s)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AWasamiBonusShard> It(World); It; ++It)
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

AWasamiBonusShard::AWasamiBonusShard()
{
	SoulShard->SetRelativeLocation(BonusCrystalLocation);
	SoulShard->SetRelativeScale3D(FVector(BonusCrystalScale));
	Capsule->InitCapsuleSize(BonusCapsuleSize, BonusCapsuleSize);
	PointLight->LightColor = BonusLightColour;
	SpawnPointClass = AWasamiBonusShardSpawnPoint::StaticClass();
	MoveFlashScale = BonusMoveFlashScale;

	CrystalMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/Ring_Assets/soul_shard")));
	CrystalMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Fords_Materials/m_crystal_Inst")));
	MapMarkMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Shared/M_Bonus_Shard")));
	DisappearFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_BonusOrb_Disappear")));
	AppearFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_BonusOrb_Appear")));
	CollectImpact = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_impact1")));
	PickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Bonus_Shard_Pickup_v1")));
	CollectEffectClass = AWasamiBonusShardCollectEffect::StaticClass();
}

void AWasamiBonusShard::LoadPickupAssets(TArray<TObjectPtr<UObject>>& Out) const
{
	if (CollectEffectClass)
	{
		CollectEffectClass->GetDefaultObject<AWasamiBonusShardCollectEffect>()->LoadDefaultAssets(Out);
	}
}

void AWasamiBonusShard::BeginCycle()
{
	GetWorldTimerManager().SetTimerForNextTick(this, &AWasamiBonusShard::CheckSave);
}

void AWasamiBonusShard::CheckSave()
{
	// Array_Contains(the game mode's Struct Save's entry for the level's BonusShards, ID). Its random Float 1 and the
	// game state it keeps are used by nothing.
	const AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>();
	const UWasamiSaveGame* Save = Mode ? Mode->GetSave() : nullptr;
	if (Save && Save->Hospital.BonusShards.Contains(ID))
	{
		Destroy();
		return;
	}
	StartSpawnTimer();
}

void AWasamiBonusShard::CollectShard()
{
	// Only the spawn timer is cleared: a flicker under way goes on (see MoveToSpawnPoint).
	if (AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>())
	{
		if (UWasamiSaveGame* Save = Mode->GetSave())
		{
			Save->Hospital.BonusShards.AddUnique(ID);
		}
	}
	SpawnFlash(CollectImpact.LoadSynchronous(), BonusImpactScale);
	UGameplayStatics::PlaySound2D(this, PickupSound.LoadSynchronous(), BonusPickupVolume, BonusPickupPitch);
	UWasamiVignetteSidesWidget::Show(this, UWasamiVignetteSidesWidget::RevealedColor, true, UWasamiVignetteSidesWidget::RevealedText());
	PlayCollectShake();

	// The looks go; the actor stays for the reveal.
	if (IsValid(StaticMesh))
	{
		StaticMesh->DestroyComponent();
	}
	RemoveCrystal();
	if (IsValid(PointLight))
	{
		PointLight->DestroyComponent();
	}
	// BP_BonusShardCollectEffect at the origin, unrotated. The original's Collected Special Shard print is left out.
	if (CollectEffectClass)
	{
		GetWorld()->SpawnActor<AWasamiBonusShardCollectEffect>(CollectEffectClass, FTransform::Identity);
	}

	// Cast to BP_DD_PlayerCharacter: without it the reveal never starts (and the shard stays).
	RevealPlayer = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (RevealPlayer.IsValid())
	{
		RevealEnemies();
	}
}

void AWasamiBonusShard::RevealEnemies()
{
	// The ForEach calls Delay(RandomFloatInRange(1, 2)) in its body, so a round with no enemy sets no next round; the
	// Delay 60 after the loop runs once from the first round.
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(this, BonusEnemyTag, Enemies);
	FTimerManager& Timers = GetWorldTimerManager();
	for (AActor* Each : Enemies)
	{
		if (AWasamiPlayerCharacter* Player = RevealPlayer.Get())
		{
			Player->AddToMap(Each->GetClass());
		}
		if (!bRoundPending)
		{
			bRoundPending = true;
			Timers.SetTimer(RoundTimer, this, &AWasamiBonusShard::OnRevealRound, FMath::FRandRange(RevealRoundMin, RevealRoundMax), false);
		}
	}
	if (!bRevealEnding)
	{
		bRevealEnding = true;
		Timers.SetTimer(RevealTimer, this, &AWasamiBonusShard::EndReveal, RevealLength, false);
	}
}

float AWasamiBonusShard::GetRevealTimeLeft() const
{
	return GetWorldTimerManager().GetTimerRemaining(RevealTimer);
}

void AWasamiBonusShard::OnRevealRound()
{
	// Through the Gate, which is open until the reveal's end (the end destroys the shard, so no round comes after it).
	bRoundPending = false;
	RevealEnemies();
}

void AWasamiBonusShard::EndReveal()
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(this, BonusEnemyTag, Enemies);
	if (AWasamiPlayerCharacter* Player = RevealPlayer.Get())
	{
		for (AActor* Each : Enemies)
		{
			Player->RemoveFromMap(Each->GetClass());
		}
	}
	Destroy();
}
