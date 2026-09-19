#include "WasamiLevelClearWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiSaveGame.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;
	using WasamiWidgetAnimation::TicksPerSecond;

	// ClearAnimation (pak_reference's UMG_LevelClear), its keys as exported with UE's auto tangents. A section that
	// starts later leaves its widget at the widget's own value before it (as UMG does outside a section).
	// The whole widget's RenderOpacity: in over 0.25 s.
	const FAnimKey RootOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};
	// ResultsBox's RenderOpacity: in from 3 s to 3.25 s.
	const FAnimKey ResultsOpacityKeys[] = {{0., 0.f, 0., 0.}, {180000., 0.f, 5.128205430082744e-06, 5.128205430082744e-06},
		{195000., 1.f, 0., 0.}};
	// Image_216 (You Escaped!): its RenderOpacity in from 0.5 s to 0.75 s, and its RenderTransform (a section from
	// 30000) turning from 45° past -10° and shrinking from twice its size past 1.1 as it lands.
	const FAnimKey EscapedOpacityKeys[] = {{0., 0.f, 0., 0.}, {30000., 0.f, 2.2222222469281405e-05, 2.2222222469281405e-05},
		{45000., 1.f, 0., 0.}};
	constexpr double EscapedTransformStart = 30000.;
	const FAnimKey EscapedAngleKeys[] = {{30000., 45.f, 0., 0.}, {45000., 0.f, -0.0030555555131286383, -0.0030555555131286383},
		{48000., -10.f, 0., 0.}, {56999., 0.f, 0., 0.}};
	const FAnimKey EscapedScaleKeys[] = {{30000., 2.f, 0., 0.}, {45000., 1.f, -4.999999509891495e-05, -4.999999509891495e-05},
		{48000., 1.100000023841858f, 0., 0.}, {56999., 1.f, 0., 0.}};
	// ClearLevel: its RenderOpacity in over 0.25 s and out from 2.75 s to 3 s, and its translation (a section from
	// 45000) jolted as the lettering lands.
	const FAnimKey ClearOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 6.060606210667174e-06, 6.060606210667174e-06},
		{165000., 1.f, -6.060606210667174e-06, -6.060606210667174e-06}, {180000., 0.f, 0., 0.}};
	constexpr double ClearJoltStart = 45000.;
	const FAnimKey ClearJoltXKeys[] = {{45000., 0.f, 0., 0.}, {48000., 6.f, -0.00033333327155560255, -0.00033333327155560255},
		{51000., -2.f, -0.0006666667759418488, -0.0006666667759418488}, {56999., 0.f, 0., 0.}};
	const FAnimKey ClearJoltYKeys[] = {{45000., 0.f, 0., 0.}, {48000., 3.f, -0.001166666392236948, -0.001166666392236948},
		{51000., -7.f, -0.0003333333879709244, -0.0003333333879709244}, {56999., 0.f, 0., 0.}};
	// Image_6 (the white vignette) and Image_7 (white over the screen, a section from 41999): ColorAndOpacity's alpha,
	// the flash as the lettering lands (red, green and blue have no keys and stay the image's white).
	const FAnimKey WhiteVignetteAlphaKeys[] = {{0., 0.f, 0., 0.}, {41999., 0.f, 2.2222222469281405e-05, 2.2222222469281405e-05},
		{45000., 1.f, 0., 0.}, {60000., 0.f, 0., 0.}};
	constexpr double FlashStart = 41999.;
	const FAnimKey FlashAlphaKeys[] = {{41999., 0.f, 0., 0.}, {45000., 1.f, 0., 0.}, {51000., 0.f, 0., 0.}};
	// easymode's RenderOpacity (a section from 165000): in from 3 s to 3.25 s.
	constexpr double EasyStart = 165000.;
	const FAnimKey EasyOpacityKeys[] = {{165000., 0.f, 0., 0.}, {180000., 0.f, 3.333333370392211e-05, 3.333333370392211e-05},
		{195000., 1.f, 0., 0.}};

	// A row's animation (Time Animation … Shard Streak Animation, pak_reference's, all alike). The value: RenderOpacity
	// in over 0.25 s, its scale from 1.5 past 1.05. Total Shards Animation plays the same two on TotalShardAmount.
	const FAnimKey RowValueOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};
	const FAnimKey RowValueScaleKeys[] = {{0., 1.5f, 0., 0.}, {15000., 1.f, -2.500000482541509e-05, -2.500000482541509e-05},
		{17999., 1.0499999523162842f, 0., -1.6666668670950457e-05}, {29999., 1.f, 0., 3.333333370392211e-05}};
	// The rank: in from 0.25 s to 0.5 s, its scale (a section from 15000) likewise.
	const FAnimKey RowRankOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 0.f, 3.333333370392211e-05, 3.333333370392211e-05},
		{30000., 1.f, 0., 0.}};
	constexpr double RowRankScaleStart = 15000.;
	const FAnimKey RowRankScaleKeys[] = {{15000., 1.5f, 0., 0.}, {30000., 1.f, -2.5000003006425686e-05, -2.5000003006425686e-05},
		{33000., 1.0499999523162842f, 0., -1.5151516890909988e-05}, {44999., 1.f, 0., 3.333333370392211e-05}};
	// The "+N": in from 0.5 s to 0.75 s, its scale (a section from 30000) likewise.
	const FAnimKey RowShardsOpacityKeys[] = {{0., 0.f, 0., 0.}, {30000., 0.f, 2.2222217012313195e-05, 2.2222217012313195e-05},
		{45000., 1.f, 0., 0.}};
	constexpr double RowShardsScaleStart = 30000.;
	const FAnimKey RowShardsScaleKeys[] = {{30000., 1.5f, 0., 0.}, {45000., 1.f, -2.5000003006425686e-05, -2.5000003006425686e-05},
		{48000., 1.0499999523162842f, 0., -1.5151516890909988e-05}, {59999., 1.f, 0., 3.333333370392211e-05}};
	// Final Rank Animation: FinalRank in over 0.25 s from 1.5 times its size past 1.15 (after it, 1: the section's last
	// key over the tree's 1.15), and CanvasPanel_0's translation (a section from 9000) jolted as the stamp lands.
	const FAnimKey FinalOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};
	const FAnimKey FinalScaleKeys[] = {{0., 1.5f, 0., 0.}, {6000., 1.f, -3.888889114023186e-05, -3.888889114023186e-05},
		{9000., 1.149999976158142f, 0., 0.}, {29999., 1.f, 0., 3.333333370392211e-05}};
	constexpr double FinalJoltStart = 9000.;
	const FAnimKey FinalJoltXKeys[] = {{9000., 0.f, 0., 0.}, {12000., 4.f, -0.0003333333588670939, -0.0003333333588670939},
		{15000., -2.f, -0.00044444447848945856, -0.00044444447848945856}, {20999., 0.f, 0., 0.}};
	const FAnimKey FinalJoltYKeys[] = {{9000., 0.f, 0., 0.}, {12000., -7.f, 0.0005000000237487257, 0.0005000000237487257},
		{15000., 3.f, 0.000777777808252722, 0.000777777808252722}, {20999., 0.f, 0., 0.}};
	// Fade Out: FadeOut's RenderOpacity in over 1 s.
	const FAnimKey FadeOpacityKeys[] = {{0., 0.f, 0., 0.}, {60000., 1.f, 0., 0.}};
	// The rows' audio tracks (Level_Clear_Grade_Stamp_v2), in ticks.
	constexpr double RowStampTicks[] = {24000., 23999., 23999., 27000., 27000., 27000.};
	// The rows' counters' spans (the 0.25 in Delay(0.25 / n)); SOUL SHARDS' event does nothing.
	constexpr float RowCountSpans[] = {0.25f, 0.f, 0.25f, 0.05f, 0.25f, 0.5f};
	constexpr int32 SoulShardsRow = 1;

	// The tree's colours.
	constexpr float HeadingGrey = 0.140625f;
	constexpr float RowGrey = 0.1412629932165146f;
	constexpr float FinalGrey = 0.41666701436042786f;
	constexpr float NextGrey = 0.11458300054073334f;
	const FLinearColor RowShardsPurple(0.19120199978351593f, 0.05286100134253502f, 0.30498701333999634f, 1.f);
	const FLinearColor TotalLabelPurple(0.19120199978351593f, 0.05448000133037567f, 0.30498701333999634f, 1.f);
	const FLinearColor TotalPurple(0.19098800420761108f, 0.05400799959897995f, 0.30498701333999634f, 1.f);

	/** One row of VerticalBox_142: its box, its label, its three bound or counted texts and their placeholders. */
	struct FRowDesign
	{
		const TCHAR* Box;
		const TCHAR* LabelName;
		const TCHAR* Label;
		const TCHAR* ValueName;
		const TCHAR* RankName;
		const TCHAR* ShardsName;
		// The "+N" as designed, which the row's counter writes over (SoulShardsShards has none).
		const TCHAR* Shards;
		// The value's RenderTransform scale (Time's alone is 1.05).
		float ValueScale;
	};
	const FRowDesign RowDesigns[] = {
		{TEXT("TimeBox"), TEXT("TextBlock_316"), TEXT("TIME"), TEXT("Time"), TEXT("TimeRank"), TEXT("TimeShards"), TEXT("+25"), 1.0499999523162842f},
		{TEXT("SoulShardsBox"), TEXT("TextBlock_320"), TEXT("SOUL SHARDS"), TEXT("SoulShards"), TEXT("SoulShardsRank"), TEXT("SoulShardsShards"), TEXT(""), 1.f},
		{TEXT("BonusShardsBox"), TEXT("TextBlock_324"), TEXT("BONUS SHARDS"), TEXT("BonusShards"), TEXT("BonusShardsRank"), TEXT("BonusShardsShards"), TEXT("+25"), 1.f},
		{TEXT("SecretsBox"), TEXT("TextBlock_328"), TEXT("SECRETS"), TEXT("Secrets"), TEXT("SecretsRank"), TEXT("SecretsShards"), TEXT("+30"), 1.f},
		{TEXT("LivesLostBox"), TEXT("TextBlock_332"), TEXT("LIVES LOST"), TEXT("LivesLost"), TEXT("LivesLostRank"), TEXT("LivesLostShards"), TEXT("+21"), 1.f},
		{TEXT("ShardStreakBox"), TEXT("TextBlock_2"), TEXT("SHARD STREAK"), TEXT("ShardStreakText"), TEXT("ShardStreakRank"), TEXT("ShardStreakShards"), TEXT("+21"), 1.f},
	};

	UCanvasPanelSlot* LevelClearPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	UVerticalBoxSlot* LevelClearStack(UVerticalBox* Box, UWidget* Child, const FMargin& Padding, EHorizontalAlignment Horizontal,
		EVerticalAlignment Vertical)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child);
		Slot->SetPadding(Padding);
		Slot->SetHorizontalAlignment(Horizontal);
		Slot->SetVerticalAlignment(Vertical);
		return Slot;
	}

	UHorizontalBoxSlot* LevelClearLine(UHorizontalBox* Box, UWidget* Child, const FMargin& Padding, EHorizontalAlignment Horizontal,
		EVerticalAlignment Vertical)
	{
		UHorizontalBoxSlot* Slot = Box->AddChildToHorizontalBox(Child);
		Slot->SetPadding(Padding);
		Slot->SetHorizontalAlignment(Horizontal);
		Slot->SetVerticalAlignment(Vertical);
		return Slot;
	}

	FSlateBrush LevelClearBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	/** The engine's Black and WhiteSquareTexture (tinted) as the original's images use them: a plain colour. */
	FSlateBrush LevelClearColourBrush(const FLinearColor& Colour)
	{
		FSlateBrush Brush = FSlateColorBrush(Colour);
		Brush.ImageSize = FVector2D(32.f, 32.f);
		return Brush;
	}

	FSlateFontInfo LevelClearFont(UFont* Object, float Size)
	{
		FSlateFontInfo Info;
		Info.FontObject = Object;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		return Info;
	}

	void LevelClearScale(UWidget* Widget, float Scale)
	{
		FWidgetTransform Transform = Widget->GetRenderTransform();
		Transform.Scale = FVector2D(Scale, Scale);
		Widget->SetRenderTransform(Transform);
	}

	void LevelClearAlpha(UImage* Image, float Alpha)
	{
		FLinearColor Colour = Image->GetColorAndOpacity();
		Colour.A = Alpha;
		Image->SetColorAndOpacity(Colour);
	}

	/** A section's curve at Seconds, or Rest before the section starts. */
	float LevelClearEvalFrom(const FRichCurve& Curve, float Seconds, double StartTicks, float Rest)
	{
		return Seconds < StartTicks / TicksPerSecond ? Rest : Eval(Curve, Seconds, UWasamiLevelClearWidget::ClearLength);
	}

	// A debug command: the screen as Escape shows it, with the save's results.
	FAutoConsoleCommandWithWorldAndArgs LevelClearCommand(TEXT("Wasami.LevelClear"),
		TEXT("Shows the level clear screen with the save's results (the time counter added), without Escape's pause or save; ")
		TEXT("NEXT takes it off at Finished."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			AWasamiGameMode* Mode = World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
			if (Mode && Mode->GetSave())
			{
				FWasamiLevelProgress Progress = Mode->GetSave()->Hospital;
				Progress.Time += Mode->GetTime();
				if (UWasamiLevelClearWidget* Screen = UWasamiLevelClearWidget::Show(World, FWasamiLevelResults::ForHospital(Progress, false)))
				{
					Screen->bRemoveWhenFinished = true;
				}
			}
		}));
}

