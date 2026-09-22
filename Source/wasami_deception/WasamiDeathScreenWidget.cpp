#include "WasamiDeathScreenWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Curves/RichCurve.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiPopUpWidget.h"
#include "WasamiPowerComponent.h"
#include "WasamiSaveGame.h"
#include "WasamiVoice.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	// The animations' keys as exported (pak_reference_2's UMG_DeathScreen; WasamiWidgetAnimation.h).
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Fade In / Fade Out: Button_0's BackgroundColor alpha; the author's tangent makes the black leave late and go fast.
	const FAnimKey FadeInKeys[] = {{0., 1.f, 0., 0.}, {60000., 0.f, -3.375811138539575e-05, -3.37581368512474e-05}};
	const FAnimKey FadeOutKeys[] = {{0., 0.f, -3.375811138539575e-05, -3.37581368512474e-05}, {59999., 1.f, 0., 0.}};

	// Shake: Image_161's alpha and scale, CanvasPanel_0's translation (its section is [6000, 18000]), and the lives'
	// green and blue (their section is [0, 18000]; red and alpha stay 1).
	const FAnimKey ShakeVignetteAlphaKeys[] = {{0., 0.f, 0., 0.}, {6000., 1.f, 2.0000001313746907e-05, 2.0000001313746907e-05},
		{15000., 0.30000001192092896f, -1.8518519937060773e-05, -1.8518519937060773e-05}, {60000., 0.f, 0., 0.}};
	const FAnimKey ShakeVignetteScaleKeys[] = {{0., 1.25f, 0., 0.}, {6000., 1.100000023841858f, -4.1666667129902635e-06, -4.1666667129902635e-06},
		{60000., 1.f, 0., 0.}};
	const FAnimKey ShakeXKeys[] = {{6000., 0.f, 0., 0.}, {9000., 10.f, -0.0008333333535119891, -0.0008333333535119891},
		{12000., -5.f, -0.0011111111380159855, -0.0011111111380159855}, {18000., 0.f, 0., 0.}};
	const FAnimKey ShakeYKeys[] = {{6000., 0.f, 0., 0.}, {9000., -10.f, 0.0008333333535119891, 0.0008333333535119891},
		{12000., 5.f, 0.0011111111380159855, 0.0011111111380159855}, {18000., 0.f, 0., 0.}};
	const FAnimKey ShakeLifeTintKeys[] = {{0., 1.f, 0., 0.}, {6000., 0.f, 0., 0.}, {18000., 1.f, 0., 0.}};

	// Death. Sections that start at 30000 (0.5 s) leave their widget alone before that.
	constexpr float DeathLateStart = 0.5f;
	// TextBlock_149 (REMAINING LIVES)'s alpha.
	const FAnimKey DeathHeadingKeys[] = {{0., 1.f, 0., 0.}, {30000., 0.f, -4.7619050747016445e-06, -4.7619050747016445e-06}, {210000., 0.f, 0., 0.}};
	// YouAreDead's alpha (from 0.5 s).
	const FAnimKey DeathDeadKeys[] = {{30000., 0.f, 0., 0.}, {150000., 1.f, 5.555555617320351e-06, 5.555555617320351e-06}, {210000., 1.f, 0., 0.}};
	// The three buttons' texts' alpha (RESTART's and QUIT TO TITLE's from 0.5 s, LAST CHECKPOINT's from 0) and
	// VerticalBox_161's RenderOpacity (from 0.5 s).
	const FAnimKey DeathMenuKeys[] = {{30000., 0.f, 0., 0.}, {150000., 1.f, 0., 0.}};
	// The buttons' ColorAndOpacity alpha: the track made for Restart is bound to LastCheckpoint, and QUITTOTITLE's
	// starts later; Restart's own stays 1.
	const FAnimKey DeathCheckpointKeys[] = {{30000., 0.f, 6.666666649834951e-06, 6.666666649834951e-06}, {150000., 1.f, 6.666666649834951e-06, 6.666666649834951e-06}};
	const FAnimKey DeathQuitKeys[] = {{0., 0.f, 0., 0.}, {60000., 0.f, 6.666666649834951e-06, 6.666666649834951e-06},
		{150000., 1.f, 6.666666649834951e-06, 6.666666649834951e-06}, {210000., 1.f, 0., 0.}};
	// Tips' RenderOpacity.
	const FAnimKey DeathTipKeys[] = {{0., 1.f, 0., 0.}, {30000., 0.f, 0., 0.}};

	// Bierce's death lines' Duration (pak_reference_2's SoundWaves): BierceDeathTraps (Shared/Bierce_Death_Traps_02..08)
	// and BierceDeathAsylum (Ch06/Bierce_Asylum_Death_01..04).
	const float TrapsVoiceLengths[] = {3.952176809310913f, 3.5742404460906982f, 2.873401403427124f, 3.2425169944763184f,
		4.371201992034912f, 3.2083446979522705f, 2.432267665863037f};
	const float AsylumVoiceLengths[] = {4.180272102355957f, 2.8550567626953125f, 2.4934165477752686f, 3.7052292823791504f};

	// The widget tree's colours: Image_161's tint, Tips', the buttons' ColorAndOpacity and their texts'.
	const FLinearColor VignetteTint(0.380207986f, 0.f, 0.f, 1.f);
	constexpr float TipGrey = 0.619791985f;
	constexpr float ButtonGrey = 0.114583001f;
	constexpr float ButtonTextGrey = 0.520833015f;
	// The buttons' hover: white, and back to Unhover Color.
	const FLinearColor UnhoverColour(ButtonGrey, ButtonGrey, ButtonGrey, 1.f);

	// The questions, as written.
	const TCHAR* const RestartQuestion = TEXT("ARE YOU SURE YOU WANT TO RESTART?");
	const TCHAR* const LastCheckpointWarning = TEXT("Obtaining S Rank is not possible with Last Checkpoint.\r\n \r\nContinue anyway?");

	AWasamiGameMode* GameModeOf(const UUserWidget* Widget)
	{
		UWorld* World = Widget->GetWorld();
		return World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	FSlateBrush TextureBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	FSlateFontInfo Font(UFont* Object, const FName Typeface, float Size)
	{
		FSlateFontInfo Info;
		Info.FontObject = Object;
		Info.TypefaceFontName = Typeface;
		Info.Size = Size;
		return Info;
	}

	void SetAlpha(UImage* Image, float Alpha)
	{
		if (Image)
		{
			FLinearColor Colour = Image->GetColorAndOpacity();
			Colour.A = Alpha;
			Image->SetColorAndOpacity(Colour);
		}
	}
}

