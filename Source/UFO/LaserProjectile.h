// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserProjectile.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class ANPCUFOPawn;

/**
 * ALaserProjectile
 *
 * A fast-moving laser bolt fired from the player UFO toward an enemy NPC.
 * - Rendered as a thin stretched capsule emissive mesh
 * - Uses UProjectileMovementComponent for motion (no physics simulation)
 * - Homes toward its assigned Target if one is set
 * - Destroys itself on hit or after LifeSpan seconds
 */
UCLASS()
class UFO_API ALaserProjectile : public AActor
{
	GENERATED_BODY()

public:
	ALaserProjectile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Assign a homing target. The projectile steers toward this actor each frame.
	 * Must be called right after spawning.
	 */
	void SetTarget(ANPCUFOPawn* InTarget);

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* LaserMesh;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* LaserLight;

	UPROPERTY()
	ANPCUFOPawn* Target;

	/** cm/s */
	UPROPERTY(EditDefaultsOnly, Category = "Laser")
	float Speed = 8000.0f;

	/** Degrees/s of homing correction toward Target */
	UPROPERTY(EditDefaultsOnly, Category = "Laser")
	float HomingStrength = 720.0f;

	/** Seconds before auto-destroy if no hit occurs */
	UPROPERTY(EditDefaultsOnly, Category = "Laser")
	float LifeSpan = 4.0f;

	/** Radius (cm) within which the projectile counts as a hit */
	UPROPERTY(EditDefaultsOnly, Category = "Laser")
	float HitRadius = 80.0f;

	UPROPERTY()
	UMaterialInstanceDynamic* LaserMID;

	void CheckHit();
	void ApplyHomingSteering(float DeltaSeconds);
};
