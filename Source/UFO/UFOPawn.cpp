// Copyright Epic Games, Inc. All Rights Reserved.

#include "UFOPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Camera/CameraTypes.h"

// ---------------------------------------------------------------------------
// Local constants  (avoids magic numbers scattered through the file)
// ---------------------------------------------------------------------------
namespace UFOPawnConstants
{
	constexpr float CameraBoomLength      = 900.0f;
	constexpr float CameraBoomHeight      = 70.0f;   // Z offset of the pivot
	constexpr float ExposureBias          = 14.0f;

	constexpr float SaucerScaleXY        = 2.2f;
	constexpr float SaucerScaleZ         = 0.22f;
	constexpr float DomeOffsetZ          = 45.0f;
	constexpr float DomeScaleZ           = 0.35f;
	constexpr float ArrowOffsetX         = 220.0f;
	constexpr float ArrowOffsetZ         = 20.0f;
	constexpr float ArrowScaleXY         = 0.18f;
	constexpr float ArrowScaleZ          = 0.35f;

	constexpr float SafeSpawnRadius      = 2000.0f;  // Move ship if closer to origin
	constexpr float TiltDecayRate        = 2.5f;     // Roll returns to zero at this rate
	constexpr float TrackballRotScale    = 3.0f;     // Base rotation scale for input callbacks
	constexpr float TrackballDragScale   = 0.01f;    // Base rotation scale for UI drag callbacks
}

// ---------------------------------------------------------------------------
// Local helpers
// ---------------------------------------------------------------------------
namespace
{
	/** Returns +1 or -1 depending on whether the camera/ship is "right-side up" */
	float GetDragSignFromUpVector(const FVector& UpVector)
	{
		const float UpDot = FVector::DotProduct(UpVector, FVector::UpVector);
		return UpDot < 0.0f ? -1.0f : 1.0f;
	}
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AUFOPawn::AUFOPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	SetupUFOMeshComponents();
	SetupCameraComponents();

	// Initial orientations
	CameraRotation = FRotator(-25.0f, 0.0f, 0.0f).Quaternion();
	ShipRotation   = FQuat::Identity;
	CameraRollAngle = 0.0f;
	ShipRollAngle   = 0.0f;

	// Sensitivities
	CameraTrackballSensitivity = 1.0f;
	ShipTrackballSensitivity   = 1.0f;
	TrackballEdgeTiltStart     = 0.72f;
	TrackballEdgeTiltStrength  = 1.0f;
	TrackballEdgeRollStrength  = 1.0f;

	// Movement
	MaxForwardSpeed          = 3000.0f;
	ThrottleNormalized       = 0.0f;
	TargetThrottleNormalized = 0.0f;
	ThrottleAccelerationRate = 1.2f;
	ThrottleDecelerationRate = 2.0f;

	TrackballRadius = 100.0f;
}

// ---------------------------------------------------------------------------
// Constructor helpers
// ---------------------------------------------------------------------------

