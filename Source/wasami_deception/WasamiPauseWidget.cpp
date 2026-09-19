#include "WasamiPauseWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/BackgroundBlur.h"
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
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiBlackFadeWidget.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiOptionsWidget.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSaveGame.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// FadeIn (UMG_Pause): CanvasPanel_0's RenderOpacity 0 → 1 over 30000 ticks, both keys UE's auto tangent, flat.
	const FAnimKey PauseFadeInKeys[] = {{0., 0.f, 0., 0.}, {30000., 1.f, 0., 0.}};

	// Popup and Popup_0 (the same keys): the window's RenderOpacity 0 → 1 (0.25 s; the section ends there and the value
	// stays), as CanvasPanel_3's, and Blur+Red's 1 → 0 alike; the window's scale 0 → 1 (0.25 s, UE's auto tangent, so it
	// goes a little past 1) and 1 to 0.5 s.
	const FAnimKey PausePopupOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};
	const FAnimKey PausePopupScaleKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 3.333333370392211e-05, 3.333333370392211e-05}, {30000., 1.f, 0., 0.}};

	// Image_1's and redblock's tint (the same as the options screen's wash).
	const FLinearColor PauseWashRed(0.182292f, 0.f, 0.f, 0.371429f);

	UCanvasPanelSlot* PlaceInPause(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
		const FVector2D& Alignment = FVector2D::ZeroVector, bool bAutoSize = false)
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

	FSlateBrush PauseBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	/** The text block's default font (the engine's Roboto Bold) at Size, as every text of the menu has it. */
	void SetPauseFontSize(UTextBlock* Text, float Size)
	{
		FSlateFontInfo Info = Text->GetFont();
		Info.Size = Size;
		Text->SetFont(Info);
	}

	FAutoConsoleCommandWithWorldAndArgs PauseCommand(TEXT("Wasami.Pause"),
		TEXT("Wasami.Pause: the player's Esc (the pause menu, unless the game is paused). In the editor Esc stops the play session."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0)))
			{
				Player->EscapePressed();
			}
		}));

	/** The button property IsFocusable, which UE 5 lets be set only before the Slate widget is made. */
	void SetPauseButtonFocusable(UButton* Button, bool bFocusable)
	{
		if (FBoolProperty* Property = FindFProperty<FBoolProperty>(UButton::StaticClass(), TEXT("IsFocusable")))
		{
			Property->SetPropertyValue_InContainer(Button, bFocusable);
		}
	}
}

UWasamiPauseWidget::UWasamiPauseWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StrokeTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Pause/pause_screen_bg")));
	HeadTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Pause/T_PauseHead")));
	PeekTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Pause/T_PausePeek")));
	QuitFrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Pause/quit_window_frame")));
	RestartFrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Pause/restart_window_frame")));
	OpenSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Pause")));
	MusicSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Pause_Sound_v1")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
	PopUpSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Window_PopUp_V3")));
}

UWasamiPauseWidget* UWasamiPauseWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller || Find(WorldContextObject))
	{
		return nullptr;
	}
	UWasamiPauseWidget* Menu = CreateWidget<UWasamiPauseWidget>(Controller, StaticClass());
	if (Menu)
	{
		Menu->AddToViewport(ViewportZOrder);
	}
	return Menu;
}

UWasamiPauseWidget* UWasamiPauseWidget::Find(const UObject* WorldContextObject)
{
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(WorldContextObject, Found, StaticClass(), true);
	for (UUserWidget* Each : Found)
	{
		UWasamiPauseWidget* Menu = Cast<UWasamiPauseWidget>(Each);
		if (Menu && !Menu->IsFinished())
		{
			return Menu;
		}
	}
	return nullptr;
}

TSharedRef<SWidget> UWasamiPauseWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		BuildScreen(Root);
	}
	return Super::RebuildWidget();
}

