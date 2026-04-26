// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetActor.generated.h"

class UStaticMeshComponent;

/**
 * Planet actor - a sphere orbiting the sun
 */
UCLASS()
class UFO_API APlanetActor : public AActor
{
	GENERATED_BODY()

public:
	APlanetActor();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UStaticMeshComponent* PlanetMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	float PlanetRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FLinearColor PlanetColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FString PlanetType; // "Earth", "Neptune", "Mars", etc.

private:
	void CreatePlanetMaterial();
};
