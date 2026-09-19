#include "WasamiCascadeLibrary.h"

#if WITH_EDITOR
#include "Distributions/Distribution.h"
#include "Engine/InterpCurveEdSetup.h"
#include "Misc/StringOutputDevice.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleModuleRequired.h"
#include "ParticleEmitterInstances.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/TypeData/ParticleModuleTypeDataBase.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiCascade, Log, All);

namespace
{
	/** The classes MakeObject makes: Cascade's own. */
	bool IsCascadeClass(const UClass* Class)
	{
		return Class->IsChildOf<UParticleEmitter>() || Class->IsChildOf<UParticleLODLevel>()
			|| Class->IsChildOf<UParticleModule>() || Class->IsChildOf<UDistribution>();
	}

	/**
	 * Moves Object out of its package under a new name, so that saving the package leaves it behind and its old name
	 * is free again (a module has to stay within a particle system: the engine gives it a transient one).
	 */
	void RenameAway(UObject* Object)
	{
		UObject* Outer = GetTransientOuterForRename(Object->GetClass());
		Object->Rename(*MakeUniqueObjectName(Outer, Object->GetClass()).ToString(), Outer,
			REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
	}

	/** 'ParamModes[1]' → ('ParamModes', 1); 'Rate' → ('Rate', 0). False when the brackets are malformed. */
	bool SplitName(const FString& Name, FString& OutProperty, int32& OutIndex)
	{
		OutIndex = 0;
		int32 Open;
		if (!Name.FindChar(TEXT('['), Open))
		{
			OutProperty = Name;
			return true;
		}
		if (!Name.EndsWith(TEXT("]")))
		{
			return false;
		}
		OutProperty = Name.Left(Open);
		const FString Digits = Name.Mid(Open + 1, Name.Len() - Open - 2);
		if (Digits.IsEmpty() || !Digits.IsNumeric())
		{
			return false;
		}
		OutIndex = FCString::Atoi(*Digits);
		return true;
	}

	void SkipSpaces(const TCHAR*& Cursor)
	{
		while (*Cursor == TEXT(' ') || *Cursor == TEXT('\t'))
		{
			++Cursor;
		}
	}

	/** Moves past one value (a number, a name, a quoted string, a parenthesised list) up to its ',' or ')'. */
	bool SkipValue(const TCHAR*& Cursor, FString& Error)
	{
		int32 Depth = 0;
		while (*Cursor)
		{
			const TCHAR C = *Cursor;
			if (C == TEXT('"'))
			{
				++Cursor;
				while (*Cursor && *Cursor != TEXT('"'))
				{
					Cursor += (*Cursor == TEXT('\\') && Cursor[1]) ? 2 : 1;
				}
				if (!*Cursor)
				{
					Error = TEXT("an unterminated quoted string");
					return false;
				}
			}
			else if (C == TEXT('('))
			{
				++Depth;
			}
			else if (C == TEXT(')') || C == TEXT(','))
			{
				if (Depth == 0)
				{
					return true;
				}
				if (C == TEXT(')'))
				{
					--Depth;
				}
			}
			++Cursor;
		}
		return Depth == 0;
	}

	bool CheckStructText(const UStruct* Struct, const TCHAR*& Cursor, FString& Error);

	/** Whether a struct's text is checked member by member (a struct with its own text import is left to it). */
	bool IsCheckedStruct(const FProperty* Property)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		return StructProperty && !(StructProperty->Struct->StructFlags & STRUCT_ImportTextItemNative);
	}

	/** Checks each element of an array of Struct's text ('((A=1),(A=2))') with CheckStructText. */
	bool CheckStructArrayText(const UStruct* Struct, const TCHAR*& Cursor, FString& Error)
	{
		SkipSpaces(Cursor);
		if (*Cursor != TEXT('('))
		{
			Error = FString::Printf(TEXT("the text of an array of %s does not open with '('"), *Struct->GetName());
			return false;
		}
		++Cursor;
		for (;;)
		{
			SkipSpaces(Cursor);
			if (*Cursor == TEXT(')'))
			{
				++Cursor;
				return true;
			}
			if (!CheckStructText(Struct, Cursor, Error))
			{
				return false;
			}
			SkipSpaces(Cursor);
			if (*Cursor == TEXT(','))
			{
				++Cursor;
			}
			else if (*Cursor != TEXT(')'))
			{
				Error = FString::Printf(TEXT("the text of an array of %s does not close"), *Struct->GetName());
				return false;
			}
		}
	}

