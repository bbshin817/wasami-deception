#include "WasamiViewcone.h"

#include "Components/StaticMeshComponent.h"
#include "Curves/RichCurve.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WasamiAssets.h"

namespace
{
	// The player's map capture draws the actors with this tag (AWasamiPlayerCharacter).
	const FName ViewconeMinimapTag(TEXT("dd_minimap"));
	const FName ViewconeOpacityName(TEXT("Opacity"));

	/** Fade In's Visibility track: CurveFloat_0, two cubic keys whose auto tangents are flat. */
	FRichCurve MakeFadeInCurve()
	{
		FRichCurve Curve;
		for (const FVector2f& Each : {FVector2f(0.f, 0.f), FVector2f(AWasamiViewcone::FadeInLength, 1.f)})
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(Each.X, Each.Y));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_Auto;
		}
		Curve.AutoSetTangents();
		return Curve;
	}

	/** A plane that is only a mark for the map: no shadow, no collision, drawn in scene captures only. */
	UStaticMeshComponent* MakeMapPlane(AActor* Owner, const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Plane = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Plane->SetupAttachment(Parent);
		Plane->SetStaticMesh(Mesh);
		Plane->SetCastShadow(false);
		Plane->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Plane->SetCanEverAffectNavigation(false);
		Plane->bVisibleInSceneCaptureOnly = true;
		return Plane;
	}
}

// BP_06_Miniboss_viewcone's Plane_GEN_VARIABLE and Plane1_GEN_VARIABLE, and ReceiveBeginPlay's offset.
const FVector AWasamiViewcone::PlaneLift(0., 0., 1000.);
const FVector AWasamiViewcone::PlaneLocation(4.800000190734863, 0., 2000.);
const FVector AWasamiViewcone::DotLocation(0., 0., 1000.);

AWasamiViewcone::AWasamiViewcone()
{
	// The original ticks, doing nothing; here the tick plays Fade In only.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Hidden until play begins (the class's bHidden).
	SetHidden(true);
	Tags.Add(ViewconeMinimapTag);

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	Scene->SetupAttachment(DefaultSceneRoot);

	// The engine's own content, which the pipeline never rebuilds, so it may load here.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	Plane = MakeMapPlane(this, TEXT("Plane"), Scene, PlaneMesh.Object);
	Plane->SetRelativeLocation(PlaneLocation);
	Plane1 = MakeMapPlane(this, TEXT("Plane1"), Scene, PlaneMesh.Object);
	Plane1->SetRelativeLocation(DotLocation);

	FanMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Blueprints/06_Hospital/Miniboss/Tex/map_enemy_search_Mat")));
	DotMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/M5VFXVOL2/Materials/Master/0_DotCircle_Mat")));
}

void AWasamiViewcone::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Fan = FanMaterial.LoadSynchronous())
	{
		Plane->SetMaterial(0, Fan);
	}
	if (UMaterialInterface* Dot = DotMaterial.LoadSynchronous())
	{
		Plane1->SetMaterial(0, Dot);
	}
	SetActorHiddenInGame(false);
	// Delay(0), then the player kept (Get DD Player Character: nothing reads it), the fan's Opacity 0 and the lift.
	GetWorldTimerManager().SetTimerForNextTick(this, &AWasamiViewcone::BeginPlayNextTick);
}

void AWasamiViewcone::BeginPlayNextTick()
{
	Plane->SetScalarParameterValueOnMaterials(ViewconeOpacityName, 0.f);
	Plane->AddLocalOffset(PlaneLift);
}

void AWasamiViewcone::Initialize()
{
	GetWorldTimerManager().SetTimer(SightTimer, this, &AWasamiViewcone::UpdateSight,
		FMath::FRandRange(SightRateMin, SightRateMax), true);
}

bool AWasamiViewcone::IsInitialized() const
{
	return GetWorldTimerManager().IsTimerActive(SightTimer);
}

void AWasamiViewcone::UpdateSight()
{
	// Delay(Offset): a call while it waits is dropped. From the second on Offset is 0, a Delay that ends on the next
	// tick; the checks run at once here.
	if (GetWorldTimerManager().IsTimerActive(SightDelayTimer))
	{
		return;
	}
	if (Offset > 0.f)
	{
		GetWorldTimerManager().SetTimer(SightDelayTimer, this, &AWasamiViewcone::LookForPlayer, Offset, false);
		return;
	}
	LookForPlayer();
}

void AWasamiViewcone::LookForPlayer()
{
	Offset = 0.f;
	// A sequence: first the sight, then the end of initializing.
	if (PlayerInsideCone() && PlayerInFullView() && !bSpottedClosed)
	{
		bSpottedClosed = true;
		TellPlayerSpotted();
		// A sentry destroys its cone as it is spotted (its second DoOnce is closed by then in the original).
		if (IsActorBeingDestroyed())
		{
			return;
		}
	}
	if (!bInitializeFinished)
	{
		bInitializeFinished = true;
		InitializeFinished();
		if (bAutoOn)
		{
			TurnOn();
		}
	}
}

