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
class UFireButtonWidget;

/**
 * AUFOPlayerController
 *
 * Handles:
 *  - Touch input  : tap to select an NPC
 *  - NPC selection: highlight selected NPC with a colour ring
 *  - Per-frame    : refresh enemy highlights and token billboards
 *  - Target view  : live 3-D scene capture preview of the selected NPC (top-left HUD)
 *  - Firing       : shoot a laser projectile at the selected enemy NPC
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

	/** Fire a laser projectile toward SelectedNPC (called by FireButtonWidget) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void FireLaserAtSelectedNPC();

	// -----------------------------------------------------------------------
	// Protected state
	// -----------------------------------------------------------------------
protected:
	FTimerHandle PawnCheckTimer;

	UPROPERTY(BlueprintReadOnly, Category = "UFO")
	AUFOPawn* UFOPawn;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UTrackballUI* TrackballWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	ANPCUFOPawn* SelectedNPC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	float MaxSelectionScreenDistance;

	UPROPERTY(BlueprintReadOnly, Category = "Target View")
	ATargetCaptureActor* TargetCaptureActor;

	UPROPERTY(BlueprintReadOnly, Category = "Target View")
	UTargetViewWidget* TargetViewWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	UFireButtonWidget* FireButtonWidget;

	/** Minimum seconds between shots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FireCooldown = 0.4f;

	// -----------------------------------------------------------------------
	// Private helpers
	// -----------------------------------------------------------------------
private:
	void OnPawnReady();
	void CreateTrackballUI();
	void SpawnTargetCaptureActor();
	void CreateTargetViewUI();
	void CreateFireButtonUI();
	void RefreshTargetPreview();

	void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
	void SelectNPC(ANPCUFOPawn* NewSelection);

	ANPCUFOPawn* FindBestNPCAtScreenPosition(const FVector2D& ScreenPosition) const;
	ANPCUFOPawn* RaycastNPCAtScreen(const FVector2D& ScreenPosition) const;
	ANPCUFOPawn* FindNearestNPCOnScreen(const FVector2D& ScreenPosition) const;

	bool           IsNPCSelectableFromDock(const ANPCUFOPawn* NPC) const;
	EColorDockSide GetDockSideForNPC(const ANPCUFOPawn* NPC) const;
	void           ApplyNPCSelectionVisuals(ANPCUFOPawn* NPC, EColorDockSide DockSide) const;
	void           RefreshNPCEnemyHighlighting() const;
	void           RefreshTokenSelectedBillboards() const;

	float LastFireTime = -1.0f;
};