UWasamiDeathScreenWidget::UWasamiDeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LifeTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/life_icon_02")));
	DeadTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/you_are_dead")));
	VignetteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Streaks/T_Vignette")));
	HeadingFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
	MenuFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	TipFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/_Engine/EngineFonts/RobotoTiny")));
	LifeLostSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Life_Lost")));
	GameOverSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/66_-_Game_Over")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
	LifeVoiceSound = TSoftObjectPtr<USoundBase>(WasamiVoice::Path(EWasamiVoice::Fine));
	GameOverVoiceSound = TSoftObjectPtr<USoundBase>(WasamiVoice::Path(EWasamiVoice::Over));
}

UWasamiDeathScreenWidget* UWasamiDeathScreenWidget::Show(const UObject* WorldContextObject, uint8 InLevel)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiDeathScreenWidget* Screen = CreateWidget<UWasamiDeathScreenWidget>(Controller, StaticClass());
	if (Screen)
	{
		Screen->Level = InLevel;
		Screen->AddToViewport(ViewportZOrder);
		UGameplayStatics::SetGamePaused(WorldContextObject, true);
	}
	return Screen;
}

TSharedRef<SWidget> UWasamiDeathScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = RootCanvas;
		BuildScreen(RootCanvas);
	}
	return Super::RebuildWidget();
}

