#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiSaveGame.h"
#include "WasamiCollectable.generated.h"

class UBoxComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A secret file, after Dark Deception's Blueprints/Main/BP_Collectable (pak_reference_2): the folder (secret_file at 60)
 * bobbing and turning over a small light, taken by walking into its box. BeginPlay plays Bounce (its 5 s loop at 3
 * times its speed: the folder between 35 and 40 cm up and turned 0 to 7°), and 0.2 s on removes the file if the save
 * already has its ID among the level's Secrets. The player's touch takes it once: the pickup voice (2D, at 0.85), NEW
 * EXTRAS UNLOCKED! on the player's screen (UWasamiCollectablesWidget), Unlock, and the file gone with its light. Unlock
 * puts the extras its Collectables list names into the save's ExtrasArt and ExtrasSFX, written there and then (the
 * extras screen off the title screen shows them), and its ID into the save's Secrets (written at the next checkpoint's
 * save; the score's SECRETS counts them).
 *
 * Secret? (true on every file) only adds to the game state's Secrets Amount, which nothing reads, and starts the hotel's
 * achievement, so it is left out: the ID goes into the save's Secrets either way (Unlock's end). So are the Audio
 * component, never played, and the cast to DD_GameState that Bounce waits on.
 *
 * The mesh (FileMesh, the stage's secret_file, which carries MM_Shared_Secret_Folder on both slots) is put on by
 * OnConstruction, as the special shards' are: the level build places the files, and Zone 2's flow spawns the one after
 * the maze (Postmaze Transition), which has no build to dress it.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiCollectable : public AActor
{
	GENERATED_BODY()

public:
	AWasamiCollectable();

	virtual void Tick(float DeltaSeconds) override;

	/** ID: which of the level's files it is, in the save's Secrets (Zone 1's 0 and 1, Zone 2's 2, after the maze 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable")
	int32 ID = 0;

	/**
	 * Collectables: the extras it unlocks (the level's values: Zone 1's file 1 Art Gallery 19 and 20, Zone 2's file 2
	 * Art Gallery 21 and 22; its file 0 and the one after the maze none). The original has file 0 unlock its Sound 5,
	 * the one of that chapter's tracks, but this game's Extras holds no track of its own and its Sound Archive stays
	 * locked (19 record), so the build drops the sounds and that file unlocks nothing (dd_level's COLLECTABLE_SKIP,
	 * 01 record).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collectable")
	TArray<FWasamiCollectableEntry> Collectables;

	/** Collect (the box's overlap with the player; the tests call it directly): the pickup, once. */
	void Collect();

	/**
	 * Unlock: for each of Collectables, the save read from its slot (none: on to the next), the extra put into its list
	 * (UWasamiSaveGame::Unlock) and the save written; then the ID into the game mode's save's Secrets. The original
	 * writes the extras to its other save (SaveSlot) and leaves the game mode's copy of it alone; this game's one save
	 * holds both, so the game mode's copy takes the extras as well, or the next checkpoint's save would write them out.
	 */
	void Unlock();

	/** Whether it has been taken (the DoOnce is closed). */
	bool IsTaken() const { return bTaken; }

	/** Bounce's position (s, of its 5) and its NewTrack_0 at Seconds (0 at 0 and 5, 1 at 2.5). */
	float GetBouncePosition() const { return BouncePosition; }
	static float EvaluateBounce(float Seconds);

	/** The folder's height (cm) and turn (°, its yaw) at a NewTrack_0 value: Lerp(35, 40, v) and Lerp(0, 7, v). */
	static float BounceHeight(float Value);
	static float BounceYaw(float Value);

	/** Bounce's length (s), its play rate, and the Delay before the save's check (s). */
	static constexpr float BounceLength = 5.f;
	static constexpr float BouncePlayRate = 3.f;
	static constexpr float SaveCheckDelay = 0.2f;

	/** CreateSound2D(Sound 2D, 0.85, 1). */
	static constexpr float PickupVolume = 0.85f;
	static constexpr float PickupPitch = 1.f;

	UBoxComponent* GetBox() const { return Box; }
	UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
	UPointLightComponent* GetPointLight() const { return PointLight; }

	/** Sound 2D: Bierce_Secret_Files_Pickup. */
	UPROPERTY(EditAnywhere, Category = "Collectable|Assets")
	TSoftObjectPtr<USoundBase> PickupSound;

	/** StaticMesh_GEN_VARIABLE's mesh: secret_file (the stage's). */
	UPROPERTY(EditAnywhere, Category = "Collectable|Assets")
	TSoftObjectPtr<UStaticMesh> FileMesh;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Collectable")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Box: 39 × 42 × 32 cm (half), 36 cm up; UE's overlap box otherwise. */
	UPROPERTY(VisibleAnywhere, Category = "Collectable")
	TObjectPtr<UBoxComponent> Box;

	/** StaticMesh: secret_file at 60, touching nothing; Bounce moves it. */
	UPROPERTY(VisibleAnywhere, Category = "Collectable")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** PointLight: 1000 (unitless), 250 cm, a soft source of 2000, no shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Collectable")
	TObjectPtr<UPointLightComponent> PointLight;

private:
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** The Delay's end: removed if the save's Secrets has the ID. */
	void CheckSave();

	/** Bounce's update: the folder's place and turn at the position. */
	void ApplyBounce();

	/** What the pickup uses, loaded at BeginPlay so that it waits on nothing. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	FTimerHandle SaveCheckTimer;
	float BouncePosition = 0.f;
	bool bTaken = false;
};
