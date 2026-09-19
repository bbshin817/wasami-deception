#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "../WasamiAssets.h"
#include "../WasamiCollectablesWidget.h"
#include "../WasamiMysteryNoteWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	template <typename W>
	void RunSecretsWidget(W* Widget, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds - 1e-4f; Time += 1.f / 60.f)
		{
			Widget->Advance(1.f / 60.f);
		}
	}

	FString SecretsResourceName(const UImage* Image)
	{
		const UObject* Resource = Image ? Image->GetBrush().GetResourceObject() : nullptr;
		return Resource ? Resource->GetName() : FString(TEXT("<none>"));
	}

	/** The names of Panel's children, in order, joined by commas. */
	FString SecretsChildren(const UPanelWidget* Panel)
	{
		TArray<FString> Names;
		for (int32 Index = 0; Panel && Index < Panel->GetChildrenCount(); ++Index)
		{
			Names.Add(Panel->GetChildAt(Index)->GetName());
		}
		return FString::Join(Names, TEXT(", "));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsCollectablesWidgetTest, "Wasami.Secrets.Widgets.Collectables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsCollectablesWidgetTest::RunTest(const FString& Parameters)
{
	using W = UWasamiCollectablesWidget;

	// UMG_Collectables' NewAnimation_1, Construct's Delay and BP_Collectable's AddToPlayerScreen.
	TestEqual(TEXT("NewAnimation_1 runs 132001 ticks"), W::AnimLength, 2.2000167f, 1e-5f);
	TestEqual(TEXT("off after 2 s"), W::RemoveDelay, 2.f);
	TestEqual(TEXT("on the player's screen at Z 0"), W::ZOrder, 0);
	TestEqual(TEXT("the words"), W::UnlockedText().ToString(), FString(TEXT("NEW EXTRAS UNLOCKED!")));

	// The banner pops in from nothing past 1.1 and settles at its size; it is in by 0.25 s and out from 1.75 s to 2 s.
	TestEqual(TEXT("nothing at 0"), W::EvaluateScale(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("1.1 at 0.25 s"), W::EvaluateScale(0.25f), 1.1f, 1e-4f);
	TestEqual(TEXT("its size at 0.4 s"), W::EvaluateScale(0.4f), 1.f, 1e-4f);
	TestEqual(TEXT("and at 1.5 s"), W::EvaluateScale(1.5f), 1.f, 1e-4f);
	TestEqual(TEXT("unseen at 0"), W::EvaluateOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("seen at 0.25 s"), W::EvaluateOpacity(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("still at 1.75 s"), W::EvaluateOpacity(1.75f), 1.f, 1e-4f);
	TestEqual(TEXT("gone at 2 s"), W::EvaluateOpacity(2.f), 0.f, 1e-4f);
	// The red flash: 0.2 at 0.25 s between 0.15 s and 0.6 s.
	TestEqual(TEXT("no flash at 0"), W::EvaluateFlashOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("none at 0.15 s"), W::EvaluateFlashOpacity(0.15f), 0.f, 1e-4f);
	TestEqual(TEXT("0.2 at 0.25 s"), W::EvaluateFlashOpacity(0.25f), 0.2f, 1e-4f);
	TestEqual(TEXT("gone at 0.6 s"), W::EvaluateFlashOpacity(0.6f), 0.f, 1e-4f);
	TestEqual(TEXT("and after"), W::EvaluateFlashOpacity(1.5f), 0.f, 1e-4f);

	// The tree, built as the widget is taken (its Construct runs then).
	W* Banner = NewObject<W>();
	Banner->Initialize();
	Banner->TakeWidget();
	UCanvasPanel* Canvas = Banner->GetCanvas();
	if (!TestNotNull(TEXT("CanvasPanel_0"), Canvas) || !TestNotNull(TEXT("TextBlock_150"), Banner->GetTextBlock()))
	{
		return false;
	}
	TestEqual(TEXT("the flash, the frame, the picture and the words, in the original's order"), SecretsChildren(Canvas),
		FString(TEXT("Image_297, Image_89, Image_249, TextBlock_150")));
	TestEqual(TEXT("the frame is extras_unlock_bg"), SecretsResourceName(Banner->GetFrame()), FString(TEXT("extras_unlock_bg")));
	TestEqual(TEXT("the flash is WhiteSquareTexture"), SecretsResourceName(Banner->GetFlash()), FString(TEXT("WhiteSquareTexture")));
	TestEqual(TEXT("tinted red"), Banner->GetFlash()->GetBrush().TintColor.GetSpecifiedColor(), FLinearColor(1.f, 0.f, 0.f, 1.f));
	TestEqual(TEXT("NEW EXTRAS UNLOCKED!"), Banner->GetTextBlock()->GetText().ToString(), FString(TEXT("NEW EXTRAS UNLOCKED!")));
	TestEqual(TEXT("the words at 20"), static_cast<float>(Banner->GetTextBlock()->GetFont().Size), 20.f);
	const int32 Drawn = Banner->GetIconIndex();
	TestTrue(TEXT("a picture drawn from the four"), Drawn >= 0 && Drawn <= 3);
	const TCHAR* Pictures[] = {TEXT("art_icon"), TEXT("diary_icon"), TEXT("sound_icon"), TEXT("movie_icon")};
	if (Drawn >= 0 && Drawn <= 3)
	{
		TestEqual(TEXT("the picture drawn"), SecretsResourceName(Banner->GetIcon()), FString(Pictures[Drawn]));
	}
	TestEqual(TEXT("the picture kept at 159 × 145"), FVector2D(Banner->GetIcon()->GetBrush().ImageSize), FVector2D(159.f, 145.f));
	TestEqual(TEXT("unseen at the start"), Canvas->GetRenderOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("from nothing"), FVector2D(Canvas->GetRenderTransform().Scale), FVector2D::ZeroVector);

	// Every picture comes up now and then.
	TSet<int32> Seen;
	for (int32 Draw = 0; Draw < 200 && Seen.Num() < 4; ++Draw)
	{
		Banner->Begin();
		Seen.Add(Banner->GetIconIndex());
	}
	TestEqual(TEXT("all four pictures drawn in 200 goes"), Seen.Num(), 4);

	Banner->Begin();
	RunSecretsWidget(Banner, 0.25f);
	TestEqual(TEXT("seen at 0.25 s"), Canvas->GetRenderOpacity(), 1.f, 1e-2f);
	TestEqual(TEXT("the flash at 0.25 s"), Banner->GetFlash()->GetRenderOpacity(), 0.2f, 1e-2f);
	RunSecretsWidget(Banner, 1.7f);
	TestFalse(TEXT("up until 2 s"), Banner->IsFinished());
	RunSecretsWidget(Banner, 0.1f);
	TestTrue(TEXT("off at 2 s"), Banner->IsFinished());

	// No player, no screen (as in a test's world).
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	TestNull(TEXT("nothing without a player"), W::Show(Wrapper.GetTestWorld()));
	TestNull(TEXT("nor the secret's"), UWasamiCollectablesSecretWidget::Show(Wrapper.GetTestWorld()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsRoomWidgetTest, "Wasami.Secrets.Widgets.Secret",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsRoomWidgetTest::RunTest(const FString& Parameters)
{
	using W = UWasamiCollectablesSecretWidget;

	// UMG_Collectables_Secret: BP_SecretRoomZone's CreateAndAddWidget at Z 1, the music at 0.5, the words.
	TestEqual(TEXT("added at Z 1"), W::SecretZOrder, 1);
	TestEqual(TEXT("the music at 0.5"), W::MusicVolume, 0.5f);
	TestEqual(TEXT("the words"), W::FoundText().ToString(), FString(TEXT("YOU FOUND A MYSTERIOUS ROOM")));
	TestEqual(TEXT("the music"), GetDefault<W>()->MusicSound.ToSoftObjectPath().GetAssetName(),
		FString(TEXT("DD_LVL2_15_V1_Secret_Mystery_Room_120818")));

	W* Banner = NewObject<W>();
	Banner->Initialize();
	Banner->TakeWidget();
	UTextBlock* Text = Banner->GetTextBlock();
	if (!TestNotNull(TEXT("TextBlock_150"), Text) || !TestNotNull(TEXT("Image_249"), Banner->GetIcon()))
	{
		return false;
	}
	TestEqual(TEXT("the same tree"), SecretsChildren(Banner->GetCanvas()),
		FString(TEXT("Image_297, Image_89, Image_249, TextBlock_150")));
	TestEqual(TEXT("the room's picture"), SecretsResourceName(Banner->GetIcon()), FString(TEXT("T_MysteryRoom")));
	TestEqual(TEXT("no picture drawn"), Banner->GetIconIndex(), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("YOU FOUND A MYSTERIOUS ROOM"), Text->GetText().ToString(), FString(TEXT("YOU FOUND A MYSTERIOUS ROOM")));
	TestEqual(TEXT("the words at 18"), static_cast<float>(Text->GetFont().Size), 18.f);
	if (const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Text->Slot))
	{
		TestEqual(TEXT("129 px left of the middle"), Slot->GetOffsets().Left, -128.9609375f, 1e-4f);
		TestEqual(TEXT("4 px up"), Slot->GetOffsets().Top, -4.024012f, 1e-4f);
	}
	else
	{
		AddError(TEXT("TextBlock_150 is not on the canvas"));
	}
	RunSecretsWidget(Banner, 2.05f);
	TestTrue(TEXT("off at 2 s"), Banner->IsFinished());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsMysteryNoteWidgetTest, "Wasami.Secrets.Widgets.MysteryNote",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsMysteryNoteWidgetTest::RunTest(const FString& Parameters)
{
	using W = UWasamiMysteryNoteWidget;

	// UMG_MysteryNote's animations, CLOSE's reverse play and Delay, and BP_MysteryCollectable's AddToViewport.
	TestEqual(TEXT("Open runs 45001 ticks"), W::OpenLength, 0.7500167f, 1e-5f);
	TestEqual(TEXT("SwitchPage runs 15001 ticks"), W::SwitchPageLength, 0.2500167f, 1e-5f);
	TestEqual(TEXT("CLOSE plays Open back from 0.35 s before its end"), W::CloseFrom, 0.35f);
	TestEqual(TEXT("and takes the screen off 0.5 s on"), W::CloseDelay, 0.5f);
	TestEqual(TEXT("added at Z 2"), W::ViewportZOrder, 2);
	TestTrue(TEXT("E Note by default"), GetDefault<W>()->bENote);

	// The paper swings up from below past its place and settles at 0.35 s; the rest fades in over the same time, the
	// page from 0.15 s, and the blur goes to 3 and stays.
	TestEqual(TEXT("the paper 1052 below at 0"), W::EvaluatePaperY(0.f), 1052.0336f, 1e-3f);
	TestEqual(TEXT("50 up at 0.15 s"), W::EvaluatePaperY(0.15f), -50.f, 1e-3f);
	TestEqual(TEXT("in place at 0.35 s"), W::EvaluatePaperY(0.35f), 0.f, 1e-3f);
	TestEqual(TEXT("and on"), W::EvaluatePaperY(0.75f), 0.f, 1e-3f);
	TestEqual(TEXT("tilted 1.75° at 0"), W::EvaluatePaperAngle(0.f), 1.7458093f, 1e-4f);
	TestEqual(TEXT("−0.75° at 0.15 s"), W::EvaluatePaperAngle(0.15f), -0.7455843f, 1e-4f);
	TestEqual(TEXT("straight at 0.35 s"), W::EvaluatePaperAngle(0.35f), 0.f, 1e-4f);
	TestEqual(TEXT("unseen at 0"), W::EvaluateFade(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("half at 0.175 s"), W::EvaluateFade(0.175f), 0.5f, 1e-4f);
	TestEqual(TEXT("seen at 0.35 s"), W::EvaluateFade(0.35f), 1.f, 1e-4f);
	TestEqual(TEXT("the page unseen at 0.15 s"), W::EvaluatePageOpacity(0.15f), 0.f, 1e-4f);
	TestEqual(TEXT("seen at 0.35 s"), W::EvaluatePageOpacity(0.35f), 1.f, 1e-4f);
	TestEqual(TEXT("no blur at 0"), W::EvaluateBlur(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("3 at 0.35 s"), W::EvaluateBlur(0.35f), 3.f, 1e-4f);
	TestEqual(TEXT("kept at 3"), W::EvaluateBlur(0.75f), 3.f, 1e-4f);
	TestEqual(TEXT("a new page unseen at 0"), W::EvaluateSwitchPage(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("seen at 0.25 s"), W::EvaluateSwitchPage(0.25f), 1.f, 1e-4f);

	// A note of two pages, with its picture: the tree, built as the widget is taken (its Construct runs then, without a
	// world: nothing is paused and no sound made).
	UTexture2D* Picture = TSoftObjectPtr<UTexture2D>(
		WasamiAssets::Path(TEXT("/Game/DD/Textures/06_Hospital/mysteryroom_hospital_note_01"))).LoadSynchronous();
	if (!Picture)
	{
		AddError(TEXT("mysteryroom_hospital_note_01 is missing: run Tools/dd/prepare_stage.py and import_dd_stage_assets"));
	}
	W* Note = NewObject<W>();
	Note->Texture = Picture;
	Note->Texts = {FText::FromString(TEXT("The first page.")), FText::FromString(TEXT("The second page."))};
	Note->Initialize();
	// Held for the test: the rich text lets its styles go with its Slate widget.
	const TSharedRef<SWidget> NoteSlate = Note->TakeWidget();
	UWidgetTree* Tree = Note->WidgetTree;
	const UCanvasPanel* Root = Tree ? Cast<UCanvasPanel>(Tree->RootWidget) : nullptr;
	if (!TestNotNull(TEXT("CanvasPanel_0 at the root"), Root) || !TestNotNull(TEXT("RichTextBlock_0"), Note->GetTextBlock()))
	{
		return false;
	}
	TestEqual(TEXT("the paper, the blur, the black, the words, CLOSE, the page and the arrow, in the original's order"),
		SecretsChildren(Root),
		FString(TEXT("SizeBox_1, BackgroundBlur_0, Image_0, SizeBox_0, Close, TextBlock_256, NextPage")));
	for (const TCHAR* Name : {TEXT("ScaleBox_91"), TEXT("paper"), TEXT("ScaleBox_0"), TEXT("RichTextBlock_0"), TEXT("TextBlock_0")})
	{
		TestNotNull(FString::Printf(TEXT("%s is in the tree"), Name), Tree->FindWidget(Name));
	}
	if (Picture)
	{
		TestEqual(TEXT("the paper is the note's picture"), SecretsResourceName(Note->GetPaper()), Picture->GetName());
		TestEqual(TEXT("at its own size"), FVector2D(Note->GetPaper()->GetBrush().ImageSize),
			FVector2D(static_cast<double>(Picture->GetSizeX()), static_cast<double>(Picture->GetSizeY())));
	}
	TestEqual(TEXT("the first page"), Note->GetTextBlock()->GetText().ToString(), FString(TEXT("The first page.")));
	TestEqual(TEXT("1/2"), Note->GetPageBlock()->GetText().ToString(), FString(TEXT("1/2")));
	TestNotNull(TEXT("the arrow stays for two pages"), Note->GetNextPageButton()->GetParent());
	const UDataTable* Styles = Note->GetTextBlock()->GetTextStyleSet();
	if (TestNotNull(TEXT("MysteryText's styles"), Styles))
	{
		TestNotNull(TEXT("Default"), Styles->FindRow<FRichTextStyleRow>(TEXT("Default"), TEXT("test"), false));
		TestNotNull(TEXT("Player"), Styles->FindRow<FRichTextStyleRow>(TEXT("Player"), TEXT("test"), false));
	}
	const FTextBlockStyle& Style = Note->GetTextBlock()->GetDefaultTextStyle();
	TestEqual(TEXT("the words at 18"), static_cast<float>(Style.Font.Size), 18.f);
	TestEqual(TEXT("with a 2 px outline"), Style.Font.OutlineSettings.OutlineSize, 2);
	TestEqual(TEXT("off-white"), Style.ColorAndOpacity.GetSpecifiedColor(), FLinearColor(0.964686f, 0.964686f, 0.964686f, 1.f));
	TestEqual(TEXT("helvetica-normal"), Style.Font.FontObject ? Style.Font.FontObject->GetName() : FString(),
		FString(TEXT("helvetica-normal_Font")));
	TestEqual(TEXT("the words shrink only"), Note->GetTextBox()->GetStretchDirection(), EStretchDirection::DownOnly);
	TestEqual(TEXT("the arrow is selection_bar_arrow_hover"),
		Note->GetNextPageButton()->GetStyle().Normal.GetResourceObject()
			? Note->GetNextPageButton()->GetStyle().Normal.GetResourceObject()->GetName() : FString(),
		FString(TEXT("selection_bar_arrow_hover")));
	TestEqual(TEXT("CLOSE's button clear"), Note->GetCloseButton()->GetBackgroundColor().A, 0.f);
	TestNull(TEXT("no sound without a world"), Note->GetSound());

	// Open from its start.
	TestEqual(TEXT("the paper below at the start"), static_cast<float>(Note->GetPaperBox()->GetRenderTransform().Translation.Y), 1052.0336f, 1e-3f);
	TestEqual(TEXT("the black unseen"), Note->GetDim()->GetRenderOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("no blur"), Note->GetBlur()->GetBlurStrength(), 0.f, 1e-4f);
	RunSecretsWidget(Note, 0.75f);
	TestEqual(TEXT("the paper in place"), static_cast<float>(Note->GetPaperBox()->GetRenderTransform().Translation.Y), 0.f, 1e-3f);
	TestEqual(TEXT("the black seen"), Note->GetDim()->GetRenderOpacity(), 1.f, 1e-4f);
	TestEqual(TEXT("the page seen"), Note->GetPageBlock()->GetRenderOpacity(), 1.f, 1e-4f);
	TestEqual(TEXT("blurred at 3"), Note->GetBlur()->GetBlurStrength(), 3.f, 1e-4f);
	TestEqual(TEXT("the words at their own opacity"), Note->GetTextBlock()->GetRenderOpacity(), 1.f, 1e-4f);

	// The arrow: SwitchPage and the next page, then round to the first.
	Note->PressNextPage();
	TestEqual(TEXT("page 2"), Note->GetCurrentText(), 1);
	TestEqual(TEXT("the second page"), Note->GetTextBlock()->GetText().ToString(), FString(TEXT("The second page.")));
	TestEqual(TEXT("2/2"), Note->GetPageBlock()->GetText().ToString(), FString(TEXT("2/2")));
	TestEqual(TEXT("the words fading in"), Note->GetTextBlock()->GetRenderOpacity(), 0.f, 1e-4f);
	RunSecretsWidget(Note, 0.25f);
	TestEqual(TEXT("the words in"), Note->GetTextBlock()->GetRenderOpacity(), 1.f, 1e-3f);
	Note->PressNextPage();
	TestEqual(TEXT("round to page 1"), Note->GetCurrentText(), 0);
	TestEqual(TEXT("1/2 again"), Note->GetPageBlock()->GetText().ToString(), FString(TEXT("1/2")));

	// CLOSE: Open back from 0.4 s (still whole for 0.05 s), all gone at 0.4 s, and the screen off at 0.5 s.
	Note->PressClose();
	TestTrue(TEXT("closing"), Note->IsClosing());
	TestEqual(TEXT("Open back from 0.4 s"), Note->GetOpenTime(), 0.4000167f, 1e-5f);
	RunSecretsWidget(Note, 0.05f);
	TestEqual(TEXT("still whole at 0.05 s"), Note->GetDim()->GetRenderOpacity(), 1.f, 1e-3f);
	RunSecretsWidget(Note, 0.35f);
	TestEqual(TEXT("gone at 0.4 s"), Note->GetDim()->GetRenderOpacity(), 0.f, 1e-3f);
	TestEqual(TEXT("the paper back down"), static_cast<float>(Note->GetPaperBox()->GetRenderTransform().Translation.Y), 1052.0336f, 1e-2f);
	TestFalse(TEXT("up until 0.5 s"), Note->IsFinished());
	Note->PressClose();
	RunSecretsWidget(Note, 0.12f);
	TestTrue(TEXT("off at 0.5 s (a second CLOSE does not put it off)"), Note->IsFinished());

	// A note of one page drops the arrow.
	W* Single = NewObject<W>();
	Single->Texts = {FText::FromString(TEXT("Only page."))};
	Single->bENote = false;
	Single->Initialize();
	const TSharedRef<SWidget> SingleSlate = Single->TakeWidget();
	if (TestNotNull(TEXT("NextPage"), Single->GetNextPageButton()))
	{
		TestNull(TEXT("dropped for a single page"), Single->GetNextPageButton()->GetParent());
	}
	TestEqual(TEXT("1/1"), Single->GetPageBlock()->GetText().ToString(), FString(TEXT("1/1")));

	// No player, no screen (as in a test's world).
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	TestNull(TEXT("nothing without a player"), W::Show(Wrapper.GetTestWorld(), Picture, Note->Texts, true));
	return true;
}

#endif
