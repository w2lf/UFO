// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkyBoxActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"

ASkyBoxActor::ASkyBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SkyboxRadius = 50000.0f;

	// Create mesh component
	SkyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyMesh"));
	RootComponent = SkyMesh;

	// Use the engine's built-in sky sphere mesh (same one BP_Sky_Sphere uses)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkySphere(TEXT("/Engine/EngineSky/SM_SkySphere"));
	if (SkySphere.Succeeded())
	{
		SkyMesh->SetStaticMesh(SkySphere.Object);
	}

	// Apply M_Star material from the Content folder
	static ConstructorHelpers::FObjectFinder<UMaterial> StarMaterial(TEXT("/Game/M_Star"));
	if (StarMaterial.Succeeded())
	{
		SkyMesh->SetMaterial(0, StarMaterial.Object);
	}

	// No collision, no shadow
	SkyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyMesh->CastShadow = false;

	ApplySkyboxScale();
}

void ASkyBoxActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASkyBoxActor::SetSkyboxRadius(float InRadius)
{
	SkyboxRadius = FMath::Max(1000.0f, InRadius);
	ApplySkyboxScale();
}

void ASkyBoxActor::ApplySkyboxScale()
{
	if (SkyMesh)
	{
		SkyMesh->SetRelativeScale3D(FVector(SkyboxRadius / 50.0f));
	}
}
