#include "WasamiPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiArrowPointer.h"
#include "WasamiAssets.h"
#include "WasamiBonusShard.h"
#include "WasamiChameleonComponent.h"
#include "WasamiDeathScreenWidget.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiInteractWidget.h"
#include "WasamiInteractable.h"
#include "WasamiPauseWidget.h"
#include "WasamiPowerComponent.h"
#include "WasamiPowerOrb.h"
#include "WasamiSettingsSaveGame.h"
#include "WasamiShard.h"
#include "WasamiTabletWidget.h"

namespace
{
	// The original's mouse axes: UE4's mouse smoothing, then UE4's FOV scaling (FOVScale 0.01111). Their sensitivity
	// 0.07 is the Mouse2D AxisConfig in Config/DefaultInput.ini, which Enhanced Input adds to every mouse mapping by
	// itself (ApplyAxisPropertyModifiers), so the mapping must not scale it again. AddControllerYaw/PitchInput scale by
	// the controller's legacy 2.5 / −2.5 (bEnableLegacyInputScales in the same file).
	constexpr float MouseFOVScale = 0.01111f;

	// The original's FOV Multiplier runs on a 0.001 s looping timer: several times a frame, each with the frame's delta.
	constexpr float FOVTimerRate = 0.001f;

	// Update Bob stops the head bob at or below this speed (cm/s).
	constexpr float BobMinSpeed = 1.f;

	// The tablet, from the original's BP_DD_PlayerCharacter. Its TabletInterp / Timeline_1 move the plate between
	// these two heights in the camera's space and turn it the right way up as it comes; the X and Y stay put.
	constexpr float TabletX = 35.39891f;
	constexpr float TabletY = -21.994417f;
	constexpr float TabletStowedZ = -39.701378f;
	constexpr float TabletRaisedZ = -4.321648f;
	constexpr float TabletRaiseLength = 0.5f;
	constexpr float TabletLowerLength = 0.3f;
	// PlaySound2D's volume and pitch for the two wooshes and for the map's resize.
	constexpr float WooshVolume = 0.5f;
	constexpr float WooshPitch = 1.5f;
	constexpr float ResizeVolume = 0.5f;
	constexpr float ResizePitch = 4.f;
	// The minimap's scene capture: 40 m across, 100 m when the map is zoomed out.
	constexpr float MinimapOrthoWidth = 4000.f;
	constexpr float MinimapZoomedOrthoWidth = 10000.f;
	constexpr float MinimapHeight = 3000.f;
	// How often the screen counts the shards and picks up what the capture should show (the original does it every
	// 0.01 s from the widget's own loop; 0.1 s cannot be told apart on a count that only changes on a collect).
	constexpr float ScreenRefreshRate = 0.1f;
	// The level's map plane carries this tag (the original finds its BP_MapTexture actors by class).
	const FName MinimapTag(TEXT("dd_minimap"));

	void AddCurveKey(FRichCurve& Curve, float Time, float Value, ERichCurveInterpMode Interp, float Arrive, float Leave)
	{
		const FKeyHandle Handle = Curve.AddKey(Time, Value);
		FRichCurveKey& Key = Curve.GetKey(Handle);
		Key.InterpMode = Interp;
		Key.TangentMode = RCTM_User;
		Key.ArriveTangent = Arrive;
		Key.LeaveTangent = Leave;
	}
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

	// Tablet: the original hangs it under an empty Scene at (0, 0, −94.9577) below the camera, but the heights its
	// timelines write (−39.70 → −4.32) are the camera's own — with that Scene in between the plate would sit a metre
	// under the view. Straight on the camera, those heights put it exactly where the original's recording shows it.
	Tablet = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tablet"));
	Tablet->SetupAttachment(Camera);
	Tablet->SetCollisionProfileName(TEXT("NoCollision"));
	Tablet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tablet->bSelfShadowOnly = true;
	Tablet->SetRelativeLocation(FVector(TabletX, TabletY, TabletStowedZ));
	Tablet->SetRelativeRotation(FRotator(0.f, 90.f, 180.f));

