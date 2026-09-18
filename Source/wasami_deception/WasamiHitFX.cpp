#include "WasamiHitFX.h"

#include "Camera/CameraShakeBase.h"
#include "Components/PostProcessComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"

AWasamiHitFX::AWasamiHitFX()
{
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Unbound (the component's default): the whole view, whatever the actor's place.
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(DefaultSceneRoot);
	PostProcess->BlendWeight = 0.f;
	FPostProcessSettings& Settings = PostProcess->Settings;
	Settings.bOverride_ColorGain = true;
	Settings.ColorGain = FVector4(1., 0.21828000247478485, 0.18047699332237244, 1.);
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = 10.f;
	// Overridden at its default, as the original has it.
	Settings.bOverride_ChromaticAberrationStartOffset = true;

	// CurveFloat_0: 1 at 0 s falling steeply (cubic, its own tangents) to 0 at 0.35 s.
	Curve = CreateDefaultSubobject<UCurveFloat>(TEXT("CurveFloat_0"));
	FRichCurve& Float = Curve->FloatCurve;
	FRichCurveKey& Start = Float.Keys.Emplace_GetRef(0.f, 1.f, -5.21370792388916f, -5.213721752166748f, RCIM_Cubic);
	Start.TangentMode = RCTM_User;
	Float.Keys.Emplace(0.3499999940395355f, 0.f, 0.f, 0.f, RCIM_Linear);

	Timeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("Timeline_0"));

	ShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/04_Sewer/Bossfight/BP_04_BossFight_CameraShake_Initial")));
}

void AWasamiHitFX::BeginPlay()
{
	Super::BeginPlay();
	FOnTimelineFloat Update;
	Update.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AWasamiHitFX, OnTimelineUpdate));
	Timeline->AddInterpFloat(Curve, Update);
	FOnTimelineEvent Finished;
	Finished.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AWasamiHitFX, OnTimelineFinished));
	Timeline->SetTimelineFinishedFunc(Finished);
	// A Blueprint timeline's length (a C++ one runs to its last key by default).
	Timeline->SetTimelineLengthMode(TL_TimelineLength);
	Timeline->SetTimelineLength(TimelineLength);
	HitEffect();
}

void AWasamiHitFX::HitEffect()
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (const TSubclassOf<UCameraShakeBase> Shake = ShakeClass.LoadSynchronous(); Shake && Controller)
	{
		Controller->ClientStartCameraShake(Shake, ShakeScale, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
	}
	Timeline->SetPlayRate(PlayRate);
	Timeline->PlayFromStart();
}

void AWasamiHitFX::OnTimelineUpdate(float Value)
{
	PostProcess->BlendWeight = Value;
}

void AWasamiHitFX::OnTimelineFinished()
{
	Destroy();
}
