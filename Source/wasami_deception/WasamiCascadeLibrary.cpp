#include "WasamiCascadeLibrary.h"

#if WITH_EDITOR
#include "Distributions/Distribution.h"
#include "Distributions/DistributionFloat.h"
#include "Distributions/DistributionVector.h"
#include "Engine/InterpCurveEdSetup.h"
#include "Misc/StringOutputDevice.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleSystem.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
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

	/**
	 * Checks that every member a struct's text names ('(A=1,B=(C=2))') is a property of Struct, down through nested
	 * structs: UE's own import passes over unknown members without a word (it reports them at LogExec's Verbose).
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
			const FStructProperty* StructMember = CastField<FStructProperty>(Member);
			const bool bNested = StructMember && *Cursor == TEXT('(')
				&& !(StructMember->Struct->StructFlags & STRUCT_ImportTextItemNative);
			if (!(bNested ? CheckStructText(StructMember->Struct, Cursor, Error) : SkipValue(Cursor, Error)))
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

	/**
	 * Moves out the distribution objects Module holds but no longer uses: a module makes its own when it is made, and a
	 * table written over one (with Distribution=None) leaves it behind.
	 */
	void RenameAwayUnusedDistributions(UParticleModule* Module)
	{
		TSet<UObject*> Used;
		for (TFieldIterator<FProperty> It(Module->GetClass()); It; ++It)
		{
			if (const FStructProperty* StructProperty = CastField<FStructProperty>(*It))
			{
				const FName StructName = StructProperty->Struct->GetFName();
				for (int32 Index = 0; Index < StructProperty->ArrayDim; ++Index)
				{
					if (StructName == NAME_RawDistributionFloat)
					{
						Used.Add(StructProperty->ContainerPtrToValuePtr<FRawDistributionFloat>(Module, Index)->Distribution);
					}
					else if (StructName == NAME_RawDistributionVector)
					{
						Used.Add(StructProperty->ContainerPtrToValuePtr<FRawDistributionVector>(Module, Index)->Distribution);
					}
				}
			}
			else if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(*It))
			{
				for (int32 Index = 0; Index < ObjectProperty->ArrayDim; ++Index)
				{
					Used.Add(ObjectProperty->GetObjectPropertyValue_InContainer(Module, Index));
				}
			}
		}
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
	const TArray<UObject*>& Modules)
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
		if (!Module || Module->GetOuter() != System || Module->IsA<UParticleModuleRequired>() || Module->IsA<UParticleModuleSpawn>())
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
	Level->RequiredModule = Required;
	Level->SpawnModule = Spawn;
	Level->Modules = MoveTemp(LevelModules);
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
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		const TCHAR* Cursor = *Text;
		if (!(StructProperty->Struct->StructFlags & STRUCT_ImportTextItemNative)
			&& !CheckStructText(StructProperty->Struct, Cursor, Error))
		{
			return FString::Printf(TEXT("%s.%s: %s"), *Object->GetName(), *Name, *Error);
		}
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
#endif
