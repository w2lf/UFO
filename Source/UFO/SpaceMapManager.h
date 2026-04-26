// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpaceMapManager.generated.h"

class UPN_MeshParticles;
class UTextureCube;

/**
 * Space map manager - sets up the entire space arena
 * Spawns sun, planets, stars, and sets up the environment
 */
UCLASS()
class UFO_API ASpaceMapManager : public AActor
{
	GENERATED_BODY()

public:
	ASpaceMapManager();

	virtual void BeginPlay() override;

protected:
	// Scene capture for skybox
	class USceneCaptureComponent2D* SkySphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
	float SunDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
	float PlanetLeftDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
	float PlanetRightDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space")
	float PlanetVerticalOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space|Sky")
	float SkyboxRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space|Sky")
	TSoftObjectPtr<UTextureCube> SkyLightCubemap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Space|NPC")
	int32 NPCUFOCount;

private:
	void SetupEnvironment();
	void SpawnAmbientSkyLight();
	void SpawnSun();
	void SpawnPlanets();
	void SpawnSkybox();
	void SpawnNPCUFOs();
};
