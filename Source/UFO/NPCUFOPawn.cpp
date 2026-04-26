// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCUFOPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Math/RotationMatrix.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace NPCUFOPawnConstants
{
	constexpr float SaucerScaleXY        = 2.2f;
	constexpr float SaucerScaleZ         = 0.22f;
	constexpr float DomeOffsetZ          = 45.0f;
	constexpr float DomeScaleZ           = 0.35f;

	constexpr int32  RingSegmentCount    = 12;
	constexpr float  RingRadius          = 260.0f;
	constexpr float  SegmentScaleXY      = 0.05f;
	constexpr float  SegmentScaleZ       = 0.28f;
	constexpr float  MarkerPivotZ        = 170.0f;

	constexpr float  EnemyEmissiveScale  = 2.0f;
	constexpr float  RingEmissiveScale   = 14.0f;

	const FLinearColor EnemyBodyColor(1.0f, 0.26f, 0.26f, 1.0f);
	const char* BasicShapeMaterialPath = "/Engine/BasicShapes/BasicShapeMaterial";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ANPCUFOPawn::ANPCUFOPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Movement defaults
	MoveSpeed               = 600.0f;
	WanderRadius            = 4000.0f;
	TurnSpeed               = 90.0f;
	WaypointReachedDistance = 200.0f;

	// Selection marker defaults
	SelectionMarkerSpinSpeed       = 120.0f;
	SelectionMarkerMinScale        = 0.7f;
	SelectionMarkerMaxScale        = 1.9f;
	SelectionMarkerDistanceScale   = 2200.0f;
	SelectionMarkerBoundsPadding   = 0.5f;

	// Identity defaults
	ColorTokenIndex    = INDEX_NONE;
	TokenColor         = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
	SelectionRingColor = FLinearColor(0.1f, 1.0f, 0.2f, 1.0f);

	// Runtime state
	CurrentWaypoint       = FVector::ZeroVector;
	bIsSelected           = false;
	SelectionMarkerAngle  = 0.0f;
	bEnemyHighlighted     = false;
	UFOMeshMID            = nullptr;
	UFODomeMeshMID        = nullptr;

	// Mesh assets
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	SetupMeshComponents(
		CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr,
		SphereMesh.Succeeded()   ? SphereMesh.Object   : nullptr);

	SetupSelectionRing(
		CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr);
}

// ---------------------------------------------------------------------------
// Constructor helpers
// ---------------------------------------------------------------------------

void ANPCUFOPawn::SetupMeshComponents(const UStaticMesh* CylinderMesh, const UStaticMesh* SphereMesh)
{
	// --- Saucer body (root) ---
	UFOMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFOMesh"));
	RootComponent = UFOMesh;
	UFOMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	UFOMesh->SetCollisionObjectType(ECC_WorldDynamic);
	UFOMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	UFOMesh->SetMobility(EComponentMobility::Movable);

	if (CylinderMesh)
	{
		UFOMesh->SetStaticMesh(const_cast<UStaticMesh*>(CylinderMesh));
		UFOMesh->SetRelativeScale3D(FVector(
			NPCUFOPawnConstants::SaucerScaleXY,
			NPCUFOPawnConstants::SaucerScaleXY,
			NPCUFOPawnConstants::SaucerScaleZ));
	}

	// --- Glass dome ---
	UFODomeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UFODomeMesh"));
	UFODomeMesh->SetupAttachment(UFOMesh);
	UFODomeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UFODomeMesh->SetMobility(EComponentMobility::Movable);

	if (SphereMesh)
	{
		UFODomeMesh->SetStaticMesh(const_cast<UStaticMesh*>(SphereMesh));
		UFODomeMesh->SetRelativeLocation(FVector(0.0f, 0.0f, NPCUFOPawnConstants::DomeOffsetZ));
		UFODomeMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, NPCUFOPawnConstants::DomeScaleZ));
	}
}

