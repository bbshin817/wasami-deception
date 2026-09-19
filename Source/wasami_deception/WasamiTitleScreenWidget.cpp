#include "WasamiTitleScreenWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Curves/RichCurve.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GeneralProjectSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiExtrasWidget.h"
#include "WasamiGameMode.h"
#include "WasamiOptionsWidget.h"
#include "WasamiPopUpWidget.h"
#include "WasamiSaveGame.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	// The animations' keys as exported (pak_reference's UMG_TitleScreen; WasamiWidgetAnimation.h). Every section keeps
	// its last values once it ends (KeepState).
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Slideshow: Image_128's RenderOpacity.
	const FAnimKey SlideshowKeys[] = {{0., 1.f, 0., 0.}, {150000., 0.f, 0., 0.}};
	// FadeOut and FadeOut_0: Image_0's RenderOpacity.
	const FAnimKey BlackKeys[] = {{0., 0.f, 0., 0.}, {210000., 1.f, 4.444444584805751e-06, 4.444444584805751e-06}, {225000., 1.f, 0., 0.}};
	// FadeOut: CanvasPanel_0's scale (X and Y alike) and Image_2's RenderOpacity, in sections from 9000; before that
	// their first keys equal the tree's own values, so the curves hold them.
	const FAnimKey PulseKeys[] = {{9000., 1.f, 0., 0.}, {15000., 1.0499999523162842f, 0., 0.}, {18000., 1.f, 0., 0.}};
	const FAnimKey RedKeys[] = {{9000., 0.f, 0., 0.}, {15000., 0.5f, 2.2222220650292e-05, 2.2222220650292e-05},
		{18000., 0.20000000298023224f, -2.3809525373508222e-06, -2.3809525373508222e-06}, {225000., 0.f, 0., 0.}};
	// FadeOut's audio track: Start_New_Game's volume.
	const FAnimKey StartVolumeKeys[] = {{0., 0.6000000238418579f, 0., 0.}, {27000., 0.6000000238418579f, -3.030303105333587e-06, -3.030303105333587e-06},
		{98999., 0.30000001192092896f, 0., 0.}};

	// The tree's colours: the texts' (and Unhovered Color) and Image_2's.
	constexpr float TextGrey = 0.10702300071716309f;
	const FLinearColor UnhoveredColour(TextGrey, TextGrey, TextGrey, 1.f);
	const FLinearColor RedColour(0.265625f, 0.f, 0.0019359999569132924f, 1.f);

	// This game's logo takes the place of Image_103 (1043.7 × 564.9 at (4, -44)): its letters over the original's,
	// as the WebGL version set it. T_TitleLogo is 1942 × 809.
	constexpr float LogoLeft = 58.8f;
	constexpr float LogoTop = 61.6f;
	constexpr float LogoWidth = 848.f;
	constexpr float LogoHeight = LogoWidth * 809.f / 1942.f;
	// T_TitleLogoGlow: the logo's alpha at a quarter (486 × 202) with 100 of its pixels of room on every side
	// (Tools/dd/prepare_title.py), drawn under the logo over the same box and that room.
	constexpr float GlowInnerWidth = 486.f;
	constexpr float GlowInnerHeight = 202.f;
	constexpr float GlowPad = 100.f;

	// The notice in the place of the original's copyright.
	const TCHAR* const NoticeText = TEXT("UNOFFICIAL FAN GAME — NOT AFFILIATED WITH GLOWSTICK ENTERTAINMENT");

	UCanvasPanelSlot* Place(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets, bool bAutoSize)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = Anchors;
		Layout.Offsets = Offsets;
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(bAutoSize);
		return Slot;
	}

	FSlateBrush ResourceBrush(UObject* Resource, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Resource);
		Brush.ImageSize = Size;
		return Brush;
	}

	FSlateFontInfo MenuFontInfo(UFont* Object, float Size)
	{
		FSlateFontInfo Info;
		Info.FontObject = Object;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		return Info;
	}

	void SetTextColour(UTextBlock* Text, const FLinearColor& Colour)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Colour));
		}
	}

	const UWasamiSaveGame* ReadSave(const FString& Slot)
	{
		return UGameplayStatics::DoesSaveGameExist(Slot, UWasamiSaveGame::UserIndex)
			? Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, UWasamiSaveGame::UserIndex)) : nullptr;
	}
}

