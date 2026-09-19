#include "WasamiMysteryCollectable.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "WasamiMysteryNoteWidget.h"

namespace
{
	// BP_MysteryCollectable's Plane_GEN_VARIABLE (pak_reference_2): the engine's Plane, stood up (roll 90) 215 cm above
	// the root and narrowed to 0.64 of its width, tagged interact; UStaticMeshComponent's BlockAllDynamic otherwise, so
	// the look's Visibility trace hits its box. The notes placed in Zone 2 put it back on their roots and scale those.
	const FVector MysteryPlaneLocation(0., -0.0001220703125, 215.42315673828125);
	const FRotator MysteryPlaneRotation(0., 0., 90.00009155273438);
	const FVector MysteryPlaneScale(0.6404496431350708, 1., 1.);
	const FName MysteryInteractTag(TEXT("interact"));
}

AWasamiMysteryCollectable::AWasamiMysteryCollectable()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// The engine's own content, so found here (WasamiAssets.h is about the pipeline's).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaneMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	Plane->SetupAttachment(DefaultSceneRoot);
	Plane->SetRelativeLocationAndRotation(MysteryPlaneLocation, MysteryPlaneRotation);
	Plane->SetRelativeScale3D(MysteryPlaneScale);
	Plane->SetStaticMesh(PlaneMesh.Object);
	Plane->SetMaterial(0, PlaneMaterial.Object);
	Plane->ComponentTags.Add(MysteryInteractTag);
}

void AWasamiMysteryCollectable::BeginPlay()
{
	Super::BeginPlay();
	LoadedAssets.Add(Texture.LoadSynchronous());
	UWasamiMysteryNoteWidget::LoadAssets(LoadedAssets);
}

void AWasamiMysteryCollectable::InteractWithObject_Implementation(AActor* Interactee)
{
	// InteractWithObject: Create(Self, UMG_MysteryNote_C, None), its Texture, Texts and E Note (= Lore Note) set by
	// name, AddToViewport(2).
	LastNote = UWasamiMysteryNoteWidget::Show(this, Texture.LoadSynchronous(), Texts, bLoreNote);
}
