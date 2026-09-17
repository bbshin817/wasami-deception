#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WasamiTelekinesisInterface.generated.h"

UINTERFACE(BlueprintType)
class UWasamiTelekinesisInterface : public UInterface
{
	GENERATED_BODY()
};

/** What the telekinesis pulls, after the original's DD_TelekinesisInterface (pak_reference_2; BP_Shard implements it). */
class WASAMI_DECEPTION_API IWasamiTelekinesisInterface
{
	GENERATED_BODY()

public:
	/** Activate: the telekinesis reached this; a shard flies to the player and is collected. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Telekinesis")
	void Activate();

protected:
	virtual void Activate_Implementation() {}
};