const TCHAR* const UWasamiTitleScreenWidget::NewGameQuestion = TEXT("STARTING A NEW GAME WILL RESET ALL PROGRESS.");

UWasamiTitleScreenWidget::UWasamiTitleScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SaveSlotName = UWasamiSaveGame::SlotName;
	VideoMaskTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/title_screen_video_mask")));
	StrokesMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/MM_TitleScreen_Mask_Grey")));
	MarkerTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/title_screen_selection_marker")));
	LogoTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Title/T_TitleLogo")));
	LogoGlowTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Title/T_TitleLogoGlow")));
	FaceTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Title/T_TitleFace")));
	MenuFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	MusicSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Pause_Sound_v1")));
	StartSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Start_New_Game")));
	VoiceSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Titlescreen/Bierce_Title_Modified_03")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
}

UWasamiTitleScreenWidget* UWasamiTitleScreenWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiTitleScreenWidget* Screen = CreateWidget<UWasamiTitleScreenWidget>(Controller, StaticClass());
	if (Screen)
	{
		Screen->AddToViewport(ViewportZOrder);
	}
	return Screen;
}

bool UWasamiTitleScreenWidget::HasProgress(const UWasamiSaveGame* Save)
{
	return Save && Save->Hospital.LevelCheckpoint > 0;
}

FText UWasamiTitleScreenWidget::VersionText()
{
	return FText::FromString(TEXT("v") + GetDefault<UGeneralProjectSettings>()->ProjectVersion);
}

TSharedRef<SWidget> UWasamiTitleScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = RootCanvas;
		BuildScreen(RootCanvas);
	}
	return Super::RebuildWidget();
}

