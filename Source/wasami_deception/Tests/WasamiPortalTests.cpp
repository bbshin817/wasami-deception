#include "Misc/AutomationTest.h"
#include "../WasamiPortal.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystemComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString MaterialName(const UStaticMeshComponent* Plane)
	{
		const UMaterialInterface* Material = Plane->GetMaterial(0);
		return Material ? Material->GetName() : FString();
	}

	/** A portal spawned locked or open, masked or not (as the level places it: its values set before it is built). */
	AWasamiPortal* SpawnPortal(UWorld* World, bool bLocked, bool bMasked)
	{
		AWasamiPortal* Portal = World->SpawnActorDeferred<AWasamiPortal>(AWasamiPortal::StaticClass(), FTransform::Identity);
		if (Portal)
		{
			Portal->bLocked = bLocked;
			Portal->bMaskedPortalMaterial = bMasked;
			Portal->FinishSpawning(FTransform::Identity);
		}
		return Portal;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPortalActorTest, "Wasami.Portal.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPortalActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiPortal* Portal = SpawnPortal(World, false, false);
	if (!TestNotNull(TEXT("the portal"), Portal))
	{
		return false;
	}

	// BP_00_Teleport's SCS: the disc a metre up at 70 times its size, the rings on it, the logo and the lock on the inner.
	UStaticMeshComponent* Vortex = Portal->GetVortex();
	TestTrue(TEXT("the vortex on the root"), Vortex->GetAttachParent() == Portal->GetRootComponent());
	TestTrue(TEXT("a metre up"), Vortex->GetRelativeLocation().Equals(FVector(-1.309136152267456, 0., 100.), 1e-3));
	TestTrue(TEXT("70 times its size"), Vortex->GetRelativeScale3D().Equals(FVector(70.), 1e-3));
	TestTrue(TEXT("the outer on the vortex"), Portal->GetOuter()->GetAttachParent() == Vortex);
	TestTrue(TEXT("the inner on the outer"), Portal->GetInner()->GetAttachParent() == Portal->GetOuter());
	TestTrue(TEXT("the logo on the inner"), Portal->GetLogo()->GetAttachParent() == Portal->GetInner());
	TestTrue(TEXT("the lock on the inner"), Portal->GetLogoLock()->GetAttachParent() == Portal->GetInner());
	TestTrue(TEXT("the sounds and bursts on the logo"), Portal->GetPortalLoop()->GetAttachParent() == Portal->GetLogo()
		&& Portal->GetAudio()->GetAttachParent() == Portal->GetLogo()
		&& Portal->GetPortalAppear()->GetAttachParent() == Portal->GetLogo()
		&& Portal->GetPortalAppearLock()->GetAttachParent() == Portal->GetLogo());
	for (const UStaticMeshComponent* Plane : {Vortex, Portal->GetOuter(), Portal->GetInner()})
	{
		TestTrue(TEXT("the disc"), Plane->GetStaticMesh() && Plane->GetStaticMesh()->GetName() == TEXT("circle_portal_decal"));
		TestFalse(TEXT("no shadow"), Plane->CastShadow);
		TestTrue(TEXT("colliding with nothing"), Plane->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	TestTrue(TEXT("the logo's plane"), Portal->GetLogo()->GetStaticMesh() && Portal->GetLogo()->GetStaticMesh()->GetName() == TEXT("Plane"));
	TestEqual(TEXT("this game's logo"), MaterialName(Portal->GetLogo()), FString(TEXT("MI_Portal_Wasami")));
	TestEqual(TEXT("the lock's material"), MaterialName(Portal->GetLogoLock()), FString(TEXT("M_00_Portal_Lock")));
	const UBoxComponent* Box = Portal->GetCollision();
	TestTrue(TEXT("the box blocks nothing"), Box->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
		&& Box->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Overlap);

	// Open (UserConstructionScript): the logo at 0.02, no lock, the open materials, a red light, the loop at half.
	TestTrue(TEXT("the logo at 0.02"), Portal->GetLogo()->GetRelativeScale3D().Equals(FVector(0.02), 1e-6));
	TestFalse(TEXT("no lock"), Portal->GetLogoLock()->IsVisible());
	TestEqual(TEXT("the vortex open"), MaterialName(Vortex), FString(TEXT("M_00_Portal_Vortex_Inst")));
	TestEqual(TEXT("the outer open"), MaterialName(Portal->GetOuter()), FString(TEXT("M_00_Portal_Vortex_Outer_Inst")));
	TestEqual(TEXT("the inner open"), MaterialName(Portal->GetInner()), FString(TEXT("M_00_Portal_Vortex_Inner_Inst")));
	TestTrue(TEXT("a red light"), Portal->GetStrobingLight()->LightColor == FLinearColor(1.f, 0.f, 0.f).ToFColor(true));
	TestEqual(TEXT("the loop at half"), Portal->GetPortalLoop()->VolumeMultiplier, 0.5f);
	TestTrue(TEXT("the loop's sound"), Portal->GetPortalLoop()->Sound && Portal->GetPortalLoop()->Sound->GetName() == TEXT("Portal_Sound_v3"));
	TestTrue(TEXT("the unlock's sound"), Portal->GetAudio()->Sound && Portal->GetAudio()->Sound->GetName() == TEXT("portal_unlocked"));

	// BP_00_StrobingLight's Strobe: 0.5, up to 1 at 1 s, back to 0.5 at 2 s, looping; x Light Intensity.
	TestEqual(TEXT("0.5 at the start"), AWasamiPortal::EvaluateStrobe(0.f), 0.5f);
	TestEqual(TEXT("1 at 1 s"), AWasamiPortal::EvaluateStrobe(1.f), 1.f);
	TestEqual(TEXT("0.5 again at 2 s"), AWasamiPortal::EvaluateStrobe(1.9999f), 0.5f, 1e-3f);
	TestEqual(TEXT("looping"), AWasamiPortal::EvaluateStrobe(3.f), 1.f);
	const float Rising = AWasamiPortal::EvaluateStrobe(0.5f);
	TestTrue(TEXT("rising between"), Rising > 0.5f && Rising < 1.f);
	// 1 s in ticks of 0.1 s (a test world cuts a longer tick short).
	for (int32 Tick = 0; Tick < 10; ++Tick)
	{
		Wrapper.TickTestWorld(0.1f);
	}
	TestEqual(TEXT("the light strobes"), Portal->GetStrobingLight()->Intensity, Portal->StrobingLightIntensity, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPortalLockTest, "Wasami.Portal.Lock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPortalLockTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	// Placed as the hotel's exit: locked and masked.
	AWasamiPortal* Portal = SpawnPortal(World, true, true);
	if (!TestNotNull(TEXT("the portal"), Portal))
	{
		return false;
	}
	auto TestLocked = [this, Portal](const TCHAR* When)
	{
		TestTrue(FString::Printf(TEXT("%s: no logo"), When), Portal->GetLogo()->GetRelativeScale3D().IsZero());
		TestTrue(FString::Printf(TEXT("%s: the lock"), When), Portal->GetLogoLock()->IsVisible());
		TestEqual(FString::Printf(TEXT("%s: the vortex locked, masked"), When), MaterialName(Portal->GetVortex()),
			FString(TEXT("M_00_Portal_Vortex_Locked_Inst_Masked")));
		TestEqual(FString::Printf(TEXT("%s: the outer locked"), When), MaterialName(Portal->GetOuter()),
			FString(TEXT("M_00_Portal_Vortex_Outer_Locked_Inst")));
		TestEqual(FString::Printf(TEXT("%s: the inner locked"), When), MaterialName(Portal->GetInner()),
			FString(TEXT("M_00_Portal_Vortex_Inner_Locked_Inst")));
		TestTrue(FString::Printf(TEXT("%s: Locked Light Color"), When),
			Portal->GetStrobingLight()->LightColor == Portal->LockedLightColor.ToFColor(true));
		TestEqual(FString::Printf(TEXT("%s: the loop silent"), When), Portal->GetPortalLoop()->VolumeMultiplier, 0.f);
	};
	TestLocked(TEXT("placed"));

	// Lock/Unlock(False, True), as the zone opens it.
	Portal->LockUnlock(false, true);
	TestFalse(TEXT("open"), Portal->bLocked);
	TestTrue(TEXT("the logo at 0.02"), Portal->GetLogo()->GetRelativeScale3D().Equals(FVector(0.02), 1e-6));
	TestFalse(TEXT("no lock"), Portal->GetLogoLock()->IsVisible());
	TestEqual(TEXT("the vortex open, masked"), MaterialName(Portal->GetVortex()), FString(TEXT("M_00_Portal_Vortex_Masked")));
	TestEqual(TEXT("the outer open"), MaterialName(Portal->GetOuter()), FString(TEXT("M_00_Portal_Vortex_Outer_Inst")));
	TestTrue(TEXT("Lock/Unlock's red"), Portal->GetStrobingLight()->LightColor == FLinearColor(1.f, 0.0152f, 0.f).ToFColor(true));
	TestEqual(TEXT("the loop at half"), Portal->GetPortalLoop()->VolumeMultiplier, 0.5f);

	Portal->LockUnlock(true, false);
	TestTrue(TEXT("locked again"), Portal->bLocked);
	TestLocked(TEXT("locked again"));
	return true;
}

#endif
