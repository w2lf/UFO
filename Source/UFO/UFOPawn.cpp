// Copyright Epic Games, Inc. All Rights Reserved.

#include "UFOPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Camera/CameraTypes.h"

namespace
{
	float GetDragSignFromUpVector(const FVector& UpVector)
	{
		const float UpDot = FVector::DotProduct(UpVector, FVector::UpVector);
		return UpDot < 0.0f ? -1.0f : 1.0f;
	}
}

AUFOPawn::AUFOPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;

	// Don't rotate character with camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create UFO mesh
	UFOMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOMesh"));
	UFODomeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFODomeMesh"));
	UFOFrontArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOFrontArrowMesh"));
	if (UFOMesh)
	{
		UFOMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		UFOMesh->SetCollisionObjectType(ECC_WorldDynamic);
		UFOMesh->SetMobility(EComponentMobility::Movable);
		RootComponent = UFOMesh;

		// Build a simple UFO silhouette using engine primitives (saucer + dome + heading arrow).
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

		if (CylinderMesh.Succeeded())
		{
			UFOMesh->SetStaticMesh(CylinderMesh.Object);
			UFOMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 0.22f));
			UE_LOG(LogTemp, Warning, TEXT("UFOPawn: UFO saucer body mesh loaded"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UFOPawn: Failed to load cylinder mesh, using wireframe fallback"));
		}

		if (UFODomeMesh)
		{
			UFODomeMesh->SetupAttachment(UFOMesh);
			UFODomeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UFODomeMesh->SetMobility(EComponentMobility::Movable);
			UFODomeMesh->SetVisibility(true);
			if (SphereMesh.Succeeded())
			{
				UFODomeMesh->SetStaticMesh(SphereMesh.Object);
				UFODomeMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
				UFODomeMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.35f));
			}
		}

		if (UFOFrontArrowMesh)
		{
			UFOFrontArrowMesh->SetupAttachment(UFOMesh);
			UFOFrontArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UFOFrontArrowMesh->SetMobility(EComponentMobility::Movable);
			UFOFrontArrowMesh->SetVisibility(true);
			if (ConeMesh.Succeeded())
			{
				UFOFrontArrowMesh->SetStaticMesh(ConeMesh.Object);
				UFOFrontArrowMesh->SetRelativeLocation(FVector(220.0f, 0.0f, 20.0f));
				UFOFrontArrowMesh->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
				UFOFrontArrowMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.35f));
			}
		}
		
		// Make sure the mesh is visible even if empty
		UFOMesh->SetVisibility(true);
		#if WITH_EDITORONLY_DATA
		UFOMesh->bVisualizeComponent = true;
		if (UFODomeMesh)
		{
			UFODomeMesh->bVisualizeComponent = true;
		}
		if (UFOFrontArrowMesh)
		{
			UFOFrontArrowMesh->bVisualizeComponent = true;
		}
		#endif
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UFOPawn: Failed to create UFO mesh component!"));
		// Create a dummy root component to prevent crashes
		USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DummyRoot"));
		if (DummyRoot)
		{
			RootComponent = DummyRoot;
			UE_LOG(LogTemp, Warning, TEXT("UFOPawn: Created dummy scene component as fallback root"));
		}
	}

	// Create camera boom (RootComponent is guaranteed to exist at this point)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	if (CameraBoom && RootComponent)
	{
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->TargetArmLength = 900.0f;
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));
		// Keep spring-arm orientation fully independent of parent ship rotation.
		CameraBoom->SetUsingAbsoluteRotation(true);
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritYaw = false;
		CameraBoom->bInheritRoll = false;
		CameraBoom->bEnableCameraLag = false;
		CameraBoom->bEnableCameraRotationLag = false;
		CameraBoom->bDoCollisionTest = false;
	}

	// Create camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	if (Camera && CameraBoom)
	{
		Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
		Camera->bUsePawnControlRotation = false;
		Camera->PostProcessBlendWeight = 1.0f;
		Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
		Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
		Camera->PostProcessSettings.AutoExposureBias = 14.0f;
	}

	// Start camera with a slight top-down tilt so the ship sits ahead in frame.
	CameraRotation = FRotator(-25.0f, 0.0f, 0.0f).Quaternion();
	ShipRotation = FQuat::Identity;
	CameraRollAngle = 0.0f;
	ShipRollAngle = 0.0f;

	// Input sensitivities
	CameraTrackballSensitivity = 1.0f;
	ShipTrackballSensitivity = 1.0f;
	TrackballEdgeTiltStart = 0.72f;
	TrackballEdgeTiltStrength = 1.0f;
	TrackballEdgeRollStrength = 1.0f;
	MaxForwardSpeed = 3000.0f;
	ThrottleNormalized = 0.0f;
	TargetThrottleNormalized = 0.0f;
	ThrottleAccelerationRate = 1.2f;
	ThrottleDecelerationRate = 2.0f;

	// Trackball setup
	TrackballRadius = 100.0f; // Virtual trackball radius
}

