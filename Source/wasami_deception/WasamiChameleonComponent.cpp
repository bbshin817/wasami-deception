#include "WasamiChameleonComponent.h"

#include "Components/PostProcessComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WasamiAssets.h"

namespace
{
	// M_CameraShake's parameters.
	const FName ShakePowerName(TEXT("ShakePower"));
	const FName ShakeFrequencyName(TEXT("ShakeFQ"));
	// Set Advanced Effect Features adds each effect's instance at this weight.
	constexpr float EffectWeight = 1.f;
}

UWasamiChameleonComponent::UWasamiChameleonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CameraShakeMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/Pipeline/Materials/M_DD_ChameleonCameraShake")));
}

void UWasamiChameleonComponent::BeginPlay()
{
	Super::BeginPlay();

	// The Chameleon's InternalPP, Enabled and Unbound (both true); its Native Post Process overrides nothing, so the
	// volume's own settings stay at their defaults.
	AActor* Owner = GetOwner();
	Volume = NewObject<UPostProcessComponent>(Owner, TEXT("InternalPP"));
	Volume->bEnabled = true;
	Volume->bUnbound = true;
	Volume->SetupAttachment(Owner->GetRootComponent());
	Volume->RegisterComponent();

	// Create Material Instances.
	if (UMaterialInterface* Material = CameraShakeMaterial.LoadSynchronous())
	{
		CameraShakeInstance = UMaterialInstanceDynamic::Create(Material, this);
	}
}

void UWasamiChameleonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Volume)
	{
		return;
	}

	// InitChameleon: ApplyChameleonSettings writes the Native Post Process over the settings, which drops every
	// blendable, and each effect that is on puts its instance back with its current settings.
	Volume->Settings.WeightedBlendables.Array.Reset();
	if (bCameraShake && CameraShakeInstance)
	{
		CameraShakeInstance->SetScalarParameterValue(ShakePowerName, CameraShakePower);
		CameraShakeInstance->SetScalarParameterValue(ShakeFrequencyName, CameraShakeFrequency);
		Volume->AddOrUpdateBlendable(CameraShakeInstance, EffectWeight);
	}
}
