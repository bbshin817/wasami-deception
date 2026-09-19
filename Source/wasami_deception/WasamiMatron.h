#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiViewcone.h"
#include "WasamiMatron.generated.h"

class UBoxComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * Zone 2's Matron over the miniboss corridor, after Dark Deception's BP_06_Matron_MiniBoss (pak_reference_2), as the big
 * boss Wasami (SK_WasamiBoss, UWasamiBossAnimInstance). She stands at her desk and never moves; she watches through two
 * view cones the level places, Long Cone and Short Cone. Zone 2's Activate MiniBoss Enemies calls Activate after the
 * sentries': her cones are hers (their owner) and start looking, and Switch runs every second, turning on the long cone
 * while the player is outside CloseArea (the strip before her desk; bMode) and the short one while inside, the other
 * off, only as that changes. Player Spotted (from either cone), once: both cones destroyed, Detected played (the
 * Detected montage, left on its last pose), bSpotted (her animation turns her to the player), and 1.0948 s on (the
 * montage's PlayMontageNotify) every sentry nurse is told Player Spotted, so that the six leap down and chase.
 *
 * Tagged Enemy (the original's CDO): what removes the enemies takes her too. Left out: the call for help her montage
 * plays at 0.111 s (Matron_ReinforcementCall_01; the enemies' voices are item 20's).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiMatron : public AActor, public IWasamiViewconeInterface
{
	GENERATED_BODY()

public:
	AWasamiMatron();

	/** Activate's looping timer on Switch. */
	static constexpr float SwitchRate = 1.f;
	/** The Detected montage's PlayMontageNotify, whose Notify Begin tells the sentries (s from the montage's start). */
	static constexpr float ReinforcementTime = 1.0948f;
	// TODO(仮): the boss's size. The placed Matron's long cone (her eyes, read so) is 676.26 cm up and her mesh's origin
	// 107.22 cm under the floor (the root's -50.22 and the mesh's -57); SK_WasamiBoss's head bone in Idle's first frame
	// is 128.2 cm up at scale 1 (dd_boss), so this scale puts it at the cone. The original draws SK_Matron at 5.
	static constexpr double EyeHeight = 676.2613525390625;
	static constexpr double MeshOriginHeight = -107.2183837890625;
	static constexpr double IdleHeadHeight = 128.2;
	static constexpr double MeshScale = (EyeHeight - MeshOriginHeight) / IdleHeadHeight;

	/** Activate: Switch every second from now on, and her cones hers and initialized. */
	UFUNCTION(BlueprintCallable, Category = "Matron")
	void Activate();

	/** Whether Activate has started Switch. */
	UFUNCTION(BlueprintPure, Category = "Matron")
	bool IsActivated() const;

	/**
	 * Switch: bMode, the player not in CloseArea; the long cone on and the short off as bMode turns true (DoOnce A), the
	 * short on and the long off as it turns false (DoOnce B), each reopening the other.
	 */
	void Switch();

	USkeletalMeshComponent* GetMesh() const { return SkeletalMesh; }
	UBoxComponent* GetCloseArea() const { return CloseArea; }

	/** Long Cone: her sight while the player is away (the level's BP_06_Miniboss_viewcone_Matron_Long). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matron")
	TObjectPtr<AWasamiViewcone> LongCone;

	/** Short Cone: her sight while the player is before her desk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matron")
	TObjectPtr<AWasamiViewcone> ShortCone;

	/** bMode: the player is not in CloseArea (the long cone's turn). Her animation's bAlert. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Matron")
	bool bMode = false;

	/** bSpotted: a cone saw the player. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Matron")
	bool bSpotted = false;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PlayerSpotted_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matron")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** The boss, drawn MeshScale times its size; no collision (the cones' traces pass her). The level places it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matron")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	/** CloseArea: overlapping Pawn only, and not on the navigation. The level places and sizes it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matron")
	TObjectPtr<UBoxComponent> CloseArea;

	UPROPERTY(EditDefaultsOnly, Category = "Matron")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;

private:
	/** The Detected montage's Notify Begin: Player Spotted on every sentry nurse. */
	void CallSentries();

	// Player Spotted's DoOnce, and Switch's two (the long cone's turn and the short cone's).
	bool bSpottedClosed = false;
	bool bLongTurnClosed = false;
	bool bShortTurnClosed = false;
	FTimerHandle SwitchTimer;
	FTimerHandle SentryTimer;
};
