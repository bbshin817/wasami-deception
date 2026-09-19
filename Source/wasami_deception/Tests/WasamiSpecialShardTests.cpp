#include "Misc/AutomationTest.h"
#include "../WasamiBonusShard.h"
#include "../WasamiBonusShardCollectEffect.h"
#include "../WasamiEnemy.h"
#include "../WasamiGameMode.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiPowerOrb.h"
#include "../WasamiPrimalPower.h"
#include "../WasamiSaveGame.h"
#include "../WasamiSpecialSpawnPoint.h"
#include "../WasamiStunCollectEffect.h"
#include "../WasamiTelekinesisInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Tests/AutomationCommon.h"
#include "WasamiTestEnemy.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The ticks' length. */
	constexpr float OrbStep = 0.05f;

	// Where the orb is placed (off the map, as in the levels) and its spawn points.
	const FVector OrbPlaced(0., 0., 5725.);
	const FVector OrbPoints[] = {FVector(1000., 0., 0.), FVector(-2000., 500., 0.), FVector(0., 3000., 400.)};
	const FVector OrbPlayerAway(8000., 8000., 100.);

	void AdvanceOrb(FTestWorldWrapper& Wrapper, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f; Left -= OrbStep)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, OrbStep));
		}
	}

	/** The player: possessed by the first player controller, held where it is put (moved by teleports). */
	ACharacter* SpawnOrbPlayer(UWorld* World)
	{
		ACharacter* Player = World->SpawnActor<ACharacter>(OrbPlayerAway, FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Controller)
		{
			return nullptr;
		}
		Controller->Possess(Player);
		Player->GetCharacterMovement()->DisableMovement();
		return UGameplayStatics::GetPlayerCharacter(World, 0) == Player ? Player : nullptr;
	}

	void OrbTeleport(AActor* Actor, const FVector& Where)
	{
		Actor->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
	}

	bool OrbAtOnePoint(const AActor* Orb)
	{
		for (const FVector& Point : OrbPoints)
		{
			if (Orb->GetActorLocation().Equals(Point, 1e-3))
			{
				return true;
			}
		}
		return false;
	}

	template <typename T>
	TArray<T*> OrbActorsOf(UWorld* World)
	{
		TArray<T*> Found;
		for (TActorIterator<T> It(World); It; ++It)
		{
			Found.Add(*It);
		}
		return Found;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPowerOrbPartsTest, "Wasami.PowerOrb.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPowerOrbPartsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiPowerOrb* Orb = World->SpawnActor<AWasamiPowerOrb>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the orb"), Orb))
	{
		return false;
	}

	// BP_PowerOrb's crystal: power_orb in m_crystal_Inst3, 125.25 cm up at 0.5408, touching nothing.
	const UStaticMeshComponent* Crystal = Orb->GetCrystal();
	TestEqual(TEXT("the crystal's mesh"), Crystal->GetStaticMesh() ? Crystal->GetStaticMesh()->GetPathName() : FString(),
		FString(TEXT("/Game/DD/Meshes/Shared/power_orb.power_orb")));
	TestEqual(TEXT("the crystal's material"), Crystal->GetMaterial(0) ? Crystal->GetMaterial(0)->GetPathName() : FString(),
		FString(TEXT("/Game/DD/Materials/Fords_Materials/m_crystal_Inst3.m_crystal_Inst3")));
	TestEqual(TEXT("the crystal 125.25 cm up"), Crystal->GetComponentLocation().Z, 125.25003051757812, 1e-3);
	TestEqual(TEXT("the crystal's scale"), Crystal->GetComponentScale().X, 0.5407835245132446, 1e-6);
	TestEqual(TEXT("the crystal collides with nothing"), static_cast<int32>(Crystal->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));

	// The capsule under the crystal: 840.6 at 0.1 × 0.5408, a ball of 45.46 cm; OverlapAllDynamic.
	const UCapsuleComponent* Capsule = Orb->GetCapsule();
	TestEqual(TEXT("a ball of 45.46 cm"), Capsule->GetScaledCapsuleRadius(), 840.6161499023438f * 0.05407835245132446f, 1e-3f);
	TestEqual(TEXT("its half height"), Capsule->GetScaledCapsuleHalfHeight(), 840.6161499023438f * 0.05407835245132446f, 1e-3f);
	TestEqual(TEXT("its centre"), Capsule->GetComponentLocation().Z, 125.25003051757812 + 0.2914627194404602 * 0.5407835245132446, 1e-3);
	TestEqual(TEXT("OverlapAllDynamic"), Capsule->GetCollisionProfileName(), FName(TEXT("OverlapAllDynamic")));
	TestEqual(TEXT("overlapping pawns"), static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_Pawn)), static_cast<int32>(ECR_Overlap));
	TestTrue(TEXT("with overlap events"), Capsule->GetGenerateOverlapEvents());

	// The light: orange, 1000 without units, 200 cm, no shadows.
	const UPointLightComponent* Light = Orb->GetLight();
	TestTrue(TEXT("the light's colour"), Light->LightColor == FColor(255, 146, 0, 255));
	TestEqual(TEXT("the light's intensity"), Light->Intensity, 1000.f);
	TestTrue(TEXT("in no units"), Light->IntensityUnits == ELightUnits::Unitless);
	TestEqual(TEXT("the light's radius"), Light->AttenuationRadius, 200.f);
	TestFalse(TEXT("no shadows"), Light->CastShadows);

	// The map's mark: the plane in M_PowerOrb 20 m up at (1.5, 1.5, 10), no shadow.
	const UStaticMeshComponent* Mark = Orb->GetMapMark();
	TestEqual(TEXT("the mark's material"), Mark->GetMaterial(0) ? Mark->GetMaterial(0)->GetPathName() : FString(),
		FString(TEXT("/Game/DD/Materials/Shared/M_PowerOrb.M_PowerOrb")));
	TestEqual(TEXT("the mark 20 m up"), Mark->GetComponentLocation().Z, 2000., 1e-3);
	TestTrue(TEXT("the mark's scale"), Mark->GetComponentScale().Equals(FVector(1.5, 1.5, 10.), 1e-6));
	TestFalse(TEXT("the mark casts no shadow"), Mark->CastShadow);
	TestEqual(TEXT("the mark collides with nothing"), static_cast<int32>(Mark->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));

	TestEqual(TEXT("Shard Spawn Time"), Orb->ShardSpawnTime, 150.f);
	TestFalse(TEXT("the telekinesis does not pull it"), Orb->Implements<UWasamiTelekinesisInterface>());
	TestTrue(TEXT("the minimap shows it"), GetDefault<AWasamiPlayerCharacter>()->MinimapActorClasses.Contains(AWasamiPowerOrb::StaticClass()));

	// The spawn points: a 32 cm billboard, hidden in game.
	const AWasamiPowerOrbSpawnPoint* Point = World->SpawnActor<AWasamiPowerOrbSpawnPoint>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("an orb spawn point"), Point))
	{
		const UMaterialBillboardComponent* Billboard = Point->GetBillboard();
		TestTrue(TEXT("the billboard is the root"), Point->GetRootComponent() == Billboard);
		TestTrue(TEXT("hidden in game"), Billboard->bHiddenInGame);
		TestTrue(TEXT("one element of 32 cm in m_crystal_Inst3"), Billboard->Elements.Num() == 1
			&& Billboard->Elements[0].BaseSizeX == 32.f && Billboard->Elements[0].BaseSizeY == 32.f
			&& !Billboard->Elements[0].bSizeIsInScreenSpace && Billboard->Elements[0].Material
			&& Billboard->Elements[0].Material->GetName() == TEXT("m_crystal_Inst3"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiStunCollectEffectTest, "Wasami.PowerOrb.CollectEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiStunCollectEffectTest::RunTest(const FString& Parameters)
{
	// BP_StunCollectEffect's tracks (Bezier over the exported tangents); float2 is Primal Fear's.
	struct FRow
	{
		float Time, Growth, Fade, Desaturation, Opacity;
	};
	const FRow Rows[] = {
		{0.f, 0.f, -0.000698f, 0.015488f, 1.f},
		{0.2f, 0.100715f, 0.132451f, 0.038253f, 0.935615f},
		{0.5f, 0.262702f, 1.f, 0.089868f, 0.782464f},
		{1.f, 0.543513f, 1.f, 0.164949f, 0.465993f},
		{1.5f, 0.808173f, 1.f, 0.231473f, 0.127908f},
		{2.f, 1.022151f, 1.f, 0.254143f, -0.002317f},
	};
	for (const FRow& Row : Rows)
	{
		const FString At = FString::Printf(TEXT(" at %.1f s"), Row.Time);
		TestEqual(TEXT("float") + At, AWasamiStunCollectEffect::GrowthCurve().Eval(Row.Time), Row.Growth, 1e-5f);
		TestEqual(TEXT("float2") + At, AWasamiPrimalPower::PrimalFadeCurve().Eval(Row.Time), Row.Fade, 1e-5f);
		TestEqual(TEXT("desaturation") + At, AWasamiStunCollectEffect::DesaturationCurve().Eval(Row.Time), Row.Desaturation, 1e-5f);
		TestEqual(TEXT("opacity") + At, AWasamiStunCollectEffect::OpacityCurve().Eval(Row.Time), Row.Opacity, 1e-5f);
	}

	// The class's values: an orange tint, Primal's flash, a 40 m Range, the sphere's colour and the wave at pitch 2.
	const AWasamiStunCollectEffect* Defaults = GetDefault<AWasamiStunCollectEffect>();
	const UPostProcessComponent* Tint = Defaults->GetTint();
	const UPostProcessComponent* Flash = Defaults->GetFlash();
	TestTrue(TEXT("the tint washes out the colour"), Tint->Settings.bOverride_ColorSaturation && Tint->Settings.ColorSaturation == FVector4(0., 0., 0., 1.));
	TestTrue(TEXT("the tint's orange gain"), Tint->Settings.bOverride_ColorGain && Tint->Settings.ColorGain.Equals(FVector4(1.61, 0.941551, 0., 1.), 1e-6));
	TestTrue(TEXT("the flash's midtones"), Flash->Settings.bOverride_ColorGainMidtones && Flash->Settings.ColorGainMidtones == FVector4(100., 100., 100., 1.));
	TestTrue(TEXT("the flash's fringe"), Flash->Settings.bOverride_SceneFringeIntensity && Flash->Settings.SceneFringeIntensity == 50.f);
	TestTrue(TEXT("both start at weight 0"), Tint->BlendWeight == 0.f && Flash->BlendWeight == 0.f);
	TestEqual(TEXT("Range"), Defaults->Range, 4000.f);
	TestEqual(TEXT("the wave's pitch"), Defaults->GetWavePitch(), 2.f);
	TestTrue(TEXT("the sphere's colour"), Defaults->GetSphereColor().IsSet() && Defaults->GetSphereColor().GetValue().Equals(FLinearColor(0.258f, 0.0737f, 0.f, 1.f)));
	TestTrue(TEXT("the sphere collides with nothing"), Defaults->GetSphere()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Primal Fear sets no colour"), GetDefault<AWasamiPrimalPower>()->GetSphereColor().IsSet());
	TestEqual(TEXT("Primal Fear's wave at pitch 1"), GetDefault<AWasamiPrimalPower>()->GetWavePitch(), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPowerOrbCycleTest, "Wasami.PowerOrb.Cycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPowerOrbCycleTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	for (const FVector& Point : OrbPoints)
	{
		World->SpawnActor<AWasamiPowerOrbSpawnPoint>(Point, FRotator::ZeroRotator);
	}
	// A bonus shard's point is not the orb's.
	World->SpawnActor<AWasamiBonusShardSpawnPoint>(FVector(5000., 0., 0.), FRotator::ZeroRotator);
	AWasamiPowerOrb* Orb = World->SpawnActor<AWasamiPowerOrb>(OrbPlaced, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the orb"), Orb))
	{
		return false;
	}
	TestEqual(TEXT("the level's three orb spawn points"), Orb->GetSpawnPoints().Num(), 3);
	TestEqual(TEXT("150 s to the first flicker"), Orb->GetTimeToSpawn(), 150.f, 1e-3f);
	USceneComponent* Root = Orb->GetRootComponent();

	// Nothing until 150 s; then 5 s of flicker where it was placed. (Ticks of 0.05 s fall behind the clock a little
	// over 150 s, so each look is half a second off the timers.)
	AdvanceOrb(Wrapper, 150.f - 0.5f);
	TestFalse(TEXT("not flickering before 150 s"), Orb->IsFlickering());
	TestTrue(TEXT("seen before"), Root->IsVisible() && Orb->GetCrystal()->IsVisible());
	AdvanceOrb(Wrapper, 1.f);
	TestTrue(TEXT("flickering after 150 s"), Orb->IsFlickering());
	TestTrue(TEXT("still where it was placed"), Orb->GetActorLocation().Equals(OrbPlaced, 1e-3));
	TestEqual(TEXT("no spawn timer while it flickers"), Orb->GetTimeToSpawn(), -1.f);

	// The root with everything under it turns over every 0.1 s; the capsule stays to be touched.
	int32 Turns = 0;
	bool bWasVisible = Root->IsVisible();
	bool bLightFollows = true;
	bool bCapsuleStays = true;
	for (float Left = 3.f; Left > 1e-4f; Left -= OrbStep)
	{
		Wrapper.TickTestWorld(OrbStep);
		if (Root->IsVisible() != bWasVisible)
		{
			++Turns;
			bWasVisible = Root->IsVisible();
		}
		bLightFollows &= Orb->GetLight()->IsVisible() == Root->IsVisible() && Orb->GetMapMark()->IsVisible() == Root->IsVisible();
		bCapsuleStays &= Orb->GetCapsule()->IsCollisionEnabled();
	}
	TestTrue(FString::Printf(TEXT("about 30 turns in 3 s (%d)"), Turns), Turns >= 28 && Turns <= 31);
	TestTrue(TEXT("the light and the mark go with the crystal"), bLightFollows);
	TestTrue(TEXT("the capsule can be touched throughout"), bCapsuleStays);

	// After 155 s: seen, at one of the spawn points, 150 s to the next from the move.
	AdvanceOrb(Wrapper, 2.f);
	TestFalse(TEXT("the flicker is over"), Orb->IsFlickering());
	TestTrue(TEXT("seen after"), Root->IsVisible() && Orb->GetCrystal()->IsVisible() && Orb->GetLight()->IsVisible());
	TestTrue(TEXT("at a spawn point"), OrbAtOnePoint(Orb));
	TestTrue(FString::Printf(TEXT("150 s to the next from the move (%.3f)"), Orb->GetTimeToSpawn()),
		FMath::IsNearlyEqual(Orb->GetTimeToSpawn(), 149.5f, 0.2f));

	// It goes on every 155 s, always to a point.
	for (int32 Round = 0; Round < 2; ++Round)
	{
		AdvanceOrb(Wrapper, 155.f);
		TestTrue(FString::Printf(TEXT("round %d: at a spawn point"), Round + 2), OrbAtOnePoint(Orb) && !Orb->IsFlickering());
	}

	// The debug move: to the point asked for, at once.
	Orb->MoveToSpawnPoint(1);
	TestTrue(TEXT("moved to point 1"), Orb->GetActorLocation().Equals(Orb->GetSpawnPoints()[1]->GetActorLocation(), 1e-3));
	TestEqual(TEXT("150 s from the move"), Orb->GetTimeToSpawn(), 150.f, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPowerOrbCollectTest, "Wasami.PowerOrb.Collect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPowerOrbCollectTest::RunTest(const FString& Parameters)
{
	// The test world has no local player, so ENEMIES STUNNED is not put on a screen.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 0);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	World->SpawnActor<AWasamiPowerOrbSpawnPoint>(OrbPoints[0], FRotator::ZeroRotator);
	AWasamiPowerOrb* Orb = World->SpawnActor<AWasamiPowerOrb>(OrbPlaced, FRotator::ZeroRotator);
	ACharacter* Player = SpawnOrbPlayer(World);
	if (!TestNotNull(TEXT("the orb"), Orb) || !TestNotNull(TEXT("the player"), Player))
	{
		return false;
	}
	// Enemies anywhere in the level, far from the orb; one with the tag but not the interface.
	AWasamiTestEnemy* Near = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(1500., 0., 100.));
	AWasamiTestEnemy* Far = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(-30000., 20000., 5000.));
	AActor* TagOnly = World->SpawnActor<AActor>();
	TagOnly->Tags.Add(TEXT("Enemy"));

	// Another character touching it takes nothing.
	ACharacter* Other = World->SpawnActor<ACharacter>(OrbPlayerAway + FVector(0., 500., 0.), FRotator::ZeroRotator);
	Other->GetCharacterMovement()->DisableMovement();
	Orb->MoveToSpawnPoint(0);
	OrbTeleport(Other, Orb->GetCapsule()->GetComponentLocation());
	AdvanceOrb(Wrapper, OrbStep);
	TestTrue(TEXT("another character takes nothing"), IsValid(Orb) && Near->SetStateCount == 0);
	OrbTeleport(Other, OrbPlayerAway + FVector(0., 500., 0.));

	// Flickered out, it is still there to be taken.
	Orb->SpawnSpecialShard();
	AdvanceOrb(Wrapper, OrbStep);
	TestFalse(TEXT("flickered out"), Orb->GetRootComponent()->IsVisible());
	OrbTeleport(Player, Orb->GetCapsule()->GetComponentLocation());
	AdvanceOrb(Wrapper, OrbStep);
	TestFalse(TEXT("taken and gone"), IsValid(Orb));
	for (const AWasamiTestEnemy* Each : {Near, Far})
	{
		TestEqual(TEXT("one Set State"), Each->SetStateCount, 1);
		TestTrue(TEXT("Stun"), Each->State == EWasamiEnemyState::Stun);
		TestTrue(TEXT("by the orb"), Each->bLastByOrb);
	}
	const TArray<AWasamiStunCollectEffect*> Effects = OrbActorsOf<AWasamiStunCollectEffect>(World);
	if (TestEqual(TEXT("one collect effect"), Effects.Num(), 1))
	{
		TestTrue(TEXT("on the player"), Effects[0]->GetActorLocation().Equals(Player->GetActorLocation(), 1e-3));
	}

	// The effect is gone after its 2 s; the enemies are sent nothing more.
	AdvanceOrb(Wrapper, AWasamiPowerBurst::TimelineLength + 2.f * OrbStep);
	TestEqual(TEXT("the effect is gone"), OrbActorsOf<AWasamiStunCollectEffect>(World).Num(), 0);
	TestEqual(TEXT("once"), Near->SetStateCount, 1);
	TestEqual(TEXT("StunAllEnemies counts the implementers"), AWasamiPowerOrb::StunAllEnemies(World), 2);
	return true;
}

namespace
{
	/** The player as the bonus shard's reveal needs it: player 0's AWasamiPlayerCharacter, unpossessed (no widgets). */
	AWasamiPlayerCharacter* SpawnBonusPlayer(UWorld* World)
	{
		AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(OrbPlayerAway, FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Controller)
		{
			return nullptr;
		}
		Controller->SetPawn(Player);
		Player->GetCharacterMovement()->DisableMovement();
		return UGameplayStatics::GetPlayerCharacter(World, 0) == Player ? Player : nullptr;
	}

	AWasamiBonusShard* SpawnBonusShard(UWorld* World, int32 ID)
	{
		const FTransform Placed(OrbPlaced);
		AWasamiBonusShard* Bonus = World->SpawnActorDeferred<AWasamiBonusShard>(AWasamiBonusShard::StaticClass(), Placed);
		if (Bonus)
		{
			Bonus->ID = ID;
			Bonus->FinishSpawning(Placed);
		}
		return Bonus;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiBonusShardPartsTest, "Wasami.BonusShard.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiBonusShardPartsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiBonusShard* Bonus = World->SpawnActor<AWasamiBonusShard>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the bonus shard"), Bonus))
	{
		return false;
	}

	// BP_BonusShard's crystal: soul_shard in m_crystal_Inst, 97.09 cm up at 20, touching nothing.
	const UStaticMeshComponent* Crystal = Bonus->GetCrystal();
	TestEqual(TEXT("the crystal's mesh"), Crystal->GetStaticMesh() ? Crystal->GetStaticMesh()->GetPathName() : FString(),
		FString(TEXT("/Game/DD/Meshes/Ring_Assets/soul_shard.soul_shard")));
	TestEqual(TEXT("the crystal's material"), Crystal->GetMaterial(0) ? Crystal->GetMaterial(0)->GetPathName() : FString(),
		FString(TEXT("/Game/DD/Materials/Fords_Materials/m_crystal_Inst.m_crystal_Inst")));
	TestEqual(TEXT("the crystal 97.09 cm up"), Crystal->GetComponentLocation().Z, 97.08537292480469, 1e-3);
	TestEqual(TEXT("the crystal's scale"), Crystal->GetComponentScale().X, 20., 1e-6);
	TestEqual(TEXT("the crystal collides with nothing"), static_cast<int32>(Crystal->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));

	// The capsule: 49.57 at 0.1 × 20, a ball of 99.14 cm; OverlapAllDynamic.
	const UCapsuleComponent* Capsule = Bonus->GetCapsule();
	TestEqual(TEXT("a ball of 99.14 cm"), Capsule->GetScaledCapsuleRadius(), 49.57180404663086f * 2.f, 1e-3f);
	TestEqual(TEXT("its half height"), Capsule->GetScaledCapsuleHalfHeight(), 49.57180404663086f * 2.f, 1e-3f);
	TestEqual(TEXT("its centre"), Capsule->GetComponentLocation().Z, 97.08537292480469 + 0.2914627194404602 * 20., 1e-3);
	TestEqual(TEXT("OverlapAllDynamic"), Capsule->GetCollisionProfileName(), FName(TEXT("OverlapAllDynamic")));

	// The light: red, 1000 without units, 200 cm, no shadows.
	const UPointLightComponent* Light = Bonus->GetLight();
	TestTrue(TEXT("the light's colour"), Light->LightColor == FColor(255, 31, 0, 255));
	TestTrue(TEXT("1000 in no units"), Light->Intensity == 1000.f && Light->IntensityUnits == ELightUnits::Unitless);
	TestEqual(TEXT("the light's radius"), Light->AttenuationRadius, 200.f);
	TestFalse(TEXT("no shadows"), Light->CastShadows);

	// The map's mark: the plane in M_Bonus_Shard 20 m up at (1.5, 1.5, 10), no shadow, no collision.
	const UStaticMeshComponent* Mark = Bonus->GetMapMark();
	TestEqual(TEXT("the mark's material"), Mark->GetMaterial(0) ? Mark->GetMaterial(0)->GetPathName() : FString(),
		FString(TEXT("/Game/DD/Materials/Shared/M_Bonus_Shard.M_Bonus_Shard")));
	TestEqual(TEXT("the mark 20 m up"), Mark->GetComponentLocation().Z, 2000., 1e-3);
	TestTrue(TEXT("the mark's scale"), Mark->GetComponentScale().Equals(FVector(1.5, 1.5, 10.), 1e-6));
	TestFalse(TEXT("the mark casts no shadow"), Mark->CastShadow);
	TestEqual(TEXT("the mark collides with nothing"), static_cast<int32>(Mark->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));

	TestEqual(TEXT("ID 0 by default"), Bonus->ID, 0);
	TestEqual(TEXT("Shard Spawn Time"), Bonus->ShardSpawnTime, 150.f);
	TestFalse(TEXT("the telekinesis does not pull it"), Bonus->Implements<UWasamiTelekinesisInterface>());
	TestTrue(TEXT("the minimap shows it"), GetDefault<AWasamiPlayerCharacter>()->MinimapActorClasses.Contains(AWasamiBonusShard::StaticClass()));

	// BP_BonusShardCollectEffect: a red tint, Primal's flash and fade, the wave at 0.5 and pitch 1.5.
	const AWasamiBonusShardCollectEffect* Effect = GetDefault<AWasamiBonusShardCollectEffect>();
	const UPostProcessComponent* Tint = Effect->GetTint();
	const UPostProcessComponent* Flash = Effect->GetFlash();
	TestTrue(TEXT("the tint washes out the colour"), Tint->Settings.bOverride_ColorSaturation && Tint->Settings.ColorSaturation == FVector4(0., 0., 0., 1.));
	TestTrue(TEXT("the tint's red gain"), Tint->Settings.bOverride_ColorGain && Tint->Settings.ColorGain.Equals(FVector4(1.61, 0., 0.447186, 1.), 1e-6));
	TestTrue(TEXT("the flash's midtones"), Flash->Settings.bOverride_ColorGainMidtones && Flash->Settings.ColorGainMidtones == FVector4(100., 100., 100., 1.));
	TestTrue(TEXT("the flash's fringe"), Flash->Settings.bOverride_SceneFringeIntensity && Flash->Settings.SceneFringeIntensity == 50.f);
	TestTrue(TEXT("both start at weight 0"), Tint->BlendWeight == 0.f && Flash->BlendWeight == 0.f);
	TestTrue(TEXT("the wave at 0.5 and pitch 1.5"), AWasamiBonusShardCollectEffect::WaveVolume == 0.5f && AWasamiBonusShardCollectEffect::WavePitch == 1.5f);

	// The nurse's mark on an enemy: the plane in M_Enemy 10 m over the capsule's centre, 21.9 cm to the front of the
	// nurse's mesh (+X), at (2.52, 2.52, 10); no shadow, no collision.
	AWasamiEnemy* Enemy = AWasamiEnemy::SpawnEnemy(World, FVector(0., 0., 500.), 0.f);
	if (TestNotNull(TEXT("an enemy"), Enemy))
	{
		const UStaticMeshComponent* EnemyMark = Enemy->GetMapMark();
		TestTrue(TEXT("on the capsule"), EnemyMark->GetAttachParent() == Enemy->GetRootComponent());
		TestTrue(TEXT("the engine's Plane"), EnemyMark->GetStaticMesh() && EnemyMark->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Plane.Plane"));
		TestEqual(TEXT("the enemy's mark's material"), EnemyMark->GetMaterial(0) ? EnemyMark->GetMaterial(0)->GetPathName() : FString(),
			FString(TEXT("/Game/DD/Materials/Shared/M_Enemy.M_Enemy")));
		const FVector Offset = EnemyMark->GetComponentLocation() - Enemy->GetActorLocation();
		TestTrue(FString::Printf(TEXT("10 m up, 21.9 cm ahead (%s)"), *Offset.ToString()), Offset.Equals(FVector(21.884, 0., 1000.), 0.01));
		TestTrue(TEXT("its scale"), EnemyMark->GetComponentScale().Equals(FVector(2.5238659381866455, 2.5238659381866455, 10.), 1e-4));
		TestFalse(TEXT("no shadow"), EnemyMark->CastShadow);
		TestEqual(TEXT("no collision"), static_cast<int32>(EnemyMark->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiBonusShardSaveTest, "Wasami.BonusShard.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiBonusShardSaveTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiGameMode* Mode = World->GetAuthGameMode<AWasamiGameMode>();
	if (!TestNotNull(TEXT("the project's game mode"), Mode) || !TestNotNull(TEXT("its save"), Mode->GetSave()))
	{
		return false;
	}
	// Only in memory: nothing here writes the slot.
	Mode->GetSave()->Hospital.BonusShards = {1};

	// A frame after BeginPlay (the Delay 0) the one whose ID the save has is gone; the other waits its 150 s.
	AWasamiBonusShard* Zone1 = SpawnBonusShard(World, 0);
	AWasamiBonusShard* Zone2 = SpawnBonusShard(World, 1);
	if (!TestNotNull(TEXT("ID 0"), Zone1) || !TestNotNull(TEXT("ID 1"), Zone2))
	{
		return false;
	}
	TestTrue(TEXT("both there at first"), IsValid(Zone1) && IsValid(Zone2));
	TestEqual(TEXT("no spawn timer before the Delay 0"), Zone1->GetTimeToSpawn(), -1.f);
	AdvanceOrb(Wrapper, OrbStep);
	TestFalse(TEXT("the saved one is gone"), IsValid(Zone2));
	TestTrue(TEXT("the other stays"), IsValid(Zone1));
	TestTrue(FString::Printf(TEXT("150 s to its first flicker (%.3f)"), Zone1->GetTimeToSpawn()), FMath::IsNearlyEqual(Zone1->GetTimeToSpawn(), 150.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiBonusShardCollectTest, "Wasami.BonusShard.Collect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiBonusShardCollectTest::RunTest(const FString& Parameters)
{
	// The test world has no local player, so ENEMIES REVEALED is not put on a screen.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 0);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiGameMode* Mode = World->GetAuthGameMode<AWasamiGameMode>();
	if (!TestNotNull(TEXT("the project's game mode"), Mode) || !TestNotNull(TEXT("its save"), Mode->GetSave()))
	{
		return false;
	}
	TArray<int32>& Saved = Mode->GetSave()->Hospital.BonusShards;
	Saved.Reset();

	World->SpawnActor<AWasamiBonusShardSpawnPoint>(OrbPoints[0], FRotator::ZeroRotator);
	// An orb's point is not the bonus shard's.
	World->SpawnActor<AWasamiPowerOrbSpawnPoint>(OrbPoints[1], FRotator::ZeroRotator);
	AWasamiBonusShard* Bonus = SpawnBonusShard(World, 0);
	AWasamiPlayerCharacter* Player = SpawnBonusPlayer(World);
	if (!TestNotNull(TEXT("the bonus shard"), Bonus) || !TestNotNull(TEXT("the player"), Player))
	{
		return false;
	}
	TestEqual(TEXT("the level's one bonus shard spawn point"), Bonus->GetSpawnPoints().Num(), 1);
	// Enemies far from it, of two classes: a stand-in and a nurse.
	AWasamiTestEnemy* Near = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(1500., 0., 100.));
	AWasamiEnemy* Nurse = AWasamiEnemy::SpawnEnemy(World, FVector(-3000., 3000., 500.), 0.f);
	if (!TestNotNull(TEXT("a stand-in enemy"), Near) || !TestNotNull(TEXT("a nurse"), Nurse))
	{
		return false;
	}
	AdvanceOrb(Wrapper, 0.2f);
	TestTrue(TEXT("the map shows the shard"), Player->IsOnMap(Bonus));
	TestFalse(TEXT("but no enemy"), Player->IsOnMap(Near) || Player->IsOnMap(Nurse));

	// Taken while it flickers, at its spawn point.
	Bonus->MoveToSpawnPoint(0);
	Bonus->SpawnSpecialShard();
	OrbTeleport(Player, Bonus->GetCapsule()->GetComponentLocation());
	AdvanceOrb(Wrapper, OrbStep);
	TestTrue(TEXT("taken"), Bonus->IsCollected());
	TestTrue(TEXT("the actor stays"), IsValid(Bonus));
	TestTrue(TEXT("its ID in the save's BonusShards"), Saved.Num() == 1 && Saved[0] == 0);
	TestFalse(TEXT("the crystal is gone"), IsValid(Bonus->GetCrystal()));
	TestFalse(TEXT("the mark is gone"), IsValid(Bonus->GetMapMark()));
	TestFalse(TEXT("the light is gone"), IsValid(Bonus->GetLight()));
	TestEqual(TEXT("one collect effect"), OrbActorsOf<AWasamiBonusShardCollectEffect>(World).Num(), 1);
	TestTrue(TEXT("the reveal runs"), Bonus->IsRevealing());
	TestTrue(FString::Printf(TEXT("for 60 s (%.3f)"), Bonus->GetRevealTimeLeft()), FMath::IsNearlyEqual(Bonus->GetRevealTimeLeft(), 60.f, 0.1f));
	TestTrue(TEXT("the enemies on the map at once"), Player->IsOnMap(Near) && Player->IsOnMap(Nurse));
	TestEqual(TEXT("no spawn timer"), Bonus->GetTimeToSpawn(), -1.f);

	// The map's refreshes keep them; the flicker ends as it would have; the effect's 2 s end.
	AdvanceOrb(Wrapper, 1.f);
	TestTrue(TEXT("kept through the map's refreshes"), Player->IsOnMap(Near) && Player->IsOnMap(Nurse));
	AdvanceOrb(Wrapper, AWasamiSpecialShard::FlickerLength);
	TestFalse(TEXT("the flicker is over"), Bonus->IsFlickering());
	TestEqual(TEXT("the effect is gone"), OrbActorsOf<AWasamiBonusShardCollectEffect>(World).Num(), 0);
	TestTrue(TEXT("still revealing"), IsValid(Bonus) && Bonus->IsRevealing());

	// An enemy that comes later is shown by the next round (1 to 2 s on).
	AWasamiTestEnemy* Later = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(0., -1500., 100.));
	AdvanceOrb(Wrapper, 2.1f);
	TestTrue(TEXT("a later enemy shown within 2 s"), Player->IsOnMap(Later));

	// 60 s from the pickup: every enemy off the map, and the shard gone.
	const float Left = Bonus->GetRevealTimeLeft();
	AdvanceOrb(Wrapper, Left - 0.2f);
	TestTrue(TEXT("still there just before 60 s"), IsValid(Bonus) && Player->IsOnMap(Near));
	AdvanceOrb(Wrapper, 0.4f);
	TestFalse(TEXT("gone at 60 s"), IsValid(Bonus));
	TestFalse(TEXT("the enemies off the map"), Player->IsOnMap(Near) || Player->IsOnMap(Nurse) || Player->IsOnMap(Later));
	AdvanceOrb(Wrapper, 0.3f);
	TestFalse(TEXT("and they stay off"), Player->IsOnMap(Near) || Player->IsOnMap(Nurse) || Player->IsOnMap(Later));
	TestEqual(TEXT("the save keeps the ID"), Saved.Num(), 1);
	Saved.Reset();
	return true;
}

#endif
