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

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace UFOControllerConstants
{
	/** Colour used for the selection ring on enemy NPCs */
	const FLinearColor EnemyRingColor    = FLinearColor(1.0f, 0.22f, 0.22f, 1.0f);

	/** Colour used for the selection ring on friendly NPCs */
	const FLinearColor FriendlyRingColor = FLinearColor(0.1f, 1.0f, 0.2f, 1.0f);

	/** Seconds after BeginPlay before we try to cache the possessed pawn */
	constexpr float PawnCheckDelay = 0.1f;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
AUFOPlayerController::AUFOPlayerController()
{
	bReplicates = true;
	bAutoManageActiveCameraTarget = true;
	SelectedNPC = nullptr;
	MaxSelectionScreenDistance = 180.0f;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

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

	// Allow both game and UI to receive input; show the cursor for touch selection
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor  = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;

	// The pawn may not be possessed yet — wait one frame then cache it
	GetWorld()->GetTimerManager().SetTimer(
		PawnCheckTimer,
		this,
		&AUFOPlayerController::OnPawnReady,
		UFOControllerConstants::PawnCheckDelay,
		false);
}

void AUFOPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshNPCEnemyHighlighting();
	RefreshTokenSelectedBillboards();
}

// ---------------------------------------------------------------------------
// Initialisation helpers
// ---------------------------------------------------------------------------

void AUFOPlayerController::OnPawnReady()
{
	UFOPawn = Cast<AUFOPawn>(GetPawn());
	if (UFOPawn)
	{
		SetViewTarget(UFOPawn);
		CreateTrackballUI();
		UE_LOG(LogTemp, Warning, TEXT("AUFOPlayerController: Pawn ready, UI created."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AUFOPlayerController: Pawn not found after delay!"));
	}
}

void AUFOPlayerController::CreateTrackballUI()
{
	if (!GetLocalPlayer() || TrackballWidget) return;

	TrackballWidget = CreateWidget<UTrackballUI>(this, UTrackballUI::StaticClass());
	if (TrackballWidget)
	{
		TrackballWidget->SetVisibility(ESlateVisibility::Visible);
		TrackballWidget->AddToViewport(100);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AUFOPlayerController: Failed to create TrackballUI."));
	}
}

// ---------------------------------------------------------------------------
// Touch / Selection
// ---------------------------------------------------------------------------

void AUFOPlayerController::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	FVector2D ScreenPosition(Location.X, Location.Y);

	// Some platforms report zero on the first event — fall back to the input state
	if (ScreenPosition.IsNearlyZero())
	{
		float ScreenX = 0.0f, ScreenY = 0.0f;
		bool  bPressed = false;
		GetInputTouchState(FingerIndex, ScreenX, ScreenY, bPressed);
		if (!bPressed) return;
		ScreenPosition = FVector2D(ScreenX, ScreenY);
	}

	SelectNPC(FindBestNPCAtScreenPosition(ScreenPosition));
}

void AUFOPlayerController::SelectNPC(ANPCUFOPawn* NewSelection)
{
	if (SelectedNPC == NewSelection) return;
	SelectedNPC = NewSelection;

	if (SelectedNPC)
	{
		ApplyNPCSelectionVisuals(SelectedNPC, GetDockSideForNPC(SelectedNPC));
	}
}

void AUFOPlayerController::SelectNPCForToken(int32 TokenIndex, EColorDockSide PreferredDockSide)
{
	if (TokenIndex == INDEX_NONE || PreferredDockSide == EColorDockSide::None) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		ANPCUFOPawn* NPC = *It;
		if (NPC
			&& NPC->GetColorTokenIndex() == TokenIndex
			&& GetDockSideForNPC(NPC) == PreferredDockSide)
		{
			SelectNPC(NPC);
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// NPC finding
// ---------------------------------------------------------------------------

ANPCUFOPawn* AUFOPlayerController::FindBestNPCAtScreenPosition(const FVector2D& ScreenPosition) const
{
	// 1. Try a direct raycast first (most accurate)
	if (ANPCUFOPawn* Hit = RaycastNPCAtScreen(ScreenPosition))
	{
		return Hit;
	}

	// 2. Fall back to the nearest projected NPC within the distance threshold
	return FindNearestNPCOnScreen(ScreenPosition);
}

ANPCUFOPawn* AUFOPlayerController::RaycastNPCAtScreen(const FVector2D& ScreenPosition) const
{
	FHitResult HitResult;
	if (!GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, true, HitResult))
	{
		return nullptr;
	}

	ANPCUFOPawn* HitNPC = Cast<ANPCUFOPawn>(HitResult.GetActor());
	if (HitNPC && IsNPCSelectableFromDock(HitNPC))
	{
		return HitNPC;
	}
	return nullptr;
}

ANPCUFOPawn* AUFOPlayerController::FindNearestNPCOnScreen(const FVector2D& ScreenPosition) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	ANPCUFOPawn* BestNPC    = nullptr;
	float        BestDistSq = MaxSelectionScreenDistance * MaxSelectionScreenDistance;

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		ANPCUFOPawn* NPC = *It;
		if (!NPC || !LineOfSightTo(NPC) || !IsNPCSelectableFromDock(NPC)) continue;

		FVector2D NPCScreenPos;
		if (!ProjectWorldLocationToScreen(NPC->GetActorLocation(), NPCScreenPos, true)) continue;

		const float DistSq = FVector2D::DistSquared(ScreenPosition, NPCScreenPos);
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestNPC    = NPC;
		}
	}
	return BestNPC;
}

// ---------------------------------------------------------------------------
// NPC visual helpers
// ---------------------------------------------------------------------------

void AUFOPlayerController::ApplyNPCSelectionVisuals(ANPCUFOPawn* NPC, EColorDockSide DockSide) const
{
	if (!NPC) return;

	if (DockSide == EColorDockSide::Enemy)
	{
		NPC->SetSelectionRingColor(UFOControllerConstants::EnemyRingColor);
		NPC->SetSelected(true);
	}
	else if (DockSide == EColorDockSide::Friendly)
	{
		NPC->SetSelectionRingColor(UFOControllerConstants::FriendlyRingColor);
		NPC->SetSelected(true);
	}
	else
	{
		NPC->SetSelected(false);
	}
}

bool AUFOPlayerController::IsNPCSelectableFromDock(const ANPCUFOPawn* NPC) const
{
	if (!NPC || !TrackballWidget) return true;

	const int32 TokenIndex = NPC->GetColorTokenIndex();
	if (TokenIndex == INDEX_NONE) return true;

	return TrackballWidget->GetDockAssignmentForToken(TokenIndex) != EColorDockSide::None;
}

EColorDockSide AUFOPlayerController::GetDockSideForNPC(const ANPCUFOPawn* NPC) const
{
	if (!NPC || !TrackballWidget) return EColorDockSide::None;

	const int32 TokenIndex = NPC->GetColorTokenIndex();
	if (TokenIndex == INDEX_NONE) return EColorDockSide::None;

	return TrackballWidget->GetPreferredDockForToken(TokenIndex);
}

void AUFOPlayerController::RefreshNPCEnemyHighlighting() const
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		if (ANPCUFOPawn* NPC = *It)
		{
			NPC->SetEnemyHighlighted(GetDockSideForNPC(NPC) == EColorDockSide::Enemy);
		}
	}
}

void AUFOPlayerController::RefreshTokenSelectedBillboards() const
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<ANPCUFOPawn> It(World); It; ++It)
	{
		if (ANPCUFOPawn* NPC = *It)
		{
			ApplyNPCSelectionVisuals(NPC, GetDockSideForNPC(NPC));
		}
	}
}
