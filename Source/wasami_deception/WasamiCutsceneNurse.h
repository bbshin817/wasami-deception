#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiCutsceneNurse.generated.h"

class USkeletalMeshComponent;

/**
 * The nurses the cut scenes act with (item 25), which the level sequences move, show and animate: Zone 1's two
 * BP_06_Nurse_Cutscene (the original's Character, its mesh hung under the capsule's centre) and Zone 2's nurse_idle1
 * and nurse_idle2 (the original's SkeletalMeshActors, their mesh at the actor). The original's characters stay out of
 * this game, so this one carries the enemy Wasami's mesh (the stand-in of .claude/references/enemy-wasami-motions.md's
 * 「場面の代用」, which the sequences play the clips of), under a scene root the sequence's transform track moves as
 * the original moves its actor.
 *
 * SkeletalMesh is named as the original's Character names its mesh, so a sequence's binding finds it by name;
 * AWasamiEnemy's own offsets put it where the nurse's is — its height scale, and under the capsule for the ones the
 * original makes a Character. It collides with nothing (the player walks the cut scene's room), and the mesh is
 * loaded in OnConstruction, never from the constructor (WasamiAssets.h).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiCutsceneNurse : public AActor
{
	GENERATED_BODY()

public:
	AWasamiCutsceneNurse();

	/**
	 * Hangs the mesh under the capsule's centre and turns it as BP_06_Nurse_Cutscene does (AWasamiEnemy::MeshZ and
	 * MeshYaw), for the nurses the original makes a Character; the others keep the mesh at the actor, as the
	 * original's SkeletalMeshActor has it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cutscene Nurse")
	void SetUnderCapsule(bool bInUnderCapsule);

	bool IsUnderCapsule() const { return bUnderCapsule; }
	USkeletalMeshComponent* GetSkeletalMesh() const { return SkeletalMesh; }

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/** DefaultSceneRoot: what the sequence's transform track moves (the original's capsule or SkeletalMeshActor). */
	UPROPERTY(VisibleAnywhere, Category = "Cutscene Nurse")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** SkeletalMesh: SK_WasamiEnemy at the nurse's height, which the sequence's animation track plays. */
	UPROPERTY(VisibleAnywhere, Category = "Cutscene Nurse")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	/** Set by the level build, after the original's actor it stands in for. */
	UPROPERTY(EditAnywhere, Category = "Cutscene Nurse")
	bool bUnderCapsule = false;

	UPROPERTY(EditAnywhere, Category = "Cutscene Nurse")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;
};
