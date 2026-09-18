#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiDoorBreak.generated.h"

class AWasamiPlayerCharacter;
class UBoxComponent;
class USoundAttenuation;
class USoundBase;
class UWasamiSwitchboxWidget;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiDoorBreakFinishedSignature);

/**
 * Dark Deception's BP_06_Hospital_DoorBreak (pak_reference_2's Blueprints/06_Hospital), a lock the player picks by
 * pressing Interact (F) over and over: its box (100 × 150 × 100 at half scale) wakes when the zone calls Enable Switch;
 * standing in it shows the lock (UMG_07_Boss_Switchbox in screen space, UWasamiSwitchboxWidget) and each press fills
 * the ring by Progress Speed with a lockpicking click; walking out hides the lock and empties it. When it fills, the
 * lock gives (two sounds), the box goes, Finished Event fires, and the lock is taken away 1 s on. The zones place one at
 * each door they break open (Zone 1's lift, Zone 2's cell) and listen to Finished Event.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiDoorBreak : public AActor
{
	GENERATED_BODY()

public:
	AWasamiDoorBreak();

	/** Finished Event. */
	UPROPERTY(BlueprintAssignable, Category = "Door Break")
	FWasamiDoorBreakFinishedSignature FinishedEvent;

	/** Progress Speed: what one press adds to the lock's 100 (the class's 5; the level's actors set their own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Break")
	float ProgressSpeed = 5.f;

	/** Enable Switch: the box starts noticing the player. */
	UFUNCTION(BlueprintCallable, Category = "Door Break")
	void EnableSwitch();

	/** Disable: the lock and the box taken away. */
	UFUNCTION(BlueprintCallable, Category = "Door Break")
	void Disable();

	/** Interact (F) pressed: in range, the lock's Interact Event and the lockpicking sound at the actor. */
	UFUNCTION(BlueprintCallable, Category = "Door Break")
	void Interact();

	/**
	 * The player walked into the box (bBegin) or out: in range and the lock shown, or out of range, the lock hidden and
	 * emptied. The box's overlaps call it for the player only; the tests call it directly.
	 */
	void NotifyPlayerOverlap(bool bBegin);

	/** Finished's Delay before the lock is taken away. */
	static constexpr float RemoveWidgetDelay = 1.f;

	bool IsInRange() const { return bInRange; }
	UBoxComponent* GetBox() const { return Box1; }
	UWidgetComponent* GetWidget() const { return Widget; }
	UWasamiSwitchboxWidget* GetUIWidget() const { return UIWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Door Break")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Widget: the lock, in screen space (64 × 64), hidden until the player is in range. */
	UPROPERTY(VisibleAnywhere, Category = "Door Break")
	TObjectPtr<UWidgetComponent> Widget;

	/** Box1: UE's default overlap box, 100 × 150 × 100 at half scale, not generating overlaps until Enable Switch. */
	UPROPERTY(VisibleAnywhere, Category = "Door Break")
	TObjectPtr<UBoxComponent> Box1;

	/** SFX_06_Lockpicking (a SoundCue of four clicks and a silence) at each press, through 01_Lobby_Attenuation. */
	UPROPERTY(EditAnywhere, Category = "Door Break|Assets")
	TSoftObjectPtr<USoundBase> LockpickingSound;

	UPROPERTY(EditAnywhere, Category = "Door Break|Assets")
	TSoftObjectPtr<USoundAttenuation> LockpickingAttenuation;

	/** Press_Slam_02 and SFX_06_Lockpicked, 2D, as the lock gives. */
	UPROPERTY(EditAnywhere, Category = "Door Break|Assets")
	TSoftObjectPtr<USoundBase> SlamSound;

	UPROPERTY(EditAnywhere, Category = "Door Break|Assets")
	TSoftObjectPtr<USoundBase> LockpickedSound;

private:
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** Finished: bound to the lock's Finished. */
	UFUNCTION()
	void Finished();

	void RemoveWidget();

	/** UI Widget: the widget component's UMG_07_Boss_Switchbox. */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSwitchboxWidget> UIWidget;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedLockpickingSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> LoadedLockpickingAttenuation;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedSlamSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedLockpickedSound;

	/** The player whose Interact the actor listens to (the original's actor takes the key itself). */
	TWeakObjectPtr<AWasamiPlayerCharacter> InteractSource;
	FDelegateHandle InteractHandle;

	FTimerHandle RemoveWidgetTimer;
	bool bInRange = false;
};
