#include "Misc/AutomationTest.h"
#include "../WasamiCascadeLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Distributions/DistributionFloat.h"
#include "Distributions/DistributionVector.h"
#include "Math/RandomStream.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleSystem.h"
#include "Particles/Size/ParticleModuleSize.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/SubUV/ParticleModuleSubUV.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

namespace
{
	bool SetText(FAutomationTestBase& Test, UObject* Object, const TCHAR* Name, const TCHAR* Text)
	{
		const FString Error = UWasamiCascadeLibrary::SetPropertyText(Object, Name, Text);
		Test.TestTrue(FString::Printf(TEXT("%s takes %s (%s)"), Name, Text, *Error), Error.IsEmpty());
		return Error.IsEmpty();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCascadeBuildTest, "Wasami.Cascade.Build",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCascadeBuildTest::RunTest(const FString& Parameters)
{
	UParticleSystem* System = NewObject<UParticleSystem>(GetTransientPackage(), NAME_None, RF_Transient);
	UObject* Emitter = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleSpriteEmitter"), TEXT("ParticleSpriteEmitter_1"));
	UObject* Near = UWasamiCascadeLibrary::MakeObject(Emitter, TEXT("ParticleLODLevel"), TEXT("ParticleLODLevel_3"));
	UObject* Far = UWasamiCascadeLibrary::MakeObject(Emitter, TEXT("ParticleLODLevel"), TEXT("ParticleLODLevel_1"));
	UObject* Required = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModuleRequired"), TEXT("ParticleModuleRequired_1"));
	UObject* NearSpawn = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModuleSpawn"), TEXT("ParticleModuleSpawn_1"));
	UObject* FarSpawn = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModuleSpawn"), TEXT("ParticleModuleSpawn_4"));
	UObject* Size = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModuleSize"), TEXT("ParticleModuleSize_0"));
	UObject* SubUV = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModuleSubUV"), TEXT("ParticleModuleSubUV_4"));
	if (!TestTrue(TEXT("every object is made"), Emitter && Near && Far && Required && NearSpawn && FarSpawn && Size && SubUV))
	{
		return false;
	}
	AddExpectedError(TEXT("is not a Cascade class that can be made"), EAutomationExpectedErrorFlags::Contains, 2);
	AddExpectedError(TEXT("is not a module made in"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("an engine class that is not Cascade's is refused"),
		UWasamiCascadeLibrary::MakeObject(System, TEXT("StaticMeshComponent"), TEXT("Mesh")));
	TestNull(TEXT("an abstract Cascade class is refused"),
		UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModule"), TEXT("Module")));

	// The export's values as the cook left them: lookup tables, no distribution objects.
	SetText(*this, Emitter, TEXT("EmitterName"), TEXT("cutter"));
	SetText(*this, Far, TEXT("Level"), TEXT("1"));
	SetText(*this, NearSpawn, TEXT("Rate"), TEXT("(MinValue=10.0,MaxValue=10.0,Table=(Op=1,EntryCount=1,EntryStride=1,Values=(10.0)),Distribution=None)"));
	// A distribution the cook kept as an object (a spawn module's burst scale).
	UObject* BurstScale = UWasamiCascadeLibrary::MakeObject(NearSpawn, TEXT("DistributionFloatConstant"), TEXT("BurstScaleDistribution"));
	SetText(*this, BurstScale, TEXT("Constant"), TEXT("1.0"));
	SetText(*this, NearSpawn, TEXT("BurstScale"), *FString::Printf(TEXT("(Table=(),Distribution=\"%s\")"), *BurstScale->GetPathName()));
	SetText(*this, FarSpawn, TEXT("Rate"), TEXT("(MinValue=25.0,MaxValue=25.0,Table=(Op=1,EntryCount=1,EntryStride=1,Values=(25.0)),Distribution=None)"));
	SetText(*this, Size, TEXT("StartSize"), TEXT("(MinValue=5.0,MaxValue=25.0,MinValueVec=(X=5.0,Y=5.0,Z=5.0),MaxValueVec=(X=15.0,Y=15.0,Z=25.0),")
		TEXT("Table=(Op=2,EntryCount=1,EntryStride=6,SubEntryStride=3,Values=(5.0,5.0,5.0,15.0,15.0,25.0)),Distribution=None)"));
	SetText(*this, SubUV, TEXT("SubImageIndex"), TEXT("(MaxValue=15.0,Table=(TimeScale=15.0,Op=1,EntryCount=16,EntryStride=1,Values=(0.0,2.8058248,5.238904,")
		TEXT("7.3239365,9.089217,10.560319,11.764762,12.728793,13.479359,14.043178,14.446826,14.717292,14.880983,14.96496,14.995657,15.0)),Distribution=None)"));
	SetText(*this, Required, TEXT("InterpolationMethod"), TEXT("PSUVIM_Linear_Blend"));

	// A distribution object the module made for itself is not its template: a new one replaces it.
	UParticleModuleRequired* RequiredModule = Cast<UParticleModuleRequired>(Required);
	const UObject* DefaultSpawnRate = RequiredModule->SpawnRate.Distribution;
	UObject* SpawnRate = UWasamiCascadeLibrary::MakeObject(Required, TEXT("DistributionFloatConstant"), TEXT("RequiredDistributionSpawnRate"));
	TestTrue(TEXT("the module's own spawn rate distribution is replaced"), SpawnRate && SpawnRate != DefaultSpawnRate);
	TestTrue(TEXT("the replaced one has left the module"), DefaultSpawnRate && DefaultSpawnRate->GetOuter() != Required);
	SetText(*this, Required, TEXT("SpawnRate"), *FString::Printf(TEXT("(Distribution=\"%s\")"), *SpawnRate->GetPathName()));
	TestTrue(TEXT("the spawn rate points at the new distribution"), RequiredModule->SpawnRate.Distribution.Get() == SpawnRate);

	// Refusals.
	TestFalse(TEXT("a missing property"), UWasamiCascadeLibrary::SetPropertyText(Size, TEXT("NoSuchProperty"), TEXT("1")).IsEmpty());
	TestFalse(TEXT("a missing struct member"), UWasamiCascadeLibrary::SetPropertyText(Size, TEXT("StartSize"), TEXT("(NoSuchMember=1)")).IsEmpty());
	TestFalse(TEXT("text left over"), UWasamiCascadeLibrary::SetPropertyText(Emitter, TEXT("DetailModeBitmask"), TEXT("15 16")).IsEmpty());
	TestFalse(TEXT("an element past a fixed array"), UWasamiCascadeLibrary::SetPropertyText(Emitter, TEXT("DetailModeBitmask[1]"), TEXT("15")).IsEmpty());
	TestFalse(TEXT("a module outside the system"), UWasamiCascadeLibrary::AddLODLevel(Emitter, Near, Required, NearSpawn, { Emitter }));

	TestTrue(TEXT("the emitter is added"), UWasamiCascadeLibrary::AddEmitter(System, Emitter));
	TestTrue(TEXT("the near LOD level is added"), UWasamiCascadeLibrary::AddLODLevel(Emitter, Near, Required, NearSpawn, { Size, SubUV }));
	TestTrue(TEXT("the far LOD level is added"), UWasamiCascadeLibrary::AddLODLevel(Emitter, Far, Required, FarSpawn, { Size, SubUV }));
	UWasamiCascadeLibrary::FinishParticleSystem(System);

	TArray<UObject*> SizeChildren;
	GetObjectsWithOuter(Size, SizeChildren, EGetObjectsFlags::None);
	TestEqual(TEXT("the size module's own distribution has left it (its table is used)"), SizeChildren.Num(), 0);
	TestTrue(TEXT("a distribution object in use stays in its module"), BurstScale->GetOuter() == NearSpawn
		&& Cast<UParticleModuleSpawn>(NearSpawn)->BurstScale.Distribution.Get() == BurstScale);
	TestEqual(TEXT("the burst scale reads its object"), Cast<UParticleModuleSpawn>(NearSpawn)->BurstScale.GetValue(), 1.f);

	const UParticleEmitter* ParticleEmitter = Cast<UParticleEmitter>(Emitter);
	TestEqual(TEXT("one emitter"), System->Emitters.Num(), 1);
	TestEqual(TEXT("two LOD levels"), ParticleEmitter->LODLevels.Num(), 2);
	TestTrue(TEXT("the emitter's name"), ParticleEmitter->EmitterName == FName(TEXT("cutter")));
	TestEqual(TEXT("the shared required module is valid in both"), (int32)RequiredModule->LODValidity, 3);
	TestEqual(TEXT("the near spawn module in LOD 0"), (int32)Cast<UParticleModule>(NearSpawn)->LODValidity, 1);
	TestEqual(TEXT("the far spawn module in LOD 1"), (int32)Cast<UParticleModule>(FarSpawn)->LODValidity, 2);
	TestEqual(TEXT("the shared size module in both"), (int32)Cast<UParticleModule>(Size)->LODValidity, 3);
	const UParticleLODLevel* NearLevel = Cast<UParticleLODLevel>(Near);
	TestTrue(TEXT("the size module spawns"), NearLevel->SpawnModules.Contains(Cast<UParticleModule>(Size)));
	TestTrue(TEXT("the sub-UV module updates"), NearLevel->UpdateModules.Contains(Cast<UParticleModule>(SubUV)));
	TestTrue(TEXT("the read-back lists the required and spawn modules first"),
		UWasamiCascadeLibrary::GetLODModules(Near) == TArray<UObject*>({ Required, NearSpawn, Size, SubUV }));

	// The tables are what the particles read.
	UParticleModuleSpawn* NearSpawnModule = Cast<UParticleModuleSpawn>(NearSpawn);
	TestNull(TEXT("no spawn rate distribution object"), NearSpawnModule->Rate.Distribution.Get());
	TestEqual(TEXT("the near spawn rate"), NearSpawnModule->Rate.GetValue(), 10.f);
	TestEqual(TEXT("the far spawn rate"), Cast<UParticleModuleSpawn>(FarSpawn)->Rate.GetValue(), 25.f);
	UParticleModuleSize* SizeModule = Cast<UParticleModuleSize>(Size);
	FRandomStream Stream(7);
	for (int32 Sample = 0; Sample < 20; ++Sample)
	{
		const FVector Value = SizeModule->StartSize.GetValue(0.f, nullptr, 0, &Stream);
		TestTrue(TEXT("a start size inside the table's range"), Value.X >= 5. && Value.X <= 15. && Value.Y >= 5. && Value.Y <= 15.
			&& Value.Z >= 5. && Value.Z <= 25.);
	}
	TestEqual(TEXT("a frame index between two entries"), Cast<UParticleModuleSubUV>(SubUV)->SubImageIndex.GetValue(0.5f),
		(12.728793f + 13.479359f) / 2.f, 1e-4f);
	TestEqual(TEXT("the text reads back"), UWasamiCascadeLibrary::GetPropertyText(Required, TEXT("InterpolationMethod")),
		FString(TEXT("PSUVIM_Linear_Blend")));
	TestEqual(TEXT("the type of a distribution"), UWasamiCascadeLibrary::GetPropertyType(Size, TEXT("StartSize")),
		FString(TEXT("FRawDistributionVector")));

	// A rebuild starts from nothing, and the old names are free again.
	UWasamiCascadeLibrary::ResetParticleSystem(System);
	TestEqual(TEXT("no emitters after a reset"), System->Emitters.Num(), 0);
	TestTrue(TEXT("the old emitter has left the system"), Emitter->GetPackage() != System->GetPackage() || Emitter->GetOuter() != System);
	TestTrue(TEXT("the old module has left the system"), Size->GetOuter() != System);
	UObject* Again = UWasamiCascadeLibrary::MakeObject(System, TEXT("ParticleModuleSize"), TEXT("ParticleModuleSize_0"));
	TestTrue(TEXT("a module of an old name is made anew"), Again && Again != Size && Again->GetFName() == FName(TEXT("ParticleModuleSize_0")));
	return true;
}

#endif
