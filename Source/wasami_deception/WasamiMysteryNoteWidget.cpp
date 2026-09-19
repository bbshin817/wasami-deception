#include "WasamiMysteryNoteWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Open (UMG_MysteryNote), its keys as exported with UE's auto tangents. ScaleBox_91 (the paper): RenderTransform's
	// translation's y from 1052 below up past its place (−50 at 0.15 s) and back (0.35 s), and its angle from 1.75°
	// through −0.75° to 0 alike (the translation's x has one key, 0; the scale none).
	const FAnimKey NotePaperYKeys[] = {{0., 1052.0335693359375f, 0., 0.}, {9000., -50.f, -0.0233785230666399, -0.0233785230666399},
		{21000., 0.f, 0., 0.}};
	const FAnimKey NotePaperAngleKeys[] = {{0., 1.7458093166351318f, 0., 0.},
		{9000., -0.7455843091011047f, -8.313377475133166e-05, -8.313377475133166e-05}, {21000., 0.f, 0., 0.}};
	// RenderOpacity 0 → 1 over 0.35 s: ScaleBox_91, Image_0, Close, ScaleBox_0 and NextPage (the same two flat keys);
	// TextBlock_256 from 0.15 s; BackgroundBlur_0's BlurStrength 0 → 3 (its own is 2). The sections end at 21000 ticks
	// and a widget animation keeps what they set (its DefaultCompletionMode is KeepState).
	const FAnimKey NoteFadeKeys[] = {{0., 0.f, 0., 0.}, {21000., 1.f, 0., 0.}};
	const FAnimKey NotePageKeys[] = {{0., 0.f, 0., 0.}, {9000., 0.f, 4.761904710903764e-05, 4.761904710903764e-05}, {21000., 1.f, 0., 0.}};
	const FAnimKey NoteBlurKeys[] = {{0., 0.f, 0., 0.}, {21000., 3.f, 0., 0.}};
	// SwitchPage: RichTextBlock_0's RenderOpacity 0 → 1 over 0.25 s.
	const FAnimKey NoteSwitchPageKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};

	// Image_0: black at 0.798 over the whole screen.
	const FLinearColor NoteDim(0.f, 0.f, 0.f, 0.7979999780654907f);
	// TextBlock_0 (CLOSE): white at 0.5.
	const FLinearColor NoteCloseColor(1.f, 1.f, 1.f, 0.5f);
	// NextPage's Normal look: the arrow greyed to 0.703.
	const FLinearColor NoteArrowGrey(0.703125f, 0.703125f, 0.703125f, 1.f);
	// MysteryText's rows: Default (off-white) and Player (a pale yellow).
	const FLinearColor NoteTextDefault(0.9646859765052795f, 0.9646859765052795f, 0.9646859765052795f, 1.f);
	const FLinearColor NoteTextPlayer(1.f, 0.9002810120582581f, 0.43229201436042786f, 1.f);

	UCanvasPanelSlot* NotePlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	FSlateFontInfo NoteFont(UFont* Font, float Size, int32 Outline = 0)
	{
		FSlateFontInfo Info;
		Info.FontObject = Font;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		Info.OutlineSettings.OutlineSize = Outline;
		return Info;
	}

	/** A MysteryText row: helvetica-normal 18 with a 2 px black outline, no shadow, in Color. */
	FRichTextStyleRow NoteStyleRow(UFont* Font, const FLinearColor& Color)
	{
		FRichTextStyleRow Row;
		FSlateFontInfo Info = NoteFont(Font, 18.f, 2);
		Info.OutlineSettings.OutlineColor = FLinearColor::Black;
		Row.TextStyle.SetFont(Info);
		Row.TextStyle.SetColorAndOpacity(FSlateColor(Color));
		Row.TextStyle.SetShadowOffset(FVector2D::ZeroVector);
		Row.TextStyle.SetShadowColorAndOpacity(FLinearColor::Black);
		return Row;
	}
}

UWasamiMysteryNoteWidget::UWasamiMysteryNoteWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BoldFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
	NormalFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	ArrowTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/selection_bar_arrow_hover")));
	LoreNoteSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/DD_LoreNote_01")));
}

