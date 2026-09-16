#include "WasamiPowerComponent.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiTabletWidget.h"

namespace
{
	// BP_DD_PlayerCharacter (pak_reference_2): the DoOnce + Delay behind Q, E and Use Power.
	constexpr float UseLockSeconds = 0.5f;
	// PlaySound2D's volume and pitch for power_refilled and for a cycle's UI_Select_V3.
	constexpr float RefillVolume = 0.5f;
	constexpr float CycleVolume = 1.5f;
	constexpr float CyclePitch = 2.f;
	// The speeds the end of the speed boost writes back (constants in the original, not the speeds from before).
	constexpr float BoostEndWalkingSpeed = 300.f;
	constexpr float BoostEndSprintingSpeed = 600.f;
}

UWasamiPowerComponent::UWasamiPowerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	for (int32 Index = 0; Index < WasamiPowerCount; ++Index)
	{
		UnlockedPowers.Add(static_cast<EWasamiPower>(Index));
	}

	RefillSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/power_refilled")));
	CycleSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
	BoostSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/Shard_Streak_Milestone_V5")));
	BoostShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak")));
}

void UWasamiPowerComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadedRefillSound = RefillSound.LoadSynchronous();
	LoadedCycleSound = CycleSound.LoadSynchronous();
	LoadedBoostSound = BoostSound.LoadSynchronous();
	LoadedBoostShake = BoostShakeClass.LoadSynchronous();

	// UMG_TabletPowers' Check: every unlocked power, ready. The sockets keep their indices (0 and 0 on a new game, as the
	// original's GameInstance starts them).
	Powers.Reset(UnlockedPowers.Num());
	for (EWasamiPower Power : UnlockedPowers)
	{
		FWasamiPowerSlot& Slot = Powers.AddDefaulted_GetRef();
		Slot.Power = Power;
		Slot.bAvailable = true;
	}
}

void UWasamiPowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	for (FWasamiPowerGauge& Each : Gauges)
	{
		Each.Tick(DeltaTime);
	}
}

AWasamiPlayerCharacter* UWasamiPowerComponent::GetPlayer() const
{
	return Cast<AWasamiPlayerCharacter>(GetOwner());
}

void UWasamiPowerComponent::Delay(FTimerHandle& Handle, float Seconds, void (UWasamiPowerComponent::*Callback)())
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (!Timers.IsTimerActive(Handle))
	{
		Timers.SetTimer(Handle, this, Callback, Seconds, false);
	}
}

void UWasamiPowerComponent::UsePowerKey(bool bLeft)
{
	const AWasamiPlayerCharacter* Player = GetPlayer();
	bool& bClosed = bLeft ? bLeftKeyClosed : bRightKeyClosed;
	if (!Player || !Player->bCanInteract || bClosed)
	{
		return;
	}
	bClosed = true;
	UsePower(bLeft);
	if (bLeft)
	{
		Delay(LeftKeyTimer, UseLockSeconds, &UWasamiPowerComponent::ReopenLeftKey);
	}
	else
	{
		Delay(RightKeyTimer, UseLockSeconds, &UWasamiPowerComponent::ReopenRightKey);
	}
}

void UWasamiPowerComponent::CyclePower(bool bLeft)
{
	const AWasamiPlayerCharacter* Player = GetPlayer();
	if (!Player || !Player->IsTabletUp() || !(bLeft ? bCanCycleLeft : bCanCycleRight))
	{
		return;
	}
	if (Powers.Num() > 0)
	{
		UGameplayStatics::PlaySound2D(this, LoadedCycleSound, CycleVolume, CyclePitch);
	}
	// Past the last unlocked power it wraps to the first; either socket may show the same power as the other.
	int32& Index = bLeft ? LeftIndex : RightIndex;
	Index = Index == Powers.Num() - 1 ? 0 : FMath::Clamp(Index + 1, 0, WasamiPowerCount - 1);
}

void UWasamiPowerComponent::UsePower(bool bLeft)
{
	AWasamiPlayerCharacter* Player = GetPlayer();
	if (!Player || !Player->bCanUseTablet || !Player->bHasInput)
	{
		return;
	}
	// The socket bounces before anything is checked, so it bounces for a power that is not ready too.
	if (UWasamiTabletWidget* Screen = Player->GetTabletScreen())
	{
		Screen->BounceSocket(bLeft);
	}

	const int32 Index = bLeft ? LeftIndex : RightIndex;
	// Array_Get past the end gives the struct's defaults, which cannot be used.
	const FWasamiPowerSlot ToUse = Powers.IsValidIndex(Index) ? Powers[Index] : FWasamiPowerSlot();
	if (!ToUse.bAvailable)
	{
		return;
	}

	if (!bUseClosed)
	{
		bUseClosed = true;
		OnPowerUsed.Broadcast(ToUse.Power);
		switch (ToUse.Power)
		{
		case EWasamiPower::SpeedBoost:
			UseSpeedBoost();
			break;
		default:
			break;
		}
	}
	Delay(UseTimer, UseLockSeconds, &UWasamiPowerComponent::ReopenUse);
}