void UWasamiPauseWidget::BuildScreen(UCanvasPanel* Root)
{
	// Each slot is the original's (UMG_Pause's WidgetTree), in its order; what the export leaves out is the slot's
	// default. The washes reach a little past the screen's edges.
	RootPanel = Root;
	const FAnchors Fill(0.f, 0.f, 1.f, 1.f);
	const FAnchors Middle(0.5f, 0.5f);
	const FVector2D Centred(0.5f, 0.5f);
	UTexture2D* Peek = PeekTexture.LoadSynchronous();

	// A blur of 8 and the red wash over it: Blur+Red, and CanvasPanel_3 (the pop-ups' second wash, clear until then).
	auto MakeWash = [this, &Fill](const TCHAR* PanelName, const TCHAR* BlurName, const TCHAR* RedName, bool bHitTestInvisible)
	{
		UCanvasPanel* Panel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), PanelName);
		UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), BlurName);
		Blur->SetBlurStrength(8.f);
		UImage* Red = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), RedName);
		FSlateBrush RedBrush;
		RedBrush.TintColor = FSlateColor(PauseWashRed);
		Red->SetBrush(RedBrush);
		if (bHitTestInvisible)
		{
			Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
			Red->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		PlaceInPause(Panel, Blur, Fill, FMargin(0.f, -12.012011528015137f, -12.912841796875f, -10.48046875f));
		PlaceInPause(Panel, Red, Fill, FMargin(-15.0150146484375f, -18.018016815185547f, -21.921836853027344f, -28.49853515625f));
		return Panel;
	};
	// A button as the tree has them: UE 4's default button (its paddings) with a clear background and its content in
	// Unhovered Color's grey, the label in the default font at Size, centred in UE's default button slot (4, 2).
	auto MakeButton = [this](const TCHAR* Name, const TCHAR* TextName, const TCHAR* Label, float Size, bool bFocusable)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		Button->SetColorAndOpacity(FLinearColor(UnhoveredGrey, UnhoveredGrey, UnhoveredGrey, 1.f));
		Button->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
		SetPauseButtonFocusable(Button, bFocusable);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		Text->SetText(FText::FromString(Label));
		SetPauseFontSize(Text, Size);
		UButtonSlot* TextSlot = Cast<UButtonSlot>(Button->AddChild(Text));
		TextSlot->SetPadding(FMargin(4.f, 2.f));
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		HoverButtons.Add(Button);
		return Button;
	};

	WashPanel = MakeWash(TEXT("Blur+Red"), TEXT("BackgroundBlur_0"), TEXT("Image_1"), false);
	PlaceInPause(Root, WashPanel, Fill, FMargin(-12.012011528015137f, -13.51351261138916f, -9.909912109375f, -22.492431640625f));

	// Image_152: the brush stroke, 912 wide, a little left of the middle and down the whole height (and past it).
	UImage* Stroke = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_152"));
	Stroke->SetBrush(PauseBrush(StrokeTexture.LoadSynchronous(), FVector2D(912.f, 1200.f)));
	PlaceInPause(Root, Stroke, FAnchors(0.5f, 0.f, 0.5f, 1.f), FMargin(-20.56032943725586f, -20.6296329498291f, 0.f, -38.9189453125f), Centred, true);

	// Icon: the head, 900 square at the middle of the top, a fifth of it above the screen.
	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
	Icon->SetBrush(PauseBrush(HeadTexture.LoadSynchronous(), FVector2D(900.f, 900.f), HeadTint()));
	PlaceInPause(Root, Icon, FAnchors(0.5f, 0.f), FMargin(0.f, 0.f, 0.f, 40.f), FVector2D(0.5f, 0.2f), true);

	// TextBlock_1: EASY MODE, its top 515.46 above the middle; its colour is bound (RefreshColours).
	EasyModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_1"));
	EasyModeText->SetText(FText::FromString(TEXT("EASY MODE")));
	EasyModeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5254899859428406f, 0.f, 0.f, 1.f)));
	SetPauseFontSize(EasyModeText, 36.f);
	EasyModeText->SetMinDesiredWidth(358.48272705078125f);
	EasyModeText->SetJustification(ETextJustify::Center);
	PlaceInPause(Root, EasyModeText, Middle, FMargin(0.f, -515.4616088867188f, 0.f, 0.f), FVector2D(0.5f, 0.f), true);

	// VerticalBox_113: RESUME, RESTART, OPTIONS and QUIT, their top at the middle of the screen, each as wide as the
	// widest (the vertical box slot's default fill); none takes the focus.
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_113"));
	ResumeButton = MakeButton(TEXT("RESUME"), TEXT("TextBlock_0"), TEXT("RESUME"), 36.f, false);
	UButton* RestartButton = MakeButton(TEXT("RESTART"), TEXT("Restarttext"), TEXT("RESTART"), 36.f, false);
	UButton* OptionsButton = MakeButton(TEXT("OPTIONS"), TEXT("TextBlock_2"), TEXT("OPTIONS"), 36.f, false);
	UButton* QuitButton = MakeButton(TEXT("QUIT"), TEXT("TextBlock_3"), TEXT("QUIT"), 36.f, false);
	for (UButton* Item : {ResumeButton.Get(), RestartButton, OptionsButton, QuitButton})
	{
		Menu->AddChildToVerticalBox(Item);
	}
	PlaceInPause(Root, Menu, Middle, FMargin(0.f), FVector2D(0.5f, 0.f), true);

	// CanvasPanel_3: clear, and let clicks through, until a pop-up.
	VeilPanel = MakeWash(TEXT("CanvasPanel_3"), TEXT("BackgroundBlur_1"), TEXT("redblock"), true);
	VeilPanel->SetRenderOpacity(0.f);
	PlaceInPause(Root, VeilPanel, Fill, FMargin(-28.528528213500977f, -21.021020889282227f, -21.921875f, -36.0059814453125f));
	// (FindWidget walks down from the root, so only once the wash is in it.)
	RedBlock = Cast<UImage>(WidgetTree->FindWidget(TEXT("redblock")));

	// A window: its panel in the middle, at scale 0 and clear until its pop-up animation; the frame and the head let
	// clicks through.
	auto MakeWindow = [this](const TCHAR* Name)
	{
		UCanvasPanel* Window = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), Name);
		Window->SetRenderScale(FVector2D::ZeroVector);
		Window->SetRenderOpacity(0.f);
		return Window;
	};
	auto AddPicture = [this, &Middle, &Centred](UCanvasPanel* Window, const TCHAR* Name, UTexture2D* Texture, const FVector2D& Size,
		float Top, const FLinearColor& Tint)
	{
		UImage* Picture = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Picture->SetBrush(PauseBrush(Texture, Size, Tint));
		Picture->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceInPause(Window, Picture, Middle, FMargin(0.f, Top, 0.f, 40.f), Centred, true);
	};

	// Givingupbox: GIVING UP? (on the frame), the head over it, and under the middle the text and the three buttons.
	UCanvasPanel* GivingUp = GivingUpBox = MakeWindow(TEXT("Givingupbox"));
	AddPicture(GivingUp, TEXT("Image_420"), QuitFrameTexture.LoadSynchronous(), FVector2D(1222.f, 928.f), 0.f, FLinearColor::White);
	AddPicture(GivingUp, TEXT("window_quit_head"), Peek, FVector2D(1222.f, 928.f), 0.f, HeadTint());
	UVerticalBox* QuitBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_0"));
	// quittext: the original's Construct sets it from the game instance's Replay Mode? (never on here).
	UTextBlock* QuitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("quittext"));
	QuitText->SetText(FText::FromString(TEXT("YOU WILL BE ABLE TO RESTART \r\nFROM LAST CHECKPOINT")));
	SetPauseFontSize(QuitText, 30.f);
	QuitText->SetMinDesiredWidth(72.5020980834961f);
	QuitText->SetJustification(ETextJustify::Center);
	auto AddToQuitBox = [QuitBox](UWidget* Child, const FMargin& SlotPadding)
	{
		UVerticalBoxSlot* QuitSlot = QuitBox->AddChildToVerticalBox(Child);
		QuitSlot->SetPadding(SlotPadding);
		QuitSlot->SetHorizontalAlignment(HAlign_Center);
		QuitSlot->SetVerticalAlignment(VAlign_Center);
	};
	UButton* QuitToTitleButton = MakeButton(TEXT("QuitToTitleButton"), TEXT("TextBlock_4"), TEXT("QUIT TO TITLE"), 28.f, true);
	UButton* QuitToDesktopButton = MakeButton(TEXT("QuitToDesktopButton"), TEXT("TextBlock_5"), TEXT("QUIT TO DESKTOP"), 28.f, true);
	UButton* CancelButton = MakeButton(TEXT("CancelButton"), TEXT("TextBlock_6"), TEXT("CANCEL"), 28.f, true);
	AddToQuitBox(QuitText, FMargin(0.f, 0.f, 0.f, 30.f));
	for (UButton* Choice : {QuitToTitleButton, QuitToDesktopButton, CancelButton})
	{
		AddToQuitBox(Choice, FMargin(0.f, 15.f, 0.f, 0.f));
	}
	PlaceInPause(GivingUp, QuitBox, Middle, FMargin(0.f, 167.7858123779297f, 0.f, 0.f), Centred, true);
	PlaceInPause(Root, GivingUp, Middle, FMargin(0.f), Centred, true);

	// RestartBox: RESTART? (on the frame), the head lower over it, and YES / NO side by side under the middle.
	UCanvasPanel* Restart = RestartBox = MakeWindow(TEXT("RestartBox"));
	AddPicture(Restart, TEXT("Image_0"), RestartFrameTexture.LoadSynchronous(), FVector2D(1222.f, 532.f), 0.f, FLinearColor::White);
	AddPicture(Restart, TEXT("quitwindowhead"), Peek, FVector2D(1222.f, 928.f), 77.3114013671875f, HeadTint());
	UHorizontalBox* Choices = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_0"));
	UButton* YesButton = MakeButton(TEXT("YesButton"), TEXT("TextBlock_12"), TEXT("YES"), 28.f, true);
	UButton* NoButton = MakeButton(TEXT("NoButton"), TEXT("TextBlock_13"), TEXT("NO"), 28.f, true);
	for (UButton* Choice : {YesButton, NoButton})
	{
		UHorizontalBoxSlot* ChoiceSlot = Choices->AddChildToHorizontalBox(Choice);
		ChoiceSlot->SetPadding(FMargin(35.f, 15.f, 35.f, 0.f));
		ChoiceSlot->SetHorizontalAlignment(HAlign_Center);
		ChoiceSlot->SetVerticalAlignment(VAlign_Center);
	}
	PlaceInPause(Restart, Choices, Middle, FMargin(0.f, 60.38557052612305f, 0.f, 0.f), FVector2D(0.5f, 0.f), true);
	PlaceInPause(Root, Restart, Middle, FMargin(0.f), Centred, true);

	ResumeButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnResumeClicked);
	RestartButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnRestartClicked);
	OptionsButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnOptionsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnQuitClicked);
	QuitToTitleButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnQuitToTitleClicked);
	QuitToDesktopButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnQuitToDesktopClicked);
	CancelButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnCancelClicked);
	YesButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnYesClicked);
	NoButton->OnClicked.AddDynamic(this, &UWasamiPauseWidget::OnNoClicked);
	ApplyAnimation();
}

void UWasamiPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Cast To GameMode → Global Settings Save Instance, for EASY MODE's binding: the game instance's settings (new
	// defaults without one).
	UWasamiSettingsSaveGame* Read = Settings;
	if (!Read)
	{
		if (UWorld* World = GetWorld())
		{
			if (UWasamiGameInstance* Instance = Cast<UWasamiGameInstance>(World->GetGameInstance()))
			{
				Read = Instance->GetSettings();
			}
		}
	}
	if (!Read)
	{
		Read = NewObject<UWasamiSettingsSaveGame>(this);
	}
	Begin(Read);
}

void UWasamiPauseWidget::NativeDestruct()
{
	// Destruct (@6316): the music's FadeOut(0.5, 0) and the game mode's Unpause Time Counter.
	if (Music)
	{
		Music->FadeOut(MusicFadeOutSeconds, 0.f);
	}
	if (AWasamiGameMode* Mode = Cast<AWasamiGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->UnpauseTimeCounter();
	}
	Super::NativeDestruct();
}

void UWasamiPauseWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiPauseWidget::Begin(UWasamiSettingsSaveGame* InSettings)
{
	// Construct (@861): UI_Pause; PlayAnimation(FadeIn); SetGamePaused(True); SetInputMode_UIOnlyEx(Self, DoNotLock) and
	// the cursor; CreateSound2D(Pause_Sound_v1, 1, 1, 0, None, False, True).FadeIn(1, 1, 0); the heads by the game
	// mode's Level (this game's Wasami, set in the tree); the game mode's Pause Time Counter.
	Settings = InSettings;
	FadeInTime = 0.f;
	bResuming = bFinished = false;
	ResumeElapsed = 0.f;
	Popups[0] = Popups[1] = FPopupPlay();
	LastPopup = INDEX_NONE;
	bRestarting = bLeft = false;
	LevelToOpen.Reset();
	ApplyAnimation();
	RefreshColours();
	if (!GetWorld())
	{
		return;
	}
	PlaySound(OpenSound, 1.f);
	UGameplayStatics::SetGamePaused(this, true);
	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, this, EMouseLockMode::DoNotLock);
		Controller->SetShowMouseCursor(true);
	}
	// CreateSound2D makes a UI sound (it goes on while the game is paused).
	if (USoundBase* Loaded = MusicSound.LoadSynchronous())
	{
		Music = UGameplayStatics::CreateSound2D(this, Loaded, 1.f, 1.f, 0.f, nullptr, false, true);
		if (Music)
		{
			Music->FadeIn(MusicFadeInSeconds, 1.f, 0.f);
		}
	}
	if (AWasamiGameMode* Mode = Cast<AWasamiGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		Mode->PauseTimeCounter();
	}
}

void UWasamiPauseWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// The animations move on first and the Delay comes due after them, as a user widget ticks them.
	FadeInTime = bResuming ? FMath::Max(FadeInTime - DeltaSeconds, 0.f) : FMath::Min(FadeInTime + DeltaSeconds, FadeInLength);
	for (FPopupPlay& Play : Popups)
	{
		if (Play.bPlaying)
		{
			Play.Time = Play.bReverse ? FMath::Max(Play.Time - DeltaSeconds, 0.f) : FMath::Min(Play.Time + DeltaSeconds, PopupLength);
			Play.bPlaying = Play.bReverse ? Play.Time > 0.f : Play.Time < PopupLength;
		}
	}
	ApplyAnimation();
	RefreshColours();
	if (bResuming)
	{
		ResumeElapsed += DeltaSeconds;
		if (ResumeElapsed >= ResumeDelay)
		{
			// @15: SetGamePaused(False), RemoveFromParent.
			bFinished = true;
			if (GetWorld())
			{
				UGameplayStatics::SetGamePaused(this, false);
			}
			RemoveFromParent();
		}
	}
}

void UWasamiPauseWidget::PressResume()
{
	// @3654: UI_Select_V3; PlayAnimation(FadeIn, 0, 1, Reverse, 1), which starts over from the end each time;
	// SetInputMode_GameOnly and the cursor hidden; Delay(0.5), which a pending Delay ignores.
	PlaySound(SelectSound, 1.f);
	FadeInTime = FadeInLength;
	if (!bResuming)
	{
		bResuming = true;
		ResumeElapsed = 0.f;
	}
	ApplyAnimation();
	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
		Controller->SetShowMouseCursor(false);
	}
}

