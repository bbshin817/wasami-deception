#include "Misc/AutomationTest.h"
#include "../WasamiInteractWidget.h"
#include "../WasamiPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "WasamiTestInteractable.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiInteractWidgetTest, "Wasami.Interact.Widget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiInteractWidgetTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWasamiInteractWidget* Widget = CreateWidget<UWasamiInteractWidget>(Wrapper.GetTestWorld(), UWasamiInteractWidget::StaticClass());
	if (!TestNotNull(TEXT("the hand"), Widget))
	{
		return false;
	}
	Widget->TakeWidget();
	UImage* Image = Widget->GetImage();
	if (!TestNotNull(TEXT("Image_18"), Image))
	{
		return false;
	}

	// UMG_Interact's tree.
	const UCanvasPanel* Root = Cast<UCanvasPanel>(Widget->GetRootWidget());
	TestTrue(TEXT("a canvas at the root, out of the mouse's way"), Root && Root->GetVisibility() == ESlateVisibility::HitTestInvisible);
	const FSlateBrush& Brush = Image->GetBrush();
	const UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
	TestTrue(TEXT("interact_icon_03"), Texture && Texture->GetName() == TEXT("interact_icon_03"));
	TestTrue(TEXT("90 × 90"), Brush.ImageSize.Equals(FVector2D(90.f, 90.f)));
	TestTrue(TEXT("white at half opacity"), Image->GetColorAndOpacity().Equals(FLinearColor(1.f, 1.f, 1.f, 0.5f)));
	TestTrue(TEXT("drawn at half scale"), Image->GetRenderTransform().Scale.Equals(FVector2D(0.5f, 0.5f)));
	TestTrue(TEXT("the image out of the mouse's way too"), Image->GetVisibility() == ESlateVisibility::HitTestInvisible);
	const UCanvasPanelSlot* ImageSlot = Cast<UCanvasPanelSlot>(Image->Slot);
	if (TestNotNull(TEXT("in the canvas"), ImageSlot))
	{
		const FAnchorData Layout = ImageSlot->GetLayout();
		TestTrue(TEXT("anchored at the centre"), Layout.Anchors.Minimum.Equals(FVector2D(0.5f, 0.5f)) && Layout.Anchors.Maximum.Equals(FVector2D(0.5f, 0.5f)));
		TestTrue(TEXT("centred on it"), Layout.Alignment.Equals(FVector2D(0.5f, 0.5f)));
		TestTrue(TEXT("offsets (0, 0, 100, 40)"), Layout.Offsets == FMargin(0.f, 0.f, 100.f, 40.f));
		TestTrue(TEXT("at its own size"), ImageSlot->GetAutoSize());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiInteractTraceTest, "Wasami.Interact.Trace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiInteractTraceTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	// No controller: the player stays where it is put, looking along its own forward. The world is not ticked.
	AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector(0., 0., 200.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player))
	{
		return false;
	}
	UWasamiInteractWidget* Hand = Player->GetInteractWidget();
	if (!TestNotNull(TEXT("BeginPlay made the hand"), Hand))
	{
		return false;
	}
	TestTrue(TEXT("collapsed at first"), Hand->GetVisibility() == ESlateVisibility::Collapsed);

	// A box whose near face is 100 cm ahead of the camera.
	const UCameraComponent* Camera = Player->GetCamera();
	const FVector Eye = Camera->GetComponentLocation();
	const FVector Ahead = Camera->GetForwardVector();
	AWasamiTestInteractable* Thing = World->SpawnActor<AWasamiTestInteractable>(Eye + Ahead * 150., FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("something to use"), Thing))
	{
		return false;
	}
	FHitResult Hit;
	TestTrue(TEXT("the trace hits it"), Player->TraceInteract(Hit) && Hit.GetActor() == Thing);

	// The hand: shown on a component tagged interact, collapsed on one without.
	Player->UpdateInteractWidget();
	TestTrue(TEXT("the hand shows"), Hand->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);
	Thing->GetBox()->ComponentTags.Remove(TEXT("interact"));
	Player->UpdateInteractWidget();
	TestTrue(TEXT("not without the tag"), Hand->GetVisibility() == ESlateVisibility::Collapsed);
	Thing->GetBox()->ComponentTags.Add(TEXT("interact"));
	Player->bCanInteract = false;
	Player->UpdateInteractWidget();
	TestTrue(TEXT("without Can Interact? the hand stays as it was"), Hand->GetVisibility() == ESlateVisibility::Collapsed);
	Player->bCanInteract = true;
	Player->UpdateInteractWidget();
	TestTrue(TEXT("shown again"), Hand->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);
	Player->bCanInteract = false;
	Player->UpdateInteractWidget();
	TestTrue(TEXT("and stays shown"), Hand->GetVisibility() == ESlateVisibility::SelfHitTestInvisible);
	Player->bCanInteract = true;

	// The click: InteractWithObject on press, StopInteractWithObject on release.
	Player->InteractSecondaryPressed();
	TestEqual(TEXT("a press uses it"), Thing->InteractCount, 1);
	TestTrue(TEXT("with the player"), Thing->LastInteractee.Get() == Player);
	Player->InteractSecondaryReleased();
	TestEqual(TEXT("the release stops it"), Thing->StopCount, 1);

	// Without Can Interact? a press does nothing, but the release still goes to what the last press hit.
	Player->bCanInteract = false;
	Player->InteractSecondaryPressed();
	TestEqual(TEXT("no use"), Thing->InteractCount, 1);
	Player->InteractSecondaryReleased();
	TestEqual(TEXT("the release still stops it"), Thing->StopCount, 2);
	Player->bCanInteract = true;

	// Beyond 200 cm: no hand, and a press hits nothing, so its release goes nowhere.
	Thing->SetActorLocation(Eye + Ahead * 300.);
	Player->UpdateInteractWidget();
	TestTrue(TEXT("no hand far off"), Hand->GetVisibility() == ESlateVisibility::Collapsed);
	Player->InteractSecondaryPressed();
	TestEqual(TEXT("out of reach"), Thing->InteractCount, 1);
	Player->InteractSecondaryReleased();
	TestEqual(TEXT("nothing to stop"), Thing->StopCount, 2);

	// At 199 cm the near face is in reach again.
	Thing->SetActorLocation(Eye + Ahead * 249.);
	Player->InteractSecondaryPressed();
	TestEqual(TEXT("in reach at 199 cm"), Thing->InteractCount, 2);
	return true;
}

#endif
