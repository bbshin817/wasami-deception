#include "WasamiExtrasWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/DataTable.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectIterator.h"
#include "WasamiAssets.h"
#include "WasamiExtrasItemWidget.h"
#include "WasamiExtrasSoundWidget.h"
#include "WasamiSaveGame.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// FadeIn (UMG_Extras): CanvasPanel_0's RenderOpacity 0 → 1 over 0.5 s, the keys as exported (auto tangents, flat).
	const FAnimKey ExtrasOpacityKeys[] = {{0., 0.f, 0., 0.}, {30000., 1.f, 0., 0.}};
	// Credits_Scroll: RichTextBlock_258's RenderTransform translation's Y 1110 → -4121.86 in a straight line over 39.8 s
	// (its X stays 0).
	const FAnimKey CreditsYKeys[] = {
		{0., 1110.f, 0., -0.0035302701871842146, RCIM_Linear},
		{2388000., -4121.8603515625f, -0.0035302701871842146, 0., RCIM_Linear}};

	// The hospital's music, as AWasamiMusicPlayer plays it (its folder under /Game/DD).
	TSoftObjectPtr<USoundBase> ExtrasMusicTrack(const TCHAR* Name)
	{
		return TSoftObjectPtr<USoundBase>(WasamiAssets::Path(*(FString(TEXT("/Game/DD/Audio/06_Hospital/Music/")) + Name)));
	}

	// The movies' IDs in WrapBox_3's order (the original's third to tenth are all 2).
	const int32 VideoIDs[UWasamiExtrasWidget::VideoCount] = {0, 1, 2, 2, 2, 2, 2, 2, 2, 2};

	UCanvasPanelSlot* ExtrasScreenPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	FSlateFontInfo ExtrasFont(UFont* Font, float Size, const TCHAR* Typeface = TEXT("Default"))
	{
		FSlateFontInfo Info;
		Info.FontObject = Font;
		Info.TypefaceFontName = Typeface;
		Info.Size = Size;
		return Info;
	}

	/**
	 * A clear button as the original's text buttons are (UE 4's default style kept, its background at alpha 0, the words
	 * in Unhovered Color): UE 4's paddings, which UE 5's default style changed.
	 */
	void ExtrasTextButton(UButton* Button)
	{
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		const float Grey = UWasamiExtrasWidget::UnhoveredGrey;
		Button->SetColorAndOpacity(FLinearColor(Grey, Grey, Grey, 1.f));
		Button->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
	}

	FLinearColor SectionColour(bool bLit)
	{
		const float Grey = bLit ? 1.f : UWasamiExtrasWidget::UnhoveredGrey;
		return FLinearColor(Grey, Grey, Grey, 1.f);
	}
}

const TCHAR* const UWasamiExtrasWidget::CreditsText = TEXT(
	"<RED_BOLD>WASAMI DECEPTION</>\n"
	"<REG>A DARK DECEPTION FAN GAME</>\n"
	"\n"
	"<RED>ORIGINAL GAME</>\n"
	"<REG>DARK DECEPTION</>\n"
	"<REG>GLOWSTICK ENTERTAINMENT</>");