UWasamiLevelClearWidget::UWasamiLevelClearWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EscapedTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/you_escaped")));
	RuleTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/results_window")));
	LevelNameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy")));
	VignetteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Streaks/T_Vignette")));
	TextFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
	EscapedSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_YouEscaped")));
	RowStampSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Level_Clear_Grade_Stamp_v2")));
	FinalStampSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Level_Clear_Grade_Stamp_v1")));
	FillSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_XP_Bar_Fill_V2A_0617")));
}

UWasamiLevelClearWidget* UWasamiLevelClearWidget::Show(const UObject* WorldContextObject, const FWasamiLevelResults& InResults)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiLevelClearWidget* Widget = CreateWidget<UWasamiLevelClearWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->Results = InResults;
		Widget->AddToViewport(ViewportZOrder);
	}
	return Widget;
}

TSharedRef<SWidget> UWasamiLevelClearWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;
		RootPanel = Root;
		BuildScreen(Root);
	}
	return Super::RebuildWidget();
}

void UWasamiLevelClearWidget::BuildScreen(UCanvasPanel* Root)
{
	// Each slot is the original's, in the original's order (which is also the order they draw in); the brushes and
	// fonts hold on to what is loaded here.
	UFont* Bold = TextFont.LoadSynchronous();
	UTexture2D* Rule = RuleTexture.LoadSynchronous();
	const FAnchors Fill(0.f, 0.f, 1.f, 1.f);
	auto MakeText = [this, Bold](const TCHAR* Name, const TCHAR* Text, float Size, const FLinearColor& Colour)
	{
		UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(FText::FromString(Text));
		Block->SetFont(LevelClearFont(Bold, Size));
		Block->SetColorAndOpacity(FSlateColor(Colour));
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	};
	auto MakeImage = [this](const TCHAR* Name, const FSlateBrush& Brush)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	};

	// Image_4: black past every edge, under everything.
	LevelClearPlace(Root, MakeImage(TEXT("Image_4"), LevelClearColourBrush(FLinearColor::Black)), Fill,
		FMargin(-38.03803634643555f, -32.03203201293945f, -63.9639892578125f, -40.010009765625f), FVector2D::ZeroVector, false);

	// ResultsBox: 1820 × 980 at the middle, clear until ClearAnimation's last quarter second.
	ResultsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResultsBox"));
	ResultsBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	ResultsBox->SetRenderOpacity(0.f);
	LevelClearPlace(Root, ResultsBox, FAnchors(0.5f, 0.5f), FMargin(0.f, 0.f, 1820.f, 980.f), FVector2D(0.5f, 0.5f), false);
	{
		// LevelName: the level's title at 648.72 × 129.6, tinted red (Construct's SetBrushFromTexture keeps the size).
		LevelClearStack(ResultsBox, MakeImage(TEXT("LevelName"), LevelClearBrush(LevelNameTexture.LoadSynchronous(),
			FVector2D(648.7200317382812f, 129.60000610351562f), FLinearColor(1.f, 0.f, 0.f, 1.f))), FMargin(20.f), HAlign_Center, VAlign_Center);
		LevelClearStack(ResultsBox, MakeText(TEXT("TextBlock_0"), TEXT("RESULTS"), 36.f, FLinearColor(HeadingGrey, HeadingGrey, HeadingGrey, 1.f)),
			FMargin(20.f), HAlign_Center, VAlign_Fill);
		// Image_1: the rule, stretched across the box.
		LevelClearStack(ResultsBox, MakeImage(TEXT("Image_1"), LevelClearBrush(Rule, FVector2D(914.f, 18.f))), FMargin(10.f), HAlign_Fill, VAlign_Center);

		// VerticalBox_142: the six rows — the label 250 from the left, the value 500 on, the rank letter, the "+N".
		UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_142"));
		Rows->SetVisibility(ESlateVisibility::HitTestInvisible);
		LevelClearStack(ResultsBox, Rows, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		const FLinearColor Grey(RowGrey, RowGrey, RowGrey, 1.f);
		ValueTexts.Reset();
		RankTexts.Reset();
		ShardsTexts.Reset();
		for (const FRowDesign& Design : RowDesigns)
		{
			UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Design.Box);
			Box->SetVisibility(ESlateVisibility::HitTestInvisible);
			LevelClearStack(Rows, Box, FMargin(0.f), HAlign_Fill, VAlign_Center);

			UTextBlock* Label = MakeText(Design.LabelName, Design.Label, 35.f, Grey);
			Label->SetMinDesiredWidth(400.f);
			LevelClearLine(Box, Label, FMargin(250.f, 0.f, 0.f, 0.f), HAlign_Left, VAlign_Center);

			UTextBlock* Value = MakeText(Design.ValueName, TEXT(""), 30.f, Grey);
			Value->SetMinDesiredWidth(196.15191650390625f);
			Value->SetJustification(ETextJustify::Right);
			LevelClearScale(Value, Design.ValueScale);
			Value->SetRenderOpacity(0.f);
			LevelClearLine(Box, Value, FMargin(500.f, 0.f, 0.f, 0.f), HAlign_Left, VAlign_Center);

			UTextBlock* Rank = MakeText(Design.RankName, TEXT(""), 40.f, FLinearColor::Transparent);
			Rank->SetRenderOpacity(0.f);
			LevelClearLine(Box, Rank, FMargin(30.f, 0.f, 0.f, 0.f), HAlign_Center, VAlign_Center);

			UTextBlock* Shards = MakeText(Design.ShardsName, Design.Shards, 25.f, RowShardsPurple);
			Shards->SetMinDesiredWidth(243.00189208984375f);
			Shards->SetRenderOpacity(0.f);
			UHorizontalBoxSlot* ShardsSlot = LevelClearLine(Box, Shards, FMargin(65.f, 0.f, 0.f, 0.f), HAlign_Right, VAlign_Center);
			FSlateChildSize ShardsSize(ESlateSizeRule::Automatic);
			ShardsSize.Value = 0.5f;
			ShardsSlot->SetSize(ShardsSize);

			ValueTexts.Add(Value);
			RankTexts.Add(Rank);
			ShardsTexts.Add(Shards);
		}

		LevelClearStack(ResultsBox, MakeImage(TEXT("Image_2"), LevelClearBrush(Rule, FVector2D(914.f, 18.f))), FMargin(10.f), HAlign_Fill, VAlign_Center);

		// HorizontalBox_0: TOTAL SHARDS: and the total.
		UHorizontalBox* Total = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_0"));
		Total->SetVisibility(ESlateVisibility::HitTestInvisible);
		LevelClearStack(ResultsBox, Total, FMargin(0.f), HAlign_Center, VAlign_Center);
		UTextBlock* TotalLabel = MakeText(TEXT("TextBlock_1"), TEXT("TOTAL SHARDS:"), 30.f, TotalLabelPurple);
		TotalLabel->SetJustification(ETextJustify::Center);
		LevelClearLine(Total, TotalLabel, FMargin(10.f, 10.f, 0.f, 10.f), HAlign_Center, VAlign_Center);
		TotalShardAmount = MakeText(TEXT("TotalShardAmount"), TEXT(""), 50.f, TotalPurple);
		TotalShardAmount->SetMinDesiredWidth(140.66238403320312f);
		TotalShardAmount->SetJustification(ETextJustify::Center);
		TotalShardAmount->SetAutoWrapText(true);
		TotalShardAmount->SetRenderOpacity(0.f);
		LevelClearLine(Total, TotalShardAmount, FMargin(10.f), HAlign_Center, VAlign_Center);

		// HorizontalBox_1: FINAL RANK and its letter, 1.15 times its size.
		UHorizontalBox* Final = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_1"));
		Final->SetVisibility(ESlateVisibility::HitTestInvisible);
		LevelClearStack(ResultsBox, Final, FMargin(0.f), HAlign_Center, VAlign_Center);
		UTextBlock* FinalLabel = MakeText(TEXT("TextBlock_4"), TEXT("FINAL RANK"), 50.f, FLinearColor(FinalGrey, FinalGrey, FinalGrey, 1.f));
		FinalLabel->SetJustification(ETextJustify::Center);
		LevelClearLine(Final, FinalLabel, FMargin(10.f), HAlign_Center, VAlign_Center);
		FinalRank = MakeText(TEXT("FinalRank"), TEXT(""), 75.f, FLinearColor::Transparent);
		FinalRank->SetJustification(ETextJustify::Right);
		FinalRank->SetAutoWrapText(true);
		LevelClearScale(FinalRank, 1.149999976158142f);
		FinalRank->SetRenderOpacity(0.f);
		LevelClearLine(Final, FinalRank, FMargin(10.f), HAlign_Center, VAlign_Center);
	}

	// easymode: EASY MODE in UMG's default font under the middle (Construct takes it off unless EASY).
	EasyMode = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("easymode"));
	EasyMode->SetText(FText::FromString(TEXT("EASY MODE")));
	EasyMode->SetColorAndOpacity(FSlateColor(FLinearColor(FinalGrey, FinalGrey, FinalGrey, 1.f)));
	EasyMode->SetRenderOpacity(0.f);
	LevelClearPlace(Root, EasyMode, FAnchors(0.48359373211860657f, 0.9000000357627869f), FMargin(-213.8624267578125f, -74.5794906616211f, 100.f, 30.f),
		FVector2D::ZeroVector, false);

	// NextButton: NEXT at the bottom right. Its own brush is invisible (BackgroundColor's alpha 0) and its
	// ColorAndOpacity greys the text.
	NextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextButton"));
	NextButton->SetColorAndOpacity(FLinearColor(NextGrey, NextGrey, NextGrey, 1.f));
	NextButton->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
	UTextBlock* Next = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_314"));
	Next->SetText(FText::FromString(TEXT("NEXT")));
	Next->SetFont(LevelClearFont(Bold, 35.f));
	if (UButtonSlot* NextSlot = Cast<UButtonSlot>(NextButton->AddChild(Next)))
	{
		NextSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));
		NextSlot->SetHorizontalAlignment(HAlign_Right);
		NextSlot->SetVerticalAlignment(VAlign_Bottom);
	}
	LevelClearPlace(Root, NextButton, FAnchors(1.f, 1.f), FMargin(-163.37745666503906f, -89.0810546875f, 100.f, 30.f), FVector2D::ZeroVector, true);
	NextButton->OnClicked.AddDynamic(this, &UWasamiLevelClearWidget::OnNextClicked);
	NextButton->OnHovered.AddDynamic(this, &UWasamiLevelClearWidget::OnNextHovered);
	NextButton->OnUnhovered.AddDynamic(this, &UWasamiLevelClearWidget::OnNextUnhovered);

	// ClearLevel: the red past every edge (Image_5) and You Escaped! (Image_216, 1141 × 276 at the middle), over the
	// results until it leaves.
	ClearLevel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ClearLevel"));
	ClearLevel->SetVisibility(ESlateVisibility::HitTestInvisible);
	ClearLevel->SetRenderOpacity(0.f);
	LevelClearPlace(Root, ClearLevel, Fill, FMargin(-12.01202392578125f, -7.507506847381592f, -5.405272960662842f, -4.474365234375f),
		FVector2D::ZeroVector, false);
	LevelClearPlace(ClearLevel, MakeImage(TEXT("Image_5"), LevelClearColourBrush(FLinearColor(1.f, 0.f, 0.f, 1.f))), Fill,
		FMargin(-67.56756591796875f, -66.0660629272461f, -102.00202941894531f, -72.04203796386719f), FVector2D::ZeroVector, false);
	Escaped = MakeImage(TEXT("Image_216"), LevelClearBrush(EscapedTexture.LoadSynchronous(), FVector2D(1141.f, 276.f)));
	LevelClearPlace(ClearLevel, Escaped, FAnchors(0.5f, 0.5f), FMargin(0.f, 0.f, 580.48046875f, 314.7747497558594f), FVector2D(0.5f, 0.5f), true);

	// FadeOut: the black NEXT fades in, past every edge.
	FadeOut = MakeImage(TEXT("FadeOut"), LevelClearColourBrush(FLinearColor::Black));
	FadeOut->SetRenderOpacity(0.f);
	LevelClearPlace(Root, FadeOut, Fill, FMargin(-51.75832748413086f, -43.92692947387695f, -40.54238510131836f, -72.04203796386719f),
		FVector2D::ZeroVector, false);

	// Image_6 and Image_7: the white vignette and the white over the screen, clear until the flash.
	const FMargin Screen(0.f, 0.f, 1.9218759536743164f, 1.0810539722442627f);
	Vignette = MakeImage(TEXT("Image_6"), LevelClearBrush(VignetteTexture.LoadSynchronous(), FVector2D(1920.f, 1080.f)));
	Vignette->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	LevelClearPlace(Root, Vignette, Fill, Screen, FVector2D(0.5f, 0.5f), true);
	Flash = MakeImage(TEXT("Image_7"), LevelClearColourBrush(FLinearColor::White));
	Flash->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	LevelClearPlace(Root, Flash, Fill, Screen, FVector2D(0.5f, 0.5f), true);
}

void UWasamiLevelClearWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiLevelClearWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiLevelClearWidget::Begin()
{
	// Construct: PlayAnimation(ClearAnimation), Final Rank, LevelName by the level (the hospital's, set in the tree),
	// and easymode's RemoveFromParent unless the difficulty is EASY. The XP box's Shards Needed For Next Level and
	// TotalSoulShardsXP are left out with it.
	ApplyResults();
	if (EasyMode && !Results.bEasy)
	{
		EasyMode->RemoveFromParent();
	}
	Elapsed = 0.f;
	bEscapedSoundPlayed = false;
	bResultsShown = false;
	ResultsStep = 0;
	for (int32 Row = 0; Row < 6; ++Row)
	{
		RowTimes[Row] = -1.f;
		bRowStamped[Row] = false;
		bRowCounted[Row] = false;
	}
	TotalTime = -1.f;
	bTotalCounted = false;
	FinalTime = -1.f;
	bFinalStamped = false;
	for (FCounter& Counter : Counters)
	{
		Counter = FCounter();
	}
	FadeTime = -1.f;
	FinishRemaining = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiLevelClearWidget::NativeDestruct()
{
	// The fill sounds loop until their counters stop them; the level opening after Finished ends them in the original.
	for (FCounter& Counter : Counters)
	{
		if (UAudioComponent* Sound = Counter.Sound.Get())
		{
			Sound->Stop();
		}
		Counter.bDelaying = false;
	}
	Super::NativeDestruct();
}

void UWasamiLevelClearWidget::Advance(float DeltaSeconds)
{
	// As UUserWidget's tick: the playing animations move on, the latent actions (the Delays) count down, then the
	// animations' events. What starts in this tick counts from the next.
	Elapsed += DeltaSeconds;
	for (float& Time : RowTimes)
	{
		if (Time >= 0.f)
		{
			Time += DeltaSeconds;
		}
	}
	for (float* Time : {&TotalTime, &FinalTime, &FadeTime})
	{
		if (*Time >= 0.f)
		{
			*Time += DeltaSeconds;
		}
	}
	TickDelays(DeltaSeconds);
	// The audio track's section starts at 0.75 s: the sound from where the animation is in it.
	if (!bEscapedSoundPlayed && Elapsed >= EscapedSoundTime)
	{
		bEscapedSoundPlayed = true;
		if (Elapsed < ClearLength)
		{
			PlayUISound(EscapedSound, Elapsed - EscapedSoundTime);
		}
	}
	if (!bResultsShown && Elapsed >= ShowResultsTime)
	{
		bResultsShown = true;
		ShowResults();
	}
	TickResultsEvents();
	ApplyAnimation();
}

void UWasamiLevelClearWidget::ShowResults()
{
	// PlayAnimation(Time Animation) and the Delay to the next.
	ResultsStep = 0;
	RunResultsStep();
}

void UWasamiLevelClearWidget::TickDelays(float DeltaSeconds)
{
	// Each Delay as UE's: the time left less the tick's, done at 0 or below.
	if (ResultsStep > 0 && ResultsStep < ResultsSteps)
	{
		ResultsDelayRemaining -= DeltaSeconds;
		if (ResultsDelayRemaining <= 0.f)
		{
			RunResultsStep();
		}
	}
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Counters)); ++Index)
	{
		FCounter& Counter = Counters[Index];
		if (Counter.bDelaying)
		{
			Counter.DelayRemaining -= DeltaSeconds;
			if (Counter.DelayRemaining <= 0.f)
			{
				StepCounter(Index);
			}
		}
	}
	if (FadeTime >= 0.f && !bFinished)
	{
		FinishRemaining -= DeltaSeconds;
		if (FinishRemaining <= 0.f)
		{
			Finish();
		}
	}
}