	TabletScreen = CreateDefaultSubobject<UWidgetComponent>(TEXT("TabletScreen"));
	TabletScreen->SetupAttachment(Tablet);
	TabletScreen->SetRelativeLocation(FVector(0.f, 0.8922737f, 0.2078171f));
	TabletScreen->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	TabletScreen->SetRelativeScale3D(FVector(0.28f, 0.024465779f, 0.024465779f));
	TabletScreen->SetWidgetSpace(EWidgetSpace::World);
	TabletScreen->SetWidgetClass(UWasamiTabletWidget::StaticClass());
	TabletScreen->SetDrawSize(FVector2D(UWasamiTabletWidget::ScreenWidth, UWasamiTabletWidget::ScreenHeight));
	// Masked and one-sided picks Widget3DPassThrough_Masked_OneSided, the material the original overrides with.
	TabletScreen->SetBlendMode(EWidgetBlendMode::Masked);
	TabletScreen->SetTwoSided(false);
	TabletScreen->SetCollisionProfileName(TEXT("NoCollision"));
	TabletScreen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TabletScreen->SetGenerateOverlapEvents(false);

	MinimapCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCapture"));
	MinimapCapture->SetupAttachment(Capsule);
	MinimapCapture->SetRelativeLocation(FVector(0.f, 0.f, MinimapHeight));
	MinimapCapture->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	MinimapCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapCapture->OrthoWidth = MinimapOrthoWidth;
	MinimapCapture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
	MinimapCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// The original captures every frame; here it only runs while the tablet is up, where the map can be seen
	// (.claude/guides/performance.md — what draws every frame has to earn it). The picture is the same either way.
	MinimapCapture->bCaptureEveryFrame = false;

	ArrowPointer = CreateDefaultSubobject<UChildActorComponent>(TEXT("BP_ArrowPointer"));
	ArrowPointer->SetupAttachment(GetMesh());
	ArrowPointer->SetRelativeLocation(AWasamiArrowPointer::PlayerRelativeLocation);
	ArrowPointer->SetRelativeScale3D(AWasamiArrowPointer::PlayerRelativeScale);
	ArrowPointer->SetChildActorClass(AWasamiArrowPointer::StaticClass());

	Powers = CreateDefaultSubobject<UWasamiPowerComponent>(TEXT("Powers"));

	// FX: the original's child actor sits 20 m over the capsule, but its volume is unbound, so where it is changes nothing.
	Chameleon = CreateDefaultSubobject<UWasamiChameleonComponent>(TEXT("FX"));

	// The pipeline's assets, loaded at BeginPlay (WasamiAssets.h says why not here).
	TabletMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/Player/Tablet/tablet_new_pCube2")));
	MinimapTarget = TSoftObjectPtr<UTextureRenderTarget2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Minimap/T_NewMap")));
	ShardActorClass = AWasamiShard::StaticClass();
	MinimapActorClasses = {AWasamiPowerOrb::StaticClass(), AWasamiBonusShard::StaticClass()};
	TabletUpSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/05_Tablet_Woosh_v2_1")));
	TabletDownSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/05_Tablet_Woosh_v1_1")));
	ResizeMapSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
	WalkShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake")));
	RunShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_RunShake")));

	// TabletInterp's CurveFloat_1 and Timeline_1's CurveFloat_1_2, key for key.
	AddCurveKey(TabletRaiseCurve, 0.f, 0.f, RCIM_Cubic, 0.f, 0.f);
	AddCurveKey(TabletRaiseCurve, 0.3f, 0.9f, RCIM_Cubic, 2.f, 2.f);
	AddCurveKey(TabletRaiseCurve, TabletRaiseLength, 1.f, RCIM_Linear, 0.f, 0.f);
	AddCurveKey(TabletLowerCurve, 0.f, 1.f, RCIM_Cubic, -0.46747881f, -0.46748275f);
	AddCurveKey(TabletLowerCurve, 0.2f, 0.1f, RCIM_Cubic, -1.3626982f, -3.1061733f);
	AddCurveKey(TabletLowerCurve, TabletLowerLength, 0.f, RCIM_Linear, 0.f, 0.f);

