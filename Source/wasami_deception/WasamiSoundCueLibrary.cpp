#include "WasamiSoundCueLibrary.h"

#if WITH_EDITOR
#include "Sound/SoundCue.h"
#include "Sound/SoundNode.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiSoundCue, Log, All);

void UWasamiSoundCueLibrary::ResetSoundCue(USoundCue* Cue)
{
	if (!Cue)
	{
		return;
	}
	Cue->Modify();
	Cue->ResetGraph();
	Cue->MarkPackageDirty();
}

UObject* UWasamiSoundCueLibrary::AddSoundNode(USoundCue* Cue, const FString& ClassName)
{
	if (!Cue)
	{
		return nullptr;
	}
	UClass* Class = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass | EFindFirstObjectOptions::NativeFirst);
	if (!Class || !Class->IsChildOf<USoundNode>() || Class->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogWasamiSoundCue, Error, TEXT("AddSoundNode: '%s' is not a sound node class that can be made"), *ClassName);
		return nullptr;
	}
	return Cue->ConstructSoundNode<USoundNode>(Class, false);
}

FString UWasamiSoundCueLibrary::SetChildNodes(UObject* Node, const TArray<UObject*>& Children)
{
	USoundNode* SoundNode = Cast<USoundNode>(Node);
	if (!SoundNode)
	{
		return TEXT("not a sound node");
	}
	TArray<USoundNode*> ChildNodes;
	for (UObject* Child : Children)
	{
		// None is an input left empty (a random node's that plays nothing when picked).
		if (!Child)
		{
			ChildNodes.Add(nullptr);
			continue;
		}
		USoundNode* ChildNode = Cast<USoundNode>(Child);
		if (!ChildNode || ChildNode->GetOuter() != SoundNode->GetOuter())
		{
			return FString::Printf(TEXT("%s is not a node of the same cue"), *GetNameSafe(Child));
		}
		ChildNodes.Add(ChildNode);
	}
	if (ChildNodes.Num() > SoundNode->GetMaxChildNodes() || ChildNodes.Num() < SoundNode->GetMinChildNodes())
	{
		return FString::Printf(TEXT("%s takes %d to %d inputs, not %d"), *SoundNode->GetClass()->GetName(),
			SoundNode->GetMinChildNodes(), SoundNode->GetMaxChildNodes(), ChildNodes.Num());
	}
	// An input removed here would leave its graph pin behind, which the graph's linking does not allow.
	if (ChildNodes.Num() < SoundNode->ChildNodes.Num())
	{
		return FString::Printf(TEXT("%s already has %d inputs"), *SoundNode->GetName(), SoundNode->ChildNodes.Num());
	}
	SoundNode->Modify();
	while (SoundNode->ChildNodes.Num() < ChildNodes.Num())
	{
		SoundNode->InsertChildNode(SoundNode->ChildNodes.Num());
	}
	for (int32 Index = 0; Index < ChildNodes.Num(); ++Index)
	{
		SoundNode->ChildNodes[Index] = ChildNodes[Index];
	}
	return FString();
}

bool UWasamiSoundCueLibrary::SetWave(UObject* Node, USoundWave* Wave)
{
	USoundNodeWavePlayer* Player = Cast<USoundNodeWavePlayer>(Node);
	if (!Player)
	{
		return false;
	}
	Player->Modify();
	Player->SetSoundWave(Wave);
	return true;
}

void UWasamiSoundCueLibrary::FinishSoundCue(USoundCue* Cue, UObject* Root)
{
	if (!Cue)
	{
		return;
	}
	Cue->Modify();
	Cue->FirstNode = Cast<USoundNode>(Root);
	Cue->LinkGraphNodesFromSoundNodes();
	Cue->PostEditChange();
	Cue->MarkPackageDirty();
}

TArray<UObject*> UWasamiSoundCueLibrary::GetSoundNodes(USoundCue* Cue)
{
	TArray<UObject*> Out;
	if (Cue && Cue->FirstNode)
	{
		TArray<USoundNode*> Nodes;
		Cue->RecursiveFindAllNodes(Cue->FirstNode, Nodes);
		for (USoundNode* Node : Nodes)
		{
			Out.AddUnique(Node);
		}
	}
	return Out;
}

TArray<UObject*> UWasamiSoundCueLibrary::GetChildNodes(UObject* Node)
{
	TArray<UObject*> Out;
	if (const USoundNode* SoundNode = Cast<USoundNode>(Node))
	{
		for (USoundNode* Child : SoundNode->ChildNodes)
		{
			Out.Add(Child);
		}
	}
	return Out;
}
#endif