void AUFOPawn::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("UFOPawn::BeginPlay called"));

	// If spawned too close to origin (where sun is), move away
	FVector CurrentLocation = GetActorLocation();
	float DistanceFromOrigin = CurrentLocation.Length();
	
	UE_LOG(LogTemp, Warning, TEXT("UFOPawn spawn distance from origin: %.2f"), DistanceFromOrigin);
	
	if (DistanceFromOrigin < 2000.0f)  // If within 2000 units of origin
	{
		// Move ship 2000 units forward and 500 units up from origin
		FVector SafeLocation = FVector(2000.0f, 0.0f, 500.0f);
		SetActorLocation(SafeLocation);
		UE_LOG(LogTemp, Warning, TEXT("UFOPawn moved to safe location: %.2f, %.2f, %.2f"), SafeLocation.X, SafeLocation.Y, SafeLocation.Z);
	}

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			UE_LOG(LogTemp, Warning, TEXT("UFOPawn: Input mapping context added"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UFOPawn: No input subsystem available yet"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UFOPawn: Not yet possessed by controller"));
	}

	// Get viewport size for trackball positioning
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		// Left trackball at bottom-left (30% from left, 70% from top on viewport)
		LeftTrackballCenter = FVector2D(ViewportSize.X * 0.2f, ViewportSize.Y * 0.8f);

		// Right trackball at bottom-right
		RightTrackballCenter = FVector2D(ViewportSize.X * 0.8f, ViewportSize.Y * 0.8f);
	}

	LeftTrackballPosition = LeftTrackballCenter;
	RightTrackballPosition = RightTrackballCenter;
}

void AUFOPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float TiltDecayRate = 2.5f;
	CameraRollAngle = FMath::FInterpConstantTo(CameraRollAngle, 0.0f, DeltaTime, TiltDecayRate);
	ShipRollAngle = FMath::FInterpConstantTo(ShipRollAngle, 0.0f, DeltaTime, TiltDecayRate);

	const FQuat ShipRollQuat(ShipRotation.GetForwardVector(), ShipRollAngle);
	SetActorRotation(ShipRollQuat * ShipRotation, ETeleportType::None);

	// Smooth throttle changes so acceleration/deceleration is not instant.
	if (!FMath::IsNearlyEqual(ThrottleNormalized, TargetThrottleNormalized, KINDA_SMALL_NUMBER))
	{
		const bool bAccelerating = TargetThrottleNormalized > ThrottleNormalized;
		const float InterpRate = bAccelerating ? ThrottleAccelerationRate : ThrottleDecelerationRate;
		ThrottleNormalized = FMath::FInterpConstantTo(ThrottleNormalized, TargetThrottleNormalized, DeltaTime, InterpRate);
	}

	// Move ship forward using frame-rate-independent movement.
	if (ThrottleNormalized > KINDA_SMALL_NUMBER)
	{
		const FVector ForwardDirection = ShipRotation.GetForwardVector();
		const FVector DeltaMove = ForwardDirection * (MaxForwardSpeed * ThrottleNormalized * DeltaTime);
		AddActorWorldOffset(DeltaMove, true);
	}

}

void AUFOPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);

	const float ArmLength = CameraBoom ? CameraBoom->TargetArmLength : 900.0f;
	const float PivotHeight = CameraBoom ? CameraBoom->GetRelativeLocation().Z : 70.0f;

	const FVector PivotLocation = GetActorLocation() + FVector(0.0f, 0.0f, PivotHeight);
	const FQuat CameraRollQuat(CameraRotation.GetForwardVector(), CameraRollAngle);
	const FQuat FinalCameraRot = CameraRollQuat * CameraRotation;
	const FVector CameraForward = FinalCameraRot.GetForwardVector();

	OutResult.Location = PivotLocation - (CameraForward * ArmLength);
	OutResult.Rotation = FinalCameraRot.Rotator();

	if (Camera)
	{
		OutResult.FOV = Camera->FieldOfView;
	}
}

void AUFOPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = 
		Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Left trackball - camera control
		if (LeftTrackballAction)
		{
			EnhancedInputComponent->BindAction(LeftTrackballAction, ETriggerEvent::Triggered,
				this, &AUFOPawn::OnLeftTrackball);
		}

		// Right trackball - ship rotation
		if (RightTrackballAction)
		{
			EnhancedInputComponent->BindAction(RightTrackballAction, ETriggerEvent::Triggered,
				this, &AUFOPawn::OnRightTrackball);
		}
	}
}

void AUFOPawn::OnLeftTrackball(const FInputActionValue& Value)
{
	const FVector2D TrackballInput = Value.Get<FVector2D>();
	if (TrackballInput.IsNearlyZero())
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : (1.0f / 60.0f);
	const float RotationScale = 3.0f * CameraTrackballSensitivity * DeltaSeconds;
	const FQuat CameraRollQuat(CameraRotation.GetForwardVector(), CameraRollAngle);
	const FQuat EffectiveCameraRot = CameraRollQuat * CameraRotation;
	const float DragSign = GetDragSignFromUpVector(EffectiveCameraRot.GetUpVector());
	const float RadialNorm = FMath::Clamp(TrackballInput.Size(), 0.0f, 1.0f);
	const float EdgeAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(TrackballEdgeTiltStart, 1.0f),
		FVector2D(0.0f, 1.0f),
		RadialNorm
	);

	const FVector2D PrevNormPosL = (LeftTrackballPosition - LeftTrackballCenter) / FMath::Max(1.0f, TrackballRadius);
	const FVector2D PositionDeltaL = TrackballInput - PrevNormPosL;
	float TangentialAmount = 0.0f;
	if (TrackballInput.Size() > SMALL_NUMBER)
	{
		const FVector2D RadialDir = TrackballInput.GetSafeNormal();
		const FVector2D TangentDir(-RadialDir.Y, RadialDir.X);
		TangentialAmount = FVector2D::DotProduct(PositionDeltaL, TangentDir);
	}

	const FQuat YawDelta(FVector::UpVector, -(TrackballInput.X * DragSign) * RotationScale);
	const FVector CameraRightAxis = CameraRotation.GetRightVector();
	const FQuat PitchDelta(CameraRightAxis, -TrackballInput.Y * RotationScale);

	CameraRollAngle += TangentialAmount * RotationScale * EdgeAlpha * TrackballEdgeRollStrength;

	CameraRotation = PitchDelta * YawDelta * CameraRotation;
	CameraRotation.Normalize();

	LeftTrackballPosition = LeftTrackballCenter + TrackballInput * TrackballRadius;
}

