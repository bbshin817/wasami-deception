#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiTelepathyTracker.generated.h"

class UWasamiTelepathyTrackerWidget;
class UWidgetComponent;

/**
 * One telepathy marker, after Dark Deception's BP_TelepathyTracker (pak_reference_2). The telepathy spawns one on each
 * enemy it finds; every tick it moves onto its enemy and sizes its marker by the distance to the player. The marker is a
 * screen-space widget component (UMG_TelepathyTracker), which the viewport draws over the world, so it shows through
 * walls. Remove stops the following, plays the marker's Disappear and destroys the tracker 0.5 s later; a tracker whose
 * enemy is gone removes itself.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTelepathyTracker : public AActor
{
	GENERATED_BODY()

public:
	AWasamiTelepathyTracker();

	virtual void Tick(float DeltaSeconds) override;

	/** The marker's scale at a distance (cm) from the player: 0.5 at 0 to 0.1 at 10000, unclamped (0 at 12500). */
	UFUNCTION(BlueprintPure, Category = "Telepathy")
	static float SizeForDistance(float Distance);

	/** Remove: stops following, plays the marker's Disappear, and destroys the tracker 0.5 s later. */
	UFUNCTION(BlueprintCallable, Category = "Telepathy")
	void Remove();

	/** The enemy followed (the telepathy sets it before the spawn finishes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telepathy", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<AActor> Actor;

	/** Whether the Gate before Update is open (from BeginPlay until Remove). */
	bool IsFollowing() const { return bGateOpen; }
	UWidgetComponent* GetWidget() const { return Widget; }
	UWasamiTelepathyTrackerWidget* GetWidgetReference() const { return WidgetReference; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telepathy")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Widget: UMG_TelepathyTracker in screen space, at the engine's default draw size and pivot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telepathy")
	TObjectPtr<UWidgetComponent> Widget;

private:
	/** Update: onto the enemy, and the marker's size; Remove when the enemy is gone. */
	void Update();
	void DestroyAfterRemove() { Destroy(); }

	/** Widget Reference: the component's UMG_TelepathyTracker. */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiTelepathyTrackerWidget> WidgetReference;

	/** The Gate, which starts closed. */
	bool bGateOpen = false;
	FTimerHandle RemoveTimer;
};
