#include "WasamiSecretRoomZone.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiCollectablesWidget.h"

namespace
{
	// BP_SecretRoomZone's Box_GEN_VARIABLE (pak_reference_2): UBoxComponent's 32 cm extent sized by its scale, out of the
	// navigation like the other overlap boxes (its AreaClass, NavArea_Obstacle, blocks nothing).
	const FVector SecretRoomBoxLocation(0., 0., 20.);
	const FVector SecretRoomBoxScale(5., 2., 1.5);

	// CreateSound2D(67-Dark_Whispers_SFX_0704, 1, 1, 0, None, False, False); FadeIn's volume, FadeOut's.
	constexpr float WhispersVolume = 1.f;
	constexpr float WhispersInVolume = 1.f;
	constexpr float WhispersOutVolume = 0.f;

	// Set Advanced Effect Features' BlendingOpacity, and the Glitch Func's parameters by the material's names. The
	// struct's other entries (BlendMode 0, the white mask at 1 × 1, BlendDistance 0, sharpness 10, no custom depth or
	// stencil) are the branches the estimate keeps, so they are not set.
	const FName GlitchOpacityParameter(TEXT("BlendingOpacity"));
	constexpr float GlitchBlendableWeight = 1.f;
}

AWasamiSecretRoomZone::AWasamiSecretRoomZone()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(DefaultSceneRoot);
	Box->SetRelativeLocation(SecretRoomBoxLocation);
	Box->SetRelativeScale3D(SecretRoomBoxScale);
	Box->SetCanEverAffectNavigation(false);
	Box->OnComponentBeginOverlap.AddDynamic(this, &AWasamiSecretRoomZone::OnBoxBeginOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &AWasamiSecretRoomZone::OnBoxEndOverlap);

	// The Chameleon's PostProcessComponent_0 (BlendRadius 0) as its ApplyChameleonSettings leaves it: bUnbound (the
	// class's Unbound) and its own settings (Native Post Process) overriding nothing.
	Glitch = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Glitch"));
	Glitch->SetupAttachment(DefaultSceneRoot);
	Glitch->bUnbound = true;
	Glitch->BlendRadius = 0.f;

	WhispersSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/67-Dark_Whispers_SFX_0704")));
	GlitchBase = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/Pipeline/Materials/M_DD_ChameleonGlitch")));
}

void AWasamiSecretRoomZone::BeginPlay()
{
	Super::BeginPlay();
	UWasamiCollectablesSecretWidget::LoadAssets(LoadedAssets);

	// ReceiveBeginPlay: the whispers, kept as Sound (a UI sound, as CreateSound2D's are), and the Chameleon kept.
	if (USoundBase* Sound = WhispersSound.LoadSynchronous())
	{
		Whispers = UGameplayStatics::CreateSound2D(this, Sound, WhispersVolume, 1.f, 0.f, nullptr, false, false);
	}

	// The Chameleon's Glitch Func, run by its tick: the glitch's instance with its values, added to the post process.
	if (UMaterialInterface* Base = GlitchBase.LoadSynchronous())
	{
		GlitchMaterial = UMaterialInstanceDynamic::Create(Base, this);
		GlitchMaterial->SetScalarParameterValue(TEXT("Amount"), GlitchBlocking);
		GlitchMaterial->SetScalarParameterValue(TEXT("Speed"), GlitchSpeed);
		GlitchMaterial->SetScalarParameterValue(TEXT("Density"), GlitchLines);
		GlitchMaterial->SetScalarParameterValue(TEXT("GridDistortionPower"), GlitchGridDistortionPower);
		GlitchMaterial->SetScalarParameterValue(TEXT("GridDistortionSize"), GlitchGridDistortionSize);
		GlitchMaterial->SetScalarParameterValue(TEXT("GridDistortionSpeed"), GlitchGridDistortionSpeed);
		Glitch->AddOrUpdateBlendable(GlitchMaterial.Get(), GlitchBlendableWeight);
	}
	// The zone's template has the ChildActor's Glitch - Advanced at BlendingOpacity 0.
	SetGlitchOpacity(0.f);
}

void AWasamiSecretRoomZone::SetGlitchOpacity(float Opacity)
{
	GlitchOpacity = Opacity;
	if (GlitchMaterial)
	{
		GlitchMaterial->SetScalarParameterValue(GlitchOpacityParameter, Opacity);
	}
}

void AWasamiSecretRoomZone::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		NotifyPlayerOverlap(true);
	}
}

void AWasamiSecretRoomZone::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		NotifyPlayerOverlap(false);
	}
}

void AWasamiSecretRoomZone::NotifyPlayerOverlap(bool bBegin)
{
	if (bBegin)
	{
		// Sound.FadeIn(1, 1, 0, Linear); Glitch - Advanced with BlendingOpacity 1; DoOnce → CreateAndAddWidget(
		// UMG_Collectables_Secret_C, None, 1).
		if (Whispers)
		{
			Whispers->FadeIn(WhispersFadeSeconds, WhispersInVolume, 0.f);
		}
		SetGlitchOpacity(1.f);
		if (!bBannerShown)
		{
			bBannerShown = true;
			UWasamiCollectablesSecretWidget::Show(this);
		}
	}
	else
	{
		// Sound.FadeOut(1, 0, Linear); Glitch - Advanced with BlendingOpacity 0.
		if (Whispers)
		{
			Whispers->FadeOut(WhispersFadeSeconds, WhispersOutVolume);
		}
		SetGlitchOpacity(0.f);
	}
}
