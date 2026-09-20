#pragma once

#include "CoreMinimal.h"
#include "../WasamiBierceTalk.h"
#include "WasamiTestBierceTalk.generated.h"

/**
 * A talker whose component can be held busy: an automation world has no audio device, so nothing a Play starts there
 * ever reports itself as playing. bSpeaking stands in for that, and the wait loop can be walked through.
 */
UCLASS(NotBlueprintable)
class WASAMI_DECEPTION_API AWasamiTestBierceTalk : public AWasamiBierceTalk
{
	GENERATED_BODY()

public:
	/** What IsSpeaking answers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	bool bSpeaking = false;

protected:
	virtual bool IsSpeaking() const override { return bSpeaking; }
};