	GetCharacterMovement()->MaxWalkSpeed = WalkingSpeed;
}

void AWasamiPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	Tablet->SetStaticMesh(TabletMesh.LoadSynchronous());
	MinimapCapture->TextureTarget = MinimapTarget.LoadSynchronous();
	LoadedTabletUpSound = TabletUpSound.LoadSynchronous();
	LoadedTabletDownSound = TabletDownSound.LoadSynchronous();
	LoadedResizeMapSound = ResizeMapSound.LoadSynchronous();
	LoadedWalkShake = WalkShakeClass.LoadSynchronous();
	LoadedRunShake = RunShakeClass.LoadSynchronous();

	// PlaceTablet reads the point of view the camera manager has just worked out, so the tick has to come after it.
	SetTickGroup(ETickingGroup::TG_PostUpdateWork);
	ApplySpeed();
	ApplyTabletInterp(0.f);
	WasamiGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AWasamiGameMode>() : nullptr;
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		if (const UWasamiSettingsSaveGame* Settings = Instance->GetSettings())
		{
			ApplySettings(*Settings);
		}
	}
	UpdateTabletScreen();
	// UMG_Interact, added at 0 and collapsed until the player looks at something to use.
	InteractWidget = CreateWidget<UWasamiInteractWidget>(GetWorld(), UWasamiInteractWidget::StaticClass());
	if (InteractWidget)
	{
		if (GetWorld()->GetGameViewport())
		{
			InteractWidget->AddToViewport(0);
		}
		InteractWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	GetWorldTimerManager().SetTimer(FOVTimer, this, &AWasamiPlayerCharacter::UpdateFOV, FOVTimerRate, true);
	GetWorldTimerManager().SetTimer(TabletScreenTimer, this, &AWasamiPlayerCharacter::UpdateTabletScreen, ScreenRefreshRate, true);
}

void AWasamiPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateInteractWidget();
	UpdateTablet(DeltaSeconds);
	UpdateHeadBob();
}

void AWasamiPlayerCharacter::ApplySettings(const UWasamiSettingsSaveGame& Settings)
{
	bToggleSprint = Settings.bToggleSprint;
	MouseSensitivity = UWasamiSettingsSaveGame::PlayerSensitivityFor(Settings.MouseSensitivity);
	bInvertY = Settings.bInvertedYAxis;
	bHeadBob = Settings.bHeadBobbing;
	ApplySpeed();
}

void AWasamiPlayerCharacter::SetUpMouseSmoothing(const UWasamiSettingsSaveGame& Settings)
{
	SpringArm->CameraRotationLagSpeed = UWasamiSettingsSaveGame::RotationLagSpeedFor(Settings.bMouseSmoothing);
}

void AWasamiPlayerCharacter::SetMoveSpeeds(float Walking, float Sprinting)
{
	WalkingSpeed = Walking;
	SprintingSpeed = Sprinting;
	ApplySpeed();
}