void UWasamiDeathScreenWidget::BuildScreen(UCanvasPanel* Root)
{
	// The brushes and fonts below hold on to what is loaded here. Each slot is the original's CanvasPanelSlot, in the
	// original's order (which is also the order they draw in).
	UTexture2D* Life = LifeTexture.LoadSynchronous();
	UFont* ButtonFont = MenuFont.LoadSynchronous();
	const FAnchors Fill(0.f, 0.f, 1.f, 1.f);

	// Button_22: black past every edge, the row of lives (HorizontalBox_114) in its middle. Its button never takes
	// input (HitTestInvisible), so it is a border with the button's black brush.
	UBorder* Back = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Button_22"));
	Back->SetBrushColor(FLinearColor::Black);
	Back->SetHorizontalAlignment(HAlign_Center);
	Back->SetVerticalAlignment(VAlign_Center);
	Back->SetVisibility(ESlateVisibility::HitTestInvisible);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_114"));
	Back->AddChild(Row);
	LivesArray.Reset();
	for (int32 Index = 0; Index < LifeIcons; ++Index)
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Life%d"), Index + 1));
		Icon->SetBrush(TextureBrush(Life, FVector2D(100.f, 100.f)));
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
		IconSlot->SetPadding(FMargin(10.f));
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		LivesArray.Add(Icon);
	}
	Place(Root, Back, Fill, FMargin(-25.f), FVector2D::ZeroVector, false);

	// VerticalBox_161: RESTART, LAST CHECKPOINT, QUIT TO TITLE, hanging from 358 px above the bottom's middle. The
	// buttons' own brush is invisible (BackgroundColor's alpha 0) and their ColorAndOpacity greys the text.
	Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_161"));
	ButtonTexts.Reset();
	auto MakeButton = [this, ButtonFont](const TCHAR* Name, const TCHAR* TextName, const TCHAR* Label, const FLinearColor& Background)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetBackgroundColor(Background);
		Button->SetColorAndOpacity(FLinearColor(ButtonGrey, ButtonGrey, ButtonGrey, 1.f));
		Button->SetVisibility(ESlateVisibility::HitTestInvisible);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		Text->SetText(FText::FromString(Label));
		Text->SetFont(Font(ButtonFont, TEXT("Default"), 30.f));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(ButtonTextGrey, ButtonTextGrey, ButtonTextGrey, 0.f)));
		Button->AddChild(Text);
		UVerticalBoxSlot* ButtonSlot = Menu->AddChildToVerticalBox(Button);
		ButtonSlot->SetPadding(FMargin(10.f));
		ButtonTexts.Add(Text);
		return Button;
	};
	RestartButton = MakeButton(TEXT("Restart"), TEXT("TextBlock_0"), TEXT("RESTART"), FLinearColor(86.f, 86.f, 86.f, 0.f));
	LastCheckpointButton = MakeButton(TEXT("LastCheckpoint"), TEXT("TextBlock_2"), TEXT("LAST CHECKPOINT"), FLinearColor(86.f, 86.f, 86.f, 0.f));
	QuitButton = MakeButton(TEXT("QUITTOTITLE"), TEXT("TextBlock_1"), TEXT("QUIT TO TITLE "), FLinearColor(1.f, 1.f, 1.f, 0.f));
	// The original's ComponentDelegateBinding: each button's click and hover.
	RestartButton->OnClicked.AddDynamic(this, &UWasamiDeathScreenWidget::OnRestartClicked);
	RestartButton->OnHovered.AddDynamic(this, &UWasamiDeathScreenWidget::OnRestartHovered);
	RestartButton->OnUnhovered.AddDynamic(this, &UWasamiDeathScreenWidget::OnRestartUnhovered);
	LastCheckpointButton->OnClicked.AddDynamic(this, &UWasamiDeathScreenWidget::OnLastCheckpointClicked);
	LastCheckpointButton->OnHovered.AddDynamic(this, &UWasamiDeathScreenWidget::OnLastCheckpointHovered);
	LastCheckpointButton->OnUnhovered.AddDynamic(this, &UWasamiDeathScreenWidget::OnLastCheckpointUnhovered);
	QuitButton->OnClicked.AddDynamic(this, &UWasamiDeathScreenWidget::OnQuitClicked);
	QuitButton->OnHovered.AddDynamic(this, &UWasamiDeathScreenWidget::OnQuitHovered);
	QuitButton->OnUnhovered.AddDynamic(this, &UWasamiDeathScreenWidget::OnQuitUnhovered);
	Place(Root, Menu, FAnchors(0.5f, 1.f), FMargin(0.f, -358.1879577636719f, 0.f, 0.f), FVector2D(0.5f, 0.f), true);

	// TextBlock_149: REMAINING LIVES, 308 px from the top's middle.
	Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_149"));
	Heading->SetText(FText::FromString(TEXT("REMAINING LIVES:")));
	Heading->SetFont(Font(HeadingFont.LoadSynchronous(), TEXT("Default"), 50.f));
	Heading->SetJustification(ETextJustify::Center);
	Place(Root, Heading, FAnchors(0.5f, 0.f), FMargin(0.f, 308.3973083496094f, 0.f, 0.f), FVector2D(0.5f, 0.f), true);

	// YouAreDead: 1285 × 301 at the middle, a little above, scaled 1.05 and invisible until Death.
	YouAreDead = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("YouAreDead"));
	YouAreDead->SetBrush(TextureBrush(DeadTexture.LoadSynchronous(), FVector2D(1285.f, 301.f)));
	YouAreDead->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	YouAreDead->SetRenderScale(FVector2D(1.05f, 1.05f));
	Place(Root, YouAreDead, FAnchors(0.5f, 0.5f), FMargin(0.f, -30.2824764251709f, 0.f, 0.f), FVector2D(0.5f, 0.5f), true);

	// Tips: under the middle, in RobotoTiny's Light at UMG's default size.
	TipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Tips"));
	TipText->SetFont(Font(TipFont.LoadSynchronous(), TEXT("Light"), 24.f));
	TipText->SetColorAndOpacity(FSlateColor(FLinearColor(TipGrey, TipGrey, TipGrey, 1.f)));
	TipText->SetJustification(ETextJustify::Center);
	TipText->SetText(Tip);
	Place(Root, TipText, FAnchors(0.5f, 0.5f), FMargin(2.539062976837158f, 161.32643127441406f, 151.f, 40.f), FVector2D(0.5f, 0.f), true);

	// Button_0: the black cover Fade In and Fade Out play with, past every edge (a border, as Button_22).
	Cover = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Button_0"));
	Cover->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, CoverAlpha));
	Cover->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Root, Cover, Fill, FMargin(-49.5495491027832f, -76.57657623291016f, -43.543540954589844f, -82.58258056640625f),
		FVector2D::ZeroVector, false);

	// Image_161: the red vignette Shake flashes, over everything.
	Vignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_161"));
	Vignette->SetBrush(TextureBrush(VignetteTexture.LoadSynchronous(), FVector2D(1920.f, 1080.f), VignetteTint));
	Vignette->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Root, Vignette, Fill, FMargin(0.f, 0.f, 1.9218759536743164f, 1.0810539722442627f), FVector2D(0.5f, 0.5f), true);
}

void UWasamiDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Decrement Lives; then the level's deaths + 1 and its streak back to 0, written at once.
	int32 Lives = 0;
	bool bEasyMode = false;
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		Instance->DecrementLives();
		Lives = Instance->GetLives();
		bEasyMode = Instance->IsEasy();
	}
	bool bCheckpoint = false;
	if (AWasamiGameMode* Mode = GameModeOf(this))
	{
		if (UWasamiSaveGame* Save = Mode->GetSave())
		{
			++Save->Hospital.Deaths;
			Save->Hospital.CurrentStreak = 0;
			Mode->WriteSave();
			bCheckpoint = Save->Hospital.LevelCheckpoint > 0;
		}
	}

	// Set Tip, and the death line, each picked at random.
	const TArray<FText> Tips = TipsFor(UGameplayStatics::GetCurrentLevelName(this, true));
	Tip = Tips[FMath::RandRange(0, Tips.Num() - 1)];
	if (TipText)
	{
		TipText->SetText(Tip);
	}
	const TConstArrayView<float> Voices = VoiceLengths(Level);
	const float Voice = Voices.Num() > 0 ? Voices[FMath::RandRange(0, Voices.Num() - 1)] : 0.f;

	Begin(Lives, Voice, bCheckpoint, bEasyMode);
}

void UWasamiDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiDeathScreenWidget::Begin(int32 LivesLeft, float VoiceSeconds, bool bInHasCheckpoint, bool bInEasy)
{
	LocalLives = LivesLeft;
	VoiceLength = FMath::Max(VoiceSeconds, 0.f);
	bHasCheckpoint = bInHasCheckpoint;
	bEasy = bInEasy;
	Pending.Reset();
	Elapsed = 0.f;
	StepTime = 0.f;
	bEasyHold = bProceeded = bGameOver = bButtonsShown = bRespawned = false;
	bRestartClosed = bLastCheckpointClosed = bQuitClosed = bLeft = false;
	Choice = EChoice::None;
	FadeInTime = FadeOutTime = ShakeTime = DeathTime = -1.f;
	CoverAlpha = 1.f;
	ShownLives = LifeIcons;
	ApplyAnimations();
	Schedule(EStep::LifeLost, RevealDelay);
	Schedule(EStep::Reveal, RevealDelay);
}

void UWasamiDeathScreenWidget::Schedule(EStep Step, float Delay)
{
	Pending.Add({StepTime + Delay, Step});
}

void UWasamiDeathScreenWidget::Advance(float DeltaSeconds)
{
	Elapsed += DeltaSeconds;

	// The animations move on first and the Delays come due after them, as a user widget ticks them; an animation a
	// Delay starts shows its first frame at once.
	auto Move = [DeltaSeconds](float& Time, float Speed, float Length)
	{
		if (Time >= 0.f)
		{
			Time = FMath::Min(Time + DeltaSeconds * Speed, Length);
		}
	};
	Move(FadeInTime, FadeInSpeed, FadeLength);
	Move(FadeOutTime, 1.f, FadeLength);
	Move(ShakeTime, 1.f, ShakeLength);
	Move(DeathTime, 1.f, DeathLength);
	if (ShakeTime >= UpdateLifeTime && !bLifeUpdated)
	{
		bLifeUpdated = true;
		UpdateLife();
	}

	// The Delays in the order they come due; a step may start more.
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
		if (Next == INDEX_NONE)
		{
			break;
		}
		const FPendingStep Due = Pending[Next];
		Pending.RemoveAt(Next);
		StepTime = Due.At;
		RunStep(Due.Step);
	}

	ApplyAnimations();

	// An animation that reached its end keeps its last values (see the implementation record).
	for (const TPair<float*, float>& Each : {TPair<float*, float>(&FadeInTime, FadeLength), TPair<float*, float>(&FadeOutTime, FadeLength),
		TPair<float*, float>(&ShakeTime, ShakeLength), TPair<float*, float>(&DeathTime, DeathLength)})
	{
		if (*Each.Key >= Each.Value)
		{
			*Each.Key = -1.f;
		}
	}
}