void UWasamiLevelClearWidget::RunResultsStep()
{
	if (ResultsStep < 6)
	{
		RowTimes[ResultsStep] = 0.f;
	}
	else if (ResultsStep == 6)
	{
		TotalTime = 0.f;
	}
	else if (ResultsStep == 7)
	{
		FinalTime = 0.f;
	}
	else if (APlayerController* Controller = GetOwningPlayer())
	{
		// SetInputMode_UIOnlyEx and the mouse cursor: NEXT takes clicks from here.
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, this, EMouseLockMode::DoNotLock);
		Controller->SetShowMouseCursor(true);
	}
	++ResultsStep;
	if (ResultsStep < ResultsSteps)
	{
		ResultsDelayRemaining = ResultsDelays[ResultsStep - 1];
	}
}

void UWasamiLevelClearWidget::TickResultsEvents()
{
	// The rows' audio tracks (the stamp, from where the animation is in it) and event tracks (the counters at 0.5 s).
	const TArray<const FWasamiResultRow*, TFixedAllocator<6>> Rows = Results.GetRows();
	for (int32 Row = 0; Row < 6; ++Row)
	{
		if (RowTimes[Row] < 0.f)
		{
			continue;
		}
		const float Time = RowTimes[Row];
		if (!bRowStamped[Row] && Time >= RowStampTime(Row))
		{
			bRowStamped[Row] = true;
			PlayUISound(RowStampSound, Time - RowStampTime(Row));
		}
		if (!bRowCounted[Row] && Time >= RowCounterTime)
		{
			bRowCounted[Row] = true;
			if (Row != SoulShardsRow && Rows.IsValidIndex(Row))
			{
				StartCounter(Row, Rows[Row]->Shards, RowCountSpan(Row));
			}
		}
	}
	// Total Shards Animation's event at its start: TotalShardsCounter over the total.
	if (TotalTime >= 0.f && !bTotalCounted)
	{
		bTotalCounted = true;
		StartCounter(TotalCounter, Results.GetTotalShards(), TotalCountSpan);
	}
	if (FinalTime >= 0.f && !bFinalStamped && FinalTime >= FinalStampTime)
	{
		bFinalStamped = true;
		PlayUISound(FinalStampSound, FinalTime - FinalStampTime);
	}
}

