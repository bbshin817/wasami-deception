#include "WasamiVanishPower.h"

#include "Components/PostProcessComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiEnemyInterface.h"

namespace
{
	// BP_VanishPower (pak_reference_2). PostProcess: saturation 0 under this gain, a purple screen. PostProcess1 turns its
	// fringe override on at 0 (Primal's is 50). Its grain override (UE 4's GrainIntensity, at its default 0) is left out:
	// UE 5.8 keeps that property only as GrainIntensity_DEPRECATED, which nothing blends or renders.
	const FVector4 VanishTintGain(0.6976670026779175, 0., 1.6100000143051147, 1.);
	constexpr float VanishFlashFringe = 0.f;
	// The puff's place under the root (the export's RelativeLocation).
	const FVector PuffLocation(92.42288208007812, -0.00042724609375, -152.14666748046875);
	// PlaySoundAtLocation(Stun_Wave_Attack_New_04) at the origin (the wave has no attenuation, so it is not placed).
	constexpr float WaveVolume = 1.f;
	constexpr float WavePitch = 1.f;
	// GetAllActorsWithTag.
	const FName EnemyTag(TEXT("Enemy"));

	// The timeline's float2 track (CurveFloat_1). Its float, desaturation and opacity tracks (Primal's keys) drive nothing.
	const FWasamiCurveKey FadeKeys[] = {
		{-0.011600494384765625f, 0.f, RCIM_Cubic, -0.0950283631682396f, -0.09502881020307541f},
		{0.30000001192092896f, 1.f, RCIM_Cubic, 1.0569698810577393f, 1.0569703578948975f},
	};
}

const FRichCurve& AWasamiVanishPower::VanishFadeCurve()
{
	static const FRichCurve Curve = MakeCurve(FadeKeys);
	return Curve;
}

AWasamiVanishPower::AWasamiVanishPower()
{
	PostProcess->Settings.ColorGain = VanishTintGain;
	PostProcess1->Settings.SceneFringeIntensity = VanishFlashFringe;
	FadeCurve = VanishFadeCurve();

	// The template comes on BeginPlay; the component activates itself then (it is auto-activated, as the original's)
	// and turns its own tick on.
	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	ParticleSystem->SetupAttachment(SceneRoot);
	ParticleSystem->SetRelativeLocation(PuffLocation);
	ParticleSystem->PrimaryComponentTick.bStartWithTickEnabled = false;

	PuffParticles = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff")));
	WaveSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04")));
}

void AWasamiVanishPower::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const AWasamiVanishPower* Defaults = GetDefault<AWasamiVanishPower>();
	Out.Add(Defaults->PuffParticles.LoadSynchronous());
	Out.Add(Defaults->WaveSound.LoadSynchronous());
}

int32 AWasamiVanishPower::NotifyEnemies(const UObject* WorldContextObject)
{
	// Every tagged actor, cast to the interface: a tagged actor without it (the original's Zone 2 matron) is not told.
	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, EnemyTag, Tagged);
	int32 Told = 0;
	for (AActor* Each : Tagged)
	{
		if (Each && Each->Implements<UWasamiEnemyInterface>())
		{
			IWasamiEnemyInterface::Execute_PlayerVanish(Each);
			++Told;
		}
	}
	return Told;
}

void AWasamiVanishPower::StartPower()
{
	// The original's component starts with its template when it registers, under the spawn; its particles are made on
	// its ticks, after the move.
	ParticleSystem->SetTemplate(PuffParticles.LoadSynchronous());

	// From the spawn 50 m down onto the player's capsule centre, keeping the spawn's turn (the player's).
	if (const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		SetActorLocation(Player->GetActorLocation());
	}
	UGameplayStatics::PlaySoundAtLocation(this, WaveSound.LoadSynchronous(), FVector::ZeroVector, WaveVolume, WavePitch);
	NotifyEnemies(this);
}
