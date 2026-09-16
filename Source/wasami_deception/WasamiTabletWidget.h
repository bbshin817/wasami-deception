#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiPowerTypes.h"
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
 * The sockets follow the latest version's UMG_TabletPowers (pak_reference_2): six power icons, shown by what each
 * socket points at.
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

	/** The count the screen shows (0 until one is set). */
	int32 GetShardCount() const { return FMath::Max(LastShardCount, 0); }

	/**
	 * Count Shake (a shard collected): the count jolts and grows, and the map flashes purple, at twice the animation's
	 * speed, from its start each time. At its end the count's transform goes back to what it was before (the transform
	 * section restores its state); the flash stays out.
	 */
	void PlayCountShake();

	/** The game mode's Current Objective; the band shows it in upper case, as the original's binding does. */
	void SetObjective(const FText& Objective);

	/** Both sockets, hidden when no power is unlocked (UMG_TabletPowers' Check). */
	void SetPowersVisible(bool bVisible);

	/** Update Powers: the icon each socket shows (None shows nothing). */
	void ShowSocketPowers(EWasamiPower Left, EWasamiPower Right);

	/** A power icon's MM_Powers `Percent` (1 = full); a socket showing that power shows it. */
	void SetPowerPercent(EWasamiPower Power, float Percent);

	/** Use Left / Use Right: the socket swells to 1.25 and settles back over 0.5 s, from the start each time. */
	void BounceSocket(bool bLeft);

	/** Moves the sockets' bounce and the count shake on by DeltaSeconds. */
	void TickAnimations(float DeltaSeconds);

	/** The bounce's scale at Seconds into it (1 before and after). */
	static float EvaluateSocketBounce(float Seconds);

	/** Count Shake's length and speed: it ends at its tick 12000 and PlayAnimation plays it at 2. */
	static constexpr float CountShakeLength = 0.2f;
	static constexpr float CountShakeSpeed = 2.f;

	/** Count Shake at Seconds of the animation: the count's translation (px) and scale, and the flash's alpha. */
	static FVector2D EvaluateCountShakeTranslation(float Seconds);
	static float EvaluateCountShakeScale(float Seconds);
	static float EvaluateCountShakeFlash(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** tablet_screen_bg: the background with the bar and the map's frame drawn on it. */
	UPROPERTY(EditAnywhere, Category = "Tablet|Assets")
	TSoftObjectPtr<UTexture2D> BackgroundTexture;

	/** tablet_map_player: the mark at the middle of the map. */
	UPROPERTY(EditAnywhere, Category = "Tablet|Assets")
	TSoftObjectPtr<UTexture2D> PlayerMarkTexture;

	/** T_Vignette: the purple flash over the map when a shard is collected (Count Shake's Image_41). */
	UPROPERTY(EditAnywhere, Category = "Tablet|Assets")
	TSoftObjectPtr<UTexture2D> VignetteTexture;

	/** M_DD_MapScreen: the scene capture's render target, as the original's M_NewMap. */
	UPROPERTY(EditAnywhere, Category = "Tablet|Assets")
	TSoftObjectPtr<UMaterialInterface> MapMaterial;

	/** The MM_Powers instances of the six powers, in EWasamiPower order. */
	UPROPERTY(EditAnywhere, Category = "Tablet|Assets")
	TArray<TSoftObjectPtr<UMaterialInterface>> PowerMaterials;

	/** helvetica-neue-bold_Font: every text of the original's tablet. */
	UPROPERTY(EditAnywhere, Category = "Tablet|Assets")
	TSoftObjectPtr<UFont> ScreenFont;

private:
	void BuildScreen(UCanvasPanel* Root);
	void ApplyCountShake();
	UImage* Socket(bool bLeft) const { return bLeft ? LeftSocket : RightSocket; }

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ShardCountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FlashImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> LeftSocket;

	UPROPERTY(Transient)
	TObjectPtr<UImage> RightSocket;

	/** One dynamic instance per power, in EWasamiPower order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PowerIcons;

	int32 LastShardCount = -1;
	bool bPowersVisible = true;
	float LastPercent[WasamiPowerCount] = {-1.f, -1.f, -1.f, -1.f, -1.f, -1.f};
	/** The power each socket shows, as an int so that nothing matches before the first call. */
	int32 ShownPower[2] = {-1, -1};
	/** Seconds into each socket's bounce; negative when it is not playing. */
	float BounceTime[2] = {-1.f, -1.f};
	/** Seconds into Count Shake's animation; negative when it is not playing. */
	float CountShakeTime = -1.f;
	/** The count's transform before Count Shake began, which its end restores. */
	FWidgetTransform CountRestTransform;
};
