#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WasamiSoundCueLibrary.generated.h"

class USoundCue;
class USoundWave;

/**
 * Builds SoundCues for the pipeline, which writes the original's exported cues into them
 * (Content/Python/wasami_tools/pipeline/dd_assets.py). A cue's node tree (FirstNode, each node's ChildNodes) and its
 * editor graph are not reachable from Python, so the nodes are made and linked here; their values (a modulator's
 * pitch range) are written by name with UWasamiCascadeLibrary::SetPropertyText. Nodes are passed as UObject so that
 * Python needs no binding of the sound node classes.
 * Editor only: the pipeline runs in the editor.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSoundCueLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** Removes every node of Cue and its graph nodes; the graph keeps its output node. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static void ResetSoundCue(USoundCue* Cue);

	/**
	 * Makes a node of the sound node class named ClassName ('SoundNodeModulator', 'SoundNodeWavePlayer') in Cue at its
	 * class defaults, with its graph node and its starting inputs. Returns null for a class that is not a sound node.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static UObject* AddSoundNode(USoundCue* Cue, const FString& ClassName);

	/**
	 * Makes Children (nodes of the same cue) Node's inputs in order, adding inputs as needed. Returns '' when it is
	 * done, otherwise why not (not a node, more or fewer children than the node takes, fewer than it already has).
	 */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static FString SetChildNodes(UObject* Node, const TArray<UObject*>& Children);

	/** Gives a wave player node its wave. False when Node is not a wave player. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static bool SetWave(UObject* Node, USoundWave* Wave);

	/** Finishes Cue as its editor does: Root as the first node, the graph linked from the nodes, the cached duration and
	 * distance, PostEditChange. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static void FinishSoundCue(USoundCue* Cue, UObject* Root);

	/** Cue's nodes from its first node, depth first (a node reached twice is listed once). */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static TArray<UObject*> GetSoundNodes(USoundCue* Cue);

	/** Node's inputs in order (null for an empty one). */
	UFUNCTION(BlueprintCallable, Category = "Wasami|SoundCue")
	static TArray<UObject*> GetChildNodes(UObject* Node);
#endif
};