void ANPCUFOPawn::SetupSelectionRing(const UStaticMesh* CylinderMesh)
{
	// Pivot that bills toward the camera
	SelectionMarkerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionMarkerRoot"));
	SelectionMarkerRoot->SetupAttachment(UFOMesh);
	SelectionMarkerRoot->SetRelativeLocation(FVector(0.0f, 0.0f, NPCUFOPawnConstants::MarkerPivotZ));

	// Child that actually spins
	SelectionMarkerSpinRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionMarkerSpinRoot"));
	SelectionMarkerSpinRoot->SetupAttachment(SelectionMarkerRoot);

	if (!CylinderMesh) return;

	// Shared emissive MID for all segments
	UMaterialInstanceDynamic* MarkerMID = nullptr;
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr, UTF8_TO_TCHAR(NPCUFOPawnConstants::BasicShapeMaterialPath)))
	{
		MarkerMID = UMaterialInstanceDynamic::Create(Base, this);
		if (MarkerMID)
		{
			MarkerMID->SetVectorParameterValue(FName("Color"),        SelectionRingColor);
			MarkerMID->SetVectorParameterValue(FName("BaseColor"),    SelectionRingColor);
			MarkerMID->SetVectorParameterValue(FName("EmissiveColor"),
				SelectionRingColor * NPCUFOPawnConstants::RingEmissiveScale);
		}
	}

	const int32 Count = NPCUFOPawnConstants::RingSegmentCount;
	for (int32 i = 0; i < Count; ++i)
	{
		const FString Name          = FString::Printf(TEXT("SelectionMarkerSegment_%d"), i);
		UStaticMeshComponent* Seg   = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Seg->SetupAttachment(SelectionMarkerSpinRoot);
		Seg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Seg->SetMobility(EComponentMobility::Movable);
		Seg->SetCastShadow(false);
		Seg->SetStaticMesh(const_cast<UStaticMesh*>(CylinderMesh));
		Seg->SetVisibility(false);

		const float AngleRad = FMath::DegreesToRadians((360.0f / Count) * i);
		Seg->SetRelativeLocation(FVector(
			FMath::Cos(AngleRad) * NPCUFOPawnConstants::RingRadius,
			FMath::Sin(AngleRad) * NPCUFOPawnConstants::RingRadius,
			0.0f));
		Seg->SetRelativeRotation(FRotator(90.0f, FMath::RadiansToDegrees(AngleRad), 0.0f));
		Seg->SetRelativeScale3D(FVector(
			NPCUFOPawnConstants::SegmentScaleXY,
			NPCUFOPawnConstants::SegmentScaleXY,
			NPCUFOPawnConstants::SegmentScaleZ));

		if (MarkerMID)
		{
			Seg->SetMaterial(0, MarkerMID);
		}

		SelectionMarkerSegments.Add(Seg);
	}
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ANPCUFOPawn::BeginPlay()
{
	Super::BeginPlay();
	PickNewWaypoint();
}

void ANPCUFOPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- Wander movement ---
	const FVector Location  = GetActorLocation();
	FVector ToWaypoint      = CurrentWaypoint - Location;
	float   Distance        = ToWaypoint.Size();

	if (Distance < WaypointReachedDistance)
	{
		PickNewWaypoint();
		ToWaypoint = CurrentWaypoint - Location;
		Distance   = ToWaypoint.Size();
	}

	if (Distance > 1.0f)
	{
		const FVector Dir    = ToWaypoint / Distance;
		const FRotator NewRot = FMath::RInterpTo(
			GetActorRotation(), Dir.Rotation(), DeltaTime, TurnSpeed / 90.0f);
		SetActorRotation(NewRot);
		SetActorLocation(Location + Dir * MoveSpeed * DeltaTime, true);
	}

	// --- Selection marker ---
	if (bIsSelected)
	{
		UpdateSelectionMarker(DeltaTime);
	}
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void ANPCUFOPawn::UpdateSelectionMarker(float DeltaTime)
{
	if (!SelectionMarkerRoot) return;

	// Spin the ring
	SelectionMarkerAngle += SelectionMarkerSpinSpeed * DeltaTime;
	if (SelectionMarkerSpinRoot)
	{
		SelectionMarkerSpinRoot->SetRelativeRotation(FRotator(0.0f, SelectionMarkerAngle, 0.0f));
	}

	// Billboard toward camera and scale by apparent UFO size
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager) return;

	const FVector MarkerLocation = SelectionMarkerRoot->GetComponentLocation();
	const FVector ToCamera       = PC->PlayerCameraManager->GetCameraLocation() - MarkerLocation;

	if (!ToCamera.IsNearlyZero())
	{
		SelectionMarkerRoot->SetWorldRotation(
			FRotationMatrix::MakeFromZ(ToCamera.GetSafeNormal()).Rotator());
	}

	const float DesiredRadius = ComputeSelectionTargetRadius() * SelectionMarkerBoundsPadding;
	const float MarkerScale   = FMath::Clamp(
		DesiredRadius / NPCUFOPawnConstants::RingRadius,
		SelectionMarkerMinScale,
		SelectionMarkerMaxScale);
	SelectionMarkerRoot->SetWorldScale3D(FVector(MarkerScale));
}

