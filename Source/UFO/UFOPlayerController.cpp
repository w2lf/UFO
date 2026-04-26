// Copyright Epic Games, Inc. All Rights Reserved.

#include "UFOPlayerController.h"
#include "UFOPawn.h"
#include "NPCUFOPawn.h"
#include "TrackballUI.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/HitResult.h"

AUFOPlayerController::AUFOPlayerController()
{
	bReplicates = true;
	bAutoManageActiveCameraTarget = true;
	SelectedNPC = nullptr;
	MaxSelectionScreenDistance = 180.0f;
}

void AUFOPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindTouch(IE_Pressed, this, &AUFOPlayerController::OnTouchPressed);
	}
}

void AUFOPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AUFOPlayerController::BeginPlay called"));

	// Ensure touch input and UI input are enabled
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;

	// Try to get the pawn, but it might not exist yet
	// Schedule a deferred check to get the pawn once it's possessed
	GetWorld()->GetTimerManager().SetTimer(
		PawnCheckTimer,
		[this]()
		{
			UFOPawn = Cast<AUFOPawn>(GetPawn());
			if (UFOPawn)
			{
				SetViewTarget(UFOPawn);
				UE_LOG(LogTemp, Warning, TEXT("AUFOPlayerController: Pawn obtained from GetPawn()"));
				UE_LOG(LogTemp, Warning, TEXT("AUFOPlayerController: View target set to UFOPawn"));
				CreateTrackballUI();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AUFOPlayerController: Still no pawn!"));
			}
		},
		0.1f,  // Check after 0.1 seconds
		false  // Don't loop
	);
}

void AUFOPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshNPCEnemyHighlighting();
	RefreshTokenSelectedBillboards();
}

void AUFOPlayerController::CreateTrackballUI()
{
	// Create and display trackball UI
	if (GetLocalPlayer() && !TrackballWidget)
	{
		TrackballWidget = CreateWidget<UTrackballUI>(this, UTrackballUI::StaticClass());
		if (TrackballWidget)
		{
			TrackballWidget->SetVisibility(ESlateVisibility::Visible);
			TrackballWidget->AddToViewport(100);
			UE_LOG(LogTemp, Warning, TEXT("AUFOPlayerController: TrackballUI created and added to viewport"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AUFOPlayerController: Failed to create TrackballUI widget"));
		}
	}
	else if (!GetLocalPlayer())
	{
		UE_LOG(LogTemp, Error, TEXT("AUFOPlayerController: No local player"));
	}
}

void AUFOPlayerController::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	FVector2D ScreenPosition(Location.X, Location.Y);

	if (ScreenPosition.IsNearlyZero())
	{
		float ScreenX = 0.0f;
		float ScreenY = 0.0f;
		bool bIsCurrentlyPressed = false;
		GetInputTouchState(FingerIndex, ScreenX, ScreenY, bIsCurrentlyPressed);
		if (!bIsCurrentlyPressed)
		{
			return;
		}

		ScreenPosition = FVector2D(ScreenX, ScreenY);
	}

	ANPCUFOPawn* BestNPC = FindBestNPCAtScreenPosition(ScreenPosition);
	SelectNPC(BestNPC);
}

void AUFOPlayerController::SelectNPC(ANPCUFOPawn* NewSelection)
{
	if (SelectedNPC == NewSelection)
	{
		return;
	}

	SelectedNPC = NewSelection;

	if (SelectedNPC)
	{
		const EColorDockSide DockSide = GetDockSideForNPC(SelectedNPC);
		if (DockSide == EColorDockSide::Enemy)
		{
			SelectedNPC->SetSelectionRingColor(FLinearColor(1.0f, 0.22f, 0.22f, 1.0f));
			SelectedNPC->SetSelected(true);
		}
		else if (DockSide == EColorDockSide::Friendly)
		{
			SelectedNPC->SetSelectionRingColor(FLinearColor(0.1f, 1.0f, 0.2f, 1.0f));
			SelectedNPC->SetSelected(true);
		}
	}
}

ANPCUFOPawn* AUFOPlayerController::FindBestNPCAtScreenPosition(const FVector2D& ScreenPosition) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FHitResult HitResult;
	if (GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, true, HitResult))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (ANPCUFOPawn* HitNPC = Cast<ANPCUFOPawn>(HitActor))
			{
				if (IsNPCSelectableFromDock(HitNPC))
				{
					return HitNPC;
				}
			}
		}
	}

	ANPCUFOPawn* BestNPC = nullptr;
	float BestDistanceSq = MaxSelectionScreenDistance * MaxSelectionScreenDistance;

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		ANPCUFOPawn* NPC = *It;
		if (!NPC)
		{
			continue;
		}

		if (!LineOfSightTo(NPC))
		{
			continue;
		}

		if (!IsNPCSelectableFromDock(NPC))
		{
			continue;
		}

		FVector2D NPCScreen;
		if (!ProjectWorldLocationToScreen(NPC->GetActorLocation(), NPCScreen, true))
		{
			continue;
		}

		const float DistSq = FVector2D::DistSquared(ScreenPosition, NPCScreen);
		if (DistSq <= BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestNPC = NPC;
		}
	}

	return BestNPC;
}