void UWasamiPowerComponent::ResetPowers()
{
	// Reset Speed Boost 1 ends a running boost, Reset Speed Boost 2 refills a used one; then the gauge goes back to 1.
	EndSpeedBoost();
	RefillSpeedBoost();
	Gauge(EWasamiPower::SpeedBoost).Stop();

	Gauge(EWasamiPower::Teleport).Stop();

	// Reset Primal and Reset Vanish close a Gate that every refill opens again, and refill: they refill (and play
	// power_refilled) whether or not the power was used. The telepathy and the telekinesis are not reset.
	Gauge(EWasamiPower::PrimalFear).Stop();
	Refill(EWasamiPower::PrimalFear);
	Gauge(EWasamiPower::Vanish).Stop();
	Refill(EWasamiPower::Vanish);
}

EWasamiPower UWasamiPowerComponent::GetSocketPower(bool bLeft) const
{
	const int32 Index = bLeft ? LeftIndex : RightIndex;
	return Powers.IsValidIndex(Index) ? Powers[Index].Power : EWasamiPower::None;
}

float UWasamiPowerComponent::GetGaugePercent(EWasamiPower Power) const
{
	const int32 Index = static_cast<int32>(Power);
	return Index < WasamiPowerCount ? Gauges[Index].Percent : 1.f;
}

bool UWasamiPowerComponent::IsPowerAvailable(EWasamiPower Power) const
{
	const FWasamiPowerSlot* Slot = Powers.FindByPredicate([Power](const FWasamiPowerSlot& Each) { return Each.Power == Power; });
	return Slot && Slot->bAvailable;
}

void UWasamiPowerComponent::SetPowerAvailable(EWasamiPower Power, bool bAvailable)
{
	for (FWasamiPowerSlot& Slot : Powers)
	{
		if (Slot.Power == Power && Slot.bAvailable != bAvailable)
		{
			Slot.bAvailable = bAvailable;
			return;
		}
	}
}

void UWasamiPowerComponent::Refill(EWasamiPower Power)
{
	UGameplayStatics::PlaySound2D(this, LoadedRefillSound, RefillVolume);
	SetPowerAvailable(Power, true);
}

void UWasamiPowerComponent::UseSpeedBoost()
{
	AWasamiPlayerCharacter* Player = GetPlayer();
	const FWasamiPowerTuning& Tuning = GetTuning(EWasamiPower::SpeedBoost);

	ActivePowers.AddUnique(EWasamiPower::SpeedBoost);
	UGameplayStatics::PlaySoundAtLocation(this, LoadedBoostSound, Player->GetActorLocation());
	SetPowerAvailable(EWasamiPower::SpeedBoost, false);
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && PC->PlayerCameraManager && LoadedBoostShake)
	{
		PC->PlayerCameraManager->StartCameraShake(LoadedBoostShake, 1.f, ECameraShakePlaySpace::CameraLocal);
	}
	// Walking and sprinting both go at the boost's speed.
	Player->SetMoveSpeeds(Tuning.BoostSpeed, Tuning.BoostSpeed);

	Gauge(EWasamiPower::SpeedBoost).SetDelay(Tuning.BoostDuration, false);
	Delay(BoostEndTimer, Tuning.BoostDuration, &UWasamiPowerComponent::EndSpeedBoost);
	bBoostEndOpen = true;
	bBoostRefillOpen = true;
}

void UWasamiPowerComponent::EndSpeedBoost()
{
	if (!bBoostEndOpen)
	{
		return;
	}
	bBoostEndOpen = false;
	const FWasamiPowerTuning& Tuning = GetTuning(EWasamiPower::SpeedBoost);
	if (AWasamiPlayerCharacter* Player = GetPlayer())
	{
		Player->SetMoveSpeeds(BoostEndWalkingSpeed, BoostEndSprintingSpeed);
	}
	ActivePowers.Remove(EWasamiPower::SpeedBoost);
	Gauge(EWasamiPower::SpeedBoost).SetDelay(Tuning.BoostCooldown, false);
	Delay(BoostRefillTimer, Tuning.BoostCooldown, &UWasamiPowerComponent::RefillSpeedBoost);
}

void UWasamiPowerComponent::RefillSpeedBoost()
{
	if (!bBoostRefillOpen)
	{
		return;
	}
	bBoostRefillOpen = false;
	Refill(EWasamiPower::SpeedBoost);
}