UWasamiMysteryNoteWidget* UWasamiMysteryNoteWidget::Show(const UObject* WorldContextObject, UTexture2D* InTexture,
	const TArray<FText>& InTexts, bool bInLoreNote)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiMysteryNoteWidget* Note = CreateWidget<UWasamiMysteryNoteWidget>(Controller, StaticClass());
	if (Note)
	{
		Note->Texture = InTexture;
		Note->Texts = InTexts;
		Note->bENote = bInLoreNote;
		Note->AddToViewport(ViewportZOrder);
	}
	return Note;
}

void UWasamiMysteryNoteWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const UWasamiMysteryNoteWidget* Defaults = GetDefault<UWasamiMysteryNoteWidget>();
	Out.Add(Defaults->BoldFont.LoadSynchronous());
	Out.Add(Defaults->NormalFont.LoadSynchronous());
	Out.Add(Defaults->ArrowTexture.LoadSynchronous());
	Out.Add(Defaults->LoreNoteSound.LoadSynchronous());
}

TSharedRef<SWidget> UWasamiMysteryNoteWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		UFont* Bold = BoldFont.LoadSynchronous();

		// SizeBox_1 → ScaleBox_91 → paper: the note's picture fitted into 1727 × 955 on the middle (the tree's
		// sewer_note_01 is not brought over; Construct puts Texture in).
		USizeBox* PaperSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_1"));
		PaperSize->SetWidthOverride(1726.6314697265625f);
		PaperSize->SetHeightOverride(955.0908813476562f);
		PaperBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ScaleBox_91"));
		PaperSize->AddChild(PaperBox);
		Paper = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("paper"));
		FSlateBrush PaperBrush;
		PaperBrush.ImageSize = FVector2D(753.f, 1061.f);
		Paper->SetBrush(PaperBrush);
		PaperBox->AddChild(Paper);
		NotePlace(Root, PaperSize, FAnchors(0.5f), FMargin(0.f, 0.f, 952.f, 1001.5f), FVector2D(0.5f, 0.5f), true);

		// BackgroundBlur_0 over the whole screen (the paper too), and Image_0's black at 0.8 over that.
		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_0"));
		Blur->SetBlurStrength(2.f);
		NotePlace(Root, Blur, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));
		Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_0"));
		FSlateBrush DimBrush;
		DimBrush.TintColor = FSlateColor(NoteDim);
		Dim->SetBrush(DimBrush);
		Dim->SetVisibility(ESlateVisibility::HitTestInvisible);
		NotePlace(Root, Dim, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

		// SizeBox_0 → ScaleBox_0 (shrinks only) → RichTextBlock_0: the words, wrapped at 1400, in MysteryText's styles.
		USizeBox* TextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_0"));
		TextSize->SetWidthOverride(1547.4156494140625f);
		TextSize->SetHeightOverride(723.7786254882812f);
		TextBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ScaleBox_0"));
		TextBox->SetStretchDirection(EStretchDirection::DownOnly);
		TextSize->AddChild(TextBox);
		TextStyles = NewObject<UDataTable>(this, TEXT("MysteryText"), RF_Transient);
		TextStyles->RowStruct = FRichTextStyleRow::StaticStruct();
		UFont* Normal = NormalFont.LoadSynchronous();
		TextStyles->AddRow(TEXT("Default"), NoteStyleRow(Normal, NoteTextDefault));
		TextStyles->AddRow(TEXT("Player"), NoteStyleRow(Normal, NoteTextPlayer));
		TextBlock = WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), TEXT("RichTextBlock_0"));
		TextBlock->SetTextStyleSet(TextStyles);
		TextBlock->SetWrapTextAt(1400.f);
		TextBox->AddChild(TextBlock);
		NotePlace(Root, TextSize, FAnchors(0.5f), FMargin(0.f, 0.f, 1202.f, 30.f), FVector2D(0.5f, 0.5f), true);

		// Close: CLOSE (helvetica-neue-bold 36, white at 0.5) on a clear button 236 px in from the bottom right.
		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Close"));
		CloseButton->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_0"));
		CloseText->SetText(FText::FromString(TEXT("CLOSE")));
		CloseText->SetColorAndOpacity(FSlateColor(NoteCloseColor));
		CloseText->SetFont(NoteFont(Bold, 36.f));
		CloseButton->AddChild(CloseText);
		NotePlace(Root, CloseButton, FAnchors(1.f), FMargin(-236.f, -104.f, 100.f, 40.f), FVector2D::ZeroVector, true);

		// TextBlock_256: the page, 150 px above the bottom's middle (helvetica-neue-bold 30, a 4 px outline, 200 wide at least).
		PageBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_256"));
		PageBlock->SetFont(NoteFont(Bold, 30.f, 4));
		PageBlock->SetMinDesiredWidth(200.f);
		PageBlock->SetJustification(ETextJustify::Center);
		NotePlace(Root, PageBlock, FAnchors(0.5f, 1.f), FMargin(0.f, -150.f, 100.f, 30.f), FVector2D(0.5f, 0.f), true);

		// NextPage: the arrow (21 × 32), grey until hovered, 76 px right of the middle and 420 down.
		NextPageButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextPage"));
		FButtonStyle Style = NextPageButton->GetStyle();
		UTexture2D* Arrow = ArrowTexture.LoadSynchronous();
		auto ArrowBrush = [Arrow](const FLinearColor& Tint)
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(Arrow);
			Brush.ImageSize = FVector2D(21.f, 32.f);
			Brush.TintColor = FSlateColor(Tint);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			return Brush;
		};
		Style.SetNormal(ArrowBrush(NoteArrowGrey));
		Style.SetHovered(ArrowBrush(FLinearColor::White));
		Style.SetPressed(ArrowBrush(FLinearColor::White));
		NextPageButton->SetStyle(Style);
		NotePlace(Root, NextPageButton, FAnchors(0.5f), FMargin(76.f, 420.f, 21.f, 32.f), FVector2D(0.f, 0.5f), false);

		NextPageButton->OnClicked.AddDynamic(this, &UWasamiMysteryNoteWidget::OnNextPageClicked);
		CloseButton->OnClicked.AddDynamic(this, &UWasamiMysteryNoteWidget::OnCloseClicked);
		ApplyAnimation();
	}
	return Super::RebuildWidget();
}

void UWasamiMysteryNoteWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiMysteryNoteWidget::NativeDestruct()
{
	// Destruct (@1361): IsValid(Sound) → Sound.FadeOut(0.5, 0).
	if (IsValid(Sound))
	{
		Sound->FadeOut(SoundFadeOutSeconds, 0.f);
	}
	Super::NativeDestruct();
}

void UWasamiMysteryNoteWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiMysteryNoteWidget::Begin()
{
	// Construct (@183): paper.SetBrushFromTexture(Texture, True); SetInputMode_UIOnlyEx(Self, DoNotLock); PlayAnimation(Open);
	// the cursor shown (Virtual Cursor, the gamepad's, left out as on the other screens); SetGamePaused(True); Update
	// Text; a single page → NextPage.RemoveFromParent. Then (@30, pushed first) E Note → CreateSound2D(DD_LoreNote_01,
	// 1, 1, 0, None, False, True) kept as Sound, and its FadeIn(0.5, 1, 0).
	CurrentText = 0;
	OpenTime = 0.f;
	bOpenReverse = false;
	SwitchPageTime = 0.f;
	bSwitchPagePlayed = false;
	bClosing = bFinished = false;
	CloseElapsed = 0.f;
	if (Paper && Texture)
	{
		Paper->SetBrushFromTexture(Texture, true);
	}
	if (GetWorld())
	{
		if (APlayerController* Controller = GetOwningPlayer())
		{
			UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, this, EMouseLockMode::DoNotLock);
			Controller->SetShowMouseCursor(true);
		}
		UGameplayStatics::SetGamePaused(this, true);
	}
	UpdateText();
	if (Texts.Num() == 1 && NextPageButton)
	{
		NextPageButton->RemoveFromParent();
	}
	ApplyAnimation();
	if (bENote && GetWorld())
	{
		// CreateSound2D makes a UI sound (it goes on while the game is paused).
		if (USoundBase* Loaded = LoreNoteSound.LoadSynchronous())
		{
			Sound = UGameplayStatics::CreateSound2D(this, Loaded, 1.f, 1.f, 0.f, nullptr, false, true);
			if (Sound)
			{
				Sound->FadeIn(SoundFadeInSeconds, 1.f, 0.f);
			}
		}
	}
}

void UWasamiMysteryNoteWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// The animations move on first and the Delay comes due after them, as a user widget ticks them.
	OpenTime = bOpenReverse ? FMath::Max(OpenTime - DeltaSeconds, 0.f) : FMath::Min(OpenTime + DeltaSeconds, OpenLength);
	if (bSwitchPagePlayed)
	{
		SwitchPageTime = FMath::Min(SwitchPageTime + DeltaSeconds, SwitchPageLength);
	}
	ApplyAnimation();
	if (bClosing)
	{
		CloseElapsed += DeltaSeconds;
		if (CloseElapsed >= CloseDelay)
		{
			bFinished = true;
			RemoveFromParent();
		}
	}
}

