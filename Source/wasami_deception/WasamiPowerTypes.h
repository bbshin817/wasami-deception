#pragma once

#include "CoreMinimal.h"
#include "WasamiPowerTypes.generated.h"

/** The tablet's powers, in the order of Dark Deception's Enum_RingAltar_Skills (pak_reference_2). */
UENUM(BlueprintType)
enum class EWasamiPower : uint8
{
	SpeedBoost,
	Teleport,
	Telepathy,
	PrimalFear,
	Telekinesis,
	Vanish,
	None
};

/** How many powers there are (every EWasamiPower before None). */
inline constexpr int32 WasamiPowerCount = static_cast<int32>(EWasamiPower::None);

/** One of the unlocked powers, as the original's Struct_Power: which power, and whether it can be used right now. */
USTRUCT(BlueprintType)
struct FWasamiPowerSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	EWasamiPower Power = EWasamiPower::SpeedBoost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
	bool bAvailable = false;
};

/**
 * What the powers do at one upgrade level: the switches of BP_DD_PlayerCharacter over Get Power Upgrade Level
 * (pak_reference_2). Speeds and ranges in cm (/s), times in seconds.
 */
struct FWasamiPowerTuning
{
	float BoostSpeed;
	float BoostDuration;
	/** After the boost ends. The latest version's code is 1 s longer than its own upgrade table (the user's pick). */
	float BoostCooldown;
	float TeleportDistance;
	float TelepathyDuration;
	/** After the telepathy ends. */
	float TelepathyCooldown;
	float PrimalRange;
	/** From 0.06 s after the use. */
	float PrimalCooldown;
	float TelekinesisRange;
	/** From 0.06 s after the use. */
	float TelekinesisCooldown;
	/** After the vanish ends. */
	float VanishCooldown;

	/** The values at an upgrade level, 0 (not bought) to 5; out-of-range levels are clamped. */
	static const FWasamiPowerTuning& ForLevel(int32 Level);

	/** The teleport's cooldown after landing, and the vanish's length: the same at every level. */
	static constexpr float TeleportCooldown = 5.f;
	static constexpr float VanishDuration = 15.f;
	static constexpr int32 MaxLevel = 5;
};

/**
 * A power's gauge on the tablet: one of BP_Powers' six timelines (1 s long, a straight 0 → 1, played at 1 / seconds)
 * and the `Percent` its update writes into the power's icon.
 */
struct FWasamiPowerGauge
{
	/**
	 * Set Delay <power>: the play rate becomes 1 / Seconds, then a FlipFlop reverses the timeline from its end on one
	 * call (the icon empties) and plays it from the start on the next (the icon fills). The teleport's has no FlipFlop:
	 * it plays from the start while its icon is below 1 and reverses from the end otherwise.
	 */
	void SetDelay(float Seconds, bool bTeleport);

	/** Stop <power> Timeline: stops the timeline and puts the icon back to 1. */
	void Stop();

	/** The timeline's tick: moves the position and, while it plays, writes it to the icon. */
	void Tick(float DeltaSeconds);

	bool IsPlaying() const { return Direction != 0; }

	/** What the icon's `Percent` holds. */
	float Percent = 1.f;

private:
	float Position = 0.f;
	float PlayRate = 1.f;
	/** +1 forward, −1 in reverse, 0 stopped. */
	int8 Direction = 0;
	/** The FlipFlop's next output is B. */
	bool bFlipFlopB = false;
};
