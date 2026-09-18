#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WasamiTestListener.generated.h"

/** For the tests: counts the calls of Hear, which a parameterless dynamic delegate can be bound to. */
UCLASS(NotBlueprintable)
class WASAMI_DECEPTION_API UWasamiTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void Hear() { ++Count; }

	int32 Count = 0;
};