void ANPCUFOPawn::PickNewWaypoint()
{
	CurrentWaypoint = FMath::VRand() * FMath::FRandRange(WanderRadius * 0.3f, WanderRadius);
}

float ANPCUFOPawn::ComputeSelectionTargetRadius() const
{
	FBox Bounds(ForceInit);
	if (UFOMesh)     { Bounds += UFOMesh->Bounds.GetBox(); }
	if (UFODomeMesh) { Bounds += UFODomeMesh->Bounds.GetBox(); }

	return Bounds.IsValid ? Bounds.GetExtent().GetAbsMax() : NPCUFOPawnConstants::RingRadius;
}

void ANPCUFOPawn::EnsureAndApplyMID(
	UStaticMeshComponent*     MeshComp,
	UMaterialInstanceDynamic*& CachedMID,
	const FLinearColor&        Color,
	float                      EmissiveScale)
{
	if (!MeshComp) return;

	if (!CachedMID)
	{
		UMaterialInterface* Base = MeshComp->GetMaterial(0);
		if (!Base)
		{
			Base = LoadObject<UMaterialInterface>(
				nullptr, UTF8_TO_TCHAR(NPCUFOPawnConstants::BasicShapeMaterialPath));
		}
		if (!Base) return;

		CachedMID = UMaterialInstanceDynamic::Create(Base, this);
		if (CachedMID)
		{
			MeshComp->SetMaterial(0, CachedMID);
		}
	}

	if (!CachedMID) return;

	CachedMID->SetVectorParameterValue(FName("Color"),        Color);
	CachedMID->SetVectorParameterValue(FName("BaseColor"),    Color);
	CachedMID->SetVectorParameterValue(FName("EmissiveColor"), Color * EmissiveScale);
	CachedMID->SetScalarParameterValue(FName("Metallic"),      0.0f);
	CachedMID->SetScalarParameterValue(FName("Roughness"),     0.35f);
}

void ANPCUFOPawn::UpdateBodyColorFromState()
{
	const FLinearColor DisplayColor = bEnemyHighlighted
		? NPCUFOPawnConstants::EnemyBodyColor
		: TokenColor;

	EnsureAndApplyMID(UFOMesh,     UFOMeshMID,     DisplayColor, NPCUFOPawnConstants::EnemyEmissiveScale);
	EnsureAndApplyMID(UFODomeMesh, UFODomeMeshMID, DisplayColor, NPCUFOPawnConstants::EnemyEmissiveScale);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void ANPCUFOPawn::SetSelected(bool bInSelected)
{
	bIsSelected = bInSelected;

	if (!bIsSelected && SelectionMarkerRoot)
	{
		SelectionMarkerRoot->SetRelativeScale3D(FVector::OneVector);
	}

	for (UStaticMeshComponent* Seg : SelectionMarkerSegments)
	{
		if (Seg) { Seg->SetVisibility(bIsSelected); }
	}
}

void ANPCUFOPawn::SetColorToken(int32 InColorTokenIndex, const FLinearColor& InTokenColor)
{
	ColorTokenIndex = InColorTokenIndex;
	TokenColor      = InTokenColor;
	UpdateBodyColorFromState();
}

void ANPCUFOPawn::SetEnemyHighlighted(bool bInEnemyHighlighted)
{
	bEnemyHighlighted = bInEnemyHighlighted;
	UpdateBodyColorFromState();
}

void ANPCUFOPawn::SetSelectionRingColor(const FLinearColor& InRingColor)
{
	SelectionRingColor = InRingColor;

	for (UStaticMeshComponent* Seg : SelectionMarkerSegments)
	{
		if (!Seg) continue;

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Seg->GetMaterial(0));
		if (!MID)
		{
			UMaterialInterface* Base = Seg->GetMaterial(0);
			if (!Base)
			{
				Base = LoadObject<UMaterialInterface>(
					nullptr, UTF8_TO_TCHAR(NPCUFOPawnConstants::BasicShapeMaterialPath));
			}
			if (!Base) continue;

			MID = UMaterialInstanceDynamic::Create(Base, this);
			if (!MID) continue;
			Seg->SetMaterial(0, MID);
		}

		MID->SetVectorParameterValue(FName("Color"),        SelectionRingColor);
		MID->SetVectorParameterValue(FName("BaseColor"),    SelectionRingColor);
		MID->SetVectorParameterValue(FName("EmissiveColor"),
			SelectionRingColor * NPCUFOPawnConstants::RingEmissiveScale);
	}
}