UWasamiExtrasWidget::UWasamiExtrasWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SaveSlotName = UWasamiSaveGame::SlotName;
	BackdropMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/MM_TitleScreen_Mask_")));
	NormalFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	BoldFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
	CreditsFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Engine/EngineFonts/Roboto")));
	OpenSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Window_PopUp_V2")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
	// TODO(仮): this game's pictures in the places the hospital's files unlock, until the pictures are decided.
	ArtTextures.SetNum(ArtCount);
	ArtTextures[19] = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Title/T_TitleFace")));
	ArtTextures[20] = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Pause/T_PausePeek")));
	ArtTextures[21] = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Pause/T_PauseHead")));
	ArtTextures[22] = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/Portal/T_Portal_Wasami")));

	// The Sound Archive's tracks: this game's four music-class sounds, in the order they are first heard. The Zone 1
	// track keeps the name the original's own Sound Archive gives it (its Sound 5, UMG_Extras' Extras_Sound_Button_C_14).
	SoundTracks.SetNum(SoundCount);
	SoundTracks[0].Name = FText::FromString(TEXT("Cold Hearted"));
	SoundTracks[0].Sound = ExtrasMusicTrack(TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING"));
	SoundTracks[1].Name = FText::FromString(TEXT("Hospital Panic Track"));
	SoundTracks[1].Sound = ExtrasMusicTrack(TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_-_Panic_Track_v1_2_-_LOOPING"));
	SoundTracks[2].Name = FText::FromString(TEXT("Hospital Zone 2 Normal Track"));
	SoundTracks[2].Sound = ExtrasMusicTrack(TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING"));
	SoundTracks[3].Name = FText::FromString(TEXT("Pause Theme"));
	SoundTracks[3].Sound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Pause_Sound_v1")));
}

UWasamiExtrasWidget* UWasamiExtrasWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiExtrasWidget* Screen = CreateWidget<UWasamiExtrasWidget>(Controller, StaticClass());
	if (Screen)
	{
		Screen->AddToViewport(ViewportZOrder);
	}
	return Screen;
}

TSharedRef<SWidget> UWasamiExtrasWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// The parts' locks are read from one save (the original reads SaveSlot in each part's Construct).
		if (!Save)
		{
			Save = UGameplayStatics::DoesSaveGameExist(SaveSlotName, UWasamiSaveGame::UserIndex)
				? Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UWasamiSaveGame::UserIndex)) : nullptr;
			if (!Save)
			{
				Save = NewObject<UWasamiSaveGame>(this);
			}
		}
		UFont* Normal = NormalFont.LoadSynchronous();

		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;

		// Button_116: black over the whole screen (UE 4's default button tinted black; Image_0 over it takes the pointer).
		UButton* Black = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_116"));
		FSlateBrush BlackBrush;
		BlackBrush.ImageSize = FVector2D(32.f, 32.f);
		BlackBrush.DrawAs = ESlateBrushDrawType::Box;
		BlackBrush.Margin = FMargin(8.f / 32.f);
		BlackBrush.TintColor = FSlateColor(FLinearColor::Black);
		FButtonStyle BlackStyle = Black->GetStyle();
		BlackStyle.SetNormal(BlackBrush).SetHovered(BlackBrush).SetPressed(BlackBrush).SetNormalPadding(FMargin(2.f))
			.SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Black->SetStyle(BlackStyle);
		ExtrasScreenPlace(Root, Black, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

		// Image_0: the title's brush strokes panning over the whole screen (MM_TitleScreen_Mask_).
		Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_0"));
		if (UMaterialInterface* Material = BackdropMaterial.LoadSynchronous())
		{
			Backdrop->SetBrushFromMaterial(Material);
		}
		ExtrasScreenPlace(Root, Backdrop, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

		// BackgroundBlur_0: what is under it blurred (3), letting the pointer through.
		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_0"));
		Blur->SetBlurStrength(3.f);
		ExtrasScreenPlace(Root, Blur, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

		// VerticalBox_0: the sections, 70 px in from the left's middle (helvetica-normal 36, left aligned in their buttons).
		SectionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_0"));
		SectionButtons.SetNum(SectionCount);
		struct FSectionTree
		{
			int32 Section;
			const TCHAR* Button;
			const TCHAR* Text;
			const TCHAR* Label;
		};
		const FSectionTree Sections[] = {
			{ArtGallerySection, TEXT("ArtGallery"), TEXT("TextBlock_1"), TEXT("ART GALLERY")},
			{BierceDiariesSection, TEXT("BierceDiaries"), TEXT("TextBlock_2"), TEXT("BIERCE DIARIES")},
			{SoundArchiveSection, TEXT("SoundArchive"), TEXT("TextBlock_3"), TEXT("SOUND ARCHIVE")},
			{MoviesSection, TEXT("Movies"), TEXT("TextBlock_4"), TEXT("MOVIES")},
			{CreditsSection, TEXT("Credits"), TEXT("TextBlock_5"), TEXT("CREDITS")}};
		for (const FSectionTree& Each : Sections)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Each.Button);
			ExtrasTextButton(Button);
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Each.Text);
			Text->SetText(FText::FromString(Each.Label));
			Text->SetFont(ExtrasFont(Normal, 36.f));
			if (UButtonSlot* TextSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Left);
			}
			SectionBox->AddChildToVerticalBox(Button);
			SectionButtons[Each.Section] = Button;
		}
		// MOVIES is hidden and disabled.
		SectionButtons[MoviesSection]->SetIsEnabled(false);
		SectionButtons[MoviesSection]->SetVisibility(ESlateVisibility::Hidden);
		ExtrasScreenPlace(Root, SectionBox, FAnchors(0.f, 0.5f), FMargin(70.02057647705078f, 0.f, 474.3743591308594f, 513.3007202148438f),
			FVector2D(0.f, 0.5f), true);

		// Back: BACK (helvetica-neue-bold 24) 24 px in from the bottom left, 69 px up.
		BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Back"));
		ExtrasTextButton(BackButton);
		UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_0"));
		BackText->SetText(FText::FromString(TEXT("BACK")));
		BackText->SetFont(ExtrasFont(BoldFont.LoadSynchronous(), 24.f));
		BackButton->AddChild(BackText);
		ExtrasScreenPlace(Root, BackButton, FAnchors(0.f, 1.f), FMargin(24.f, -69.0810546875f, 100.f, 30.f), FVector2D::ZeroVector, true);

		// Slider_0: a thin red line (a vertical slider at 1, its bar 6 thick in red at 0.5 and its handle clear) 746 px
		// long, 452 px in from the left's middle.
		Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("Slider_0"));
		Slider->SetValue(1.f);
		FSliderStyle SliderStyle = Slider->GetWidgetStyle();
		SliderStyle.SetBarThickness(6.f);
		Slider->SetWidgetStyle(SliderStyle);
		Slider->SetOrientation(Orient_Vertical);
		Slider->SetSliderBarColor(FLinearColor(1.f, 0.f, 0.f, 0.5f));
		Slider->SetSliderHandleColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
		ExtrasScreenPlace(Root, Slider, FAnchors(0.f, 0.5f), FMargin(452.f, 0.f, 100.f, 746.2161865234375f), FVector2D(0.f, 0.5f));

		// WidgetSwitcher_276: the sections' pages, from 401 px left of the middle (1328 × 1031).
		Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("WidgetSwitcher_276"));
		ExtrasScreenPlace(Root, Switcher, FAnchors(0.5f), FMargin(-400.9609375f, -514.5585327148438f, 1328.32421875f, 1030.990966796875f));

		// 0 HorizontalBox_0 → RichTextBlock_258: the credits in the middle (10 px off the bottom), centred and wrapped,
		// 1110 px down until Credits_Scroll moves them.
		UHorizontalBox* CreditsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_0"));
		Credits = WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), TEXT("RichTextBlock_258"));
		CreditsStyles = MakeCreditsStyles();
		Credits->SetTextStyleSet(CreditsStyles);
		Credits->SetText(FText::FromString(CreditsText));
		Credits->SetJustification(ETextJustify::Center);
		Credits->SetAutoWrapText(true);
		if (UHorizontalBoxSlot* TextSlot = CreditsBox->AddChildToHorizontalBox(Credits))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UWidgetSwitcherSlot* PageSlot = Cast<UWidgetSwitcherSlot>(Switcher->AddChild(CreditsBox)))
		{
			PageSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
			PageSlot->SetHorizontalAlignment(HAlign_Center);
			PageSlot->SetVerticalAlignment(VAlign_Center);
		}

		// 1 WrapBox_0: the 35 pictures, 30 px apart, wrapping at the page's width.
		ArtBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("WrapBox_0"));
		ArtBox->SetInnerSlotPadding(FVector2D(30.f, 30.f));
		for (int32 ID = 0; ID < ArtCount; ++ID)
		{
			UWasamiExtrasItemWidget* Item = WidgetTree->ConstructWidget<UWasamiExtrasItemWidget>(
				UWasamiExtrasItemWidget::StaticClass(), *FString::Printf(TEXT("Extras_Art_%d"), ID));
			Item->ID = ID;
			Item->Art = ArtTextures.IsValidIndex(ID) ? ArtTextures[ID].LoadSynchronous() : nullptr;
			Item->Save = Save;
			ArtBox->AddChildToWrapBox(Item);
			ArtItems.Add(Item);
		}
		if (UWidgetSwitcherSlot* PageSlot = Cast<UWidgetSwitcherSlot>(Switcher->AddChild(ArtBox)))
		{
			PageSlot->SetVerticalAlignment(VAlign_Center);
		}

		// 2 CanvasPanel_19 and 3 CanvasPanel_20: a player 208 px above the middle and under it its sounds (five a row at
		// 0.95), the diaries' (ten, TODO(仮) empty until they are decided with the user) and the sound archive's
		// (SoundTracks, this game's four tracks where the original has ten).
		auto AddSoundPage = [this](const TCHAR* PanelName, const TCHAR* BarName, const TCHAR* BoxName, bool bDiary,
			int32 Count, TObjectPtr<UWasamiExtrasSoundBarWidget>& OutBar, TObjectPtr<UWrapBox>& OutBox,
			TArray<TObjectPtr<UWasamiExtrasSoundButtonWidget>>& OutButtons)
		{
			UCanvasPanel* Page = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), PanelName);
			OutBar = WidgetTree->ConstructWidget<UWasamiExtrasSoundBarWidget>(UWasamiExtrasSoundBarWidget::StaticClass(), BarName);
			ExtrasScreenPlace(Page, OutBar, FAnchors(0.5f), FMargin(-661.162109375f, -207.7221221923828f, 1324.f, 30.f));
			OutBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), BoxName);
			OutBox->SetInnerSlotPadding(FVector2D(30.f, 30.f));
			OutBox->SetWrapSize(1635.440185546875f);
			OutBox->SetExplicitWrapSize(true);
			OutBox->SetRenderScale(FVector2D(0.949999988079071f, 0.949999988079071f));
			for (int32 ID = 0; ID < Count; ++ID)
			{
				UWasamiExtrasSoundButtonWidget* Button = WidgetTree->ConstructWidget<UWasamiExtrasSoundButtonWidget>(
					UWasamiExtrasSoundButtonWidget::StaticClass(),
					*FString::Printf(TEXT("%s_%d"), bDiary ? TEXT("Extras_Diary") : TEXT("Extras_Sound"), ID));
				Button->ID = ID;
				Button->bDiary = bDiary;
				Button->SoundBar = OutBar;
				Button->Save = Save;
				if (!bDiary && SoundTracks.IsValidIndex(ID))
				{
					Button->Text = SoundTracks[ID].Name;
					Button->Sound = SoundTracks[ID].Sound.LoadSynchronous();
				}
				OutBox->AddChildToWrapBox(Button);
				OutButtons.Add(Button);
			}
			ExtrasScreenPlace(Page, OutBox, FAnchors(0.5f), FMargin(0.f, -107.29153442382812f, 1289.f, 573.f), FVector2D(0.5f, 0.f), true);
			Switcher->AddChild(Page);
		};
		AddSoundPage(TEXT("CanvasPanel_19"), TEXT("UMG_Extras_Sound_Bar"), TEXT("WrapBox_1"), true, DiaryCount,
			DiaryBar, DiaryBox, DiaryButtons);
		AddSoundPage(TEXT("CanvasPanel_20"), TEXT("UMG_Extras_Sound_Bar_C_0"), TEXT("WrapBox_2"), false, SoundCount,
			SoundBar, SoundBox, SoundButtons);

		// 4 WrapBox_3: the ten movies, 30 px apart, wrapping at 1553 and moved 27 px left.
		VideoBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("WrapBox_3"));
		VideoBox->SetInnerSlotPadding(FVector2D(30.f, 30.f));
		VideoBox->SetWrapSize(1553.4720458984375f);
		VideoBox->SetExplicitWrapSize(true);
		VideoBox->SetRenderTranslation(FVector2D(-27.f, 0.f));
		for (int32 Index = 0; Index < VideoCount; ++Index)
		{
			UWasamiExtrasVideoWidget* Item = WidgetTree->ConstructWidget<UWasamiExtrasVideoWidget>(
				UWasamiExtrasVideoWidget::StaticClass(), *FString::Printf(TEXT("Extras_Video_%d"), Index));
			Item->ID = VideoIDs[Index];
			Item->Save = Save;
			VideoBox->AddChildToWrapBox(Item);
			VideoItems.Add(Item);
		}
		if (UWidgetSwitcherSlot* PageSlot = Cast<UWidgetSwitcherSlot>(Switcher->AddChild(VideoBox)))
		{
			PageSlot->SetVerticalAlignment(VAlign_Center);
		}
		// The tree's page (Construct shows the Art Gallery).
		Switcher->SetActiveWidgetIndex(BierceDiariesSection);

		// The original's ComponentDelegateBinding: each section's and BACK's click, hover and unhover.
		SectionButtons[CreditsSection]->OnClicked.AddDynamic(this, &UWasamiExtrasWidget::OnCreditsClicked);
		SectionButtons[ArtGallerySection]->OnClicked.AddDynamic(this, &UWasamiExtrasWidget::OnArtGalleryClicked);
		SectionButtons[BierceDiariesSection]->OnClicked.AddDynamic(this, &UWasamiExtrasWidget::OnBierceDiariesClicked);
		SectionButtons[SoundArchiveSection]->OnClicked.AddDynamic(this, &UWasamiExtrasWidget::OnSoundArchiveClicked);
		SectionButtons[MoviesSection]->OnClicked.AddDynamic(this, &UWasamiExtrasWidget::OnMoviesClicked);
		SectionButtons[CreditsSection]->OnHovered.AddDynamic(this, &UWasamiExtrasWidget::OnCreditsHovered);
		SectionButtons[CreditsSection]->OnUnhovered.AddDynamic(this, &UWasamiExtrasWidget::OnCreditsUnhovered);
		SectionButtons[ArtGallerySection]->OnHovered.AddDynamic(this, &UWasamiExtrasWidget::OnArtGalleryHovered);
		SectionButtons[ArtGallerySection]->OnUnhovered.AddDynamic(this, &UWasamiExtrasWidget::OnArtGalleryUnhovered);
		SectionButtons[BierceDiariesSection]->OnHovered.AddDynamic(this, &UWasamiExtrasWidget::OnBierceDiariesHovered);
		SectionButtons[BierceDiariesSection]->OnUnhovered.AddDynamic(this, &UWasamiExtrasWidget::OnBierceDiariesUnhovered);
		SectionButtons[SoundArchiveSection]->OnHovered.AddDynamic(this, &UWasamiExtrasWidget::OnSoundArchiveHovered);
		SectionButtons[SoundArchiveSection]->OnUnhovered.AddDynamic(this, &UWasamiExtrasWidget::OnSoundArchiveUnhovered);
		SectionButtons[MoviesSection]->OnHovered.AddDynamic(this, &UWasamiExtrasWidget::OnMoviesHovered);
		SectionButtons[MoviesSection]->OnUnhovered.AddDynamic(this, &UWasamiExtrasWidget::OnMoviesUnhovered);
		BackButton->OnClicked.AddDynamic(this, &UWasamiExtrasWidget::OnBackClicked);
		BackButton->OnHovered.AddDynamic(this, &UWasamiExtrasWidget::OnBackHovered);
		BackButton->OnUnhovered.AddDynamic(this, &UWasamiExtrasWidget::OnBackUnhovered);
		ApplyAnimation();
	}
	return Super::RebuildWidget();
}

