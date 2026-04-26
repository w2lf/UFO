// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SunActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UMaterialInterface;
class UParticleSystem;
class UParticleSystemComponent;

/**
 * The sun actor - an emissive sphere in the center of the arena
 */
UCLASS()
class UFO_API ASunActor : public AActor
{
	GENERATED_BODY()

public:
	ASunActor();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sun")
	UStaticMeshComponent* SunMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun")
	float SunRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun")
	FLinearColor SunColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun")
	float SunIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun|Glow")
	float SunGlowIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun|Glow")
	float SunGlowRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sun|Glow")
	UPointLightComponent* SunGlowLight;

	// Assign a custom sun material asset here (for example M_SunPlasma).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun|Shader")
	UMaterialInterface* SunMaterialTemplate;

	// Optional particle template for extra surface/plasma energy effects.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun|Particles")
	UParticleSystem* SunPlasmaParticlesTemplate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sun|Particles")
	UParticleSystemComponent* SunPlasmaParticles;

private:
	void CreateSunMaterial();
};
