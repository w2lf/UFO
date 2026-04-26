// Copyright Epic Games, Inc. All Rights Reserved.

#include "SunActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

ASunActor::ASunActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SunRadius = 1000.0f;
	SunColor = FLinearColor(127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 1.0f); // Yellow (127,127,0)
	SunIntensity = 12.0f;
	SunGlowIntensity = 140000.0f;
	SunGlowRadius = 22000.0f;
	SunMaterialTemplate = nullptr;
	SunPlasmaParticlesTemplate = nullptr;

	// Auto-load the project's M_SunPlasma material.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SunPlasmaMat(TEXT("/Game/M_SunPlasma.M_SunPlasma"));
	if (SunPlasmaMat.Succeeded())
	{
		SunMaterialTemplate = SunPlasmaMat.Object;
	}

	// Create mesh
	SunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SunMesh"));
	RootComponent = SunMesh;

	// Load sphere mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		SunMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Set collision
	SunMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SunMesh->SetCollisionObjectType(ECC_WorldStatic);
	SunMesh->CastShadow = false;
	SunMesh->SetRelativeScale3D(FVector(SunRadius / 50.0f)); // Scale to desired radius (engine sphere is 100 units)

	// Add a dedicated glow light so the sun visibly illuminates nearby objects.
	SunGlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SunGlowLight"));
	if (SunGlowLight)
	{
		SunGlowLight->SetupAttachment(SunMesh);
		SunGlowLight->SetIntensity(SunGlowIntensity);
		SunGlowLight->SetAttenuationRadius(SunGlowRadius);
		SunGlowLight->SetLightColor(SunColor);
		SunGlowLight->SetCastShadows(false);
		SunGlowLight->SetIndirectLightingIntensity(2.0f);
	}

	SunPlasmaParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SunPlasmaParticles"));
	if (SunPlasmaParticles)
	{
		SunPlasmaParticles->SetupAttachment(SunMesh);
		SunPlasmaParticles->SetRelativeScale3D(FVector(1.2f));
		SunPlasmaParticles->bAutoActivate = true;
	}
}

void ASunActor::BeginPlay()
{
	Super::BeginPlay();

	CreateSunMaterial();

	if (SunGlowLight)
	{
		SunGlowLight->SetLightColor(SunColor);
		SunGlowLight->SetIntensity(SunGlowIntensity);
		SunGlowLight->SetAttenuationRadius(SunGlowRadius);
	}

	if (SunPlasmaParticles)
	{
		if (SunPlasmaParticlesTemplate)
		{
			SunPlasmaParticles->SetTemplate(SunPlasmaParticlesTemplate);
			SunPlasmaParticles->ActivateSystem(true);
		}
		else
		{
			SunPlasmaParticles->DeactivateSystem();
		}
	}
}

void ASunActor::CreateSunMaterial()
{
	if (SunMesh)
	{
		UMaterialInterface* BaseMaterial = SunMaterialTemplate;

		// Fall back to the engine's built-in basic shape material.
		if (!BaseMaterial)
		{
			BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
		}

		if (!BaseMaterial)
		{
			BaseMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		UMaterialInstanceDynamic* SunMaterialMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (SunMaterialMID)
		{
			const FLinearColor YellowBase(127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 1.0f);
			const FLinearColor YellowEmissive(
				YellowBase.R * SunIntensity,
				YellowBase.G * SunIntensity,
				YellowBase.B * SunIntensity,
				1.0f
			);

			// Parameters matching M_SunPlasma material.
			SunMaterialMID->SetVectorParameterValue(FName("EmissiveColor"), YellowEmissive);
			SunMaterialMID->SetScalarParameterValue(FName("EmissiveStrength"), SunIntensity);
			SunMaterialMID->SetScalarParameterValue(FName("PlasmaSpeed"), 1.0f);
			SunMaterialMID->SetScalarParameterValue(FName("PlasmaDistortion"), 0.2f);
			SunMaterialMID->SetScalarParameterValue(FName("PlasmaPulse"), 1.0f);

			SunMesh->SetMaterial(0, SunMaterialMID);
		}
	}
}
