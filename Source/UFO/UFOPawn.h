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
 * AUFOPawn
 *
 * The player-controlled UFO saucer.
 * - Left trackball  : rotates the camera around the ship
 * - Right trackball : rotates the ship itself
 * All rotations use quaternions to avoid gimbal lock.
 */
UCLASS()
class UFO_API AUFOPawn : public APawn
{
	GENERATED_BODY()

	// -----------------------------------------------------------------------
	// Lifecycle
	// -----------------------------------------------------------------------
public:
	AUFOPawn();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// -----------------------------------------------------------------------
	// Components
	// -----------------------------------------------------------------------
protected:
	/** Flat saucer body (cylinder mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Mesh")
	UStaticMeshComponent* UFOMesh;

	/** Glass dome on top of the saucer */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Mesh")
	UStaticMeshComponent* UFODomeMesh;

	/** Small cone arrow showing the ship's forward direction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Mesh")
	UStaticMeshComponent* UFOFrontArrowMesh;

	/** Spring arm that positions the camera behind/above the ship */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	/** Main game camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	// -----------------------------------------------------------------------
	// Input assets (assign in Blueprint)
	// -----------------------------------------------------------------------
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LeftTrackballAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* RightTrackballAction;

	// -----------------------------------------------------------------------
	// Tuning
	// -----------------------------------------------------------------------
protected:
	/** Scales how fast touch drags rotate the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Sensitivity")
	float CameraTrackballSensitivity;

	/** Scales how fast touch drags rotate the ship */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Sensitivity")
	float ShipTrackballSensitivity;

	/** Normalised radius at which edge-roll effect begins (0–1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Trackball")
	float TrackballEdgeTiltStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Trackball")
	float TrackballEdgeTiltStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Trackball")
	float TrackballEdgeRollStrength;

	/** Top speed when throttle is at 1.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxForwardSpeed;

	/** Rate at which throttle ramps up (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ThrottleAccelerationRate;

	/** Rate at which throttle ramps down (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ThrottleDecelerationRate;

	// -----------------------------------------------------------------------
	// Runtime state
	// -----------------------------------------------------------------------
protected:
	/** Current smoothed throttle (0–1) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float ThrottleNormalized;

	/** Desired throttle set by UI (0–1) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float TargetThrottleNormalized;

	/** Current camera orientation as a quaternion */
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FQuat CameraRotation;

	/** Current ship orientation as a quaternion */
	UPROPERTY(BlueprintReadOnly, Category = "Rotation")
	FQuat ShipRotation;

	/** Accumulated roll from circular edge gestures */
	float CameraRollAngle;
	float ShipRollAngle;

	/** Screen-space trackball anchor and thumb positions */
	FVector2D LeftTrackballCenter;
	FVector2D RightTrackballCenter;
	FVector2D LeftTrackballPosition;
	FVector2D RightTrackballPosition;

	/** Radius of the virtual trackball sphere in pixels */
	float TrackballRadius;

	// -----------------------------------------------------------------------
	// Private helpers
	// -----------------------------------------------------------------------
private:
	/** Shared trackball drag logic used by both left and right trackballs */
	void ApplyTrackballDrag(
		const FVector2D& PreviousPos,
		const FVector2D& CurrentPos,
		const FVector2D& Center,
		float Sensitivity,
		FQuat& InOutRotation,
		float& InOutRollAngle) const;

	/** Sets up UFO mesh, dome and heading arrow in the constructor */
	void SetupUFOMeshComponents();

	/** Sets up spring arm and camera in the constructor */
	void SetupCameraComponents();

	/** Input callbacks bound via Enhanced Input */
	void OnLeftTrackball(const FInputActionValue& Value);
	void OnRightTrackball(const FInputActionValue& Value);

	/** Projects a 2-D trackball position onto the virtual sphere surface */
	FVector GetTrackballVector(FVector2D TrackballPos, FVector2D TrackballCenter) const;

	/** Computes the rotation quaternion between two trackball positions */
	FQuat GetTrackballRotation(FVector2D OldPos, FVector2D NewPos, FVector2D Center, float Sensitivity) const;

	// -----------------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------------
public:
	/** Called by TrackballUI when the left thumb drags across the screen */
	void ApplyLeftTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos);

	/** Called by TrackballUI when the right thumb drags across the screen */
	void ApplyRightTrackballDrag(const FVector2D& PreviousPos, const FVector2D& CurrentPos);

	UFUNCTION(BlueprintCallable, Category = "UFO")
	FQuat GetCameraRotation() const { return CameraRotation; }

	UFUNCTION(BlueprintCallable, Category = "UFO")
	FQuat GetShipRotation() const { return ShipRotation; }

	/** Returns the world-space forward vector of the ship (used for projectile launch) */
	UFUNCTION(BlueprintCallable, Category = "UFO")
	FVector GetUFOLaunchDirection() const;

	/** Set the desired throttle (0 = stop, 1 = full speed) */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetThrottleNormalized(float InThrottle);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetThrottleNormalized() const { return ThrottleNormalized; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetTargetThrottleNormalized() const { return TargetThrottleNormalized; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetCurrentForwardSpeed() const { return MaxForwardSpeed * ThrottleNormalized; }
};
