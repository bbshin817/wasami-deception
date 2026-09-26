#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "WasamiZoneFlow.generated.h"

class AWasamiBierceTalk;
class AWasamiDoubleDoors;
class AWasamiEnemy;
class AWasamiMusicPlayer;
class AWasamiGameMode;
class AWasamiTriggerBox;
class AWasamiZoneBarrier;
class UCameraShakeBase;
class ULevelSequence;
class UMediaPlayer;
class UMediaSource;
class ULevelSequencePlayer;
class USoundAttenuation;
class USoundBase;

/**
 * A hospital zone's level Blueprint (pak_reference_2's 06_Hospital_Zone_01 and _02), which this game's levels do not
 * have: the game mode spawns the zone's flow when play begins, and the flow starts the section of the checkpoint the
 * level opened at (the zone's Spawn) and moves on from section to section as the original's events do — trigger boxes
 * walked through, all the shards collected, checkpoints saved, the objective and the tablet's arrow set. It finds what
 * the original names by the tag 'src:<name>' the level build gives each placed actor.
 *
 * The flow moves the story on and plays what the level Blueprints play themselves: the level sequences placed in the
 * level, the fade, the camera shakes, sounds and the level's emitters; it wakes the door breaks and listens to them,
 * and locks, opens and destroys the double doors it names. It spawns the nurses at the level's target points as the
 * events do, and takes the zone's music away and gives it back (AWasamiMusicPlayer's bFadeOut and its fades). Bierce
 * speaks through the level's one talker (BierceTalk), as the original's events do. What the events show through other
 * actors (the barrier, the loading screen) is left to the items that make those; each has its place in the event,
 * marked by a comment.
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

	/**
	 * Disable Player Input (BP_DD_Functions): DisableInput on the player character with its controller, its movement
	 * stopped at once, Has Input off and every camera shake of its camera stopped. The cutscenes start with it.
	 */
	static void DisablePlayerInput(const UObject* WorldContextObject);

	/** Enable Player Input (BP_DD_Functions): EnableInput and Has Input back on. */
	static void EnablePlayerInput(const UObject* WorldContextObject);

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** The double doors placed from the original's actor of that name (BP_06_DoubleDoors), or null with a warning. */
	AWasamiDoubleDoors* DoubleDoors(FName Source) const;

	/** The zone barrier placed from the original's actor of that name (BP_ZoneBarrier), or null with a warning. */
	AWasamiZoneBarrier* ZoneBarrier(FName Source) const;

	/**
	 * The music player placed from the original's actor of that name (BP_06_MusicPlayer, BP_06_MusicPlayer_Zone2), or
	 * null with a warning: what the sections raise and drop bFadeOut on, as the original's level Blueprint does through
	 * its reference to the placed actor.
	 */
	AWasamiMusicPlayer* MusicPlayer(FName Source) const;

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
	 * ('06_Hospital_Zone01_ElevatorArrive'); the level build places them with their sequences (dd_sequence). With
	 * Finished given, the flow's event of that name is bound to the player's OnFinished, as the original binds it: how
	 * a sequence that is no cut scene ends, taking neither the view nor the input (Zone 2's ambulance arrival, which
	 * drives in while the player walks about).
	 */
	void PlaySequence(FName Source, FName Finished = NAME_None);

	/** GetSequencePlayer on the LevelSequenceActor placed from the original's actor of that name, or null with a warning. */
	ULevelSequencePlayer* SequencePlayer(FName Source) const;

	/**
	 * How the zones play a cutscene: the view to the cine camera placed from the original's actor of that name over
	 * CutsceneViewBlendTime when one is given, Initialize Cutscene Widget (the skip screen; bSmoothTransition slides the
	 * cinematic bars in), the sequence played, and its OnFinished bound to the flow's event of that name. Disable Player
	 * Input is the caller's, as it is in the original (Zone 1's scene leaves the player looking on).
	 */
	void PlayCutscene(FName Source, FName Finished, FName Camera = NAME_None, bool bSmoothTransition = true);

	/** SetViewTargetWithBlend(Target, BlendTime, VTBlend_Cubic, 0, no lock) on the player's controller. */
	void SetPlayerViewTarget(AActor* Target, float BlendTime);

	/** The blend the scenes cut to their cine camera over (and back to the player). */
	static constexpr float CutsceneViewBlendTime = 0.5f;

	/**
	 * ClientPlayCameraShake(Shake, Scale, CameraLocal, no rotation) on the player's controller, the original's shake
	 * Blueprints (UE4's CameraShake as LegacyCameraShake under /Game/DD, made by the level build with the sequences).
	 */
	void PlayCameraShake(const TSoftClassPtr<UCameraShakeBase>& Shake, float Scale = 1.f);

	/**
	 * PlayWorldCameraShake: the shake for every player within OuterRadius of Epicenter, full within InnerRadius and
	 * falling off by Falloff between them.
	 */
	void PlayWorldCameraShake(const TSoftClassPtr<UCameraShakeBase>& Shake, const FVector& Epicenter, float InnerRadius,
		float OuterRadius, float Falloff, bool bOrientTowardsEpicenter);

	/**
	 * Basic DD Fade Out (BP_DD_Functions): a sequence player made for FadeSequence (Ballroom_Event_Fade: black for 2 s,
	 * clear again 3 s later) and played at PlayRate.
	 */
	void PlayFadeOut(float PlayRate);

	/**
	 * Bierce Talk (BP_DD_Functions): the level's one talker (AWasamiBierceTalk::Find, which is Get All Actors Of
	 * Class's 0th) speaks What To Say. The hospital's calls are all Attenuate? False, so Bierce is heard the same
	 * wherever the player is; a line still going is never cut short (the talker waits it out).
	 */
	void BierceTalk(const TSoftObjectPtr<USoundBase>& Sound, bool bAttenuate = false);

	/** PlaySoundAtLocation(Sound, Location, no rotation, volume and pitch 1, from the start, Attenuation). */
	void PlaySoundAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Location,
		const TSoftObjectPtr<USoundAttenuation>& Attenuation);

	/** The ParticleSystemComponent's Activate(true) of the emitter placed from the original's actor of that name. */
	void ActivateEmitter(FName Source);

	/**
	 * BeginDeferredActorSpawnFromClass(Class, the transform of the target point placed from the original's of that name,
	 * AdjustIfPossibleButAlwaysSpawn), CanSpawn set, Setup (the spawn's other values, when given) and FinishSpawningActor:
	 * how the zones spawn their nurses. Null with a warning when there is no such point.
	 */
	AWasamiEnemy* SpawnEnemy(TSubclassOf<AWasamiEnemy> Class, FName SpawnPoint,
		const TFunction<void(AWasamiEnemy&)>& Setup = nullptr) const;

	/** SetCollisionEnabled on the brush of the blocking volume of that name. */
	void SetVolumeCollision(FName Source, ECollisionEnabled::Type Enabled);

	/**
	 * K2_TeleportTo(the player start's location, no rotation) and SetControlRotation(its rotation): how the zones move
	 * the player to a section's start. The player start is found by its PlayerStartTag (the original's name).
	 */
	void TeleportPlayerTo(FName PlayerStartTag);

	/**
	 * Attaches the player to the vehicle (the level actor of that source name) with the movement off, looking around
	 * still working: the ambulances' rides. The original carries the player by the based move alone, which a long frame
	 * breaks (the vehicle's unswept move pushes the capsule off its roof). False when there is no player or vehicle.
	 */
	bool RidePlayerOn(FName VehicleSource);

	/** Detaches the player from what RidePlayerOn attached them to and lets them walk again. */
	void StopPlayerRide();

	/** A Delay: Then runs Seconds on (on the next tick for 0). */
	void After(float Seconds, TFunction<void()>&& Then);

	/** The first actor placed from the original's actor of that name in this flow's world. */
	AActor* Source(FName Name) const { return FindSource(GetWorld(), Name); }

	/** Basic DD Fade Out's sequence (/Game/DD/Animation/00_Ballroom/Ballroom_Event_Fade, made by the level build). */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<ULevelSequence> FadeSequence;

	/** The ambulance screens' video (the original's Setup opens Video_AmbulanceTutorial on its looping player; the
	 *  screens' material shows the player's MediaTexture). Made by dd_movies under /Game/Wasami/Movies. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<UMediaPlayer> ScreenPlayer;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<UMediaSource> ScreenSource;

	UPROPERTY(Transient)
	TObjectPtr<AWasamiGameMode> Mode;

private:
	/** The flow's event of that name bound to the sequence player's OnFinished (BindDelegate + AddMulticastDelegate). */
	void BindSequenceFinished(ULevelSequencePlayer* Player, FName Finished);

	FName Section;
	bool bArrowShards = true;
	TOptional<FLinearColor> ArrowColor;
	TWeakObjectPtr<AActor> ArrowTarget;
};