	/**
	 * Checks that every member a struct's text names ('(A=1,B=(C=2))') is a property of Struct, down through nested
	 * structs and arrays of them: UE's own import passes over unknown members without a word (it reports them at
	 * LogExec's Verbose).
	 */
	bool CheckStructText(const UStruct* Struct, const TCHAR*& Cursor, FString& Error)
	{
		SkipSpaces(Cursor);
		if (*Cursor != TEXT('('))
		{
			return SkipValue(Cursor, Error);
		}
		++Cursor;
		for (;;)
		{
			SkipSpaces(Cursor);
			if (*Cursor == TEXT(')'))
			{
				++Cursor;
				return true;
			}
			const TCHAR* NameStart = Cursor;
			while (*Cursor && *Cursor != TEXT('=') && *Cursor != TEXT('[') && *Cursor != TEXT(' '))
			{
				++Cursor;
			}
			const FString MemberName(UE_PTRDIFF_TO_INT32(Cursor - NameStart), NameStart);
			const FProperty* Member = FindFProperty<FProperty>(Struct, FName(*MemberName));
			if (!Member)
			{
				Error = FString::Printf(TEXT("%s has no member '%s'"), *Struct->GetName(), *MemberName);
				return false;
			}
			if (*Cursor == TEXT('['))
			{
				while (*Cursor && *Cursor != TEXT(']'))
				{
					++Cursor;
				}
				if (*Cursor)
				{
					++Cursor;
				}
			}
			SkipSpaces(Cursor);
			if (*Cursor != TEXT('='))
			{
				Error = FString::Printf(TEXT("no '=' after %s.%s"), *Struct->GetName(), *MemberName);
				return false;
			}
			++Cursor;
			SkipSpaces(Cursor);
			const FArrayProperty* ArrayMember = CastField<FArrayProperty>(Member);
			bool bChecked;
			if (IsCheckedStruct(Member) && *Cursor == TEXT('('))
			{
				bChecked = CheckStructText(CastField<FStructProperty>(Member)->Struct, Cursor, Error);
			}
			else if (ArrayMember && IsCheckedStruct(ArrayMember->Inner) && *Cursor == TEXT('('))
			{
				bChecked = CheckStructArrayText(CastField<FStructProperty>(ArrayMember->Inner)->Struct, Cursor, Error);
			}
			else
			{
				bChecked = SkipValue(Cursor, Error);
			}
			if (!bChecked)
			{
				return false;
			}
			SkipSpaces(Cursor);
			if (*Cursor == TEXT(','))
			{
				++Cursor;
			}
			else if (*Cursor != TEXT(')'))
			{
				Error = FString::Printf(TEXT("the text of %s does not close"), *Struct->GetName());
				return false;
			}
		}
	}

	void CollectReferencedObjects(const UStruct* Struct, const void* Container, TSet<UObject*>& Used);

	/** The objects one value refers to: itself for an object, and down through structs and arrays. */
	void CollectReferencedObjectsInValue(const FProperty* Property, const void* Value, TSet<UObject*>& Used)
	{
		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			Used.Add(ObjectProperty->GetObjectPropertyValue(Value));
		}
		else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			CollectReferencedObjects(StructProperty->Struct, Value, Used);
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Array(ArrayProperty, Value);
			for (int32 Index = 0; Index < Array.Num(); ++Index)
			{
				CollectReferencedObjectsInValue(ArrayProperty->Inner, Array.GetRawPtr(Index), Used);
			}
		}
	}

	/** The objects Container's properties refer to (a raw distribution's Distribution among them). */
	void CollectReferencedObjects(const UStruct* Struct, const void* Container, TSet<UObject*>& Used)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			for (int32 Index = 0; Index < It->ArrayDim; ++Index)
			{
				CollectReferencedObjectsInValue(*It, It->ContainerPtrToValuePtr<void>(Container, Index), Used);
			}
		}
	}

	/**
	 * Moves out the distribution objects Module holds but no longer uses: a module makes its own when it is made, and a
	 * table written over one (with Distribution=None) leaves it behind. A distribution counts as used wherever the
	 * module's values refer to it, an array of structs included (a dynamic parameter's ParamValue).
	 */
	void RenameAwayUnusedDistributions(UParticleModule* Module)
	{
		TSet<UObject*> Used;
		CollectReferencedObjects(Module->GetClass(), Module, Used);
		TArray<UObject*> Children;
		GetObjectsWithOuter(Module, Children, EGetObjectsFlags::None);
		for (UObject* Child : Children)
		{
			if (Child->IsA<UDistribution>() && !Used.Contains(Child))
			{
				RenameAway(Child);
			}
		}
	}

	FProperty* FindProperty(const UObject* Object, const FString& Name, int32& OutIndex, FString& Error)
	{
		FString PropertyName;
		if (!Object)
		{
			Error = TEXT("no object");
			return nullptr;
		}
		if (!SplitName(Name, PropertyName, OutIndex))
		{
			Error = FString::Printf(TEXT("malformed property name '%s'"), *Name);
			return nullptr;
		}
		FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (!Property)
		{
			Error = FString::Printf(TEXT("%s has no property '%s'"), *Object->GetClass()->GetName(), *PropertyName);
			return nullptr;
		}
		if (OutIndex < 0 || OutIndex >= Property->ArrayDim)
		{
			Error = FString::Printf(TEXT("%s.%s has no element %d"), *Object->GetClass()->GetName(), *PropertyName, OutIndex);
			return nullptr;
		}
		return Property;
	}
}