UWasamiTabletWidget* AWasamiPlayerCharacter::GetTabletScreen() const
{
	return Cast<UWasamiTabletWidget>(TabletScreen->GetUserWidgetObject());
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
	Input->BindAction(UsePowerLeftAction, ETriggerEvent::Started, Powers.Get(), &UWasamiPowerComponent::UsePowerLeftPressed);
	Input->BindAction(UsePowerRightAction, ETriggerEvent::Started, Powers.Get(), &UWasamiPowerComponent::UsePowerRightPressed);
	Input->BindAction(CyclePowerLeftAction, ETriggerEvent::Started, Powers.Get(), &UWasamiPowerComponent::CyclePowerLeft);
	Input->BindAction(CyclePowerRightAction, ETriggerEvent::Started, Powers.Get(), &UWasamiPowerComponent::CyclePowerRight);
	Input->BindAction(TabletAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::ToggleTablet);
	Input->BindAction(ResizeMapAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::ResizeMap);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::InteractPressed);
	Input->BindAction(LeftMouseAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::LeftMousePressed);
	Input->BindAction(LeftMouseAction, ETriggerEvent::Completed, this, &AWasamiPlayerCharacter::LeftMouseReleased);
	Input->BindAction(MouseWheelAction, ETriggerEvent::Triggered, this, &AWasamiPlayerCharacter::MouseWheel);
	Input->BindAction(EscapeAction, ETriggerEvent::Started, this, &AWasamiPlayerCharacter::EscapePressed);
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
	UsePowerLeftAction = NewAction(TEXT("IA_UsePowerLeft"), EInputActionValueType::Boolean);
	UsePowerRightAction = NewAction(TEXT("IA_UsePowerRight"), EInputActionValueType::Boolean);
	CyclePowerLeftAction = NewAction(TEXT("IA_CyclePowerLeft"), EInputActionValueType::Boolean);
	CyclePowerRightAction = NewAction(TEXT("IA_CyclePowerRight"), EInputActionValueType::Boolean);
	TabletAction = NewAction(TEXT("IA_ToggleTablet"), EInputActionValueType::Boolean);
	ResizeMapAction = NewAction(TEXT("IA_ResizeMap"), EInputActionValueType::Boolean);
	InteractAction = NewAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	LeftMouseAction = NewAction(TEXT("IA_LeftMouseButton"), EInputActionValueType::Boolean);
	MouseWheelAction = NewAction(TEXT("IA_MouseWheelAxis"), EInputActionValueType::Axis1D);
	EscapeAction = NewAction(TEXT("IA_Escape"), EInputActionValueType::Boolean);
	// EscapePressed asks whether the game is paused itself, as the debug command comes through it too.
	EscapeAction->bTriggerWhenPaused = true;

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

	UInputModifierFOVScaling* FOVScaling = NewObject<UInputModifierFOVScaling>(this);
	FOVScaling->FOVScale = MouseFOVScale;
	FOVScaling->FOVScalingType = EFOVScalingType::UE4_BackCompat;
	Map(LookAction, EKeys::Mouse2D, {NewObject<UInputModifierSmooth>(this), FOVScaling});

	Map(SprintAction, EKeys::LeftShift);
	Map(TurnAction, EKeys::MiddleMouseButton);
	// The original's Use Power Left / Right and Cycle Power Left / Right; its Use Power (R) has no handler anywhere.
	Map(UsePowerLeftAction, EKeys::Q);
	Map(UsePowerRightAction, EKeys::E);
	Map(CyclePowerLeftAction, EKeys::One);
	Map(CyclePowerRightAction, EKeys::Two);
	Map(TabletAction, EKeys::SpaceBar);
	Map(ResizeMapAction, EKeys::Z);
	// The original's Interact (F; like the other actions, its gamepad key is not mapped), pressed.
	Map(InteractAction, EKeys::F);
	// Pressed and released, and the wheel's value (±1 a notch; the original's MouseWheelAxis has sensitivity 1). An Axis1D only
	// triggers on a frame the wheel moves, which is when the original's every-frame binding changes anything.
	Map(LeftMouseAction, EKeys::LeftMouseButton);
	Map(MouseWheelAction, EKeys::MouseWheelAxis);
	// The old version's character takes Esc itself (the latest version's player controller takes it and Gamepad Special
	// Left, which is not mapped, like the other actions' gamepad keys).
	Map(EscapeAction, EKeys::Escape);
}