void AUFOPawn::SetupUFOMeshComponents()
{
	UFOMesh           = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOMesh"));
	UFODomeMesh       = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFODomeMesh"));
	UFOFrontArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOFrontArrowMesh"));

	if (!UFOMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("UFOPawn: Failed to create UFO mesh component!"));
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
		return;
	}

	// Saucer body
	UFOMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UFOMesh->SetCollisionObjectType(ECC_WorldDynamic);
	UFOMesh->SetMobility(EComponentMobility::Movable);
	UFOMesh->SetVisibility(true);
	RootComponent = UFOMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	if (CylinderMesh.Succeeded())
	{
		UFOMesh->SetStaticMesh(CylinderMesh.Object);
		UFOMesh->SetRelativeScale3D(FVector(
			UFOPawnConstants::SaucerScaleXY,
			UFOPawnConstants::SaucerScaleXY,
			UFOPawnConstants::SaucerScaleZ));
	}

	// Glass dome
	if (UFODomeMesh && SphereMesh.Succeeded())
	{
		UFODomeMesh->SetupAttachment(UFOMesh);
		UFODomeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UFODomeMesh->SetMobility(EComponentMobility::Movable);
		UFODomeMesh->SetStaticMesh(SphereMesh.Object);
		UFODomeMesh->SetRelativeLocation(FVector(0.0f, 0.0f, UFOPawnConstants::DomeOffsetZ));
		UFODomeMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, UFOPawnConstants::DomeScaleZ));
	}

	// Heading arrow
	if (UFOFrontArrowMesh && ConeMesh.Succeeded())
	{
		UFOFrontArrowMesh->SetupAttachment(UFOMesh);
		UFOFrontArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UFOFrontArrowMesh->SetMobility(EComponentMobility::Movable);
		UFOFrontArrowMesh->SetStaticMesh(ConeMesh.Object);
		UFOFrontArrowMesh->SetRelativeLocation(FVector(
			UFOPawnConstants::ArrowOffsetX, 0.0f, UFOPawnConstants::ArrowOffsetZ));
		UFOFrontArrowMesh->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		UFOFrontArrowMesh->SetRelativeScale3D(FVector(
			UFOPawnConstants::ArrowScaleXY,
			UFOPawnConstants::ArrowScaleXY,
			UFOPawnConstants::ArrowScaleZ));
	}
}

void AUFOPawn::SetupCameraComponents()
{
	if (!RootComponent) return;

	// Spring arm — keeps camera at a fixed offset and never inherits ship roll
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	if (CameraBoom)
	{
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->TargetArmLength = UFOPawnConstants::CameraBoomLength;
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, UFOPawnConstants::CameraBoomHeight));
		CameraBoom->SetUsingAbsoluteRotation(true);
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritYaw   = false;
		CameraBoom->bInheritRoll  = false;
		CameraBoom->bEnableCameraLag         = false;
		CameraBoom->bEnableCameraRotationLag = false;
		CameraBoom->bDoCollisionTest         = false;
	}

	// Camera — manual exposure so space looks good
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	if (Camera && CameraBoom)
	{
		Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
		Camera->bUsePawnControlRotation = false;
		Camera->PostProcessBlendWeight  = 1.0f;
		Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
		Camera->PostProcessSettings.AutoExposureMethod           = EAutoExposureMethod::AEM_Manual;
		Camera->PostProcessSettings.bOverride_AutoExposureBias   = true;
		Camera->PostProcessSettings.AutoExposureBias             = UFOPawnConstants::ExposureBias;
	}
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AUFOPawn::BeginPlay()
{
	Super::BeginPlay();

	// Prevent the ship from spawning inside the sun (at origin)
	if (GetActorLocation().Length() < UFOPawnConstants::SafeSpawnRadius)
	{
		const FVector SafeLocation(UFOPawnConstants::SafeSpawnRadius, 0.0f, 500.0f);
		SetActorLocation(SafeLocation);
		UE_LOG(LogTemp, Warning, TEXT("UFOPawn: Moved to safe spawn location %s"), *SafeLocation.ToString());
	}

	// Register enhanced input mapping context
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Position the two virtual trackball anchors in the lower corners of the screen
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		LeftTrackballCenter  = FVector2D(ViewportSize.X * 0.2f, ViewportSize.Y * 0.8f);
		RightTrackballCenter = FVector2D(ViewportSize.X * 0.8f, ViewportSize.Y * 0.8f);
	}

	LeftTrackballPosition  = LeftTrackballCenter;
	RightTrackballPosition = RightTrackballCenter;
}

void AUFOPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Decay any accumulated roll back to zero
	CameraRollAngle = FMath::FInterpConstantTo(CameraRollAngle, 0.0f, DeltaTime, UFOPawnConstants::TiltDecayRate);
	ShipRollAngle   = FMath::FInterpConstantTo(ShipRollAngle,   0.0f, DeltaTime, UFOPawnConstants::TiltDecayRate);

	// Apply ship roll on top of its base rotation
	const FQuat ShipRollQuat(ShipRotation.GetForwardVector(), ShipRollAngle);
	SetActorRotation(ShipRollQuat * ShipRotation, ETeleportType::None);

	// Smooth throttle towards the target value
	if (!FMath::IsNearlyEqual(ThrottleNormalized, TargetThrottleNormalized, KINDA_SMALL_NUMBER))
	{
		const bool  bAccelerating = TargetThrottleNormalized > ThrottleNormalized;
		const float InterpRate    = bAccelerating ? ThrottleAccelerationRate : ThrottleDecelerationRate;
		ThrottleNormalized = FMath::FInterpConstantTo(ThrottleNormalized, TargetThrottleNormalized, DeltaTime, InterpRate);
	}

	// Move the ship forward
	if (ThrottleNormalized > KINDA_SMALL_NUMBER)
	{
		const FVector DeltaMove = ShipRotation.GetForwardVector() * (MaxForwardSpeed * ThrottleNormalized * DeltaTime);
		AddActorWorldOffset(DeltaMove, true);
	}
}

void AUFOPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);

	const float ArmLength   = CameraBoom ? CameraBoom->TargetArmLength            : UFOPawnConstants::CameraBoomLength;
	const float PivotHeight = CameraBoom ? CameraBoom->GetRelativeLocation().Z     : UFOPawnConstants::CameraBoomHeight;

	const FVector PivotLocation  = GetActorLocation() + FVector(0.0f, 0.0f, PivotHeight);
	const FQuat   CameraRollQuat = FQuat(CameraRotation.GetForwardVector(), CameraRollAngle);
	const FQuat   FinalCameraRot = CameraRollQuat * CameraRotation;

	OutResult.Location = PivotLocation - (FinalCameraRot.GetForwardVector() * ArmLength);
	OutResult.Rotation = FinalCameraRot.Rotator();

	if (Camera)
	{
		OutResult.FOV = Camera->FieldOfView;
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void AUFOPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (LeftTrackballAction)
		{
			EIC->BindAction(LeftTrackballAction,  ETriggerEvent::Triggered, this, &AUFOPawn::OnLeftTrackball);
		}
		if (RightTrackballAction)
		{
			EIC->BindAction(RightTrackballAction, ETriggerEvent::Triggered, this, &AUFOPawn::OnRightTrackball);
		}
	}
}

void AUFOPawn::OnLeftTrackball(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.IsNearlyZero()) return;

	const float DeltaSeconds  = GetWorld() ? GetWorld()->GetDeltaSeconds() : (1.0f / 60.0f);
	const float Sensitivity   = UFOPawnConstants::TrackballRotScale * CameraTrackballSensitivity * DeltaSeconds;

	ApplyTrackballDrag(LeftTrackballPosition, LeftTrackballCenter + Input * TrackballRadius,
		LeftTrackballCenter, Sensitivity, CameraRotation, CameraRollAngle);

	LeftTrackballPosition = LeftTrackballCenter + Input * TrackballRadius;
}

void AUFOPawn::OnRightTrackball(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.IsNearlyZero()) return;

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : (1.0f / 60.0f);
	const float Sensitivity  = UFOPawnConstants::TrackballRotScale * ShipTrackballSensitivity * DeltaSeconds;

	ApplyTrackballDrag(RightTrackballPosition, RightTrackballCenter + Input * TrackballRadius,
		RightTrackballCenter, Sensitivity, ShipRotation, ShipRollAngle);

	RightTrackballPosition = RightTrackballCenter + Input * TrackballRadius;
}

void AUFOPawn::ApplyLeftTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos)
{
	const float Sensitivity = UFOPawnConstants::TrackballDragScale * CameraTrackballSensitivity;
	ApplyTrackballDrag(PreviousPos, CurrentPos, LeftTrackballCenter, Sensitivity, CameraRotation, CameraRollAngle);
	LeftTrackballPosition = CurrentPos;
}

void AUFOPawn::ApplyRightTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos)
{
	const float Sensitivity = UFOPawnConstants::TrackballDragScale * ShipTrackballSensitivity;
	ApplyTrackballDrag(PreviousPos, CurrentPos, RightTrackballCenter, Sensitivity, ShipRotation, ShipRollAngle);
	RightTrackballPosition = CurrentPos;
}

