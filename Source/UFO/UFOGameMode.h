// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UFOGameMode.generated.h"

class ASpaceMapManager;

/**
 * Game mode for UFO - sets up the basic game and space environment
 */
UCLASS()
class UFO_API AUFOGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUFOGameMode();

	virtual void BeginPlay() override;

private:
	// Check and ensure player spawn setup
	void CheckForPlayerStart();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	ASpaceMapManager* SpaceMap;
};
