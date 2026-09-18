#include "WasamiSavingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Throbber.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// init (pak_reference_2's UMG_Saving): TextBlock_232's and Throbber_207's RenderOpacity, 0 → 0.5 (0.5 s) → 0.5 (2 s)
	// → 0 (2.5 s) → 0 (3 s). The words' keys were saved flat, the throbber's middle two with UE's auto tangents.
	const FAnimKey TextKeys[] = {{0., 0.f, 0., 0.}, {30000., 0.5f, 0., 0.}, {120000., 0.5f, 0., 0.}, {150000., 0.f, 0., 0.},
		{180000., 0.f, 0., 0.}};
	const FAnimKey ThrobberKeys[] = {{0., 0.f, 0., 0.}, {30000., 0.5f, 4.1666667129902635e-06, 4.1666667129902635e-06},
		{120000., 0.5f, -4.1666667129902635e-06, -4.1666667129902635e-06}, {150000., 0.f, 0., 0.}, {180000., 0.f, 0., 0.}};

	void PlaceInSaving(UCanvasPanel* Panel, UWidget* Child, const FMargin& Offsets)
	{
		// Both hang from the bottom right corner at their size (bAutoSize), aligned by their top left.
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = FAnchors(1.f, 1.f);
		Layout.Offsets = Offsets;
		Layout.Alignment = FVector2D::ZeroVector;
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(true);
	}
}

UWasamiSavingWidget::UWasamiSavingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TextFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Engine/EngineFonts/RobotoTiny")));
	ThrobberTexture = TSoftObjectPtr<UTexture2D>(
		WasamiAssets::Path(TEXT("/Engine/Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap")));
}

UWasamiSavingWidget* UWasamiSavingWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiSavingWidget* Widget = CreateWidget<UWasamiSavingWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(0);
	}
	return Widget;
}

TSharedRef<SWidget> UWasamiSavingWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;

		// TextBlock_232: SAVING PROGRESS in RobotoTiny's Light at UMG's default size (24), 300 px left of and 60 px
		// above the corner.
		Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_232"));
		Text->SetText(FText::FromString(TEXT("SAVING PROGRESS")));
		FSlateFontInfo Font;
		Font.FontObject = TextFont.LoadSynchronous();
		Font.TypefaceFontName = TEXT("Light");
		Font.Size = 24.f;
		Text->SetFont(Font);
		Text->SetRenderOpacity(0.f);
		PlaceInSaving(Root, Text, FMargin(-300.f, -60.f, 151.f, 40.f));

		// Throbber_207: one 25 × 25 piece that stays put (its opacity still pulses, as UMG's default), left of the words.
		Throbber = WidgetTree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), TEXT("Throbber_207"));
		Throbber->SetNumberOfPieces(1);
		Throbber->SetAnimateHorizontally(false);
		Throbber->SetAnimateVertically(false);
		FSlateBrush Piece;
		Piece.SetResourceObject(ThrobberTexture.LoadSynchronous());
		Piece.ImageSize = FVector2D(25.f, 25.f);
		Throbber->SetImage(Piece);
		Throbber->SetRenderOpacity(0.f);
		PlaceInSaving(Root, Throbber, FMargin(-332.f, -56.f, 22.63878059387207f, 40.f));
	}
	return Super::RebuildWidget();
}

void UWasamiSavingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Construct: PlayAnimation(init) and Delay 3 → RemoveFromParent.
	Elapsed = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiSavingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiSavingWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// The widget's own tick moves both on, so they go on while the game is paused.
	Elapsed += DeltaSeconds;
	ApplyAnimation();
	if (Elapsed >= RemoveDelay)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

void UWasamiSavingWidget::ApplyAnimation()
{
	if (Text)
	{
		Text->SetRenderOpacity(EvaluateTextOpacity(Elapsed));
	}
	if (Throbber)
	{
		Throbber->SetRenderOpacity(EvaluateThrobberOpacity(Elapsed));
	}
}

float UWasamiSavingWidget::EvaluateTextOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(TextKeys);
	return Eval(Curve, Seconds, InitLength);
}

float UWasamiSavingWidget::EvaluateThrobberOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ThrobberKeys);
	return Eval(Curve, Seconds, InitLength);
}
