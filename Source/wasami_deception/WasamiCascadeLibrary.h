#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WasamiCascadeLibrary.generated.h"

class UParticleSystem;

/**
 * Builds Cascade particle systems (UParticleSystem) for the pipeline, which writes the original's exported values into
 * them (Content/Python/wasami_tools/pipeline/dd_particles.py). Cascade keeps its emitters, LOD levels, modules and
 * distributions in properties Python cannot see, and its classes are not exposed to Python at all, so the structure is
 * made here and every value is written from UE's text form by the property's own name.
 * Editor only: the pipeline runs in the editor.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiCascadeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/**
	 * Empties System: its emitters, their LOD levels, modules and distributions leave the package (renamed into the
	 * transient package), and its emitter list, LOD distances and LOD settings are cleared.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static void ResetParticleSystem(UParticleSystem* System);

	/**
	 * Makes an object of the Cascade class named ClassName ('ParticleSpriteEmitter', 'ParticleLODLevel',
	 * 'ParticleModuleSize', 'DistributionFloatConstant') in Outer under Name, at its class defaults. An object already
	 * there under that name is kept when it is a default subobject of that class (the export's values are relative to
	 * its template) and renamed away otherwise (the export's are relative to the class defaults).
	 * Emitters and modules go in the system, LOD levels in their emitter, distributions in their module.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static UObject* MakeObject(UObject* Outer, const FString& ClassName, const FString& Name);

	/** Adds Emitter (made in System) after System's other emitters. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static bool AddEmitter(UParticleSystem* System, UObject* Emitter);

	/**
	 * Adds LODLevel (made in Emitter) after the emitter's other LOD levels, with its required and spawn modules and the
	 * rest of its modules in order (all made in the system; a module shared by LOD levels is passed to each).
	 * TypeDataModule, when given, makes the emitter a mesh (or beam, trail, GPU) emitter: a type data module made in the
	 * system, kept apart from Modules and the same one on every LOD level of the emitter.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static bool AddLODLevel(UObject* Emitter, UObject* LODLevel, UObject* RequiredModule, UObject* SpawnModule,
		const TArray<UObject*>& Modules, UObject* TypeDataModule = nullptr);

	/**
	 * Finishes System as Cascade does after an edit: the modules' LOD validity from where they are used, the LOD
	 * levels' module lists, the emitters' build data, the peak particle counts, the soloing state, PostEditChange.
	 * Distribution objects the modules no longer use (the ones a module makes for itself, replaced by a table or by
	 * another object) leave the package first.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static void FinishParticleSystem(UParticleSystem* System);

	/**
	 * Writes Object's property Name ('Rate', or 'ParamModes[1]' for one element of a fixed array) from UE's text form
	 * ('(MinValue=1.0,Table=(Op=1,...),Distribution=None)'). Returns '' when it is written, otherwise why not (the
	 * property is missing, a struct member is unknown, or the text is not taken whole and cleanly).
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static FString SetPropertyText(UObject* Object, const FString& Name, const FString& Text);

	/** Object's property Name in UE's text form ('' when there is no such property). */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static FString GetPropertyText(UObject* Object, const FString& Name);

	/** The C++ type of Object's property Name ('float', 'FRawDistributionFloat', 'TObjectPtr<UMaterialInterface>'). */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static FString GetPropertyType(UObject* Object, const FString& Name);

	/** System's emitters, in order. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static TArray<UObject*> GetEmitters(UParticleSystem* System);

	/** Emitter's LOD levels, in order. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static TArray<UObject*> GetLODLevels(UObject* Emitter);

	/** LODLevel's required module, spawn module, then the rest of its modules in order. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static TArray<UObject*> GetLODModules(UObject* LODLevel);

	/** LODLevel's type data module (None for a sprite emitter's). */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Cascade")
	static UObject* GetLODTypeDataModule(UObject* LODLevel);
#endif
};
