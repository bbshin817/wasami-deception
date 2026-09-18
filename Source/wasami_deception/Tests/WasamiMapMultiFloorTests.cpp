#include "Misc/AutomationTest.h"
#include "../WasamiMapArea.h"
#include "../WasamiMapTextureMultiFloor.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiShard.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Seconds of play in ticks of 0.1 s at most. */
	void AdvanceMapWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 1e-4f; Left -= 0.1f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.1f));
		}
	}

	/** The texture the map's material has now. */
	UTexture* MapTexture(const AWasamiMapTextureMultiFloor* Map)
	{
		UTexture* Texture = nullptr;
		if (Map->GetDynamicMat())
		{
			Map->GetDynamicMat()->GetTextureParameterValue(FHashedMaterialParameterInfo(AWasamiMapTextureMultiFloor::TextureParameter), Texture);
		}
		return Texture;
	}

	bool MarkShown(const AWasamiShard* Shard)
	{
		return Shard->GetPlane()->IsVisible();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMapMultiFloorTest, "Wasami.MapMultiFloor.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMapMultiFloorTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* ZoneMap = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DD/UI/Minimap/MM_Map_06_Zone2.MM_Map_06_Zone2"));
	UTexture* Floor1 = LoadObject<UTexture>(nullptr, TEXT("/Game/DD/UI/Minimap/T_06_Zone2.T_06_Zone2"));
	UTexture* Floor2 = LoadObject<UTexture>(nullptr, TEXT("/Game/DD/UI/Minimap/T_06_Zone2_02.T_06_Zone2_02"));
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!TestNotNull(TEXT("MM_Map_06_Zone2 (WasamiDDTools.import_dd_tablet makes it)"), ZoneMap)
		|| !TestNotNull(TEXT("T_06_Zone2"), Floor1) || !TestNotNull(TEXT("T_06_Zone2_02"), Floor2) || !TestNotNull(TEXT("the plane"), PlaneMesh))
	{
		return false;
	}

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();

	// Two floors' boxes, 3200 × 3200 × 320 cm each way, one over the other (Zone 2's are 800 cm apart, as here).
	const FVector BoxScale(100., 100., 10.);
	AWasamiMapArea* Lower = World->SpawnActor<AWasamiMapArea>(AWasamiMapArea::StaticClass(), FTransform(FRotator::ZeroRotator, FVector::ZeroVector, BoxScale));
	AWasamiMapArea* Upper = World->SpawnActor<AWasamiMapArea>(AWasamiMapArea::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0., 0., 800.), BoxScale));
	AWasamiShard* LowerShard = World->SpawnActor<AWasamiShard>(FVector(300., 0., 50.), FRotator::ZeroRotator);
	AWasamiShard* UpperShard = World->SpawnActor<AWasamiShard>(FVector(-300., 0., 850.), FRotator::ZeroRotator);
	AWasamiShard* Elsewhere = World->SpawnActor<AWasamiShard>(FVector(9000., 0., 50.), FRotator::ZeroRotator);
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector(0., 0., 100.), FRotator::ZeroRotator);
	// The first player controller's character without possessing it: it stays where it is put.
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the boxes"), Lower) || !TestNotNull(TEXT("the upper box"), Upper) || !TestNotNull(TEXT("the shards"), LowerShard)
		|| !TestNotNull(TEXT("the upper shard"), UpperShard) || !TestNotNull(TEXT("the far shard"), Elsewhere)
		|| !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	if (!TestTrue(TEXT("the player is the player"), UGameplayStatics::GetPlayerCharacter(World, 0) == Player))
	{
		return false;
	}

	// BP_MapArea's box: the root, overlapping world static, world dynamic and pawns only.
	const UBoxComponent* Box = Lower->GetBox();
	TestTrue(TEXT("the box is the root"), Lower->GetRootComponent() == Box);
	TestTrue(TEXT("32 cm scaled"), Box->GetScaledBoxExtent().Equals(FVector(3200., 3200., 320.), 0.01));
	TestTrue(TEXT("it overlaps world static and dynamic and pawns"), Box->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Overlap
		&& Box->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Overlap && Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
	TestTrue(TEXT("and ignores the rest"), Box->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore
		&& Box->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore && Box->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Ignore);
	TArray<AActor*> InBox;
	Lower->GetShards(InBox);
	TestTrue(TEXT("Get Shards: the lower box's shard only"), InBox.Num() == 1 && InBox.Contains(LowerShard));

	// The plane, as the level build puts it.
	AWasamiMapTextureMultiFloor* Map = World->SpawnActorDeferred<AWasamiMapTextureMultiFloor>(AWasamiMapTextureMultiFloor::StaticClass(),
		FTransform(FVector(0., 0., -1900.)));
	if (!TestNotNull(TEXT("the map"), Map))
	{
		return false;
	}
	Map->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
	Map->GetStaticMeshComponent()->SetMaterial(0, ZoneMap);
	Map->Map.Add(Lower, Floor1);
	Map->Map.Add(Upper, Floor2);
	UGameplayStatics::FinishSpawningActor(Map, FTransform(FVector(0., 0., -1900.)));
	TestTrue(TEXT("movable"), Map->GetStaticMeshComponent()->Mobility == EComponentMobility::Movable);
	TestTrue(TEXT("its material made anew of the zone's map"), Map->GetDynamicMat() && Map->GetStaticMeshComponent()->GetMaterial(0) == Map->GetDynamicMat()
		&& Map->GetDynamicMat()->Parent == ZoneMap);

	// Before the first Check Map: every mark shown, the first floor's map.
	AdvanceMapWorld(Wrapper, 0.5f);
	TestNull(TEXT("no box yet"), Map->GetCurrentMapArea());
	TestTrue(TEXT("every mark shown"), MarkShown(LowerShard) && MarkShown(UpperShard) && MarkShown(Elsewhere));
	TestTrue(TEXT("the zone's own map"), MapTexture(Map) == Floor1);

	// At 0.9 s: the player's box, its map and its shard's mark.
	AdvanceMapWorld(Wrapper, 0.5f);
	TestTrue(TEXT("the lower box"), Map->GetCurrentMapArea() == Lower);
	TestTrue(TEXT("the first floor's map"), MapTexture(Map) == Floor1);
	TestTrue(TEXT("its shard shown"), MarkShown(LowerShard));
	TestFalse(TEXT("the upper floor's hidden"), MarkShown(UpperShard));
	TestFalse(TEXT("one out of both boxes hidden"), MarkShown(Elsewhere));

	// Up a floor: the next Check Map changes both.
	Player->SetActorLocation(FVector(0., 0., 900.), false, nullptr, ETeleportType::TeleportPhysics);
	AdvanceMapWorld(Wrapper, 0.3f);
	TestTrue(TEXT("not before the timer"), Map->GetCurrentMapArea() == Lower && MapTexture(Map) == Floor1);
	AdvanceMapWorld(Wrapper, 0.7f);
	TestTrue(TEXT("the upper box"), Map->GetCurrentMapArea() == Upper);
	TestTrue(TEXT("the second floor's map"), MapTexture(Map) == Floor2);
	TestTrue(TEXT("the upper shard shown, the lower hidden"), MarkShown(UpperShard) && !MarkShown(LowerShard) && !MarkShown(Elsewhere));

	// A collected shard is gone from the list.
	UpperShard->Destroy();
	AdvanceMapWorld(Wrapper, 0.9f);
	TestTrue(TEXT("still the upper box"), Map->GetCurrentMapArea() == Upper && !MarkShown(LowerShard));

	// Out of both boxes: left as it was.
	Player->SetActorLocation(FVector(0., 0., 5000.), false, nullptr, ETeleportType::TeleportPhysics);
	AdvanceMapWorld(Wrapper, 1.f);
	TestTrue(TEXT("out of the boxes, left as it was"), Map->GetCurrentMapArea() == Upper && MapTexture(Map) == Floor2 && !MarkShown(LowerShard));

	// Back down.
	Player->SetActorLocation(FVector(0., 0., 100.), false, nullptr, ETeleportType::TeleportPhysics);
	AdvanceMapWorld(Wrapper, 1.f);
	TestTrue(TEXT("down again"), Map->GetCurrentMapArea() == Lower && MapTexture(Map) == Floor1 && MarkShown(LowerShard));
	return true;
}

#endif
