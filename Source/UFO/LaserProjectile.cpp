// Copyright Epic Games, Inc. All Rights Reserved.

#include "LaserProjectile.h"
#include "NPCUFOPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ALaserProjectile::ALaserProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	Target  = nullptr;
	LaserMID = nullptr;

	// ---- Mesh ---------------------------------------------------------------
	LaserMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserMesh"));
	RootComponent = LaserMesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(
		TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderAsset.Succeeded())
	{
		LaserMesh->SetStaticMesh(CylinderAsset.Object);
	}

	// Stretched long and thin: X/Y = 4 cm radius, Z = 60 cm length
	LaserMesh->SetRelativeScale3D(FVector(0.04f, 0.04f, 0.6f));
	LaserMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserMesh->CastShadow = false;

	// ---- Point light --------------------------------------------------------
	LaserLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LaserLight"));
	LaserLight->SetupAttachment(LaserMesh);
	LaserLight->Intensity           = 8000.0f;
	LaserLight->LightColor          = FColor(255, 80, 80);
	LaserLight->AttenuationRadius   = 400.0f;
	LaserLight->CastShadows         = false;

	// ---- Projectile movement ------------------------------------------------
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed         = Speed;
	ProjectileMovement->MaxSpeed             = Speed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce        = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ALaserProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Create a bright red/orange emissive dynamic material at runtime
	if (UMaterialInterface* BaseMat = LaserMesh->GetMaterial(0))
	{
		LaserMID = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (LaserMID)
		{
			LaserMID->SetVectorParameterValue(TEXT("BaseColor"),  FLinearColor(1.0f, 0.15f, 0.05f, 1.0f));
			LaserMID->SetScalarParameterValue(TEXT("EmissiveScale"), 30.0f);
			LaserMesh->SetMaterial(0, LaserMID);
		}
	}

	SetLifeSpan(LifeSpan);
}

void ALaserProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsValid(Target))
	{
		ApplyHomingSteering(DeltaSeconds);
		CheckHit();
	}
}

void ALaserProjectile::SetTarget(ANPCUFOPawn* InTarget)
{
	Target = InTarget;
}

void ALaserProjectile::ApplyHomingSteering(float DeltaSeconds)
{
	if (!IsValid(Target) || !ProjectileMovement) return;

	const FVector ToTarget   = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector CurrentDir = ProjectileMovement->Velocity.GetSafeNormal();

	const FVector NewDir = FMath::VInterpNormalRotationTo(
		CurrentDir, ToTarget,
		DeltaSeconds,
		HomingStrength);

	ProjectileMovement->Velocity = NewDir * Speed;
}

void ALaserProjectile::CheckHit()
{
	if (!IsValid(Target)) return;

	const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
	if (DistSq <= HitRadius * HitRadius)
	{
		// Notify the NPC it was hit
		Target->SetEnemyHighlighted(false);
		Target->SetSelected(false);

		// Destroy the NPC and this projectile
		Target->Destroy();
		Destroy();
	}
}
