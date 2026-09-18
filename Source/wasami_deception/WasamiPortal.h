#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiPortal.generated.h"

class UArrowComponent;
class UAudioComponent;
class UBoxComponent;
class UCameraShakeBase;
class UMaterialInterface;
class UParticleSystemComponent;
class UPointLightComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Dark Deception's BP_00_Teleport (pak_reference_2's Blueprints/00_Ballroom, the latest version's), the ballroom's
 * portals, which the hotel also leaves by: a disc of three spinning planes (Vortex, and Outer and Inner on it) with a
 * logo in the middle, a lock over it while it is locked, a strobing light, a humming loop, and a burst and a camera shake
 * as it opens (Lock/Unlock). The garage of the hospital's Zone 2 has one, placed as the hotel's exit is (locked, masked,
 * at 2.5 times its size), which the zone's flow opens once the ring piece is taken.
 *
 * What this game leaves out: walking into an open portal moves the player to its Cube (ReceiveActorBeginOverlap with
 * CanTeleport?, then UseTeleport), which the hotel's exit does not do (CanTeleport? false: a trigger by it ends the
 * level) and neither does the garage's; the ballroom's extra lights (Lights, Update Lights), which the hotel's exit has
 * none of; and the character on the logo (Portal Enemy's M_00_Portal_Monkey and the others), which is this game's
 * Wasami symbol (MI_Portal_Wasami). The bursts' particle systems (PPP_PortalAppear, _Lock) are not made yet, so their
 * components have no template and activating them shows nothing.
 *
 * The original's portal refers to a BP_00_StrobingLight placed by it in the level (Strobing Light, Strobing Light 2: the
 * same one at the hotel's exit), which Lock/Unlock colours; here that light is the portal's own StrobingLight, where the
 * hotel's is from its exit.
 *
 * Its materials, mesh, sounds and shake are loaded when the actor is built (OnConstruction) and as play begins: assets
 * under /Game/DD are never loaded from a constructor (WasamiAssets.h).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPortal : public AActor
{
	GENERATED_BODY()

public:
	AWasamiPortal();

	/** Locked?: shut, grey and quiet (the placed portal's state as the level starts, and after Lock/Unlock). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bLocked = false;

	/** bMaskedPortalMaterial: the vortex drawn with the masked instances (the hotel's exit sets it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bMaskedPortalMaterial = false;

	/** Logo Scale: the logo's size (x 0.02, the plane's) while the portal is open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	float LogoScale = 1.f;

	/** Locked Light Color: the strobing light's colour while the portal is locked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FLinearColor LockedLightColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.f);

	/** BP_00_StrobingLight's Light Intensity: the light's brightness at the top of its strobe (the hotel's exit's). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	float StrobingLightIntensity = 3000.f;

	/**
	 * Lock/Unlock (the original's custom event): locked, the logo shrinks to nothing, the lock shows, the planes take
	 * their locked materials, the light Locked Light Color and the loop falls silent (with bPlaySoundAndShake, the lock's
	 * burst); open, the logo grows back, the lock hides, the open materials, a red light and the loop at half volume
	 * (with bPlaySoundAndShake, portal_unlocked, BP_Portal_CameraShake around the logo and the opening burst).
	 */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void LockUnlock(bool bLock, bool bPlaySoundAndShake);

	/** BP_00_StrobingLight's Strobe: the timeline's value (0.5 – 1) Seconds into its 2 s loop. */
	static float EvaluateStrobe(float Seconds);

	static constexpr float StrobeLength = 2.f;

	UStaticMeshComponent* GetVortex() const { return Vortex; }
	UStaticMeshComponent* GetOuter() const { return Outer; }
	UStaticMeshComponent* GetInner() const { return Inner; }
	UStaticMeshComponent* GetLogo() const { return Logo; }
	UStaticMeshComponent* GetLogoLock() const { return LogoLock; }
	UBoxComponent* GetCollision() const { return Collision; }
	UPointLightComponent* GetStrobingLight() const { return StrobingLight; }
	UAudioComponent* GetPortalLoop() const { return PortalLoop; }
	UAudioComponent* GetAudio() const { return Audio; }
	UParticleSystemComponent* GetPortalAppear() const { return PortalAppear; }
	UParticleSystemComponent* GetPortalAppearLock() const { return PortalAppearLock; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The look of bLocked (the materials, the logo and the lock, the light's colour and the loop's volume). */
	void ApplyLock(const FLinearColor& OpenLightColor);

	/** The meshes and materials, loaded (from OnConstruction and BeginPlay). */
	void LoadLook();

	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Collision: a box where the disc stands (UBoxComponent's defaults: overlapping, blocking nothing). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UBoxComponent> Collision;

	/** Cube: where a portal that moves the player puts it (the engine's cube, hidden in game). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Cube;

	/** Arrow: the way the player faces after the move. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UArrowComponent> Arrow;

	/** Vortex: the disc (circle_portal_decal, 70 times its size, a metre up), turned to stand. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Vortex;

	/** Outer: the outer ring, on the Vortex, 5/7 its size. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Outer;

	/** Inner: the inner ring, on the Outer. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Inner;

	/** Logo: the engine's Plane on the Inner, the logo's material. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> Logo;

	/** LogoLock: the lock, a Plane beside the Logo (M_00_Portal_Lock), shown while locked. */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UStaticMeshComponent> LogoLock;

	/** Portal Loop: Portal_Sound_v3, on the Logo, through its own attenuation (natural sound, 2000 cm). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UAudioComponent> PortalLoop;

	/** Audio: portal_unlocked, on the Logo, played as the portal opens (a sphere of 1000 cm, falling off over 2000). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UAudioComponent> Audio;

	/** PPP_PortalAppear: the burst as it opens (no template yet). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UParticleSystemComponent> PortalAppear;

	/** PPP_PortalAppear_Lock: the burst as it locks (no template yet). */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UParticleSystemComponent> PortalAppearLock;

	/**
	 * The level's BP_00_StrobingLight by the hotel's exit, the portal's own here: 80 cm behind the disc (its back, -X;
	 * the logo and the lock lie on its front, +X, the way the Arrow points), strobing.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Portal")
	TObjectPtr<UPointLightComponent> StrobingLight;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UStaticMesh> DiscMesh;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> VortexMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> VortexMaskedMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> VortexLockedMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> VortexLockedMaskedMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> OuterMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> OuterLockedMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> InnerMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> InnerLockedMaterial;

	/** The logo: this game's Wasami symbol, where the original shows Portal Enemy's face. */
	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> LogoMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<UMaterialInterface> LockMaterial;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<USoundBase> LoopSound;

	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftObjectPtr<USoundBase> UnlockedSound;

	/** BP_Portal_CameraShake, played around the logo as the portal opens (0 – 3000 cm). */
	UPROPERTY(EditAnywhere, Category = "Portal|Assets")
	TSoftClassPtr<UCameraShakeBase> OpenShakeClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedVortexMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedVortexMaskedMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedVortexLockedMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedVortexLockedMaskedMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedOuterMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedOuterLockedMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedInnerMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedInnerLockedMaterial;

	/** Where the strobe's timeline is in its loop. */
	float StrobeSeconds = 0.f;
};