UDataTable* UWasamiExtrasWidget::MakeCreditsStyles()
{
	// RichText_Credits (pak_reference_2/_datatables.json), row for row: the font, its size and the colour; no shadow.
	UDataTable* Table = NewObject<UDataTable>(this, TEXT("RichText_Credits"));
	Table->RowStruct = FRichTextStyleRow::StaticStruct();
	UFont* Roboto = CreditsFont.LoadSynchronous();
	UFont* Normal = NormalFont.LoadSynchronous();
	auto AddRow = [Table](const TCHAR* Name, UFont* Font, const TCHAR* Typeface, float Size, const FLinearColor& Colour)
	{
		FRichTextStyleRow Row;
		Row.TextStyle.SetFont(ExtrasFont(Font, Size, Typeface))
			.SetColorAndOpacity(FSlateColor(Colour))
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black);
		Table->AddRow(Name, Row);
	};
	AddRow(TEXT("REG"), Roboto, TEXT("Light"), 16.f, FLinearColor(0.9646859765052795f, 0.9646859765052795f, 0.9646859765052795f, 1.f));
	AddRow(TEXT("YEL"), Normal, TEXT("Default"), 16.f, FLinearColor(1.f, 0.901856005191803f, 0.f, 1.f));
	AddRow(TEXT("Locked"), Normal, TEXT("Default"), 16.f, FLinearColor(0.24479199945926666f, 0.24479199945926666f, 0.24479199945926666f, 1.f));
	AddRow(TEXT("RED"), Roboto, TEXT("Bold"), 16.f, FLinearColor(0.9215819835662842f, 0.0036770000588148832f, 0.0015180000336840749f, 1.f));
	AddRow(TEXT("RED_BOLD"), Roboto, TEXT("Bold"), 18.f, FLinearColor::White);
	return Table;
}

void UWasamiExtrasWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiExtrasWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiExtrasWidget::Begin()
{
	// @649: PlaySound2D(UI_Window_PopUp_V2), PlayAnimation(FadeIn, 0, 1, Forward, 2), then @755 (the Art Gallery).
	if (GetWorld())
	{
		if (USoundBase* Loaded = OpenSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded);
		}
	}
	FadeInTime = 0.f;
	bFadeInReverse = false;
	bClosing = bFinished = false;
	CloseElapsed = 0.f;
	ShowSection(ArtGallerySection);
	ApplyAnimation();
}

void UWasamiExtrasWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// The animations move on first and the Delay comes due after them, as a user widget ticks them.
	const float Step = DeltaSeconds * FadeInSpeed;
	FadeInTime = bFadeInReverse ? FMath::Max(FadeInTime - Step, 0.f) : FMath::Min(FadeInTime + Step, FadeInLength);
	if (bCreditsScrolling)
	{
		CreditsTime = FMath::Min(CreditsTime + DeltaSeconds, CreditsScrollLength);
	}
	ApplyAnimation();
	if (bClosing)
	{
		CloseElapsed += DeltaSeconds;
		if (CloseElapsed >= BackDelay)
		{
			// @15: broadcast FadeMusic, RemoveFromParent.
			bFinished = true;
			OnFadeMusic.Broadcast();
			RemoveFromParent();
		}
	}
}

