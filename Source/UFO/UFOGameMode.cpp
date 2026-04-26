// Copyright Epic Games, Inc. All Rights Reserved.

#include "UFOGameMode.h"
#include "UFOPawn.h"
#include "UFOPlayerController.h"
#include "SpaceMapManager.h"
#include "EngineUtils.h"

AUFOGameMode::AUFOGameMode()
{
	PlayerControllerClass = AUFOPlayerController::StaticClass();
	DefaultPawnClass = AUFOPawn::StaticClass();
	bStartPlayersAsSpectators = false;
}

void AUFOGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AUFOGameMode::BeginPlay - Game starting, spawning environment"));

	// Ensure there's a player start for spawning
	CheckForPlayerStart();

	// Force spawn the player if not already spawned
	if (GetWorld())
	{
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			APlayerController* PlayerController = *It;
			if (PlayerController && !PlayerController->GetPawn())
			{
				UE_LOG(LogTemp, Warning, TEXT("AUFOGameMode: Detected controller without pawn, forcing spawn"));
				RestartPlayer(PlayerController);
			}
		}
	}

	// Spawn space map if not already in the level
	if (!SpaceMap && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpaceMap = GetWorld()->SpawnActor<ASpaceMapManager>(
			ASpaceMapManager::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (SpaceMap)
		{
			#if WITH_EDITOR
			SpaceMap->SetActorLabel(FString(TEXT("SpaceMap")));
			#endif
			UE_LOG(LogTemp, Warning, TEXT("AUFOGameMode::SpaceMap spawned successfully"));
		}
	}
}

void AUFOGameMode::CheckForPlayerStart()
{
	// The engine automatically finds a PlayerStart and spawns the default pawn there
	// This is just a safety check
	if (GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("AUFOGameMode::CheckForPlayerStart - World exists, pawn should spawn"));
	}
}