void UWasamiDeathScreenWidget::RunStep(EStep Step)
{
	switch (Step)
	{
	case EStep::LifeLost:
		if (LocalLives != 0)
		{
			PlaySound(LifeLostSound, LifeLostVolume);
			// Wasami's fine goes with it, as the WebGL version's reveal plays them together.
			PlaySound(LifeVoiceSound, VoiceVolume);
		}
		break;

	case EStep::Reveal:
		FadeInTime = 0.f;
		if (LocalLives != 0)
		{
			// The death line (silent until item 20) moves the screen on when it ends, or the 6 s do.
			Schedule(EStep::Proceed, VoiceLength);
			LifeAnimation();
			Schedule(EStep::Proceed, VoiceLimit);
			break;
		}
		if (bEasy)
		{
			// No lives left on EASY (the latest version's @2539 → @2639, the settings' Difficulty 0): Life Animation only,
			// the heading and the tip left up. The 6 s DoOnce stops at Get Lives > 0 and Proceed is bound only with
			// lives left, so nothing more comes: no game over, no input to the screen, no respawn.
			bEasyHold = true;
			LifeAnimation();
			break;
		}
		// No lives left: without a checkpoint LAST CHECKPOINT goes; the row, the heading and the tip hide.
		ShownLives = 0;
		if (!bHasCheckpoint && LastCheckpointButton)
		{
			LastCheckpointButton->RemoveFromParent();
			LastCheckpointButton = nullptr;
		}
		for (UImage* Icon : LivesArray)
		{
			if (Icon)
			{
				Icon->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		if (Heading)
		{
			Heading->SetVisibility(ESlateVisibility::Hidden);
		}
		if (TipText)
		{
			TipText->SetVisibility(ESlateVisibility::Hidden);
		}
		Schedule(EStep::GameOver, GameOverDelay);
		break;

	case EStep::Shake:
		ShakeTime = 0.f;
		bLifeUpdated = false;
		break;

	case EStep::Proceed:
		// DoOnce: whichever of the line's end and the 6 s comes first.
		if (!bProceeded)
		{
			bProceeded = true;
			if (LocalLives > 0)
			{
				Schedule(EStep::FadeOut, ProceedDelay);
			}
		}
		break;

	case EStep::FadeOut:
		FadeOutTime = 0.f;
		Schedule(EStep::Respawn, RespawnDelay);
		break;

	case EStep::Respawn:
		bRespawned = true;
		if (UWorld* World = GetWorld())
		{
			if (const AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				if (UWasamiPowerComponent* Powers = Player->GetPowers())
				{
					Powers->ResetPowers();
				}
			}
			OnRespawn.Broadcast();
			UGameplayStatics::OpenLevel(this, FName(UGameplayStatics::GetCurrentLevelName(this, true)), true);
		}
		else
		{
			OnRespawn.Broadcast();
		}
		break;

	case EStep::GameOver:
		// The game over's music and Death; the input goes to the screen. Wasami's over stands in for Bierce's game
		// over line, and the laugh 1.25 s after it is not played (the WebGL version's choice, its record 10).
		bGameOver = true;
		PlaySound(GameOverSound, GameOverVolume);
		PlaySound(GameOverVoiceSound, VoiceVolume);
		DeathTime = 0.f;
		if (APlayerController* Controller = GetOwningPlayer())
		{
			UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, this, EMouseLockMode::DoNotLock);
		}
		Schedule(EStep::Buttons, ButtonsDelay);
		break;

	case EStep::Buttons:
		bButtonsShown = true;
		for (UButton* Button : {RestartButton.Get(), QuitButton.Get(), LastCheckpointButton.Get()})
		{
			if (Button)
			{
				Button->SetVisibility(ESlateVisibility::Visible);
			}
		}
		if (APlayerController* Controller = GetOwningPlayer())
		{
			Controller->SetShowMouseCursor(true);
		}
		break;

	case EStep::Restart:
		// The level again at no checkpoint (Zone 1 turns 0 into its arrival; Zone 2 opens Zone 1).
		if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
		{
			Instance->ForgetCollectedShards();
		}
		OpenLevel();
		break;

	case EStep::LastCheckpoint:
		// The level again at its checkpoint; the shards collected stay collected.
		if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
		{
			Instance->ResetLives();
		}
		OpenLevel();
		break;

	case EStep::QuitToTitle:
		// OpenLevel(TitleScreen). The save stays as it is, so the title's RESUME goes on from its checkpoint (the title
		// gives the lives back).
		OpenLevel(AWasamiGameMode::TitleLevelName);
		break;
	}
}

void UWasamiDeathScreenWidget::PressRestart()
{
	// A question over the screen; nothing more until its YES.
	RestartPopUp = UWasamiPopUpWidget::Show(this, FText::FromString(RestartQuestion), UWasamiPopUpWidget::RestartFrame, RestartPopUpZOrder);
	if (RestartPopUp)
	{
		RestartPopUp->OnYesClick.AddDynamic(this, &UWasamiDeathScreenWidget::RestartEvent);
	}
}

void UWasamiDeathScreenWidget::RestartEvent()
{
	if (RestartPopUp)
	{
		RestartPopUp->CloseAnimation();
	}
	// Reset Game Instance and Reset Lives (Used Hard Respawn? and Hard Check Point belong to the results and the
	// entrance, which this game does not have yet or at all).
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		Instance->ForgetCollectedShards();
		Instance->ResetLives();
	}
	if (bRestartClosed)
	{
		return;
	}
	bRestartClosed = true;
	// levelStruct[the level] = levelStruct[10] (an empty entry), written.
	if (AWasamiGameMode* Mode = GameModeOf(this))
	{
		if (UWasamiSaveGame* Save = Mode->GetSave())
		{
			Save->Hospital = FWasamiLevelProgress();
			Mode->WriteSave();
		}
	}
	BeginLeaving(EChoice::Restart, EStep::Restart);
}

