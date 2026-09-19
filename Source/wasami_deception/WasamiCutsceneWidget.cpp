#include "WasamiCutsceneWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/InputComponent.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "LevelSequencePlayer.h"
#include "Materials/MaterialParameterCollection.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Start and End (pak_reference_2's UMG_CutsceneWidget): Mat_ParameterCol's Cutscene Bars in over a second and held
	// to 1.25 s, and out over a second.
	const FAnimKey CutsceneStartBarsKeys[] = {
		{0., 0.f, 0., 0.},
		{60000., 1.f, 1.3333333299669903e-05, 1.3333333299669903e-05},
		{75000., 1.f, 0., 0.},
	};
	const FAnimKey CutsceneEndBarsKeys[] = {{0., 1.f, 0., 0.}, {60000., 0.f, 0., 0.}};

	// Fade: HorizontalBox_98's RenderOpacity in over a second, held two, out over one.
	const FAnimKey CutsceneFadeKeys[] = {
		{0., 0.f, 0., 0.},
		{60000., 1.f, 8.333333425980527e-06, 8.333333425980527e-06},
		{180001., 1.f, -8.333333425980527e-06, -8.333333425980527e-06},
		{240001., 0.f, 0., 0.},
	};

	// BlackTransition: Black's RenderOpacity to 1 by 0.15 s (where the event track jumps the sequence to its end), held
	// to 0.25 s while the cutscene ends behind it, and out by 0.5 s; Overlay_0's (the prompt's corner) 1 to 0 with it.
	const FAnimKey CutsceneBlackKeys[] = {
		{0., 0.f, 0., 0.},
		{9000., 1.f, 5.555555617320351e-05, 0., RCIM_Constant},
		{15000., 1.f, -6.666666740784422e-05, -6.666666740784422e-05},
		{30000., 0.f, 0., 0.},
	};
	const FAnimKey CutsceneBlackOverlayKeys[] = {{0., 1.f, 0., 0.}, {9000., 0.f, 0., 0.}};

	// CanvasPanelSlot_3: the prompt's corner at its own size, 20 px in from the bottom right (a sized slot's offsets,
	// which auto size leaves unused, stay as they are).
	const FMargin CutscenePromptOffsets(-20.f, -20.f, 100.f, 100.f);

	// TextBlock_22 and TextBlock_25 (PRESS and TO SKIP); TextBlock_24 (the key) keeps the tree's default font.
	const FName CutscenePromptTypeface(TEXT("Light"));
	constexpr float CutscenePromptFontSize = 16.f;
}

const FName UWasamiCutsceneWidget::BarsParameter(TEXT("Cutscene Bars"));

UWasamiCutsceneWidget::UWasamiCutsceneWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ParameterCollection = TSoftObjectPtr<UMaterialParameterCollection>(
		WasamiAssets::Path(TEXT("/Game/DD/Materials/Special/Mat_ParameterCol")));
}

UWasamiCutsceneWidget* UWasamiCutsceneWidget::Show(const UObject* WorldContextObject, ULevelSequencePlayer* Sequence,
	bool bInSmoothTransition)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	// Create Cutscene Widget (DD_PlayerController): the widget with Sequence and bSmoothTransition set, then added.
	UWasamiCutsceneWidget* Widget = CreateWidget<UWasamiCutsceneWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->Sequence = Sequence;
		Widget->bSmoothTransition = bInSmoothTransition;
		if (Controller->GetWorld() && Controller->GetWorld()->GetGameViewport())
		{
			Widget->AddToViewport(ViewportZOrder);
		}
	}
	return Widget;
}

bool UWasamiCutsceneWidget::FPlaying::Advance(float DeltaSeconds, float Length)
{
	Time = FMath::Min(Time + DeltaSeconds, Length);
	return Time >= Length;
}

