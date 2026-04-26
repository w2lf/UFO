// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkyBoxActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

ASkyBoxActor::ASkyBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SkyboxRadius = 50000.0f; // Very large sphere

	// Create mesh
	SkyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyMesh"));
	RootComponent = SkyMesh;

	// Load sphere mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		SkyMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Set collision
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
	// Create black space material for skybox
	if (SkyMesh)
	{
		UMaterialInstanceDynamic* SkyMaterial = UMaterialInstanceDynamic::Create(
			UMaterial::GetDefaultMaterial(MD_Surface), this);

		if (SkyMaterial)
		{
			// Set to pure black
			SkyMaterial->SetVectorParameterValue(FName("BaseColor"), FLinearColor::Black);
			SkyMaterial->SetScalarParameterValue(FName("Metallic"), 0.0f);
			SkyMaterial->SetScalarParameterValue(FName("Roughness"), 1.0f);
			SkyMaterial->SetScalarParameterValue(FName("Emissive"), 0.0f);

			SkyMesh->SetMaterial(0, SkyMaterial);
		}

		// Make sure we render from inside the sphere
		SkyMesh->SetRenderInMainPass(true);
	}
}
