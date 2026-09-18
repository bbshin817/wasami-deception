#include "Misc/AutomationTest.h"
#include "../WasamiZoneBarrier.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneBarrierTest, "Wasami.ZoneBarrier.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneBarrierTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiZoneBarrier* Barrier = World->SpawnActor<AWasamiZoneBarrier>(FVector(100., 200., 150.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the barrier"), Barrier))
	{
		return false;
	}

	// The class's parts (BP_ZoneBarrier's SCS).
	UStaticMeshComponent* Front = Barrier->GetStaticMesh1();
	UStaticMeshComponent* Back = Barrier->GetStaticMesh();
	TestTrue(TEXT("interact"), Barrier->ActorHasTag(TEXT("interact")));
	TestTrue(TEXT("the root at 3.2"), Barrier->GetRootComponent()->GetRelativeScale3D().Equals(FVector(3.2)));
	TestTrue(TEXT("both planes the engine's Plane"), Front->GetStaticMesh() && Back->GetStaticMesh() == Front->GetStaticMesh()
		&& Front->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Plane.Plane"));
	// Standing up, facing along x: the plane's normal (its z) turned to ±x.
	TestEqual(TEXT("the front plane faces x"), FMath::Abs(Front->GetComponentQuat().GetUpVector().X), 1., 1e-4);
	TestEqual(TEXT("the back plane faces x"), FMath::Abs(Back->GetComponentQuat().GetUpVector().X), 1., 1e-4);
	TestEqual(TEXT("the back plane 3 cm behind (× 3.2)"), Back->GetComponentLocation().X - Front->GetComponentLocation().X,
		-3.035816192626953 * 3.2, 1e-3);
	TestTrue(TEXT("the planes block"), Front->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics
		&& Front->GetCollisionObjectType() == ECC_WorldStatic && Front->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
		&& Back->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	TestFalse(TEXT("and are not in the navigation"), Front->CanEverAffectNavigation());
	const UPointLightComponent* Light = Barrier->GetPointLight();
	TestTrue(TEXT("a magenta light"), Light->LightColor == FColor(255, 0, 188, 255) && Light->Intensity == 2500.f
		&& Light->AttenuationRadius == 400.f && Light->IntensityUnits == ELightUnits::Unitless);
	const UAudioComponent* Audio = Barrier->GetAudio();
	TestTrue(TEXT("the loop at 0.5, pitch 0.8"), Audio->VolumeMultiplier == 0.5f && Audio->PitchMultiplier == 0.8f);
	TestTrue(TEXT("its own attenuation"), Audio->bOverrideAttenuation && Audio->AttenuationOverrides.bEnableOcclusion
		&& Audio->AttenuationOverrides.FalloffDistance == 2000.f
		&& Audio->AttenuationOverrides.DistanceAlgorithm == EAttenuationDistanceModel::NaturalSound);
	TestTrue(TEXT("humming Barrier_Loop"), Audio->Sound && Audio->Sound->GetName() == TEXT("Barrier_Loop"));

	// Destroy: the burst at the back plane, and the barrier gone.
	const FVector BurstAt = Back->GetComponentLocation();
	Barrier->DestroyBarrier();
	TestTrue(TEXT("gone"), !IsValid(Barrier) || Barrier->IsActorBeingDestroyed());
	bool bBurst = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TArray<UParticleSystemComponent*> Systems;
		It->GetComponents(Systems);
		for (const UParticleSystemComponent* System : Systems)
		{
			bBurst |= System->Template && System->Template->GetName() == TEXT("P_ky_impact3")
				&& System->GetComponentLocation().Equals(BurstAt, 1e-3) && System->GetComponentScale().Equals(FVector(2.));
		}
	}
	TestTrue(TEXT("P_ky_impact3 at the back plane, twice its size"), bBurst);
	return true;
}

#endif