TSharedRef<SWidget> UWasamiCutsceneWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;

		// Black: the whole screen, never in the way of the mouse, transparent until BlackTransition plays.
		Black = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Black"));
		Black->SetColorAndOpacity(FLinearColor::Black);
		Black->SetVisibility(ESlateVisibility::HitTestInvisible);
		Black->SetRenderOpacity(0.f);
		UCanvasPanelSlot* BlackSlot = Root->AddChildToCanvas(Black);
		FAnchorData BlackLayout;
		BlackLayout.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
		BlackLayout.Offsets = FMargin(0.f);
		BlackSlot->SetLayout(BlackLayout);

		// Overlay_0 in the bottom right corner, holding the prompt, which Fade brings in and out again.
		PromptOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_0"));
		UCanvasPanelSlot* OverlaySlot = Root->AddChildToCanvas(PromptOverlay);
		FAnchorData OverlayLayout;
		OverlayLayout.Anchors = FAnchors(1.f, 1.f, 1.f, 1.f);
		OverlayLayout.Offsets = CutscenePromptOffsets;
		OverlayLayout.Alignment = FVector2D(1.f, 1.f);
		OverlaySlot->SetLayout(OverlayLayout);
		OverlaySlot->SetAutoSize(true);

		Prompt = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_98"));
		Prompt->SetRenderOpacity(0.f);
		PromptOverlay->AddChildToOverlay(Prompt);

		// PRESS / P / TO SKIP, each in the middle of its slot; the first fills what room the box has.
		const TCHAR* const Names[] = {TEXT("TextBlock_22"), TEXT("TextBlock_24"), TEXT("TextBlock_25")};
		const TCHAR* const Texts[] = {TEXT("PRESS "), TEXT("P"), TEXT(" TO SKIP")};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Names[Index]);
			Text->SetText(FText::FromString(Texts[Index]));
			if (Index != 1)
			{
				FSlateFontInfo Font = Text->GetFont();
				Font.TypefaceFontName = CutscenePromptTypeface;
				Font.Size = CutscenePromptFontSize;
				Text->SetFont(Font);
			}
			UHorizontalBoxSlot* TextSlot = Prompt->AddChildToHorizontalBox(Text);
			if (Index == 0)
			{
				TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UWasamiCutsceneWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct: Start from its beginning when the transition is smooth, else from a second in (the bars already down);
	// its end is Initialized, End's is Remove, and the sequence's OnFinished is Cutscene Over.
	StartAnimation.Start(bSmoothTransition ? 0.f : 1.f);
	bInitialized = false;
	bSkipped = false;
	SetBars(EvaluateStartBars(StartAnimation.Time));
	if (Sequence)
	{
		Sequence->OnFinished.AddUniqueDynamic(this, &UWasamiCutsceneWidget::CutsceneOver);
	}

	// The keys the original's player controller takes while a cutscene plays: any key shows the prompt (without eating
	// the press), and Skip Cutscene (P, Gamepad Special Right) blacks the screen out and ends the scene. They sit on the
	// controller, not on the player, whose input a cutscene turns off.
	if (APlayerController* Controller = GetOwningPlayer())
	{
		if (UInputComponent* Input = Controller->InputComponent)
		{
			Input->BindKey(EKeys::AnyKey, IE_Pressed, this, &UWasamiCutsceneWidget::AnyKeyPress).bConsumeInput = false;
			for (const FKey& Key : {EKeys::P, EKeys::Gamepad_Special_Right})
			{
				Input->BindKey(Key, IE_Pressed, this, &UWasamiCutsceneWidget::StartHold).bConsumeInput = false;
			}
		}
	}
}

void UWasamiCutsceneWidget::NativeDestruct()
{
	if (Sequence)
	{
		Sequence->OnFinished.RemoveDynamic(this, &UWasamiCutsceneWidget::CutsceneOver);
	}
	if (APlayerController* Controller = GetOwningPlayer())
	{
		if (UInputComponent* Input = Controller->InputComponent)
		{
			Input->KeyBindings.RemoveAll([this](const FInputKeyBinding& Binding)
			{
				return Binding.KeyDelegate.GetUObject() == this;
			});
		}
	}
	Super::NativeDestruct();
}

void UWasamiCutsceneWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiCutsceneWidget::AnyKeyPress()
{
	// Any Key Press: nothing before Initialized, and nothing while Fade is already playing.
	if (bInitialized && !FadeAnimation.bPlaying)
	{
		FadeAnimation.Start(0.f);
		ApplyAnimations();
	}
}

void UWasamiCutsceneWidget::StartHold()
{
	// Start Hold: nothing once End plays (the cutscene is ending of its own accord), nor twice over.
	if (EndAnimation.bPlaying || BlackTransitionAnimation.bPlaying)
	{
		return;
	}
	BlackTransitionAnimation.Start(0.f);
	bSkipped = false;
	ApplyAnimations();
	OnSkipped.Broadcast();
}