void UWasamiExtrasWidget::Select(int32 Section)
{
	if (Section < 0 || Section >= SectionCount)
	{
		return;
	}
	// ART GALLERY, BIERCE DIARIES and SOUND ARCHIVE: EqualEqual(Active Button, theirs) → nothing.
	if (Section != CreditsSection && Section != MoviesSection && ActiveButton == Section)
	{
		return;
	}
	// ART GALLERY sounds first (@2268 → @755); the others after (@2539, @3277, @3010, @3563).
	if (Section == ArtGallerySection)
	{
		PlaySelectSound(1.f);
	}
	ShowSection(Section);
	if (Section != ArtGallerySection)
	{
		PlaySelectSound(1.f);
	}
}

void UWasamiExtrasWidget::ShowSection(int32 Section)
{
	// WidgetSwitcher_276.SetActiveWidgetIndex, Active Button, Reset All Colors, the button white, CREDITS'
	// PlayAnimation(Credits_Scroll, 0, 1, Forward, 1) and Check If Playing.
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Section);
	}
	ActiveButton = Section;
	ResetAllColors();
	if (UButton* Button = GetSectionButton(Section))
	{
		Button->SetColorAndOpacity(SectionColour(true));
	}
	if (Section == CreditsSection)
	{
		CreditsTime = 0.f;
		bCreditsScrolling = true;
		ApplyAnimation();
	}
	CheckIfPlaying();
}

