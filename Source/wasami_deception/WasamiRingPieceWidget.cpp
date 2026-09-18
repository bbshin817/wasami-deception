#include "WasamiRingPieceWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiGameMode.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	// The animations' keys as exported (pak_reference_2's UMG_01_RingPieceCollect; WasamiWidgetAnimation.h).
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// NewAnimation_1 on ringpiece: its scale (X and Y keyed alike) grows from nothing to 4 by 0.25 s, swells to 4.1 and
	// settles at 4 by 0.5 s (the key at 2.5 s, past the end, holds it); it turns in from 90 degrees, past -5, to 0; and
	// it fades in over the first 0.25 s.
	const FAnimKey RingPieceScaleKeys[] = {
		{0., 0.f, 0., 0.},
		{15000., 4.f, 0.0001952473830897361, 0.0001952473830897361},
		{20999., 4.099999904632568f, 0., 0.},
		{30000., 4.f, -1.1109866136393975e-05, 0., RCIM_Linear},
		{150000., 4.f, 0., 0.},
	};
	const FAnimKey RingPieceAngleKeys[] = {
		{0., 90.f, 0., 0.},
		{15000., 0.f, -0.004523809999227524, -0.004523809999227524},
		{20999., -5.f, 0., 0.},
		{30000., 0.f, 0., 0.},
	};
	const FAnimKey RingPieceOpacityKeys[] = {
		{0., 0.f, 0., 0.},
		{15000., 1.f, 3.333333370392211e-05, 3.333333370392211e-05},
		{30000., 1.f, 0., 0.},
	};
	// NewAnimation_1 on BackgroundBlur_0's BlurStrength and Button_20's BackgroundColor alpha, over its first 0.25 s (their
	// sections end there and restore the tree's values, which are these ends).
	const FAnimKey RingScreenBlurKeys[] = {{0., 0.f, 0., 0.}, {15000., 5.f, 0., 0.}};
	const FAnimKey RingScreenVeilKeys[] = {{0., 0.f, 0., 0.}, {15000., 0.800000011920929f, 0., 0.}};
	// NewAnimation_2 on Button_107's scale: up from nothing to 1 by 0.1 s, 1.1 at 0.2 s, 1 at 0.3 s.
	const FAnimKey RingScreenCloseScaleKeys[] = {
		{0., 0.f, 0., 0.},
		{6000., 1.f, 9.16666685952805e-05, 9.16666685952805e-05},
		{12000., 1.100000023841858f, 0., 0.},
		{18000., 1.f, 0., 0.},
	};

	// The tree's slots (CanvasPanel_0's, in the order they draw).
	const FAnchors RingScreenFill(0.f, 0.f, 1.f, 1.f);
	// CanvasPanelSlot_2: the blur, a little past every edge.
	const FMargin RingScreenBlurOffsets(-40.f, -36.f, -11.951940536499023f, -21.027000427246094f);
	// CanvasPanelSlot_0: Button_20's black, past every edge.
	const FMargin RingScreenVeilOffsets(-29.7056884765625f, -24.312286376953125f, -50.7747802734375f, -29.201171875f);
	// CanvasPanelSlot_1: the piece in the middle, at its own size (its offsets' right and bottom are the slot's
	// defaults and the export's 40, which the size overrides).
	const FAnchors RingScreenMiddle(0.5f, 0.5f);
	const FMargin RingPieceOffsets(0.f, 0.f, 100.f, 40.f);
	const FVector2D RingPieceSize(200.f, 200.f);
	// CanvasPanelSlot_3: CLOSE, anchored at the bottom middle, its top 196 px up, centred across, at its own size.
	const FAnchors RingScreenBottomMiddle(0.5f, 1.f);
	const FMargin RingScreenCloseOffsets(0.f, -195.9524383544922f, 100.f, 30.f);
	const FVector2D RingScreenCloseAlignment(0.5f, 0.f);

	// Button_107: its ColorAndOpacity greys CLOSE (white while hovered), its brush unseen (BackgroundColor's alpha 0).
	constexpr float RingScreenCloseGrey = 0.11400000005960464f;
	const FLinearColor RingScreenCloseBackground(1.f, 1.f, 1.f, 0.f);
	// TextBlock_0.
	constexpr float RingScreenCloseFontSize = 36.f;

	UCanvasPanelSlot* PlaceOnRingScreen(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
		const FVector2D& Alignment, bool bAutoSize)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = Anchors;
		Layout.Offsets = Offsets;
		Layout.Alignment = Alignment;
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(bAutoSize);
		return Slot;
	}

	AWasamiGameMode* RingScreenMode(const UUserWidget* Widget)
	{
		UWorld* World = Widget->GetWorld();
		return World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	}

	/** An animation Seconds in, moved on by DeltaSeconds forwards or back and kept to [0, Length]; unplayed stays so. */
	float MoveAnimationOn(float Seconds, float DeltaSeconds, bool bReverse, float Length)
	{
		if (Seconds < 0.f)
		{
			return Seconds;
		}
		return bReverse ? FMath::Max(Seconds - DeltaSeconds, 0.f) : FMath::Min(Seconds + DeltaSeconds, Length);
	}
}

