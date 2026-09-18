#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiDoubleDoors.generated.h"

class UBoxComponent;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiDoubleDoorsOpenSignature);

/**
 * Dark Deception's BP_06_DoubleDoors (pak_reference_2's Blueprints/06_Hospital), the hospital's swinging double doors:
 * two doors hinged at the outer edges of a 400 cm doorway. A character walking into the box in front of them (or
 * behind) swings both away from it by Open Amount with a sound, and they swing shut when a character walks out of the
 * box around both sides while the player is not in it. Locked, walking in only rattles them (Locked Sound, at most once
 * in 2 s). The zones lock and open the ones they name (Zone 1's lift doors, the tunnel's doors) through Lock, Unlock,
 * Open Front and Force Close.
 *
 * The meshes are not set by the class (assets under /Game/DD are never loaded from a constructor, WasamiAssets.h): the
 * level build places the doors with the original's meshes and materials on the two door components.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiDoubleDoors : public AActor
{
	GENERATED_BODY()

public:
	AWasamiDoubleDoors();

	/** Open: a side opened (walked into or Open Front). */
	UPROPERTY(BlueprintAssignable, Category = "Double Doors")
	FWasamiDoubleDoorsOpenSignature OnOpen;

	/** Open Amount: how far each door swings, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Double Doors")
	float OpenAmount = 90.f;

	/** bLocked: walking into a box only plays Locked Sound. The zones also write it directly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Double Doors")
	bool bLocked = false;

	/**
	 * Lock: bLocked, then what walking out of Leave does, for the character that last walked out of it (none yet: nothing
	 * more). Locked, the player no longer counts as in Leave, so the doors swing shut if that was a character.
	 */
	UFUNCTION(BlueprintCallable, Category = "Double Doors")
	void Lock();

	/** Unlock: not bLocked, then what walking into the front box does, for the character that last walked into it. */
	UFUNCTION(BlueprintCallable, Category = "Double Doors")
	void Unlock();

	/** Open Front: what walking into the front box does, without the character (locked: Locked Sound). */
	UFUNCTION(BlueprintCallable, Category = "Double Doors")
	void OpenFront();

	/** Force Close: an open side swings shut. */
	UFUNCTION(BlueprintCallable, Category = "Double Doors")
	void ForceClose();

	/** Update Animation Speed: both swings' play rate. */
	UFUNCTION(BlueprintCallable, Category = "Double Doors")
	void UpdateAnimationSpeed(float Speed);

	/**
	 * An actor walked into the front box, into the back box, or out of Leave (the original's three bound events). The
	 * boxes' overlaps call them; the tests call them directly.
	 */
	void NotifyFrontEnter(AActor* Other);
	void NotifyBackEnter(AActor* Other);
	void NotifyLeave(AActor* Other);

	/** AnySideOpen. */
	bool AnySideOpen() const { return bOpenFront || bOpenBack; }
	bool IsOpenFront() const { return bOpenFront; }
	bool IsOpenBack() const { return bOpenBack; }

	/** Whether a swing is running (Timeline_0 opening, Timeline_1 closing). */
	bool IsOpening() const { return OpenTimeline.bPlaying; }
	bool IsClosing() const { return CloseTimeline.bPlaying; }

	/** Locked Sound's DoOnce is closed (it played less than 2 s ago). */
	bool IsLockedSoundBlocked() const { return bLockedSoundClosed; }

	/** The timelines' Float track (0 → 1 in 1 s, past 1 at 0.69 s) at Seconds. */
	static float EvaluateSwing(float Seconds);

	/** The timelines' length, the closing one's Sound key, and Locked Sound's Delay. */
	static constexpr float SwingLength = 1.f;
	static constexpr float CloseSoundTime = 0.592170238494873f;
	static constexpr float LockedSoundDelay = 2.f;

	/** StaticMesh (hospital_entrance_walkway_doubledoor2 at x −200) and StaticMesh1 (doubledoor1 at x +200). */
	UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
	UStaticMeshComponent* GetStaticMesh1() const { return StaticMesh1; }
	UBoxComponent* GetFrontEnter() const { return FrontEnter; }
	UBoxComponent* GetBackEnter() const { return BackEnter; }
	UBoxComponent* GetLeave() const { return Leave; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Double Doors")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** StaticMesh: the door at x −200, which swings by +Open Amount toward the back when opened from the front. */
	UPROPERTY(VisibleAnywhere, Category = "Double Doors")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** StaticMesh1: the door at x +200, which swings the other way round (−Open Amount). */
	UPROPERTY(VisibleAnywhere, Category = "Double Doors")
	TObjectPtr<UStaticMeshComponent> StaticMesh1;

	/** FrontEnter, BackEnter: the boxes in front of the doors (y −150) and behind them (y +150). */
	UPROPERTY(VisibleAnywhere, Category = "Double Doors")
	TObjectPtr<UBoxComponent> FrontEnter;

	UPROPERTY(VisibleAnywhere, Category = "Double Doors")
	TObjectPtr<UBoxComponent> BackEnter;

	/** Leave: the box over both sides, which a character walks out of to let the doors shut. */
	UPROPERTY(VisibleAnywhere, Category = "Double Doors")
	TObjectPtr<UBoxComponent> Leave;

	/** SFX_06_DoubleDoor_Open as they open and SFX_06_DoubleDoor_Close into the swing shut, through MonkeyAttenuation. */
	UPROPERTY(EditAnywhere, Category = "Double Doors|Assets")
	TSoftObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, Category = "Double Doors|Assets")
	TSoftObjectPtr<USoundBase> CloseSound;

	UPROPERTY(EditAnywhere, Category = "Double Doors|Assets")
	TSoftObjectPtr<USoundAttenuation> SwingAttenuation;

	/** Locked_Door (a SoundCue of two rattles) through 01_Lobby_Attenuation. */
	UPROPERTY(EditAnywhere, Category = "Double Doors|Assets")
	TSoftObjectPtr<USoundBase> LockedDoorSound;

	UPROPERTY(EditAnywhere, Category = "Double Doors|Assets")
	TSoftObjectPtr<USoundAttenuation> LockedAttenuation;

