#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "../WasamiCutsceneNurse.h"
#include "../WasamiEnemy.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCutsceneNurseTest, "Wasami.Cutscene.Nurse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCutsceneNurseTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		AddError(TEXT("no test world"));
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiCutsceneNurse* Nurse = World->SpawnActor<AWasamiCutsceneNurse>(FVector(0., 0., 800.), FRotator(0., 30., 0.));
	if (!TestNotNull(TEXT("the nurse is spawned"), Nurse))
	{
		return false;
	}
	USkeletalMeshComponent* Body = Nurse->GetSkeletalMesh();
	TestNotNull(TEXT("it has a mesh"), Body);
	// The sequences' bindings find it by the name BP_06_Nurse_Cutscene gives its mesh; the root is what their
	// transform tracks move, so the mesh can sit under it.
	TestEqual(TEXT("named as the original's"), Body->GetName(), FString(TEXT("SkeletalMesh")));
	TestTrue(TEXT("under the root"), Body->GetAttachParent() == Nurse->GetRootComponent());
	TestTrue(TEXT("the root is not the mesh"), Nurse->GetRootComponent() != Body);
	TestEqual(TEXT("grown to the nurse's height"), Body->GetRelativeScale3D().X, AWasamiEnemy::MeshScale, 1e-6);
	TestTrue(TEXT("it collides with nothing"), Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	// Zone 2's nurses are SkeletalMeshActors: the mesh stands at the actor.
	TestFalse(TEXT("at the actor by default"), Nurse->IsUnderCapsule());
	TestEqual(TEXT("no offset"), Body->GetRelativeLocation().Z, 0., 1e-6);
	TestEqual(TEXT("no turn"), Body->GetRelativeRotation().Yaw, 0., 1e-6);

	// Zone 1's are a Character: the mesh hangs under the capsule's centre, turned as its Blueprint turns it.
	Nurse->SetUnderCapsule(true);
	TestTrue(TEXT("under the capsule now"), Nurse->IsUnderCapsule());
	TestEqual(TEXT("hung under the capsule's centre"), Body->GetRelativeLocation().Z, AWasamiEnemy::MeshZ, 1e-6);
	TestEqual(TEXT("turned as the nurse's is"), Body->GetRelativeRotation().Yaw, AWasamiEnemy::MeshYaw, 1e-6);
	TestEqual(TEXT("the actor itself is where it was put"), Nurse->GetActorLocation().Z, 800., 1e-6);

	Nurse->SetUnderCapsule(false);
	TestEqual(TEXT("and back"), Body->GetRelativeLocation().Z, 0., 1e-6);
	return true;
}

#endif
