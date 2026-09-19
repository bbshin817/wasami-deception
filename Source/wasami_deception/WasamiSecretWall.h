#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiInteractable.h"
#include "WasamiSecretWall.generated.h"

class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;

/**
 * A secret wall, after Dark Deception's Blueprints/02_School/BP_03_SecretWall1 with its child BP_07_Zone1_SecretWall's
 * defaults (pak_reference_2; Zone 2 of the hospital has one, BP_07_Zone1_SecretWall_2, in front of its secret room): a
 * false wall (manor_fake_wall at 100) the player uses by looking at it and clicking. The first use takes the mesh's
 * interact tag off (the hand goes), plays Sliding_Wall at the wall (0.65, 01_Lobby_Attenuation) and plays Move Up from
 * its start at 0.7: the actor rises by Height (275) from where BeginPlay found it over the first 3 s of the timeline
 * (4.29 s), and stays up. Later uses do nothing (its DoOnce).
 *
 * The mesh and its material are not set by the class (WasamiAssets.h): the level build places the wall with the
 * stage's manor_fake_wall and the level's material (Zone 2's M_06_Hospital_Brick_01).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSecretWall : public AActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiSecretWall();

	virtual void Tick(float DeltaSeconds) override;

	/** Height: how far the wall rises (cm; BP_07_Zone1_SecretWall's 275). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret Wall")
	float Height = 275.f;

	/** Whether it has been used (the DoOnce is closed). */
	bool IsUsed() const { return bUsed; }

	/** Whether Move Up is playing. */
	bool IsMoving() const { return bMoving; }

	/** Move Up's position (s, of its 5). */
	float GetMoveUpPosition() const { return MoveUpPosition; }

	/** OG Height and InterpHeight: the root's z at BeginPlay, and it plus Height. */
	float GetOGHeight() const { return OGHeight; }
	float GetInterpHeight() const { return InterpHeight; }

	/** Move Up's Alpha at Seconds: 0 to 1 over 3 s, linear, then 1. */
	static float EvaluateMoveUp(float Seconds);

	/** Move Up's length (the timeline's, past its curve's 3 s) and its play rate. */
	static constexpr float MoveUpLength = 5.f;
	static constexpr float MoveUpCurveLength = 3.f;
	static constexpr float MoveUpPlayRate = 0.7f;

	/** PlaySoundAtLocation(Sliding_Wall, the actor, 0.65, 1). */
	static constexpr float SlideVolume = 0.65f;
	static constexpr float SlidePitch = 1.f;

	UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }

	/** Sliding_Wall. */
	UPROPERTY(EditAnywhere, Category = "Secret Wall|Assets")
	TSoftObjectPtr<USoundBase> SlideSound;

	/** 01_Lobby_Attenuation. */
	UPROPERTY(EditAnywhere, Category = "Secret Wall|Assets")
	TSoftObjectPtr<USoundAttenuation> SlideAttenuation;

protected:
	virtual void BeginPlay() override;
	virtual void InteractWithObject_Implementation(AActor* Interactee) override;

	UPROPERTY(VisibleAnywhere, Category = "Secret Wall")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** StaticMesh: manor_fake_wall at 100, blocking, tagged interact, out of the navigation. */
	UPROPERTY(VisibleAnywhere, Category = "Secret Wall")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

private:
	/** Move Up's update: the actor's z between OG Height and InterpHeight. */
	void ApplyMoveUp();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	float OGHeight = 0.f;
	float InterpHeight = 0.f;
	float MoveUpPosition = 0.f;
	bool bMoving = false;
	bool bUsed = false;
};