void AUFOPawn::OnRightTrackball(const FInputActionValue& Value)
{
	const FVector2D TrackballInput = Value.Get<FVector2D>();
	if (TrackballInput.IsNearlyZero())
	{
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : (1.0f / 60.0f);
	const float RotationScale = 3.0f * ShipTrackballSensitivity * DeltaSeconds;
	const FQuat ShipRollQuat(ShipRotation.GetForwardVector(), ShipRollAngle);
	const FQuat EffectiveShipRot = ShipRollQuat * ShipRotation;
	const float DragSign = GetDragSignFromUpVector(EffectiveShipRot.GetUpVector());
	const float RadialNorm = FMath::Clamp(TrackballInput.Size(), 0.0f, 1.0f);
	const float EdgeAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(TrackballEdgeTiltStart, 1.0f),
		FVector2D(0.0f, 1.0f),
		RadialNorm
	);

	const FVector2D PrevNormPosR = (RightTrackballPosition - RightTrackballCenter) / FMath::Max(1.0f, TrackballRadius);
	const FVector2D PositionDeltaR = TrackballInput - PrevNormPosR;
	float TangentialAmount = 0.0f;
	if (TrackballInput.Size() > SMALL_NUMBER)
	{
		const FVector2D RadialDir = TrackballInput.GetSafeNormal();
		const FVector2D TangentDir(-RadialDir.Y, RadialDir.X);
		TangentialAmount = FVector2D::DotProduct(PositionDeltaR, TangentDir);
	}

	const FQuat YawDelta(FVector::UpVector, -(TrackballInput.X * DragSign) * RotationScale);
	const FVector ShipRightAxis = ShipRotation.GetRightVector();
	const FQuat PitchDelta(ShipRightAxis, -TrackballInput.Y * RotationScale);

	ShipRollAngle += TangentialAmount * RotationScale * EdgeAlpha * TrackballEdgeRollStrength;

	ShipRotation = PitchDelta * YawDelta * ShipRotation;
	ShipRotation.Normalize();

	RightTrackballPosition = RightTrackballCenter + TrackballInput * TrackballRadius;
}

void AUFOPawn::ApplyLeftTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos)
{
	const FVector2D Delta = CurrentPos - PreviousPos;
	const float RotationScale = 0.01f * CameraTrackballSensitivity;
	const FQuat CameraRollQuat(CameraRotation.GetForwardVector(), CameraRollAngle);
	const FQuat EffectiveCameraRot = CameraRollQuat * CameraRotation;
	const float DragSign = GetDragSignFromUpVector(EffectiveCameraRot.GetUpVector());
	const FVector2D OffsetFromCenter = CurrentPos - LeftTrackballCenter;
	const float RadialNorm = FMath::Clamp(OffsetFromCenter.Size() / FMath::Max(1.0f, TrackballRadius), 0.0f, 1.0f);
	const float EdgeAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(TrackballEdgeTiltStart, 1.0f),
		FVector2D(0.0f, 1.0f),
		RadialNorm
	);

	float TangentialAmount = 0.0f;
	if (OffsetFromCenter.Size() > SMALL_NUMBER)
	{
		const FVector2D RadialDir = OffsetFromCenter.GetSafeNormal();
		const FVector2D TangentDir(-RadialDir.Y, RadialDir.X);
		TangentialAmount = FVector2D::DotProduct(Delta, TangentDir);
	}

	const FQuat YawDelta(FVector::UpVector, -(Delta.X * DragSign) * RotationScale);
	const FVector CameraRightAxis = CameraRotation.GetRightVector();
	const FQuat PitchDelta(CameraRightAxis, -Delta.Y * RotationScale);

	CameraRollAngle += TangentialAmount * RotationScale * EdgeAlpha * TrackballEdgeRollStrength;

	CameraRotation = PitchDelta * YawDelta * CameraRotation;
	CameraRotation.Normalize();
	LeftTrackballPosition = CurrentPos;
}

