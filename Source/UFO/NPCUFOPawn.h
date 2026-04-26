// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NPCUFOPawn.generated.h"

class UStaticMeshComponent;

/**
 * NPC UFO pawn - drifts autonomously through the arena, picking random
 * waypoints and smoothly rotating to face / fly toward them.
 */
UCLASS()
class UFO_API ANPCUFOPawn : public APawn
{
	GENERATED_BODY()

public:
	ANPCUFOPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void SetSelected(bool bInSelected);
	void SetColorToken(int32 InColorTokenIndex, const FLinearColor& InTokenColor);
	int32 GetColorTokenIndex() const { return ColorTokenIndex; }
	void SetSelectionRingColor(const FLinearColor& InRingColor);
	void SetEnemyHighlighted(bool bInEnemyHighlighted);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	UStaticMeshComponent* UFOMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO")
	UStaticMeshComponent* UFODomeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Selection")
	class USceneComponent* SelectionMarkerRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Selection")
	class USceneComponent* SelectionMarkerSpinRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UFO|Selection")
	TArray<UStaticMeshComponent*> SelectionMarkerSegments;

	/** How fast the NPC moves toward its waypoint (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float MoveSpeed;

	/** Radius of the sphere within which random waypoints are picked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float WanderRadius;

	/** How quickly the NPC rotates to face its waypoint (degrees/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float TurnSpeed;

	/** Distance to waypoint at which a new one is chosen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float WaypointReachedDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerSpinSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerMinScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerMaxScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerDistanceScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	float SelectionMarkerBoundsPadding;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Identity")
	int32 ColorTokenIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Identity")
	FLinearColor TokenColor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Selection")
	FLinearColor SelectionRingColor;

private:
	FVector CurrentWaypoint;
	bool bIsSelected;
	float SelectionMarkerAngle;
	bool bEnemyHighlighted;

	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* UFOMeshMID;

	UPROPERTY(Transient)
	class UMaterialInstanceDynamic* UFODomeMeshMID;

	void PickNewWaypoint();
	float ComputeSelectionTargetRadius() const;
	void UpdateBodyColorFromState();
};