void UWasamiMysteryNoteWidget::PressNextPage()
{
	// @917: PlayAnimation(SwitchPage) from its start; currentText back to 0 on the last page, else one on; Update Text.
	SwitchPageTime = 0.f;
	bSwitchPagePlayed = true;
	CurrentText = CurrentText == Texts.Num() - 1 ? 0 : CurrentText + 1;
	UpdateText();
	ApplyAnimation();
}

void UWasamiMysteryNoteWidget::PressClose()
{
	// @823: SetGamePaused(False); SetInputMode_GameOnly; (@624) the cursor hidden; PlayAnimation(Open, 0.35, 1, Reverse, 1),
	// which a reverse play starts 0.35 s before the end; Delay(0.5) → RemoveFromParent, which a pending Delay ignores.
	OpenTime = FMath::Max(OpenLength - CloseFrom, 0.f);
	bOpenReverse = true;
	if (!bClosing)
	{
		bClosing = true;
		CloseElapsed = 0.f;
	}
	ApplyAnimation();
	if (GetWorld())
	{
		UGameplayStatics::SetGamePaused(this, false);
		if (APlayerController* Controller = GetOwningPlayer())
		{
			UWidgetBlueprintLibrary::SetInputMode_GameOnly(Controller);
			Controller->SetShowMouseCursor(false);
		}
	}
}

FText UWasamiMysteryNoteWidget::GetPageText() const
{
	return FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentText + 1, Texts.Num()));
}

void UWasamiMysteryNoteWidget::UpdateText()
{
	// Array_Get past the end gives an empty text.
	if (TextBlock)
	{
		TextBlock->SetText(Texts.IsValidIndex(CurrentText) ? Texts[CurrentText] : FText::GetEmpty());
	}
}

void UWasamiMysteryNoteWidget::ApplyAnimation()
{
	const float Fade = EvaluateFade(OpenTime);
	if (PaperBox)
	{
		FWidgetTransform Transform = PaperBox->GetRenderTransform();
		Transform.Translation = FVector2D(0.f, EvaluatePaperY(OpenTime));
		Transform.Angle = EvaluatePaperAngle(OpenTime);
		PaperBox->SetRenderTransform(Transform);
		PaperBox->SetRenderOpacity(Fade);
	}
	if (Blur)
	{
		Blur->SetBlurStrength(EvaluateBlur(OpenTime));
	}
	UWidget* const FadedIn[] = {Dim.Get(), CloseButton.Get(), TextBox.Get(), NextPageButton.Get()};
	for (UWidget* Faded : FadedIn)
	{
		if (Faded)
		{
			Faded->SetRenderOpacity(Fade);
		}
	}
	if (PageBlock)
	{
		PageBlock->SetRenderOpacity(EvaluatePageOpacity(OpenTime));
		// GetText_0, bound to TextBlock_256's text.
		PageBlock->SetText(GetPageText());
	}
	if (TextBlock && bSwitchPagePlayed)
	{
		TextBlock->SetRenderOpacity(EvaluateSwitchPage(SwitchPageTime));
	}
}

void UWasamiMysteryNoteWidget::OnNextPageClicked()
{
	PressNextPage();
}

void UWasamiMysteryNoteWidget::OnCloseClicked()
{
	PressClose();
}

float UWasamiMysteryNoteWidget::EvaluatePaperY(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(NotePaperYKeys);
	return Eval(Curve, Seconds, OpenLength);
}

float UWasamiMysteryNoteWidget::EvaluatePaperAngle(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(NotePaperAngleKeys);
	return Eval(Curve, Seconds, OpenLength);
}

float UWasamiMysteryNoteWidget::EvaluateFade(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(NoteFadeKeys);
	return Eval(Curve, Seconds, OpenLength);
}

float UWasamiMysteryNoteWidget::EvaluatePageOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(NotePageKeys);
	return Eval(Curve, Seconds, OpenLength);
}

float UWasamiMysteryNoteWidget::EvaluateBlur(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(NoteBlurKeys);
	return Eval(Curve, Seconds, OpenLength);
}

float UWasamiMysteryNoteWidget::EvaluateSwitchPage(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(NoteSwitchPageKeys);
	return Eval(Curve, Seconds, SwitchPageLength);
}
