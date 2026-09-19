#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiSecretRoomZone.generated.h"

class UAudioComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;
class USoundBase;

/**
 * A secret room's zone, after Dark Deception's Blueprints/Shared/BP_SecretRoomZone (pak_reference_2; Zone 2 of the
 * hospital has one, BP_SecretRoomZone_2, over its secret room): a box over the room, and the Chameleon (its ChildActor)
 * that glitches the screen. BeginPlay makes the whispers (67-Dark_Whispers_SFX_0704, 2D, kept). The player walking in
 * fades the whispers in over 1 s, turns the glitch on (its Glitch - Advanced's BlendingOpacity to 1) and, the first time
 * only, puts up YOU FOUND A MYSTERIOUS ROOM (UWasamiCollectablesSecretWidget, at Z 1); walking out fades them out over
 * 1 s and turns the glitch off (0). The room is not a secret the score counts.
 *
 * The Chameleon (ThirdParty/Chameleon, a post-process pack whose tick applies its settings) is stood in for by its one
 * effect: an unbound post process with M_DD_ChameleonGlitch (the estimate of its M_GlitchHLSL, implementation record
 * 18) at weight 1, with what the Chameleon's Glitch Func gives it: the zone's Glitch Speed 10, Glitch Lines 30 and
 * Glitch Blocking 0.5, and the class's grid distortion (0.001, 10, 1).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSecretRoomZone : public AActor
{
	GENERATED_BODY()

public:
	AWasamiSecretRoomZone();

	/**
	 * The player walked into the box (bBegin) or out: the whispers, the glitch and the first time's banner. The box's
	 * overlaps call it for the player only; the tests call it directly too.
	 */
	void NotifyPlayerOverlap(bool bBegin);

	/** The glitch's BlendingOpacity (0 off, 1 on). */
	float GetGlitchOpacity() const { return GlitchOpacity; }

	/** Whether the banner has been put up (the DoOnce is closed). */
	bool HasShownBanner() const { return bBannerShown; }

	UBoxComponent* GetBox() const { return Box; }
	UPostProcessComponent* GetGlitch() const { return Glitch; }
	UMaterialInstanceDynamic* GetGlitchMaterial() const { return GlitchMaterial; }
	/** Sound: the whispers (none where the world has no audio, as in a test's). */
	UAudioComponent* GetWhispers() const { return Whispers; }

	/** FadeIn(1, 1, 0) as the player comes, FadeOut(1, 0) as the player goes. */
	static constexpr float WhispersFadeSeconds = 1.f;

	/** The Chameleon's glitch: Glitch Speed, Glitch Lines, Glitch Blocking and the three Glitch Grid Distortion values. */
	static constexpr float GlitchSpeed = 10.f;
	static constexpr float GlitchLines = 30.f;
	static constexpr float GlitchBlocking = 0.5f;
	static constexpr float GlitchGridDistortionPower = 0.001f;
	static constexpr float GlitchGridDistortionSize = 10.f;
	static constexpr float GlitchGridDistortionSpeed = 1.f;

	/** 67-Dark_Whispers_SFX_0704. */
	UPROPERTY(EditAnywhere, Category = "Secret Room|Assets")
	TSoftObjectPtr<USoundBase> WhispersSound;

	/** M_DD_ChameleonGlitch. */
	UPROPERTY(EditAnywhere, Category = "Secret Room|Assets")
	TSoftObjectPtr<UMaterialInterface> GlitchBase;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Secret Room")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Box: 160 × 64 × 48 cm (half), 20 cm up; the level sizes its own (Zone 2's covers the room). */
	UPROPERTY(VisibleAnywhere, Category = "Secret Room")
	TObjectPtr<UBoxComponent> Box;

	/** The Chameleon's InternalPP: unbound, the glitch's material its one blendable. */
	UPROPERTY(VisibleAnywhere, Category = "Secret Room")
	TObjectPtr<UPostProcessComponent> Glitch;

private:
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	/** The Chameleon's Glitch - Advanced's BlendingOpacity (the rest of the struct is what the class had). */
	void SetGlitchOpacity(float Opacity);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Whispers;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlitchMaterial;

	/** What the banner uses, loaded at BeginPlay. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	float GlitchOpacity = 0.f;
	bool bBannerShown = false;
};
