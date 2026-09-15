#include "WasamiPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The original's mouse axes (its DefaultInput.ini): Sensitivity 0.07 after UE4's mouse smoothing, then UE4's FOV
	// scaling (FOVScale 0.01111); AddControllerYaw/PitchInput scale by the controller's legacy 2.5 / −2.5
	// (bEnableLegacyInputScales in Config/DefaultInput.ini).
	constexpr float MouseAxisSensitivity = 0.07f;
	constexpr float MouseFOVScale = 0.01111f;

	// The original's FOV Multiplier runs on a 0.001 s looping timer: several times a frame, each with the frame's delta.
	constexpr float FOVTimerRate = 0.001f;

	// Update Bob stops the head bob at or below this speed (cm/s).
	constexpr float BobMinSpeed = 1.f;
}

AWasamiPlayerCharacter::AWasamiPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// CollisionCylinder: radius 50, ACharacter's half height 88, walkable slopes up to 44°.
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(50.f, 88.f);
	Capsule->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Increase, 44.f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Capsule);
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 95.f));
	SpringArm->TargetArmLength = 0.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 20.f;
	SpringArm->CameraLagSpeed = 8.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(BaseFOV);

	GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;

	static ConstructorHelpers::FClassFinder<UCameraShakeBase> WalkShake(TEXT("/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake"));
	static ConstructorHelpers::FClassFinder<UCameraShakeBase> RunShake(TEXT("/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_RunShake"));
	WalkShakeClass = WalkShake.Class;
	RunShakeClass = RunShake.Class;
}

void AWasamiPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplySpeed();
	GetWorldTimerManager().SetTimer(FOVTimer, this, &AWasamiPlayerCharacter::UpdateFOV, FOVTimerRate, true);
}

void AWasamiPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (BoostTimeLeft > 0.f)
	{
		BoostTimeLeft = FMath::Max(0.f, BoostTimeLeft - DeltaSeconds);
		if (BoostTimeLeft == 0.f)
		{
			ApplySpeed();
		}
	}
	BoostCooldownLeft = FMath::Max(0.f, BoostCooldownLeft - DeltaSeconds);

	UpdateHeadBob();
}

float AWasamiPlayerCharacter::GetBoostCharge() const
{
	const float Total = BoostDuration + BoostCooldown;
	return Total > 0.f ? 1.f - BoostCooldownLeft / Total : 1.f;
}

void AWasamiPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInput();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputContext, 0);
		}
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWasamiPlayerCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWasamiPlayerCharacter::Look);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::SprintPressed);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AWasamiPlayerCharacter::SprintReleased);
	Input->BindAction(TurnAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::TurnAround);
	Input->BindAction(BoostAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::UseBoost);
}

void AWasamiPlayerCharacter::CreateInput()
{
	if (InputContext)
	{
		return;
	}

	auto NewAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = NewAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = NewAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	SprintAction = NewAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	TurnAction = NewAction(TEXT("IA_180Turn"), EInputActionValueType::Boolean);
	BoostAction = NewAction(TEXT("IA_UsePowerRight"), EInputActionValueType::Boolean);

	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Player"));
	auto Map = [this](const UInputAction* Action, const FKey& Key, const TArray<UInputModifier*>& Modifiers = {})
	{
		FEnhancedActionKeyMapping& Mapping = InputContext->MapKey(Action, Key);
		for (UInputModifier* Modifier : Modifiers)
		{
			Mapping.Modifiers.Add(Modifier);
		}
	};
	auto Swizzle = [this]() -> UInputModifier* { return NewObject<UInputModifierSwizzleAxis>(this); };
	auto Negate = [this]() -> UInputModifier* { return NewObject<UInputModifierNegate>(this); };

	// The original's Forward (W +1, S −1) and Left (A −1, D +1) axes, and the arrow keys.
	for (const FKey& Key : {EKeys::W, EKeys::Up})
	{
		Map(MoveAction, Key, {Swizzle()});
	}
	for (const FKey& Key : {EKeys::S, EKeys::Down})
	{
		Map(MoveAction, Key, {Swizzle(), Negate()});
	}
	for (const FKey& Key : {EKeys::D, EKeys::Right})
	{
		Map(MoveAction, Key);
	}
	for (const FKey& Key : {EKeys::A, EKeys::Left})
	{
		Map(MoveAction, Key, {Negate()});
	}

	UInputModifierScalar* Sensitivity = NewObject<UInputModifierScalar>(this);
	Sensitivity->Scalar = FVector(MouseAxisSensitivity, MouseAxisSensitivity, 1.f);
	UInputModifierFOVScaling* FOVScaling = NewObject<UInputModifierFOVScaling>(this);
	FOVScaling->FOVScale = MouseFOVScale;
	FOVScaling->FOVScalingType = EFOVScalingType::UE4_BackCompat;
	Map(LookAction, EKeys::Mouse2D, {NewObject<UInputModifierSmooth>(this), Sensitivity, FOVScaling});

	Map(SprintAction, EKeys::LeftShift);
	Map(TurnAction, EKeys::MiddleMouseButton);
	Map(BoostAction, EKeys::E);
}

void AWasamiPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotationMatrix Yaw(FRotator(0.f, GetControlRotation().Yaw, 0.f));
	AddMovementInput(Yaw.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(Yaw.GetUnitAxis(EAxis::Y), Axis.X);
}

void AWasamiPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * MouseSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(bInvertY ? Axis.Y : -Axis.Y);
}

void AWasamiPlayerCharacter::SprintPressed()
{
	if (bToggleSprint)
	{
		bSprintLatch = !bSprintLatch;
	}
	else
	{
		bSprintHeld = true;
	}
	ApplySpeed();
}

void AWasamiPlayerCharacter::SprintReleased()
{
	if (!bToggleSprint)
	{
		bSprintHeld = false;
		ApplySpeed();
	}
}

void AWasamiPlayerCharacter::TurnAround()
{
	// InpActEvt_180 Turn: the control rotation straight behind and level at once; the spring arm's rotation lag turns
	// the view.
	if (AController* C = GetController())
	{
		C->SetControlRotation(FRotator(0.f, C->GetControlRotation().Yaw + 180.f, 0.f));
	}
}

void AWasamiPlayerCharacter::UseBoost()
{
	if (BoostCooldownLeft > 0.f)
	{
		return;
	}
	BoostTimeLeft = BoostDuration;
	BoostCooldownLeft = BoostDuration + BoostCooldown;
	ApplySpeed();
}

void AWasamiPlayerCharacter::ApplySpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = IsBoosting() ? BoostSpeed : (IsSprintOn() ? SprintingSpeed : WalkingSpeed);
}

void AWasamiPlayerCharacter::UpdateFOV()
{
	const float Target = static_cast<float>(FMath::GetMappedRangeValueClamped(FOVSpeedRange, FVector2D(BaseFOV, FastFOV), GetVelocity().Size()));
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, Target, GetWorld()->GetDeltaSeconds(), FOVInterpSpeed));
}

void AWasamiPlayerCharacter::UpdateHeadBob()
{
	// Update Bob: a change of the sprint stops the shake; standing still stops it; moving starts one walk or run shake.
	const bool bSprint = IsSprintOn();
	if (bSprint != bBobSprint)
	{
		bBobSprint = bSprint;
		StopHeadBob();
	}
	if (GetVelocity().Size() <= BobMinSpeed)
	{
		StopHeadBob();
		return;
	}
	if (bBobStarted)
	{
		return;
	}
	bBobStarted = true;
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const TSubclassOf<UCameraShakeBase> Shake = bSprint ? RunShakeClass : WalkShakeClass;
	if (bHeadBob && Shake && PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraShake(Shake);
	}
}

void AWasamiPlayerCharacter::StopHeadBob()
{
	if (!bBobStarted)
	{
		return;
	}
	bBobStarted = false;
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	for (const TSubclassOf<UCameraShakeBase>& Shake : {WalkShakeClass, RunShakeClass})
	{
		if (Shake)
		{
			PC->PlayerCameraManager->StopAllInstancesOfCameraShake(Shake, false);
		}
	}
}
