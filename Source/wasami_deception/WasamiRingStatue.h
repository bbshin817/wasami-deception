#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "WasamiInteractable.h"
#include "WasamiRingStatue.generated.h"

class USoundAttenuation;
class USoundBase;
class UWasamiTextPromptWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiInteractAllShardsSignature);

/**
 * Dark Deception's BP_01_Statue (pak_reference_2's Blueprints/01_Hotel): the altar the ring piece is taken from. Zone 2
 * of the hospital has one (ring_statue_2) in the room after the maze, with the crystal orb (a mesh of its own) and the
 * ring piece (AWasamiRingPiece) over it.
 *
 * Looked at and clicked (InteractWithObject; its mesh is tagged interact, so the hand shows over it), a DoOnce: with no
 * soul shard left in the level it broadcasts Interact All Shards, which the zone's flow binds to the ring piece's
 * screen, and clears the mesh's tags (no hand any more; the DoOnce stays closed). With shards left it turns the player
 * away: the denied sound and a text prompt, and the DoOnce opens again 5 s later.
 *
 * A StaticMeshActor, as the original: its StaticMeshComponent is the root (the class's scale 2.8; the hospital's altar
 * is placed at 4), blocking all (so the look's Visibility trace finds it). It is Movable where the original's is
 * Static, so that placing it leaves the level's baked lighting as it is. The mesh and its material are not set by the
 * class (assets under /Game/DD are never loaded from a constructor, WasamiAssets.h): the level build places the altar
 * with ring_statue and MM_00_Ballroom_Ring_Altar_Metal. The original's Dematerialize (a 0.5 s timeline) is not called
 * in the hospital and is not made.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiRingStatue : public AStaticMeshActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiRingStatue();

	/** Interact All Shards: clicked with every shard collected. */
	UPROPERTY(BlueprintAssignable, Category = "Ring Statue")
	FWasamiInteractAllShardsSignature OnInteractAllShards;

	/** The text of the prompt that turns the player away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Statue")
	FText DeniedText;

	/** InteractWithObject's Delay: how long after turning the player away the altar can do it again. */
	static constexpr float DeniedInterval = 5.f;

	/** The prompt the altar last put up (null before the first, or once it has gone). */
	UWasamiTextPromptWidget* GetLastPrompt() const { return LastPrompt.Get(); }

protected:
	virtual void BeginPlay() override;

	/**
	 * InteractWithObject (a DoOnce): no AWasamiShard left → Interact All Shards and the mesh's tags cleared; else
	 * DD_RingBarrierDenied_louder at the actor (0.4, through DialogueAttenuation) and the prompt, the DoOnce opening
	 * again DeniedInterval later.
	 */
	virtual void InteractWithObject_Implementation(AActor* Interactee) override;

	UPROPERTY(EditAnywhere, Category = "Ring Statue|Assets")
	TSoftObjectPtr<USoundBase> DeniedSound;

	UPROPERTY(EditAnywhere, Category = "Ring Statue|Assets")
	TSoftObjectPtr<USoundAttenuation> DeniedAttenuation;

private:
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedDeniedSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> LoadedDeniedAttenuation;

	/** The DoOnce: closed from a click until DeniedTimer opens it again (for good once every shard is collected). */
	bool bClosed = false;
	FTimerHandle DeniedTimer;
	TWeakObjectPtr<UWasamiTextPromptWidget> LastPrompt;
};
