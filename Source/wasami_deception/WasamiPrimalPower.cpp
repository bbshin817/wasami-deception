#include "WasamiPrimalPower.h"

#include "Camera/CameraShakeBase.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiEnemyInterface.h"

namespace
{
	// BP_PrimalPower (pak_reference_2). PostProcess: saturation 0 under this gain, a red screen; PostProcess1's fringe.
	const FVector4 PrimalTintGain(1.6100000143051147, 0.12956300377845764, 0., 1.);
	constexpr float PrimalFlashFringe = 50.f;
	// The sphere's scale is Lerp(0, Range, float) / this (the engine sphere's radius).
	constexpr float SphereMeshRadius = 50.f;
	// ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop, Scale, CameraLocal).
	constexpr float ShakeScale = 25.f;
	// PlaySoundAtLocation(Stun_Wave_Attack_New_04) at the origin (the wave has no attenuation, so it is not placed).
	constexpr float WaveVolume = 1.f;
	constexpr float WavePitch = 1.f;
	// M_05_Primal's parameters the timeline drives.
	const FName DesaturationName(TEXT("Desaturation"));
	const FName OpacityName(TEXT("Opacity"));

	// The timeline's tracks (CurveFloat_0 to _3), key for key.
	const FWasamiCurveKey GrowthKeys[] = {
		{0.f, 0.f, RCIM_Cubic, 1.5797574520111084f, 1.5797579288482666f},
		{0.992901086807251f, 0.9189547300338745f, RCIM_Cubic, 0.3629918098449707f, 0.36299169063568115f},
		{1.5134799480438232f, 0.9834880232810974f, RCIM_Linear, 0.f, 0.f},
	};
	const FWasamiCurveKey FadeKeys[] = {
		{-0.011600494384765625f, 0.f, RCIM_Cubic, -0.0950283631682396f, -0.09502881020307541f},
		{0.5f, 1.f, RCIM_Cubic, 4.5270514488220215f, 4.527058124542236f},
	};
	const FWasamiCurveKey DesaturationKeys[] = {
		{0.006957054138183594f, 0.015488147735595703f, RCIM_Cubic, 0.f, 0.f},
		{1.016563892364502f, 0.17036807537078857f, RCIM_Cubic, 0.6986591815948486f, 0.6986591815948486f},
		{1.4109413623809814f, 0.996394693851471f, RCIM_Linear, 0.f, 0.f},
	};
	const FWasamiCurveKey OpacityKeys[] = {
		{0.f, 1.f, RCIM_Cubic, -0.21745750308036804f, -0.21745805442333221f},
		{0.6773991584777832f, 0.725354790687561f, RCIM_Cubic, -0.5983153581619263f, -0.5983161926269531f},
		{1.592355489730835f, 0.002581477165222168f, RCIM_Linear, 0.f, 0.f},
	};
}

const FRichCurve& AWasamiPrimalPower::GrowthCurve()
{
	static const FRichCurve Curve = MakeCurve(GrowthKeys);
	return Curve;
}

const FRichCurve& AWasamiPrimalPower::DesaturationCurve()
{
	static const FRichCurve Curve = MakeCurve(DesaturationKeys);
	return Curve;
}

const FRichCurve& AWasamiPrimalPower::OpacityCurve()
{
	static const FRichCurve Curve = MakeCurve(OpacityKeys);
	return Curve;
}

const FRichCurve& AWasamiPrimalPower::PrimalFadeCurve()
{
	static const FRichCurve Curve = MakeCurve(FadeKeys);
	return Curve;
}

AWasamiPrimalPower::AWasamiPrimalPower()
{
	PostProcess->Settings.ColorGain = PrimalTintGain;
	PostProcess1->Settings.SceneFringeIntensity = PrimalFlashFringe;
	FadeCurve = PrimalFadeCurve();

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

void AWasamiPrimalPower::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const AWasamiPrimalPower* Defaults = GetDefault<AWasamiPrimalPower>();
	Out.Add(Defaults->SphereMesh.LoadSynchronous());
	Out.Add(Defaults->SphereMaterial.LoadSynchronous());
	Out.Add(Defaults->WaveSound.LoadSynchronous());
	Out.Add(Defaults->ShakeClass.LoadSynchronous());
}

int32 AWasamiPrimalPower::StunEnemies(const UObject* WorldContextObject, FVector Center, float Radius)
{
	// Pawn bodies only, no class filter, nothing ignored, and no line of sight. The original takes an actor that
	// implements DD_EnemyInterface or has the Enemy tag, then casts it to the interface: only the implementers are stunned.
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_Pawn)};
	TArray<AActor*> Found;
	UKismetSystemLibrary::SphereOverlapActors(WorldContextObject, Center, Radius, ObjectTypes, nullptr, TArray<AActor*>(), Found);
	int32 Stunned = 0;
	for (AActor* Each : Found)
	{
		if (Each && Each->Implements<UWasamiEnemyInterface>())
		{
			IWasamiEnemyInterface::Execute_SetState(Each, EWasamiEnemyState::Stun, false);
			++Stunned;
		}
	}
	return Stunned;
}

void AWasamiPrimalPower::StartPower()
{
	Sphere->SetStaticMesh(SphereMesh.LoadSynchronous());
	MaterialInstance = Sphere->CreateDynamicMaterialInstance(0, SphereMaterial.LoadSynchronous());

	// From the spawn 50 m down onto the player's capsule centre, where it stays.
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	const FVector Center = Player ? Player->GetActorLocation() : GetActorLocation();
	SetActorLocation(Center);
	UGameplayStatics::PlaySoundAtLocation(this, WaveSound.LoadSynchronous(), FVector::ZeroVector, WaveVolume, WavePitch);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientStartCameraShake(ShakeClass.LoadSynchronous(), ShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
	StunEnemies(this, Center, Range);
}

void AWasamiPrimalPower::UpdateTimeline(float Position)
{
	// The sphere's radius is Range × float; the volumes fade by float2; the material by desaturation and opacity.
	Sphere->SetWorldScale3D(FVector(FMath::Lerp(0.f, Range, GrowthCurve().Eval(Position)) / SphereMeshRadius));
	Super::UpdateTimeline(Position);
	if (MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(DesaturationName, DesaturationCurve().Eval(Position));
		MaterialInstance->SetScalarParameterValue(OpacityName, OpacityCurve().Eval(Position));
	}
}
