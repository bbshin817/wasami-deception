#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiTriggerBox.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiTriggerSignature);

/**
 * Dark Deception's BP_TriggerBox_Base (pak_reference_2): a hidden box the zones' level Blueprints listen to. The player
 * walking in fires Trigger once, or, with End Overlap set, the player walking out does. The level places it with its
 * scale; the box is UE's default 32 cm half extent, overlapping pawns only.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTriggerBox : public AActor
{
	GENERATED_BODY()

public:
	AWasamiTriggerBox();

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	/**
	 * The player walked in (bBegin) or out: each way passes once (its own DoOnce), and fires Trigger when it is the way
	 * End Overlap asks for. The overlaps call it for the player only; the debug command and the tests call it directly.
	 */
	void NotifyPlayerOverlap(bool bBegin);

	/** Trigger. */
	UPROPERTY(BlueprintAssignable, Category = "Trigger")
	FWasamiTriggerSignature OnTrigger;

	/** End Overlap: fire when the player walks out instead of in (no trigger in the hospital sets it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	bool bEndOverlap = false;

	UBoxComponent* GetBox() const { return Box; }

	/** Whether Trigger has fired. */
	bool HasFired() const { return bFired; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Trigger")
	TObjectPtr<UBoxComponent> Box;

	/** The two DoOnce gates, walking in and walking out. */
	bool bBeginClosed = false;
	bool bEndClosed = false;
	bool bFired = false;
};
