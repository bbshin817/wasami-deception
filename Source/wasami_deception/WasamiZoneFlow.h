#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "WasamiZoneFlow.generated.h"

class AWasamiGameMode;
class AWasamiTriggerBox;
class UCameraShakeBase;

/**
 * A hospital zone's level Blueprint (pak_reference_2's 06_Hospital_Zone_01 and _02), which this game's levels do not
 * have: the game mode spawns the zone's flow when play begins, and the flow starts the section of the checkpoint the
 * level opened at (the zone's Spawn) and moves on from section to section as the original's events do — trigger boxes
 * walked through, all the shards collected, checkpoints saved, the objective and the tablet's arrow set. It finds what
 * the original names by the tag 'src:<name>' the level build gives each placed actor.
 *
 * The flow moves the story on and plays what the level Blueprints play themselves: the level sequences placed in the
 * level and the camera shakes; it wakes the door breaks and listens to them. What the events show or voice through
 * other actors (doors, the barrier, the loading screen, Bierce and the nurses' lines, the music, the nurses themselves)
 * is left to the items that make those; each has its place in the event, marked by a comment.
 */
UCLASS(Abstract)
class WASAMI_DECEPTION_API AWasamiZoneFlow : public AActor
{
	GENERATED_BODY()

public:
	AWasamiZoneFlow();

	/** Spawns the flow of zone 1 or 2 for the game mode (its BeginPlay starts the section); null for any other zone. */
	static AWasamiZoneFlow* SpawnFor(AWasamiGameMode* Mode, int32 Zone);

	/** The tag the level build gives an actor placed from the original's actor of that name. */
	static FName SourceTag(FName Name);

	/** The first actor placed from the original's actor of that name, or null. */
	static AActor* FindSource(const UWorld* World, FName Name);

	/**
	 * Remove All Enemies (BP_DD_Functions): destroys every actor tagged Enemy. The zones call it as a section ends, so
	 * that the next one starts with its own nurses.
	 */
	static void RemoveAllEnemies(UWorld* World);

	/** Destroys the level's shards that are left, without collecting them (06 Transition, Postmaze Transition). */
	static void DestroyAllShards(UWorld* World);

	/**
	 * Calls an event of the flow by its function name (On05Transition, OnMazeAllShards, ...), as the debug command
	 * Wasami.Flow does; false when there is no such event.
	 */
	bool CallEvent(FName Function);

	/** The original's name of the event the flow came to last ('05_Persistent', 'Maze Transition', ...). */
	FName GetSection() const { return Section; }

	/**
	 * What the zone tells the tablet's arrow (BP_ArrowPointer): Shards? (point at the nearest shard, true by default),
	 * the colour of Change Color (unset until one is given) and the target.
	 */
	bool IsArrowOnShards() const { return bArrowShards; }
	TOptional<FLinearColor> GetArrowColor() const { return ArrowColor; }
	AActor* GetArrowTarget() const { return ArrowTarget.Get(); }

	AWasamiGameMode* GetMode() const { return Mode; }

protected:
	virtual void BeginPlay() override;

	/** The zone's Spawn: the section of the checkpoint the level opened at. */
	virtual void StartAt(int32 Checkpoint) {}

	/** Notes the event the flow came to (GetSection). */
	void Enter(const TCHAR* Name);

	/** BindDelegate + AddMulticastDelegate: the trigger box's Trigger calls the flow's event of that name. */
	void BindTrigger(FName Source, FName Function);

	/**
	 * The door break of that name's Enable Switch, and its Finished Event bound to the flow's event of that name (the
	 * zones do both together: Zone 1's lift door, Zone 2's cell door).
	 */
	void EnableDoorBreak(FName Source, FName Function);

	/** The game mode's All Shards Collected calls the flow's event of that name. */
	void BindAllShardsCollected(FName Function);

	/** The game mode's Current Objective. */
	void SetObjective(const FText& Objective);

	/** The game mode's checkpoint save (SAVING PROGRESS, the checkpoint, the time). */
	void SaveCheckpoint(int32 Checkpoint);

	void SetArrowShards(bool bShards) { bArrowShards = bShards; }
	void SetArrowColor(const FLinearColor& Color) { ArrowColor = Color; }
	void SetArrowTarget(AActor* Target) { ArrowTarget = Target; }

	/**
	 * GetSequencePlayer → Play on the LevelSequenceActor placed from the original's actor of that name
	 * ('06_Hospital_Zone01_ElevatorArrive'); the level build places them with their sequences (dd_sequence).
	 */
	void PlaySequence(FName Source);

	/**
	 * ClientPlayCameraShake(Shake, Scale, CameraLocal, no rotation) on the player's controller, the original's shake
	 * Blueprints (UE4's CameraShake as LegacyCameraShake under /Game/DD, made by the level build with the sequences).
	 */
	void PlayCameraShake(const TSoftClassPtr<UCameraShakeBase>& Shake, float Scale = 1.f);

	/** SetCollisionEnabled on the brush of the blocking volume of that name. */
	void SetVolumeCollision(FName Source, ECollisionEnabled::Type Enabled);

	/**
	 * K2_TeleportTo(the player start's location, no rotation) and SetControlRotation(its rotation): how the zones move
	 * the player to a section's start. The player start is found by its PlayerStartTag (the original's name).
	 */
	void TeleportPlayerTo(FName PlayerStartTag);

	/** A Delay: Then runs Seconds on (on the next tick for 0). */
	void After(float Seconds, TFunction<void()>&& Then);

	/** The first actor placed from the original's actor of that name in this flow's world. */
	AActor* Source(FName Name) const { return FindSource(GetWorld(), Name); }

	UPROPERTY(Transient)
	TObjectPtr<AWasamiGameMode> Mode;

private:
	FName Section;
	bool bArrowShards = true;
	TOptional<FLinearColor> ArrowColor;
	TWeakObjectPtr<AActor> ArrowTarget;
};
