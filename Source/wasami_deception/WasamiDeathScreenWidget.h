#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiDeathScreenWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWasamiPopUpWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiRespawnSignature);

/**
 * The death screen, after Dark Deception's Blueprints/UMG/UMG_DeathScreen (pak_reference_2): black, the row of lives
 * that loses one with a red shake, REMAINING LIVES and a tip, over which a black cover fades in and out; with no lives
 * left, YOU ARE DEAD and the three buttons instead (RESTART and LAST CHECKPOINT ask first with a UWasamiPopUpWidget).
 * The tree is built here as in the original, slot for slot; its Construct (lose a life, count the death in the save,
 * then the delays and the four animations) runs from NativeConstruct and is ticked by the widget, so it goes on while
 * the game is paused under it.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiDeathScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** The original's Enum_Levels values the level Blueprints give Level: which death lines play. */
	static constexpr uint8 TrapsLevel = 4;
	static constexpr uint8 AsylumLevel = 7;

	/**
	 * Level, set before the screen is added: Zone 1 gives Traps when the player caused the death and Asylum otherwise,
	 * Zone 2 the other way round.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Death")
	uint8 Level = AsylumLevel;

	/**
	 * What the level Blueprints' DeathEvent does with the screen: creates it for the first player, gives it Level, adds
	 * it to the viewport at Z 5 and pauses the game. Returns the screen (null without a player controller).
	 */
	UFUNCTION(BlueprintCallable, Category = "Death", meta = (WorldContext = "WorldContextObject"))
	static UWasamiDeathScreenWidget* Show(const UObject* WorldContextObject, uint8 InLevel);

	/** The Z order the level Blueprints add the screen at. */
	static constexpr int32 ViewportZOrder = 5;

	/** Respawn Event: with lives left, after the fade out and the powers' reset, just before the level opens again. */
	UPROPERTY(BlueprintAssignable, Category = "Death")
	FWasamiRespawnSignature OnRespawn;

	// Construct's delays, in seconds.
	/** The two Delays of 0.5 before the life-lost sound and the reveal (the death line, Fade In, Life Animation). */
	static constexpr float RevealDelay = 0.5f;
	/** Fade In plays at 1.5. */
	static constexpr float FadeInSpeed = 1.5f;
	/** Life Animation's Delay before Shake. */
	static constexpr float ShakeDelay = 1.f;
	/** The screen moves on when the death line ends or after this, whichever is first (a DoOnce). */
	static constexpr float VoiceLimit = 6.f;
	/** Then this Delay before Fade Out. */
	static constexpr float ProceedDelay = 0.5f;
	/** And this after Fade Out begins before the respawn. */
	static constexpr float RespawnDelay = 2.f;
	/** With no lives left: the Delay after the reveal before the music and Death, and the one before the buttons. */
	static constexpr float GameOverDelay = 0.25f;
	static constexpr float ButtonsDelay = 2.f;

	static constexpr float LifeLostVolume = 0.6f;
	static constexpr float GameOverVolume = 0.7f;

	/** Life1 to Life6. */
	static constexpr int32 LifeIcons = 6;

	/** A game over button's way out: Fade Out, then this Delay before the level opens. */
	static constexpr float LeaveDelay = 1.f;
	/** The Z orders RESTART's question and LAST CHECKPOINT's warning are added at. */
	static constexpr int32 RestartPopUpZOrder = 5;
	static constexpr int32 WarningPopUpZOrder = 10;

	/** The game over button whose way out began. */
	enum class EChoice : uint8
	{
		None,
		Restart,
		LastCheckpoint,
		QuitToTitle,
	};

	/** The animations' lengths. */
	static constexpr float FadeLength = 1.f;
	static constexpr float ShakeLength = 1.f;
	static constexpr float DeathLength = 3.5f;
	/** Shake's event Update Life (the lost life leaves the row). */
	static constexpr float UpdateLifeTime = 0.1f;

	/**
	 * Construct's run from its start, with the lives left after the one lost, how long the death line lasts, and
	 * whether LAST CHECKPOINT may show; NativeConstruct calls it after the game's side (the tests call it alone).
	 */
	void Begin(int32 LivesLeft, float VoiceSeconds, bool bHasCheckpoint);

	/** Moves the delays and the animations on by DeltaSeconds. */
	void Advance(float DeltaSeconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Button_0's black over everything but the vignette (1 = opaque). */
	float GetCoverAlpha() const { return CoverAlpha; }

	/** How many lives the row shows. */
	int32 GetShownLives() const { return ShownLives; }

	/** Whether the screen went on to the game over. */
	bool IsGameOver() const { return bGameOver; }

	/** Whether the game over's buttons can be pressed. */
	bool AreButtonsShown() const { return bButtonsShown; }

	/** Whether the screen is done and asked for the respawn. */
	bool HasRespawned() const { return bRespawned; }

	/**
	 * RESTART's click: asks ARE YOU SURE YOU WANT TO RESTART? (Frame 0, Z 5); its YES is RestartEvent. LAST
	 * CHECKPOINT's: without the save's Last Checkpoint Warning, warns that the S rank is lost (Frame 2, Z 10) with the
	 * select sound, whose YES is LastCheckpointEvent; with it, goes on at once. QUIT TO TITLE's: fades out and goes.
	 */
	void PressRestart();
	void PressLastCheckpoint();
	void PressQuitToTitle();

	/**
	 * RESTART's YES: closes the question, lives back to 3, the shards forgotten, and the level's save entry emptied and
	 * written; Fade Out, and 1 s later the level opens again at no checkpoint (Zone 1's arrival).
	 */
	UFUNCTION()
	void RestartEvent();

	/** LAST CHECKPOINT's YES: the warning never again (written), and on as without it. */
	UFUNCTION()
	void LastCheckpointEvent();

	/** Which way out began (Fade Out), and whether its Delay ran out and the level was asked to open. */
	EChoice GetChoice() const { return Choice; }
	bool HasLeft() const { return bLeft; }

	/** The tip the screen shows. */
	FText GetTip() const { return Tip; }

	/** When Fade Out begins, from Begin, with lives left and a death line that lasts VoiceSeconds. */
	static float ProceedTime(float VoiceSeconds);

	/** The death lines' lengths for a Level (Bierce's lines in the original; the voices come with item 20). */
	static TConstArrayView<float> VoiceLengths(uint8 InLevel);

	/** Set Tip's texts for a level name ('L_Hospital_Zone1' / 'L_Hospital_Zone2'); other levels have one. */
	static TArray<FText> TipsFor(const FString& LevelName);

	// The animations' curves at Seconds into them.
	static float EvaluateFadeIn(float Seconds);
	static float EvaluateFadeOut(float Seconds);
	static float EvaluateShakeVignetteAlpha(float Seconds);
	static float EvaluateShakeVignetteScale(float Seconds);
	static FVector2D EvaluateShakeTranslation(float Seconds);
	/** The lives' green and blue (their red and alpha stay 1). */
	static float EvaluateShakeLifeTint(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** life_icon_02: one life. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<UTexture2D> LifeTexture;

	/** you_are_dead. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<UTexture2D> DeadTexture;

	/** T_Vignette: the red edge the shake flashes. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<UTexture2D> VignetteTexture;

	/** helvetica-neue-bold_Font: REMAINING LIVES. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<UFont> HeadingFont;

	/** helvetica-normal_Font: the buttons. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<UFont> MenuFont;

	/** The engine's RobotoTiny (its Light typeface): the tip. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<UFont> TipFont;

	/** Life_Lost. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<USoundBase> LifeLostSound;

	/** 66_-_Game_Over. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<USoundBase> GameOverSound;

	/** UI_Select_V3: LAST CHECKPOINT's warning. */
	UPROPERTY(EditAnywhere, Category = "Death|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

private:
	/** What a Delay of Construct resumes. */
	enum class EStep : uint8
	{
		LifeLost,
		Reveal,
		Shake,
		Proceed,
		FadeOut,
		Respawn,
		GameOver,
		Buttons,
		// The game over buttons' ways out, after Fade Out and LeaveDelay.
		Restart,
		LastCheckpoint,
		QuitToTitle,
	};

	struct FPendingStep
	{
		float At;
		EStep Step;
	};

	void BuildScreen(UCanvasPanel* Root);
	void Schedule(EStep Step, float Delay);
	void RunStep(EStep Step);
	void LifeAnimation();
	void UpdateLife();
	void ApplyAnimations();
	void PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Volume) const;

	/** LAST CHECKPOINT past its warning: DoOnce, the cursor off, Fade Out, and the level again after LeaveDelay. */
	void LastCheckpointProceed();
	/** A way out's DoOnce passed: the cursor off, Fade Out, and Step after LeaveDelay. */
	void BeginLeaving(EChoice InChoice, EStep Step);
	/** SetInputMode_GameOnly and OpenLevel, the current level without a name (with no world, only marks the screen left). */
	void OpenLevel(const TCHAR* LevelName = nullptr);

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnLastCheckpointClicked();

	UFUNCTION()
	void OnQuitClicked();

	// The buttons' hover: white, and back to Unhover Color.
	UFUNCTION()
	void OnRestartHovered();

	UFUNCTION()
	void OnRestartUnhovered();

	UFUNCTION()
	void OnLastCheckpointHovered();

	UFUNCTION()
	void OnLastCheckpointUnhovered();

	UFUNCTION()
	void OnQuitHovered();

	UFUNCTION()
	void OnQuitUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> LivesArray;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Cover;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Heading;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TipText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> YouAreDead;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> Menu;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LastCheckpointButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ButtonTexts;

	/** RESTART's question, which RestartEvent closes. */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiPopUpWidget> RestartPopUp;

	TArray<FPendingStep> Pending;
	float Elapsed = 0.f;
	/** When the step being run was due (its Delays count from there). */
	float StepTime = 0.f;
	int32 LocalLives = 0;
	float VoiceLength = 0.f;
	bool bHasCheckpoint = false;
	bool bProceeded = false;
	bool bGameOver = false;
	bool bButtonsShown = false;
	bool bRespawned = false;
	FText Tip;
	/** Each way out's DoOnce, as in the original. */
	bool bRestartClosed = false;
	bool bLastCheckpointClosed = false;
	bool bQuitClosed = false;
	EChoice Choice = EChoice::None;
	bool bLeft = false;

	/** Seconds into each animation (already scaled by its speed); negative when it is not playing. */
	float FadeInTime = -1.f;
	float FadeOutTime = -1.f;
	float ShakeTime = -1.f;
	float DeathTime = -1.f;
	bool bLifeUpdated = false;
	float CoverAlpha = 1.f;
	int32 ShownLives = LifeIcons;
};
