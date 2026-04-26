// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NPCUFOPawn.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * ANPCUFOPawn
 *
 * An autonomous UFO saucer that wanders the arena by picking random waypoints
 * and smoothly rotating toward them.
 *
 * Also carries:
 *  - A colour token identity (used by the TrackballUI system)
 *  - A spinning selection ring (shown when the player selects this NPC)
 *  - Enemy highlight state (body turns red when flagged as an enemy)
 */
UCLASS()
class UFO_API ANPCUFOPawn : public APawn
{
	GENERATED_BODY()

	// -----------------------------------------------------------------------
	// Lifecycle
	// -----------------------------------------------------------------------
public:
	ANPCUFOPawn();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// -----------------------------------------------------------------------
	// Components
	// -----------------------------------------------------------------------
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Mesh")
	UStaticMeshComponent* UFOMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Mesh")
	UStaticMeshComponent* UFODomeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Selection")
	class USceneComponent* SelectionMarkerRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Selection")
	class USceneComponent* SelectionMarkerSpinRoot;

	/** Individual cylinder segments that make up the spinning selection ring */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Selection")
	TArray<UStaticMeshComponent*> SelectionMarkerSegments;

	// -----------------------------------------------------------------------
	// Tuning
	// -----------------------------------------------------------------------
protected:
	/** Top translation speed (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement")
	float MoveSpeed;

	/** Radius of the sphere within which random waypoints are chosen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement")
	float WanderRadius;

	/** Rotation speed toward the current waypoint (degrees/s scaled by TurnSpeed/90) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement")
	float TurnSpeed;

	/** Distance at which we consider a waypoint reached and pick a new one */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement")
	float WaypointReachedDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerSpinSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerMinScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerMaxScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerDistanceScale;

	/** How much larger than the mesh bounds the selection ring is drawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerBoundsPadding;

	// -----------------------------------------------------------------------
	// Identity
	// -----------------------------------------------------------------------
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Identity")
	int32 ColorTokenIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Identity")
	FLinearColor TokenColor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Selection")
	FLinearColor SelectionRingColor;

	// -----------------------------------------------------------------------
	// Private state
	// -----------------------------------------------------------------------
private:
	FVector  CurrentWaypoint;
	bool     bIsSelected;
	float    SelectionMarkerAngle;
	bool     bEnemyHighlighted;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* UFOMeshMID;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* UFODomeMeshMID;

	// -----------------------------------------------------------------------
	// Private helpers
	// -----------------------------------------------------------------------
private:
	/** Constructor helper: create saucer body, dome and selection ring */
	void SetupMeshComponents(const UStaticMesh* CylinderMesh, const UStaticMesh* SphereMesh);

	/** Constructor helper: build the spinning ring out of cylinder segments */
	void SetupSelectionRing(const UStaticMesh* CylinderMesh);

	/** Tick helper: orient and scale the selection marker toward the camera */
	void UpdateSelectionMarker(float DeltaTime);

	/** Picks a new random waypoint inside the wander sphere */
	void PickNewWaypoint();

	/** Returns the approximate world-space radius of the UFO (used to size the ring) */
	float ComputeSelectionTargetRadius() const;

	/** Ensures a dynamic material instance exists on MeshComp and applies colour */
	void EnsureAndApplyMID(
		UStaticMeshComponent* MeshComp,
		UMaterialInstanceDynamic*& CachedMID,
		const FLinearColor& Color,
		float EmissiveScale);

	/** Re-applies body colour based on enemy-highlight state and token colour */
	void UpdateBodyColorFromState();

	// -----------------------------------------------------------------------
	// Public API
	// -----------------------------------------------------------------------
public:
	void  SetSelected(bool bInSelected);
	void  SetColorToken(int32 InColorTokenIndex, const FLinearColor& InTokenColor);
	void  SetSelectionRingColor(const FLinearColor& InRingColor);
	void  SetEnemyHighlighted(bool bInEnemyHighlighted);

	int32 GetColorTokenIndex() const { return ColorTokenIndex; }
};