void UWasamiTitleScreenWidget::BuildScreen(UCanvasPanel* Root)
{
	// Each slot is the original's CanvasPanelSlot, in the original's order (which is also the order they draw in).
	const FAnchors Fill(0.f, 0.f, 1.f, 1.f);
	const FAnchors LeftEdge(0.f, 0.f, 0.f, 1.f);
	UFont* Font = MenuFont.LoadSynchronous();

	// Button_0: black over the whole screen. Its button never takes input (HitTestInvisible), so it is a border with the
	// button's black brush.
	UBorder* Back = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Button_0"));
	Back->SetBrushColor(FLinearColor::Black);
	Back->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Root, Back, Fill, FMargin(0.f), false);

	// CanvasPanel_1 (the hidden slideshow under a blur of the black) and Image_1 (the hidden video) are not made.

	// Image_97: the face, 1100 square, from 1089.6 left of the right edge's middle (the original's enemy of the
	// chapter reached; here Wasami's face).
	UImage* Face = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_97"));
	Face->SetBrush(ResourceBrush(FaceTexture.LoadSynchronous(), FVector2D(1100.f, 1100.f)));
	Place(Root, Face, FAnchors(1.f, 0.5f), FMargin(-1089.6160888671875f, -549.239013671875f, 100.f, 30.f), true);

	// VideoMask: the smoke's black over the left 2029.65, top to bottom.
	UImage* VideoMask = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VideoMask"));
	VideoMask->SetBrush(ResourceBrush(VideoMaskTexture.LoadSynchronous(), FVector2D(32.f, 32.f)));
	Place(Root, VideoMask, LeftEdge, FMargin(0.f, 0.f, 2029.6546630859375f, 0.f), false);

	// Image_104: the brush strokes over the left 1654.65 (disabled in the tree, as in the original).
	UImage* Strokes = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_104"));
	Strokes->SetBrush(ResourceBrush(StrokesMaterial.LoadSynchronous(), FVector2D(32.f, 32.f)));
	Strokes->SetIsEnabled(false);
	Place(Root, Strokes, LeftEdge, FMargin(0.f, 0.f, 1654.6546630859375f, 0.f), false);

	// The logo's glow, then Image_103: the logo.
	const float GlowScaleX = LogoWidth / GlowInnerWidth;
	const float GlowScaleY = LogoHeight / GlowInnerHeight;
	UImage* Glow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoGlow"));
	const FVector2D GlowSize((GlowInnerWidth + 2.f * GlowPad) * GlowScaleX, (GlowInnerHeight + 2.f * GlowPad) * GlowScaleY);
	Glow->SetBrush(ResourceBrush(LogoGlowTexture.LoadSynchronous(), GlowSize));
	Glow->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Root, Glow, FAnchors(0.f, 0.f), FMargin(LogoLeft - GlowPad * GlowScaleX, LogoTop - GlowPad * GlowScaleY, GlowSize.X, GlowSize.Y), false);
	UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_103"));
	Logo->SetBrush(ResourceBrush(LogoTexture.LoadSynchronous(), FVector2D(LogoWidth, LogoHeight)));
	Place(Root, Logo, FAnchors(0.f, 0.f), FMargin(LogoLeft, LogoTop, LogoWidth, LogoHeight), false);

	// TextBlock_79: the notice, bottom left.
	UTextBlock* Notice = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_79"));
	Notice->SetText(FText::FromString(NoticeText));
	Notice->SetFont(MenuFontInfo(Font, 18.f));
	SetTextColour(Notice, UnhoveredColour);
	Place(Root, Notice, FAnchors(0.f, 1.f), FMargin(48.f, -81.0810546875f, 151.f, 40.f), true);

	// VerticalBox_160: the menu, 421.75 wide from the left edge's middle (Chapters and Replay, between NEW GAME and
	// EXTRAS, are not made). Each button is the original's after Setup
	// Buttons: its brush title_screen_selection_marker, invisible until hovered or pressed, and UE 4's default button
	// paddings (UE 5's default style differs).
	Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_160"));
	FSlateBrush Marker = ResourceBrush(MarkerTexture.LoadSynchronous(), FVector2D(394.f, 74.f));
	Marker.DrawAs = ESlateBrushDrawType::Image;
	FSlateBrush Unmarked = Marker;
	Unmarked.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
	auto MakeButton = [this, Font, &Marker, &Unmarked](const TCHAR* Name, const TCHAR* TextName, const TCHAR* Label, TObjectPtr<UTextBlock>& OutText)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(Unmarked).SetHovered(Marker).SetPressed(Marker).SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		Text->SetText(FText::FromString(Label));
		Text->SetFont(MenuFontInfo(Font, 30.f));
		SetTextColour(Text, UnhoveredColour);
		Text->SetMinDesiredWidth(250.f);
		Text->SetMargin(FMargin(10.f));
		// ButtonSlot_0: the padding as written, the rest of UE's defaults (the right 4 and bottom 2, centred).
		UButtonSlot* TextSlot = Cast<UButtonSlot>(Button->AddChild(Text));
		TextSlot->SetPadding(FMargin(40.f, 3.f, 4.f, 2.f));
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		UVerticalBoxSlot* ButtonSlot = Menu->AddChildToVerticalBox(Button);
		ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, -10.f));
		OutText = Text;
		return Button;
	};
	ResumeButton = MakeButton(TEXT("Resume"), TEXT("Resume_Text"), TEXT("RESUME"), ResumeText);
	NewGameButton = MakeButton(TEXT("NewGame"), TEXT("NewGame_Text"), TEXT("NEW GAME"), NewGameText);
	ExtrasButton = MakeButton(TEXT("Extras"), TEXT("Extras_Text"), TEXT("EXTRAS"), ExtrasText);
	OptionsButton = MakeButton(TEXT("Options"), TEXT("Options_Text"), TEXT("OPTIONS"), OptionsText);
	QuitButton = MakeButton(TEXT("Quit"), TEXT("Quit_Text"), TEXT("QUIT"), QuitText);
	// The original's ComponentDelegateBinding: RESUME on its press, the others on their click, and each one's hover.
	ResumeButton->OnPressed.AddDynamic(this, &UWasamiTitleScreenWidget::OnResumePressed);
	NewGameButton->OnClicked.AddDynamic(this, &UWasamiTitleScreenWidget::OnNewGameClicked);
	ExtrasButton->OnClicked.AddDynamic(this, &UWasamiTitleScreenWidget::OnExtrasClicked);
	OptionsButton->OnClicked.AddDynamic(this, &UWasamiTitleScreenWidget::OnOptionsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UWasamiTitleScreenWidget::OnQuitClicked);
	ResumeButton->OnHovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnResumeHovered);
	ResumeButton->OnUnhovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnResumeUnhovered);
	NewGameButton->OnHovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnNewGameHovered);
	NewGameButton->OnUnhovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnNewGameUnhovered);
	ExtrasButton->OnHovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnExtrasHovered);
	ExtrasButton->OnUnhovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnExtrasUnhovered);
	OptionsButton->OnHovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnOptionsHovered);
	OptionsButton->OnUnhovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnOptionsUnhovered);
	QuitButton->OnHovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnQuitHovered);
	QuitButton->OnUnhovered.AddDynamic(this, &UWasamiTitleScreenWidget::OnQuitUnhovered);
	Place(Root, Menu, FAnchors(0.f, 0.5f), FMargin(7.05760383605957f, -93.09329986572266f, 421.75030517578125f, 550.7249755859375f), false);

	// Image_0: the black FadeOut brings in; Image_128: the black Slideshow lifts, a little past every edge; Image_2:
	// FadeOut's red. None takes input.
	auto MakeVeil = [this](const TCHAR* Name, const FLinearColor& Colour)
	{
		UImage* Veil = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Veil->SetColorAndOpacity(Colour);
		Veil->SetRenderOpacity(0.f);
		Veil->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Veil;
	};
	Black = MakeVeil(TEXT("Image_0"), FLinearColor::Black);
	Place(Root, Black, Fill, FMargin(0.f), false);
	Cover = MakeVeil(TEXT("Image_128"), FLinearColor::Black);
	Place(Root, Cover, Fill, FMargin(-31.5f, -80.f, -22.f, -57.5f), false);
	Red = MakeVeil(TEXT("Image_2"), RedColour);
	Place(Root, Red, Fill, FMargin(0.f), false);

	// TextBlock_0: the version, from 81.92 left of the top right corner, over everything.
	UTextBlock* Version = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_0"));
	Version->SetText(VersionText());
	Version->SetFont(MenuFontInfo(Font, 18.f));
	SetTextColour(Version, UnhoveredColour);
	Place(Root, Version, FAnchors(1.f, 0.f), FMargin(-81.921875f, 12.f, 151.f, 40.f), true);

	ApplyAnimations();
}

void UWasamiTitleScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct's DoOnce; SaveSlot decides RESUME.
	if (bConstructed)
	{
		return;
	}
	bConstructed = true;
	Begin(HasProgress(ReadSave(SaveSlotName)));
}

void UWasamiTitleScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiTitleScreenWidget::Begin(bool bInHasProgress)
{
	Elapsed = 0.f;
	SlideshowTime = 0.f;
	FadeOutTime = FadeOut0Time = -1.f;
	bVoicePlayed = false;
	LevelToOpen.Reset();
	LeaveAt = -1.f;
	bLeft = bQuit = false;
	if (!bInHasProgress && ResumeButton)
	{
		ResumeButton->RemoveFromParent();
		ResumeButton = nullptr;
	}
	ApplyAnimations();

	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, this, EMouseLockMode::DoNotLock);
		Controller->SetShowMouseCursor(true);
	}
	FadeInMusic();
}

void UWasamiTitleScreenWidget::FadeInMusic()
{
	// CreateSound2D makes a UI sound (it goes on while the game is paused).
	if (GetWorld())
	{
		if (USoundBase* Loaded = MusicSound.LoadSynchronous())
		{
			Music = UGameplayStatics::CreateSound2D(this, Loaded, MusicVolume, MusicPitch, 0.f, nullptr, false, true);
			if (Music)
			{
				Music->SetSound(Loaded);
				Music->FadeIn(MusicFadeInSeconds, MusicFadeInLevel, 0.f);
			}
		}
	}
}

