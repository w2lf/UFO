// Copyright Epic Games, Inc. All Rights Reserved.

#include "TargetCaptureActor.h"
#include "NPCUFOPawn.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

ATargetCaptureActor::ATargetCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	CurrentTarget = nullptr;
	RenderTarget  = nullptr;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(SceneRoot);
	SceneCapture->ProjectionType            = ECameraProjectionMode::Perspective;
	SceneCapture->FOVAngle                  = 30.0f;
	SceneCapture->bCaptureEveryFrame        = false;
	SceneCapture->bCaptureOnMovement        = false;
	SceneCapture->PrimitiveRenderMode       = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCapture->CaptureSource             = ESceneCaptureSource::SCS_FinalColorLDR;
}

void ATargetCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	// Build the render target at runtime so the size UPROPERTY is respected
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (RenderTarget)
	{
		RenderTarget->InitAutoFormat(RenderTargetSize, RenderTargetSize);
		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->UpdateResourceImmediate(true);
		SceneCapture->TextureTarget = RenderTarget;
	}

	// Start hidden/inactive until a target is assigned
	SetActorHiddenInGame(true);
}

void ATargetCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasValidTarget()) return;

	// Guard against the NPC being destroyed mid-game
	if (!IsValid(CurrentTarget))
	{
		SetTarget(nullptr);
		return;
	}

	UpdateCaptureTransform();
	SceneCapture->CaptureScene();
}

void ATargetCaptureActor::SetTarget(ANPCUFOPawn* NewTarget)
{
	CurrentTarget = NewTarget;

	if (HasValidTarget())
	{
		SetActorHiddenInGame(false);
		UpdateCaptureTransform();
		SceneCapture->CaptureScene();
	}
	else
	{
		SetActorHiddenInGame(true);
	}
}

bool ATargetCaptureActor::HasValidTarget() const
{
	return IsValid(CurrentTarget);
}

void ATargetCaptureActor::UpdateCaptureTransform()
{
	if (!HasValidTarget()) return;

	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector FocusPoint     = TargetLocation + FVector(0.0f, 0.0f, TargetLookHeight);

	// Orbit behind the NPC's forward vector
	const FVector  Forward       = CurrentTarget->GetActorForwardVector();
	const FVector  CameraLocation = TargetLocation
	                                - (Forward * CaptureDistance)
	                                + FVector(0.0f, 0.0f, CaptureHeight);
	const FRotator CameraRotation = (FocusPoint - CameraLocation).Rotation();

	SetActorLocationAndRotation(CameraLocation, CameraRotation);
}
