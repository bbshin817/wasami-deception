#include "WasamiExtrasSoundWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Timespan.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "UObject/UObjectIterator.h"
#include "WasamiAssets.h"
#include "WasamiSaveGame.h"

namespace
{
	// UMG_Extras_Sound_Bar: ProgressBar_316's ground (a dark red at 0.54) and fill (red, never filled), and
	// ProgressBar_315's fill (dark red) over a clear ground.
	const FLinearColor SoundBarGround(0.05729199945926666f, 0.f, 0.f, 0.5400000214576721f);
	const FLinearColor SoundBarGroundFill(1.f, 0.f, 0.f, 1.f);
	const FLinearColor SoundBarPlayedFill(0.5028859972953796f, 0.f, 0.f, 1.f);
	// The bars' margins from the sides.
	constexpr float SoundBarInset = 148.17108154296875f;

	UCanvasPanelSlot* SoundPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	FSlateFontInfo SoundFont(UFont* Font, float Size)
	{
		FSlateFontInfo Info;
		Info.FontObject = Font;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		return Info;
	}

	/** UE 4's default button brush (a box, 8/32 kept at each edge) of Texture at Size, tinted Tint. */
	FSlateBrush SoundButtonBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(8.f / 32.f);
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	/** A progress bar as the tree's: UE's default style (as UE 4's) with WhiteSquareTexture as its fill, Ground as its ground. */
	void StyleSoundBar(UProgressBar* Bar, UTexture2D* White, UTexture2D* GroundTexture, const FLinearColor& GroundTint,
		const FLinearColor& Fill)
	{
		FProgressBarStyle Style = Bar->GetWidgetStyle();
		Style.BackgroundImage.SetResourceObject(GroundTexture);
		Style.BackgroundImage.ImageSize = FVector2D(32.f, 32.f);
		Style.BackgroundImage.TintColor = FSlateColor(GroundTint);
		Style.FillImage.SetResourceObject(White);
		Style.FillImage.ImageSize = FVector2D(32.f, 32.f);
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Fill);
	}
}

const FLinearColor UWasamiExtrasSoundButtonWidget::RestColour(0.502f, 0.f, 0.f, 1.f);
const FLinearColor UWasamiExtrasSoundButtonWidget::HoverColour(1.f, 0.f, 0.f, 1.f);
const FLinearColor UWasamiExtrasSoundButtonWidget::SelectedColour(1.f, 1.f, 1.f, 1.f);

UWasamiExtrasSoundBarWidget::UWasamiExtrasSoundBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NormalFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	WhiteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Engine/EngineResources/WhiteSquareTexture")));
}

TSharedRef<SWidget> UWasamiExtrasSoundBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_1"));
		WidgetTree->RootWidget = Root;
		UTexture2D* White = WhiteTexture.LoadSynchronous();
		UFont* Font = NormalFont.LoadSynchronous();
		const FAnchors AcrossMiddle(0.f, 0.5f, 1.f, 0.5f);
		const FMargin BarOffsets(SoundBarInset, 0.f, SoundBarInset, 30.f);

		// ProgressBar_316: the ground, 30 high across the middle 148 px in from the sides (its fill never moves).
		GroundBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar_316"));
		StyleSoundBar(GroundBar, White, White, SoundBarGround, SoundBarGroundFill);
		SoundPlace(Root, GroundBar, AcrossMiddle, BarOffsets, FVector2D(0.5f, 0.5f));

		// ProgressBar_315: the part played over it (its ground clear; Playback Percent through its binding).
		PlayedBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar_315"));
		StyleSoundBar(PlayedBar, White, nullptr, FLinearColor(1.f, 1.f, 1.f, 0.f), SoundBarPlayedFill);
		PlayedBar->SetPercent(0.33333298563957214f);
		SoundPlace(Root, PlayedBar, AcrossMiddle, BarOffsets, FVector2D(0.5f, 0.5f));

		// TextBlock_202: the name, 92 px above the middle (helvetica-normal 30).
		TitleBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_202"));
		TitleBlock->SetFont(SoundFont(Font, 30.f));
		SoundPlace(Root, TitleBlock, FAnchors(0.5f), FMargin(0.f, -92.f, 151.f, 40.f), FVector2D(0.5f, 0.f), true);

		// TextBlock_357 and TextBlock_358: the time played 30 px in from the left, the length 30 px in from the right
		// (helvetica-normal 24).
		ElapsedBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_357"));
		ElapsedBlock->SetFont(SoundFont(Font, 24.f));
		SoundPlace(Root, ElapsedBlock, FAnchors(0.f, 0.5f), FMargin(30.f, 0.f, 151.f, 40.f), FVector2D(0.f, 0.5f), true);
		DurationBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_358"));
		DurationBlock->SetFont(SoundFont(Font, 24.f));
		SoundPlace(Root, DurationBlock, FAnchors(1.f, 0.5f), FMargin(-30.f, 0.f, 151.f, 40.f), FVector2D(1.f, 0.5f), true);
		ApplyBindings();
	}
	return Super::RebuildWidget();
}