private:
	/** A Timeline (UE's FTimeline, playing forwards only): its position and whether it plays. */
	struct FSwing
	{
		float Position = 0.f;
		bool bPlaying = false;
	};

	UFUNCTION()
	void OnFrontEnterBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBackEnterBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnLeaveEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** The front box's event from its bLocked check on (@550): Open Front. */
	void OpenFromFront();

	/** Leave's event (@889) for that actor. */
	void CloseFromLeave(AActor* Other);

	/** Leave's event from AnySideOpen on (@1054): Force Close. */
	void CloseOpenSide();

	/** Open Door: the opening sound and Timeline_0 from the start. */
	void OpenDoor();

	/** Close Door: both doors' collision on and Timeline_1 from the start. */
	void CloseDoor();

	/** Locked Sound: a DoOnce reset 2 s on. */
	void LockedSound();

	/** Player Overlapping?: the player is in Leave (never while locked). */
	bool IsPlayerOverlapping() const;

	/** Update Func: the swing's value at this point of the opening (Timeline_0) or closing (Timeline_1). */
	void UpdateFunc(bool bOpen, float Alpha);

	/** Apply Update Values: StaticMesh at that yaw, StaticMesh1 at the opposite. */
	void ApplyUpdateValues(float Value);

	void PlayFromStart(FSwing& Swing, bool bOpen);
	void TickSwing(FSwing& Swing, bool bOpen, float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedOpenSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedCloseSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> LoadedSwingAttenuation;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedLockedDoorSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> LoadedLockedAttenuation;

	/** The actors the front box's and Leave's events last had (Unlock and Lock run those events again for them). */
	TWeakObjectPtr<AActor> LastFrontEnter;
	TWeakObjectPtr<AActor> LastLeave;

	FSwing OpenTimeline;
	FSwing CloseTimeline;
	float PlayRate = 1.f;

	bool bOpenFront = false;
	bool bOpenBack = false;
	/** Animation: the doors swing (or swung) as from the front, toward the back. */
	bool bAnimation = false;

	bool bLockedSoundClosed = false;
	FTimerHandle LockedSoundTimer;
};
