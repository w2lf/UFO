// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkyBoxActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"

ASkyBoxActor::ASkyBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SkyboxRadius  = 50000.0f;
	StarMaterial  = nullptr;

	// Create mesh
	SkyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyMesh"));
	RootComponent = SkyMesh;

	// Load sphere mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		SkyMesh->SetStaticMesh(SphereMesh.Object);
	}

	// FObjectFinder MUST live in the constructor — cache the result for use in BeginPlay
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialFinder(TEXT("/Game/M_Stars"));
	if (MaterialFinder.Succeeded())
	{
		StarMaterial = MaterialFinder.Object;
	}

	SkyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyMesh->CastShadow = false;
	ApplySkyboxScale();
}

void ASkyBoxActor::BeginPlay()
{
	Super::BeginPlay();
	CreateSkyboxMaterial();
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

void ASkyBoxActor::CreateSkyboxMaterial()
{
	if (!SkyMesh) return;

	if (StarMaterial)
	{
		SkyMesh->SetMaterial(0, StarMaterial);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ASkyBoxActor: M_Stars material not found at /Game/M_Stars — check the asset path."));
	}

	SkyMesh->SetRenderInMainPass(true);
}