void UWasamiExtrasWidget::HoverSection(int32 Section, bool bHovered)
{
	// The hovers (@1226 and its like: white) and unhovers (@1184: Unhovered Color), unless Active Button is theirs.
	if (ActiveButton == Section)
	{
		return;
	}
	if (UButton* Button = GetSectionButton(Section))
	{
		Button->SetColorAndOpacity(SectionColour(bHovered));
	}
}

void UWasamiExtrasWidget::PressBack()
{
	// @1023: PlayAnimation(FadeIn, 0, 1, Reverse, 2), which a reverse play starts at the end; PlaySound2D(UI_Select_V3,
	// 1, 0.7); Delay(0.25), which a pending Delay ignores.
	FadeInTime = FadeInLength;
	bFadeInReverse = true;
	ApplyAnimation();
	PlaySelectSound(BackPitch);
	if (!bClosing)
	{
		bClosing = true;
		CloseElapsed = 0.f;
	}
}

void UWasamiExtrasWidget::HoverBack(bool bHovered)
{
	if (BackButton)
	{
		BackButton->SetColorAndOpacity(SectionColour(bHovered));
	}
}

void UWasamiExtrasWidget::ResetAllColors()
{
	for (UButton* Button : SectionButtons)
	{
		if (Button)
		{
			Button->SetColorAndOpacity(SectionColour(false));
		}
	}
}

