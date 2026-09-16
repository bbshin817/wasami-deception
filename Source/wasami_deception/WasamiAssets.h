#pragma once

#include "CoreMinimal.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"

/**
 * The assets the pipeline makes (/Game/DD, /Game/Pipeline) are referenced softly and loaded when first used, never from
 * a constructor: whatever loads while the editor starts up joins the root set (AsyncLoading2 under GIsInitialLoad), and
 * the pipeline can then no longer rebuild it — deleting a rooted material's expressions asserts on !IsRooted().
 */
namespace WasamiAssets
{
	/** '/Game/A/Name' → the soft path of the asset in that package ('/Game/A/Name.Name'). */
	inline FSoftObjectPath Path(const TCHAR* Package)
	{
		const FString Name = FPackageName::GetShortName(Package);
		return FSoftObjectPath(FString::Printf(TEXT("%s.%s"), Package, *Name));
	}

	/** '/Game/A/BP_Name' → the soft path of the Blueprint's class ('/Game/A/BP_Name.BP_Name_C'). */
	inline FSoftObjectPath ClassPath(const TCHAR* Package)
	{
		const FString Name = FPackageName::GetShortName(Package);
		return FSoftObjectPath(FString::Printf(TEXT("%s.%s_C"), Package, *Name));
	}
}
