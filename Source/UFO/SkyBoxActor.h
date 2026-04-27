// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyBoxActor.generated.h"

class UStaticMeshComponent;
class UMaterial;

/**
 * Skybox actor - a large sphere with the M_Stars material applied
 */
UCLASS()
class UFO_API ASkyBoxActor : public AActor
{
	GENERATED_BODY()

public:
	ASkyBoxActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Skybox")
	void SetSkyboxRadius(float InRadius);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skybox")
	UStaticMeshComponent* SkyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skybox")
	float SkyboxRadius;

private:
	/** Loaded in the constructor via FObjectFinder, applied in BeginPlay */
	UPROPERTY()
	UMaterial* StarMaterial;

	void ApplySkyboxScale();
	void CreateSkyboxMaterial();
};
