#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "../WasamiGameMode.h"
#include "../WasamiSaveGame.h"
#include "../WasamiTitleScreenWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString TitleTestSlotName(TEXT("WasamiTest_Title"));

	/** A screen whose tree is built and whose Construct ran, reading the test slot. */
	UWasamiTitleScreenWidget* MakeScreen()
	{
		UWasamiTitleScreenWidget* Screen = NewObject<UWasamiTitleScreenWidget>();
		Screen->SaveSlotName = TitleTestSlotName;
		Screen->Initialize();
		Screen->TakeWidget();
		return Screen;
	}

	/** Runs the screen at 60 frames a second until Seconds since Begin. */
	void RunUntil(UWasamiTitleScreenWidget* Screen, float Seconds)
	{
		while (Screen->GetElapsed() + 1.f / 120.f < Seconds)
		{
			Screen->Advance(1.f / 60.f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTitleCurvesTest, "Wasami.Title.Curves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTitleCurvesTest::RunTest(const FString& Parameters)
{
	using W = UWasamiTitleScreenWidget;
	// Slideshow lifts the black cover over 2.5 s.
	TestEqual(TEXT("the cover starts black"), W::EvaluateSlideshow(0.f), 1.f, 1e-4f);
	TestEqual(TEXT("halfway at 1.25 s"), W::EvaluateSlideshow(1.25f), 0.5f, 1e-3f);
	TestEqual(TEXT("gone at 2.5 s"), W::EvaluateSlideshow(2.5f), 0.f, 1e-4f);

	// FadeOut / FadeOut_0: the black by 3.5 s.
	TestEqual(TEXT("no black at first"), W::EvaluateBlack(0.f), 0.f, 1e-4f);
	TestTrue(TEXT("coming in at 3 s"), W::EvaluateBlack(3.f) > 0.5f && W::EvaluateBlack(3.f) < 1.f);
	TestEqual(TEXT("black at 3.5 s"), W::EvaluateBlack(3.5f), 1.f, 1e-4f);
	TestEqual(TEXT("and at 3.75 s"), W::EvaluateBlack(3.75f), 1.f, 1e-3f);
	// FadeOut: the pulse and the red flash from 0.15 s.
	TestEqual(TEXT("no pulse before 0.15 s"), W::EvaluatePulse(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("1.05 at 0.25 s"), W::EvaluatePulse(0.25f), 1.05f, 1e-4f);
	TestEqual(TEXT("back at 0.3 s"), W::EvaluatePulse(0.3f), 1.f, 1e-4f);
	TestEqual(TEXT("no red before 0.15 s"), W::EvaluateRed(0.15f), 0.f, 1e-4f);
	TestEqual(TEXT("the red 0.5 at 0.25 s"), W::EvaluateRed(0.25f), 0.5f, 1e-4f);
	TestEqual(TEXT("0.2 at 0.3 s"), W::EvaluateRed(0.3f), 0.2f, 1e-4f);
	TestEqual(TEXT("gone at 3.75 s"), W::EvaluateRed(3.75f), 0.f, 1e-4f);
	// Start_New_Game: 0.6 until 0.45 s, 0.3 from 1.65 s on.
	TestEqual(TEXT("the start sound at 0.6"), W::EvaluateStartVolume(0.f), 0.6f, 1e-4f);
	TestEqual(TEXT("still 0.6 at 0.45 s"), W::EvaluateStartVolume(0.45f), 0.6f, 1e-4f);
	TestEqual(TEXT("0.3 at 1.65 s"), W::EvaluateStartVolume(1.65f), 0.3f, 1e-4f);
	TestEqual(TEXT("and after"), W::EvaluateStartVolume(5.f), 0.3f, 1e-4f);

	// The lengths, the line's time and the music, as exported and written.
	TestEqual(TEXT("FadeOut runs as long as Start_New_Game"), W::FadeOutLength, 9.42633f, 1e-4f);
	TestEqual(TEXT("FadeOut_0 runs 3.75 s"), W::FadeOut0Length, 3.75f);
	TestEqual(TEXT("Bierce's line at 98999 ticks"), W::VoiceTime, 1.64998f, 1e-4f);
	// The latest version's theme (@10519): CreateSound2D(Theme_v1_3, 0.6, 1) then FadeIn(2, 0.5).
	TestEqual(TEXT("the music at volume 0.6"), W::MusicVolume, 0.6f);
	TestEqual(TEXT("at pitch 1"), W::MusicPitch, 1.f);
	TestEqual(TEXT("fading in over 2 s"), W::MusicFadeInSeconds, 2.f);
	TestEqual(TEXT("to 0.5"), W::MusicFadeInLevel, 0.5f);
	TestEqual(TEXT("added at Z 1"), W::ViewportZOrder, 1);

	// RESUME with a game begun; the version from the project settings.
	TestFalse(TEXT("no save, no progress"), W::HasProgress(nullptr));
	UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
	TestFalse(TEXT("an empty save, no progress"), W::HasProgress(Save));
	Save->Hospital.LevelCheckpoint = 4;
	TestTrue(TEXT("Zone 1's arrival is progress"), W::HasProgress(Save));
	const FString Version = W::VersionText().ToString();
	TestTrue(TEXT("the version is 'v' and the project's"), Version.StartsWith(TEXT("v")) && Version.Len() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTitleScreenTreeTest, "Wasami.Title.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTitleScreenTreeTest::RunTest(const FString& Parameters)
{
	// Without a save, RESUME goes.
	UGameplayStatics::DeleteGameInSlot(TitleTestSlotName, UWasamiSaveGame::UserIndex);
	UWasamiTitleScreenWidget* Screen = MakeScreen();
	UWidgetTree* Tree = Screen->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}

	// The original's slots in its order, the glow under the logo; none of the hidden or unmade ones.
	const UCanvasPanel* Root = Cast<UCanvasPanel>(Tree->RootWidget);
	if (!TestNotNull(TEXT("CanvasPanel_0 at the root"), Root))
	{
		return false;
	}
	int32 Last = INDEX_NONE;
	for (const TCHAR* Name : {TEXT("Button_0"), TEXT("Image_97"), TEXT("VideoMask"), TEXT("Image_104"), TEXT("LogoGlow"), TEXT("Image_103"),
		TEXT("TextBlock_79"), TEXT("VerticalBox_160"), TEXT("Image_0"), TEXT("Image_128"), TEXT("Image_2"), TEXT("TextBlock_0")})
	{
		UWidget* Widget = Tree->FindWidget(Name);
		const int32 Index = Widget ? Root->GetChildIndex(Widget) : INDEX_NONE;
		TestTrue(FString::Printf(TEXT("%s draws after the one before"), Name), Index != INDEX_NONE && Index > Last);
		Last = Index;
	}
	for (const TCHAR* Name : {TEXT("Chapters"), TEXT("Replay"), TEXT("Image_1"), TEXT("Slideshow_img"), TEXT("CanvasPanel_1")})
	{
		TestNull(FString::Printf(TEXT("no %s"), Name), Tree->FindWidget(Name));
	}
	const TCHAR* const Resources[][2] = {{TEXT("Image_97"), TEXT("T_TitleFace")}, {TEXT("VideoMask"), TEXT("title_screen_video_mask")},
		{TEXT("Image_104"), TEXT("MM_TitleScreen_Mask_Grey")}, {TEXT("LogoGlow"), TEXT("T_TitleLogoGlow")}, {TEXT("Image_103"), TEXT("T_TitleLogo")}};
	for (const auto& Each : Resources)
	{
		const UImage* Image = Cast<UImage>(Tree->FindWidget(Each[0]));
		const UObject* Resource = Image ? Image->GetBrush().GetResourceObject() : nullptr;
		TestTrue(FString::Printf(TEXT("%s shows %s"), Each[0], Each[1]), Resource && Resource->GetName() == Each[1]);
	}
	const UTextBlock* Version = Cast<UTextBlock>(Tree->FindWidget(TEXT("TextBlock_0")));
	TestTrue(TEXT("the version top right"), Version && Version->GetText().EqualTo(UWasamiTitleScreenWidget::VersionText()));

	// The menu: NEW GAME, EXTRAS, OPTIONS, QUIT without progress (the original's order without Chapters and Replay).
	const UVerticalBox* Menu = Cast<UVerticalBox>(Tree->FindWidget(TEXT("VerticalBox_160")));
	TestFalse(TEXT("no RESUME without a save"), Screen->HasResume());
	TestTrue(TEXT("RESUME is off the menu"), Menu && Menu->GetChildrenCount() == 4);
	if (Menu && Menu->GetChildrenCount() == 4)
	{
		int32 Index = 0;
		for (const TCHAR* Name : {TEXT("NewGame"), TEXT("Extras"), TEXT("Options"), TEXT("Quit")})
		{
			TestEqual(FString::Printf(TEXT("%s in its place"), Name), Menu->GetChildAt(Index++)->GetFName(), FName(Name));
		}
	}

	// Setup Buttons' style: the marker when hovered or pressed, invisible otherwise, UE 4's paddings.
	UButton* NewGame = Cast<UButton>(Tree->FindWidget(TEXT("NewGame")));
	if (TestNotNull(TEXT("NEW GAME"), NewGame))
	{
		const FButtonStyle& Style = NewGame->GetStyle();
		TestEqual(TEXT("unmarked until hovered"), Style.Normal.TintColor.GetSpecifiedColor().A, 0.f);
		TestTrue(TEXT("the marker when hovered"), Style.Hovered.GetResourceObject() && Style.Hovered.GetResourceObject()->GetName() == TEXT("title_screen_selection_marker"));
		TestTrue(TEXT("and pressed"), Style.Pressed.GetResourceObject() == Style.Hovered.GetResourceObject());
		TestEqual(TEXT("the normal padding 2"), Style.NormalPadding.Left, 2.f);
		TestEqual(TEXT("the pressed padding 3 at the top"), Style.PressedPadding.Top, 3.f);

		// The hover: white, and back to Unhovered Color.
		const UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(TEXT("NewGame_Text")));
		NewGame->OnHovered.Broadcast();
		TestTrue(TEXT("white when hovered"), Text && Text->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::White));
		NewGame->OnUnhovered.Broadcast();
		TestTrue(TEXT("grey again"), Text && FMath::IsNearlyEqual(Text->GetColorAndOpacity().GetSpecifiedColor().R, 0.107023f, 1e-5f));
	}

	// EXTRAS: the same style, text and hover.
	UButton* ExtrasButton = Cast<UButton>(Tree->FindWidget(TEXT("Extras")));
	const UTextBlock* ExtrasText = Cast<UTextBlock>(Tree->FindWidget(TEXT("Extras_Text")));
	if (TestNotNull(TEXT("EXTRAS"), ExtrasButton) && TestNotNull(TEXT("its text"), ExtrasText))
	{
		TestTrue(TEXT("EXTRAS' text"), ExtrasText->GetText().ToString() == TEXT("EXTRAS"));
		TestTrue(TEXT("the marker when hovered"), NewGame && ExtrasButton->GetStyle().Hovered.GetResourceObject() == NewGame->GetStyle().Hovered.GetResourceObject());
		TestEqual(TEXT("unmarked until hovered"), ExtrasButton->GetStyle().Normal.TintColor.GetSpecifiedColor().A, 0.f);
		ExtrasButton->OnHovered.Broadcast();
		TestTrue(TEXT("EXTRAS white when hovered"), ExtrasText->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::White));
		ExtrasButton->OnUnhovered.Broadcast();
		TestTrue(TEXT("and grey again"), FMath::IsNearlyEqual(ExtrasText->GetColorAndOpacity().GetSpecifiedColor().R, 0.107023f, 1e-5f));
	}

	// With a game begun, RESUME stays on top.
	UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
	Save->Hospital.LevelCheckpoint = 8;
	UGameplayStatics::SaveGameToSlot(Save, TitleTestSlotName, UWasamiSaveGame::UserIndex);
	UWasamiTitleScreenWidget* Resumable = MakeScreen();
	const UVerticalBox* ResumableMenu = Cast<UVerticalBox>(Resumable->WidgetTree->FindWidget(TEXT("VerticalBox_160")));
	TestTrue(TEXT("RESUME with progress"), Resumable->HasResume());
	TestTrue(TEXT("first of five"), ResumableMenu && ResumableMenu->GetChildrenCount() == 5 && ResumableMenu->GetChildAt(0)->GetFName() == TEXT("Resume"));
	UGameplayStatics::DeleteGameInSlot(TitleTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTitleAnimationsTest, "Wasami.Title.Animations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTitleAnimationsTest::RunTest(const FString& Parameters)
{
	UGameplayStatics::DeleteGameInSlot(TitleTestSlotName, UWasamiSaveGame::UserIndex);
	UWasamiTitleScreenWidget* Screen = MakeScreen();
	const UWidget* CoverWidget = Screen->WidgetTree->FindWidget(TEXT("Image_128"));

	// Construct plays Slideshow: black at once, clear by 2.5 s.
	TestEqual(TEXT("the cover black at Construct"), Screen->GetCoverOpacity(), 1.f, 1e-4f);
	TestTrue(TEXT("on Image_128"), CoverWidget && FMath::IsNearlyEqual(CoverWidget->GetRenderOpacity(), 1.f, 1e-4f));
	RunUntil(Screen, 1.25f);
	TestEqual(TEXT("half at 1.25 s"), Screen->GetCoverOpacity(), 0.5f, 0.02f);
	RunUntil(Screen, 3.f);
	TestEqual(TEXT("clear after 2.5 s"), Screen->GetCoverOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("no black yet"), Screen->GetBlackOpacity(), 0.f, 1e-4f);

	// NEW GAME's FadeOut from 3 s.
	Screen->PlayFadeOut();
	TestEqual(TEXT("the start sound at 0.6"), Screen->GetStartVolume(), 0.6f, 1e-4f);
	RunUntil(Screen, 3.25f);
	TestEqual(TEXT("the pulse at 0.25 s"), Screen->GetPulseScale(), 1.05f, 2e-3f);
	TestEqual(TEXT("the red at 0.25 s"), Screen->GetRedOpacity(), 0.5f, 0.01f);
	RunUntil(Screen, 4.6f);
	TestFalse(TEXT("no line before 1.65 s"), Screen->HasPlayedVoice());
	RunUntil(Screen, 4.7f);
	TestTrue(TEXT("Bierce's line at 1.65 s"), Screen->HasPlayedVoice());
	TestEqual(TEXT("the start sound down to 0.3"), Screen->GetStartVolume(), 0.3f, 1e-3f);
	RunUntil(Screen, 6.8f);
	TestEqual(TEXT("black by 3.75 s"), Screen->GetBlackOpacity(), 1.f, 1e-3f);
	RunUntil(Screen, 14.f);
	TestEqual(TEXT("and it stays when FadeOut ends"), Screen->GetBlackOpacity(), 1.f, 1e-3f);
	TestEqual(TEXT("the red gone"), Screen->GetRedOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("the pulse over"), Screen->GetPulseScale(), 1.f, 1e-4f);
	const UWidget* BlackWidget = Screen->WidgetTree->FindWidget(TEXT("Image_0"));
	TestTrue(TEXT("on Image_0"), BlackWidget && FMath::IsNearlyEqual(BlackWidget->GetRenderOpacity(), 1.f, 1e-3f));

	// RESUME's FadeOut_0: the black only.
	UWasamiTitleScreenWidget* Resuming = MakeScreen();
	RunUntil(Resuming, 3.f);
	Resuming->PlayFadeOut0();
	RunUntil(Resuming, 3.25f);
	TestEqual(TEXT("no pulse"), Resuming->GetPulseScale(), 1.f, 1e-4f);
	TestEqual(TEXT("no red"), Resuming->GetRedOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("no start sound"), Resuming->GetStartVolume(), 0.f, 1e-4f);
	RunUntil(Resuming, 8.f);
	TestEqual(TEXT("black by 3.75 s"), Resuming->GetBlackOpacity(), 1.f, 1e-3f);
	TestFalse(TEXT("no line"), Resuming->HasPlayedVoice());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTitleWaysOutTest, "Wasami.Title.WaysOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTitleWaysOutTest::RunTest(const FString& Parameters)
{
	using W = UWasamiTitleScreenWidget;
	const FString Zone1(AWasamiGameMode::Zone1LevelName);
	const FString Zone2(AWasamiGameMode::Zone2LevelName);
	auto WriteCheckpoint = [](int32 Checkpoint)
	{
		UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
		Save->Hospital.LevelCheckpoint = Checkpoint;
		Save->Hospital.Deaths = 3;
		Save->bLastCheckpointWarning = true;
		UGameplayStatics::SaveGameToSlot(Save, TitleTestSlotName, UWasamiSaveGame::UserIndex);
	};
	auto ReadSave = []()
	{
		return Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(TitleTestSlotName, UWasamiSaveGame::UserIndex));
	};

	// The entrance's Spawn: Zone 2 for 7 to 10, Zone 1 for the rest.
	TestEqual(TEXT("0 goes to Zone 1"), FString(AWasamiGameMode::LevelForCheckpoint(0)), Zone1);
	for (int32 Checkpoint = 4; Checkpoint <= 6; ++Checkpoint)
	{
		TestEqual(FString::Printf(TEXT("%d goes on in Zone 1"), Checkpoint), FString(AWasamiGameMode::LevelForCheckpoint(Checkpoint)), Zone1);
	}
	for (int32 Checkpoint = 7; Checkpoint <= 10; ++Checkpoint)
	{
		TestEqual(FString::Printf(TEXT("%d goes on in Zone 2"), Checkpoint), FString(AWasamiGameMode::LevelForCheckpoint(Checkpoint)), Zone2);
	}
	TestEqual(TEXT("the title's level"), FString(AWasamiGameMode::TitleLevelName), FString(TEXT("L_Title")));

	// Erase Save Files: a new save in the slot.
	WriteCheckpoint(9);
	UWasamiSaveGame::Erase(TitleTestSlotName);
	const UWasamiSaveGame* Erased = ReadSave();
	TestTrue(TEXT("erased: no checkpoint, deaths or warning"), Erased && Erased->Hospital.LevelCheckpoint == 0 && Erased->Hospital.Deaths == 0
		&& !Erased->bLastCheckpointWarning);

	// NEW GAME without a game begun: no question; the save erased, FadeOut, and Zone 1 10 s later.
	UGameplayStatics::DeleteGameInSlot(TitleTestSlotName, UWasamiSaveGame::UserIndex);
	UWasamiTitleScreenWidget* Fresh = MakeScreen();
	RunUntil(Fresh, 3.f);
	Fresh->PressNewGame();
	TestEqual(TEXT("NEW GAME goes to Zone 1"), Fresh->GetLevelToOpen(), Zone1);
	TestEqual(TEXT("with FadeOut's start sound"), Fresh->GetStartVolume(), 0.6f, 1e-4f);
	TestTrue(TEXT("and an empty save written"), UGameplayStatics::DoesSaveGameExist(TitleTestSlotName, UWasamiSaveGame::UserIndex));
	RunUntil(Fresh, 12.95f);
	TestFalse(TEXT("not before 10 s"), Fresh->HasLeft());
	RunUntil(Fresh, 13.05f);
	TestTrue(TEXT("Zone 1 at 10 s"), Fresh->HasLeft());
	TestEqual(TEXT("black by then"), Fresh->GetBlackOpacity(), 1.f, 1e-3f);

	// NEW GAME with a game begun asks first (nothing without a player to ask), and its YES goes on as above.
	WriteCheckpoint(8);
	UWasamiTitleScreenWidget* Asked = MakeScreen();
	RunUntil(Asked, 3.f);
	Asked->PressNewGame();
	TestTrue(TEXT("nothing before the answer"), Asked->GetLevelToOpen().IsEmpty());
	const UWasamiSaveGame* Kept = ReadSave();
	TestTrue(TEXT("the save kept"), Kept && Kept->Hospital.LevelCheckpoint == 8);
	Asked->NewGameEvent();
	TestEqual(TEXT("YES goes to Zone 1"), Asked->GetLevelToOpen(), Zone1);
	const UWasamiSaveGame* Restarted = ReadSave();
	TestTrue(TEXT("and erases the save"), Restarted && Restarted->Hospital.LevelCheckpoint == 0 && !Restarted->bLastCheckpointWarning);
	RunUntil(Asked, 13.05f);
	TestTrue(TEXT("Zone 1 10 s after YES"), Asked->HasLeft());

	// RESUME: FadeOut_0, and the zone of the checkpoint 5 s later; nothing else after it.
	WriteCheckpoint(8);
	UWasamiTitleScreenWidget* Resume = MakeScreen();
	TestTrue(TEXT("RESUME is there"), Resume->HasResume());
	RunUntil(Resume, 3.f);
	Resume->PressResume();
	TestEqual(TEXT("checkpoint 8 goes on in Zone 2"), Resume->GetLevelToOpen(), Zone2);
	Resume->PressNewGame();
	TestEqual(TEXT("NEW GAME after it changes nothing"), Resume->GetLevelToOpen(), Zone2);
	const UWasamiSaveGame* Untouched = ReadSave();
	TestTrue(TEXT("nor erases the save"), Untouched && Untouched->Hospital.LevelCheckpoint == 8);
	RunUntil(Resume, 3.25f);
	TestEqual(TEXT("no red"), Resume->GetRedOpacity(), 0.f, 1e-4f);
	RunUntil(Resume, 7.95f);
	TestFalse(TEXT("not before 5 s"), Resume->HasLeft());
	RunUntil(Resume, 8.05f);
	TestTrue(TEXT("Zone 2 at 5 s"), Resume->HasLeft());
	TestEqual(TEXT("black by then"), Resume->GetBlackOpacity(), 1.f, 1e-3f);

	WriteCheckpoint(5);
	UWasamiTitleScreenWidget* ResumeZone1 = MakeScreen();
	ResumeZone1->PressResume();
	TestEqual(TEXT("checkpoint 5 goes on in Zone 1"), ResumeZone1->GetLevelToOpen(), Zone1);

	// QUIT asks (nothing without a player); its YES quits. OPTIONS and EXTRAS open nothing without a player either,
	// and EXTRAS is no way out.
	UWasamiTitleScreenWidget* Quit = MakeScreen();
	Quit->PressQuit();
	Quit->PressOptions();
	Quit->PressExtras();
	TestNull(TEXT("no extras screen without a player"), Quit->GetExtras());
	TestFalse(TEXT("QUIT waits for its answer"), Quit->HasQuit());
	TestTrue(TEXT("and nothing opens"), Quit->GetLevelToOpen().IsEmpty());
	TestEqual(TEXT("EXTRAS takes the music out over 1 s"), W::ExtrasMusicFadeOut, 1.f);
	Quit->QuitEvent();
	TestTrue(TEXT("YES quits"), Quit->HasQuit());
	TestEqual(TEXT("the question"), FString(W::NewGameQuestion), FString(TEXT("STARTING A NEW GAME WILL RESET ALL PROGRESS.")));
	TestEqual(TEXT("the pop-ups at Z 2"), W::PopUpZOrder, 2);
	UGameplayStatics::DeleteGameInSlot(TitleTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
