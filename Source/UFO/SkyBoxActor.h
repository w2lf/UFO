// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkyBoxActor.generated.h"

class UStaticMeshComponent;

/**
 * Skybox actor - uses the engine's built-in SM_SkySphere mesh with M_Star material
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
	void ApplySkyboxScale();
};