UWasamiRingPieceWidget::UWasamiRingPieceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RingPieceTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/Textures/Ring_Assets/T_RingPiece_1")));
	CloseFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

UWasamiRingPieceWidget* UWasamiRingPieceWidget::Show(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	UWasamiRingPieceWidget* Screen = CreateWidget<UWasamiRingPieceWidget>(World, StaticClass());
	if (Screen && World->GetGameViewport())
	{
		Screen->AddToViewport(0);
	}
	return Screen;
}

TSharedRef<SWidget> UWasamiRingPieceWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;

		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_0"));
		Blur->SetBlurStrength(5.f);
		Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceOnRingScreen(Root, Blur, RingScreenFill, RingScreenBlurOffsets, FVector2D::ZeroVector, false);

		// Button_20 never takes input (HitTestInvisible) and draws its brush tinted black under BackgroundColor, so it is
		// a border with that black (as the death screen's Button_22).
		Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Button_20"));
		Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.800000011920929f));
		Veil->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceOnRingScreen(Root, Veil, RingScreenFill, RingScreenVeilOffsets, FVector2D::ZeroVector, false);

		RingPiece = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ringpiece"));
		FSlateBrush Brush;
		Brush.SetResourceObject(RingPieceTexture.LoadSynchronous());
		Brush.ImageSize = RingPieceSize;
		RingPiece->SetBrush(Brush);
		RingPiece->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceOnRingScreen(Root, RingPiece, RingScreenMiddle, RingPieceOffsets, FVector2D(0.5f, 0.5f), true);

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_107"));
		CloseButton->SetColorAndOpacity(FLinearColor(RingScreenCloseGrey, RingScreenCloseGrey, RingScreenCloseGrey, 1.f));
		CloseButton->SetBackgroundColor(RingScreenCloseBackground);
		CloseButton->SetRenderScale(FVector2D::ZeroVector);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_0"));
		Label->SetText(FText::FromString(TEXT("CLOSE")));
		FSlateFontInfo Font;
		Font.FontObject = CloseFont.LoadSynchronous();
		Font.TypefaceFontName = TEXT("Default");
		Font.Size = RingScreenCloseFontSize;
		Label->SetFont(Font);
		CloseButton->AddChild(Label);
		PlaceOnRingScreen(Root, CloseButton, RingScreenBottomMiddle, RingScreenCloseOffsets, RingScreenCloseAlignment, true);
		// The original's ComponentDelegateBinding: the click and the hover.
		CloseButton->OnClicked.AddDynamic(this, &UWasamiRingPieceWidget::OnCloseClicked);
		CloseButton->OnHovered.AddDynamic(this, &UWasamiRingPieceWidget::OnCloseHovered);
		CloseButton->OnUnhovered.AddDynamic(this, &UWasamiRingPieceWidget::OnCloseUnhovered);
	}
	return Super::RebuildWidget();
}

void UWasamiRingPieceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct: ringpiece's brush from ringpiece_texture (at the brush's size), NewAnimation_1, the game paused, the
	// cursor, the input to the screen (nothing focused, the mouse not locked); the Delay goes on in Begin.
	if (RingPiece)
	{
		RingPiece->SetBrushFromTexture(RingPieceTexture.LoadSynchronous(), false);
	}
	Begin();
	UGameplayStatics::SetGamePaused(this, true);
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		Controller->SetShowMouseCursor(true);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, nullptr, EMouseLockMode::DoNotLock);
	}
}

void UWasamiRingPieceWidget::NativeDestruct()
{
	// Destruct: Unpause Time Counter.
	if (AWasamiGameMode* Mode = RingScreenMode(this))
	{
		Mode->UnpauseTimeCounter();
	}
	Super::NativeDestruct();
}

void UWasamiRingPieceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiRingPieceWidget::Begin()
{
	Pending.Reset();
	Elapsed = 0.f;
	StepTime = 0.f;
	InTime = 0.f;
	bInReverse = false;
	CloseTime = -1.f;
	bCloseReverse = false;
	bClosing = false;
	bClosed = false;
	Schedule(EStep::ShowClose, CloseButtonDelay);
	ApplyAnimations();
}

