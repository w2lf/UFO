// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "UFOPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UEnhancedInputComponent;
class UEnhancedInputSubsystems;
class UInputMappingContext;
class UInputAction;
struct FMinimalViewInfo;

/**
 * UFO Pawn with trackball-based camera and rotation controls
 * Left trackball: camera rotation
 * Right trackball: UFO rotation
 * Uses quaternions to avoid gimbal lock
 */
UCLASS()
class UFO_API AUFOPawn : public APawn
{
	GENERATED_BODY()

public:
	AUFOPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// UFO mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	UStaticMeshComponent* UFOMesh;

	// Upper dome mesh to make silhouette look like a classic UFO.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	UStaticMeshComponent* UFODomeMesh;

	// Front arrow marker to indicate ship heading.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	UStaticMeshComponent* UFOFrontArrowMesh;

	// Spring arm for camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	// Enhanced Input System
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LeftTrackballAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* RightTrackballAction;

	// Trackball sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float CameraTrackballSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float ShipTrackballSensitivity;

	// Edge behavior: blend from rotate to tilt as thumb approaches trackball ring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Trackball")
	float TrackballEdgeTiltStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Trackball")
	float TrackballEdgeTiltStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Trackball")
	float TrackballEdgeRollStrength;

	// Movement settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxForwardSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float ThrottleNormalized;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float TargetThrottleNormalized;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ThrottleAccelerationRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ThrottleDecelerationRate;

	// Current quaternion rotations
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FQuat CameraRotation;

	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FQuat ShipRotation;

	// Accumulated tilt/roll from circular edge gestures.
	float CameraRollAngle;
	float ShipRollAngle;

	// Trackball positions
	FVector2D LeftTrackballCenter;
	FVector2D RightTrackballCenter;
	FVector2D LeftTrackballPosition;
	FVector2D RightTrackballPosition;
	float TrackballRadius;

	// Input callbacks
	void OnLeftTrackball(const FInputActionValue& Value);
	void OnRightTrackball(const FInputActionValue& Value);
	
	// Trackball helper functions
	FVector GetTrackballVector(FVector2D TrackballPos, FVector2D TrackballCenter) const;
	FQuat GetTrackballRotation(FVector2D OldPos, FVector2D NewPos, FVector2D Center, float Sensitivity) const;

public:
	// UI drag callbacks (screen-space)
	void ApplyLeftTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos);
	void ApplyRightTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos);

	// Getters
	UFUNCTION(BlueprintCallable, Category = "UFO")
	FQuat GetCameraRotation() const { return CameraRotation; }

	UFUNCTION(BlueprintCallable, Category = "UFO")
	FQuat GetShipRotation() const { return ShipRotation; }

	UFUNCTION(BlueprintCallable, Category = "UFO")
	FVector GetUFOLaunchDirection() const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetThrottleNormalized(float InThrottle);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetThrottleNormalized() const { return ThrottleNormalized; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetTargetThrottleNormalized() const { return TargetThrottleNormalized; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetCurrentForwardSpeed() const { return MaxForwardSpeed * ThrottleNormalized; }
};
