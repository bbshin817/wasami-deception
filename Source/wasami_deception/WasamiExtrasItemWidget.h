#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiExtrasItemWidget.generated.h"

class UButton;
class UImage;
class UScaleBox;
class USoundBase;
class UTexture2D;
class UWasamiSaveGame;

/**
 * A picture on the extras screen's Art Gallery, after Dark Deception's UI/Main/TitleScreen/UMG_Extras_Extra
 * (pak_reference_2): a 150 square button framed in ring_altar_power_equipped_frame, grey until hovered and white while
 * it is, with the picture over it filling the square but for 5 px each side (a scale box that fills and crops).
 * Construct reads the save: a picture whose ID is in Extras_Art shows Art and can be pressed, any other shows the lock
 * and lets the pointer through. Pressing an unlocked one plays UI_Select_V3 and opens it large
 * (UWasamiMaximizePictureWidget) with Text.
 *
 * The tree is built here as in the original, slot for slot. The extras screen sets ID, Art and Text (and may hand over
 * the save it read) before it adds the picture. The original's ClickedEvent is left out: nothing broadcasts it.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiExtrasItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiExtrasItemWidget(const FObjectInitializer& ObjectInitializer);

	/** ID: the extra's place in its list (Extras_Art's for a picture). */
	UPROPERTY(Transient)
	int32 ID = 0;

	/** Art: the picture, which Image_1 shows once it is unlocked and the large view shows. */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Art;

	/** Text: the words over the large view (mostly who drew it). */
	UPROPERTY(Transient)
	FText Text;

	/**
	 * The save the lock is read from. Construct reads SaveSlotName's when none is given (the original reads SaveSlot in
	 * each extra's Construct), and takes a new one when the slot is empty (the original's title makes SaveSlot, in the
	 * game mode's Check For Save, before EXTRAS can be pressed).
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSaveGame> Save;

	FString SaveSlotName;

	/** The Z order the large view is added at, and the select sound's pitch on a press. */
	static constexpr int32 MaximizeZOrder = 3;
	static constexpr float SelectPitch = 1.25f;

	/** Construct's Normal tint for the frame (grey); Hovered's is white. */
	static constexpr float NormalGrey = 0.4531f;

	/**
	 * Construct (@252): Button_104 restyled (Normal: the frame in grey; Hovered: the frame in white; Pressed: a plain white
	 * square), then with the save Image_1 from Art (unlocked) or the lock, and Button_104 Visible or HitTestInvisible.
	 * NativeConstruct calls it (the tests call it alone, without a world).
	 */
	void Begin();

	/**
	 * The click (@2511): an unlocked extra plays UI_Select_V3 (1, 1.25) and opens large. Returns whether it was unlocked
	 * (the large view needs a player controller besides).
	 */
	bool Press();

	/** Whether the save has this extra unlocked (Begin reads the save). */
	bool IsUnlocked() const;

	/** Button_104, Image_1 and ScaleBox_0, for the tests. */
	UButton* GetButton() const { return Button; }
	UImage* GetImage() const { return Image; }
	UScaleBox* GetScaleBox() const { return ScaleBox; }

	/** ring_altar_power_equipped_frame (the button's frame), locked (the lock) and the engine's WhiteSquareTexture. */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> FrameTexture;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> LockedTexture;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> WhiteTexture;

	/** UI_Select_V3. */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	/** Button_104's square slot and its Normal brush's size (150 and 150; the video's are 250 and 300). */
	virtual float GetSlotSize() const { return 150.f; }
	virtual float GetNormalSize() const { return 150.f; }

	/** Whether Read has the extra unlocked: Array_Contains(Extras_Art, ID). */
	virtual bool IsUnlockedIn(const UWasamiSaveGame& Read) const;

	/** The picture an unlocked extra shows (Art). */
	virtual UTexture2D* GetUnlockedTexture() const { return Art; }

	/** Create(UMG_MaximizePicture) with Texture and Text set, AddToViewport(3). */
	virtual void Open();

private:
	UFUNCTION()
	void OnButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> ScaleBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;
};

/**
 * A movie on the extras screen's Movies, after UMG_Extras_Extra_Video (pak_reference_2): the picture's button at 250 (its
 * Normal brush 300), whose PreConstruct and Construct run alike. It is unlocked by Extras_Movies[ID].Unlocked? (and shows
 * that entry's Image), which nothing in the hospital sets: BP_Collectable's Unlock has no case for a movie, so this
 * game's save keeps no Extras_Movies and every movie stays locked. The large view it would open (UMG_MaximizeVideo) is
 * not made. TODO(仮): the movies are to be decided with the user.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiExtrasVideoWidget : public UWasamiExtrasItemWidget
{
	GENERATED_BODY()

protected:
	virtual float GetSlotSize() const override { return 250.f; }
	virtual float GetNormalSize() const override { return 300.f; }
	virtual bool IsUnlockedIn(const UWasamiSaveGame& Read) const override { return false; }
	virtual UTexture2D* GetUnlockedTexture() const override { return nullptr; }
	virtual void Open() override {}
};