void UWasamiDeathScreenWidget::PressLastCheckpoint()
{
	// The original reads SaveSlot and does nothing if it cannot.
	AWasamiGameMode* Mode = GameModeOf(this);
	UWasamiSaveGame* Save = Mode ? Mode->GetSave() : nullptr;
	if (!Save)
	{
		return;
	}
	if (Save->bLastCheckpointWarning)
	{
		LastCheckpointProceed();
		return;
	}
	if (UWasamiPopUpWidget* Warning = UWasamiPopUpWidget::Show(this, FText::FromString(LastCheckpointWarning),
		UWasamiPopUpWidget::BlankFrame, WarningPopUpZOrder))
	{
		Warning->OnYesClick.AddDynamic(this, &UWasamiDeathScreenWidget::LastCheckpointEvent);
	}
	PlaySound(SelectSound, 1.f);
}

void UWasamiDeathScreenWidget::LastCheckpointEvent()
{
	// The warning stays on the screen until the level opens (nothing closes it).
	if (AWasamiGameMode* Mode = GameModeOf(this))
	{
		if (UWasamiSaveGame* Save = Mode->GetSave())
		{
			Save->bLastCheckpointWarning = true;
			Mode->WriteSave();
		}
	}
	LastCheckpointProceed();
}

void UWasamiDeathScreenWidget::LastCheckpointProceed()
{
	// Used Hard Respawn? and Game Instance Time are the results' (item 14).
	if (bLastCheckpointClosed)
	{
		return;
	}
	bLastCheckpointClosed = true;
	BeginLeaving(EChoice::LastCheckpoint, EStep::LastCheckpoint);
}

void UWasamiDeathScreenWidget::PressQuitToTitle()
{
	if (bQuitClosed)
	{
		return;
	}
	bQuitClosed = true;
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		Instance->ForgetCollectedShards();
	}
	BeginLeaving(EChoice::QuitToTitle, EStep::QuitToTitle);
}

void UWasamiDeathScreenWidget::BeginLeaving(EChoice InChoice, EStep Step)
{
	if (APlayerController* Controller = GetOwningPlayer())
	{
		Controller->SetShowMouseCursor(false);
	}
	Choice = InChoice;
	FadeOutTime = 0.f;
	ApplyAnimations();
	// A click comes between ticks: its Delay counts from now.
	StepTime = Elapsed;
	Schedule(Step, LeaveDelay);
}

void UWasamiDeathScreenWidget::OpenLevel(const TCHAR* LevelName)
{
	bLeft = true;
	LevelToOpen = LevelName ? LevelName : TEXT("");
	if (GetWorld())
	{
		if (APlayerController* Controller = GetOwningPlayer())
		{
			UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
		}
		UGameplayStatics::OpenLevel(this, LevelName ? FName(LevelName) : FName(UGameplayStatics::GetCurrentLevelName(this, true)), true);
	}
}

