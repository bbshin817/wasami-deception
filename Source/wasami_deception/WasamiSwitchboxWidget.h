#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiSwitchboxWidget.generated.h"

class UCanvasPanel;
class UFont;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiSwitchboxFinishedSignature);

/**
 * Dark Deception's UMG_07_Boss_Switchbox (pak_reference_2's Blueprints/07_FunPlace/Boss), the lock the hospital's door
 * breaks show over their doors (AWasamiDoorBreak): the key F under a red ring (M_04_UI_Radial_Red) that fills as it is
 * pressed, with two black rings behind it and two red sparks, hidden until the lock gives. Each Interact Event adds
 * Progress Speed to Progress (0..100), fills the ring to Progress / 100 and plays Interact or Interact_1 at random (the
 * key and the rings jolt); past 99 it fires Finished and plays Completed at twice its rate (the sparks flash and it all
 * fades), once. The animations keep their last values when they end, and while two play, the one started later writes
 * over the other.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSwitchboxWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiSwitchboxWidget(const FObjectInitializer& ObjectInitializer);

	virtual bool Initialize() override;

	/** Finished: Progress went past FinishAbove (once). */
	UPROPERTY(BlueprintAssignable, Category = "Switchbox")
	FWasamiSwitchboxFinishedSignature Finished;

	/** Progress, 0..100. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Switchbox")
	float Progress = 0.f;

	/** Progress Speed: what one Interact Event adds (the door break gives its own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Switchbox")
	float ProgressSpeed = 5.f;

	/** Interact Event: one press. */
	UFUNCTION(BlueprintCallable, Category = "Switchbox")
	void InteractEvent();

	/** Set Progress: the ring's Percentage (0..1), once Construct has made its material. */
	UFUNCTION(BlueprintCallable, Category = "Switchbox")
	void SetProgress(float Value);

	static constexpr float MaxProgress = 100.f;
	/** Progress above this finishes the lock. */
	static constexpr float FinishAbove = 99.f;
	/** The animations' last frames (s): Interact and Interact_1 end at tick 15000, Completed at 30001. */
	static constexpr float InteractLength = 0.25f;
	static constexpr float CompletedLength = static_cast<float>(30001. / 60000.);
	/** PlayAnimation's rates: 1 for the presses, 2 for Completed. */
	static constexpr float CompletedRate = 2.f;

	/** The tracks at a time (s) of their animation: Interact (bFirst) or Interact_1, and Completed. */
	static float EvaluateInteractKeyAngle(bool bFirst, float Seconds);
	static float EvaluateInteractKeyScale(float Seconds);
	static float EvaluateInteractRingsScale(float Seconds);
	static float EvaluateCompletedSparkOpacity(float Seconds);
	static float EvaluateCompletedSparkScaleX(float Seconds);
	static float EvaluateCompletedSpark2Scale(float Seconds);
	static float EvaluateCompletedRingsOpacity(float Seconds);
	static float EvaluateCompletedKeyOpacity(float Seconds);

	/** M_04_UI_Radial_Red (its parent M_UI_Radial is an estimate from the cooked shader). */
	UPROPERTY(EditAnywhere, Category = "Switchbox|Assets")
	TSoftObjectPtr<UMaterialInterface> RingMaterial;

	/** M_07_Spark and M_07_Spark2. */
	UPROPERTY(EditAnywhere, Category = "Switchbox|Assets")
	TSoftObjectPtr<UMaterialInterface> SparkMaterial;

	UPROPERTY(EditAnywhere, Category = "Switchbox|Assets")
	TSoftObjectPtr<UMaterialInterface> Spark2Material;

	/** helvetica-neue-bold_Font. */
	UPROPERTY(EditAnywhere, Category = "Switchbox|Assets")
	TSoftObjectPtr<UFont> KeyFont;

	bool HasFinished() const { return bFinished; }
	bool IsPlaying() const { return Players.Num() > 0; }
	UMaterialInstanceDynamic* GetRingMaterial() const { return Material; }
	UTextBlock* GetKey() const { return Key; }
	UCanvasPanel* GetRings() const { return Rings; }
	UImage* GetSpark() const { return Spark; }
	UImage* GetSpark2() const { return Spark2; }

	/** Moves the animations on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EAnimation : uint8
	{
		Interact,
		Interact1,
		Completed,
	};

	struct FPlayer
	{
		EAnimation Animation;
		float Time;
		float Rate;
	};

	/** PlayAnimation(Animation, 0, 1 loop, forward, Rate): from its start again if it plays; its first frame at once. */
	void Play(EAnimation Animation, float Rate);
	void Apply(const FPlayer& Player);

	/** CanvasPanel_0, the root. */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Root;

	/** TextBlock_62: F. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Key;

	/** CanvasPanel_107: the rings. */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Rings;

	/** circle: the ring that fills. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Circle;

	/** Image_33 and Image_55: the sparks. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Spark;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Spark2;

	/** Material: circle's dynamic material, made by Construct. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	/** The animations playing, in the order they started. */
	TArray<FPlayer> Players;

	bool bConstructed = false;
	bool bFinished = false;
};