void UWasamiExtrasSoundBarWidget::NativeDestruct()
{
	// Destruct (@531 → @96): Sound.Stop().
	StopSound();
	Super::NativeDestruct();
}

void UWasamiExtrasSoundBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ApplyBindings();
}

void UWasamiExtrasSoundBarWidget::SetSound(USoundBase* NewSound, const FText& InText)
{
	// @193. Construct's CreateSound2D(None) makes nothing, so Sound is empty until the first Set Sound. Playback
	// Percent and Sound Wave stay the last sound's until the new one reports.
	StopSound();
	Text = InText;
	Sound = GetWorld() && NewSound ? UGameplayStatics::CreateSound2D(this, NewSound, 1.f, 1.f, 0.f, nullptr, false, true) : nullptr;
	if (Sound)
	{
		Sound->OnAudioPlaybackPercent.AddDynamic(this, &UWasamiExtrasSoundBarWidget::OnPlaybackPercent);
		Sound->Play(0.f);
	}
	ApplyBindings();
}

void UWasamiExtrasSoundBarWidget::Pause()
{
	if (IsValid(Sound))
	{
		Sound->SetPaused(true);
	}
}

void UWasamiExtrasSoundBarWidget::Resume()
{
	if (IsValid(Sound))
	{
		Sound->SetPaused(false);
	}
}

void UWasamiExtrasSoundBarWidget::StopSound()
{
	if (IsValid(Sound))
	{
		Sound->Stop();
	}
}

void UWasamiExtrasSoundBarWidget::OnPlaybackPercent(const USoundWave* PlayingSoundWave, const float InPlaybackPercent)
{
	PlaybackPercent = InPlaybackPercent;
	SoundWave = PlayingSoundWave;
}

FText UWasamiExtrasSoundBarWidget::FormatTime(float Seconds)
{
	const FTimespan Time = FTimespan::FromSeconds(Seconds);
	return FText::FromString(FString::Printf(TEXT("%d:%02d"), Time.GetMinutes(), Time.GetSeconds()));
}

FText UWasamiExtrasSoundBarWidget::GetDurationText() const
{
	// Sound Wave's Duration (0 before any report: the original reads None's, which gives 0).
	const USoundWave* Wave = SoundWave.Get();
	return FormatTime(Wave ? Wave->Duration : 0.f);
}

FText UWasamiExtrasSoundBarWidget::GetElapsedText() const
{
	const USoundWave* Wave = SoundWave.Get();
	return FormatTime((Wave ? Wave->Duration : 0.f) * PlaybackPercent);
}

FText UWasamiExtrasSoundBarWidget::GetTitleText() const
{
	return Text.ToUpper();
}

void UWasamiExtrasSoundBarWidget::ApplyBindings()
{
	// GetPercent_0 → ProgressBar_315's Percent; GetText_0/1/2 → TextBlock_358, TextBlock_357, TextBlock_202.
	if (PlayedBar)
	{
		PlayedBar->SetPercent(PlaybackPercent);
	}
	if (DurationBlock)
	{
		DurationBlock->SetText(GetDurationText());
	}
	if (ElapsedBlock)
	{
		ElapsedBlock->SetText(GetElapsedText());
	}
	if (TitleBlock)
	{
		TitleBlock->SetText(GetTitleText());
	}
}

