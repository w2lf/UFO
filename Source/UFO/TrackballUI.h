// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrackballUI.generated.h"

class AUFOPawn;

UENUM()
enum class ETrackballSide : uint8
{
	None,
	Left,
	Right,
	SpeedSlider
};

UENUM()
enum class EColorDockSide : uint8
{
	None,
	Enemy,
	Friendly
};

/**
 * UI widget to show trackball zones visually
 */
UCLASS()
class UFO_API UTrackballUI : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	EColorDockSide GetDockAssignmentForToken(int32 TokenIndex) const;
	EColorDockSide GetPreferredDockForToken(int32 TokenIndex) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackball")
	float TrackballRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackball")
	FLinearColor TrackballColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackball")
	float TrackballOpacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackball")
	float ThumbRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackball")
	float RingThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowInputDebug;

	// Slider output tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Slider")
	float SliderMinThrottle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Slider")
	float SliderMaxThrottle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Slider")
	float SliderResponseExponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Dock")
	float ColorTokenRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Dock")
	float ColorTokenOutlineThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Dock")
	float AssignedTokenRowY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Dock")
	float UnassignedTokenRowY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Dock")
	float ColorDockDeadZone;

private:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	ETrackballSide ResolveTrackballSide(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
	int32 ResolveColorTokenIndex(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
	EColorDockSide ResolveColorDockSide(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;
	FVector2D GetColorTokenPaintPosition(int32 TokenIndex, const FVector2D& WidgetSize) const;
	FVector2D ClampPointToTrackball(const FVector2D& Point, const FVector2D& Center) const;
	void UpdateSpeedFromPointer(const FVector2D& LocalPos, const FVector2D& WidgetSize);
	FReply HandlePointerPressed(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId);
	FReply HandlePointerMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId);
	FReply HandlePointerReleased(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId);
	bool IsEnemySelectedForToken(int32 TokenIndex) const;
	bool IsFriendlySelectedForToken(int32 TokenIndex) const;

	TMap<int32, ETrackballSide> ActivePointers;
	TMap<int32, FVector2D> PreviousPointerPositions;
	TArray<FLinearColor> ColorTokenColors;
	TArray<bool> ColorTokenEnemySelections;
	TArray<bool> ColorTokenFriendlySelections;
	TArray<EColorDockSide> ColorTokenLastTouchedDock;
	TMap<int32, int32> ActiveColorTokenPointers;
	TMap<int32, FVector2D> ActiveColorTokenPositions;
	TSet<int32> ActiveTouchPointerIds;
	TWeakObjectPtr<AUFOPawn> CachedPawn;
	float SpeedSliderValueNormalized = 0.0f;
};