void AWasamiPlayerCharacter::InteractPressed()
{
	OnInteract.Broadcast();
}

void AWasamiPlayerCharacter::EscapePressed()
{
	// The key's binding does not execute while the game is paused (bExecuteWhenPaused off), but here it does over EASY's
	// death screen with no lives, whose only way out is this menu.
	if (!UGameplayStatics::IsGamePaused(this) || UWasamiDeathScreenWidget::FindHoldingOnEasy(this))
	{
		UWasamiPauseWidget::Show(this);
	}
}

void AWasamiPlayerCharacter::LeftMousePressed()
{
	// The teleport's aim takes the click without consuming it (its input sits above the pawn's), so the player's own
	// Interact (Secondary) runs on the same click.
	Powers->ConfirmTeleport();
	InteractSecondaryPressed();
}

void AWasamiPlayerCharacter::LeftMouseReleased()
{
	InteractSecondaryReleased();
}

bool AWasamiPlayerCharacter::TraceInteract(FHitResult& OutHit) const
{
	// LineTraceSingle(Visibility, not complex, no actors to ignore but the player itself) from the camera.
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InteractDistance;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(WasamiInteract), false, this);
	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}

void AWasamiPlayerCharacter::InteractSecondaryPressed()
{
	if (!bCanInteract)
	{
		return;
	}
	// A held item's Use would come first; this game has none.
	FHitResult Hit;
	const bool bHit = TraceInteract(Hit);
	InteractHitActor = Hit.GetActor();
	AActor* Target = Hit.GetActor();
	if (bHit && Target && Target->Implements<UWasamiInteractable>())
	{
		IWasamiInteractable::Execute_InteractWithObject(Target, this);
	}
}

void AWasamiPlayerCharacter::InteractSecondaryReleased()
{
	AActor* Target = InteractHitActor.Get();
	if (Target && Target->Implements<UWasamiInteractable>())
	{
		IWasamiInteractable::Execute_StopInteractWithObject(Target);
	}
}

void AWasamiPlayerCharacter::UpdateInteractWidget()
{
	if (!bCanInteract || !InteractWidget)
	{
		return;
	}
	FHitResult Hit;
	const bool bHit = TraceInteract(Hit);
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	const bool bShow = bHit && IsValid(Hit.GetActor()) && HitComponent && HitComponent->ComponentHasTag(TEXT("interact"));
	InteractWidget->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void AWasamiPlayerCharacter::MouseWheel(const FInputActionValue& Value)
{
	Powers->AdjustTeleportDistance(Value.Get<float>());
}

void AWasamiPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!bCanMove)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotationMatrix Yaw(FRotator(0.f, GetControlRotation().Yaw, 0.f));
	AddMovementInput(Yaw.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(Yaw.GetUnitAxis(EAxis::Y), Axis.X);
}

void AWasamiPlayerCharacter::Look(const FInputActionValue& Value)
{
	if (!bCanMove)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>() * MouseSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(bInvertY ? Axis.Y : -Axis.Y);
}

