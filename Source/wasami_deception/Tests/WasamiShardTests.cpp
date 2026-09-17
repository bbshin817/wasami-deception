#include "Misc/AutomationTest.h"
#include "../WasamiShard.h"
#include "../WasamiTabletWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CapsuleComponent.h"
#include "Components/Image.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiShardPullCurveTest, "Wasami.Shard.PullCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiShardPullCurveTest::RunTest(const FString& Parameters)
{
	// Shard Pull's Alpha: linear from 0 to 1 over 0.75 s, then 1 to the timeline's end.
	TestEqual(TEXT("Alpha starts at 0"), AWasamiShard::EvaluatePullAlpha(0.f), 0.f, 1e-5f);
	TestEqual(TEXT("halfway at 0.375 s"), AWasamiShard::EvaluatePullAlpha(0.375f), 0.5f, 1e-5f);
	TestEqual(TEXT("1 at 0.75 s"), AWasamiShard::EvaluatePullAlpha(0.75f), 1.f, 1e-5f);
	TestEqual(TEXT("1 at the end"), AWasamiShard::EvaluatePullAlpha(AWasamiShard::PullLength), 1.f, 1e-5f);

	// VEase with ExpoIn toward the player's X and Y at the start's height: 2^(10(Alpha - 1)).
	const FVector From(1000., 0., 50.);
	const FVector Player(0., 400., 90.);
	TestTrue(TEXT("at the start with Alpha 0"), AWasamiShard::PullLocation(From, Player, 0.f).Equals(From, 1e-3));
	const FVector Half = AWasamiShard::PullLocation(From, Player, 0.5f);
	TestEqual(TEXT("barely moved at Alpha 0.5 (X)"), Half.X, 1000. - 1000. / 32., 1e-2);
	TestEqual(TEXT("barely moved at Alpha 0.5 (Y)"), Half.Y, 400. / 32., 1e-2);
	const FVector Late = AWasamiShard::PullLocation(From, Player, 0.9f);
	TestEqual(TEXT("halfway at Alpha 0.9"), Late.X, 500., 1e-2);
	const FVector End = AWasamiShard::PullLocation(From, Player, 1.f);
	TestTrue(TEXT("at the player's X and Y with Alpha 1"), End.Equals(FVector(0., 400., 50.), 1e-3));
	TestEqual(TEXT("the height is the start's"), Late.Z, 50., 1e-6);

	// The crystal's animation turns it twice in 1.6667 s at a RateScale of 0.5.
	TestEqual(TEXT("the slowest spin"), AWasamiShard::SpinSpeed(0.05f), 10.8f, 1e-3f);
	TestEqual(TEXT("the fastest spin"), AWasamiShard::SpinSpeed(0.15f), 32.4f, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiShardActorTest, "Wasami.Shard.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiShardActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	const FVector Placed(1000., 0., 0.);
	AWasamiShard* Shard = World->SpawnActor<AWasamiShard>(Placed, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the shard"), Shard))
	{
		return false;
	}

	// BP_Shard's components, under the crystal's place 97.085 cm up at a scale of 10.
	const UCapsuleComponent* Capsule = Shard->FindComponentByClass<UCapsuleComponent>();
	const UPointLightComponent* Light = Shard->FindComponentByClass<UPointLightComponent>();
	if (!TestNotNull(TEXT("the capsule"), Capsule) || !TestNotNull(TEXT("the light"), Light))
	{
		return false;
	}
	TestEqual(TEXT("a ball of 49.57 cm"), Capsule->GetScaledCapsuleRadius(), 49.571804f, 1e-3f);
	TestEqual(TEXT("its half height"), Capsule->GetScaledCapsuleHalfHeight(), 49.571804f, 1e-3f);
	TestEqual(TEXT("100 cm up"), Capsule->GetComponentLocation().Z, 97.08537292480469 + 2.914627194404602, 1e-3);
	TestEqual(TEXT("on the world static channel"), static_cast<int32>(Capsule->GetCollisionObjectType()), static_cast<int32>(ECC_WorldStatic));
	TestEqual(TEXT("with a custom profile"), Capsule->GetCollisionProfileName(), FName(TEXT("Custom")));
	TestEqual(TEXT("for queries only"), static_cast<int32>(Capsule->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::QueryOnly));
	TestEqual(TEXT("overlapping pawns"), static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_Pawn)), static_cast<int32>(ECR_Overlap));
	TestEqual(TEXT("overlapping the world"), static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_WorldStatic)), static_cast<int32>(ECR_Overlap));
	TestTrue(TEXT("with overlap events"), Capsule->GetGenerateOverlapEvents());

	TestTrue(TEXT("the light below the crystal's middle"),
		Light->GetComponentLocation().Equals(Placed + FVector(-2.1608541905879974, 0.00018310546875, 92.5668), 1e-2));
	TestEqual(TEXT("its intensity"), Light->Intensity, 175.f);
	TestEqual(TEXT("in no units"), static_cast<int32>(Light->IntensityUnits), static_cast<int32>(ELightUnits::Unitless));
	TestEqual(TEXT("its radius"), Light->AttenuationRadius, 200.f);
	TestEqual(TEXT("its colour"), Light->LightColor, FColor(194, 0, 255, 255));
	TestFalse(TEXT("without shadows"), static_cast<bool>(Light->CastShadows));
	TestEqual(TEXT("movable"), static_cast<int32>(Light->Mobility.GetValue()), static_cast<int32>(EComponentMobility::Movable));

	const UStaticMeshComponent* Mochi = nullptr;
	const UStaticMeshComponent* Plane = nullptr;
	TArray<UStaticMeshComponent*> Meshes;
	Shard->GetComponents(Meshes);
	for (const UStaticMeshComponent* Each : Meshes)
	{
		(Each->GetFName() == TEXT("Plane") ? Plane : Mochi) = Each;
	}
	if (!TestNotNull(TEXT("the mochi"), Mochi) || !TestNotNull(TEXT("the map's mark"), Plane))
	{
		return false;
	}
	TestTrue(TEXT("the mochi is 0.55 m"), Mochi->GetComponentScale().Equals(FVector(0.55), 1e-6));
	TestEqual(TEXT("the mochi at the crystal's middle"), Mochi->GetComponentLocation().Z, 97.08537292480469, 1e-3);
	TestEqual(TEXT("the mochi's draw distance"), Mochi->LDMaxDrawDistance, 3000.f);
	TestEqual(TEXT("the mark 20 m up"), Plane->GetComponentLocation().Z, 2000., 1e-3);
	TestTrue(TEXT("the mark is 1.5 m"), Plane->GetComponentScale().Equals(FVector(1.5, 1.5, 10.), 1e-6));
	TestEqual(TEXT("the mark does not collide"), static_cast<int32>(Plane->GetCollisionEnabled()), static_cast<int32>(ECollisionEnabled::NoCollision));

	TestTrue(TEXT("a play rate from 0.05 to 0.15"), Shard->GetSpinRate() >= 0.05f && Shard->GetSpinRate() <= 0.15f);
	const FSoftObjectProperty* FlashProperty = FindFProperty<FSoftObjectProperty>(AWasamiShard::StaticClass(), TEXT("CollectFlash"));
	TestTrue(TEXT("the collect flash is P_ky_flash3"), FlashProperty && FlashProperty->GetPropertyValue_InContainer(Shard).ToSoftObjectPath()
		== FSoftObjectPath(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash3.P_ky_flash3")));

	// Activate with no player: the shard heads for the origin, and at the end Collect finds no tablet and stops.
	TestFalse(TEXT("not pulled before"), Shard->IsPulling());
	IWasamiTelekinesisInterface::Execute_Activate(Shard);
	TestTrue(TEXT("pulled"), Shard->IsPulling());
	TestTrue(TEXT("at a rate from 0.8 to 1.2"), Shard->GetPullRate() >= 0.8f && Shard->GetPullRate() <= 1.2f);
	TestTrue(TEXT("still where it was at the start"), Shard->GetActorLocation().Equals(Placed, 1e-3));
	constexpr float Step = 1.f / 60.f;
	const float Length = AWasamiShard::PullLength / Shard->GetPullRate();
	float Done = 0.f;
	// 0.45 of the pull is Alpha 0.6, where ExpoIn has gone a sixteenth of the way (62.5 cm).
	for (; Done < Length * 0.45f; Done += Step)
	{
		Wrapper.TickTestWorld(Step);
	}
	TestTrue(TEXT("a little way along at 0.45 of the pull"), Shard->GetActorLocation().X > 900. && Shard->GetActorLocation().X < 1000.);
	for (; Done < Length + 0.1f; Done += Step)
	{
		Wrapper.TickTestWorld(Step);
	}
	TestFalse(TEXT("the pull is over"), Shard->IsPulling());
	TestTrue(TEXT("at the target"), Shard->GetActorLocation().Equals(FVector::ZeroVector, 1e-2));
	TestTrue(TEXT("kept without a tablet to count on"), IsValid(Shard));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTabletCountShakeTest, "Wasami.Tablet.CountShake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTabletCountShakeTest::RunTest(const FString& Parameters)
{
	// The curves at their keys (animation seconds) and between the flash's two keys (flat tangents).
	TestTrue(TEXT("no offset at the start"), UWasamiTabletWidget::EvaluateCountShakeTranslation(0.f).Equals(FVector2D::ZeroVector, 1e-4));
	TestTrue(TEXT("left and down at 0.05 s"), UWasamiTabletWidget::EvaluateCountShakeTranslation(0.05f).Equals(FVector2D(-14., 9.), 1e-3));
	TestTrue(TEXT("up at 0.1 s"), UWasamiTabletWidget::EvaluateCountShakeTranslation(0.1f).Equals(FVector2D(0., -12.), 1e-3));
	TestTrue(TEXT("back at the end"), UWasamiTabletWidget::EvaluateCountShakeTranslation(0.2f).Equals(FVector2D::ZeroVector, 1e-4));
	TestEqual(TEXT("grown at 0.05 s"), UWasamiTabletWidget::EvaluateCountShakeScale(0.05f), 1.1f, 1e-4f);
	TestEqual(TEXT("back to size at 0.1 s"), UWasamiTabletWidget::EvaluateCountShakeScale(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("the flash starts at a quarter"), UWasamiTabletWidget::EvaluateCountShakeFlash(0.f), 0.25f, 1e-5f);
	TestEqual(TEXT("half of it halfway"), UWasamiTabletWidget::EvaluateCountShakeFlash(0.1f), 0.125f, 1e-4f);
	TestEqual(TEXT("gone at the end"), UWasamiTabletWidget::EvaluateCountShakeFlash(0.2f), 0.f, 1e-5f);

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWasamiTabletWidget* Screen = CreateWidget<UWasamiTabletWidget>(Wrapper.GetTestWorld(), UWasamiTabletWidget::StaticClass());
	if (!TestNotNull(TEXT("the screen"), Screen))
	{
		return false;
	}
	Screen->TakeWidget();
	const UTextBlock* Count = Cast<UTextBlock>(Screen->WidgetTree->FindWidget(TEXT("ShardCount")));
	const UImage* Flash = Cast<UImage>(Screen->WidgetTree->FindWidget(TEXT("Flash")));
	if (!TestNotNull(TEXT("the count"), Count) || !TestNotNull(TEXT("the flash"), Flash))
	{
		return false;
	}
	TestEqual(TEXT("0 before a count is set"), Screen->GetShardCount(), 0);
	Screen->SetShardCount(337);
	TestEqual(TEXT("the count set"), Screen->GetShardCount(), 337);
	const FVector2D Rest = Count->GetRenderTransform().Translation;
	TestTrue(TEXT("the count's own lift"), Rest.Equals(FVector2D(0., -12.), 1e-4));

	// Played at twice the speed: its first frame at once, 0.05 s of it after 0.025 s, the end after 0.1 s.
	Screen->PlayCountShake();
	TestTrue(TEXT("the first frame puts the count at no offset"), Count->GetRenderTransform().Translation.Equals(FVector2D::ZeroVector, 1e-4));
	TestEqual(TEXT("the flash shows"), Flash->GetColorAndOpacity().A, 0.25f, 1e-5f);
	Screen->TickAnimations(0.025f);
	TestTrue(TEXT("jolted"), Count->GetRenderTransform().Translation.Equals(FVector2D(-14., 9.), 1e-3));
	TestEqual(TEXT("grown"), Count->GetRenderTransform().Scale.X, 1.1, 1e-4);
	Screen->TickAnimations(0.075f);
	TestTrue(TEXT("the transform is back at its end"), Count->GetRenderTransform().Translation.Equals(Rest, 1e-4));
	TestEqual(TEXT("its size too"), Count->GetRenderTransform().Scale.X, 1., 1e-6);
	TestEqual(TEXT("the flash stays out"), Flash->GetColorAndOpacity().A, 0.f, 1e-5f);

	// Played again mid-way: it starts over, and its end still gives back the transform from before it first played.
	Screen->PlayCountShake();
	Screen->TickAnimations(0.03f);
	Screen->PlayCountShake();
	TestEqual(TEXT("started over"), Flash->GetColorAndOpacity().A, 0.25f, 1e-5f);
	Screen->TickAnimations(0.2f);
	TestTrue(TEXT("back to the lift"), Count->GetRenderTransform().Translation.Equals(Rest, 1e-4));
	return true;
}

#endif