void UWasamiCascadeLibrary::ResetParticleSystem(UParticleSystem* System)
{
	if (!System)
	{
		return;
	}
	System->PreEditChange(nullptr);
	TArray<UObject*> Children;
	GetObjectsWithOuter(System, Children, EGetObjectsFlags::None);
	for (UObject* Child : Children)
	{
		if (!IsCascadeClass(Child->GetClass()))
		{
			continue;
		}
		// Cascade's curve editor keeps the distributions it has shown; a saved reference to one that left the package
		// would fail the save.
		if (System->CurveEdSetup)
		{
			TArray<UObject*> Nested;
			GetObjectsWithOuter(Child, Nested, EGetObjectsFlags::IncludeNestedObjects);
			Nested.Add(Child);
			for (UObject* Curve : Nested)
			{
				System->CurveEdSetup->RemoveCurve(Curve);
			}
		}
		RenameAway(Child);
	}
	System->Emitters.Empty();
	System->LODDistances.Empty();
	System->LODSettings.Empty();
	System->MarkPackageDirty();
}

UObject* UWasamiCascadeLibrary::MakeObject(UObject* Outer, const FString& ClassName, const FString& Name)
{
	if (!Outer)
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("MakeObject %s: no outer"), *ClassName);
		return nullptr;
	}
	UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass | EFindFirstObjectOptions::NativeFirst);
	if (!Class || !IsCascadeClass(Class) || Class->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("MakeObject: '%s' is not a Cascade class that can be made"), *ClassName);
		return nullptr;
	}
	const FName ObjectName = Name.IsEmpty() ? NAME_None : FName(*Name);
	if (ObjectName != NAME_None)
	{
		if (UObject* Existing = StaticFindObjectFast(nullptr, Outer, ObjectName))
		{
			if (Existing->GetClass() == Class && Existing->HasAnyFlags(RF_DefaultSubObject))
			{
				return Existing;
			}
			RenameAway(Existing);
		}
	}
	return NewObject<UObject>(Outer, Class, ObjectName, RF_Transactional);
}

bool UWasamiCascadeLibrary::AddEmitter(UParticleSystem* System, UObject* Emitter)
{
	UParticleEmitter* ParticleEmitter = Cast<UParticleEmitter>(Emitter);
	if (!System || !ParticleEmitter || ParticleEmitter->GetOuter() != System)
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("AddEmitter: %s is not an emitter made in %s"), *GetNameSafe(Emitter), *GetNameSafe(System));
		return false;
	}
	System->Emitters.Add(ParticleEmitter);
	return true;
}

