#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiTelepathyPower.generated.h"

class AWasamiTelepathyTracker;

/**
 * Telepathy, after Dark Deception's BP_Telepathy (pak_reference_2). The power spawns it at the world's origin with the
 * upgrade level's Time. At once and then every 0.8 s it puts a tracker (AWasamiTelepathyTracker) on each enemy of the
 * level that implements the enemy interface, does not answer No Telepathy and has none yet, whatever its distance, so
 * enemies that appear later are found too. After Time it removes every tracker in the world and destroys itself. The
 * rest of the telepathy (the sounds, the shake, the gauge, the cooldown) is the power component's.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTelepathyPower : public AActor
{
	GENERATED_BODY()

public:
	AWasamiTelepathyPower();

	/** Update Targets: a tracker on each enemy that should have one and has none. Returns how many were spawned. */
	UFUNCTION(BlueprintCallable, Category = "Telepathy")
	int32 UpdateTargets();

	/** Finish: stops looking for enemies, removes every tracker in the world, and destroys this. */
	UFUNCTION(BlueprintCallable, Category = "Telepathy")
	void Finish();

	/** How long the telepathy lasts (s); the power sets it before the spawn finishes. 0 never finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telepathy", meta = (ExposeOnSpawn = "true"))
	float Time = 0.f;

	/** BP_TelepathyTracker. */
	UPROPERTY(EditAnywhere, Category = "Telepathy")
	TSubclassOf<AWasamiTelepathyTracker> TrackerClass;

	/** Actors With Tracker: every enemy a tracker was put on. */
	const TArray<TObjectPtr<AActor>>& GetActorsWithTracker() const { return ActorsWithTracker; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telepathy")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	void UpdateTargetsOnTimer() { UpdateTargets(); }

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> ActorsWithTracker;

	FTimerHandle UpdateTimer;
	FTimerHandle FinishTimer;
};
