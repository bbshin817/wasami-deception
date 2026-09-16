#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiTabletWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UFont;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextBlock;
class UTexture2D;

/**
 * The tablet's screen, after Dark Deception's UI/Tablet/UMG_Tablet (pak_reference): 714 × 864 px of background with
 * the shard count, the two power sockets, the minimap with the player's mark, the objective band and the "Z" of the
 * map's resize. The tree is built here rather than in a widget Blueprint so every position stays the original's px.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiTabletWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiTabletWidget(const FObjectInitializer& ObjectInitializer);

	/** The original's screen in px: everything below is placed inside this. */
	static constexpr float ScreenWidth = 714.f;
	static constexpr float ScreenHeight = 864.f;

	/** How many shards are left in the level (the original counts the BP_Shard actors). */
	void SetShardCount(int32 Count);

	/** The game mode's Current Objective; the band shows it in upper case, as the original's binding does. */
	void SetObjective(const FText& Objective);

	/** A power socket's MM_Powers `Percent`: 1 when it can be used, 0 when it cannot. */
	void SetPowerCharge(bool bLeftSocket, float Percent);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildScreen(UCanvasPanel* Root);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ShardCountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FlashImage;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LeftPower;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RightPower;

	/** tablet_screen_bg: the background with the bar and the map's frame drawn on it. */
	UPROPERTY()
	TObjectPtr<UTexture2D> BackgroundTexture;

	/** tablet_map_player: the mark at the middle of the map. */
	UPROPERTY()
	TObjectPtr<UTexture2D> PlayerMarkTexture;

	/** T_Vignette: the purple flash over the map when a shard is collected (Count Shake's Image_41). */
	UPROPERTY()
	TObjectPtr<UTexture2D> VignetteTexture;

	/** M_DD_MapScreen: the scene capture's render target, as the original's M_NewMap. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> MapMaterial;

	/** MM_Powers_Inst_Teleport / MM_Powers_SpeedBoost: the left and right sockets (the original's power order). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> LeftPowerMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> RightPowerMaterial;

	/** helvetica-neue-bold_Font: every text of the original's tablet. */
	UPROPERTY()
	TObjectPtr<UFont> ScreenFont;

	int32 LastShardCount = -1;
	float LastCharge[2] = {-1.f, -1.f};
};