void UWasamiRingPieceWidget::Advance(float DeltaSeconds)
{
	if (bClosed)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	InTime = MoveAnimationOn(InTime, DeltaSeconds, bInReverse, AnimationLength);
	CloseTime = MoveAnimationOn(CloseTime, DeltaSeconds, bCloseReverse, AnimationLength);
	// The Delays that ran out, earliest first (a step may schedule another that is due at once).
	for (;;)
	{
		int32 Next = INDEX_NONE;
		for (int32 Index = 0; Index < Pending.Num(); ++Index)
		{
			if (Pending[Index].At <= Elapsed && (Next == INDEX_NONE || Pending[Index].At < Pending[Next].At))
			{
				Next = Index;
			}
		}
		if (Next == INDEX_NONE || bClosed)
		{
			break;
		}
		const FPendingStep Due = Pending[Next];
		Pending.RemoveAt(Next);
		StepTime = Due.At;
		RunStep(Due.Step);
	}
	ApplyAnimations();
}

void UWasamiRingPieceWidget::Schedule(EStep Step, float Delay)
{
	Pending.Add({StepTime + Delay, Step});
}

void UWasamiRingPieceWidget::RunStep(EStep Step)
{
	switch (Step)
	{
	case EStep::ShowClose:
		// NewAnimation_2 from the start, and Pause Time Counter.
		CloseTime = 0.f;
		bCloseReverse = false;
		if (AWasamiGameMode* Mode = RingScreenMode(this))
		{
			Mode->PauseTimeCounter();
		}
		break;

	case EStep::ReverseIn:
		// NewAnimation_1 back from its end.
		InTime = AnimationLength;
		bInReverse = true;
		Schedule(EStep::Leave, LeaveDelay);
		break;

	case EStep::Leave:
		// The cursor off, the input to the game, the game unpaused, Close, and off the screen.
		if (GetWorld())
		{
			if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
			{
				Controller->SetShowMouseCursor(false);
				UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
			}
			UGameplayStatics::SetGamePaused(this, false);
		}
		bClosed = true;
		OnClose.Broadcast();
		RemoveFromParent();
		break;
	}
}

void UWasamiRingPieceWidget::PressClose()
{
	// The DoOnce, then NewAnimation_2 back from its end and the Delay before NewAnimation_1 goes back.
	if (bClosing || bClosed)
	{
		return;
	}
	bClosing = true;
	CloseTime = AnimationLength;
	bCloseReverse = true;
	// A click comes between ticks: its Delay counts from now.
	StepTime = Elapsed;
	Schedule(EStep::ReverseIn, ReverseDelay);
	ApplyAnimations();
}

void UWasamiRingPieceWidget::ApplyAnimations()
{
	const float In = FMath::Max(InTime, 0.f);
	if (RingPiece)
	{
		const float Scale = EvaluatePieceScale(In);
		RingPiece->SetRenderScale(FVector2D(Scale, Scale));
		RingPiece->SetRenderTransformAngle(EvaluatePieceAngle(In));
		RingPiece->SetRenderOpacity(EvaluatePieceOpacity(In));
	}
	if (Blur)
	{
		Blur->SetBlurStrength(EvaluateBlurStrength(In));
	}
	if (Veil)
	{
		Veil->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, EvaluateVeilAlpha(In)));
	}
	if (CloseButton)
	{
		// Unplayed, the tree's scale 0.
		const float Scale = CloseTime < 0.f ? 0.f : EvaluateCloseScale(CloseTime);
		CloseButton->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UWasamiRingPieceWidget::OnCloseClicked()
{
	PressClose();
}

void UWasamiRingPieceWidget::OnCloseHovered()
{
	CloseButton->SetColorAndOpacity(FLinearColor::White);
}

void UWasamiRingPieceWidget::OnCloseUnhovered()
{
	CloseButton->SetColorAndOpacity(FLinearColor(RingScreenCloseGrey, RingScreenCloseGrey, RingScreenCloseGrey, 1.f));
}

float UWasamiRingPieceWidget::EvaluatePieceScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RingPieceScaleKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiRingPieceWidget::EvaluatePieceAngle(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RingPieceAngleKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiRingPieceWidget::EvaluatePieceOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RingPieceOpacityKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiRingPieceWidget::EvaluateBlurStrength(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RingScreenBlurKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiRingPieceWidget::EvaluateVeilAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RingScreenVeilKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiRingPieceWidget::EvaluateCloseScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RingScreenCloseScaleKeys);
	return Eval(Curve, Seconds, AnimationLength);
}
