// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrackballUI.h"
#include "UFOPlayerController.generated.h"

class AUFOPawn;
class ANPCUFOPawn;
class ATargetCaptureActor;
class UTargetViewWidget;

/**
 * AUFOPlayerController
 *
 * Handles:
 *  - Touch input  : tap to select an NPC
 *  - NPC selection: highlight selected NPC with a colour ring
 *  - Per-frame    : refresh enemy highlights and token billboards
 *  - Target view  : live 3-D scene capture preview of the selected NPC (top-left HUD)
 */
UCLASS()
class UFO_API AUFOPlayerController : public APlayerController
{
	GENERATED_BODY()

	// -----------------------------------------------------------------------
	// Lifecycle
	// -----------------------------------------------------------------------
public:
	AUFOPlayerController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	// -----------------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------------
public:
	/** Select the NPC that belongs to a given token index and dock side */
	void SelectNPCForToken(int32 TokenIndex, EColorDockSide PreferredDockSide);

	// -----------------------------------------------------------------------
	// Protected state
	// -----------------------------------------------------------------------
protected:
	/** Timer used to wait for the pawn to be possessed before caching it */
	FTimerHandle PawnCheckTimer;

	UPROPERTY(BlueprintReadOnly, Category = "UFO")
	AUFOPawn* UFOPawn;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UTrackballUI* TrackballWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	ANPCUFOPawn* SelectedNPC;

	/** Maximum screen-space distance (px) for proximity-based NPC selection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	float MaxSelectionScreenDistance;

	/** The actor that orbits the selected NPC and captures its 3-D view */
	UPROPERTY(BlueprintReadOnly, Category = "Target View")
	ATargetCaptureActor* TargetCaptureActor;

	/** HUD widget that shows the live capture in the top-left corner */
	UPROPERTY(BlueprintReadOnly, Category = "Target View")
	UTargetViewWidget* TargetViewWidget;

	// -----------------------------------------------------------------------
	// Private helpers
	// -----------------------------------------------------------------------
private:
	/** Called once the pawn is ready — caches UFOPawn and creates the UI */
	void OnPawnReady();

	/** Creates and adds the TrackballUI widget to the viewport */
	void CreateTrackballUI();

	/** Spawns the ATargetCaptureActor into the world */
	void SpawnTargetCaptureActor();

	/** Creates and adds the TargetViewWidget to the viewport */
	void CreateTargetViewUI();

	/** Updates the capture actor and widget to reflect SelectedNPC */
	void RefreshTargetPreview();

	/** Touch press handler — finds and selects the tapped NPC */
	void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);

	/** Sets SelectedNPC and applies the appropriate selection ring colour */
	void SelectNPC(ANPCUFOPawn* NewSelection);

	/**
	 * Finds the best NPC near ScreenPosition.
	 * First tries a direct raycast; falls back to nearest projected NPC.
	 */
	ANPCUFOPawn* FindBestNPCAtScreenPosition(const FVector2D& ScreenPosition) const;

	/** Raycast helper — returns the NPC directly under ScreenPosition, or nullptr */
	ANPCUFOPawn* RaycastNPCAtScreen(const FVector2D& ScreenPosition) const;

	/** Proximity helper — returns the nearest on-screen NPC within MaxSelectionScreenDistance */
	ANPCUFOPawn* FindNearestNPCOnScreen(const FVector2D& ScreenPosition) const;

	/** Returns true if NPC has a valid dock assignment and can be selected */
	bool IsNPCSelectableFromDock(const ANPCUFOPawn* NPC) const;

	/** Returns Enemy / Friendly / None for the given NPC based on the trackball dock */
	EColorDockSide GetDockSideForNPC(const ANPCUFOPawn* NPC) const;

	/** Applies the correct selection ring colour and selected state to an NPC */
	void ApplyNPCSelectionVisuals(ANPCUFOPawn* NPC, EColorDockSide DockSide) const;

	/** Per-frame: marks every NPC as enemy-highlighted or not */
	void RefreshNPCEnemyHighlighting() const;

	/** Per-frame: updates selection ring colour for all token-assigned NPCs */
	void RefreshTokenSelectedBillboards() const;
};
