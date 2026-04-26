// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrackballUI.h"
#include "UFOPlayerController.generated.h"

class AUFOPawn;
class ANPCUFOPawn;

/**
 * Player controller for UFO - sets up enhanced input and manages UFO interactions
 */
UCLASS()
class UFO_API AUFOPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUFOPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;
	void SelectNPCForToken(int32 TokenIndex, EColorDockSide PreferredDockSide);

protected:
	// Timer for checking when pawn is ready
	FTimerHandle PawnCheckTimer;

	// Internal helper to create trackball UI
	void CreateTrackballUI();

	UPROPERTY(BlueprintReadOnly, Category = "UFO")
	AUFOPawn* UFOPawn;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UTrackballUI* TrackballWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	ANPCUFOPawn* SelectedNPC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	float MaxSelectionScreenDistance;

	void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
	void SelectNPC(ANPCUFOPawn* NewSelection);
	ANPCUFOPawn* FindBestNPCAtScreenPosition(const FVector2D& ScreenPosition) const;
	bool IsNPCSelectableFromDock(const ANPCUFOPawn* NPC) const;
	EColorDockSide GetDockSideForNPC(const ANPCUFOPawn* NPC) const;
	void RefreshNPCEnemyHighlighting() const;
	void RefreshTokenSelectedBillboards() const;
};
