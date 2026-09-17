#include "WasamiPowerTypes.h"

namespace
{
	// BP_DD_PlayerCharacter (pak_reference_2), a row per upgrade level. Every value matches the ring altar's tables
	// (PowersTable_*, whose Upgrade2–6 are levels 1–5) except the speed boost's cooldown, which the code makes 1 s longer.
	const FWasamiPowerTuning Levels[FWasamiPowerTuning::MaxLevel + 1] = {
		//  boost speed / time / cooldown, teleport, telepathy time / cooldown, primal range / cooldown, telekinesis range / cooldown, vanish cooldown
		{870.f, 6.75f, 9.5f, 1000.f, 5.f, 8.5f, 1500.f, 35.f, 1750.f, 10.5f, 30.f},
		{870.f, 6.75f, 9.5f, 1000.f, 5.f, 8.5f, 1500.f, 35.f, 2000.f, 10.f, 28.f},
		{890.f, 7.5f, 9.f, 1125.f, 6.f, 8.f, 2000.f, 32.f, 2250.f, 9.5f, 24.f},
		{910.f, 8.25f, 8.5f, 1250.f, 7.f, 7.5f, 2500.f, 29.f, 2500.f, 9.f, 20.f},
		{930.f, 9.f, 8.f, 1375.f, 8.f, 7.f, 3000.f, 26.f, 2750.f, 8.5f, 18.f},
		{950.f, 9.75f, 7.5f, 1500.f, 9.f, 6.5f, 3500.f, 23.f, 3000.f, 8.f, 15.f},
	};
}

const FWasamiPowerTuning& FWasamiPowerTuning::ForLevel(int32 Level)
{
	return Levels[FMath::Clamp(Level, 0, MaxLevel)];
}

void FWasamiPowerGauge::SetDelay(float Seconds, bool bTeleport)
{
	PlayRate = Seconds > 0.f ? 1.f / Seconds : 1.f;
	bool bFill;
	if (bTeleport)
	{
		bFill = Percent < 1.f;
	}
	else
	{
		bFill = bFlipFlopB;
		bFlipFlopB = !bFlipFlopB;
	}
	// PlayFromStart / ReverseFromEnd move the position without an update; the icon follows on the next tick.
	Position = bFill ? 0.f : 1.f;
	Direction = bFill ? 1 : -1;
}

void FWasamiPowerGauge::Stop()
{
	Direction = 0;
	Percent = 1.f;
}

void FWasamiPowerGauge::Tick(float DeltaSeconds)
{
	if (Direction == 0)
	{
		return;
	}
	Position += DeltaSeconds * PlayRate * Direction;
	if (Position >= 1.f || Position <= 0.f)
	{
		Position = FMath::Clamp(Position, 0.f, 1.f);
		Direction = 0;
	}
	Percent = Position;
}
