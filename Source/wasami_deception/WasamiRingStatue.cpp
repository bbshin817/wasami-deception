#include "WasamiRingStatue.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiShard.h"
#include "WasamiTextPromptWidget.h"

namespace
{
	// BP_01_Statue (pak_reference_2): StaticMeshComponent0's RelativeScale3D and ComponentTags, the CDO's Tags.
	const FVector RingStatueScale(2.799999952316284);
	const FName RingStatueInteractTag(TEXT("interact"));

	// InteractWithObject: PlaySoundAtLocation(DD_RingBarrierDenied_louder, the actor's location, no rotation, 0.4, 1, 0,
	// DialogueAttenuation).
	constexpr float RingStatueDeniedVolume = 0.4f;
}

AWasamiRingStatue::AWasamiRingStatue()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(RingStatueInteractTag);

	UStaticMeshComponent* Mesh = GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetRelativeScale3D(RingStatueScale);
	Mesh->ComponentTags.Add(RingStatueInteractTag);

	DeniedSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/RingStatue/DD_RingBarrierDenied_louder")));
	DeniedAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/DialogueAttenuation")));
	DeniedText = NSLOCTEXT("Wasami", "RingStatueDeniedText", "Collect all soul shards to break the ring barrier.");
}

void AWasamiRingStatue::BeginPlay()
{
	Super::BeginPlay();
	LoadedDeniedSound = DeniedSound.LoadSynchronous();
	LoadedDeniedAttenuation = DeniedAttenuation.LoadSynchronous();
}

void AWasamiRingStatue::InteractWithObject_Implementation(AActor* Interactee)
{
	if (bClosed)
	{
		return;
	}
	bClosed = true;
	// GetAllActorsOfClass(BP_Shard) < 1: a collected shard destroys itself.
	if (!TActorIterator<AWasamiShard>(GetWorld()))
	{
		OnInteractAllShards.Broadcast();
		GetStaticMeshComponent()->ComponentTags.Empty();
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(this, LoadedDeniedSound, GetActorLocation(), FRotator::ZeroRotator,
		RingStatueDeniedVolume, 1.f, 0.f, LoadedDeniedAttenuation);
	LastPrompt = UWasamiTextPromptWidget::Show(this, DeniedText);
	GetWorldTimerManager().SetTimer(DeniedTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { bClosed = false; }),
		DeniedInterval, false);
}
