#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WasamiInteractable.generated.h"

UINTERFACE(BlueprintType)
class UWasamiInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Something the player uses by looking at it and clicking, after Dark Deception's BP_InteractInterface
 * (pak_reference_2). The player's left click (Interact (Secondary)) traces 200 cm ahead of the camera and calls
 * InteractWithObject on the actor it hits; letting go calls StopInteractWithObject on that same actor. The hand on the
 * screen shows while the trace hits a component tagged interact (the player's tick). The original's Use belongs to
 * held items, which this game has none of.
 */
class WASAMI_DECEPTION_API IWasamiInteractable
{
	GENERATED_BODY()

public:
	/** InteractWithObject: Interactee is the player. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
	void InteractWithObject(AActor* Interactee);

	/** StopInteractWithObject: the click let go. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
	void StopInteractWithObject();

protected:
	virtual void InteractWithObject_Implementation(AActor* Interactee) {}
	virtual void StopInteractWithObject_Implementation() {}
};