UWasamiExtrasSoundButtonWidget::UWasamiExtrasSoundButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SaveSlotName = UWasamiSaveGame::SlotName;
	PlayTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/extras_play_icon")));
	PauseTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/extras_pause_icon")));
	LockedTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/locked_-_Copy")));
	WhiteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Engine/EngineResources/WhiteSquareTexture")));
	NormalFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
}

TSharedRef<SWidget> UWasamiExtrasSoundButtonWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		UTexture2D* White = WhiteTexture.LoadSynchronous();

		// Button_0: a 256 square of WhiteSquareTexture in the middle, dark red (its background colour) until hovered.
		Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_0"));
		FButtonStyle Style = Button->GetStyle();
		const FSlateBrush Square = SoundButtonBrush(White, FVector2D(32.f, 32.f), FLinearColor::White);
		Style.SetNormal(Square).SetHovered(Square).SetPressed(Square).SetNormalPadding(FMargin(2.f))
			.SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		Button->SetColorAndOpacity(RestColour);
		Button->SetBackgroundColor(RestColour);
		SoundPlace(Root, Button, FAnchors(0.5f), FMargin(0.f, 0.f, 256.f, 256.f), FVector2D(0.5f, 0.5f));

		// Button_1: a black 245 square inside it (a button that only shows its Normal and lets the pointer through),
		// holding Buton: the play icon (100 × 107) in dark red.
		Inner = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_1"));
		FButtonStyle InnerStyle = Inner->GetStyle();
		InnerStyle.SetNormal(SoundButtonBrush(White, FVector2D(245.f, 245.f), FLinearColor::Black)).SetHovered(Square).SetPressed(Square)
			.SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Inner->SetStyle(InnerStyle);
		// IsFocusable, which UE 5 lets be set only before the Slate widget is made.
		if (FBoolProperty* Property = FindFProperty<FBoolProperty>(UButton::StaticClass(), TEXT("IsFocusable")))
		{
			Property->SetPropertyValue_InContainer(Inner, false);
		}
		Inner->SetVisibility(ESlateVisibility::HitTestInvisible);
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Buton"));
		FSlateBrush IconBrush;
		IconBrush.SetResourceObject(PlayTexture.LoadSynchronous());
		IconBrush.ImageSize = FVector2D(100.f, 107.f);
		Icon->SetBrush(IconBrush);
		Icon->SetColorAndOpacity(RestColour);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		// ButtonSlot_1: UE's defaults (4, 2), centred.
		Inner->AddChild(Icon);
		SoundPlace(Root, Inner, FAnchors(0.5f), FMargin(0.f, 0.f, 245.f, 245.f), FVector2D(0.5f, 0.5f));

		// TextBlock_131: the label under the icon (helvetica-normal 18, dark red).
		Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_131"));
		Label->SetFont(SoundFont(NormalFont.LoadSynchronous(), 18.f));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.5028859972953796f, 0.f, 0.f, 1.f)));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		SoundPlace(Root, Label, FAnchors(0.5f), FMargin(0.f, 0.f, 100.f, 30.f), FVector2D(0.5f, -2.5f), true);

		Button->OnClicked.AddDynamic(this, &UWasamiExtrasSoundButtonWidget::OnButtonClicked);
		Button->OnHovered.AddDynamic(this, &UWasamiExtrasSoundButtonWidget::OnButtonHovered);
		Button->OnUnhovered.AddDynamic(this, &UWasamiExtrasSoundButtonWidget::OnButtonUnhovered);
	}
	return Super::RebuildWidget();
}

void UWasamiExtrasSoundButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiExtrasSoundButtonWidget::Begin()
{
	// @950: LoadGameFromSlot('SaveSlot'); unlocked (switch on Diary?: Level Ranks[ID] == None, or Extras_SFX contains
	// ID) → Text = Text; locked → Button_0.SetVisibility(HitTestInvisible), TextBlock_131.RemoveFromParent,
	// Buton.SetBrushFromTexture(locked_-_Copy, True).
	if (!Save)
	{
		Save = UGameplayStatics::DoesSaveGameExist(SaveSlotName, UWasamiSaveGame::UserIndex)
			? Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UWasamiSaveGame::UserIndex)) : nullptr;
		if (!Save)
		{
			Save = NewObject<UWasamiSaveGame>(this);
		}
	}
	if (Label)
	{
		Label->SetText(GetLabelText());
	}
	if (IsUnlocked())
	{
		return;
	}
	if (Button)
	{
		Button->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (Label)
	{
		Label->RemoveFromParent();
	}
	if (Icon)
	{
		Icon->SetBrushFromTexture(LockedTexture.LoadSynchronous(), true);
	}
}

