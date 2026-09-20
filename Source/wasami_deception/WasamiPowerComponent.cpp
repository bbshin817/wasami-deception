#include "WasamiPowerComponent.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiCameraAnim.h"
#include "WasamiChameleonComponent.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiPrimalPower.h"
#include "WasamiTelekinesisPower.h"
#include "WasamiSpeedBoostWidget.h"
#include "WasamiTabletWidget.h"
#include "WasamiTelepathyPower.h"
#include "WasamiTelepathyTrackerWidget.h"
#include "WasamiTeleportAim.h"
#include "WasamiVanishPower.h"
#include "WasamiVanishWidget.h"
#include "WasamiVoice.h"

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
	// PlayCameraAnim(CameraAnim_SpeedBoost, Rate, Scale, BlendInTime, BlendOutTime, bLoop false, Duration = the boost's).
	constexpr float BoostAnimRate = 1.f;
	constexpr float BoostAnimScale = 1.f;
	constexpr float BoostAnimBlendTime = 0.5f;
	// UMG_SpeedBoost goes on the viewport at this Z order.
	constexpr int32 BoostWidgetZOrder = 1;
	// Sprinting Effects runs on a 0.001 s looping timer; it only writes values that follow the speed, so once a frame
	// comes to the same.
	constexpr float SprintingEffectsRate = 0.001f;
	// Sprinting Effects: the camera shake's frequency and power follow the speed from 0 to 870 cm/s.
	constexpr float ShakeFullSpeed = 870.f;
	constexpr float ShakeMaxFrequency = 15.f;
	constexpr float ShakeMaxPower = 0.003f;
	// The teleport (pak_reference): Teleport_Mode_Entered's volume, the icon's drop when the aim comes out, and the aim's
	// spawn 50 m under the player.
	constexpr float TeleportAimVolume = 1.75f;
	constexpr float TeleportGaugeDropSeconds = 0.05f;
	const FVector TeleportAimSpawnOffset(0., 0., -5000.);
	// The telepathy: its start's volume, its end's volume and pitch (Teleport_Mode_Entered), the shake's scale, and the
	// spawn at the world's origin.
	constexpr float TelepathyVolume = 0.6f;
	constexpr float TelepathyEndVolume = 1.f;
	constexpr float TelepathyEndPitch = 1.5f;
	constexpr float TelepathyShakeScale = 1.f;
	// Primal Fear: the icon's drop, the spawn 50 m under the player, and the delay before the cooldown starts.
	constexpr float PrimalGaugeDropSeconds = 0.05f;
	const FVector PrimalSpawnOffset(0., 0., -5000.);
	constexpr float PrimalCooldownDelay = 0.06f;
	// The telekinesis: the icon's drop, the spawn 50 m under the player, and the delay before the cooldown starts.
	constexpr float TelekinesisGaugeDropSeconds = 0.05f;
	const FVector TelekinesisSpawnOffset(0., 0., -5000.);
	constexpr float TelekinesisCooldownDelay = 0.06f;
	// Vanish: the spawn 50 m under the player, and UMG_Vanish on the player's screen at this Z order.
	const FVector VanishSpawnOffset(0., 0., -5000.);
	constexpr int32 VanishWidgetZOrder = 0;
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
	BoostCameraAnim = TSoftObjectPtr<UWasamiCameraAnim>(WasamiAssets::Path(TEXT("/Game/DD/Animation/Camera/CameraAnim_SpeedBoost")));
	BoostWidgetClass = UWasamiSpeedBoostWidget::StaticClass();
	TeleportAimSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Mode_Entered")));
	TeleportAimClass = AWasamiTeleportAim::StaticClass();
	TelepathySound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Telepathy")));
	TelepathyEndSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Mode_Entered")));
	TelepathyShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak")));
	TelepathyPowerClass = AWasamiTelepathyPower::StaticClass();
	PrimalPowerClass = AWasamiPrimalPower::StaticClass();
	TelekinesisPowerClass = AWasamiTelekinesisPower::StaticClass();
	VanishPowerClass = AWasamiVanishPower::StaticClass();
	VanishWidgetClass = UWasamiVanishWidget::StaticClass();
}

