// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlanetActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

APlanetActor::APlanetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PlanetRadius = 180.0f;
	PlanetColor = FLinearColor(0.2f, 0.5f, 0.8f, 1.0f); // Blue
	PlanetType = FString(TEXT("Earth"));

	// Create mesh
	PlanetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlanetMesh"));
	RootComponent = PlanetMesh;

	// Load sphere mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		PlanetMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Set collision
	PlanetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PlanetMesh->SetCollisionObjectType(ECC_WorldStatic);
	PlanetMesh->CastShadow = true;
	PlanetMesh->SetRelativeScale3D(FVector(PlanetRadius / 50.0f)); // Scale to desired radius
}

void APlanetActor::BeginPlay()
{
	Super::BeginPlay();

	CreatePlanetMaterial();
}

void APlanetActor::CreatePlanetMaterial()
{
	if (PlanetMesh)
	{
		UMaterialInstanceDynamic* PlanetMaterial = UMaterialInstanceDynamic::Create(
			UMaterial::GetDefaultMaterial(MD_Surface), this);

		if (PlanetMaterial)
		{
			// Set planet appearance
			PlanetMaterial->SetVectorParameterValue(FName("BaseColor"), PlanetColor);
			PlanetMaterial->SetScalarParameterValue(FName("Metallic"), 0.0f);
			PlanetMaterial->SetScalarParameterValue(FName("Roughness"), 0.8f);

			PlanetMesh->SetMaterial(0, PlanetMaterial);
		}
	}
}