void UWasamiTitleScreenWidget::Advance(float DeltaSeconds)
{
	Elapsed += DeltaSeconds;
	auto Move = [DeltaSeconds](float& Time, float Length)
	{
		if (Time >= 0.f)
		{
			Time = FMath::Min(Time + DeltaSeconds, Length);
		}
	};
	Move(SlideshowTime, SlideshowLength);
	Move(FadeOutTime, FadeOutLength);
	Move(FadeOut0Time, FadeOut0Length);

	// FadeOut's second audio section: Bierce's line from its start.
	if (FadeOutTime >= VoiceTime && !bVoicePlayed)
	{
		bVoicePlayed = true;
		if (GetWorld())
		{
			if (USoundBase* Loaded = VoiceSound.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(this, Loaded);
			}
		}
	}

	ApplyAnimations();

	// The way out's Delay, after the animations as a user widget ticks them.
	if (LeaveAt >= 0.f && Elapsed >= LeaveAt)
	{
		LeaveAt = -1.f;
		OpenLevel();
	}

	// An animation that reached its end keeps its last values (KeepState).
	for (const TPair<float*, float>& Each : {TPair<float*, float>(&SlideshowTime, SlideshowLength),
		TPair<float*, float>(&FadeOutTime, FadeOutLength), TPair<float*, float>(&FadeOut0Time, FadeOut0Length)})
	{
		if (*Each.Key >= Each.Value)
		{
			*Each.Key = -1.f;
		}
	}
}

void UWasamiTitleScreenWidget::PlayFadeOut()
{
	FadeOutTime = 0.f;
	bVoicePlayed = false;
	StartVolume = EvaluateStartVolume(0.f);
	if (GetWorld())
	{
		if (USoundBase* Loaded = StartSound.LoadSynchronous())
		{
			StartAudio = UGameplayStatics::CreateSound2D(this, Loaded, StartVolume, 1.f, 0.f, nullptr, false, true);
			if (StartAudio)
			{
				StartAudio->Play();
			}
		}
	}
	ApplyAnimations();
}

void UWasamiTitleScreenWidget::PlayFadeOut0()
{
	FadeOut0Time = 0.f;
	ApplyAnimations();
}

void UWasamiTitleScreenWidget::FadeOutMusic(float Seconds)
{
	if (Music)
	{
		Music->FadeOut(Seconds, 0.f);
	}
}

void UWasamiTitleScreenWidget::PressNewGame()
{
	// @8532: SaveSlot's New Game? (set until a new game begins) lets it go without asking. This game's save has no such
	// flag; a game begun and not ended (RESUME's progress) is what the question warns about.
	if (!LevelToOpen.IsEmpty())
	{
		return;
	}
	if (!HasProgress(ReadSave(SaveSlotName)))
	{
		BeginNewGame();
		return;
	}
	// @1333: Create(UMG_PopUp) with the question (Frame left at 0), AddToViewport(2), YesClick bound to New Game, and
	// the select sound.
	NewGamePopUp = UWasamiPopUpWidget::Show(this, FText::FromString(NewGameQuestion), UWasamiPopUpWidget::RestartFrame, PopUpZOrder);
	if (NewGamePopUp)
	{
		NewGamePopUp->OnYesClick.AddDynamic(this, &UWasamiTitleScreenWidget::NewGameEvent);
	}
	PlaySelect();
}