void UWasamiPowerComponent::BeginPlay()
{
	Super::BeginPlay();

	LoadedRefillSound = RefillSound.LoadSynchronous();
	LoadedCycleSound = CycleSound.LoadSynchronous();
	LoadedBoostSound = BoostSound.LoadSynchronous();
	LoadedBoostShake = BoostShakeClass.LoadSynchronous();
	LoadedBoostCameraAnim = BoostCameraAnim.LoadSynchronous();
	UWasamiSpeedBoostWidget::LoadAssets(LoadedBoostWidgetAssets);
	LoadedTeleportAimSound = TeleportAimSound.LoadSynchronous();
	AWasamiTeleportAim::LoadAssets(LoadedTeleportAimAssets);
	LoadedTelepathySound = TelepathySound.LoadSynchronous();
	LoadedTelepathyEndSound = TelepathyEndSound.LoadSynchronous();
	LoadedTelepathyShake = TelepathyShakeClass.LoadSynchronous();
	UWasamiTelepathyTrackerWidget::LoadAssets(LoadedTelepathyAssets);
	AWasamiPrimalPower::LoadAssets(LoadedPrimalAssets);
	AWasamiTelekinesisPower::LoadAssets(LoadedTelekinesisAssets);
	AWasamiVanishPower::LoadAssets(LoadedVanishAssets);
	UWasamiVanishWidget::LoadAssets(LoadedVanishAssets);

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

AWasamiTeleportAim* UWasamiPowerComponent::GetTeleportAim() const
{
	return IsValid(TeleportAim) ? TeleportAim.Get() : nullptr;
}

void UWasamiPowerComponent::ConfirmTeleport()
{
	if (AWasamiTeleportAim* Aim = GetTeleportAim())
	{
		Aim->Confirm();
	}
}

void UWasamiPowerComponent::AdjustTeleportDistance(float AxisValue)
{
	if (AWasamiTeleportAim* Aim = GetTeleportAim())
	{
		Aim->AdjustDistance(AxisValue);
	}
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
		// The latest version stays silent here; the take-back of an aiming teleport by its own side is the old
		// version's (pak_reference), which the teleport follows.
		if (Powers.Num() > 0 && IsUsingPower(EWasamiPower::Teleport) && bLeft == bTeleportLeft)
		{
			ResetTeleport();
		}
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
		case EWasamiPower::Teleport:
			UseTeleport(bLeft);
			break;
		case EWasamiPower::Telepathy:
			UseTelepathy();
			break;
		case EWasamiPower::PrimalFear:
			UsePrimal();
			break;
		case EWasamiPower::Telekinesis:
			UseTelekinesis();
			break;
		case EWasamiPower::Vanish:
			UseVanish();
			break;
		default:
			break;
		}
	}
	Delay(UseTimer, UseLockSeconds, &UWasamiPowerComponent::ReopenUse);
}

void UWasamiPowerComponent::ResetPowers()
{
	// Reset Speed Boost 1 stops the boost's camera anim at once and ends a running boost, Reset Speed Boost 2 refills a
	// used one; then the gauge goes back to 1.
	const AWasamiPlayerCharacter* Player = GetPlayer();
	const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (UWasamiCameraAnimModifier* Anims = PC ? UWasamiCameraAnimModifier::Get(PC->PlayerCameraManager) : nullptr)
	{
		Anims->Stop(BoostCameraAnimHandle, true);
	}
	EndSpeedBoost();
	RefillSpeedBoost();
	Gauge(EWasamiPower::SpeedBoost).Stop();

	ResetTeleport();

	// Reset Primal and Reset Vanish close a Gate that every refill opens again, and refill: they refill (and play
	// power_refilled) whether or not the power was used. Vanish's refill takes its widget away, but a running Vanish
	// keeps the capsule and Active Powers until its own 15 s end. The telepathy and the telekinesis are not reset.
	Gauge(EWasamiPower::PrimalFear).Stop();
	RefillPrimal();
	Gauge(EWasamiPower::Vanish).Stop();
	RefillVanish();
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
	// Not the original's: the WebGL version's fast on a boost that went off (its record 04's onBoost).
	WasamiVoice::Say(this, EWasamiVoice::Fast);
	SetPowerAvailable(EWasamiPower::SpeedBoost, false);
	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && PC->PlayerCameraManager && LoadedBoostShake)
	{
		PC->PlayerCameraManager->StartCameraShake(LoadedBoostShake, 1.f, ECameraShakePlaySpace::CameraLocal);
	}
	// Walking and sprinting both go at the boost's speed.
	Player->SetMoveSpeeds(Tuning.BoostSpeed, Tuning.BoostSpeed);

	// The Sequence after the speeds: the gauge, the red tint for the boost's length, the end, the DoOnce nodes, the
	// sprinting effects with the FX's camera shake, and UMG_SpeedBoost.
	Gauge(EWasamiPower::SpeedBoost).SetDelay(Tuning.BoostDuration, false);
	if (UWasamiCameraAnimModifier* Anims = PC ? UWasamiCameraAnimModifier::Get(PC->PlayerCameraManager) : nullptr)
	{
		BoostCameraAnimHandle = Anims->Play(LoadedBoostCameraAnim, BoostAnimRate, BoostAnimScale, BoostAnimBlendTime,
			BoostAnimBlendTime, false, Tuning.BoostDuration);
	}
	Delay(BoostEndTimer, Tuning.BoostDuration, &UWasamiPowerComponent::EndSpeedBoost);
	bBoostEndOpen = true;
	bBoostRefillOpen = true;
	FTimerManagerTimerParameters EffectsTimer;
	EffectsTimer.bLoop = true;
	EffectsTimer.bMaxOncePerFrame = true;
	GetWorld()->GetTimerManager().SetTimer(SprintingEffectsTimer, this, &UWasamiPowerComponent::UpdateSprintingEffects,
		SprintingEffectsRate, EffectsTimer);
	if (UWasamiChameleonComponent* FX = Player->GetChameleon())
	{
		FX->bCameraShake = true;
	}
	if (PC && BoostWidgetClass)
	{
		BoostWidget = CreateWidget<UWasamiSpeedBoostWidget>(PC, BoostWidgetClass);
		BoostWidget->AddToViewport(BoostWidgetZOrder);
	}
}