bool UWasamiCascadeLibrary::AddLODLevel(UObject* Emitter, UObject* LODLevel, UObject* RequiredModule, UObject* SpawnModule,
	const TArray<UObject*>& Modules, UObject* TypeDataModule)
{
	UParticleEmitter* ParticleEmitter = Cast<UParticleEmitter>(Emitter);
	UParticleLODLevel* Level = Cast<UParticleLODLevel>(LODLevel);
	UParticleModuleRequired* Required = Cast<UParticleModuleRequired>(RequiredModule);
	UParticleModuleSpawn* Spawn = Cast<UParticleModuleSpawn>(SpawnModule);
	if (!ParticleEmitter || !Level || Level->GetOuter() != ParticleEmitter || !Required || !Spawn)
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("AddLODLevel: needs an emitter, a LOD level made in it, a required and a spawn module (got %s, %s, %s, %s)"),
			*GetNameSafe(Emitter), *GetNameSafe(LODLevel), *GetNameSafe(RequiredModule), *GetNameSafe(SpawnModule));
		return false;
	}
	const UObject* System = ParticleEmitter->GetOuter();
	TArray<TObjectPtr<UParticleModule>> LevelModules;
	for (UObject* Object : Modules)
	{
		UParticleModule* Module = Cast<UParticleModule>(Object);
		if (!Module || Module->GetOuter() != System || Module->IsA<UParticleModuleRequired>() || Module->IsA<UParticleModuleSpawn>()
			|| Module->IsA<UParticleModuleTypeDataBase>())
		{
			UE_LOG(LogWasamiCascade, Error, TEXT("AddLODLevel: %s is not a module made in %s"), *GetNameSafe(Object), *GetNameSafe(System));
			return false;
		}
		LevelModules.Add(Module);
	}
	if (Required->GetOuter() != System || Spawn->GetOuter() != System)
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("AddLODLevel: the required and spawn modules must be made in %s"), *GetNameSafe(System));
		return false;
	}
	UParticleModuleTypeDataBase* TypeData = Cast<UParticleModuleTypeDataBase>(TypeDataModule);
	if (TypeDataModule && (!TypeData || TypeData->GetOuter() != System))
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("AddLODLevel: %s is not a type data module made in %s"), *GetNameSafe(TypeDataModule), *GetNameSafe(System));
		return false;
	}
	// The engine takes an emitter's LOD levels to share one type data module (UParticleLODLevel::GenerateFromLODLevel).
	if (ParticleEmitter->LODLevels.Num() > 0 && ParticleEmitter->LODLevels[0] && ParticleEmitter->LODLevels[0]->TypeDataModule != TypeData)
	{
		UE_LOG(LogWasamiCascade, Error, TEXT("AddLODLevel: %s's type data module %s differs from its first LOD level's %s"),
			*GetNameSafe(LODLevel), *GetNameSafe(TypeDataModule), *GetNameSafe(ParticleEmitter->LODLevels[0]->TypeDataModule.Get()));
		return false;
	}
	Level->RequiredModule = Required;
	Level->SpawnModule = Spawn;
	Level->Modules = MoveTemp(LevelModules);
	Level->TypeDataModule = TypeData;
	ParticleEmitter->LODLevels.Add(Level);
	return true;
}

void UWasamiCascadeLibrary::FinishParticleSystem(UParticleSystem* System)
{
	if (!System)
	{
		return;
	}
	TArray<UObject*> Children;
	GetObjectsWithOuter(System, Children, EGetObjectsFlags::None);
	for (UObject* Child : Children)
	{
		if (UParticleModule* Module = Cast<UParticleModule>(Child))
		{
			RenameAwayUnusedDistributions(Module);
		}
	}
	System->SetupLODValidity();
	for (UParticleEmitter* Emitter : System->Emitters)
	{
		if (Emitter)
		{
			// The LOD levels' spawn / update lists, then the emitter's build data.
			Emitter->UpdateModuleLists();
		}
	}
	System->UpdateAllModuleLists();
	System->CalculateMaxActiveParticleCounts();
	System->SetupSoloing();
	System->PostEditChange();
	System->MarkPackageDirty();
}

FString UWasamiCascadeLibrary::SetPropertyText(UObject* Object, const FString& Name, const FString& Text)
{
	int32 Index = 0;
	FString Error;
	const FProperty* Property = FindProperty(Object, Name, Index, Error);
	if (!Property)
	{
		return Error;
	}
	const TCHAR* Cursor = *Text;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
	bool bChecked = true;
	if (IsCheckedStruct(Property))
	{
		bChecked = CheckStructText(CastField<FStructProperty>(Property)->Struct, Cursor, Error);
	}
	else if (ArrayProperty && IsCheckedStruct(ArrayProperty->Inner))
	{
		bChecked = CheckStructArrayText(CastField<FStructProperty>(ArrayProperty->Inner)->Struct, Cursor, Error);
	}
	if (!bChecked)
	{
		return FString::Printf(TEXT("%s.%s: %s"), *Object->GetName(), *Name, *Error);
	}
	FStringOutputDevice Errors;
	void* Value = Property->ContainerPtrToValuePtr<void>(Object, Index);
	const TCHAR* Rest = Property->ImportText_Direct(*Text, Value, Object, PPF_None, &Errors);
	if (!Rest)
	{
		return FString::Printf(TEXT("%s.%s did not take '%s': %s"), *Object->GetName(), *Name, *Text, *Errors);
	}
	if (!FString(Rest).TrimStartAndEnd().IsEmpty() || !Errors.IsEmpty())
	{
		return FString::Printf(TEXT("%s.%s took '%s' only in part (left '%s'): %s"), *Object->GetName(), *Name, *Text, Rest, *Errors);
	}
	return FString();
}

