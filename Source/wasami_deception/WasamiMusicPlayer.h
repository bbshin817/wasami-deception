#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiMusicPlayer.generated.h"

class UAudioComponent;
class USoundBase;

/** What one Update asks of one music component, as the original's fade nodes (their values are the actor's constants). */
enum class EWasamiMusicFade : uint8
{
	/** Nothing this Update. */
	None,
	/** FadeIn(1, 1, 0, Linear). */
	In,
	/** FadeOut(1, 0, Linear). */
	Out,
	/** If Playing Fade Out: the same FadeOut, but only while the component plays. */
	OutIfPlaying
};

/** The three components' fades of one Update. */
struct FWasamiMusicFades
{
	EWasamiMusicFade Regular = EWasamiMusicFade::None;
	EWasamiMusicFade Panic = EWasamiMusicFade::None;
	EWasamiMusicFade Override = EWasamiMusicFade::None;

	bool IsSilent() const
	{
		return Regular == EWasamiMusicFade::None && Panic == EWasamiMusicFade::None && Override == EWasamiMusicFade::None;
	}
};

/**
 * One DoOnce of Update: it lets a change through once and closes, until its Reset opens it again. A node that starts
 * closed (the original's Start Closed, only the one that fades the override music out) lets nothing through until a
 * Reset comes.
 */
struct FWasamiMusicDoOnce
{
	explicit FWasamiMusicDoOnce(bool bInStartClosed = false) : bStartClosed(bInStartClosed) {}

	/** Entering the node: true when it lets through (and closes for good until a Reset). */
	bool Enter();
	/** Its Reset pin. */
	void Reset()
	{
		bInitialised = true;
		bClosed = false;
	}

	bool IsClosed() const { return bClosed; }

private:
	bool bStartClosed = false;
	bool bInitialised = false;
	bool bClosed = false;
};

/**
 * Update's five DoOnce nodes, kept together so the switching can be checked without a world (AWasamiMusicPlayer::Update
 * only plays what this answers). Every Update reads the three conditions in the original's order and returns the fades
 * of that turn — which are none while nothing has changed, since each branch closes its DoOnce behind it.
 *
 * Two of the original's habits matter and are kept: fading out (bFadeOut) does not open the intense / calm pair, so the
 * music does not come back on its own when bFadeOut goes false again in the same state (Zone 2's flow fades its regular
 * music in itself after the cell scene); and while bFadeOut or bOverrideMusic is on, the Update stops there and the
 * chase is not looked at.
 */
struct WASAMI_DECEPTION_API FWasamiMusicState
{
	/** One turn of the original's Update, at the state given. */
	FWasamiMusicFades Update(bool bFadeOut, bool bOverrideMusic, bool bIntense);

private:
	/** The bFadeOut branch: fades all three out, once. */
	FWasamiMusicDoOnce FadeOutOnce;
	/** The bOverrideMusic branch: the override music in, the other two out. */
	FWasamiMusicDoOnce OverrideOnOnce;
	/** Its other side: the override music out. Starts closed — nothing plays at the level's start. */
	FWasamiMusicDoOnce OverrideOffOnce{true};
	/** Intense Music ? true: the panic music in, the regular out. */
	FWasamiMusicDoOnce IntenseOnce;
	/** Intense Music ? false: the regular music in, the panic out. */
	FWasamiMusicDoOnce CalmOnce;
};