void AUFOPlayerController::SelectNPCForToken(int32 TokenIndex, EColorDockSide PreferredDockSide)
{
	if (TokenIndex == INDEX_NONE || PreferredDockSide == EColorDockSide::None)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		ANPCUFOPawn* NPC = *It;
		if (!NPC)
		{
			continue;
		}

		if (NPC->GetColorTokenIndex() != TokenIndex)
		{
			continue;
		}

		if (GetDockSideForNPC(NPC) != PreferredDockSide)
		{
			continue;
		}

		SelectNPC(NPC);
		return;
	}
}

bool AUFOPlayerController::IsNPCSelectableFromDock(const ANPCUFOPawn* NPC) const
{
	if (!NPC || !TrackballWidget)
	{
		return true;
	}

	const int32 TokenIndex = NPC->GetColorTokenIndex();
	if (TokenIndex == INDEX_NONE)
	{
		return true;
	}

	const EColorDockSide DockSide = TrackballWidget->GetDockAssignmentForToken(TokenIndex);
	return DockSide != EColorDockSide::None;
}

EColorDockSide AUFOPlayerController::GetDockSideForNPC(const ANPCUFOPawn* NPC) const
{
	if (!NPC || !TrackballWidget)
	{
		return EColorDockSide::None;
	}

	const int32 TokenIndex = NPC->GetColorTokenIndex();
	if (TokenIndex == INDEX_NONE)
	{
		return EColorDockSide::None;
	}

	return TrackballWidget->GetPreferredDockForToken(TokenIndex);
}

void AUFOPlayerController::RefreshNPCEnemyHighlighting() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		ANPCUFOPawn* NPC = *It;
		if (!NPC)
		{
			continue;
		}

		const bool bEnemyAssigned = (GetDockSideForNPC(NPC) == EColorDockSide::Enemy);
		NPC->SetEnemyHighlighted(bEnemyAssigned);
	}
}

void AUFOPlayerController::RefreshTokenSelectedBillboards() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		ANPCUFOPawn* NPC = *It;
		if (!NPC)
		{
			continue;
		}

		const EColorDockSide DockSide = GetDockSideForNPC(NPC);
		if (DockSide == EColorDockSide::Enemy)
		{
			NPC->SetSelectionRingColor(FLinearColor(1.0f, 0.22f, 0.22f, 1.0f));
			NPC->SetSelected(true);
		}
		else if (DockSide == EColorDockSide::Friendly)
		{
			NPC->SetSelectionRingColor(FLinearColor(0.1f, 1.0f, 0.2f, 1.0f));
			NPC->SetSelected(true);
		}
		else
		{
			NPC->SetSelected(false);
		}
	}
}