void AWasamiPlayerCharacter::SprintPressed()
{
	if (!bCanMove)
	{
		return;
	}
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

void AWasamiPlayerCharacter::StopSprinting()
{
	bSprintHeld = false;
	bSprintLatch = false;
	ApplySpeed();
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

void AWasamiPlayerCharacter::ApplySpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = IsSprintOn() ? SprintingSpeed : WalkingSpeed;
}

void AWasamiPlayerCharacter::ToggleTablet()
{
	// Toggle Tablet: nothing happens while the player cannot move or cannot use it; either way a woosh plays and the
	// matching timeline runs from its start, even if the other one was still going.
	if (!bCanMove || !bCanUseTablet)
	{
		return;
	}
	bTabletUp = !bTabletUp;
	bTabletMoving = true;
	TabletTime = 0.f;
	if (bTabletUp)
	{
		MinimapCapture->bCaptureEveryFrame = true;
	}
	UGameplayStatics::PlaySound2D(this, bTabletUp ? LoadedTabletUpSound : LoadedTabletDownSound, WooshVolume, WooshPitch);
}

void AWasamiPlayerCharacter::PutDownTablet()
{
	// Put Down Tablet (the capture calls it): Toggle Tablet's lowering without its checks.
	if (!bTabletUp)
	{
		return;
	}
	bTabletUp = false;
	bTabletMoving = true;
	TabletTime = 0.f;
	UGameplayStatics::PlaySound2D(this, LoadedTabletDownSound, WooshVolume, WooshPitch);
}

void AWasamiPlayerCharacter::ResizeMap()
{
	// Resize Map: only with the tablet up, and it flips between the two OrthoWidths.
	if (!bTabletUp)
	{
		return;
	}
	UGameplayStatics::PlaySound2D(this, LoadedResizeMapSound, ResizeVolume, ResizePitch);
	bMapZoomedOut = !bMapZoomedOut;
	MinimapCapture->OrthoWidth = bMapZoomedOut ? MinimapZoomedOrthoWidth : MinimapOrthoWidth;
}

void AWasamiPlayerCharacter::ApplyTabletInterp(float Value)
{
	TabletInterp = Value;
	PlaceTablet();
}

void AWasamiPlayerCharacter::PlaceTablet()
{
	// Where the timelines put the plate, in the view's own space.
	const FVector Local(TabletX, TabletY, FMath::Lerp(TabletStowedZ, TabletRaisedZ, TabletInterp));
	// RLerp with the shortest path: the plate swings up the right way round as it rises.
	static const FQuat Stowed = FRotator(0.f, 90.f, 180.f).Quaternion();
	static const FQuat Raised = FRotator(0.f, 90.f, 0.f).Quaternion();
	const FQuat LocalRotation = FQuat::Slerp(Stowed, Raised, TabletInterp).GetNormalized();

	// The tablet has to sit still on the screen while the player walks (the user's call, as on the WebGL version:
	// its 10 記録, 2026-09-12). UE puts a camera shake on the camera manager's point of view, not on the camera
	// component, so a child of the camera would swing the other way on screen — instead the plate is placed on the
	// view that is about to be rendered, every frame, in TG_PostUpdateWork.
	// While the view is another's (the capture's room), the plate stays with this camera, as the original's child of
	// the camera does.
	FTransform View(Camera->GetComponentQuat(), Camera->GetComponentLocation());
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (const APlayerCameraManager* Manager = PC->PlayerCameraManager; Manager && PC->GetViewTarget() == this)
		{
			View = FTransform(Manager->GetCameraRotation().Quaternion(), Manager->GetCameraLocation());
		}
	}
	const FTransform Placed = FTransform(LocalRotation, Local) * View;
	Tablet->SetWorldLocationAndRotation(Placed.GetLocation(), Placed.GetRotation());
}

void AWasamiPlayerCharacter::UpdateTablet(float DeltaSeconds)
{
	PlaceTablet();
	if (bTabletMoving)
	{
		TabletTime += DeltaSeconds;
		const float Length = bTabletUp ? TabletRaiseLength : TabletLowerLength;
		const FRichCurve& Curve = bTabletUp ? TabletRaiseCurve : TabletLowerCurve;
		if (TabletTime >= Length)
		{
			TabletTime = Length;
			bTabletMoving = false;
			if (!bTabletUp)
			{
				MinimapCapture->bCaptureEveryFrame = false;
			}
		}
		ApplyTabletInterp(Curve.Eval(TabletTime));
	}

	if (UWasamiTabletWidget* Screen = GetTabletScreen())
	{
		// UMG_TabletPowers: the sockets show the powers they point at, each icon filled as far as its gauge.
		Screen->SetPowersVisible(Powers->HasPowers());
		Screen->ShowSocketPowers(Powers->GetSocketPower(true), Powers->GetSocketPower(false));
		for (int32 Index = 0; Index < WasamiPowerCount; ++Index)
		{
			const EWasamiPower Power = static_cast<EWasamiPower>(Index);
			Screen->SetPowerPercent(Power, Powers->GetGaugePercent(Power));
		}
		Screen->TickAnimations(DeltaSeconds);
		Screen->SetObjective(WasamiGameMode ? WasamiGameMode->CurrentObjective : FText::GetEmpty());
	}
}

