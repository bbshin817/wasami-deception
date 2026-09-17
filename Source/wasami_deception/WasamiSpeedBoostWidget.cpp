#include "WasamiSpeedBoostWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "WasamiAssets.h"

namespace
{
	// UMG_SpeedBoost: both images fill the canvas, tinted red, scaled up about their middle.
	const FLinearColor ImageColour(1.f, 0.f, 0.f, 1.f);
	constexpr float LinesScale = 1.25f;
	constexpr float VignetteScale = 1.2f;
	// Tick: the opacities follow the player's speed from 0 to 900 cm/s.
	constexpr float FullSpeed = 900.f;
	constexpr float LinesMaxOpacity = 0.15f;
	constexpr float VignetteMaxOpacity = 0.5f;

	UImage* AddFill(UWidgetTree* Tree, UCanvasPanel* Panel, const TCHAR* Name, float Scale)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetColorAndOpacity(ImageColour);
		Image->SetRenderScale(FVector2D(Scale));
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Image);
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
		return Image;
	}
}

UWasamiSpeedBoostWidget::UWasamiSpeedBoostWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LinesMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/Powers/M_Speedlines")));
	VignetteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Streaks/T_VignetteNew")));
}

void UWasamiSpeedBoostWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const UWasamiSpeedBoostWidget* Defaults = GetDefault<UWasamiSpeedBoostWidget>();
	Out.Add(Defaults->LinesMaterial.LoadSynchronous());
	Out.Add(Defaults->VignetteTexture.LoadSynchronous());
}

TSharedRef<SWidget> UWasamiSpeedBoostWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		// Lines under Image_72, as the canvas' slots are ordered.
		Lines = AddFill(WidgetTree, Root, TEXT("Lines"), LinesScale);
		Lines->SetBrushFromMaterial(LinesMaterial.LoadSynchronous());
		Vignette = AddFill(WidgetTree, Root, TEXT("Image_72"), VignetteScale);
		Vignette->SetBrushFromTexture(VignetteTexture.LoadSynchronous(), false);
	}
	return Super::RebuildWidget();
}

void UWasamiSpeedBoostWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Tick → Delay(0.001) → the opacities: the delay started on one tick runs out before the next one.
	if (bOpacityDelayed && Lines && Vignette)
	{
		const APawn* Player = GetOwningPlayerPawn();
		const float Speed = Player ? static_cast<float>(Player->GetVelocity().Size()) : 0.f;
		Lines->SetOpacity(FMath::GetMappedRangeValueClamped(FVector2f(0.f, FullSpeed), FVector2f(0.f, LinesMaxOpacity), Speed));
		Vignette->SetOpacity(FMath::GetMappedRangeValueClamped(FVector2f(0.f, FullSpeed), FVector2f(0.f, VignetteMaxOpacity), Speed));
	}
	bOpacityDelayed = true;
}