void UWasamiDeathScreenWidget::OnRestartClicked()
{
	PressRestart();
}

void UWasamiDeathScreenWidget::OnLastCheckpointClicked()
{
	PressLastCheckpoint();
}

void UWasamiDeathScreenWidget::OnQuitClicked()
{
	PressQuitToTitle();
}

void UWasamiDeathScreenWidget::OnRestartHovered()
{
	RestartButton->SetColorAndOpacity(FLinearColor::White);
}

void UWasamiDeathScreenWidget::OnRestartUnhovered()
{
	RestartButton->SetColorAndOpacity(UnhoverColour);
}

void UWasamiDeathScreenWidget::OnLastCheckpointHovered()
{
	if (LastCheckpointButton)
	{
		LastCheckpointButton->SetColorAndOpacity(FLinearColor::White);
	}
}

void UWasamiDeathScreenWidget::OnLastCheckpointUnhovered()
{
	if (LastCheckpointButton)
	{
		LastCheckpointButton->SetColorAndOpacity(UnhoverColour);
	}
}

void UWasamiDeathScreenWidget::OnQuitHovered()
{
	QuitButton->SetColorAndOpacity(FLinearColor::White);
}

void UWasamiDeathScreenWidget::OnQuitUnhovered()
{
	QuitButton->SetColorAndOpacity(UnhoverColour);
}

void UWasamiDeathScreenWidget::LifeAnimation()
{
	// The row keeps lives + 1 (the one being lost last); the first removal starts the 1 s to Shake (a Delay inside the
	// loop, so only once). With 5 lives or more nothing goes and nothing shakes.
	bool bScheduled = false;
	for (int32 Index = LocalLives + 1; Index <= LifeIcons - 1; ++Index)
	{
		if (LivesArray.IsValidIndex(Index) && LivesArray[Index])
		{
			LivesArray[Index]->RemoveFromParent();
		}
		if (!bScheduled)
		{
			bScheduled = true;
			Schedule(EStep::Shake, ShakeDelay);
		}
	}
	if (bScheduled)
	{
		ShownLives = LocalLives + 1;
	}
}

void UWasamiDeathScreenWidget::UpdateLife()
{
	if (LivesArray.IsValidIndex(LocalLives) && LivesArray[LocalLives])
	{
		LivesArray[LocalLives]->RemoveFromParent();
	}
	ShownLives = FMath::Min(ShownLives, LocalLives);
}

void UWasamiDeathScreenWidget::ApplyAnimations()
{
	if (FadeInTime >= 0.f)
	{
		CoverAlpha = EvaluateFadeIn(FadeInTime);
	}
	if (FadeOutTime >= 0.f)
	{
		CoverAlpha = EvaluateFadeOut(FadeOutTime);
	}
	if (Cover)
	{
		// A colour's alpha below 0 draws as 0 (Fade Out's first 0.35 s dip under it).
		Cover->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(CoverAlpha, 0.f, 1.f)));
	}

	if (ShakeTime >= 0.f)
	{
		SetAlpha(Vignette, EvaluateShakeVignetteAlpha(ShakeTime));
		if (Vignette)
		{
			const float Scale = EvaluateShakeVignetteScale(ShakeTime);
			Vignette->SetRenderScale(FVector2D(Scale, Scale));
		}
		if (RootCanvas)
		{
			RootCanvas->SetRenderTranslation(EvaluateShakeTranslation(ShakeTime));
		}
		const float Tint = EvaluateShakeLifeTint(ShakeTime);
		for (UImage* Icon : LivesArray)
		{
			if (Icon)
			{
				Icon->SetColorAndOpacity(FLinearColor(1.f, Tint, Tint, 1.f));
			}
		}
	}

	if (DeathTime >= 0.f)
	{
		static const FRichCurve HeadingCurve = MakeCurve(DeathHeadingKeys);
		static const FRichCurve DeadCurve = MakeCurve(DeathDeadKeys);
		static const FRichCurve MenuCurve = MakeCurve(DeathMenuKeys);
		static const FRichCurve CheckpointCurve = MakeCurve(DeathCheckpointKeys);
		static const FRichCurve QuitCurve = MakeCurve(DeathQuitKeys);
		static const FRichCurve TipCurve = MakeCurve(DeathTipKeys);
		const float T = DeathTime;
		const bool bLate = T >= DeathLateStart;
		if (Heading)
		{
			Heading->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, Eval(HeadingCurve, T, DeathLength))));
		}
		if (bLate)
		{
			SetAlpha(YouAreDead, Eval(DeadCurve, T, DeathLength));
		}
		const float MenuAlpha = Eval(MenuCurve, T, DeathLength);
		for (int32 Index = 0; Index < ButtonTexts.Num(); ++Index)
		{
			// TextBlock_0 (RESTART) and TextBlock_1 (QUIT TO TITLE) from 0.5 s, TextBlock_2 (LAST CHECKPOINT) from 0.
			const bool bCheckpointText = Index == 1;
			if (ButtonTexts[Index] && (bLate || bCheckpointText))
			{
				ButtonTexts[Index]->SetColorAndOpacity(FSlateColor(FLinearColor(ButtonTextGrey, ButtonTextGrey, ButtonTextGrey, MenuAlpha)));
			}
		}
		if (LastCheckpointButton)
		{
			LastCheckpointButton->SetColorAndOpacity(FLinearColor(ButtonGrey, ButtonGrey, ButtonGrey, Eval(CheckpointCurve, T, DeathLength)));
		}
		if (QuitButton)
		{
			QuitButton->SetColorAndOpacity(FLinearColor(ButtonGrey, ButtonGrey, ButtonGrey, Eval(QuitCurve, T, DeathLength)));
		}
		if (Menu && bLate)
		{
			Menu->SetRenderOpacity(MenuAlpha);
		}
		if (TipText)
		{
			TipText->SetRenderOpacity(Eval(TipCurve, T, DeathLength));
		}
	}
}