void UWasamiExtrasWidget::CheckIfPlaying()
{
	// @3337: GetAllWidgetsOfClass(UMG_Extras_Sound_Button, not top level only): the world's (or, without one, those
	// without one), and each Playing? one Deselect'ed and its bar's Sound.Stop().
	UWorld* World = GetWorld();
	for (TObjectIterator<UWasamiExtrasSoundButtonWidget> Each; Each; ++Each)
	{
		if (IsValid(*Each) && !Each->IsTemplate() && Each->GetWorld() == World && Each->IsPlaying())
		{
			Each->Deselect();
			if (Each->SoundBar)
			{
				Each->SoundBar->StopSound();
			}
		}
	}
}

void UWasamiExtrasWidget::PlaySelectSound(float Pitch)
{
	if (GetWorld())
	{
		if (USoundBase* Loaded = SelectSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, Pitch);
		}
	}
}

void UWasamiExtrasWidget::ApplyAnimation()
{
	if (Root)
	{
		Root->SetRenderOpacity(EvaluateOpacity(FadeInTime));
	}
	if (Credits)
	{
		Credits->SetRenderTranslation(FVector2D(0.f, EvaluateCreditsY(CreditsTime)));
	}
}

float UWasamiExtrasWidget::EvaluateOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ExtrasOpacityKeys);
	return Eval(Curve, Seconds, FadeInLength);
}

float UWasamiExtrasWidget::EvaluateCreditsY(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CreditsYKeys);
	return Eval(Curve, Seconds, CreditsScrollLength);
}

void UWasamiExtrasWidget::OnCreditsClicked()
{
	Select(CreditsSection);
}

void UWasamiExtrasWidget::OnArtGalleryClicked()
{
	Select(ArtGallerySection);
}

void UWasamiExtrasWidget::OnBierceDiariesClicked()
{
	Select(BierceDiariesSection);
}

void UWasamiExtrasWidget::OnSoundArchiveClicked()
{
	Select(SoundArchiveSection);
}

void UWasamiExtrasWidget::OnMoviesClicked()
{
	Select(MoviesSection);
}

void UWasamiExtrasWidget::OnCreditsHovered()
{
	HoverSection(CreditsSection, true);
}

void UWasamiExtrasWidget::OnCreditsUnhovered()
{
	HoverSection(CreditsSection, false);
}

void UWasamiExtrasWidget::OnArtGalleryHovered()
{
	HoverSection(ArtGallerySection, true);
}

void UWasamiExtrasWidget::OnArtGalleryUnhovered()
{
	HoverSection(ArtGallerySection, false);
}

void UWasamiExtrasWidget::OnBierceDiariesHovered()
{
	HoverSection(BierceDiariesSection, true);
}

void UWasamiExtrasWidget::OnBierceDiariesUnhovered()
{
	HoverSection(BierceDiariesSection, false);
}

void UWasamiExtrasWidget::OnSoundArchiveHovered()
{
	HoverSection(SoundArchiveSection, true);
}

void UWasamiExtrasWidget::OnSoundArchiveUnhovered()
{
	HoverSection(SoundArchiveSection, false);
}

void UWasamiExtrasWidget::OnMoviesHovered()
{
	HoverSection(MoviesSection, true);
}

void UWasamiExtrasWidget::OnMoviesUnhovered()
{
	HoverSection(MoviesSection, false);
}

void UWasamiExtrasWidget::OnBackClicked()
{
	PressBack();
}

void UWasamiExtrasWidget::OnBackHovered()
{
	HoverBack(true);
}

void UWasamiExtrasWidget::OnBackUnhovered()
{
	HoverBack(false);
}
