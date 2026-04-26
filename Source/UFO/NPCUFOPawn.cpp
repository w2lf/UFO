// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCUFOPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Math/RotationMatrix.h"

ANPCUFOPawn::ANPCUFOPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	MoveSpeed = 600.0f;
	WanderRadius = 4000.0f;
	TurnSpeed = 90.0f;
	WaypointReachedDistance = 200.0f;
	SelectionMarkerSpinSpeed = 120.0f;
	SelectionMarkerMinScale = 0.7f;
	SelectionMarkerMaxScale = 1.9f;
	SelectionMarkerDistanceScale = 2200.0f;
	SelectionMarkerBoundsPadding = 0.5f;
	ColorTokenIndex = INDEX_NONE;
	TokenColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
	SelectionRingColor = FLinearColor(0.1f, 1.0f, 0.2f, 1.0f);
	CurrentWaypoint = FVector::ZeroVector;
	bIsSelected = false;
	SelectionMarkerAngle = 0.0f;
	bEnemyHighlighted = false;
	UFOMeshMID = nullptr;
	UFODomeMeshMID = nullptr;

	// Saucer body
	UFOMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOMesh"));
	RootComponent = UFOMesh;
	UFOMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UFOMesh->SetCollisionObjectType(ECC_WorldDynamic);
	UFOMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	UFOMesh->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (CylinderMesh.Succeeded())
	{
		UFOMesh->SetStaticMesh(CylinderMesh.Object);
		UFOMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 0.22f));
	}

	// Dome on top
	UFODomeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFODomeMesh"));
	UFODomeMesh->SetupAttachment(UFOMesh);
	UFODomeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UFODomeMesh->SetMobility(EComponentMobility::Movable);
	if (SphereMesh.Succeeded())
	{
		UFODomeMesh->SetStaticMesh(SphereMesh.Object);
		UFODomeMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
		UFODomeMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.35f));
	}

	SelectionMarkerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionMarkerRoot"));
	SelectionMarkerRoot->SetupAttachment(UFOMesh);
	SelectionMarkerRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
	SelectionMarkerRoot->SetRelativeScale3D(FVector::OneVector);

	SelectionMarkerSpinRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionMarkerSpinRoot"));
	SelectionMarkerSpinRoot->SetupAttachment(SelectionMarkerRoot);

	UMaterialInterface* MarkerBaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	UMaterialInstanceDynamic* MarkerMID = nullptr;
	if (MarkerBaseMaterial)
	{
		MarkerMID = UMaterialInstanceDynamic::Create(MarkerBaseMaterial, this);
		if (MarkerMID)
		{
			MarkerMID->SetVectorParameterValue(FName("Color"), SelectionRingColor);
			MarkerMID->SetVectorParameterValue(FName("BaseColor"), SelectionRingColor);
			MarkerMID->SetVectorParameterValue(FName("EmissiveColor"), SelectionRingColor * 14.0f);
		}
	}

	if (CylinderMesh.Succeeded())
	{
		constexpr int32 SegmentCount = 12;
		constexpr float RingRadius = 260.0f;

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const FString SegmentName = FString::Printf(TEXT("SelectionMarkerSegment_%d"), SegmentIndex);
			UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(*SegmentName);
			Segment->SetupAttachment(SelectionMarkerSpinRoot);
			Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Segment->SetMobility(EComponentMobility::Movable);
			Segment->SetCastShadow(false);
			Segment->SetStaticMesh(CylinderMesh.Object);

			const float AngleRadians = FMath::DegreesToRadians((360.0f / SegmentCount) * SegmentIndex);
			const FVector SegmentLocation(
				FMath::Cos(AngleRadians) * RingRadius,
				FMath::Sin(AngleRadians) * RingRadius,
				0.0f
			);

			Segment->SetRelativeLocation(SegmentLocation);
			Segment->SetRelativeRotation(FRotator(90.0f, FMath::RadiansToDegrees(AngleRadians), 0.0f));
			Segment->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.28f));
			Segment->SetVisibility(false);

			if (MarkerMID)
			{
				Segment->SetMaterial(0, MarkerMID);
			}

			SelectionMarkerSegments.Add(Segment);
		}
	}
}

void ANPCUFOPawn::BeginPlay()
{
	Super::BeginPlay();
	PickNewWaypoint();
}

void ANPCUFOPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Location = GetActorLocation();
	FVector ToWaypoint = CurrentWaypoint - Location;
	float Distance = ToWaypoint.Size();

	if (Distance < WaypointReachedDistance)
	{
		PickNewWaypoint();
		ToWaypoint = CurrentWaypoint - Location;
		Distance = ToWaypoint.Size();
	}

	if (Distance > 1.0f)
	{
		FVector Direction = ToWaypoint / Distance;

		// Smoothly rotate toward waypoint
		FRotator CurrentRot = GetActorRotation();
		FRotator TargetRot = Direction.Rotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, TurnSpeed / 90.0f);
		SetActorRotation(NewRot);

		// Move forward
		FVector NewLocation = Location + Direction * MoveSpeed * DeltaTime;
		SetActorLocation(NewLocation, true);
	}

	if (bIsSelected && SelectionMarkerRoot)
	{
		SelectionMarkerAngle += SelectionMarkerSpinSpeed * DeltaTime;

		if (SelectionMarkerSpinRoot)
		{
			SelectionMarkerSpinRoot->SetRelativeRotation(FRotator(0.0f, SelectionMarkerAngle, 0.0f));
		}

		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
				{
					const FVector MarkerLocation = SelectionMarkerRoot->GetComponentLocation();
					const FVector ToCamera = CameraManager->GetCameraLocation() - MarkerLocation;

					if (!ToCamera.IsNearlyZero())
					{
						SelectionMarkerRoot->SetWorldRotation(FRotationMatrix::MakeFromZ(ToCamera.GetSafeNormal()).Rotator());
					}

					const float DesiredRadius = ComputeSelectionTargetRadius() * SelectionMarkerBoundsPadding;
					const float MarkerScale = FMath::Clamp(
						DesiredRadius / 260.0f,
						SelectionMarkerMinScale,
						SelectionMarkerMaxScale
					);
					SelectionMarkerRoot->SetWorldScale3D(FVector(MarkerScale));
				}
			}
		}
	}
}

float ANPCUFOPawn::ComputeSelectionTargetRadius() const
{
	FBox CombinedBounds(ForceInit);

	if (UFOMesh)
	{
		CombinedBounds += UFOMesh->Bounds.GetBox();
	}

	if (UFODomeMesh)
	{
		CombinedBounds += UFODomeMesh->Bounds.GetBox();
	}

	if (!CombinedBounds.IsValid)
	{
		return 260.0f;
	}

	return CombinedBounds.GetExtent().GetAbsMax();
}

void ANPCUFOPawn::PickNewWaypoint()
{
	FVector Origin = FVector::ZeroVector; // Wander around the arena center
	FVector RandomOffset = FMath::VRand() * FMath::FRandRange(WanderRadius * 0.3f, WanderRadius);
	CurrentWaypoint = Origin + RandomOffset;
}

void ANPCUFOPawn::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;
	if (!bIsSelected && SelectionMarkerRoot)
	{
		SelectionMarkerRoot->SetRelativeScale3D(FVector::OneVector);
	}

	for (UStaticMeshComponent* Segment : SelectionMarkerSegments)
	{
		if (Segment)
		{
			Segment->SetVisibility(bIsSelected);
		}
	}
}

void ANPCUFOPawn::SetColorToken(int32 InColorTokenIndex, const FLinearColor& InTokenColor)
{
	ColorTokenIndex = InColorTokenIndex;
	TokenColor = InTokenColor;
	UpdateBodyColorFromState();
}

void ANPCUFOPawn::SetEnemyHighlighted(bool bInEnemyHighlighted)
{
	bEnemyHighlighted = bInEnemyHighlighted;
	UpdateBodyColorFromState();
}

void ANPCUFOPawn::UpdateBodyColorFromState()
{
	const FLinearColor DisplayColor = bEnemyHighlighted
		? FLinearColor(1.0f, 0.26f, 0.26f, 1.0f)
		: TokenColor;

	auto EnsureAndApplyMID = [&](UStaticMeshComponent* MeshComp, UMaterialInstanceDynamic*& CachedMID, float EmissiveScale)
	{
		if (!MeshComp)
		{
			return;
		}

		if (!CachedMID)
		{
			UMaterialInterface* BaseMaterial = MeshComp->GetMaterial(0);
			if (!BaseMaterial)
			{
				BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
			}

			if (!BaseMaterial)
			{
				return;
			}

			CachedMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (CachedMID)
			{
				MeshComp->SetMaterial(0, CachedMID);
			}
		}

		if (!CachedMID)
		{
			return;
		}

		CachedMID->SetVectorParameterValue(FName("Color"), DisplayColor);
		CachedMID->SetVectorParameterValue(FName("BaseColor"), DisplayColor);
		CachedMID->SetVectorParameterValue(FName("EmissiveColor"), DisplayColor * EmissiveScale);
		CachedMID->SetScalarParameterValue(FName("Metallic"), 0.0f);
		CachedMID->SetScalarParameterValue(FName("Roughness"), 0.35f);
	};

	EnsureAndApplyMID(UFOMesh, UFOMeshMID, 2.0f);
	EnsureAndApplyMID(UFODomeMesh, UFODomeMeshMID, 2.0f);
}

void ANPCUFOPawn::SetSelectionRingColor(const FLinearColor& InRingColor)
{
	SelectionRingColor = InRingColor;
	for (UStaticMeshComponent* Segment : SelectionMarkerSegments)
	{
		if (!Segment)
		{
			continue;
		}

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Segment->GetMaterial(0));
		if (!MID)
		{
			UMaterialInterface* BaseMaterial = Segment->GetMaterial(0);
			if (!BaseMaterial)
			{
				BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
			}

			if (!BaseMaterial)
			{
				continue;
			}

			MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (!MID)
			{
				continue;
			}

			Segment->SetMaterial(0, MID);
		}

		MID->SetVectorParameterValue(FName("Color"), SelectionRingColor);
		MID->SetVectorParameterValue(FName("BaseColor"), SelectionRingColor);
		MID->SetVectorParameterValue(FName("EmissiveColor"), SelectionRingColor * 14.0f);
	}
}