void UWasamiLevelClearWidget::StartCounter(int32 Index, int32 Number, float Span)
{
	// CreateSound2D(UI_XP_Bar_Fill_V2A_0617, 1, 1, 0, None, False, True) → Play, then the loop's first pass at once.
	FCounter& Counter = Counters[Index];
	Counter = FCounter();
	Counter.Number = Number;
	Counter.Span = Span;
	USoundBase* Sound = GetWorld() ? FillSound.LoadSynchronous() : nullptr;
	if (UAudioComponent* Component = Sound ? UGameplayStatics::CreateSound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, false, true) : nullptr)
	{
		Component->Play(0.f);
		Counter.Sound = Component;
	}
	StepCounter(Index);
}

void UWasamiLevelClearWidget::StepCounter(int32 Index)
{
	// +1; below n: "+count" and Delay(span / n); at n: "+n" and the sound stops. TOTAL SHARDS' text is bound to the
	// total (Get_TotalShardAmount_Text_0), so its counter's SetText shows nothing: only its sound tells.
	FCounter& Counter = Counters[Index];
	UTextBlock* Text = ShardsTexts.IsValidIndex(Index) ? ShardsTexts[Index].Get() : nullptr;
	++Counter.Count;
	if (Counter.Count < Counter.Number)
	{
		if (Text)
		{
			Text->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Counter.Count)));
		}
		Counter.DelayRemaining = Counter.Span / static_cast<float>(Counter.Number);
		Counter.bDelaying = true;
		return;
	}
	if (Text)
	{
		Text->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Counter.Number)));
	}
	if (UAudioComponent* Sound = Counter.Sound.Get())
	{
		Sound->Stop();
	}
	Counter.bDelaying = false;
}

