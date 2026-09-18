#include "Misc/AutomationTest.h"
#include "../WasamiArrowPointer.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiShard.h"
#include "../WasamiZoneShardChecker.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Seconds of play in ticks of 0.1 s at most (as the zone flow's tests). */
	void AdvanceArrowWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 1e-4f; Left -= 0.1f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.1f));
		}
	}

	/** A zone shard checker over the origin, its box 3200 × 3200 × 320 cm each way. */
	AWasamiZoneShardChecker* SpawnChecker(UWorld* World)
	{
		return World->SpawnActor<AWasamiZoneShardChecker>(AWasamiZoneShardChecker::StaticClass(),
			FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(100., 100., 10.)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiArrowPointerTest, "Wasami.ArrowPointer.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiArrowPointerTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	// No controller: the player stays where it is put (its movement runs only when controlled).
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player))
	{
		return false;
	}
	AWasamiArrowPointer* Arrow = Player->GetArrowPointer();
	if (!TestNotNull(TEXT("the player carries the arrow"), Arrow))
	{
		return false;
	}

	// BP_DD_PlayerCharacter's child actor and BP_ArrowPointer's plane.
	const UChildActorComponent* Child = Player->FindComponentByClass<UChildActorComponent>();
	TestTrue(TEXT("on the mesh"), Child && Child->GetAttachParent() == Player->GetMesh());
	TestTrue(TEXT("2000 cm over it"), Child && Child->GetRelativeLocation().Equals(FVector(0., 0., 2000.)));
	TestTrue(TEXT("(5, 5, 1)"), Child && Child->GetRelativeScale3D().Equals(FVector(5., 5., 1.)));
	UStaticMeshComponent* Plane = Arrow->GetPlane();
	TestTrue(TEXT("the engine's Plane"), Plane->GetStaticMesh() && Plane->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Plane.Plane"));
	TestEqual(TEXT("turned 90°"), Plane->GetRelativeRotation().Yaw, 90.00023651123047, 1e-4);
	TestTrue(TEXT("1.5 times"), Plane->GetRelativeScale3D().Equals(FVector(1.5, 1.5, 1.)));
	TestFalse(TEXT("no shadow"), Plane->CastShadow);
	TestTrue(TEXT("M_Arrow_Inst"), Plane->GetMaterial(0) && Plane->GetMaterial(0)->GetName() == TEXT("M_Arrow_Inst"));
	TestTrue(TEXT("Shards? to begin with"), Arrow->bShards);

	// The map's capture draws it.
	AdvanceArrowWorld(Wrapper, 0.2f);
	const USceneCaptureComponent2D* Capture = Player->FindComponentByClass<USceneCaptureComponent2D>();
	TestTrue(TEXT("on the map's Show Only"), Capture && Capture->ShowOnlyActors.Contains(Arrow));

	// Out of every zone shard checker's box: hidden.
	Plane->SetVisibility(true);
	Arrow->FindObject();
	TestFalse(TEXT("hidden out of the zone"), Plane->IsVisible());

	// In the zone, fewer than 100 shards left: at the nearest.
	AWasamiZoneShardChecker* Checker = SpawnChecker(World);
	AWasamiShard* Far = World->SpawnActor<AWasamiShard>(FVector(1000., 1000., 0.), FRotator::ZeroRotator);
	AWasamiShard* Near = World->SpawnActor<AWasamiShard>(FVector(-300., 0., 0.), FRotator::ZeroRotator);
	AWasamiShard* Middle = World->SpawnActor<AWasamiShard>(FVector(500., 0., 0.), FRotator::ZeroRotator);
	AWasamiShard* Outside = World->SpawnActor<AWasamiShard>(FVector(-250., 0., 5000.), FRotator::ZeroRotator);
	TArray<AActor*> InBox;
	Checker->GetShards(InBox);
	TestEqual(TEXT("three shards in the box"), InBox.Num(), 3);
	TestFalse(TEXT("not the one above it"), InBox.Contains(Outside));
	Arrow->FindObject();
	TestTrue(TEXT("shown in the zone"), Plane->IsVisible());
	TestTrue(TEXT("at the nearest shard in the box"), Arrow->GetTarget() == Near);
	TestTrue(TEXT("FindClosestShard"), Arrow->FindClosestShard() == Near);

	// Set Rotation and Smooth Rotation: it turns to the shard (−X) and grows to 7 − 300 / 3000 × 3 = 6.7.
	AdvanceArrowWorld(Wrapper, 1.f);
	TestEqual(TEXT("pointing at it"), FMath::Abs(Arrow->GetTargetRotation().Yaw), 180., 1e-3);
	TestEqual(TEXT("turned there"), FMath::Abs(Arrow->GetActorRotation().Yaw), 180., 0.01);
	TestEqual(TEXT("level"), Arrow->GetActorRotation().Pitch, 0., 1e-3);
	TestEqual(TEXT("6.7 across"), Arrow->GetActorScale3D().X, 6.7, 0.01);
	TestEqual(TEXT("as wide"), Arrow->GetActorScale3D().Y, Arrow->GetActorScale3D().X, 1e-6);
	TestEqual(TEXT("1 high"), Arrow->GetActorScale3D().Z, 1., 1e-6);

	// Collected: the next nearest.
	Near->Destroy();
	Arrow->FindObject();
	TestTrue(TEXT("then the next nearest"), Arrow->GetTarget() == Middle);
	AdvanceArrowWorld(Wrapper, 1.f);
	TestEqual(TEXT("turned to it"), Arrow->GetActorRotation().Yaw, 0., 0.01);

	// 100 or more left: hidden.
	TArray<AWasamiShard*> Many;
	for (int32 Index = 0; Index < 98; ++Index)
	{
		Many.Add(World->SpawnActor<AWasamiShard>(FVector(-2000. + 40. * Index, -2000., 0.), FRotator::ZeroRotator));
	}
	Arrow->FindObject();
	TestFalse(TEXT("hidden with 100 left"), Plane->IsVisible());
	Many.Pop()->Destroy();
	Arrow->FindObject();
	TestTrue(TEXT("shown with 99"), Plane->IsVisible());
	for (AWasamiShard* Shard : Many)
	{
		Shard->Destroy();
	}

	// None left in the box: shown still, at what it had.
	Middle->Destroy();
	Far->Destroy();
	Arrow->FindObject();
	TestTrue(TEXT("shown with none left"), Plane->IsVisible());

	// Without Shards?: shown while the zone's target is there.
	Arrow->bShards = false;
	Arrow->SetTarget(Outside);
	Arrow->FindObject();
	TestTrue(TEXT("shown at the target"), Plane->IsVisible());
	Arrow->SetTarget(nullptr);
	Arrow->FindObject();
	TestFalse(TEXT("hidden without one"), Plane->IsVisible());

	// Change Color: the materials' Color.
	Arrow->ChangeColor(FLinearColor(1.f, 0.8002f, 0.f, 1.f));
	const UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Plane->GetMaterial(0));
	FLinearColor Color;
	TestTrue(TEXT("coloured"), Dynamic && Dynamic->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color)
		&& Color.Equals(FLinearColor(1.f, 0.8002f, 0.f, 1.f), 1e-4f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneShardCheckerTest, "Wasami.ArrowPointer.ZoneShardChecker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneShardCheckerTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiZoneShardChecker* Checker = SpawnChecker(World);
	if (!TestNotNull(TEXT("the checker"), Checker))
	{
		return false;
	}
	// UBoxComponent's defaults, as BP_ZoneShardChecker's box.
	const UBoxComponent* Box = Checker->GetBox();
	TestTrue(TEXT("32 cm"), Box->GetUnscaledBoxExtent().Equals(FVector(32.)));
	TestEqual(TEXT("OverlapAllDynamic"), Box->GetCollisionProfileName(), FName(TEXT("OverlapAllDynamic")));

	AWasamiShard* Shard = World->SpawnActor<AWasamiShard>(FVector(100., 0., 0.), FRotator::ZeroRotator);
	Checker->CheckShards();
	AdvanceArrowWorld(Wrapper, 1.1f);
	TestTrue(TEXT("stays while a shard is left"), IsValid(Checker));

	// Check Shards is a Delay: a second call while one waits does not put it back.
	Shard->Destroy();
	Checker->CheckShards();
	AdvanceArrowWorld(Wrapper, 0.6f);
	Checker->CheckShards();
	AdvanceArrowWorld(Wrapper, 0.3f);
	TestTrue(TEXT("waits its second"), IsValid(Checker));
	AdvanceArrowWorld(Wrapper, 0.2f);
	TestFalse(TEXT("gone a second after, with none left"), IsValid(Checker));
	return true;
}

#endif
