#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiInteractable.h"
#include "WasamiFakeUseActor.generated.h"

class ALevelSequenceActor;
class UAudioComponent;
class UBoxComponent;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiFakeUseUsedSignature);

/**
 * Something that is only there to be used, after Dark Deception's Blueprints/Main/BP_FakeUseActor (pak_reference_2): a
 * box tagged interact that the look's Visibility trace hits (it ignores the rest, pawns among them), used by looking at
 * it and clicking. The first use broadcasts Used and runs Used Event, which here takes the actor away; the children put
 * their own in its place. Later uses do nothing (its DoOnce), though the box keeps its tag and the hand.
 *
 * With bInactive the box starts with no collision (no hand, no use) until Activate gives it its queries back. None of
 * the hospital's is inactive, and nothing binds Used there.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiFakeUseActor : public AActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiFakeUseActor();

	/** bInactive: the box has no collision from BeginPlay until Activate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake Use")
	bool bInactive = false;

	/** Used: broadcast by the first use, before Used Event. */
	UPROPERTY(BlueprintAssignable, Category = "Fake Use")
	FWasamiFakeUseUsedSignature OnUsed;

	/** Activate: Box.SetCollisionEnabled(QueryOnly). */
	UFUNCTION(BlueprintCallable, Category = "Fake Use")
	void Activate();

	/** Whether it has been used (the DoOnce is closed). */
	bool IsUsed() const { return bUsed; }

	UBoxComponent* GetBox() const { return Box; }

protected:
	virtual void BeginPlay() override;
	virtual void InteractWithObject_Implementation(AActor* Interactee) override;

	/** Used Event: BP_FakeUseActor's destroys the actor; the children replace it without calling it. */
	virtual void UsedEvent();

	UPROPERTY(VisibleAnywhere, Category = "Fake Use")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Box: UE's 32 cm box on the root, tagged interact, blocking only the traces, out of the navigation. */
	UPROPERTY(VisibleAnywhere, Category = "Fake Use")
	TObjectPtr<UBoxComponent> Box;

private:
	bool bUsed = false;
};

/**
 * BP_FakeUseActor_SequencePlayer: Used Event plays the level sequence Sequence (GetSequencePlayer → Play). The
 * hospital's Zone 1 has two, in front of its secret elevators (BP_FakeUseActor_2 and BP_FakeUseActor5), which open
 * their doors (06_Hospital_Zone1_SecretElevator and 06_Hospital_Zone1_SecretElevator1_2). The actor stays.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiFakeUseSequencePlayer : public AWasamiFakeUseActor
{
	GENERATED_BODY()

public:
	/** Sequence: the level's LevelSequenceActor (the level build sets it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fake Use")
	TObjectPtr<ALevelSequenceActor> Sequence;

protected:
	virtual void UsedEvent() override;
};

/**
 * BP_FakeUseActor_06_HospitalZone1_Elevator (Blueprints/06_Hospital): a lift that is not one. Zone 1 has five
 * (BP_FakeUseActor2, 3_2, 4, 6 and 7). Its doors, two meshes under Scene (305, 70, −145 from the root), are closed;
 * Used Event plays its ActorSequence, whose two transform tracks slide the left door (StaticMesh1) 145 cm along x and the
 * right one (StaticMesh) −145 cm between 2.23 s and 4.97 s of its 5 (cubic, flat at both keys) and keep them there, and
 * plays Audio1 (DD_TT_Elevator_Doors_Open with 01_Lobby_Attenuation, 160 cm above Scene) at once. The actor stays.
 *
 * The sequence is played on the actor's tick rather than by an ActorSequenceComponent. The doors' meshes are not set by
 * the class (WasamiAssets.h): the level build sets the stage's hospital_elevator_doors_R_elevator_door and
 * hospital_elevator_doors_L_elevator_door_.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiFakeUseElevator : public AWasamiFakeUseActor
{
	GENERATED_BODY()

public:
	AWasamiFakeUseElevator();

	virtual void Tick(float DeltaSeconds) override;

	/** The doors' opening at Seconds into the sequence: 0 until 2.23 s, 1 from 4.97 s, cubic between. */
	static float EvaluateDoors(float Seconds);

	/** The sequence's keys (24000 ticks a second) and its length. */
	static constexpr float DoorsStart = 53600.f / 24000.f;
	static constexpr float DoorsEnd = 119200.f / 24000.f;
	static constexpr float SequenceLength = 120000.f / 24000.f;

	/** How far each door slides (cm, along x under Scene: the left one +, the right one −). */
	static constexpr float DoorTravel = 145.f;

	/** Whether the sequence is playing, and where. */
	bool IsPlaying() const { return bPlaying; }
	float GetSequencePosition() const { return SequencePosition; }

	USceneComponent* GetScene() const { return Scene; }
	UStaticMeshComponent* GetRightDoor() const { return StaticMesh; }
	UStaticMeshComponent* GetLeftDoor() const { return StaticMesh1; }
	UAudioComponent* GetAudio() const { return Audio1; }

	/** DD_TT_Elevator_Doors_Open. */
	UPROPERTY(EditAnywhere, Category = "Fake Use|Assets")
	TSoftObjectPtr<USoundBase> DoorsSound;

	/** 01_Lobby_Attenuation. */
	UPROPERTY(EditAnywhere, Category = "Fake Use|Assets")
	TSoftObjectPtr<USoundAttenuation> DoorsAttenuation;

protected:
	virtual void BeginPlay() override;
	virtual void UsedEvent() override;

	UPROPERTY(VisibleAnywhere, Category = "Fake Use")
	TObjectPtr<USceneComponent> Scene;

	/** StaticMesh: the right door. */
	UPROPERTY(VisibleAnywhere, Category = "Fake Use")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** StaticMesh1: the left door. */
	UPROPERTY(VisibleAnywhere, Category = "Fake Use")
	TObjectPtr<UStaticMeshComponent> StaticMesh1;

	UPROPERTY(VisibleAnywhere, Category = "Fake Use")
	TObjectPtr<UAudioComponent> Audio1;

private:
	/** The transform tracks at the sequence's position: the doors' relative x. */
	void ApplyDoors();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	float SequencePosition = 0.f;
	bool bPlaying = false;
};