void UWasamiExtrasSoundButtonWidget::Press()
{
	// @2043: PlaySound2D(UI_Select_V3, 1, 1.5). Playing? → Paused? on, the bar paused, the play icon, Playing? off;
	// else Paused? → the bar resumed, Playing? on, the pause icon (Paused? stays on); else the bar's Set Sound(Sound,
	// Text), Playing? on, the pause icon.
	if (GetWorld())
	{
		if (USoundBase* Loaded = SelectSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, SelectPitch);
		}
	}
	if (bPlaying)
	{
		bPaused = true;
		if (SoundBar)
		{
			SoundBar->Pause();
		}
		SetIcon(PlayTexture);
		bPlaying = false;
	}
	else
	{
		if (bPaused)
		{
			if (SoundBar)
			{
				SoundBar->Resume();
			}
		}
		else if (SoundBar)
		{
			SoundBar->SetSound(Sound, Text);
		}
		bPlaying = true;
		SetIcon(PauseTexture);
	}
	// @29: a DoOnce: GetAllWidgetsOfClass(UMG_Extras_Sound_Button, not top level only) and every one but this
	// deselected, then Selected? on, the colours white and the pause icon.
	if (bSelectGateClosed)
	{
		return;
	}
	bSelectGateClosed = true;
	UWorld* World = GetWorld();
	for (TObjectIterator<UWasamiExtrasSoundButtonWidget> Each; Each; ++Each)
	{
		if (*Each != this && IsValid(*Each) && !Each->IsTemplate() && Each->GetWorld() == World)
		{
			Each->Deselect();
		}
	}
	bSelected = true;
	SetColours(SelectedColour);
	SetIcon(PauseTexture);
}

void UWasamiExtrasSoundButtonWidget::Deselect()
{
	// @2409: the same check as Construct's (with the save it read); @2393 opens the DoOnce again.
	if (!IsUnlocked())
	{
		return;
	}
	bSelectGateClosed = false;
	bPaused = bPlaying = bSelected = false;
	SetIcon(PlayTexture);
	SetColours(RestColour);
}

void UWasamiExtrasSoundButtonWidget::Hover(bool bHovered)
{
	if (!bSelected)
	{
		SetColours(bHovered ? HoverColour : RestColour);
	}
}

bool UWasamiExtrasSoundButtonWidget::IsUnlocked() const
{
	if (!Save)
	{
		return false;
	}
	return bDiary ? ID >= 0 && ID < LevelRankCount : Save->IsUnlocked(FWasamiCollectableEntry{EWasamiCollectableType::Sound, ID});
}

FText UWasamiExtrasSoundButtonWidget::GetLabelText() const
{
	return FText::FromString(FString::Printf(TEXT("%s%d"), bDiary ? TEXT("DIARY ") : TEXT("SOUND "), ID + 1));
}

void UWasamiExtrasSoundButtonWidget::SetColours(const FLinearColor& Colour)
{
	// TextBlock_131's colour, Buton's and Button_0's background.
	if (Label)
	{
		Label->SetColorAndOpacity(FSlateColor(Colour));
	}
	if (Icon)
	{
		Icon->SetColorAndOpacity(Colour);
	}
	if (Button)
	{
		Button->SetBackgroundColor(Colour);
	}
}

void UWasamiExtrasSoundButtonWidget::SetIcon(const TSoftObjectPtr<UTexture2D>& Texture)
{
	// Buton.SetBrushFromTexture(…, False): the icon at the brush's size.
	if (Icon)
	{
		Icon->SetBrushFromTexture(Texture.LoadSynchronous(), false);
	}
}

void UWasamiExtrasSoundButtonWidget::OnButtonClicked()
{
	Press();
}

void UWasamiExtrasSoundButtonWidget::OnButtonHovered()
{
	Hover(true);
}

void UWasamiExtrasSoundButtonWidget::OnButtonUnhovered()
{
	Hover(false);
}
