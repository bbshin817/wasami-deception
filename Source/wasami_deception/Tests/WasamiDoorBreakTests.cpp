#include "Misc/AutomationTest.h"
#include "../WasamiDoorBreak.h"
#include "../WasamiSwitchboxWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Seconds of play in ticks of 0.1 s at most (as the zone flow's tests; a tick of nothing starts pending timers). */
	void AdvanceDoorBreakWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 0.f; Left -= 0.1f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.1f));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDoorBreakLockTest, "Wasami.DoorBreak.Lock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDoorBreakLockTest::RunTest(const FString& Parameters)
{
	// The animations (UMG_07_Boss_Switchbox's keys).
	TestEqual(TEXT("Interact starts the key at -3°"), UWasamiSwitchboxWidget::EvaluateInteractKeyAngle(true, 0.f), -3.f);
	TestEqual(TEXT("Interact_1 at 3°"), UWasamiSwitchboxWidget::EvaluateInteractKeyAngle(false, 0.f), 3.f);
	TestEqual(TEXT("past the other way at 0.1 s"), UWasamiSwitchboxWidget::EvaluateInteractKeyAngle(true, 0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("and straight at the end"), UWasamiSwitchboxWidget::EvaluateInteractKeyAngle(false, UWasamiSwitchboxWidget::InteractLength), 0.f, 1e-4f);
	TestEqual(TEXT("the key swells to 1.1"), UWasamiSwitchboxWidget::EvaluateInteractKeyScale(0.f), 1.1f, 1e-4f);
	TestEqual(TEXT("the rings dip to 0.98 at 0.1 s"), UWasamiSwitchboxWidget::EvaluateInteractRingsScale(0.1f), 0.98f, 1e-4f);
	TestEqual(TEXT("the sparks flash"), UWasamiSwitchboxWidget::EvaluateCompletedSparkOpacity(0.f), 1.f);
	TestEqual(TEXT("and are gone at 0.5 s"), UWasamiSwitchboxWidget::EvaluateCompletedSparkOpacity(UWasamiSwitchboxWidget::CompletedLength), 0.f, 1e-4f);
	TestEqual(TEXT("the rings gone at 0.25 s"), UWasamiSwitchboxWidget::EvaluateCompletedRingsOpacity(0.25f), 0.f, 1e-4f);
	TestEqual(TEXT("the second spark shrinks to 1.5"), UWasamiSwitchboxWidget::EvaluateCompletedSpark2Scale(UWasamiSwitchboxWidget::CompletedLength), 1.5f, 1e-4f);

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWasamiSwitchboxWidget* Lock = CreateWidget<UWasamiSwitchboxWidget>(Wrapper.GetTestWorld(), UWasamiSwitchboxWidget::StaticClass());
	if (!TestNotNull(TEXT("the lock"), Lock) || !TestNotNull(TEXT("its key"), Lock->GetKey()))
	{
		return false;
	}
	TestEqual(TEXT("F"), Lock->GetKey()->GetText().ToString(), FString(TEXT("F")));
	TestEqual(TEXT("the sparks unseen"), Lock->GetSpark()->GetRenderOpacity(), 0.f);
	TestEqual(TEXT("the class's speed"), Lock->ProgressSpeed, 5.f);

	// Zone 1's lock: 1.5 a press, so 66 presses reach 99 and the 67th passes it.
	Lock->ProgressSpeed = 1.5f;
	for (int32 Press = 0; Press < 66; ++Press)
	{
		Lock->InteractEvent();
	}
	TestEqual(TEXT("66 presses"), Lock->Progress, 99.f, 1e-3f);
	TestFalse(TEXT("hold"), Lock->HasFinished());
	TestTrue(TEXT("a press jolts it"), Lock->IsPlaying());
	Lock->InteractEvent();
	TestTrue(TEXT("the 67th opens it"), Lock->HasFinished());
	TestEqual(TEXT("full"), Lock->Progress, 100.f);
	TestEqual(TEXT("Completed's first frame at once"), Lock->GetSpark()->GetRenderOpacity(), 1.f);

	// Completed at twice its rate: 0.1 s is 0.2 s into it.
	Lock->Advance(0.1f);
	TestEqual(TEXT("the key faded out"), Lock->GetKey()->GetRenderOpacity(), UWasamiSwitchboxWidget::EvaluateCompletedKeyOpacity(0.2f), 1e-4f);
	TestEqual(TEXT("the rings fading"), Lock->GetRings()->GetRenderOpacity(), UWasamiSwitchboxWidget::EvaluateCompletedRingsOpacity(0.2f), 1e-4f);
	Lock->Advance(0.2f);
	TestFalse(TEXT("all done by 0.25 s"), Lock->IsPlaying());
	TestEqual(TEXT("the sparks gone"), Lock->GetSpark()->GetRenderOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("the key's jolt over"), Lock->GetKey()->GetRenderTransformAngle(), 0.f, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDoorBreakActorTest, "Wasami.DoorBreak.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDoorBreakActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiDoorBreak* Door = World->SpawnActorDeferred<AWasamiDoorBreak>(AWasamiDoorBreak::StaticClass(), FTransform::Identity);
	Door->ProgressSpeed = 3.f;
	Door->FinishSpawning(FTransform::Identity);
	UBoxComponent* Box = Door->GetBox();
	UWidgetComponent* Widget = Door->GetWidget();
	UWasamiSwitchboxWidget* Lock = Door->GetUIWidget();
	if (!TestNotNull(TEXT("the lock"), Lock))
	{
		return false;
	}
	TestTrue(TEXT("100 × 150 × 100"), Box->GetUnscaledBoxExtent().Equals(FVector(100., 150., 100.), 1e-3));
	TestTrue(TEXT("at half scale"), Box->GetRelativeScale3D().Equals(FVector(0.5), 1e-4));
	TestFalse(TEXT("asleep"), Box->GetGenerateOverlapEvents());
	TestTrue(TEXT("the lock in screen space"), Widget->GetWidgetSpace() == EWidgetSpace::Screen);
	TestTrue(TEXT("64 × 64"), Widget->GetDrawSize() == FIntPoint(64, 64));
	TestFalse(TEXT("unseen"), Widget->IsVisible());
	TestEqual(TEXT("the lock takes the door's speed"), Lock->ProgressSpeed, 3.f);

	Door->EnableSwitch();
	TestTrue(TEXT("Enable Switch wakes the box"), Box->GetGenerateOverlapEvents());

	Door->Interact();
	TestEqual(TEXT("out of range, a press does nothing"), Lock->Progress, 0.f);
	Door->NotifyPlayerOverlap(true);
	TestTrue(TEXT("in range"), Door->IsInRange());
	TestTrue(TEXT("the lock shown"), Widget->IsVisible());
	Door->Interact();
	Door->Interact();
	TestEqual(TEXT("two presses"), Lock->Progress, 6.f);
	Door->NotifyPlayerOverlap(false);
	TestFalse(TEXT("walking out hides it"), Widget->IsVisible());
	TestEqual(TEXT("and empties it"), Lock->Progress, 0.f);

	Door->NotifyPlayerOverlap(true);
	for (int32 Press = 0; Press < 34; ++Press)
	{
		Door->Interact();
	}
	TestTrue(TEXT("Zone 2's 34 presses open it"), Lock->HasFinished());
	TestFalse(TEXT("the box gone"), IsValid(Box));
	TestFalse(TEXT("out of range"), Door->IsInRange());
	TestTrue(TEXT("the lock shown for its end"), Widget->IsVisible());
	AdvanceDoorBreakWorld(Wrapper, AWasamiDoorBreak::RemoveWidgetDelay - 0.2f);
	TestTrue(TEXT("still there at 0.8 s"), IsValid(Widget));
	AdvanceDoorBreakWorld(Wrapper, 0.3f);
	TestFalse(TEXT("taken away 1 s on"), IsValid(Widget));
	return true;
}

#endif
