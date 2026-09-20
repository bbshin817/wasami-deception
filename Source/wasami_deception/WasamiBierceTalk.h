#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiBierceTalk.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;

/**
 * What one entry into the original's ubergraph does at the state it finds. Both ways in come down to this: the Talk
 * event (which the Halt check follows) and the Delay's return, which enters past that check.
 */
enum class EWasamiTalkStep : uint8
{
	/** Halt is on: the event ends there and nothing is said. */
	Nothing,
	/** The component is free: SetSound(What To Say) and Play(0). */
	Play,
	/** It is busy: Delay(0.5) and look again. */
	Wait,
	/**
	 * It is busy and a Delay is already running. Blueprint's Delay leaves a running one alone, so this entry ends
	 * here — and the waiting turn plays whatever Talk asked for last, not what it was started for.
	 */
	AlreadyWaiting
};

/** One entry of the loop, at the state given. */
WASAMI_DECEPTION_API EWasamiTalkStep WasamiTalkStep(bool bHalt, bool bPlaying, bool bWaiting);

/**
 * Dark Deception's BierceTalk_Blueprint (pak_reference_2's Blueprints/00_Ballroom): the one actor in a level that
 * speaks Bierce's lines. It is an AmbientSound child there — an actor rooted on a single AudioComponent0 that starts
 * itself off (bAutoActivate false) and plays through DialogueAttenuation — and its graph is three things:
 *
 *  - Talk(What To Say, Attenuate?): puts Attenuate? on the component's bAllowSpatialization (the hospital's calls are
 *    all False, so its lines are heard the same everywhere), and then, unless Halt is on, plays What To Say as soon as
 *    the component is free — waiting half a second at a time while a line is still going. It never cuts a line short.
 *  - Stop Talking(): the component's Stop(), and nothing else; a Delay already running is left alone.
 *  - Halt: while it is on, Talk says nothing. The hospital never raises it.
 *
 * The zones' flow (AWasamiZone1Flow, AWasamiZone2Flow) is what speaks through it, as the original's level Blueprints
 * do: either through their reference to the placed actor or through BP_DD_Functions' Bierce Talk, which takes the
 * first actor of this class in the world (Find).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiBierceTalk : public AActor
{
	GENERATED_BODY()

public:
	AWasamiBierceTalk();

	/** The Delay the loop waits by (s). */
	static constexpr float WaitInterval = 0.5f;

	/** Halt: while it is on, Talk says nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bHalt = false;

	/**
	 * Talk(What To Say, Attenuate?). Attenuate? reaches the component whether or not Halt stops the rest, as the
	 * original's order has it, and so does What To Say: a turn already waiting picks it up.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Talk(USoundBase* WhatToSay, bool bAttenuate);

	/** Stop Talking(). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StopTalking();

	/**
	 * BP_DD_Functions' Bierce Talk, which is Get All Actors Of Class's 0th: the first talker in the world. A level
	 * holds one (the hospital's BierceTalk_Blueprint_2), so which one that is does not come up.
	 */
	static AWasamiBierceTalk* Find(const UObject* WorldContext);

	UAudioComponent* GetAudioComponent() const { return AudioComponent; }

	/** What Talk asked for last — the ubergraph's What To Say, which a waiting turn plays. For the tests. */
	USoundBase* GetPendingSound() const { return PendingSound; }

	/** Whether a Delay is running. */
	bool IsWaiting() const;

	/** What the last entry did (Nothing before any). For the tests. */
	EWasamiTalkStep GetLastStep() const { return LastStep; }

protected:
	virtual void BeginPlay() override;

	/** Whether a line is still going. Virtual so the tests can hold the component busy without an audio device. */
	virtual bool IsSpeaking() const;

	/** AudioComponent0, the actor's root, as the original's AmbientSound parent makes it. */
	UPROPERTY(VisibleAnywhere, Category = "Dialogue")
	TObjectPtr<UAudioComponent> AudioComponent;

	/** DialogueAttenuation, put on the component when play begins (never loaded from a constructor, WasamiAssets.h). */
	UPROPERTY(EditAnywhere, Category = "Dialogue|Assets")
	TSoftObjectPtr<USoundAttenuation> Attenuation;

private:
	/**
	 * One entry into the ubergraph: Talk's (Halt looked at, and a Delay may be running) and the Delay's return (past
	 * the check, and nothing waiting — the Delay that woke it has run out).
	 */
	void Step(bool bLookAtHalt, bool bWaiting);

	/** The Delay's return. */
	void Resume();

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> PendingSound;

	EWasamiTalkStep LastStep = EWasamiTalkStep::Nothing;
	FTimerHandle WaitTimer;
};
