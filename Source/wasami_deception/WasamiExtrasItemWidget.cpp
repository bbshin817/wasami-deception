#include "WasamiExtrasItemWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiMaximizePictureWidget.h"
#include "WasamiSaveGame.h"

namespace
{
	// UE 4's default button brush (the tree's brushes keep what they do not set): drawn as a box, 8/32 of the size kept
	// at each edge, 32 square, white.
	const FMargin ExtrasDefaultBoxMargin(8.f / 32.f);

	FSlateBrush ExtrasBrush(UTexture2D* Texture, const FVector2D& Size, const FMargin& Margin, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = Margin;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	UCanvasPanelSlot* ExtrasPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
		const FVector2D& Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = Anchors;
		Layout.Offsets = Offsets;
		Layout.Alignment = Alignment;
		Slot->SetLayout(Layout);
		return Slot;
	}
}

UWasamiExtrasItemWidget::UWasamiExtrasItemWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SaveSlotName = UWasamiSaveGame::SlotName;
	FrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/RingAltar_UI/Textures/ring_altar_power_equipped_frame")));
	LockedTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/TitleScreen/locked")));
	WhiteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Engine/EngineResources/WhiteSquareTexture")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
}

TSharedRef<SWidget> UWasamiExtrasItemWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		UTexture2D* Frame = FrameTexture.LoadSynchronous();
		UTexture2D* White = WhiteTexture.LoadSynchronous();

		// Button_104: a square in the middle (150; the video's 250), the frame as its Normal (0.1 kept at the edges,
		// tinted 0.516) and Hovered (105 × 104) brushes, a white square as Pressed and a black one as Disabled; Construct
		// restyles it.
		Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_104"));
		const float Normal = GetNormalSize();
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(ExtrasBrush(Frame, FVector2D(Normal, Normal), FMargin(0.1f), FLinearColor(0.515625f, 0.515625f, 0.515625f, 1.f)))
			.SetHovered(ExtrasBrush(Frame, FVector2D(105.f, 104.f), ExtrasDefaultBoxMargin, FLinearColor::White))
			.SetPressed(ExtrasBrush(White, FVector2D(32.f, 32.f), ExtrasDefaultBoxMargin, FLinearColor::White))
			.SetDisabled(ExtrasBrush(White, FVector2D(32.f, 32.f), ExtrasDefaultBoxMargin, FLinearColor::Black))
			.SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		const float SlotSize = GetSlotSize();
		ExtrasPlace(Root, Button, FAnchors(0.5f), FMargin(0.f, 0.f, SlotSize, SlotSize), FVector2D(0.5f, 0.5f));

		// ScaleBox_0 → Image_1: the lock (its brush 2500 square) filling the extra but for 5 px each side, cropped, and
		// letting the pointer through to the button.
		ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ScaleBox_0"));
		ScaleBox->SetStretch(EStretch::ScaleToFill);
		ScaleBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_1"));
		FSlateBrush Lock;
		Lock.SetResourceObject(LockedTexture.LoadSynchronous());
		Lock.ImageSize = FVector2D(2500.f, 2500.f);
		Image->SetBrush(Lock);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		ScaleBox->AddChild(Image);
		ExtrasPlace(Root, ScaleBox, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(5.f));

		Button->OnClicked.AddDynamic(this, &UWasamiExtrasItemWidget::OnButtonClicked);
	}
	return Super::RebuildWidget();
}

void UWasamiExtrasItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiExtrasItemWidget::Begin()
{
	// @252: Button_104.SetStyle with Normal the tree's (the frame) tinted 0.4531 grey, Hovered the tree's (the frame)
	// tinted white, Pressed a plain white square (no texture, drawn as an image, 32) and the rest the tree's.
	if (Button)
	{
		FButtonStyle Style = Button->GetStyle();
		FSlateBrush Normal = Style.Normal;
		Normal.TintColor = FSlateColor(FLinearColor(NormalGrey, NormalGrey, NormalGrey, 1.f));
		FSlateBrush Hovered = Style.Hovered;
		Hovered.TintColor = FSlateColor(FLinearColor::White);
		FSlateBrush Pressed;
		Pressed.ImageSize = FVector2D(32.f, 32.f);
		Pressed.DrawAs = ESlateBrushDrawType::Image;
		Pressed.TintColor = FSlateColor(FLinearColor::White);
		Style.SetNormal(Normal).SetHovered(Hovered).SetPressed(Pressed);
		Button->SetStyle(Style);
	}
	// LoadGameFromSlot('SaveSlot') → Image_1.SetBrushFromTexture(unlocked ? Art : locked, False) and Button_104
	// Visible (0) or HitTestInvisible (3).
	if (!Save)
	{
		Save = UGameplayStatics::DoesSaveGameExist(SaveSlotName, UWasamiSaveGame::UserIndex)
			? Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UWasamiSaveGame::UserIndex)) : nullptr;
		if (!Save)
		{
			Save = NewObject<UWasamiSaveGame>(this);
		}
	}
	const bool bUnlocked = IsUnlocked();
	if (Image)
	{
		Image->SetBrushFromTexture(bUnlocked ? GetUnlockedTexture() : LockedTexture.LoadSynchronous(), false);
	}
	if (Button)
	{
		Button->SetVisibility(bUnlocked ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	}
}

bool UWasamiExtrasItemWidget::Press()
{
	// @2511: the save read again, and only an unlocked extra goes on: PlaySound2D(UI_Select_V3, 1, 1.25), then @10.
	if (!IsUnlocked())
	{
		return false;
	}
	if (GetWorld())
	{
		if (USoundBase* Loaded = SelectSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, SelectPitch);
		}
	}
	Open();
	return true;
}

bool UWasamiExtrasItemWidget::IsUnlocked() const
{
	return Save && IsUnlockedIn(*Save);
}

bool UWasamiExtrasItemWidget::IsUnlockedIn(const UWasamiSaveGame& Read) const
{
	return Read.IsUnlocked(FWasamiCollectableEntry{EWasamiCollectableType::ArtGallery, ID});
}

void UWasamiExtrasItemWidget::Open()
{
	UWasamiMaximizePictureWidget::Show(this, Art, Text);
}

void UWasamiExtrasItemWidget::OnButtonClicked()
{
	Press();
}
