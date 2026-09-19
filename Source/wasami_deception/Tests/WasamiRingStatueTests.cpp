#include "Misc/AutomationTest.h"
#include "../WasamiRingPiece.h"
#include "../WasamiRingStatue.h"
#include "../WasamiShard.h"
#include "../WasamiTextPromptWidget.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Particles/ParticleSystemComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "WasamiTestListener.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Whether a Visibility trace down through the actor from above finds it. */
	bool LookFinds(UWorld* World, const AActor* Actor)
	{
		FHitResult Hit;
		const FVector At = Actor->GetActorLocation();
		return World->LineTraceSingleByChannel(Hit, At + FVector(0., 0., 1000.), At - FVector(0., 0., 1000.), ECC_Visibility)
			&& Hit.GetActor() == Actor;
	}

	/** Seconds of play in ticks of 0.1 s at most (a tick of nothing first starts the timers set since the last). */
	void AdvanceStatueWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 0.f; Left -= 0.1f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.1f));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiRingStatueActorTest, "Wasami.RingStatue.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiRingStatueActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	AWasamiRingStatue* Statue = World->SpawnActor<AWasamiRingStatue>(FVector(0., 0., 0.), FRotator::ZeroRotator);
	AWasamiRingPiece* Piece = World->SpawnActor<AWasamiRingPiece>(FVector(1000., 0., 0.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the altar"), Statue) || !TestNotNull(TEXT("the piece"), Piece) || !TestNotNull(TEXT("a cube"), Cube))
	{
		return false;
	}

	// BP_01_Statue: a StaticMeshActor, its mesh the root at 2.8 and tagged interact.
	UStaticMeshComponent* Mesh = Statue->GetStaticMeshComponent();
	TestTrue(TEXT("the mesh is the root"), Statue->GetRootComponent() == Mesh);
	TestTrue(TEXT("at 2.8"), Mesh->GetRelativeScale3D().Equals(FVector(2.8), 1e-5));
	TestTrue(TEXT("the hand shows over it"), Mesh->ComponentHasTag(TEXT("interact")) && Statue->ActorHasTag(TEXT("interact")));
	TestTrue(TEXT("movable (the level's baked lighting stays)"), Mesh->Mobility == EComponentMobility::Movable);
	TestTrue(TEXT("something to use"), Statue->Implements<UWasamiInteractable>());
	Mesh->SetStaticMesh(Cube);
	TestTrue(TEXT("the look finds it"), LookFinds(World, Statue));

	// BP_08_RingPiece_NoPickup: turned, 20 times its size, only to be seen.
	UStaticMeshComponent* PieceMesh = Piece->GetStaticMesh();
	TestTrue(TEXT("turned (-40, 0, 30)"), PieceMesh->GetRelativeRotation().Equals(FRotator(-40., 0., 30.), 1e-3));
	TestTrue(TEXT("20 times its size"), PieceMesh->GetRelativeScale3D().Equals(FVector(20.)));
	TestFalse(TEXT("no shadow"), PieceMesh->CastShadow);
	TestTrue(TEXT("colliding with nothing"), PieceMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& PieceMesh->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore
		&& PieceMesh->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore);
	PieceMesh->SetStaticMesh(Cube);
	TestFalse(TEXT("the look goes through it"), LookFinds(World, Piece));
	TestTrue(TEXT("its glow"), Piece->GetParticleSystem()->GetAttachParent() == Piece->GetRootComponent());
	const UPointLightComponent* Light = Piece->GetPointLight();
	TestTrue(TEXT("a violet light"), Light->LightColor == FColor(169, 78, 232, 255) && Light->Intensity == 500.f
		&& Light->IntensityUnits == ELightUnits::Unitless && Light->AttenuationRadius == 500.f);
	TestTrue(TEXT("drawn to 3000 cm, fading over 2000"), Light->MaxDrawDistance == 3000.f && Light->MaxDistanceFadeRange == 2000.f);
	TestFalse(TEXT("casting no shadow"), Light->CastShadows);
	IWasamiInteractable::Execute_InteractWithObject(Piece, nullptr);
	TestTrue(TEXT("clicking it does nothing"), IsValid(Piece) && !Piece->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiRingStatueInteractTest, "Wasami.RingStatue.Interact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiRingStatueInteractTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiRingStatue* Statue = World->SpawnActor<AWasamiRingStatue>(FVector::ZeroVector, FRotator::ZeroRotator);
	AWasamiShard* Shard = World->SpawnActor<AWasamiShard>(FVector(2000., 0., 0.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the altar"), Statue) || !TestNotNull(TEXT("a shard"), Shard))
	{
		return false;
	}
	// Held against the garbage collection the ticks below run.
	TStrongObjectPtr<UWasamiTestListener> Listener(NewObject<UWasamiTestListener>());
	Statue->OnInteractAllShards.AddDynamic(Listener.Get(), &UWasamiTestListener::Hear);

	// A shard left: the player turned away, at most once every 5 s.
	IWasamiInteractable::Execute_InteractWithObject(Statue, nullptr);
	UWasamiTextPromptWidget* First = Statue->GetLastPrompt();
	if (!TestNotNull(TEXT("a prompt"), First))
	{
		return false;
	}
	TestEqual(TEXT("saying to collect the shards"), First->Text.ToString(),
		FString(TEXT("Collect all soul shards to break the ring barrier.")));
	TestEqual(TEXT("not Interact All Shards"), Listener->Count, 0);
	IWasamiInteractable::Execute_InteractWithObject(Statue, nullptr);
	TestTrue(TEXT("clicked again at once: no second prompt"), Statue->GetLastPrompt() == First);
	AdvanceStatueWorld(Wrapper, 4.8f);
	IWasamiInteractable::Execute_InteractWithObject(Statue, nullptr);
	TestTrue(TEXT("nor at 4.8 s"), Statue->GetLastPrompt() == First);

	// Every shard collected: Interact All Shards once, and the hand gone for good.
	Shard->Destroy();
	AdvanceStatueWorld(Wrapper, 0.3f);
	IWasamiInteractable::Execute_InteractWithObject(Statue, nullptr);
	TestEqual(TEXT("Interact All Shards"), Listener->Count, 1);
	TestTrue(TEXT("and no prompt"), Statue->GetLastPrompt() == First || Statue->GetLastPrompt() == nullptr);
	TestEqual(TEXT("the mesh's tags cleared"), Statue->GetStaticMeshComponent()->ComponentTags.Num(), 0);
	AdvanceStatueWorld(Wrapper, 5.5f);
	IWasamiInteractable::Execute_InteractWithObject(Statue, nullptr);
	TestEqual(TEXT("never again"), Listener->Count, 1);
	return true;
}

#endif
