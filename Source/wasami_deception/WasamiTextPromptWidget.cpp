#include "WasamiTextPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// NewAnimation_1 (pak_reference_2's UMG_TextPrompt), both tracks keyed at 0, 0.25 s, 4.5 s and 5 s: the alpha 0 → 1,
	// held, → 0, and the y 98 → 0, held, → 110. The 0.25 s keys are linear (the hold between them is flat); the rest
	// cubic.
	const FAnimKey TextPromptOpacityKeys[] = {
		{0., 0.f, 0., 0.},
		{15000., 1.f, 3.703703669089009e-06, 3.703703669089009e-06, RCIM_Linear},
		{270000., 1.f, -3.5087718970316928e-06, -3.5087718970316928e-06},
		{300000., 0.f, 0., 0.},
	};
	const FAnimKey TextPromptTranslationKeys[] = {
		{0., 98.0560073852539f, 0., 0.},
		{15000., 0.f, -0.0003631704021245241, -0.0003631704021245241, RCIM_Linear},
		{270000., 0.f, 0.00038656144170090556, 0.00038656144170090556},
		{300000., 110.17000579833984f, 0., 0.},
	};

	// TextBlock_95: helvetica-normal (40 in the tree; Construct sets Font Size) with its drop shadow.
	constexpr float TextPromptTreeFontSize = 40.f;
	const FVector2D TextPromptShadowOffset(3.6068990230560303, 4.328499794006348);
	const FLinearColor TextPromptShadowColor(0.f, 0.f, 0.f, 0.6819999814033508f);
	// CanvasPanelSlot_0: anchored at the bottom middle, its top 217 px up, centred across, at its own size.
	const FAnchors TextPromptAnchors(0.5f, 1.f);
	const FMargin TextPromptOffsets(0.f, -217.0810546875f, 151.f, 40.f);
	const FVector2D TextPromptAlignment(0.5f, 0.f);
}

UWasamiTextPromptWidget::UWasamiTextPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PromptFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
}

UWasamiTextPromptWidget* UWasamiTextPromptWidget::Show(const UObject* WorldContextObject, const FText& InText)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	UWasamiTextPromptWidget* Prompt = CreateWidget<UWasamiTextPromptWidget>(World, StaticClass());
	if (Prompt)
	{
		Prompt->Text = InText;
		if (World->GetGameViewport() && Prompt->GetOwningLocalPlayer())
		{
			Prompt->AddToPlayerScreen(0);
		}
	}
	return Prompt;
}

TSharedRef<SWidget> UWasamiTextPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_95"));
		FSlateFontInfo Font;
		Font.FontObject = PromptFont.LoadSynchronous();
		Font.TypefaceFontName = TEXT("Default");
		Font.Size = TextPromptTreeFontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetShadowOffset(TextPromptShadowOffset);
		TextBlock->SetShadowColorAndOpacity(TextPromptShadowColor);
		UCanvasPanelSlot* PromptSlot = Root->AddChildToCanvas(TextBlock);
		FAnchorData Layout;
		Layout.Anchors = TextPromptAnchors;
		Layout.Offsets = TextPromptOffsets;
		Layout.Alignment = TextPromptAlignment;
		PromptSlot->SetLayout(Layout);
		PromptSlot->SetAutoSize(true);
	}
	return Super::RebuildWidget();
}

void UWasamiTextPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct: the font again at Font Size (its object, material, outline and typeface as they were), the text, and
	// NewAnimation_1 from the start once, whose end calls Destroy (RemoveFromParent).
	if (TextBlock)
	{
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = static_cast<float>(FontSize);
		TextBlock->SetFont(Font);
		TextBlock->SetText(Text);
	}
	AnimationTime = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiTextPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiTextPromptWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	AnimationTime = FMath::Min(AnimationTime + DeltaSeconds, AnimationLength);
	ApplyAnimation();
	if (AnimationTime >= AnimationLength)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

void UWasamiTextPromptWidget::ApplyAnimation()
{
	if (TextBlock)
	{
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, EvaluateOpacity(AnimationTime))));
		TextBlock->SetRenderTranslation(FVector2D(0.f, EvaluateTranslationY(AnimationTime)));
	}
}

float UWasamiTextPromptWidget::EvaluateOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(TextPromptOpacityKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiTextPromptWidget::EvaluateTranslationY(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(TextPromptTranslationKeys);
	return Eval(Curve, Seconds, AnimationLength);
}