void UWasamiDeathScreenWidget::PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Volume) const
{
	// PlaySound2D plays a UI sound, which goes on while the game is paused.
	if (GetWorld())
	{
		if (USoundBase* Loaded = Sound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, Volume);
		}
	}
}

float UWasamiDeathScreenWidget::ProceedTime(float VoiceSeconds)
{
	return RevealDelay + FMath::Min(FMath::Max(VoiceSeconds, 0.f), VoiceLimit) + ProceedDelay;
}

TConstArrayView<float> UWasamiDeathScreenWidget::VoiceLengths(uint8 InLevel)
{
	if (InLevel == TrapsLevel)
	{
		return TrapsVoiceLengths;
	}
	if (InLevel == AsylumLevel)
	{
		return AsylumVoiceLengths;
	}
	return {};
}

TArray<FText> UWasamiDeathScreenWidget::TipsFor(const FString& LevelName)
{
	// Set Tip's arrays for 06_Hospital_Zone_01 (and the entrance) and 06_Hospital_Zone_02, as written (with their
	// trailing spaces).
	const FText Skating = FText::FromString(TEXT("Listen for skating sounds to determine if any Reaper Nurses are close to you. "));
	const FText Doors = FText::FromString(TEXT("Use double doors to reveal the location of cloaked Reaper Nurses. Be careful though - the nurses can hear you opening them as well. "));
	if (LevelName.Contains(TEXT("Zone1")))
	{
		return {Skating, Doors,
			FText::FromString(TEXT("Don't always do what you're told. The Reaper Nurses should not be trusted. (hospital tests)"))};
	}
	if (LevelName.Contains(TEXT("Zone2")))
	{
		return {Skating, Doors,
			FText::FromString(TEXT("Watch your step. Saw blades in the floor can be difficult to see. ")),
			FText::FromString(TEXT("Use your tablet to check if you are within each Reaper Nurse's view. Avoid being detected. "))};
	}
	return {FText::FromString(TEXT("Try not to die next time."))};
}

float UWasamiDeathScreenWidget::EvaluateFadeIn(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(FadeInKeys);
	return Eval(Curve, Seconds, FadeLength);
}

float UWasamiDeathScreenWidget::EvaluateFadeOut(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(FadeOutKeys);
	return Eval(Curve, Seconds, FadeLength);
}

float UWasamiDeathScreenWidget::EvaluateShakeVignetteAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ShakeVignetteAlphaKeys);
	return Eval(Curve, Seconds, ShakeLength);
}

float UWasamiDeathScreenWidget::EvaluateShakeVignetteScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ShakeVignetteScaleKeys);
	return Eval(Curve, Seconds, ShakeLength);
}

FVector2D UWasamiDeathScreenWidget::EvaluateShakeTranslation(float Seconds)
{
	static const FRichCurve X = MakeCurve(ShakeXKeys);
	static const FRichCurve Y = MakeCurve(ShakeYKeys);
	return FVector2D(Eval(X, Seconds, ShakeLength), Eval(Y, Seconds, ShakeLength));
}

float UWasamiDeathScreenWidget::EvaluateShakeLifeTint(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ShakeLifeTintKeys);
	return Eval(Curve, Seconds, ShakeLength);
}