void AWasamiPlayerCharacter::UpdateTabletScreen()
{
	RefreshMinimapContents();
	if (UWasamiTabletWidget* Screen = GetTabletScreen())
	{
		Screen->SetShardCount(ShardsLeft);
	}
}

AWasamiArrowPointer* AWasamiPlayerCharacter::GetArrowPointer() const
{
	return Cast<AWasamiArrowPointer>(ArrowPointer->GetChildActor());
}

void AWasamiPlayerCharacter::AddToMap(TSubclassOf<AActor> ActorClass)
{
	TArray<AActor*> OfClass;
	UGameplayStatics::GetAllActorsOfClass(this, ActorClass, OfClass);
	for (AActor* Actor : OfClass)
	{
		MapAddedActors.AddUnique(Actor);
	}
	RefreshMinimapContents();
}

void AWasamiPlayerCharacter::RemoveFromMap(TSubclassOf<AActor> ActorClass)
{
	TArray<AActor*> OfClass;
	UGameplayStatics::GetAllActorsOfClass(this, ActorClass, OfClass);
	for (AActor* Actor : OfClass)
	{
		MapAddedActors.Remove(Actor);
	}
	RefreshMinimapContents();
}

bool AWasamiPlayerCharacter::IsOnMap(const AActor* Actor) const
{
	return Actor && MinimapCapture->ShowOnlyActors.Contains(Actor);
}

void AWasamiPlayerCharacter::RefreshMinimapContents()
{
	// Show Only: the capture draws the level's map plane, the shards, the arrow, the classes it always shows and what
	// Add To Map added, and nothing else of the world.
	TArray<AActor*> Shown;
	UGameplayStatics::GetAllActorsWithTag(this, MinimapTag, Shown);
	if (AWasamiArrowPointer* Arrow = GetArrowPointer())
	{
		Shown.Add(Arrow);
	}
	for (const TSubclassOf<AActor>& Class : MinimapActorClasses)
	{
		TArray<AActor*> OfClass;
		UGameplayStatics::GetAllActorsOfClass(this, Class, OfClass);
		Shown.Append(OfClass);
	}
	MapAddedActors.RemoveAll([](const TWeakObjectPtr<AActor>& Actor) { return !Actor.IsValid(); });
	for (const TWeakObjectPtr<AActor>& Actor : MapAddedActors)
	{
		Shown.Add(Actor.Get());
	}
	ShardsLeft = 0;
	if (ShardActorClass)
	{
		TArray<AActor*> Shards;
		UGameplayStatics::GetAllActorsOfClass(this, ShardActorClass, Shards);
		ShardsLeft = Shards.Num();
		Shown.Append(Shards);
	}
	MinimapCapture->ShowOnlyActors.Reset(Shown.Num());
	for (AActor* Actor : Shown)
	{
		MinimapCapture->ShowOnlyActors.Add(Actor);
	}
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
	const TSubclassOf<UCameraShakeBase> Shake = bSprint ? LoadedRunShake : LoadedWalkShake;
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
	for (const TSubclassOf<UCameraShakeBase>& Shake : {LoadedWalkShake, LoadedRunShake})
	{
		if (Shake)
		{
			PC->PlayerCameraManager->StopAllInstancesOfCameraShake(Shake, false);
		}
	}
}