void UWasamiLevelClearWidget::PlayUISound(const TSoftObjectPtr<USoundBase>& Sound, float StartTime)
{
	// PlaySound2D plays a UI sound, which goes on while the game is paused.
	if (USoundBase* Loaded = GetWorld() ? Sound.LoadSynchronous() : nullptr)
	{
		UGameplayStatics::PlaySound2D(this, Loaded, 1.f, 1.f, StartTime);
	}
}

void UWasamiLevelClearWidget::PressNext()
{
	// NEXT when replaying (the XP box's count is left out): DoOnce → PlayAnimation(Fade Out), SetInputMode_GameOnly, no
	// cursor, Delay 4. A click comes between ticks: its Delay and the fade count from now.
	if (FadeTime >= 0.f)
	{
		return;
	}
	FadeTime = 0.f;
	FinishRemaining = FinishDelay;
	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
		Controller->SetShowMouseCursor(false);
	}
	ApplyAnimation();
}

void UWasamiLevelClearWidget::Finish()
{
	// Finished, then the game mode's Reset Game Instance(False): the game instance forgets the shards it collected.
	bFinished = true;
	OnFinished.Broadcast();
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		Instance->ForgetCollectedShards();
	}
	if (bRemoveWhenFinished)
	{
		RemoveFromParent();
	}
}

