#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiTelepathyTracker.generated.h"

class UMaterialInterface;
class UPostProcessComponent;
class USkinnedMeshComponent;

/**
 * One telepathy marker, after Dark Deception's BP_TelepathyTracker (pak_reference_2). The telepathy spawns one on each
 * enemy it finds; every tick it moves onto its enemy. The original's tracker shows a smoky red disc over the enemy (the
 * screen-space widget UMG_TelepathyTracker); this game marks the enemy's body instead (the user's choice, 2026-09-27):
 * each of the enemy's skinned meshes renders into custom depth with a stencil value, which the post-process material
 * MI_WasamiTelepathySilhouette (on the tracker's unbound post-process component) draws as red smoke on and around the
 * body, through walls. The stencil carries the marker's fade: in over the original's Appear (0.5 s), out over its
 * Disappear (0.3 s). Remove stops the following, fades out and destroys the tracker 0.5 s later; a tracker whose enemy
 * is gone removes itself.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTelepathyTracker : public AActor
{
	GENERATED_BODY()

public:
	AWasamiTelepathyTracker();

	virtual void Tick(float DeltaSeconds) override;

	/** Loads what the markers show into Out, so that the first telepathy does not wait for it. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The fade in (the original's Appear opacity) at a time (s) of its 0.5 s, and out (Disappear's) of its 0.3 s. */
	static float EvaluateAppearOpacity(float Seconds);
	static float EvaluateDisappearOpacity(float Seconds);
	static constexpr float AppearLength = 0.5f;
	static constexpr float DisappearLength = 0.3f;

	/** The custom depth stencil value a fade (0 - 1) is drawn with: 0 shows nothing, 255 the whole smoke. */
	static int32 StencilForFade(float Fade);

	/** Remove: stops following, fades out, and destroys the tracker 0.5 s later. */
	UFUNCTION(BlueprintCallable, Category = "Telepathy")
	void Remove();

	/** The enemy followed (the telepathy sets it before the spawn finishes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telepathy", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<AActor> Actor;

	/** The smoke's post-process material. */
	UPROPERTY(EditAnywhere, Category = "Telepathy")
	TSoftObjectPtr<UMaterialInterface> SilhouetteMaterial;

	/** Whether the Gate before Update is open (from BeginPlay until Remove). */
	bool IsFollowing() const { return bGateOpen; }
	bool IsFadingOut() const { return bFadingOut; }
	float GetFade() const { return Fade; }
	UPostProcessComponent* GetPostProcess() const { return PostProcess; }
	const TArray<TWeakObjectPtr<USkinnedMeshComponent>>& GetMarkedMeshes() const { return MarkedMeshes; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telepathy")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Unbound, with the silhouette material; the engine draws one material once however many trackers hold it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Telepathy")
	TObjectPtr<UPostProcessComponent> PostProcess;

private:
	/** Update: onto the enemy; Remove when the enemy is gone. */
	void Update();
	/** The fade by the time since the fade in or out began, onto the marked meshes' stencil. */
	void UpdateFade();
	/** Custom depth on the enemy's skinned meshes (the body), remembered. */
	void MarkMeshes();
	/** Custom depth off again on the meshes no other tracker marks. */
	void UnmarkMeshes();
	void DestroyAfterRemove() { Destroy(); }

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<USkinnedMeshComponent>> MarkedMeshes;

	/** The Gate, which starts closed. */
	bool bGateOpen = false;
	bool bFadingOut = false;
	float FadeTime = 0.f;
	float Fade = 0.f;
	FTimerHandle RemoveTimer;
};