void UWasamiTitleScreenWidget::NewGameEvent()
{
	// New Game (@8743 → @1712): the pop-up's Press No, then as without the question.
	if (NewGamePopUp)
	{
		NewGamePopUp->PressNo();
	}
	BeginNewGame();
}

void UWasamiTitleScreenWidget::BeginNewGame()
{
	// @1748: Music.FadeOut(1), the game mode's Erase Save Files (and New Game? off, written), SetInputMode_GameOnly,
	// PlayAnimation(FadeOut), Delay(10), OpenLevel. The original opens 00_TypeWriter, the first chapter's; this game's is
	// Zone 1 (whose empty save starts at the lift's arrival).
	if (!LevelToOpen.IsEmpty())
	{
		return;
	}
	FadeOutMusic(NewGameMusicFadeOut);
	UWasamiSaveGame::Erase(SaveSlotName);
	Leave(AWasamiGameMode::Zone1LevelName, NewGameDelay);
	PlayFadeOut();
}

void UWasamiTitleScreenWidget::PressResume()
{
	// @9272 → @115 (the checkpoint's question left out): Music.FadeOut(4), SetInputMode_GameOnly,
	// PlayAnimation(FadeOut_0), Delay(5), OpenLevel. The original opens the chapter's first level (00_Ballroom), whose
	// Spawn goes on at the checkpoint; the hospital's entrance picks the zone, which here is picked now.
	if (!LevelToOpen.IsEmpty())
	{
		return;
	}
	const UWasamiSaveGame* Save = ReadSave(SaveSlotName);
	FadeOutMusic(ResumeMusicFadeOut);
	Leave(AWasamiGameMode::LevelForCheckpoint(Save ? Save->Hospital.LevelCheckpoint : 0), ResumeDelay);
	PlayFadeOut0();
}

void UWasamiTitleScreenWidget::PressExtras()
{
	// @2293: Create(UMG_Extras), AddToViewport(2), its FadeMusic bound to FadeInMusic, the select sound, and
	// Music.FadeOut(1).
	Extras = UWasamiExtrasWidget::Show(this);
	if (Extras)
	{
		Extras->OnFadeMusic.AddUObject(this, &UWasamiTitleScreenWidget::FadeInMusic);
	}
	PlaySelect();
	FadeOutMusic(ExtrasMusicFadeOut);
}

void UWasamiTitleScreenWidget::PressOptions()
{
	// @8405: CreateAndAddWidget(UMG_Options, Z 10) and the select sound.
	UWasamiOptionsWidget::Show(this);
	PlaySelect();
}

void UWasamiTitleScreenWidget::PressQuit()
{
	// @1004: Create(UMG_PopUp) with Frame 1 (no text: the quit frame asks), AddToViewport(2), YesClick bound to QuitGame,
	// and the select sound.
	if (UWasamiPopUpWidget* PopUp = UWasamiPopUpWidget::Show(this, FText::GetEmpty(), UWasamiPopUpWidget::QuitFrame, PopUpZOrder))
	{
		PopUp->OnYesClick.AddDynamic(this, &UWasamiTitleScreenWidget::QuitEvent);
	}
	PlaySelect();
}

void UWasamiTitleScreenWidget::QuitEvent()
{
	// @8705: QuitGame(Self, None, Quit, False). The question stays on the screen.
	bQuit = true;
	if (GetWorld())
	{
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
	}
}

void UWasamiTitleScreenWidget::Leave(const TCHAR* LevelName, float Delay)
{
	LevelToOpen = LevelName;
	// A click comes between ticks: its Delay counts from now.
	LeaveAt = Elapsed + Delay;
	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
	}
}

void UWasamiTitleScreenWidget::OpenLevel()
{
	bLeft = true;
	if (GetWorld())
	{
		UGameplayStatics::OpenLevel(this, FName(LevelToOpen), true);
	}
}

void UWasamiTitleScreenWidget::PlaySelect() const
{
	// PlaySound2D(UI_Select_V3, 1, 1): a UI sound.
	if (GetWorld())
	{
		if (USoundBase* Loaded = SelectSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, 1.f);
		}
	}
}