void UWasamiLevelClearWidget::OnNextClicked()
{
	PressNext();
}

void UWasamiLevelClearWidget::OnNextHovered()
{
	NextButton->SetColorAndOpacity(FLinearColor::White);
}

void UWasamiLevelClearWidget::OnNextUnhovered()
{
	// Unhovered Color, the tree's grey.
	NextButton->SetColorAndOpacity(FLinearColor(NextGrey, NextGrey, NextGrey, 1.f));
}

bool UWasamiLevelClearWidget::IsCounting(int32 Counter) const
{
	return Counter >= 0 && Counter < static_cast<int32>(UE_ARRAY_COUNT(Counters)) && Counters[Counter].bDelaying;
}

FText UWasamiLevelClearWidget::GetShardsText(int32 Row) const
{
	return ShardsTexts.IsValidIndex(Row) ? ShardsTexts[Row]->GetText() : FText::GetEmpty();
}

float UWasamiLevelClearWidget::RowStampTime(int32 Row)
{
	return Row >= 0 && Row < 6 ? static_cast<float>(RowStampTicks[Row] / TicksPerSecond) : 0.f;
}

float UWasamiLevelClearWidget::RowCountSpan(int32 Row)
{
	return Row >= 0 && Row < 6 ? RowCountSpans[Row] : 0.f;
}

void UWasamiLevelClearWidget::ApplyResults()
{
	const TArray<const FWasamiResultRow*, TFixedAllocator<6>> Rows = Results.GetRows();
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (ValueTexts.IsValidIndex(Index) && RankTexts.IsValidIndex(Index))
		{
			ValueTexts[Index]->SetText(Rows[Index]->Value);
			RankTexts[Index]->SetText(FWasamiLevelResults::RankText(Rows[Index]->Rank));
			RankTexts[Index]->SetColorAndOpacity(FSlateColor(FWasamiLevelResults::RankColor(Rows[Index]->Rank)));
		}
	}
	if (TotalShardAmount)
	{
		TotalShardAmount->SetText(FWasamiLevelResults::IntText(Results.GetTotalShards()));
	}
	if (FinalRank)
	{
		const uint8 Rank = Results.GetFinalRank();
		FinalRank->SetText(FWasamiLevelResults::RankText(Rank));
		FinalRank->SetColorAndOpacity(FSlateColor(FWasamiLevelResults::RankColor(Rank)));
	}
}

void UWasamiLevelClearWidget::ApplyAnimation()
{
	SetRenderOpacity(EvaluateRootOpacity(Elapsed));
	if (ResultsBox)
	{
		ResultsBox->SetRenderOpacity(EvaluateResultsOpacity(Elapsed));
	}
	if (Escaped)
	{
		Escaped->SetRenderOpacity(EvaluateEscapedOpacity(Elapsed));
		FWidgetTransform Transform = Escaped->GetRenderTransform();
		Transform.Angle = EvaluateEscapedAngle(Elapsed);
		const float Scale = EvaluateEscapedScale(Elapsed);
		Transform.Scale = FVector2D(Scale, Scale);
		Escaped->SetRenderTransform(Transform);
	}
	if (ClearLevel)
	{
		ClearLevel->SetRenderOpacity(EvaluateClearOpacity(Elapsed));
		ClearLevel->SetRenderTranslation(EvaluateClearJolt(Elapsed));
	}
	if (Vignette)
	{
		LevelClearAlpha(Vignette, EvaluateVignetteAlpha(Elapsed));
	}
	if (Flash)
	{
		LevelClearAlpha(Flash, EvaluateFlashAlpha(Elapsed));
	}
	if (EasyMode && EasyMode->GetParent())
	{
		EasyMode->SetRenderOpacity(EvaluateEasyOpacity(Elapsed));
	}

	// The rows', TOTAL SHARDS' and FINAL RANK's animations from when they started (untouched before), Fade Out from NEXT.
	for (int32 Row = 0; Row < 6; ++Row)
	{
		if (RowTimes[Row] < 0.f || !ValueTexts.IsValidIndex(Row) || !RankTexts.IsValidIndex(Row) || !ShardsTexts.IsValidIndex(Row))
		{
			continue;
		}
		const float Time = RowTimes[Row];
		ValueTexts[Row]->SetRenderOpacity(EvaluateRowValueOpacity(Time));
		LevelClearScale(ValueTexts[Row], EvaluateRowValueScale(Time));
		RankTexts[Row]->SetRenderOpacity(EvaluateRowRankOpacity(Time));
		LevelClearScale(RankTexts[Row], EvaluateRowRankScale(Time));
		ShardsTexts[Row]->SetRenderOpacity(EvaluateRowShardsOpacity(Time));
		LevelClearScale(ShardsTexts[Row], EvaluateRowShardsScale(Time));
	}
	if (TotalTime >= 0.f && TotalShardAmount)
	{
		const float Time = FMath::Min(TotalTime, TotalLength);
		TotalShardAmount->SetRenderOpacity(EvaluateRowValueOpacity(Time));
		LevelClearScale(TotalShardAmount, EvaluateRowValueScale(Time));
	}
	if (FinalTime >= 0.f && FinalRank)
	{
		const float Time = FinalTime;
		FinalRank->SetRenderOpacity(EvaluateFinalOpacity(Time));
		LevelClearScale(FinalRank, EvaluateFinalScale(Time));
		if (RootPanel)
		{
			RootPanel->SetRenderTranslation(EvaluateFinalJolt(Time));
		}
	}
	if (FadeTime >= 0.f && FadeOut)
	{
		FadeOut->SetRenderOpacity(EvaluateFadeOpacity(FadeTime));
	}
}