void AWasamiViewcone::TellPlayerSpotted()
{
	// The Blueprint VM drops a message to an actor already destroyed (an invalid context): the Matron's cones outlive her
	// (the maze's start takes the enemies away) and keep their owner.
	AActor* Parent = GetParentActor();
	AActor* Carrier = GetOwner();
	if (IsValid(Parent) && Parent->Implements<UWasamiViewconeInterface>())
	{
		IWasamiViewconeInterface::Execute_PlayerSpotted(Parent);
	}
	if (IsValid(Carrier) && Carrier->Implements<UWasamiViewconeInterface>())
	{
		IWasamiViewconeInterface::Execute_PlayerSpotted(Carrier);
	}
}

bool AWasamiViewcone::PlayerInsideCone() const
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!bOn || !Player)
	{
		return false;
	}
	const FVector Toward = UKismetMathLibrary::Normal(Player->GetActorLocation() - GetActorLocation(), 0.0001f);
	const bool bWithinAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(Toward, GetActorForwardVector())) < Angle;
	return bWithinAngle && GetDistanceTo(Player) < Length;
}

bool AWasamiViewcone::PlayerInFullView() const
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!bOn || !Player)
	{
		return false;
	}
	// LineTraceSingle on the Camera channel, complex, past the carrier and itself: Vanish has the player's capsule ignore
	// Camera. The original casts what it hits to its player character's class; here it is the player itself.
	TArray<AActor*> Ignored;
	if (AActor* Parent = GetParentActor())
	{
		Ignored.Add(Parent);
	}
	FHitResult Hit;
	if (!UKismetSystemLibrary::LineTraceSingle(this, GetActorLocation(), Player->GetActorLocation(),
		UEngineTypes::ConvertToTraceType(ECC_Camera), true, Ignored, EDrawDebugTrace::None, Hit, true))
	{
		return false;
	}
	return Hit.Distance <= Length && Hit.GetActor() == Player && PlayerInsideCone();
}

void AWasamiViewcone::TurnOn()
{
	if (AActor* Parent = GetParentActor(); IsValid(Parent) && Parent->Implements<UWasamiViewconeInterface>())
	{
		IWasamiViewconeInterface::Execute_StartLooking(Parent);
	}
	if (!GetWorldTimerManager().IsTimerActive(TurnOnTimer))
	{
		GetWorldTimerManager().SetTimer(TurnOnTimer, this, &AWasamiViewcone::SeeAfterTurnOn, TurnOnDelay, false);
	}
}

void AWasamiViewcone::SeeAfterTurnOn()
{
	bOn = true;
	// Fade In's Play: forward from where it is.
	FadeDirection = 1;
	SetActorTickEnabled(true);
}

void AWasamiViewcone::TurnOff()
{
	if (AActor* Parent = GetParentActor(); IsValid(Parent) && Parent->Implements<UWasamiViewconeInterface>())
	{
		IWasamiViewconeInterface::Execute_StopLooking(Parent);
	}
	bOn = false;
	// Fade In's Reverse: back from where it is.
	FadeDirection = -1;
	SetActorTickEnabled(true);
}

float AWasamiViewcone::GetFade() const
{
	static const FRichCurve Curve = MakeFadeInCurve();
	return Curve.Eval(FadePosition);
}

void AWasamiViewcone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// FTimeline: moves the position, stops at either end (and updates there once more).
	FadePosition = FMath::Clamp(FadePosition + FadeDirection * DeltaSeconds, 0.f, FadeInLength);
	UpdateFade();
	if ((FadeDirection > 0 && FadePosition >= FadeInLength) || (FadeDirection < 0 && FadePosition <= 0.f))
	{
		FadeDirection = 0;
	}
	if (FadeDirection == 0)
	{
		SetActorTickEnabled(false);
	}
}

void AWasamiViewcone::UpdateFade()
{
	Plane->SetScalarParameterValueOnMaterials(ViewconeOpacityName, GetFade());
}

// BP_06_Miniboss_viewcone_Nurse's Plane_GEN_VARIABLE.
const FVector AWasamiViewconeNurse::NursePlaneLocation(837.800048828125, 0., 0.);
const FVector AWasamiViewconeNurse::NursePlaneScale(17.00623321533203, 9.620299339294434, 28.);

AWasamiViewconeNurse::AWasamiViewconeNurse()
{
	Length = 1500.f;
	Angle = 20.f;
	Plane->SetRelativeLocation(NursePlaneLocation);
	Plane->SetRelativeScale3D(NursePlaneScale);
}

void AWasamiViewconeNurse::InitializeFinished()
{
	GetWorldTimerManager().SetTimer(TurnTimer, this, &AWasamiViewconeNurse::Turn, TurnRate, true);
}

void AWasamiViewconeNurse::Turn()
{
	bTurnedOff = !bTurnedOff;
	if (bTurnedOff)
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

namespace
{
	/** The Matron's cones' class: Plane and Plane1 at 0, Plane1 hidden, and not turned on by initializing. */
	void MakeMatronCone(AWasamiViewcone& Cone, float Length, float Angle)
	{
		Cone.Length = Length;
		Cone.Angle = Angle;
		Cone.bAutoOn = false;
		Cone.GetPlane()->SetRelativeLocation(FVector::ZeroVector);
		Cone.GetDot()->SetRelativeLocation(FVector::ZeroVector);
		Cone.GetDot()->SetVisibility(false);
	}
}

AWasamiViewconeMatronLong::AWasamiViewconeMatronLong()
{
	MakeMatronCone(*this, 3000.f, 35.f);
}

AWasamiViewconeMatronShort::AWasamiViewconeMatronShort()
{
	MakeMatronCone(*this, 1350.f, 35.f);
}
