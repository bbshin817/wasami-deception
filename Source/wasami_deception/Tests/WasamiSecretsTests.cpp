#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/BackgroundBlur.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RichTextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"
#include "../WasamiAssets.h"
#include "../WasamiCollectable.h"
#include "../WasamiCollectablesWidget.h"
#include "../WasamiGameMode.h"
#include "../WasamiInteractable.h"
#include "../WasamiMysteryNoteWidget.h"
#include "../WasamiSaveGame.h"
#include "../WasamiSecretRoomZone.h"
#include "../WasamiSecretWall.h"

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

namespace
{
	/** The ticks' length. */
	constexpr float SecretsStep = 0.05f;

	const FVector SecretsPlayerAway(8000., 8000., 100.);

	void AdvanceSecrets(FTestWorldWrapper& Wrapper, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f; Left -= SecretsStep)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, SecretsStep));
		}
	}

	/** The player: possessed by the first player controller (not a local player's: no screens), held where it is put. */
	ACharacter* SpawnSecretsPlayer(UWorld* World)
	{
		ACharacter* Player = World->SpawnActor<ACharacter>(SecretsPlayerAway, FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!Player || !Controller)
		{
			return nullptr;
		}
		Controller->Possess(Player);
		Player->GetCharacterMovement()->DisableMovement();
		return UGameplayStatics::GetPlayerCharacter(World, 0) == Player ? Player : nullptr;
	}

	void SecretsTeleport(AActor* Actor, const FVector& Where)
	{
		Actor->SetActorLocation(Where, false, nullptr, ETeleportType::TeleportPhysics);
	}

	AWasamiCollectable* SpawnCollectable(UWorld* World, int32 ID, const FVector& Where)
	{
		const FTransform Placed(Where);
		AWasamiCollectable* File = World->SpawnActorDeferred<AWasamiCollectable>(AWasamiCollectable::StaticClass(), Placed);
		if (File)
		{
			File->ID = ID;
			File->FinishSpawning(Placed);
		}
		return File;
	}

	/** The game mode's save's Secrets, emptied for a test and put back by the guard. */
	struct FSecretsSaveGuard
	{
		explicit FSecretsSaveGuard(TArray<int32>& InSecrets) : Secrets(InSecrets), Kept(InSecrets) { Secrets.Reset(); }
		~FSecretsSaveGuard() { Secrets = Kept; }
		TArray<int32>& Secrets;
		TArray<int32> Kept;
	};

	UWasamiSaveGame* SecretsSave(FAutomationTestBase& Test, UWorld* World)
	{
		AWasamiGameMode* Mode = World->GetAuthGameMode<AWasamiGameMode>();
		if (!Test.TestNotNull(TEXT("the project's game mode"), Mode))
		{
			return nullptr;
		}
		UWasamiSaveGame* Save = Mode->GetSave();
		Test.TestNotNull(TEXT("its save"), Save);
		return Save;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsCollectablePartsTest, "Wasami.Secrets.Collectable.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsCollectablePartsTest::RunTest(const FString& Parameters)
{
	using A = AWasamiCollectable;

	// Bounce: 0, 1, 0 at 0, 2.5 and 5 s with flat tangents, looping at 3 times its speed; the folder 35 to 40 cm up and
	// turned 0 to 7°. The Delay before the save's check, and the voice's volume.
	TestEqual(TEXT("low at 0"), A::EvaluateBounce(0.f), 0.f, 1e-5f);
	TestEqual(TEXT("half way at 1.25 s"), A::EvaluateBounce(1.25f), 0.5f, 1e-5f);
	TestEqual(TEXT("high at 2.5 s"), A::EvaluateBounce(2.5f), 1.f, 1e-5f);
	TestEqual(TEXT("half way down at 3.75 s"), A::EvaluateBounce(3.75f), 0.5f, 1e-5f);
	TestEqual(TEXT("low at 5 s"), A::EvaluateBounce(5.f), 0.f, 1e-5f);
	TestTrue(TEXT("eased: under a quarter a quarter of the way up"), A::EvaluateBounce(0.625f) < 0.25f);
	TestEqual(TEXT("35 cm at the bottom"), A::BounceHeight(0.f), 35.f);
	TestEqual(TEXT("40 cm at the top"), A::BounceHeight(1.f), 40.f);
	TestEqual(TEXT("turned 7° at the top"), A::BounceYaw(1.f), 7.f);
	TestEqual(TEXT("5 s long"), A::BounceLength, 5.f);
	TestEqual(TEXT("at 3 times its speed"), A::BouncePlayRate, 3.f);
	TestEqual(TEXT("the save's check 0.2 s on"), A::SaveCheckDelay, 0.2f);
	TestEqual(TEXT("the voice at 0.85"), A::PickupVolume, 0.85f);
	TestEqual(TEXT("the voice"), GetDefault<A>()->PickupSound.ToSoftObjectPath().GetAssetName(), FString(TEXT("Bierce_Secret_Files_Pickup")));
	TestEqual(TEXT("ID 0 by default"), GetDefault<A>()->ID, 0);

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	A* File = World->SpawnActor<A>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the file"), File))
	{
		return false;
	}

	// The box: 36 cm up, 1.23 × 1.30 × 1 of UE's 32 cm, overlapping, out of the navigation.
	const UBoxComponent* Box = File->GetBox();
	TestEqual(TEXT("the box 36 cm up"), Box->GetRelativeLocation(), FVector(0., 0., 36.43724060058594));
	TestEqual(TEXT("its scale"), Box->GetRelativeScale3D(), FVector(1.2296168804168701, 1.2997432947158813, 1.));
	TestEqual(TEXT("UE's 32 cm"), Box->GetUnscaledBoxExtent(), FVector(32.));
	TestTrue(TEXT("it overlaps pawns"), Box->GetGenerateOverlapEvents() && Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
	TestFalse(TEXT("out of the navigation"), Box->CanEverAffectNavigation());

	// The folder: at 60, touching nothing, moved by Bounce.
	const UStaticMeshComponent* Mesh = File->GetStaticMesh();
	TestEqual(TEXT("the folder at 60"), Mesh->GetRelativeScale3D(), FVector(60.));
	TestEqual(TEXT("touching nothing"), Mesh->GetCollisionProfileName(), UCollisionProfile::NoCollision_ProfileName);
	TestTrue(TEXT("movable"), Mesh->Mobility == EComponentMobility::Movable);

	// The light: 1000 unitless, 250 cm, a soft source of 2000, no shadows, 35.8 cm up on the root.
	const UPointLightComponent* Light = File->GetPointLight();
	TestEqual(TEXT("the light 35.8 cm up"), Light->GetRelativeLocation(), FVector(0., 0., 35.76698303222656));
	TestTrue(TEXT("unitless"), Light->IntensityUnits == ELightUnits::Unitless);
	TestEqual(TEXT("at 1000"), Light->Intensity, 1000.f);
	TestEqual(TEXT("250 cm"), Light->AttenuationRadius, 250.f);
	TestEqual(TEXT("a soft source of 2000"), Light->SoftSourceRadius, 2000.f);
	TestFalse(TEXT("no shadows"), static_cast<bool>(Light->CastShadows));
	TestTrue(TEXT("on the root, not the folder"), Light->GetAttachParent() == File->GetRootComponent());

	// Bounce moves the folder at the timeline's position, and loops.
	AdvanceSecrets(Wrapper, 0.3f);
	float Value = A::EvaluateBounce(File->GetBouncePosition());
	TestEqual(TEXT("3 times as fast"), File->GetBouncePosition(), 0.9f, 1e-3f);
	TestEqual(TEXT("the folder's height on the curve"), static_cast<float>(Mesh->GetRelativeLocation().Z), A::BounceHeight(Value), 1e-3f);
	TestEqual(TEXT("its turn on the curve"), static_cast<float>(Mesh->GetRelativeRotation().Yaw), A::BounceYaw(Value), 1e-3f);
	AdvanceSecrets(Wrapper, 1.5f);
	TestEqual(TEXT("round again after 5 s of it (1.8 s)"), File->GetBouncePosition(), 0.4f, 1e-3f);
	Value = A::EvaluateBounce(File->GetBouncePosition());
	TestEqual(TEXT("low again"), static_cast<float>(Mesh->GetRelativeLocation().Z), A::BounceHeight(Value), 1e-3f);
	TestTrue(TEXT("still there with nothing in the save"), IsValid(File));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsCollectableCollectTest, "Wasami.Secrets.Collectable.Collect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsCollectableCollectTest::RunTest(const FString& Parameters)
{
	// The test world has no local player, so NEW EXTRAS UNLOCKED! is not put on a screen.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 0);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	UWasamiSaveGame* Save = SecretsSave(*this, World);
	if (!Save)
	{
		return false;
	}
	FSecretsSaveGuard Guard(Save->Hospital.Secrets);

	AWasamiCollectable* File = SpawnCollectable(World, 2, FVector::ZeroVector);
	ACharacter* Player = SpawnSecretsPlayer(World);
	const FVector OtherAway = SecretsPlayerAway + FVector(0., 500., 0.);
	ACharacter* Other = World->SpawnActor<ACharacter>(OtherAway, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the file"), File) || !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("another character"), Other))
	{
		return false;
	}
	Other->GetCharacterMovement()->DisableMovement();
	AdvanceSecrets(Wrapper, 0.3f);
	TestTrue(TEXT("there after the save's check"), IsValid(File));

	// Only the player takes it.
	const FVector Inside = File->GetBox()->GetComponentLocation();
	SecretsTeleport(Other, Inside);
	AdvanceSecrets(Wrapper, SecretsStep);
	TestTrue(TEXT("not taken by another character"), IsValid(File) && !File->IsTaken());
	TestEqual(TEXT("nothing in the save"), Save->Hospital.Secrets.Num(), 0);
	SecretsTeleport(Other, OtherAway);

	SecretsTeleport(Player, Inside);
	AdvanceSecrets(Wrapper, SecretsStep);
	TestFalse(TEXT("taken and gone"), IsValid(File));
	TestTrue(TEXT("its ID in the save's Secrets"), Save->Hospital.Secrets.Num() == 1 && Save->Hospital.Secrets[0] == 2);

	// Another file of the same ID (the level opened again before a checkpoint's save) adds it once only.
	SecretsTeleport(Player, SecretsPlayerAway);
	AWasamiCollectable* Again = SpawnCollectable(World, 2, FVector(0., 2000., 0.));
	if (!TestNotNull(TEXT("another file"), Again))
	{
		return false;
	}
	Again->Collect();
	TestTrue(TEXT("taken"), Again->IsTaken());
	Again->Collect();
	TestEqual(TEXT("the ID in the save once"), Save->Hospital.Secrets.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsCollectableSaveTest, "Wasami.Secrets.Collectable.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsCollectableSaveTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	UWasamiSaveGame* Save = SecretsSave(*this, World);
	if (!Save)
	{
		return false;
	}
	FSecretsSaveGuard Guard(Save->Hospital.Secrets);
	Save->Hospital.Secrets = {1};

	// 0.2 s after BeginPlay the one whose ID the save has is gone; the other stays. (A timer set outside a tick starts
	// counting at the end of the next one, so it runs out in the sixth tick of 0.05 s.)
	AWasamiCollectable* First = SpawnCollectable(World, 0, FVector::ZeroVector);
	AWasamiCollectable* Second = SpawnCollectable(World, 1, FVector(0., 2000., 0.));
	if (!TestNotNull(TEXT("file 0"), First) || !TestNotNull(TEXT("file 1"), Second))
	{
		return false;
	}
	AdvanceSecrets(Wrapper, 0.15f);
	TestTrue(TEXT("both there before 0.2 s"), IsValid(First) && IsValid(Second));
	AdvanceSecrets(Wrapper, 0.15f);
	TestTrue(TEXT("file 0 stays"), IsValid(First));
	TestFalse(TEXT("file 1, in the save, is gone"), IsValid(Second));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsRoomZoneTest, "Wasami.Secrets.SecretRoomZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsRoomZoneTest::RunTest(const FString& Parameters)
{
	using A = AWasamiSecretRoomZone;

	// The test world has no local player, so YOU FOUND A MYSTERIOUS ROOM is not put on a screen.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 0);
	TestEqual(TEXT("the whispers fade over 1 s"), A::WhispersFadeSeconds, 1.f);
	TestEqual(TEXT("the whispers"), GetDefault<A>()->WhispersSound.ToSoftObjectPath().GetAssetName(), FString(TEXT("67-Dark_Whispers_SFX_0704")));
	TestEqual(TEXT("the glitch"), GetDefault<A>()->GlitchBase.ToSoftObjectPath().GetAssetName(), FString(TEXT("M_DD_ChameleonGlitch")));

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	A* Zone = World->SpawnActor<A>(FVector::ZeroVector, FRotator::ZeroRotator);
	ACharacter* Player = SpawnSecretsPlayer(World);
	const FVector OtherAway = SecretsPlayerAway + FVector(0., 500., 0.);
	ACharacter* Other = World->SpawnActor<ACharacter>(OtherAway, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the zone"), Zone) || !TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("another character"), Other))
	{
		return false;
	}
	Other->GetCharacterMovement()->DisableMovement();

	// The box: 20 cm up, 5 × 2 × 1.5 of UE's 32 cm, out of the navigation.
	const UBoxComponent* Box = Zone->GetBox();
	TestEqual(TEXT("the box 20 cm up"), Box->GetRelativeLocation(), FVector(0., 0., 20.));
	TestEqual(TEXT("its scale"), Box->GetRelativeScale3D(), FVector(5., 2., 1.5));
	TestFalse(TEXT("out of the navigation"), Box->CanEverAffectNavigation());

	// The Chameleon's post process: unbound, the glitch its one blendable at 1, with the zone's values, off.
	const UPostProcessComponent* Glitch = Zone->GetGlitch();
	UMaterialInstanceDynamic* Material = Zone->GetGlitchMaterial();
	TestTrue(TEXT("unbound"), static_cast<bool>(Glitch->bUnbound));
	if (!TestNotNull(TEXT("the glitch's material"), Material))
	{
		return false;
	}
	const TArray<FWeightedBlendable>& Blendables = Glitch->Settings.WeightedBlendables.Array;
	TestTrue(TEXT("its one blendable at 1"), Blendables.Num() == 1 && Blendables[0].Object == Material && Blendables[0].Weight == 1.f);
	TestEqual(TEXT("Glitch Speed 10"), Material->K2_GetScalarParameterValue(TEXT("Speed")), 10.f);
	TestEqual(TEXT("Glitch Lines 30"), Material->K2_GetScalarParameterValue(TEXT("Density")), 30.f);
	TestEqual(TEXT("Glitch Blocking 0.5"), Material->K2_GetScalarParameterValue(TEXT("Amount")), 0.5f);
	TestEqual(TEXT("the grid's distortion 0.001"), Material->K2_GetScalarParameterValue(TEXT("GridDistortionPower")), 0.001f, 1e-6f);
	TestEqual(TEXT("its size 10"), Material->K2_GetScalarParameterValue(TEXT("GridDistortionSize")), 10.f);
	TestEqual(TEXT("its speed 1"), Material->K2_GetScalarParameterValue(TEXT("GridDistortionSpeed")), 1.f);
	TestEqual(TEXT("off"), Material->K2_GetScalarParameterValue(TEXT("BlendingOpacity")), 0.f);
	TestFalse(TEXT("no banner yet"), Zone->HasShownBanner());

	// Another character walking in does nothing.
	const FVector Inside = Box->GetComponentLocation();
	SecretsTeleport(Other, Inside);
	AdvanceSecrets(Wrapper, SecretsStep);
	TestEqual(TEXT("not for another character"), Zone->GetGlitchOpacity(), 0.f);
	TestFalse(TEXT("nor its banner"), Zone->HasShownBanner());
	SecretsTeleport(Other, OtherAway);
	AdvanceSecrets(Wrapper, SecretsStep);

	// The player in: the glitch on, the whispers fading in, the banner.
	SecretsTeleport(Player, Inside);
	AdvanceSecrets(Wrapper, SecretsStep);
	TestEqual(TEXT("the glitch on"), Zone->GetGlitchOpacity(), 1.f);
	TestEqual(TEXT("on in the material"), Material->K2_GetScalarParameterValue(TEXT("BlendingOpacity")), 1.f);
	TestTrue(TEXT("the banner"), Zone->HasShownBanner());
	if (UAudioComponent* Whispers = Zone->GetWhispers())
	{
		TestTrue(TEXT("the whispers playing"), Whispers->IsPlaying());
	}

	// Out: off; in again: on, with no second banner (the DoOnce).
	SecretsTeleport(Player, SecretsPlayerAway);
	AdvanceSecrets(Wrapper, SecretsStep);
	TestEqual(TEXT("the glitch off"), Zone->GetGlitchOpacity(), 0.f);
	TestEqual(TEXT("off in the material"), Material->K2_GetScalarParameterValue(TEXT("BlendingOpacity")), 0.f);
	SecretsTeleport(Player, Inside);
	AdvanceSecrets(Wrapper, SecretsStep);
	TestEqual(TEXT("on again"), Zone->GetGlitchOpacity(), 1.f);
	TestTrue(TEXT("the banner put up once"), Zone->HasShownBanner());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSecretsWallTest, "Wasami.Secrets.SecretWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSecretsWallTest::RunTest(const FString& Parameters)
{
	using A = AWasamiSecretWall;

	// Move Up: 0 to 1 over 3 s, linear, in a 5 s timeline played at 0.7; Sliding_Wall at 0.65 with 01_Lobby_Attenuation.
	TestEqual(TEXT("down at 0"), A::EvaluateMoveUp(0.f), 0.f);
	TestEqual(TEXT("half way at 1.5 s"), A::EvaluateMoveUp(1.5f), 0.5f, 1e-5f);
	TestEqual(TEXT("up at 3 s"), A::EvaluateMoveUp(3.f), 1.f);
	TestEqual(TEXT("and after"), A::EvaluateMoveUp(5.f), 1.f);
	TestEqual(TEXT("played at 0.7"), A::MoveUpPlayRate, 0.7f);
	TestEqual(TEXT("the slide at 0.65"), A::SlideVolume, 0.65f);
	TestEqual(TEXT("275 cm up"), GetDefault<A>()->Height, 275.f);
	TestEqual(TEXT("the slide"), GetDefault<A>()->SlideSound.ToSoftObjectPath().GetAssetName(), FString(TEXT("Sliding_Wall")));
	TestEqual(TEXT("its attenuation"), GetDefault<A>()->SlideAttenuation.ToSoftObjectPath().GetAssetName(), FString(TEXT("01_Lobby_Attenuation")));

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	A* Wall = World->SpawnActor<A>(FVector(0., 0., 100.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the wall"), Wall))
	{
		return false;
	}

	// The wall: at 100, blocking, tagged interact (the hand), generating no overlaps, out of the navigation.
	const UStaticMeshComponent* Mesh = Wall->GetStaticMesh();
	TestEqual(TEXT("at 100"), Mesh->GetRelativeScale3D(), FVector(100.));
	TestTrue(TEXT("tagged interact"), Mesh->ComponentHasTag(TEXT("interact")));
	TestEqual(TEXT("blocking"), Mesh->GetCollisionProfileName(), UCollisionProfile::BlockAllDynamic_ProfileName);
	TestFalse(TEXT("no overlaps"), Mesh->GetGenerateOverlapEvents());
	TestFalse(TEXT("out of the navigation"), Mesh->CanEverAffectNavigation());
	TestEqual(TEXT("OG Height"), Wall->GetOGHeight(), 100.f);
	TestEqual(TEXT("InterpHeight"), Wall->GetInterpHeight(), 375.f);
	TestTrue(TEXT("usable by the look"), Wall->Implements<UWasamiInteractable>());

	// Used: the tag off, Move Up from its start.
	IWasamiInteractable::Execute_InteractWithObject(Wall, nullptr);
	TestTrue(TEXT("used"), Wall->IsUsed());
	TestFalse(TEXT("the tag off"), Mesh->ComponentHasTag(TEXT("interact")));
	TestTrue(TEXT("moving"), Wall->IsMoving());
	TestEqual(TEXT("from where it was"), Wall->GetActorLocation().Z, 100., 1e-3);

	// Half way at 1.5 s of the curve (2.14 s); up at 3 s (4.29 s); the timeline on to its 5 s (7.14 s).
	AdvanceSecrets(Wrapper, 1.5f / A::MoveUpPlayRate);
	TestEqual(TEXT("on its way at the timeline's position"), Wall->GetActorLocation().Z,
		static_cast<double>(FMath::Lerp(100.f, 375.f, A::EvaluateMoveUp(Wall->GetMoveUpPosition()))), 1e-2);
	TestEqual(TEXT("about half way"), Wall->GetActorLocation().Z, 237.5, 3.);
	AdvanceSecrets(Wrapper, 1.5f / A::MoveUpPlayRate + 0.1f);
	TestEqual(TEXT("275 cm up"), Wall->GetActorLocation().Z, 375., 1e-3);
	TestTrue(TEXT("the timeline still running"), Wall->IsMoving());

	// A second use does nothing.
	const float Position = Wall->GetMoveUpPosition();
	IWasamiInteractable::Execute_InteractWithObject(Wall, nullptr);
	TestTrue(TEXT("not started again"), Wall->GetMoveUpPosition() == Position && Wall->GetActorLocation().Z > 374.);
	AdvanceSecrets(Wrapper, 3.f);
	TestFalse(TEXT("stopped at its end"), Wall->IsMoving());
	TestEqual(TEXT("at 5 s"), Wall->GetMoveUpPosition(), A::MoveUpLength);
	TestEqual(TEXT("and up"), Wall->GetActorLocation().Z, 375., 1e-3);
	return true;
}

#endif
