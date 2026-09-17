#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WasamiMaterialLibrary.generated.h"

class UMaterialInstanceConstant;
class UMaterialInterface;

/**
 * Material instance values the pipeline writes that Python cannot reach (Content/Python/wasami_tools/pipeline/dd_assets.py):
 * a static component mask parameter's override, with which the original's particle materials pick a texture channel
 * (MI_ky_flare01_primitiveG / R). MaterialEditingLibrary has the static switch but not the mask.
 * Editor only: the pipeline runs in the editor.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiMaterialLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/**
	 * Overrides Instance's static component mask parameter Name with the channels R, G, B, A. The static permutation is
	 * rebuilt by MaterialEditingLibrary.update_material_instance afterwards (as its static switch setter leaves it).
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Material")
	static void SetStaticComponentMask(UMaterialInstanceConstant* Instance, FName Name, bool R, bool G, bool B, bool A);

	/**
	 * The channels Material's static component mask parameter Name keeps ('R', 'GA'; '' for none), or '?' when Material
	 * has no such parameter.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Material")
	static FString GetStaticComponentMask(UMaterialInterface* Material, FName Name);
#endif
};