void UWasamiPowerComponent::UpdateSprintingEffects()
{
	const AWasamiPlayerCharacter* Player = GetPlayer();
	UWasamiChameleonComponent* FX = Player ? Player->GetChameleon() : nullptr;
	if (!FX)
	{
		return;
	}
	// The radial blur's width is written here too, but the FX never has its radial blur on.
	const float Speed = static_cast<float>(Player->GetVelocity().Size());
	const FVector2f SpeedRange(0.f, ShakeFullSpeed);
	FX->CameraShakeFrequency = FMath::GetMappedRangeValueClamped(SpeedRange, FVector2f(0.f, ShakeMaxFrequency), Speed);
	FX->CameraShakePower = FMath::GetMappedRangeValueClamped(SpeedRange, FVector2f(0.f, ShakeMaxPower), Speed);
}

void UWasamiPowerComponent::EndSpeedBoost()
{
	if (!bBoostEndOpen)
	{
		return;
	}
	bBoostEndOpen = false;
	const FWasamiPowerTuning& Tuning = GetTuning(EWasamiPower::SpeedBoost);
	AWasamiPlayerCharacter* Player = GetPlayer();
	if (Player)
	{
		Player->SetMoveSpeeds(BoostEndWalkingSpeed, BoostEndSprintingSpeed);
	}
	ActivePowers.Remove(EWasamiPower::SpeedBoost);
	// The camera anim is left to run out over the boost's length.
	GetWorld()->GetTimerManager().ClearTimer(SprintingEffectsTimer);
	if (UWasamiChameleonComponent* FX = Player ? Player->GetChameleon() : nullptr)
	{
		FX->bCameraShake = false;
	}
	if (BoostWidget)
	{
		BoostWidget->RemoveFromParent();
		BoostWidget = nullptr;
	}
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

void UWasamiPowerComponent::UseTeleport(bool bLeft)
{
	AWasamiPlayerCharacter* Player = GetPlayer();
	bTeleportLeft = bLeft;
	ActivePowers.AddUnique(EWasamiPower::Teleport);
	UGameplayStatics::PlaySound2D(this, LoadedTeleportAimSound, TeleportAimVolume);
	SetPowerAvailable(EWasamiPower::Teleport, false);
	Gauge(EWasamiPower::Teleport).SetDelay(TeleportGaugeDropSeconds, true);

	// The aim comes out 50 m under the player, unrotated, whatever is there, with the level's Max Distance.
	const FTransform SpawnTransform(FRotator::ZeroRotator, Player->GetActorLocation() + TeleportAimSpawnOffset);
	AWasamiTeleportAim* Aim = GetWorld()->SpawnActorDeferred<AWasamiTeleportAim>(TeleportAimClass, SpawnTransform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Aim)
	{
		Aim->MaxDistance = GetTuning(EWasamiPower::Teleport).TeleportDistance;
		Aim->FinishSpawning(SpawnTransform);
		Aim->OnUsed.AddDynamic(this, &UWasamiPowerComponent::UsedTeleport);
	}
	TeleportAim = Aim;

	// The side it was used from cannot cycle until it is over; the cooldown's Gate and refill open.
	(bLeft ? bCanCycleLeft : bCanCycleRight) = false;
	bTeleportGateOpen = true;
	bTeleportRefillOpen = true;
}

void UWasamiPowerComponent::UsedTeleport()
{
	(bTeleportLeft ? bCanCycleLeft : bCanCycleRight) = true;
	ActivePowers.Remove(EWasamiPower::Teleport);
	// The same at every level (the original's 1 s is for its ballroom only).
	const float Cooldown = FWasamiPowerTuning::TeleportCooldown;
	Gauge(EWasamiPower::Teleport).SetDelay(Cooldown, true);
	if (bTeleportGateOpen)
	{
		Delay(TeleportRefillTimer, Cooldown, &UWasamiPowerComponent::RefillTeleport);
	}
}

void UWasamiPowerComponent::RefillTeleport()
{
	if (!bTeleportRefillOpen)
	{
		return;
	}
	bTeleportRefillOpen = false;
	Refill(EWasamiPower::Teleport);
}

void UWasamiPowerComponent::ResetTeleport()
{
	// Close the Gate, refill at once (only after a use), destroy the aim — its move, if a click already started it, goes
	// with it and the capsule keeps ignoring pawns and world-dynamic things, as in the original — then run UsedTeleport,
	// whose cooldown delay the closed Gate holds back; BP_Powers stops the icon at 1 last.
	bTeleportGateOpen = false;
	RefillTeleport();
	if (IsValid(TeleportAim))
	{
		TeleportAim->Destroy();
	}
	UsedTeleport();
	Gauge(EWasamiPower::Teleport).Stop();
}

void UWasamiPowerComponent::UseTelepathy()
{
	AWasamiPlayerCharacter* Player = GetPlayer();
	const float Duration = GetTuning(EWasamiPower::Telepathy).TelepathyDuration;
	ActivePowers.AddUnique(EWasamiPower::Telepathy);
	UGameplayStatics::PlaySound2D(this, LoadedTelepathySound, TelepathyVolume);
	SetPowerAvailable(EWasamiPower::Telepathy, false);
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && PC->PlayerCameraManager && LoadedTelepathyShake)
	{
		PC->PlayerCameraManager->StartCameraShake(LoadedTelepathyShake, TelepathyShakeScale, ECameraShakePlaySpace::CameraLocal);
	}

	// BP_Telepathy comes out at the world's origin, whatever is there, with the level's Time; it counts that time too.
	const FTransform SpawnTransform = FTransform::Identity;
	if (AWasamiTelepathyPower* Telepathy = GetWorld()->SpawnActorDeferred<AWasamiTelepathyPower>(TelepathyPowerClass,
		SpawnTransform))
	{
		Telepathy->Time = Duration;
		Telepathy->FinishSpawning(SpawnTransform);
	}
	Gauge(EWasamiPower::Telepathy).SetDelay(Duration, false);
	Delay(TelepathyEndTimer, Duration, &UWasamiPowerComponent::EndTelepathy);
}