void UWasamiPauseWidget::PressRestart()
{
	OpenPopup(EPopup::Restart);
}

void UWasamiPauseWidget::PressQuit()
{
	OpenPopup(EPopup::GivingUp);
}

void UWasamiPauseWidget::PressNo()
{
	ClosePopup(EPopup::Restart);
}

void UWasamiPauseWidget::PressCancel()
{
	ClosePopup(EPopup::GivingUp);
}

void UWasamiPauseWidget::OpenPopup(EPopup Popup)
{
	// @3996 / @4516: PlayAnimation(Popup_0 / Popup, 0, 1, Forward, 1); UI_Select_V3 and UI_Window_PopUp_V3 (in the
	// other order for QUIT; they sound together); redblock.SetVisibility(Visible).
	PlayPopup(Popup, 0.f, false);
	PlaySound(SelectSound, 1.f);
	PlaySound(PopUpSound, 1.f);
	if (RedBlock)
	{
		RedBlock->SetVisibility(ESlateVisibility::Visible);
	}
}

void UWasamiPauseWidget::ClosePopup(EPopup Popup)
{
	// @5946 / @4709: PlayAnimation(Popup_0 / Popup, 0.25, 1, Reverse, 1); UI_Select_V3 at 0.7;
	// redblock.SetVisibility(HitTestInvisible).
	PlayPopup(Popup, CloseFrom, true);
	PlaySound(SelectSound, ClosePitch);
	if (RedBlock)
	{
		RedBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UWasamiPauseWidget::PlayPopup(EPopup Popup, float StartAt, bool bReverse)
{
	FPopupPlay& Play = Popups[static_cast<int32>(Popup)];
	Play.Time = StartAt;
	Play.bReverse = bReverse;
	Play.bPlaying = true;
	LastPopup = static_cast<int32>(Popup);
	ApplyAnimation();
}

void UWasamiPauseWidget::PressYes()
{
	// The latest version's YES (@5408 → @244 → @52): levelStruct[the level] = levelStruct[10] (an empty entry) and
	// SaveGameToSlot('structSlot'); Shards To Be Removed (and Sewer Doors Opened, which the hospital does not have)
	// emptied; UI_Select_V3; SetInputMode_GameOnly and the cursor hidden; Hard Check Point 0 (the entrance's, which this
	// game does not have); UMG_BlackFade_2 with Speed 5 at Z 10, whose Animation Finished is Finish Restart. Nothing
	// there resets the lives; this game's YES gives back 3, as the death screen's RESTART does (the user's answer of
	// 2026-09-20). The old version's YES is the same but keeps the shards and, with no fade, unpauses and opens the
	// level at once. Nothing guards a second YES (the original's does it all again).
	bRestarting = true;
	if (UWorld* World = GetWorld())
	{
		if (AWasamiGameMode* Mode = Cast<AWasamiGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			if (UWasamiSaveGame* Save = Mode->GetSave())
			{
				Save->Hospital = FWasamiLevelProgress();
				Mode->WriteSave();
			}
		}
		if (UWasamiGameInstance* Instance = World->GetGameInstance<UWasamiGameInstance>())
		{
			Instance->ForgetCollectedShards();
			Instance->ResetLives();
		}
	}
	PlaySound(SelectSound, 1.f);
	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
		Controller->SetShowMouseCursor(false);
	}
	if (GetWorld())
	{
		if (UWasamiBlackFadeWidget* Fade = UWasamiBlackFadeWidget::Show(this, true, RestartFadeSpeed, RestartFadeZOrder))
		{
			Fade->OnFadeFinished.AddDynamic(this, &UWasamiPauseWidget::FinishRestart);
		}
	}
}

void UWasamiPauseWidget::FinishRestart()
{
	// @6656: SetGamePaused(False); OpenLevel(GetCurrentLevelName). The save has no checkpoint now, so Zone 1 opens at
	// the lift's arrival and Zone 2 opens Zone 1 (the game mode's Spawn).
	if (GetWorld())
	{
		UGameplayStatics::SetGamePaused(this, false);
	}
	OpenLevel(nullptr);
}

void UWasamiPauseWidget::PressOptions()
{
	// @6085: UI_Select_V3; CreateAndAddWidget(UMG_Options, 10) (the latest version's opens its SettingsUI at 1).
	PlaySound(SelectSound, 1.f);
	if (GetWorld())
	{
		UWasamiOptionsWidget::Show(this);
	}
}

void UWasamiPauseWidget::PressQuitToTitle()
{
	// @6207: UI_Select_V3; OpenLevel(TitleScreen), this game's L_Title. The save and the game instance stay as they are
	// (the title's game mode forgets the shards and resets the lives).
	PlaySound(SelectSound, 1.f);
	OpenLevel(AWasamiGameMode::TitleLevelName);
}

void UWasamiPauseWidget::PressQuitToDesktop()
{
	// @4843: UI_Select_V3; QuitGame(Self, None, Quit, False) (in the editor it ends the play session).
	PlaySound(SelectSound, 1.f);
	if (GetWorld())
	{
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
	}
}

void UWasamiPauseWidget::OpenLevel(const TCHAR* LevelName)
{
	// The original's OpenLevel leaves the input as it finds it (YES has given it to the game already).
	bLeft = true;
	LevelToOpen = LevelName ? LevelName : TEXT("");
	if (GetWorld())
	{
		UGameplayStatics::OpenLevel(this, LevelName ? FName(LevelName) : FName(UGameplayStatics::GetCurrentLevelName(this, true)), true);
	}
}

float UWasamiPauseWidget::GetPopupTime(EPopup Popup) const
{
	return Popups[static_cast<int32>(Popup)].Time;
}

bool UWasamiPauseWidget::IsMenuBlocked() const
{
	return RedBlock && RedBlock->GetVisibility() == ESlateVisibility::Visible;
}

void UWasamiPauseWidget::OnResumeClicked()
{
	PressResume();
}

void UWasamiPauseWidget::OnRestartClicked()
{
	PressRestart();
}

void UWasamiPauseWidget::OnOptionsClicked()
{
	PressOptions();
}

void UWasamiPauseWidget::OnQuitClicked()
{
	PressQuit();
}

void UWasamiPauseWidget::OnYesClicked()
{
	PressYes();
}

void UWasamiPauseWidget::OnNoClicked()
{
	PressNo();
}

void UWasamiPauseWidget::OnQuitToTitleClicked()
{
	PressQuitToTitle();
}

void UWasamiPauseWidget::OnQuitToDesktopClicked()
{
	PressQuitToDesktop();
}

void UWasamiPauseWidget::OnCancelClicked()
{
	PressCancel();
}

void UWasamiPauseWidget::PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Pitch) const
{
	// A UI sound, which goes on while the game is paused.
	if (GetWorld())
	{
		if (USoundBase* Loaded = Sound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, Pitch);
		}
	}
}