bool UWasamiLevelClearWidget::IsEasyModeShown() const
{
	return EasyMode && EasyMode->GetParent();
}

FText UWasamiLevelClearWidget::GetValueText(int32 Row) const
{
	return ValueTexts.IsValidIndex(Row) ? ValueTexts[Row]->GetText() : FText::GetEmpty();
}

FText UWasamiLevelClearWidget::GetRankText(int32 Row) const
{
	return RankTexts.IsValidIndex(Row) ? RankTexts[Row]->GetText() : FText::GetEmpty();
}

FLinearColor UWasamiLevelClearWidget::GetRankColor(int32 Row) const
{
	return RankTexts.IsValidIndex(Row) ? RankTexts[Row]->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::Transparent;
}

FText UWasamiLevelClearWidget::GetTotalText() const
{
	return TotalShardAmount ? TotalShardAmount->GetText() : FText::GetEmpty();
}

FText UWasamiLevelClearWidget::GetFinalRankText() const
{
	return FinalRank ? FinalRank->GetText() : FText::GetEmpty();
}

FLinearColor UWasamiLevelClearWidget::GetFinalRankColor() const
{
	return FinalRank ? FinalRank->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::Transparent;
}

float UWasamiLevelClearWidget::EvaluateRootOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RootOpacityKeys);
	return Eval(Curve, Seconds, ClearLength);
}

float UWasamiLevelClearWidget::EvaluateResultsOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ResultsOpacityKeys);
	return Eval(Curve, Seconds, ClearLength);
}

float UWasamiLevelClearWidget::EvaluateEscapedOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(EscapedOpacityKeys);
	return Eval(Curve, Seconds, ClearLength);
}

float UWasamiLevelClearWidget::EvaluateEscapedAngle(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(EscapedAngleKeys);
	return LevelClearEvalFrom(Curve, Seconds, EscapedTransformStart, 0.f);
}

float UWasamiLevelClearWidget::EvaluateEscapedScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(EscapedScaleKeys);
	return LevelClearEvalFrom(Curve, Seconds, EscapedTransformStart, 1.f);
}

float UWasamiLevelClearWidget::EvaluateClearOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ClearOpacityKeys);
	return Eval(Curve, Seconds, ClearLength);
}

FVector2D UWasamiLevelClearWidget::EvaluateClearJolt(float Seconds)
{
	static const FRichCurve X = MakeCurve(ClearJoltXKeys);
	static const FRichCurve Y = MakeCurve(ClearJoltYKeys);
	return FVector2D(LevelClearEvalFrom(X, Seconds, ClearJoltStart, 0.f), LevelClearEvalFrom(Y, Seconds, ClearJoltStart, 0.f));
}

float UWasamiLevelClearWidget::EvaluateVignetteAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(WhiteVignetteAlphaKeys);
	return Eval(Curve, Seconds, ClearLength);
}

float UWasamiLevelClearWidget::EvaluateFlashAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(FlashAlphaKeys);
	return LevelClearEvalFrom(Curve, Seconds, FlashStart, 0.f);
}

float UWasamiLevelClearWidget::EvaluateEasyOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(EasyOpacityKeys);
	return LevelClearEvalFrom(Curve, Seconds, EasyStart, 0.f);
}

float UWasamiLevelClearWidget::EvaluateRowValueOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RowValueOpacityKeys);
	return Eval(Curve, Seconds, RowLength);
}

float UWasamiLevelClearWidget::EvaluateRowValueScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RowValueScaleKeys);
	return Eval(Curve, Seconds, RowLength);
}

float UWasamiLevelClearWidget::EvaluateRowRankOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RowRankOpacityKeys);
	return Eval(Curve, Seconds, RowLength);
}

float UWasamiLevelClearWidget::EvaluateRowRankScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RowRankScaleKeys);
	return Seconds < RowRankScaleStart / TicksPerSecond ? 1.f : Eval(Curve, Seconds, RowLength);
}

float UWasamiLevelClearWidget::EvaluateRowShardsOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RowShardsOpacityKeys);
	return Eval(Curve, Seconds, RowLength);
}

float UWasamiLevelClearWidget::EvaluateRowShardsScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RowShardsScaleKeys);
	return Seconds < RowShardsScaleStart / TicksPerSecond ? 1.f : Eval(Curve, Seconds, RowLength);
}

float UWasamiLevelClearWidget::EvaluateFinalOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(FinalOpacityKeys);
	return Eval(Curve, Seconds, FinalLength);
}

float UWasamiLevelClearWidget::EvaluateFinalScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(FinalScaleKeys);
	return Eval(Curve, Seconds, FinalLength);
}

FVector2D UWasamiLevelClearWidget::EvaluateFinalJolt(float Seconds)
{
	static const FRichCurve X = MakeCurve(FinalJoltXKeys);
	static const FRichCurve Y = MakeCurve(FinalJoltYKeys);
	const float Time = FMath::Min(Seconds, FinalLength);
	return Time < FinalJoltStart / TicksPerSecond ? FVector2D::ZeroVector : FVector2D(Eval(X, Time, FinalLength), Eval(Y, Time, FinalLength));
}

float UWasamiLevelClearWidget::EvaluateFadeOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(FadeOpacityKeys);
	return Eval(Curve, Seconds, FadeLength);
}