void UWasamiTitleScreenWidget::OnResumePressed()
{
	PressResume();
}

void UWasamiTitleScreenWidget::OnNewGameClicked()
{
	PressNewGame();
}

void UWasamiTitleScreenWidget::OnExtrasClicked()
{
	PressExtras();
}

void UWasamiTitleScreenWidget::OnOptionsClicked()
{
	PressOptions();
}

void UWasamiTitleScreenWidget::OnQuitClicked()
{
	PressQuit();
}

void UWasamiTitleScreenWidget::ApplyAnimations()
{
	if (SlideshowTime >= 0.f)
	{
		CoverOpacity = EvaluateSlideshow(SlideshowTime);
	}
	if (FadeOutTime >= 0.f)
	{
		BlackOpacity = EvaluateBlack(FadeOutTime);
		PulseScale = EvaluatePulse(FadeOutTime);
		RedOpacity = EvaluateRed(FadeOutTime);
		StartVolume = EvaluateStartVolume(FadeOutTime);
		if (StartAudio)
		{
			StartAudio->SetVolumeMultiplier(StartVolume);
		}
	}
	if (FadeOut0Time >= 0.f)
	{
		BlackOpacity = EvaluateBlack(FadeOut0Time);
	}

	if (Cover)
	{
		Cover->SetRenderOpacity(CoverOpacity);
	}
	if (Black)
	{
		Black->SetRenderOpacity(BlackOpacity);
	}
	if (Red)
	{
		Red->SetRenderOpacity(RedOpacity);
	}
	if (RootCanvas)
	{
		RootCanvas->SetRenderScale(FVector2D(PulseScale, PulseScale));
	}
}

void UWasamiTitleScreenWidget::OnResumeHovered()
{
	SetTextColour(ResumeText, FLinearColor::White);
}

void UWasamiTitleScreenWidget::OnResumeUnhovered()
{
	SetTextColour(ResumeText, UnhoveredColour);
}

void UWasamiTitleScreenWidget::OnNewGameHovered()
{
	SetTextColour(NewGameText, FLinearColor::White);
}

void UWasamiTitleScreenWidget::OnNewGameUnhovered()
{
	SetTextColour(NewGameText, UnhoveredColour);
}

void UWasamiTitleScreenWidget::OnExtrasHovered()
{
	SetTextColour(ExtrasText, FLinearColor::White);
}

void UWasamiTitleScreenWidget::OnExtrasUnhovered()
{
	SetTextColour(ExtrasText, UnhoveredColour);
}

void UWasamiTitleScreenWidget::OnOptionsHovered()
{
	SetTextColour(OptionsText, FLinearColor::White);
}

void UWasamiTitleScreenWidget::OnOptionsUnhovered()
{
	SetTextColour(OptionsText, UnhoveredColour);
}

void UWasamiTitleScreenWidget::OnQuitHovered()
{
	SetTextColour(QuitText, FLinearColor::White);
}

void UWasamiTitleScreenWidget::OnQuitUnhovered()
{
	SetTextColour(QuitText, UnhoveredColour);
}

float UWasamiTitleScreenWidget::EvaluateSlideshow(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(SlideshowKeys);
	return Eval(Curve, Seconds, SlideshowLength);
}

float UWasamiTitleScreenWidget::EvaluateBlack(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(BlackKeys);
	return Eval(Curve, Seconds, FadeOutLength);
}

float UWasamiTitleScreenWidget::EvaluatePulse(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(PulseKeys);
	return Eval(Curve, Seconds, FadeOutLength);
}

float UWasamiTitleScreenWidget::EvaluateRed(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(RedKeys);
	return Eval(Curve, Seconds, FadeOutLength);
}

float UWasamiTitleScreenWidget::EvaluateStartVolume(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(StartVolumeKeys);
	return Eval(Curve, Seconds, FadeOutLength);
}
