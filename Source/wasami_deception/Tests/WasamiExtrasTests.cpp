#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/RichTextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WrapBox.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundWave.h"
#include "../WasamiExtrasItemWidget.h"
#include "../WasamiExtrasSoundWidget.h"
#include "../WasamiExtrasWidget.h"
#include "../WasamiMaximizePictureWidget.h"
#include "../WasamiSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * A save as a played game writes it: Art Gallery 19 and 20 unlocked, the two the hospital's Zone 1 file gives, and
	 * nothing else. Extras_SFX stays empty, this game listing no track (2026-09-23, the user's 「EXTRAS に曲は不要」);
	 * the tests that want an unlocked sound unlock one themselves.
	 */
	UWasamiSaveGame* ExtrasSave()
	{
		UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
		Save->Unlock(FWasamiCollectableEntry{EWasamiCollectableType::ArtGallery, 19});
		Save->Unlock(FWasamiCollectableEntry{EWasamiCollectableType::ArtGallery, 20});
		return Save;
	}

	/** A widget of class T with its tree built and Construct's run given the save, as the extras screen would add it. */
	template <typename T>
	T* MakeExtra(UWasamiSaveGame* Save, int32 ID)
	{
		T* Widget = NewObject<T>();
		Widget->ID = ID;
		Widget->Save = Save;
		Widget->Initialize();
		Widget->TakeWidget();
		Widget->Begin();
		return Widget;
	}

	UTexture2D* ExtrasTestTexture()
	{
		return LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	}

	const UObject* BrushTexture(const UImage* Image)
	{
		return Image ? Image->GetBrush().GetResourceObject() : nullptr;
	}

	bool NearlyColour(const FLinearColor& A, const FLinearColor& B)
	{
		return A.Equals(B, 1.e-3f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiExtrasItemTest, "Wasami.Extras.Item",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiExtrasItemTest::RunTest(const FString& Parameters)
{
	UWasamiSaveGame* Save = ExtrasSave();
	UTexture2D* Art = ExtrasTestTexture();

	// An unlocked picture: Art over the button, which takes the pointer.
	UWasamiExtrasItemWidget* Unlocked = NewObject<UWasamiExtrasItemWidget>();
	Unlocked->ID = 19;
	Unlocked->Art = Art;
	Unlocked->Save = Save;
	Unlocked->Initialize();
	Unlocked->TakeWidget();
	Unlocked->Begin();
	TestTrue(TEXT("19 is unlocked"), Unlocked->IsUnlocked());
	TestEqual(TEXT("it shows its Art"), BrushTexture(Unlocked->GetImage()), static_cast<const UObject*>(Art));
	TestEqual(TEXT("at the tree's 2500 (the brush's size kept)"), FVector2D(Unlocked->GetImage()->GetBrush().ImageSize), FVector2D(2500.f, 2500.f));
	TestTrue(TEXT("its button is Visible"), Unlocked->GetButton()->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("a press opens it (no screen to go on here)"), Unlocked->Press());

	// Construct's style: Normal the frame in grey (0.1 kept), Hovered the frame in white, Pressed a plain white square.
	const FButtonStyle& Style = Unlocked->GetButton()->GetStyle();
	TestTrue(TEXT("Normal is tinted 0.4531"), NearlyColour(Style.Normal.TintColor.GetSpecifiedColor(), FLinearColor(0.4531f, 0.4531f, 0.4531f, 1.f)));
	TestEqual(TEXT("Normal is 150"), FVector2D(Style.Normal.ImageSize), FVector2D(150.f, 150.f));
	TestEqual(TEXT("Normal keeps 0.1 at its edges"), Style.Normal.Margin.Left, 0.1f);
	TestTrue(TEXT("Normal is drawn as a box"), Style.Normal.DrawAs.GetValue() == ESlateBrushDrawType::Box);
	TestTrue(TEXT("Normal is the frame"), Style.Normal.GetResourceObject() != nullptr && Style.Normal.GetResourceObject()->GetName() == TEXT("ring_altar_power_equipped_frame"));
	TestTrue(TEXT("Hovered is white"), NearlyColour(Style.Hovered.TintColor.GetSpecifiedColor(), FLinearColor::White));
	TestEqual(TEXT("Hovered is 105 x 104"), FVector2D(Style.Hovered.ImageSize), FVector2D(105.f, 104.f));
	TestNull(TEXT("Pressed has no texture"), Style.Pressed.GetResourceObject());
	TestTrue(TEXT("Pressed is drawn as an image"), Style.Pressed.DrawAs.GetValue() == ESlateBrushDrawType::Image);

	// The tree: the button a 150 square in the middle, the scale box filling the picture but for 5 px and cropping.
	const UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Unlocked->GetButton()->Slot);
	TestTrue(TEXT("the button is 150 in the middle"), ButtonSlot && ButtonSlot->GetOffsets() == FMargin(0.f, 0.f, 150.f, 150.f)
		&& ButtonSlot->GetAlignment() == FVector2D(0.5f, 0.5f));
	const UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(Unlocked->GetScaleBox()->Slot);
	TestTrue(TEXT("the scale box fills but for 5"), BoxSlot && BoxSlot->GetOffsets() == FMargin(5.f)
		&& BoxSlot->GetAnchors().Maximum == FVector2D(1.f, 1.f));
	TestTrue(TEXT("and fills (cropping)"), Unlocked->GetScaleBox()->GetStretch() == EStretch::ScaleToFill);
	TestTrue(TEXT("letting the pointer through"), Unlocked->GetScaleBox()->GetVisibility() == ESlateVisibility::HitTestInvisible);

	// A locked one: the lock, and the button lets the pointer through.
	UWasamiExtrasItemWidget* Locked = MakeExtra<UWasamiExtrasItemWidget>(Save, 18);
	TestFalse(TEXT("18 is locked"), Locked->IsUnlocked());
	TestTrue(TEXT("it shows the lock"), BrushTexture(Locked->GetImage()) != nullptr && BrushTexture(Locked->GetImage())->GetName() == TEXT("locked"));
	TestTrue(TEXT("its button is HitTestInvisible"), Locked->GetButton()->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestFalse(TEXT("a press does nothing"), Locked->Press());

	// Sound 5's ID is not a picture's (nothing in this game unlocks a sound; the save keeps the original's Extras_SFX).
	Save->Unlock(FWasamiCollectableEntry{EWasamiCollectableType::Sound, 5});
	TestFalse(TEXT("Extras_SFX does not unlock a picture"), MakeExtra<UWasamiExtrasItemWidget>(Save, 5)->IsUnlocked());

	// No save in the slot: a new one (the original's title makes SaveSlot first), with nothing unlocked.
	UWasamiExtrasItemWidget* Unsaved = NewObject<UWasamiExtrasItemWidget>();
	Unsaved->ID = 19;
	Unsaved->SaveSlotName = TEXT("WasamiTest_Extras_None");
	Unsaved->Initialize();
	Unsaved->TakeWidget();
	Unsaved->Begin();
	TestNotNull(TEXT("a new save is taken"), Unsaved->Save.Get());
	TestFalse(TEXT("and 19 is locked in it"), Unsaved->IsUnlocked());

	// A movie: 250 (its Normal 300), and locked whatever the save has.
	UWasamiExtrasVideoWidget* Video = MakeExtra<UWasamiExtrasVideoWidget>(Save, 0);
	TestFalse(TEXT("a movie is locked"), Video->IsUnlocked());
	TestTrue(TEXT("its button is HitTestInvisible"), Video->GetButton()->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("it shows the lock"), BrushTexture(Video->GetImage()) != nullptr && BrushTexture(Video->GetImage())->GetName() == TEXT("locked"));
	const UCanvasPanelSlot* VideoSlot = Cast<UCanvasPanelSlot>(Video->GetButton()->Slot);
	TestTrue(TEXT("the movie's button is 250"), VideoSlot && VideoSlot->GetOffsets() == FMargin(0.f, 0.f, 250.f, 250.f));
	TestEqual(TEXT("its Normal is 300"), FVector2D(Video->GetButton()->GetStyle().Normal.ImageSize), FVector2D(300.f, 300.f));
	TestFalse(TEXT("a press does nothing"), Video->Press());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiExtrasMaximizePictureTest, "Wasami.Extras.MaximizePicture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiExtrasMaximizePictureTest::RunTest(const FString& Parameters)
{
	UTexture2D* Art = ExtrasTestTexture();
	UWasamiMaximizePictureWidget* View = NewObject<UWasamiMaximizePictureWidget>();
	View->Texture = Art;
	View->Text = FText::FromString(TEXT("by someone"));
	View->Initialize();
	View->TakeWidget();
	View->Begin();

	// The tree: the dim, the blur (6), the picture 50 px in on its black ground 100 px in from the edges, the text, BACK.
	TestEqual(TEXT("the picture at its own size"), FVector2D(View->GetImage()->GetBrush().ImageSize),
		FVector2D(Art->GetSizeX(), Art->GetSizeY()));
	TestEqual(TEXT("the picture shown"), BrushTexture(View->GetImage()), static_cast<const UObject*>(Art));
	TestEqual(TEXT("the blur is 6"), View->GetBlur()->GetBlurStrength(), 6.f);
	TestTrue(TEXT("the dim lets the pointer through"), View->GetDim()->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("the dim is black at 0.5"), NearlyColour(View->GetDim()->GetStyle().Normal.TintColor.GetSpecifiedColor(), FLinearColor(0.f, 0.f, 0.f, 0.5f)));
	const UCanvasPanelSlot* BoxSlot = Cast<UCanvasPanelSlot>(View->GetScaleBox()->Slot);
	TestTrue(TEXT("the picture 100 px in"), BoxSlot && BoxSlot->GetOffsets() == FMargin(100.f));
	TestEqual(TEXT("the text is the Text"), View->GetTextBlock()->GetText().ToString(), FString(TEXT("by someone")));
	TestTrue(TEXT("BACK dark grey"), NearlyColour(View->GetBackButton()->GetColorAndOpacity(), FLinearColor(0.11f, 0.11f, 0.11f, 1.f)));
	View->HoverBack(true);
	TestTrue(TEXT("white while hovered"), NearlyColour(View->GetBackButton()->GetColorAndOpacity(), FLinearColor::White));
	View->HoverBack(false);
	TestTrue(TEXT("dark grey again"), NearlyColour(View->GetBackButton()->GetColorAndOpacity(), FLinearColor(0.11f, 0.11f, 0.11f, 1.f)));

	// FadeIn at twice its speed: unseen and at half size at first; the picture full size at 0.125 s and the view
	// halfway in; all in at 0.25 s.
	TestEqual(TEXT("unseen at first"), View->GetRoot()->GetRenderOpacity(), 0.f);
	TestEqual(TEXT("at half size"), View->GetGround()->GetRenderTransform().Scale, FVector2D(0.5f, 0.5f));
	View->Advance(0.125f);
	TestTrue(TEXT("full size at 0.125 s"), View->GetGround()->GetRenderTransform().Scale.Equals(FVector2D(1.f, 1.f), 1.e-3f));
	TestEqual(TEXT("halfway in"), View->GetRoot()->GetRenderOpacity(), UWasamiMaximizePictureWidget::EvaluateOpacity(0.25f), 1.e-4f);
	TestTrue(TEXT("(between the ends)"), View->GetRoot()->GetRenderOpacity() > 0.3f && View->GetRoot()->GetRenderOpacity() < 0.7f);
	View->Advance(0.125f);
	TestEqual(TEXT("all in at 0.25 s"), View->GetRoot()->GetRenderOpacity(), 1.f, 1.e-4f);

	// BACK: backwards from the end, off 0.25 s later.
	View->PressBack();
	TestTrue(TEXT("closing"), View->IsClosing());
	TestEqual(TEXT("from the end"), View->GetFadeInTime(), UWasamiMaximizePictureWidget::FadeInLength);
	View->Advance(0.2f);
	TestFalse(TEXT("still on at 0.2 s"), View->IsFinished());
	View->Advance(0.06f);
	TestTrue(TEXT("off by 0.26 s"), View->IsFinished());
	TestEqual(TEXT("faded out"), View->GetRoot()->GetRenderOpacity(), 0.f, 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiExtrasSoundButtonTest, "Wasami.Extras.SoundButton",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiExtrasSoundButtonTest::RunTest(const FString& Parameters)
{
	using Button = UWasamiExtrasSoundButtonWidget;
	UWasamiSaveGame* Save = ExtrasSave();
	// The Sound Archive lists no track (the screen gives no button a Text or a Sound), but the button is the
	// original's all the same, so two sounds are unlocked here by hand to play one.
	Save->Unlock(FWasamiCollectableEntry{EWasamiCollectableType::Sound, 0});
	Save->Unlock(FWasamiCollectableEntry{EWasamiCollectableType::Sound, 1});

	// Locks: a diary within Level Ranks' nine is unlocked, one past them is not; a sound by Extras_SFX.
	Button* Diary = MakeExtra<Button>(Save, 0);
	TestTrue(TEXT("DIARY 1 unlocked"), Diary->IsUnlocked());
	TestEqual(TEXT("labelled DIARY 1"), Diary->GetLabel()->GetText().ToString(), FString(TEXT("DIARY 1")));
	TestNotNull(TEXT("its label is on"), Diary->GetLabel()->GetParent());
	TestTrue(TEXT("its square takes the pointer"), Diary->GetButton()->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("the play icon"), BrushTexture(Diary->GetIcon()) != nullptr && BrushTexture(Diary->GetIcon())->GetName() == TEXT("extras_play_icon"));
	Button* PastDiaries = MakeExtra<Button>(Save, Button::LevelRankCount);
	TestFalse(TEXT("DIARY 10 locked"), PastDiaries->IsUnlocked());
	TestNull(TEXT("its label is off"), PastDiaries->GetLabel()->GetParent());
	TestTrue(TEXT("its square lets the pointer through"), PastDiaries->GetButton()->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("the lock"), BrushTexture(PastDiaries->GetIcon()) != nullptr && BrushTexture(PastDiaries->GetIcon())->GetName() == TEXT("locked_-_Copy"));

	UWasamiExtrasSoundBarWidget* Bar = NewObject<UWasamiExtrasSoundBarWidget>();
	Bar->Initialize();
	Bar->TakeWidget();
	auto MakeSound = [Save, Bar](int32 ID, const TCHAR* Name)
	{
		Button* Sound = NewObject<Button>();
		Sound->ID = ID;
		Sound->bDiary = false;
		Sound->Text = FText::FromString(Name);
		Sound->SoundBar = Bar;
		Sound->Save = Save;
		Sound->Initialize();
		Sound->TakeWidget();
		Sound->Begin();
		return Sound;
	};
	Button* Unlocked = MakeSound(0, TEXT("Test Track"));
	TestTrue(TEXT("SOUND 1 unlocked"), Unlocked->IsUnlocked());
	TestEqual(TEXT("labelled SOUND 1"), Unlocked->GetLabel()->GetText().ToString(), FString(TEXT("SOUND 1")));
	TestFalse(TEXT("SOUND 4 locked"), MakeSound(3, TEXT("Test Track"))->IsUnlocked());

	// Hover: red, and back to dark red.
	Unlocked->Hover(true);
	TestTrue(TEXT("red while hovered"), NearlyColour(Unlocked->GetButton()->GetBackgroundColor(), Button::HoverColour));
	Unlocked->Hover(false);
	TestTrue(TEXT("dark red again"), NearlyColour(Unlocked->GetButton()->GetBackgroundColor(), Button::RestColour));

	// Presses: play (selected, white, pause icon, the bar given the name), pause, resume.
	Unlocked->Press();
	TestTrue(TEXT("playing"), Unlocked->IsPlaying());
	TestTrue(TEXT("selected"), Unlocked->IsSelected());
	TestTrue(TEXT("white"), NearlyColour(Unlocked->GetButton()->GetBackgroundColor(), Button::SelectedColour)
		&& NearlyColour(Unlocked->GetIcon()->GetColorAndOpacity(), Button::SelectedColour));
	TestTrue(TEXT("the pause icon"), BrushTexture(Unlocked->GetIcon())->GetName() == TEXT("extras_pause_icon"));
	TestEqual(TEXT("the bar has the name"), Bar->GetTitleText().ToString(), FString(TEXT("TEST TRACK")));
	Unlocked->Hover(true);
	TestTrue(TEXT("a hover leaves it white"), NearlyColour(Unlocked->GetButton()->GetBackgroundColor(), Button::SelectedColour));
	Unlocked->Press();
	TestFalse(TEXT("paused: not playing"), Unlocked->IsPlaying());
	TestTrue(TEXT("paused"), Unlocked->IsPaused());
	TestTrue(TEXT("the play icon"), BrushTexture(Unlocked->GetIcon())->GetName() == TEXT("extras_play_icon"));
	TestTrue(TEXT("still selected"), Unlocked->IsSelected());
	Unlocked->Press();
	TestTrue(TEXT("resumed"), Unlocked->IsPlaying());
	TestTrue(TEXT("Paused? stays on"), Unlocked->IsPaused());

	// Another's press deselects this one (back to rest, the play icon) and selects that one.
	Button* Other = MakeSound(1, TEXT("Other Track"));
	Other->Press();
	TestFalse(TEXT("the first stops playing"), Unlocked->IsPlaying());
	TestFalse(TEXT("nor is paused"), Unlocked->IsPaused());
	TestFalse(TEXT("nor selected"), Unlocked->IsSelected());
	TestTrue(TEXT("dark red"), NearlyColour(Unlocked->GetButton()->GetBackgroundColor(), Button::RestColour));
	TestTrue(TEXT("with the play icon"), BrushTexture(Unlocked->GetIcon())->GetName() == TEXT("extras_play_icon"));
	TestTrue(TEXT("the other plays"), Other->IsPlaying() && Other->IsSelected());
	TestEqual(TEXT("its name on the bar"), Bar->GetTitleText().ToString(), FString(TEXT("OTHER TRACK")));

	// The first again: selected once more (its gate was opened), the other deselected.
	Unlocked->Press();
	TestTrue(TEXT("selected again"), Unlocked->IsSelected() && Unlocked->IsPlaying());
	TestFalse(TEXT("the other deselected"), Other->IsSelected());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiExtrasSoundBarTest, "Wasami.Extras.SoundBar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiExtrasSoundBarTest::RunTest(const FString& Parameters)
{
	using Bar = UWasamiExtrasSoundBarWidget;
	TestEqual(TEXT("0:00"), Bar::FormatTime(0.f).ToString(), FString(TEXT("0:00")));
	TestEqual(TEXT("seconds in two digits"), Bar::FormatTime(65.f).ToString(), FString(TEXT("1:05")));
	TestEqual(TEXT("the part of a second dropped"), Bar::FormatTime(152.9f).ToString(), FString(TEXT("2:32")));

	UWasamiExtrasSoundBarWidget* Player = NewObject<Bar>();
	Player->Initialize();
	Player->TakeWidget();
	TestEqual(TEXT("no length before a sound"), Player->GetDurationBlock()->GetText().ToString(), FString(TEXT("0:00")));
	TestEqual(TEXT("nothing played"), Player->GetPlayedBar()->GetPercent(), 0.f);
	TestEqual(TEXT("no name"), Player->GetTitleBlock()->GetText().ToString(), FString());

	// The sound's report: half of a five-minute wave.
	USoundWave* Wave = NewObject<USoundWave>();
	Wave->Duration = 300.f;
	Player->SetSound(nullptr, FText::FromString(TEXT("Test Track")));
	Player->OnPlaybackPercent(Wave, 0.5f);
	Player->ApplyBindings();
	TestEqual(TEXT("the length"), Player->GetDurationBlock()->GetText().ToString(), FString(TEXT("5:00")));
	TestEqual(TEXT("the time played"), Player->GetElapsedBlock()->GetText().ToString(), FString(TEXT("2:30")));
	TestEqual(TEXT("the bar half full"), Player->GetPlayedBar()->GetPercent(), 0.5f);
	TestEqual(TEXT("the name in capitals"), Player->GetTitleBlock()->GetText().ToString(), FString(TEXT("TEST TRACK")));

	// The tree: the bars 148 px in from the sides, 30 high; the ground's fill never moves.
	const UCanvasPanelSlot* GroundSlot = Cast<UCanvasPanelSlot>(Player->GetGroundBar()->Slot);
	TestTrue(TEXT("the ground across the middle"), GroundSlot && GroundSlot->GetAnchors().Minimum == FVector2D(0.f, 0.5f)
		&& FMath::IsNearlyEqual(GroundSlot->GetOffsets().Left, 148.171f, 1.e-2f) && GroundSlot->GetOffsets().Bottom == 30.f);
	TestEqual(TEXT("the ground empty"), Player->GetGroundBar()->GetPercent(), 0.f);
	TestTrue(TEXT("the played bar dark red"), NearlyColour(Player->GetPlayedBar()->GetFillColorAndOpacity(), FLinearColor(0.5029f, 0.f, 0.f, 1.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiExtrasScreenTest, "Wasami.Extras.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiExtrasScreenTest::RunTest(const FString& Parameters)
{
	using Screen = UWasamiExtrasWidget;
	const FLinearColor Grey(0.11f, 0.11f, 0.11f, 1.f);
	UWasamiExtrasWidget* Extras = NewObject<UWasamiExtrasWidget>();
	Extras->Save = ExtrasSave();
	Extras->Initialize();
	Extras->TakeWidget();
	Extras->Begin();

	// Construct: the Art Gallery chosen (its button white, the rest dark grey), the screen faded out.
	TestEqual(TEXT("Active Button is the Art Gallery"), Extras->GetActiveButton(), Screen::ArtGallerySection);
	TestEqual(TEXT("and the switcher shows its page"), Extras->GetSwitcher()->GetActiveWidgetIndex(), 1);
	TestTrue(TEXT("ART GALLERY is white"), NearlyColour(Extras->GetSectionButton(Screen::ArtGallerySection)->GetColorAndOpacity(), FLinearColor::White));
	TestTrue(TEXT("CREDITS is dark grey"), NearlyColour(Extras->GetSectionButton(Screen::CreditsSection)->GetColorAndOpacity(), Grey));
	TestTrue(TEXT("SOUND ARCHIVE is dark grey"), NearlyColour(Extras->GetSectionButton(Screen::SoundArchiveSection)->GetColorAndOpacity(), Grey));
	TestEqual(TEXT("the screen starts clear"), Extras->GetRoot()->GetRenderOpacity(), 0.f);

	// The tree: the sections in the original's order (MOVIES hidden and disabled), the thin red line, the pages.
	UVerticalBox* Sections = Extras->GetSectionBox();
	TestTrue(TEXT("the sections are ART GALLERY, BIERCE DIARIES, SOUND ARCHIVE, MOVIES, CREDITS"),
		Sections->GetChildrenCount() == 5 && Sections->GetChildAt(0) == Extras->GetSectionButton(Screen::ArtGallerySection)
		&& Sections->GetChildAt(1) == Extras->GetSectionButton(Screen::BierceDiariesSection)
		&& Sections->GetChildAt(2) == Extras->GetSectionButton(Screen::SoundArchiveSection)
		&& Sections->GetChildAt(3) == Extras->GetSectionButton(Screen::MoviesSection)
		&& Sections->GetChildAt(4) == Extras->GetSectionButton(Screen::CreditsSection));
	UButton* Movies = Extras->GetSectionButton(Screen::MoviesSection);
	TestTrue(TEXT("MOVIES is hidden and disabled"), Movies->GetVisibility() == ESlateVisibility::Hidden && !Movies->GetIsEnabled());
	const UCanvasPanelSlot* SectionSlot = Cast<UCanvasPanelSlot>(Sections->Slot);
	TestTrue(TEXT("the sections sit 70 px in from the left's middle"), SectionSlot && FMath::IsNearlyEqual(SectionSlot->GetOffsets().Left, 70.02f, 1.e-2f)
		&& SectionSlot->GetAnchors().Minimum == FVector2D(0.f, 0.5f) && SectionSlot->GetAlignment() == FVector2D(0.f, 0.5f) && SectionSlot->GetAutoSize());
	TestTrue(TEXT("the line is a vertical slider at 1, 6 thick"), Extras->GetSlider()->GetOrientation() == Orient_Vertical
		&& Extras->GetSlider()->GetValue() == 1.f && Extras->GetSlider()->GetWidgetStyle().BarThickness == 6.f);
	UWidgetSwitcher* Switcher = Extras->GetSwitcher();
	TestEqual(TEXT("five pages"), Switcher->GetChildrenCount(), 5);
	TestTrue(TEXT("the pages are the credits, the pictures, the diaries, the sounds and the movies"),
		Switcher->GetChildAt(0) == Extras->GetCredits()->GetParent() && Switcher->GetChildAt(1) == Extras->GetArtBox()
		&& Switcher->GetChildAt(2) == Extras->GetDiaryBox()->GetParent() && Switcher->GetChildAt(3) == Extras->GetSoundBox()->GetParent()
		&& Switcher->GetChildAt(4) == Extras->GetVideoBox());
	TestTrue(TEXT("each list's player is on its page"), Extras->GetDiaryBar()->GetParent() == Extras->GetDiaryBox()->GetParent()
		&& Extras->GetSoundBar()->GetParent() == Extras->GetSoundBox()->GetParent());
	TestTrue(TEXT("the sounds wrap at 1635, at 0.95"), Extras->GetSoundBox()->UseExplicitWrapSize()
		&& FMath::IsNearlyEqual(Extras->GetSoundBox()->GetWrapSize(), 1635.44f, 1.e-2f)
		&& FMath::IsNearlyEqual(Extras->GetSoundBox()->GetRenderTransform().Scale.X, 0.95f, 1.e-4f));

	// The pictures: 35, IDs in order; 19 and 20 unlocked (this game's pictures for now), 18 and 21 locked.
	const auto& Art = Extras->GetArtItems();
	TestEqual(TEXT("35 pictures"), Art.Num(), Screen::ArtCount);
	TestEqual(TEXT("all in the Art Gallery's box"), Extras->GetArtBox()->GetChildrenCount(), Screen::ArtCount);
	TestTrue(TEXT("their IDs are 0 to 34"), Art.Num() == Screen::ArtCount && Art[0]->ID == 0 && Art[34]->ID == 34);
	if (Art.Num() == Screen::ArtCount)
	{
		TestTrue(TEXT("19 is unlocked"), Art[19]->IsUnlocked());
		TestTrue(TEXT("and shows this game's title face"), BrushTexture(Art[19]->GetImage()) != nullptr
			&& BrushTexture(Art[19]->GetImage())->GetName() == TEXT("T_TitleFace"));
		TestTrue(TEXT("20 is unlocked"), Art[20]->IsUnlocked());
		TestFalse(TEXT("18 is locked"), Art[18]->IsUnlocked());
		TestFalse(TEXT("21 is locked"), Art[21]->IsUnlocked());
		TestTrue(TEXT("the parts read the screen's save"), Art[19]->Save == Extras->Save);
	}

	// The diaries: ten over their player, 0 to 8 unlocked (no ranks), 9 not; the sounds: the original's ten slots with
	// no track in any of them, so all locked (2026-09-23, the user's 「EXTRAS に曲は不要」).
	const auto& Diaries = Extras->GetDiaryButtons();
	const auto& Sounds = Extras->GetSoundButtons();
	const bool bLists = Diaries.Num() == Screen::DiaryCount && Sounds.Num() == Screen::SoundCount;
	TestTrue(TEXT("ten diaries and ten sounds"), bLists);
	if (bLists)
	{
		TestTrue(TEXT("the diaries are diaries on the diaries' player"), Diaries[3]->bDiary && Diaries[3]->SoundBar == Extras->GetDiaryBar());
		TestTrue(TEXT("diary 8 is unlocked"), Diaries[8]->IsUnlocked());
		TestFalse(TEXT("diary 9 is locked"), Diaries[9]->IsUnlocked());
		TestTrue(TEXT("the sounds are sounds on the sounds' player"), !Sounds[3]->bDiary && Sounds[3]->SoundBar == Extras->GetSoundBar());
		TestFalse(TEXT("no sound is unlocked"), Sounds[0]->IsUnlocked() || Sounds[9]->IsUnlocked());
		TestTrue(TEXT("none is given a name or a sound"), Sounds[0]->Text.IsEmpty() && Sounds[0]->Sound == nullptr
			&& Sounds[9]->Text.IsEmpty() && Sounds[9]->Sound == nullptr);
	}

	// The movies: ten, all locked.
	const auto& Videos = Extras->GetVideoItems();
	TestEqual(TEXT("ten movies"), Videos.Num(), Screen::VideoCount);
	TestTrue(TEXT("all locked"), Videos.Num() == Screen::VideoCount && !Videos[0]->IsUnlocked() && !Videos[9]->IsUnlocked());

	// The credits: RichText_Credits' five rows, and the provisional words.
	const UDataTable* Styles = Extras->GetCredits()->GetTextStyleSet();
	TestTrue(TEXT("the credits' styles are RichText_Credits' rows"), Styles && Styles->GetRowNames().Num() == 5);
	const FRichTextStyleRow* Reg = Styles ? Styles->FindRow<FRichTextStyleRow>(TEXT("REG"), TEXT("test")) : nullptr;
	TestTrue(TEXT("REG is Roboto Light 16"), Reg && Reg->TextStyle.Font.TypefaceFontName == TEXT("Light") && Reg->TextStyle.Font.Size == 16.f);
	const FRichTextStyleRow* RedBold = Styles ? Styles->FindRow<FRichTextStyleRow>(TEXT("RED_BOLD"), TEXT("test")) : nullptr;
	TestTrue(TEXT("RED_BOLD is Roboto Bold 18"), RedBold && RedBold->TextStyle.Font.TypefaceFontName == TEXT("Bold") && RedBold->TextStyle.Font.Size == 18.f);
	TestTrue(TEXT("the credits name this game"), Extras->GetCredits()->GetText().ToString().Contains(TEXT("WASAMI DECEPTION")));
	TestEqual(TEXT("the credits wait 1110 px down"), Extras->GetCredits()->GetRenderTransform().Translation.Y, 1110.);

	// FadeIn at twice its speed: 0.125 s in, a quarter second of it (halfway); then whole.
	Extras->Advance(0.125f);
	TestTrue(TEXT("FadeIn runs at 2"), FMath::IsNearlyEqual(Extras->GetFadeInTime(), 0.25f, 1.e-4f));
	TestTrue(TEXT("halfway in"), FMath::IsNearlyEqual(Extras->GetRoot()->GetRenderOpacity(), Screen::EvaluateOpacity(0.25f), 1.e-4f)
		&& FMath::IsNearlyEqual(Screen::EvaluateOpacity(0.25f), 0.5f, 1.e-2f));
	Extras->Advance(1.f);
	TestEqual(TEXT("then whole"), Extras->GetRoot()->GetRenderOpacity(), 1.f);

	// The hovers: a section not chosen lights and dims again, the one chosen stays white.
	Extras->HoverSection(Screen::CreditsSection, true);
	TestTrue(TEXT("CREDITS white while hovered"), NearlyColour(Extras->GetSectionButton(Screen::CreditsSection)->GetColorAndOpacity(), FLinearColor::White));
	Extras->HoverSection(Screen::CreditsSection, false);
	TestTrue(TEXT("and dark grey after"), NearlyColour(Extras->GetSectionButton(Screen::CreditsSection)->GetColorAndOpacity(), Grey));
	Extras->HoverSection(Screen::ArtGallerySection, false);
	TestTrue(TEXT("ART GALLERY stays white"), NearlyColour(Extras->GetSectionButton(Screen::ArtGallerySection)->GetColorAndOpacity(), FLinearColor::White));

	// BIERCE DIARIES: its page; a diary played there.
	Extras->Select(Screen::BierceDiariesSection);
	TestEqual(TEXT("the diaries' page"), Switcher->GetActiveWidgetIndex(), 2);
	TestTrue(TEXT("BIERCE DIARIES white, ART GALLERY dark grey"),
		NearlyColour(Extras->GetSectionButton(Screen::BierceDiariesSection)->GetColorAndOpacity(), FLinearColor::White)
		&& NearlyColour(Extras->GetSectionButton(Screen::ArtGallerySection)->GetColorAndOpacity(), Grey));
	if (bLists)
	{
		Diaries[0]->Press();
		TestTrue(TEXT("diary 0 plays"), Diaries[0]->IsPlaying());
		// Pressing the section chosen does nothing (no Check If Playing).
		Extras->Select(Screen::BierceDiariesSection);
		TestTrue(TEXT("BIERCE DIARIES again leaves it playing"), Diaries[0]->IsPlaying());
		// Another section stops it (Check If Playing), but leaves a paused one alone.
		Extras->Select(Screen::SoundArchiveSection);
		TestEqual(TEXT("the sounds' page"), Switcher->GetActiveWidgetIndex(), 3);
		TestFalse(TEXT("SOUND ARCHIVE stops the diary"), Diaries[0]->IsPlaying() || Diaries[0]->IsSelected());
		// A paused one it leaves alone (a diary here: no sound can be played, the archive listing none).
		Extras->Select(Screen::BierceDiariesSection);
		Diaries[1]->Press();
		Diaries[1]->Press();
		TestTrue(TEXT("diary 1 paused"), Diaries[1]->IsPaused() && !Diaries[1]->IsPlaying());
		Extras->Select(Screen::SoundArchiveSection);
		TestTrue(TEXT("a paused diary is left chosen"), Diaries[1]->IsSelected() && Diaries[1]->IsPaused());
	}

	// CREDITS: its page, and the credits scrolling from 1110 up to -4121.86 over 39.8 s (from the start each press).
	Extras->Select(Screen::CreditsSection);
	TestEqual(TEXT("the credits' page"), Switcher->GetActiveWidgetIndex(), 0);
	TestEqual(TEXT("Active Button 0"), Extras->GetActiveButton(), Screen::CreditsSection);
	Extras->Advance(10.f);
	const double Expected = 1110. + (-4121.8603515625 - 1110.) * 10. / (2388000. / 60000.);
	TestTrue(TEXT("10 s in, the credits have gone up in a straight line"),
		FMath::IsNearlyEqual(Extras->GetCredits()->GetRenderTransform().Translation.Y, Expected, 0.5));
	Extras->Select(Screen::CreditsSection);
	TestEqual(TEXT("CREDITS again starts them over"), Extras->GetCredits()->GetRenderTransform().Translation.Y, 1110.);
	Extras->Advance(100.f);
	TestTrue(TEXT("and they end at -4121.86"), FMath::IsNearlyEqual(Extras->GetCredits()->GetRenderTransform().Translation.Y, -4121.86, 1.e-2));

	// BACK: grey until hovered; FadeIn backwards at 2, and FadeMusic 0.25 s after the first press.
	Extras->HoverBack(true);
	TestTrue(TEXT("BACK white while hovered"), NearlyColour(Extras->GetBackButton()->GetColorAndOpacity(), FLinearColor::White));
	Extras->HoverBack(false);
	TestTrue(TEXT("dark grey after"), NearlyColour(Extras->GetBackButton()->GetColorAndOpacity(), Grey));
	int32 Faded = 0;
	Extras->OnFadeMusic.AddLambda([&Faded]() { ++Faded; });
	Extras->PressBack();
	TestTrue(TEXT("BACK closes"), Extras->IsClosing());
	TestTrue(TEXT("FadeIn from its end"), FMath::IsNearlyEqual(Extras->GetFadeInTime(), Screen::FadeInLength, 1.e-4f));
	Extras->Advance(0.1f);
	TestTrue(TEXT("backwards at 2"), FMath::IsNearlyEqual(Extras->GetFadeInTime(), Screen::FadeInLength - 0.2f, 1.e-4f));
	Extras->PressBack();
	Extras->Advance(0.1f);
	TestEqual(TEXT("no FadeMusic yet (the second press does not wait again)"), Faded, 0);
	Extras->Advance(0.06f);
	TestEqual(TEXT("FadeMusic once 0.25 s have gone"), Faded, 1);
	TestTrue(TEXT("and the screen is off"), Extras->IsFinished());
	Extras->Advance(1.f);
	TestEqual(TEXT("only once"), Faded, 1);

	// No save in the slot: a new one, with nothing unlocked.
	UWasamiExtrasWidget* Unsaved = NewObject<UWasamiExtrasWidget>();
	Unsaved->SaveSlotName = TEXT("WasamiTest_Extras_None");
	Unsaved->Initialize();
	Unsaved->TakeWidget();
	TestNotNull(TEXT("a new save is taken"), Unsaved->Save.Get());
	TestTrue(TEXT("and picture 19 is locked in it"), Unsaved->GetArtItems().Num() == Screen::ArtCount && !Unsaved->GetArtItems()[19]->IsUnlocked());
	return true;
}

#endif
