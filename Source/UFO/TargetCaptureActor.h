// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TargetCaptureActor.generated.h"

class USceneComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class ANPCUFOPawn;

/**
 * ATargetCaptureActor
 *
 * Invisible actor that orbits the selected NPC and feeds a live
 * 3-D render into a UTextureRenderTarget2D for display in TargetViewWidget.
 */
UCLASS()
class UFO_API ATargetCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	ATargetCaptureActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Point the capture camera at a new NPC target (pass nullptr to deactivate). */
	void SetTarget(ANPCUFOPawn* NewTarget);

	/** Returns the render target other systems should sample from. */
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** True when a live target is set. */
	bool HasValidTarget() const;

private:
	void UpdateCaptureTransform();

	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* SceneCapture;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY()
	ANPCUFOPawn* CurrentTarget;

	/** How far behind the target the camera sits (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Target View")
	float CaptureDistance = 350.0f;

	/** How high above the target origin the camera sits (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Target View")
	float CaptureHeight = 120.0f;

	/** Height offset applied to the look-at point (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Target View")
	float TargetLookHeight = 50.0f;

	/** Resolution of the square render target in pixels. */
	UPROPERTY(EditDefaultsOnly, Category = "Target View")
	int32 RenderTargetSize = 512;
};
