#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiInteractable.h"
#include "WasamiMysteryCollectable.generated.h"

class UStaticMeshComponent;
class UTexture2D;
class UWasamiMysteryNoteWidget;

/**
 * A note to read, after Dark Deception's Blueprints/Main/BP_MysteryCollectable (pak_reference_2; the hospital's Zone 2
 * has three on the secret room's walls, mysteryroom_note_01 to 03): an engine Plane tagged interact that the player
 * uses by looking at it and clicking. Every use puts up UMG_MysteryNote (UWasamiMysteryNoteWidget, AddToViewport(2))
 * with the note's Texture, Texts and Lore Note; the note pauses the game until it is closed, so there is no DoOnce.
 * Nothing is saved (the notes are not among the SECRETS).
 *
 * The level build sets Texture, Texts (the string table's text) and the Plane's transform and material per note.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiMysteryCollectable : public AActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiMysteryCollectable();

	/** Texture: the paper on the note's screen (the original's default, sewer_note_01, is not imported). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mystery Collectable")
	TSoftObjectPtr<UTexture2D> Texture;

	/** Texts: the note's pages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mystery Collectable")
	TArray<FText> Texts;

	/** Lore Note: the screen's E Note (DD_LoreNote_01 fades in while it is up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mystery Collectable")
	bool bLoreNote = false;

	/** The note's screen the last use put up (none without a player's screen). */
	UWasamiMysteryNoteWidget* GetLastNote() const { return LastNote.Get(); }

	UStaticMeshComponent* GetPlane() const { return Plane; }

protected:
	virtual void BeginPlay() override;
	virtual void InteractWithObject_Implementation(AActor* Interactee) override;

	UPROPERTY(VisibleAnywhere, Category = "Mystery Collectable")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Plane: the engine's Plane with BasicShapeMaterial, stood up and narrowed, tagged interact. */
	UPROPERTY(VisibleAnywhere, Category = "Mystery Collectable")
	TObjectPtr<UStaticMeshComponent> Plane;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	TWeakObjectPtr<UWasamiMysteryNoteWidget> LastNote;
};
