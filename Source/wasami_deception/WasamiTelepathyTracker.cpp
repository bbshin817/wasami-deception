#include "WasamiTelepathyTracker.h"

#include "Components/PostProcessComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Curves/RichCurve.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "WasamiAssets.h"

namespace
{
	// BP_TelepathyTracker (pak_reference_2): Remove's Delay.
	constexpr float RemoveDelay = 0.5f;

	// UMG_TelepathyTracker's Appear and Disappear opacity keys (ticks at 60000 a second, cubic with flat tangents):
	// Appear plays [0, 30001) and ends at tick 30000; Disappear plays [0, 18001) and ends at tick 18000.
	constexpr double AnimTicksPerSecond = 60000.;
	constexpr double AppearOpacityEnd = 30001.;
	constexpr double DisappearOpacityEnd = 18000.;

	// The stencil the smoke is drawn at its fullest (the post-process material reads stencil / 255).
	constexpr int32 FullStencil = 255;

	FRichCurve MakeFadeCurve(float From, float To, double EndTicks)
	{
		FRichCurve Curve;
		for (const TPair<double, float>& Each : {TPair<double, float>(0., From), TPair<double, float>(EndTicks, To)})
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(static_cast<float>(Each.Key / AnimTicksPerSecond), Each.Value));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_User;
			Key.ArriveTangent = 0.f;
			Key.LeaveTangent = 0.f;
		}
		return Curve;
	}
}

AWasamiTelepathyTracker::AWasamiTelepathyTracker()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = SceneRoot;

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(SceneRoot);
	PostProcess->bUnbound = true;

	SilhouetteMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/Wasami/Powers/MI_WasamiTelepathySilhouette")));
}

void AWasamiTelepathyTracker::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	Out.Add(GetDefault<AWasamiTelepathyTracker>()->SilhouetteMaterial.LoadSynchronous());
}

float AWasamiTelepathyTracker::EvaluateAppearOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeFadeCurve(0.f, 1.f, AppearOpacityEnd);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, AppearLength));
}

float AWasamiTelepathyTracker::EvaluateDisappearOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeFadeCurve(1.f, 0.f, DisappearOpacityEnd);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, DisappearLength));
}

int32 AWasamiTelepathyTracker::StencilForFade(float InFade)
{
	return FMath::Clamp(FMath::RoundToInt(InFade * FullStencil), 0, FullStencil);
}

void AWasamiTelepathyTracker::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Material = SilhouetteMaterial.LoadSynchronous())
	{
		PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, Material));
	}
	MarkMeshes();
	bGateOpen = true;
	UpdateFade();
}

void AWasamiTelepathyTracker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RemoveTimer);
	UnmarkMeshes();
	Super::EndPlay(EndPlayReason);
}

void AWasamiTelepathyTracker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bGateOpen)
	{
		Update();
	}
	FadeTime += DeltaSeconds;
	UpdateFade();
}

void AWasamiTelepathyTracker::Update()
{
	if (!IsValid(Actor))
	{
		Remove();
		return;
	}
	// Onto the enemy's origin (its capsule's centre).
	SetActorLocation(Actor->GetActorLocation());
}

void AWasamiTelepathyTracker::UpdateFade()
{
	const float NewFade = bFadingOut ? EvaluateDisappearOpacity(FadeTime) : EvaluateAppearOpacity(FadeTime);
	if (NewFade == Fade && FadeTime > 0.f)
	{
		return;
	}
	Fade = NewFade;
	const int32 Stencil = StencilForFade(Fade);
	for (const TWeakObjectPtr<USkinnedMeshComponent>& Each : MarkedMeshes)
	{
		if (USkinnedMeshComponent* Mesh = Each.Get())
		{
			Mesh->SetCustomDepthStencilValue(Stencil);
		}
	}
}

void AWasamiTelepathyTracker::MarkMeshes()
{
	if (!IsValid(Actor))
	{
		return;
	}
	// The body only: the skinned meshes (an enemy's map mark is a static mesh, and its capsule no mesh at all).
	TArray<USkinnedMeshComponent*> Meshes;
	Actor->GetComponents(Meshes);
	for (USkinnedMeshComponent* Mesh : Meshes)
	{
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(0);
		MarkedMeshes.Add(Mesh);
	}
}

void AWasamiTelepathyTracker::UnmarkMeshes()
{
	// A later telepathy's tracker on the same enemy may still be showing it.
	for (TActorIterator<AWasamiTelepathyTracker> It(GetWorld()); It; ++It)
	{
		if (*It != this && IsValid(*It) && It->Actor == Actor)
		{
			MarkedMeshes.Reset();
			return;
		}
	}
	for (const TWeakObjectPtr<USkinnedMeshComponent>& Each : MarkedMeshes)
	{
		if (USkinnedMeshComponent* Mesh = Each.Get())
		{
			Mesh->SetRenderCustomDepth(false);
			Mesh->SetCustomDepthStencilValue(0);
		}
	}
	MarkedMeshes.Reset();
}

void AWasamiTelepathyTracker::Remove()
{
	bGateOpen = false;
	if (!bFadingOut)
	{
		// Out from wherever the fade in reached: the time into the fade out where the fade is as high as now.
		bFadingOut = true;
		FadeTime = 0.f;
		while (FadeTime < DisappearLength && EvaluateDisappearOpacity(FadeTime) > Fade)
		{
			FadeTime += 1.f / 120.f;
		}
		UpdateFade();
	}
	// A Delay: a second Remove while it counts does not restart it.
	FTimerManager& Timers = GetWorldTimerManager();
	if (!Timers.IsTimerActive(RemoveTimer))
	{
		Timers.SetTimer(RemoveTimer, this, &AWasamiTelepathyTracker::DestroyAfterRemove, RemoveDelay, false);
	}
}