void UWasamiPowerComponent::EndTelepathy()
{
	ActivePowers.Remove(EWasamiPower::Telepathy);
	UGameplayStatics::PlaySound2D(this, LoadedTelepathyEndSound, TelepathyEndVolume, TelepathyEndPitch);
	const float Cooldown = GetTuning(EWasamiPower::Telepathy).TelepathyCooldown;
	Gauge(EWasamiPower::Telepathy).SetDelay(Cooldown, false);
	Delay(TelepathyRefillTimer, Cooldown, &UWasamiPowerComponent::RefillTelepathy);
}

void UWasamiPowerComponent::UsePrimal()
{
	const AWasamiPlayerCharacter* Player = GetPlayer();
	ActivePowers.AddUnique(EWasamiPower::PrimalFear);
	SetPowerAvailable(EWasamiPower::PrimalFear, false);
	Gauge(EWasamiPower::PrimalFear).SetDelay(PrimalGaugeDropSeconds, false);

	// BP_PrimalPower comes out 50 m under the player, unrotated, whatever is there, with the level's Range.
	const FTransform SpawnTransform(FRotator::ZeroRotator, Player->GetActorLocation() + PrimalSpawnOffset);
	if (AWasamiPrimalPower* Primal = GetWorld()->SpawnActorDeferred<AWasamiPrimalPower>(PrimalPowerClass, SpawnTransform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Primal->Range = GetTuning(EWasamiPower::PrimalFear).PrimalRange;
		Primal->FinishSpawning(SpawnTransform);
	}
	Delay(PrimalCooldownTimer, PrimalCooldownDelay, &UWasamiPowerComponent::StartPrimalCooldown);
}

void UWasamiPowerComponent::StartPrimalCooldown()
{
	// The original's 5 s is for its circus entrance only.
	const float Cooldown = GetTuning(EWasamiPower::PrimalFear).PrimalCooldown;
	Gauge(EWasamiPower::PrimalFear).SetDelay(Cooldown, false);
	ActivePowers.Remove(EWasamiPower::PrimalFear);
	Delay(PrimalRefillTimer, Cooldown, &UWasamiPowerComponent::RefillPrimal);
}

void UWasamiPowerComponent::UseTelekinesis()
{
	const AWasamiPlayerCharacter* Player = GetPlayer();
	ActivePowers.AddUnique(EWasamiPower::Telekinesis);
	SetPowerAvailable(EWasamiPower::Telekinesis, false);
	Gauge(EWasamiPower::Telekinesis).SetDelay(TelekinesisGaugeDropSeconds, false);

	// BP_TelekinesisPower comes out 50 m under the player, unrotated, whatever is there, with the level's Range.
	const FTransform SpawnTransform(FRotator::ZeroRotator, Player->GetActorLocation() + TelekinesisSpawnOffset);
	if (AWasamiTelekinesisPower* Telekinesis = GetWorld()->SpawnActorDeferred<AWasamiTelekinesisPower>(TelekinesisPowerClass,
		SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Telekinesis->Range = GetTuning(EWasamiPower::Telekinesis).TelekinesisRange;
		Telekinesis->FinishSpawning(SpawnTransform);
	}
	Delay(TelekinesisCooldownTimer, TelekinesisCooldownDelay, &UWasamiPowerComponent::StartTelekinesisCooldown);
}

void UWasamiPowerComponent::StartTelekinesisCooldown()
{
	// The original's 1 s is for its ballroom only.
	const float Cooldown = GetTuning(EWasamiPower::Telekinesis).TelekinesisCooldown;
	Gauge(EWasamiPower::Telekinesis).SetDelay(Cooldown, false);
	ActivePowers.Remove(EWasamiPower::Telekinesis);
	Delay(TelekinesisRefillTimer, Cooldown, &UWasamiPowerComponent::RefillTelekinesis);
}

void UWasamiPowerComponent::UseVanish()
{
	AWasamiPlayerCharacter* Player = GetPlayer();
	const float Duration = FWasamiPowerTuning::VanishDuration;
	ActivePowers.AddUnique(EWasamiPower::Vanish);
	SetPowerAvailable(EWasamiPower::Vanish, false);
	// The enemies look along the camera channel: their sight passes through the capsule now.
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Gauge(EWasamiPower::Vanish).SetDelay(Duration, false);

	// UMG_Vanish plays its 1 s animation over the effect's 15 s (Speed is 15 at every level, as the duration).
	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && VanishWidgetClass)
	{
		VanishWidget = CreateWidget<UWasamiVanishWidget>(PC, VanishWidgetClass);
		VanishWidget->Speed = Duration;
		VanishWidget->AddToPlayerScreen(VanishWidgetZOrder);
	}

	// BP_VanishPower comes out 50 m under the player, turned as the player, whatever is there.
	const FTransform SpawnTransform(Player->GetActorRotation(), Player->GetActorLocation() + VanishSpawnOffset);
	if (AWasamiVanishPower* Burst = GetWorld()->SpawnActorDeferred<AWasamiVanishPower>(VanishPowerClass, SpawnTransform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Burst->FinishSpawning(SpawnTransform);
	}
	Delay(VanishEndTimer, Duration, &UWasamiPowerComponent::EndVanish);
}

void UWasamiPowerComponent::EndVanish()
{
	// Nothing tells the enemies: the capsule just blocks their sight again.
	const float Cooldown = GetTuning(EWasamiPower::Vanish).VanishCooldown;
	Gauge(EWasamiPower::Vanish).SetDelay(Cooldown, false);
	ActivePowers.Remove(EWasamiPower::Vanish);
	if (AWasamiPlayerCharacter* Player = GetPlayer())
	{
		Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	}
	Delay(VanishRefillTimer, Cooldown, &UWasamiPowerComponent::RefillVanish);
}

void UWasamiPowerComponent::RefillVanish()
{
	Refill(EWasamiPower::Vanish);
	// The widget has been at an opacity of 0 since the effect's end.
	if (VanishWidget)
	{
		VanishWidget->RemoveFromParent();
	}
}
