#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../WasamiInteractable.h"
#include "WasamiTestInteractable.generated.h"

class UBoxComponent;

/**
 * Something to use for the interact tests: a 100 cm box that blocks everything, its component tagged interact, which
 * counts the calls it is sent.
 */
UCLASS(NotBlueprintable)
class WASAMI_DECEPTION_API AWasamiTestInteractable : public AActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiTestInteractable();

	UBoxComponent* GetBox() const { return Box; }

	/** How many InteractWithObject calls came, and the last one's Interactee. */
	int32 InteractCount = 0;
	TWeakObjectPtr<AActor> LastInteractee;

	/** How many StopInteractWithObject calls came. */
	int32 StopCount = 0;

protected:
	virtual void InteractWithObject_Implementation(AActor* Interactee) override;
	virtual void StopInteractWithObject_Implementation() override { ++StopCount; }

	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<UBoxComponent> Box;
};