FString UWasamiCascadeLibrary::GetPropertyText(UObject* Object, const FString& Name)
{
	int32 Index = 0;
	FString Error;
	const FProperty* Property = FindProperty(Object, Name, Index, Error);
	if (!Property)
	{
		return FString();
	}
	FString Text;
	Property->ExportTextItem_Direct(Text, Property->ContainerPtrToValuePtr<void>(Object, Index), nullptr, Object, PPF_None);
	return Text;
}

FString UWasamiCascadeLibrary::GetPropertyType(UObject* Object, const FString& Name)
{
	int32 Index = 0;
	FString Error;
	const FProperty* Property = FindProperty(Object, Name, Index, Error);
	if (!Property)
	{
		return FString();
	}
	// A container's element type comes separately ('TArray' + '<float>').
	FString Extended;
	const FString Type = Property->GetCPPType(&Extended);
	return Type + Extended;
}

TArray<UObject*> UWasamiCascadeLibrary::GetEmitters(UParticleSystem* System)
{
	TArray<UObject*> Out;
	if (System)
	{
		for (UParticleEmitter* Emitter : System->Emitters)
		{
			Out.Add(Emitter);
		}
	}
	return Out;
}

TArray<UObject*> UWasamiCascadeLibrary::GetLODLevels(UObject* Emitter)
{
	TArray<UObject*> Out;
	if (const UParticleEmitter* ParticleEmitter = Cast<UParticleEmitter>(Emitter))
	{
		for (UParticleLODLevel* Level : ParticleEmitter->LODLevels)
		{
			Out.Add(Level);
		}
	}
	return Out;
}

TArray<UObject*> UWasamiCascadeLibrary::GetLODModules(UObject* LODLevel)
{
	TArray<UObject*> Out;
	if (const UParticleLODLevel* Level = Cast<UParticleLODLevel>(LODLevel))
	{
		Out.Add(Level->RequiredModule);
		Out.Add(Level->SpawnModule);
		for (UParticleModule* Module : Level->Modules)
		{
			Out.Add(Module);
		}
	}
	return Out;
}

UObject* UWasamiCascadeLibrary::GetLODTypeDataModule(UObject* LODLevel)
{
	const UParticleLODLevel* Level = Cast<UParticleLODLevel>(LODLevel);
	return Level ? Level->TypeDataModule.Get() : nullptr;
}

FString UWasamiCascadeLibrary::DescribeEmitterInstances(UParticleSystemComponent* Component)
{
	if (!Component)
	{
		return FString();
	}
	FString Out;
	for (FParticleEmitterInstance* Instance : Component->EmitterInstances)
	{
		if (!Instance || !Instance->SpriteTemplate)
		{
			Out += TEXT("(no instance)\n");
			continue;
		}
		const UParticleLODLevel* Level = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
		Out += FString::Printf(TEXT("%s enabled=%d lod=%d lodEnabled=%d spawnModules=%d updateModules=%d receivers=%d generator=%d active=%d max=%d time=%.3f"),
			*Instance->SpriteTemplate->EmitterName.ToString(), Instance->bEnabled ? 1 : 0, Instance->CurrentLODLevelIndex,
			Level ? (Level->bEnabled ? 1 : 0) : -1, Level ? Level->SpawnModules.Num() : -1, Level ? Level->UpdateModules.Num() : -1,
			Level ? Level->EventReceiverModules.Num() : -1, Level && Level->EventGenerator ? 1 : 0, Instance->ActiveParticles,
			Instance->MaxActiveParticles, Instance->EmitterTime);
		if (Instance->ActiveParticles > 0 && Instance->ParticleData && Instance->ParticleIndices)
		{
			const FBaseParticle* Particle = reinterpret_cast<const FBaseParticle*>(
				Instance->ParticleData + Instance->ParticleStride * Instance->ParticleIndices[0]);
			Out += FString::Printf(TEXT(" first: relativeTime=%.3f oneOverMaxLifetime=%.3f"), Particle->RelativeTime,
				Particle->OneOverMaxLifetime);
		}
		Out += TEXT("\n");
	}
	return Out;
}
#endif