void UWasamiCutsceneWidget::CutsceneOver()
{
	EndAnimation.Start(0.f);
	SetBars(EvaluateEndBars(EndAnimation.Time));
}

void UWasamiCutsceneWidget::Advance(float DeltaSeconds)
{
	if (StartAnimation.bPlaying)
	{
		const bool bEnded = StartAnimation.Advance(DeltaSeconds, StartLength);
		SetBars(EvaluateStartBars(StartAnimation.Time));
		if (bEnded)
		{
			// Initialized.
			StartAnimation.bPlaying = false;
			bInitialized = true;
		}
	}
	if (FadeAnimation.bPlaying && FadeAnimation.Advance(DeltaSeconds, FadeLength))
	{
		FadeAnimation.bPlaying = false;
	}
	if (BlackTransitionAnimation.bPlaying)
	{
		const bool bEnded = BlackTransitionAnimation.Advance(DeltaSeconds, BlackTransitionLength);
		if (!bSkipped && BlackTransitionAnimation.Time >= SkipAt)
		{
			bSkipped = true;
			SkipCutscene();
		}
		if (bEnded)
		{
			BlackTransitionAnimation.bPlaying = false;
		}
	}
	ApplyAnimations();
	if (EndAnimation.bPlaying)
	{
		const bool bEnded = EndAnimation.Advance(DeltaSeconds, EndLength);
		SetBars(EvaluateEndBars(EndAnimation.Time));
		if (bEnded)
		{
			// Remove: the screen off once the bars are up again.
			EndAnimation.bPlaying = false;
			RemoveFromParent();
		}
	}
}

void UWasamiCutsceneWidget::SkipCutscene()
{
	// Skip Cutscene: the sequence to its end behind the black, so that whatever its end binds still runs. UE4's
	// JumpToSeconds(Seconds) is SetPlaybackPosition(Seconds, Jump) here; either way the time is clamped to the
	// sequence's playback range, so any time past its end lands on its end.
	if (Sequence)
	{
		Sequence->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(SkipToSeconds, EUpdatePositionMethod::Jump));
	}
}

void UWasamiCutsceneWidget::ApplyAnimations()
{
	if (FadeAnimation.bPlaying || FadeAnimation.Time > 0.f)
	{
		PromptOpacity = EvaluateFade(FadeAnimation.Time);
	}
	if (BlackTransitionAnimation.bPlaying || BlackTransitionAnimation.Time > 0.f)
	{
		BlackOpacity = EvaluateBlackTransition(BlackTransitionAnimation.Time);
		OverlayOpacity = EvaluateBlackTransitionOverlay(BlackTransitionAnimation.Time);
	}
	if (Black)
	{
		Black->SetRenderOpacity(BlackOpacity);
	}
	if (PromptOverlay)
	{
		PromptOverlay->SetRenderOpacity(OverlayOpacity);
	}
	if (Prompt)
	{
		Prompt->SetRenderOpacity(PromptOpacity);
	}
}

void UWasamiCutsceneWidget::SetBars(float Value)
{
	Bars = Value;
	UMaterialParameterCollection* Collection = GetWorld() ? ParameterCollection.LoadSynchronous() : nullptr;
	if (Collection)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, BarsParameter, Bars);
	}
}

float UWasamiCutsceneWidget::EvaluateStartBars(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CutsceneStartBarsKeys);
	return Eval(Curve, Seconds, StartLength);
}

float UWasamiCutsceneWidget::EvaluateEndBars(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CutsceneEndBarsKeys);
	return Eval(Curve, Seconds, EndLength);
}

float UWasamiCutsceneWidget::EvaluateFade(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CutsceneFadeKeys);
	return Eval(Curve, Seconds, FadeLength);
}

float UWasamiCutsceneWidget::EvaluateBlackTransition(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CutsceneBlackKeys);
	return Eval(Curve, Seconds, BlackTransitionLength);
}

float UWasamiCutsceneWidget::EvaluateBlackTransitionOverlay(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CutsceneBlackOverlayKeys);
	return Eval(Curve, Seconds, BlackTransitionLength);
}
