#include "WasamiSphereBurst.h"

#include "Camera/CameraShakeBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"

namespace
{
	// The sphere's scale is Lerp(0, Range, float) / this (the engine sphere's radius).
	constexpr float SphereMeshRadius = 50.f;
	// ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop, Scale, CameraLocal).
	constexpr float ShakeScale = 25.f;
	// PlaySoundAtLocation(Stun_Wave_Attack_New_04) at the origin (the wave has no attenuation, so it is not placed).
	constexpr float WaveVolume = 1.f;
	// M_05_Primal's parameters.
	const FName ColorName(TEXT("Color"));
	const FName DesaturationName(TEXT("Desaturation"));
	const FName OpacityName(TEXT("Opacity"));
}

AWasamiSphereBurst::AWasamiSphereBurst()
{
	// At the root's origin, unscaled until the timeline's first update; no collision at all.
	Sphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(SceneRoot);
	Sphere->SetMobility(EComponentMobility::Movable);
	Sphere->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	SphereMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Sphere")));
	SphereMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/05_Circus/M_05_Primal")));
	WaveSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04")));
	ShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop")));
}

void AWasamiSphereBurst::LoadDefaultAssets(TArray<TObjectPtr<UObject>>& Out) const
{
	Out.Add(SphereMesh.LoadSynchronous());
	Out.Add(SphereMaterial.LoadSynchronous());
	Out.Add(WaveSound.LoadSynchronous());
	Out.Add(ShakeClass.LoadSynchronous());
}

void AWasamiSphereBurst::StartPower()
{
	Sphere->SetStaticMesh(SphereMesh.LoadSynchronous());
	MaterialInstance = Sphere->CreateDynamicMaterialInstance(0, SphereMaterial.LoadSynchronous());
	if (MaterialInstance && SphereColor.IsSet())
	{
		MaterialInstance->SetVectorParameterValue(ColorName, SphereColor.GetValue());
	}

	// From where it was spawned onto the player's capsule centre, where it stays.
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	SetActorLocation(Player ? Player->GetActorLocation() : GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, WaveSound.LoadSynchronous(), FVector::ZeroVector, WaveVolume, WavePitch);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientStartCameraShake(ShakeClass.LoadSynchronous(), ShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
}

void AWasamiSphereBurst::UpdateTimeline(float Position)
{
	// The sphere's radius is Range × float; the volumes fade by float2; the material by desaturation and opacity.
	Sphere->SetWorldScale3D(FVector(FMath::Lerp(0.f, Range, GrowthTrack.Eval(Position)) / SphereMeshRadius));
	Super::UpdateTimeline(Position);
	if (MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(DesaturationName, DesaturationTrack.Eval(Position));
		MaterialInstance->SetScalarParameterValue(OpacityName, OpacityTrack.Eval(Position));
	}
}