void UWasamiPauseWidget::ApplyAnimation()
{
	Opacity = EvaluateFadeIn(FadeInTime);
	if (RootPanel)
	{
		RootPanel->SetRenderOpacity(Opacity);
	}
	// Each window by its own animation (at 0 before one plays, as the tree has it); the washes by the pop-up played last
	// (both animations have tracks on them).
	for (const EPopup Popup : {EPopup::GivingUp, EPopup::Restart})
	{
		if (UCanvasPanel* Window = WindowOf(Popup))
		{
			const float Time = GetPopupTime(Popup);
			const float Scale = EvaluatePopupScale(Time);
			Window->SetRenderOpacity(EvaluatePopupOpacity(Time));
			Window->SetRenderScale(FVector2D(Scale, Scale));
		}
	}
	if (LastPopup != INDEX_NONE)
	{
		const float Shown = EvaluatePopupOpacity(Popups[LastPopup].Time);
		if (WashPanel)
		{
			WashPanel->SetRenderOpacity(1.f - Shown);
		}
		if (VeilPanel)
		{
			VeilPanel->SetRenderOpacity(Shown);
		}
	}
}

void UWasamiPauseWidget::RefreshColours()
{
	// GetColorAndOpacity_0 reads the settings' Difficulty every frame; each button's OnHovered makes its content white
	// and OnUnhovered Unhovered Color (@3419 / @3486 and the rest), which the hover state gives here.
	if (EasyModeText)
	{
		const FLinearColor Colour = EasyModeColor(Settings ? Settings->Difficulty : EWasamiDifficulty::Normal);
		if (EasyModeText->GetColorAndOpacity().GetSpecifiedColor() != Colour)
		{
			EasyModeText->SetColorAndOpacity(FSlateColor(Colour));
		}
	}
	const FLinearColor Unhovered(UnhoveredGrey, UnhoveredGrey, UnhoveredGrey, 1.f);
	for (UButton* Button : HoverButtons)
	{
		const FLinearColor Colour = Button->IsHovered() ? FLinearColor::White : Unhovered;
		if (Button->GetColorAndOpacity() != Colour)
		{
			Button->SetColorAndOpacity(Colour);
		}
	}
}

float UWasamiPauseWidget::EvaluateFadeIn(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(PauseFadeInKeys);
	return Eval(Curve, Seconds, FadeInLength);
}

float UWasamiPauseWidget::EvaluatePopupScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(PausePopupScaleKeys);
	return Eval(Curve, Seconds, PopupLength);
}

float UWasamiPauseWidget::EvaluatePopupOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(PausePopupOpacityKeys);
	return Eval(Curve, Seconds, PopupLength);
}

FLinearColor UWasamiPauseWidget::EasyModeColor(EWasamiDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case EWasamiDifficulty::Easy:
		return FLinearColor(0.5255f, 0.f, 0.f, 1.f);
	case EWasamiDifficulty::Normal:
		return FLinearColor(1.f, 0.f, 1.f, 0.f);
	default:
		return FLinearColor(1.f, 0.f, 1.f, 1.f);
	}
}

FLinearColor UWasamiPauseWidget::HeadTint()
{
	return FLinearColor::FromSRGBColor(FColor(192, 0, 0));
}
