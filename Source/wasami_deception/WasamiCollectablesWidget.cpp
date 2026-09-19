#include "WasamiCollectablesWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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

	// NewAnimation_1 (UMG_Collectables and UMG_Collectables_Secret alike), its keys as exported with UE's auto tangents.
	// CanvasPanel_0: RenderTransform's scale (Scale and Scale[1] alike), from nothing past 1.1 at 0.25 s and back to 1 at
	// 0.4 s; RenderOpacity in by 0.25 s and out from 1.75 s to 2 s.
	const FAnimKey CollectablesScaleKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.1f, 4.166666622040793e-05, 4.166666622040793e-05},
		{24000., 1.f, -2.2222227471502265e-06, -2.2222227471502265e-06}, {60000., 1.f, 0., 0.},
		{105000., 1.f, 0., 3.3333342344121775e-06}};
	const FAnimKey CollectablesOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 9.523810149403289e-06, 9.523810149403289e-06},
		{105000., 1.f, -9.523810149403289e-06, -9.523810149403289e-06}, {120000., 0.f, 0., 0.}};
	// Image_297: the red flash's RenderOpacity, 0.2 at 0.25 s between 0.15 s and 0.6 s, in a section that ends at 60001
	// ticks (at 0, which it keeps).
	const FAnimKey CollectablesFlashKeys[] = {{9000., 0.f, 0., 0.}, {15000., 0.2f, 0., 0.}, {36000., 0.f, 0., 0.}};

	// CanvasPanel_0 at rest: 1.2 times its size and unseen (NewAnimation_1 sets both from its start).
	constexpr float CollectablesRestScale = 1.2f;
	constexpr float CollectablesRestOpacity = 0.f;
	// Image_297: WhiteSquareTexture tinted red, unseen at rest.
	const FLinearColor CollectablesFlashTint(1.f, 0.f, 0.f, 1.f);

	UCanvasPanelSlot* CollectablesPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
		const FVector2D& Alignment, bool bAutoSize)
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

	FSlateBrush CollectablesBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}
}

UWasamiCollectablesWidget::UWasamiCollectablesWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/Collectables/extras_unlock_bg")));
	IconTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/Collectables/art_icon")));
	for (const TCHAR* Name : {TEXT("art_icon"), TEXT("diary_icon"), TEXT("sound_icon"), TEXT("movie_icon")})
	{
		RandomIcons.Add(TSoftObjectPtr<UTexture2D>(
			WasamiAssets::Path(*FString::Printf(TEXT("/Game/DD/UI/Main/Collectables/%s"), Name))));
	}
	FlashTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Engine/EngineResources/WhiteSquareTexture")));
	TextFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

FText UWasamiCollectablesWidget::UnlockedText()
{
	return FText::FromString(TEXT("NEW EXTRAS UNLOCKED!"));
}

UWasamiCollectablesWidget* UWasamiCollectablesWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiCollectablesWidget* Widget = CreateWidget<UWasamiCollectablesWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->AddToPlayerScreen(ZOrder);
	}
	return Widget;
}

void UWasamiCollectablesWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const UWasamiCollectablesWidget* Defaults = GetDefault<UWasamiCollectablesWidget>();
	Out.Add(Defaults->FrameTexture.LoadSynchronous());
	Out.Add(Defaults->IconTexture.LoadSynchronous());
	for (const TSoftObjectPtr<UTexture2D>& Each : Defaults->RandomIcons)
	{
		Out.Add(Each.LoadSynchronous());
	}
	Out.Add(Defaults->FlashTexture.LoadSynchronous());
	Out.Add(Defaults->TextFont.LoadSynchronous());
}

TSharedRef<SWidget> UWasamiCollectablesWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		// Nothing here takes the mouse: the banner lies over the game for its 2 s.
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		FWidgetTransform CanvasTransform;
		CanvasTransform.Scale = FVector2D(CollectablesRestScale);
		Canvas->SetRenderTransform(CanvasTransform);
		Canvas->SetRenderOpacity(CollectablesRestOpacity);
		WidgetTree->RootWidget = Canvas;

		// Image_297: the red flash, a little past the screen's edges on every side, under the rest.
		Flash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_297"));
		FSlateBrush FlashBrush;
		FlashBrush.SetResourceObject(FlashTexture.LoadSynchronous());
		FlashBrush.TintColor = FSlateColor(CollectablesFlashTint);
		Flash->SetBrush(FlashBrush);
		Flash->SetRenderOpacity(0.f);
		CollectablesPlace(Canvas, Flash, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(-24.90087890625f, -22.306304931640625f, -19.5435791015625f, -17.6937255859375f), FVector2D::ZeroVector, false);

		// Image_89: the extras frame (696 × 204) on the screen's middle.
		Frame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_89"));
		Frame->SetBrush(CollectablesBrush(FrameTexture.LoadSynchronous(), FVector2D(696.f, 204.f)));
		CollectablesPlace(Canvas, Frame, FAnchors(0.5f), FMargin(0.f, 0.f, 100.f, 40.f), FVector2D(0.5f, 0.5f), true);

		// Image_249: the picture (159 × 145) centred 185 px left of the middle.
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_249"));
		Icon->SetBrush(CollectablesBrush(IconTexture.LoadSynchronous(), FVector2D(159.f, 145.f)));
		CollectablesPlace(Canvas, Icon, FAnchors(0.5f), FMargin(-185.06370544433594f, 0.f, 100.f, 40.f), FVector2D(0.5f, 0.5f), true);

		// TextBlock_150: the words from left of the middle rightwards, white, centred on the middle's height.
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_150"));
		TextBlock->SetText(FText::FromString(TEXT("NEW ART UNLOCKED!")));
		FSlateFontInfo Font;
		Font.FontObject = TextFont.LoadSynchronous();
		Font.TypefaceFontName = TEXT("Default");
		Font.Size = TextSize;
		TextBlock->SetFont(Font);
		CollectablesPlace(Canvas, TextBlock, FAnchors(0.5f), TextOffsets, FVector2D(0.f, 0.5f), true);
	}
	return Super::RebuildWidget();
}

void UWasamiCollectablesWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiCollectablesWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiCollectablesWidget::Begin()
{
	// Construct: PlayAnimation(NewAnimation_1), then the widget's own part, then Delay 2 → RemoveFromParent.
	Elapsed = 0.f;
	bFinished = false;
	ApplyAnimation();
	ConstructContent();
}

void UWasamiCollectablesWidget::ConstructContent()
{
	// TextBlock_150.SetText(NEW EXTRAS UNLOCKED!); RandomIntegerInRange(0, 3) picks art_icon, diary_icon, sound_icon or
	// movie_icon, and Image_249.SetBrushFromTexture(it, False) keeps the brush's 159 × 145.
	if (TextBlock)
	{
		TextBlock->SetText(UnlockedText());
	}
	IconIndex = FMath::RandRange(0, 3);
	if (Icon && RandomIcons.IsValidIndex(IconIndex))
	{
		Icon->SetBrushFromTexture(RandomIcons[IconIndex].LoadSynchronous(), false);
	}
}

void UWasamiCollectablesWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	ApplyAnimation();
	if (Elapsed >= RemoveDelay)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

void UWasamiCollectablesWidget::ApplyAnimation()
{
	if (Canvas)
	{
		FWidgetTransform Transform = Canvas->GetRenderTransform();
		Transform.Scale = FVector2D(EvaluateScale(Elapsed));
		Canvas->SetRenderTransform(Transform);
		Canvas->SetRenderOpacity(EvaluateOpacity(Elapsed));
	}
	if (Flash)
	{
		Flash->SetRenderOpacity(EvaluateFlashOpacity(Elapsed));
	}
}

float UWasamiCollectablesWidget::EvaluateScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CollectablesScaleKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiCollectablesWidget::EvaluateOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CollectablesOpacityKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiCollectablesWidget::EvaluateFlashOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CollectablesFlashKeys);
	return Eval(Curve, Seconds, AnimLength);
}

UWasamiCollectablesSecretWidget::UWasamiCollectablesSecretWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The secret's tree: Image_249 is T_MysteryRoom, and TextBlock_150 is 18 and sits 129 px left of the middle, 4 px up.
	IconTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/T_MysteryRoom")));
	RandomIcons.Reset();
	TextSize = 18.f;
	TextOffsets = FMargin(-128.9609375f, -4.024012088775635f, 151.f, 40.f);
	MusicSound = TSoftObjectPtr<USoundBase>(
		WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/DD_LVL2_15_V1_Secret_Mystery_Room_120818")));
}

FText UWasamiCollectablesSecretWidget::FoundText()
{
	return FText::FromString(TEXT("YOU FOUND A MYSTERIOUS ROOM"));
}

UWasamiCollectablesSecretWidget* UWasamiCollectablesSecretWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiCollectablesSecretWidget* Widget = CreateWidget<UWasamiCollectablesSecretWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(SecretZOrder);
	}
	return Widget;
}

void UWasamiCollectablesSecretWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const UWasamiCollectablesSecretWidget* Defaults = GetDefault<UWasamiCollectablesSecretWidget>();
	Out.Add(Defaults->FrameTexture.LoadSynchronous());
	Out.Add(Defaults->IconTexture.LoadSynchronous());
	Out.Add(Defaults->FlashTexture.LoadSynchronous());
	Out.Add(Defaults->TextFont.LoadSynchronous());
	Out.Add(Defaults->MusicSound.LoadSynchronous());
}

void UWasamiCollectablesSecretWidget::ConstructContent()
{
	// Construct: PlaySound2D(DD_LVL2_15_V1_Secret_Mystery_Room_120818, 0.5, 1) before the animation, then
	// TextBlock_150.SetText(YOU FOUND A MYSTERIOUS ROOM). The picture stays the tree's T_MysteryRoom.
	if (GetWorld())
	{
		if (USoundBase* Music = MusicSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Music, MusicVolume, 1.f);
		}
	}
	if (UTextBlock* Text = GetTextBlock())
	{
		Text->SetText(FoundText());
	}
}