/**
 * Dark Deception's BP_06_MusicPlayer (pak_reference_2's Blueprints/06_Hospital, its Update and the rest from its parent
 * BP_08_MusicPlayer): the one actor in a zone that plays the hospital's music. It holds three audio components that
 * start silent — the zone's regular track, the panic track both zones share, and an override track the hospital leaves
 * empty — and every half second from BeginPlay it looks at bFadeOut, bOverrideMusic and whether any enemy is chasing,
 * and crossfades between them over a second.
 *
 * The zones' flow (AWasamiZone1Flow, AWasamiZone2Flow) is what turns bFadeOut on and off, as the original's level
 * Blueprints do. Zone 2's player is AWasamiMusicPlayerZone2, whose regular track is that zone's, as the original's
 * BP_06_MusicPlayer_Zone2 is a child that swaps only that sound.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiMusicPlayer : public AActor
{
	GENERATED_BODY()

public:
	AWasamiMusicPlayer();

	/** The looping timer BeginPlay sets on Update. */
	static constexpr float UpdateInterval = 0.5f;
	/** Every fade's length (s). */
	static constexpr float FadeDuration = 1.f;
	/** What a fade in reaches. */
	static constexpr float FadeInVolume = 1.f;

	/**
	 * bFadeOut: the zone's flow raises it to take the music away (the lift's arrival, the scenes, the escape) and drops
	 * it to give it back. The level places Zone 1's player with it true, so the level opens in silence.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bFadeOut = false;

	/** bOverrideMusic: plays the override track instead of the other two. The hospital never raises it (its sound is empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bOverrideMusic = false;

	/** Update: one turn of the original's, every UpdateInterval. */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void Update();

	/** Intense Music ?: any actor of the enemy interface answering Chasing. */
	UFUNCTION(BlueprintPure, Category = "Music")
	bool IsIntenseMusic() const;

	/**
	 * Regular Music's FadeIn(Duration, Volume, 0, Linear), which Zone 2's cell scene makes itself: bFadeOut can only
	 * take all three away, and the scene wants the zone's track kept on under it, at a third of its volume.
	 */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void FadeRegularMusicIn(float Duration, float Volume);

	/**
	 * The three components' FadeOut(Duration, 0, Linear) at once — what bFadeOut's Update would do, asked for straight
	 * away. Zone 2's escape needs it: Escape pauses the game, which stops the 0.5 s Update from coming round and
	 * silences the game's sounds (FAudioDevice::HandlePause pauses every source that is not a UI sound), so the fade
	 * has to be begun and heard before the game stops.
	 */
	UFUNCTION(BlueprintCallable, Category = "Music")
	void FadeAllMusicOut(float Duration);

	/** What the last Update faded (none while nothing changed). For the tests and the tools. */
	const FWasamiMusicFades& GetLastFades() const { return LastFades; }

	/** The last FadeRegularMusicIn, or a negative duration when none has come. For the tests and the tools. */
	float GetLastRegularFadeInDuration() const { return LastRegularFadeInDuration; }
	float GetLastRegularFadeInVolume() const { return LastRegularFadeInVolume; }

	/** The last FadeAllMusicOut, or a negative duration when none has come. For the tests and the tools. */
	float GetLastFadeOutDuration() const { return LastFadeOutDuration; }

	UAudioComponent* GetRegularMusic() const { return RegularMusic; }
	UAudioComponent* GetPanicMusic() const { return PanicMusic; }
	UAudioComponent* GetOverrideMusic() const { return OverrideMusic; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Regular Music: the zone's ordinary track, not started until a fade in. */
	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> RegularMusic;

	/** Panic Music: the chase's track, the same in both zones. */
	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> PanicMusic;

	/** Override Music: empty in the hospital, as in the original; only bOverrideMusic reaches it. */
	UPROPERTY(VisibleAnywhere, Category = "Music")
	TObjectPtr<UAudioComponent> OverrideMusic;

	/** The tracks, put on the components when play begins (never loaded from a constructor, WasamiAssets.h). */
	UPROPERTY(EditAnywhere, Category = "Music|Assets")
	TSoftObjectPtr<USoundBase> RegularMusicSound;

	UPROPERTY(EditAnywhere, Category = "Music|Assets")
	TSoftObjectPtr<USoundBase> PanicMusicSound;

	UPROPERTY(EditAnywhere, Category = "Music|Assets")
	TSoftObjectPtr<USoundBase> OverrideMusicSound;

private:
	/** Puts one turn's fade on one component. */
	static void ApplyFade(UAudioComponent* Component, EWasamiMusicFade Fade);

	FWasamiMusicState State;
	FWasamiMusicFades LastFades;
	float LastRegularFadeInDuration = -1.f;
	float LastRegularFadeInVolume = -1.f;
	float LastFadeOutDuration = -1.f;
	FTimerHandle UpdateTimer;
};

/** BP_06_MusicPlayer_Zone2: the same player with Zone 2's regular track. */
UCLASS()
class WASAMI_DECEPTION_API AWasamiMusicPlayerZone2 : public AWasamiMusicPlayer
{
	GENERATED_BODY()

public:
	AWasamiMusicPlayerZone2();
};
