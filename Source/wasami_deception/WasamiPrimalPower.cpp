#include "WasamiPrimalPower.h"

#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "WasamiEnemyInterface.h"

namespace
{
	// BP_PrimalPower (pak_reference_2). PostProcess: saturation 0 under this gain, a red screen; PostProcess1's fringe.
	const FVector4 PrimalTintGain(1.6100000143051147, 0.12956300377845764, 0., 1.);
	constexpr float PrimalFlashFringe = 50.f;

	// The timeline's tracks (CurveFloat_0 to _3), key for key.
	const FWasamiCurveKey GrowthKeys[] = {
		{0.f, 0.f, RCIM_Cubic, 1.5797574520111084f, 1.5797579288482666f},
		{0.992901086807251f, 0.9189547300338745f, RCIM_Cubic, 0.3629918098449707f, 0.36299169063568115f},
		{1.5134799480438232f, 0.9834880232810974f, RCIM_Linear, 0.f, 0.f},
	};
	const FWasamiCurveKey FadeKeys[] = {
		{-0.011600494384765625f, 0.f, RCIM_Cubic, -0.0950283631682396f, -0.09502881020307541f},
		{0.5f, 1.f, RCIM_Cubic, 4.5270514488220215f, 4.527058124542236f},
	};
	const FWasamiCurveKey DesaturationKeys[] = {
		{0.006957054138183594f, 0.015488147735595703f, RCIM_Cubic, 0.f, 0.f},
		{1.016563892364502f, 0.17036807537078857f, RCIM_Cubic, 0.6986591815948486f, 0.6986591815948486f},
		{1.4109413623809814f, 0.996394693851471f, RCIM_Linear, 0.f, 0.f},
	};
	const FWasamiCurveKey OpacityKeys[] = {
		{0.f, 1.f, RCIM_Cubic, -0.21745750308036804f, -0.21745805442333221f},
		{0.6773991584777832f, 0.725354790687561f, RCIM_Cubic, -0.5983153581619263f, -0.5983161926269531f},
		{1.592355489730835f, 0.002581477165222168f, RCIM_Linear, 0.f, 0.f},
	};
}

const FRichCurve& AWasamiPrimalPower::GrowthCurve()
{
	static const FRichCurve Curve = MakeCurve(GrowthKeys);
	return Curve;
}

const FRichCurve& AWasamiPrimalPower::DesaturationCurve()
{
	static const FRichCurve Curve = MakeCurve(DesaturationKeys);
	return Curve;
}

const FRichCurve& AWasamiPrimalPower::OpacityCurve()
{
	static const FRichCurve Curve = MakeCurve(OpacityKeys);
	return Curve;
}

const FRichCurve& AWasamiPrimalPower::PrimalFadeCurve()
{
	static const FRichCurve Curve = MakeCurve(FadeKeys);
	return Curve;
}

AWasamiPrimalPower::AWasamiPrimalPower()
{
	PostProcess->Settings.ColorGain = PrimalTintGain;
	PostProcess1->Settings.SceneFringeIntensity = PrimalFlashFringe;
	FadeCurve = PrimalFadeCurve();
	GrowthTrack = GrowthCurve();
	DesaturationTrack = DesaturationCurve();
	OpacityTrack = OpacityCurve();
	// The wave at pitch 1; M_05_Primal's own red.
}

void AWasamiPrimalPower::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	GetDefault<AWasamiPrimalPower>()->LoadDefaultAssets(Out);
}

int32 AWasamiPrimalPower::StunEnemies(const UObject* WorldContextObject, FVector Center, float Radius)
{
	// Pawn bodies only, no class filter, nothing ignored, and no line of sight. The original takes an actor that
	// implements DD_EnemyInterface or has the Enemy tag, then casts it to the interface: only the implementers are stunned.
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_Pawn)};
	TArray<AActor*> Found;
	UKismetSystemLibrary::SphereOverlapActors(WorldContextObject, Center, Radius, ObjectTypes, nullptr, TArray<AActor*>(), Found);
	int32 Stunned = 0;
	for (AActor* Each : Found)
	{
		if (Each && Each->Implements<UWasamiEnemyInterface>())
		{
			IWasamiEnemyInterface::Execute_SetState(Each, EWasamiEnemyState::Stun, false);
			++Stunned;
		}
	}
	return Stunned;
}

void AWasamiPrimalPower::StartPower()
{
	// The stun comes after the shake, around the player's capsule centre (where the burst now is).
	Super::StartPower();
	StunEnemies(this, GetActorLocation(), Range);
}