void AUFOPawn::ApplyRightTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos)
{
	const FVector2D Delta = CurrentPos - PreviousPos;
	const float RotationScale = 0.01f * ShipTrackballSensitivity;
	const FQuat ShipRollQuat(ShipRotation.GetForwardVector(), ShipRollAngle);
	const FQuat EffectiveShipRot = ShipRollQuat * ShipRotation;
	const float DragSign = GetDragSignFromUpVector(EffectiveShipRot.GetUpVector());
	const FVector2D OffsetFromCenter = CurrentPos - RightTrackballCenter;
	const float RadialNorm = FMath::Clamp(OffsetFromCenter.Size() / FMath::Max(1.0f, TrackballRadius), 0.0f, 1.0f);
	const float EdgeAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(TrackballEdgeTiltStart, 1.0f),
		FVector2D(0.0f, 1.0f),
		RadialNorm
	);

	float TangentialAmount = 0.0f;
	if (OffsetFromCenter.Size() > SMALL_NUMBER)
	{
		const FVector2D RadialDir = OffsetFromCenter.GetSafeNormal();
		const FVector2D TangentDir(-RadialDir.Y, RadialDir.X);
		TangentialAmount = FVector2D::DotProduct(Delta, TangentDir);
	}

	const FQuat YawDelta(FVector::UpVector, -(Delta.X * DragSign) * RotationScale);
	const FVector ShipRightAxis = ShipRotation.GetRightVector();
	const FQuat PitchDelta(ShipRightAxis, -Delta.Y * RotationScale);

	ShipRollAngle += TangentialAmount * RotationScale * EdgeAlpha * TrackballEdgeRollStrength;

	ShipRotation = PitchDelta * YawDelta * ShipRotation;
	ShipRotation.Normalize();
	RightTrackballPosition = CurrentPos;
}

FVector AUFOPawn::GetTrackballVector(FVector2D TrackballPos, FVector2D TrackballCenter) const
{
	FVector2D Offset = TrackballPos - TrackballCenter;
	
	// Clamp to trackball radius
	float DistanceSquared = Offset.SizeSquared();
	float RadiusSquared = TrackballRadius * TrackballRadius;

	FVector Result;
	if (DistanceSquared <= RadiusSquared)
	{
		// Inside sphere
		Result = FVector(Offset.X, Offset.Y, FMath::Sqrt(FMath::Max(0.0f, RadiusSquared - DistanceSquared)));
	}
	else
	{
		// On edge of sphere
		Result = FVector(Offset.X, Offset.Y, 0.0f);
	}

	Result.Normalize();
	return Result;
}

FQuat AUFOPawn::GetTrackballRotation(FVector2D OldPos, FVector2D NewPos, FVector2D Center, float Sensitivity) const
{
	FVector OldVector = GetTrackballVector(OldPos, Center);
	FVector NewVector = GetTrackballVector(NewPos, Center);

	// Calculate axis and angle using cross product
	FVector Axis = FVector::CrossProduct(OldVector, NewVector);
	
	if (Axis.SizeSquared() < 1e-6f)
	{
		// No rotation
		return FQuat::Identity;
	}

	Axis.Normalize();

	// Calculate angle using dot product
	float Angle = FMath::Acos(FMath::Clamp(FVector::DotProduct(OldVector, NewVector), -1.0f, 1.0f));

	// Create quaternion from axis-angle
	FQuat ResultQuat = FQuat(Axis, Angle * Sensitivity);
	ResultQuat.Normalize();

	return ResultQuat;
}

FVector AUFOPawn::GetUFOLaunchDirection() const
{
	// Returns the forward direction of the UFO based on its rotation
	return ShipRotation.GetForwardVector();
}

void AUFOPawn::SetThrottleNormalized(float InThrottle)
{
	TargetThrottleNormalized = FMath::Clamp(InThrottle, 0.0f, 1.0f);
}