// ---------------------------------------------------------------------------
// Shared trackball math  (replaces the four near-identical blocks above)
// ---------------------------------------------------------------------------

void AUFOPawn::ApplyTrackballDrag(
	const FVector2D& PreviousPos,
	const FVector2D& CurrentPos,
	const FVector2D& Center,
	float            Sensitivity,
	FQuat&           InOutRotation,
	float&           InOutRollAngle) const
{
	const FVector2D Delta          = CurrentPos - PreviousPos;
	const FVector2D OffsetFromCenter = CurrentPos - Center;

	// How far from center (0=center, 1=edge)
	const float RadialNorm = FMath::Clamp(
		OffsetFromCenter.Size() / FMath::Max(1.0f, TrackballRadius), 0.0f, 1.0f);

	// Edge-roll alpha: zero near center, ramps to 1 near the edge
	const float EdgeAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(TrackballEdgeTiltStart, 1.0f),
		FVector2D(0.0f, 1.0f),
		RadialNorm);

	// Tangential component of the delta (controls roll)
	float TangentialAmount = 0.0f;
	if (OffsetFromCenter.Size() > SMALL_NUMBER)
	{
		const FVector2D RadialDir  = OffsetFromCenter.GetSafeNormal();
		const FVector2D TangentDir = FVector2D(-RadialDir.Y, RadialDir.X);
		TangentialAmount = FVector2D::DotProduct(Delta, TangentDir);
	}

	// Flip yaw direction if rotation is upside-down
	const FQuat   RollQuat   = FQuat(InOutRotation.GetForwardVector(), InOutRollAngle);
	const FQuat   EffRot     = RollQuat * InOutRotation;
	const float   DragSign   = GetDragSignFromUpVector(EffRot.GetUpVector());

	const FQuat YawDelta   = FQuat(FVector::UpVector,                -(Delta.X * DragSign) * Sensitivity);
	const FQuat PitchDelta = FQuat(InOutRotation.GetRightVector(),   -Delta.Y              * Sensitivity);

	InOutRollAngle += TangentialAmount * Sensitivity * EdgeAlpha * TrackballEdgeRollStrength;
	InOutRotation   = (PitchDelta * YawDelta * InOutRotation).GetNormalized();
}

// ---------------------------------------------------------------------------
// Trackball geometry helpers
// ---------------------------------------------------------------------------

FVector AUFOPawn::GetTrackballVector(FVector2D TrackballPos, FVector2D TrackballCenter) const
{
	const FVector2D Offset       = TrackballPos - TrackballCenter;
	const float     RadiusSq     = TrackballRadius * TrackballRadius;
	const float     DistanceSq   = Offset.SizeSquared();

	FVector Result;
	if (DistanceSq <= RadiusSq)
	{
		// Point is inside the sphere — compute the Z component
		Result = FVector(Offset.X, Offset.Y, FMath::Sqrt(FMath::Max(0.0f, RadiusSq - DistanceSq)));
	}
	else
	{
		// Point is outside — project onto the equator
		Result = FVector(Offset.X, Offset.Y, 0.0f);
	}

	return Result.GetSafeNormal();
}

FQuat AUFOPawn::GetTrackballRotation(FVector2D OldPos, FVector2D NewPos, FVector2D Center, float Sensitivity) const
{
	const FVector OldVec = GetTrackballVector(OldPos, Center);
	const FVector NewVec = GetTrackballVector(NewPos, Center);

	FVector Axis = FVector::CrossProduct(OldVec, NewVec);
	if (Axis.SizeSquared() < 1e-6f) return FQuat::Identity;

	Axis.Normalize();
	const float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(OldVec, NewVec), -1.0f, 1.0f));
	return FQuat(Axis, Angle * Sensitivity).GetNormalized();
}

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------

FVector AUFOPawn::GetUFOLaunchDirection() const
{
	return ShipRotation.GetForwardVector();
}

void AUFOPawn::SetThrottleNormalized(float InThrottle)
{
	TargetThrottleNormalized = FMath::Clamp(InThrottle, 0.0f, 1.0f);
}
