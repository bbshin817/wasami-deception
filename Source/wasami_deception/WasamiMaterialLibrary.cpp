#include "WasamiMaterialLibrary.h"

#if WITH_EDITOR
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"

void UWasamiMaterialLibrary::SetStaticComponentMask(UMaterialInstanceConstant* Instance, FName Name, bool R, bool G, bool B, bool A)
{
	if (!Instance)
	{
		return;
	}
	Instance->Modify();
	Instance->SetStaticComponentMaskParameterValueEditorOnly(FMaterialParameterInfo(Name), FStaticComponentMaskValue(R, G, B, A));
}

FString UWasamiMaterialLibrary::GetStaticComponentMask(UMaterialInterface* Material, FName Name)
{
	// UE 5.8's GetStaticComponentMaskParameterValue asks for a static switch of that name, so the value is read as a
	// static component mask here.
	FMaterialParameterMetadata Result;
	if (!Material || !Material->GetParameterValue(EMaterialParameterType::StaticComponentMask, FMemoryImageMaterialParameterInfo(Name), Result))
	{
		return TEXT("?");
	}
	const FStaticComponentMaskValue Mask = Result.Value.AsStaticComponentMask();
	FString Channels;
	if (Mask.R)
	{
		Channels += TEXT("R");
	}
	if (Mask.G)
	{
		Channels += TEXT("G");
	}
	if (Mask.B)
	{
		Channels += TEXT("B");
	}
	if (Mask.A)
	{
		Channels += TEXT("A");
	}
	return Channels;
}
#endif
