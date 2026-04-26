// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrackballUI.h"
#include "UFOPawn.h"
#include "UFOPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/CoreStyle.h"
#include "Slate/SlateBrushAsset.h"
#include "Slate/WidgetTransform.h"

void UTrackballUI::NativeConstruct()
{
	Super::NativeConstruct();

	TrackballRadius = 160.0f;  // 2x larger for mobile visibility
	TrackballColor = FLinearColor(0.0f, 1.0f, 1.0f, 0.3f); // Cyan, semi-transparent
	TrackballOpacity = 0.3f;
	ThumbRadius = 40.0f;  // 2x larger
	RingThickness = 2.5f;
	bShowInputDebug = false;
	SliderMinThrottle = 0.0f;
	SliderMaxThrottle = 1.0f;
	SliderResponseExponent = 1.6f;
	ColorTokenRadius = 55.0f;
	ColorTokenOutlineThickness = 11.0f;
	AssignedTokenRowY = 0.66f;
	UnassignedTokenRowY = 0.82f;
	ColorDockDeadZone = 0.08f;
	ColorTokenColors = {
		FLinearColor(0.24f, 0.85f, 0.38f, 1.0f),
		FLinearColor(1.0f, 0.58f, 0.16f, 1.0f),
		FLinearColor(0.26f, 0.65f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.56f, 0.62f, 1.0f),
		FLinearColor(0.83f, 0.52f, 0.13f, 1.0f),
		FLinearColor(0.74f, 0.68f, 0.64f, 1.0f),
		FLinearColor(0.73f, 0.57f, 1.0f, 1.0f)
	};
	ColorTokenEnemySelections.Init(false, ColorTokenColors.Num());
	ColorTokenFriendlySelections.Init(false, ColorTokenColors.Num());
	ColorTokenLastTouchedDock.Init(EColorDockSide::None, ColorTokenColors.Num());

	SetIsFocusable(true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		CachedPawn = Cast<AUFOPawn>(PC->GetPawn());
	}
}

void UTrackballUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

int32 UTrackballUI::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	
	if (WidgetSize.X <= 0 || WidgetSize.Y <= 0)
	{
		return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	// Calculate responsive scale factor based on smallest screen dimension (scales to device size)
	const float BaseScreenSize = 1080.0f;  // Base reference size for scaling
	const float ScreenScale = FMath::Min(WidgetSize.X, WidgetSize.Y) / BaseScreenSize;
	const float ScaledTrackballRadius = TrackballRadius * ScreenScale;
	const float ScaledThumbRadius = ThumbRadius * ScreenScale;
	const float ScaledColorTokenRadius = ColorTokenRadius * ScreenScale;
	const float ScaledColorTokenOutlineThickness = ColorTokenOutlineThickness * ScreenScale;

	const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 14);
	const FLinearColor FillColor(TrackballColor.R, TrackballColor.G, TrackballColor.B, TrackballOpacity);
	const FLinearColor RingColor(TrackballColor.R, TrackballColor.G, TrackballColor.B, 0.9f);
	const FLinearColor InnerRingColor(TrackballColor.R, TrackballColor.G, TrackballColor.B, 0.35f);
	const FLinearColor ThumbColor(0.85f, 0.95f, 1.0f, 0.95f);
	const FLinearColor ThumbHighlight(1.0f, 1.0f, 1.0f, 0.75f);
	const FLinearColor SliderTrackColor(0.1f, 0.75f, 0.95f, 0.3f);
	const FLinearColor SliderFillColor(0.2f, 0.95f, 1.0f, 0.75f);
	const FLinearColor SliderThumbColor(0.95f, 1.0f, 1.0f, 0.95f);

	// Position trackballs at left and right edges of screen relative to size
	FVector2D LeftCenter(WidgetSize.X * 0.15f, WidgetSize.Y * 0.8f);
	FVector2D RightCenter(WidgetSize.X * 0.85f, WidgetSize.Y * 0.8f);
	FVector2D LeftThumbPos = LeftCenter;
	FVector2D RightThumbPos = RightCenter;
	const float SliderX = WidgetSize.X * 0.93f;
	const float SliderTop = WidgetSize.Y * 0.2f;
	const float SliderBottom = WidgetSize.Y * 0.85f;
	const float SliderWidth = 52.0f * ScreenScale;  // 2x larger, responsive to screen
	const float SliderHeight = SliderBottom - SliderTop;

	float SliderValue = SpeedSliderValueNormalized;
	if (CachedPawn.IsValid())
	{
		SliderValue = CachedPawn->GetThrottleNormalized();
	}
	const float SliderThumbY = FMath::Lerp(SliderBottom, SliderTop, FMath::Clamp(SliderValue, 0.0f, 1.0f));

	for (const TPair<int32, ETrackballSide>& Pair : ActivePointers)
	{
		const FVector2D* PointerPos = PreviousPointerPositions.Find(Pair.Key);
		if (!PointerPos)
		{
			continue;
		}

		if (Pair.Value == ETrackballSide::Left)
		{
			LeftThumbPos = ClampPointToTrackball(*PointerPos, LeftCenter);
		}
		else if (Pair.Value == ETrackballSide::Right)
		{
			RightThumbPos = ClampPointToTrackball(*PointerPos, RightCenter);
		}
	}

	auto DrawTrackballRing = [&](const FVector2D& Center, float Radius, int32 DrawLayer)
	{
		const float ScaledRadius = Radius * ScreenScale;
		TArray<FVector2f> RingPoints;
		constexpr int32 Segments = 48;
		RingPoints.Reserve(Segments + 1);

		for (int32 i = 0; i <= Segments; ++i)
		{
			const float T = (2.0f * PI * i) / Segments;
			const FVector2D P = Center + FVector2D(FMath::Cos(T) * ScaledRadius, FMath::Sin(T) * ScaledRadius);
			RingPoints.Add(FVector2f(P));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(),
			RingPoints,
			ESlateDrawEffect::None,
			RingColor,
			true,
			RingThickness
		);

		TArray<FVector2f> InnerRingPoints;
		InnerRingPoints.Reserve(Segments + 1);
		const float InnerRadius = ScaledRadius * 0.55f;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float T = (2.0f * PI * i) / Segments;
			const FVector2D P = Center + FVector2D(FMath::Cos(T) * InnerRadius, FMath::Sin(T) * InnerRadius);
			InnerRingPoints.Add(FVector2f(P));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(),
			InnerRingPoints,
			ESlateDrawEffect::None,
			InnerRingColor,
			true,
			1.5f
		);

		TArray<FVector2f> CoreRingPoints;
		CoreRingPoints.Reserve(Segments + 1);
		const float CoreRadius = ScaledRadius * 0.25f;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float T = (2.0f * PI * i) / Segments;
			const FVector2D P = Center + FVector2D(FMath::Cos(T) * CoreRadius, FMath::Sin(T) * CoreRadius);
			CoreRingPoints.Add(FVector2f(P));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(),
			CoreRingPoints,
			ESlateDrawEffect::None,
			FillColor,
			true,
			2.0f
		);
	};

	auto DrawThumb = [&](const FVector2D& ThumbPos, int32 DrawLayer)
	{
		TArray<FVector2f> ThumbOuter;
		TArray<FVector2f> ThumbInner;
		constexpr int32 Segments = 36;
		ThumbOuter.Reserve(Segments + 1);
		ThumbInner.Reserve(Segments + 1);

		for (int32 i = 0; i <= Segments; ++i)
		{
			const float T = (2.0f * PI * i) / Segments;
			ThumbOuter.Add(FVector2f(ThumbPos + FVector2D(FMath::Cos(T) * ScaledThumbRadius, FMath::Sin(T) * ScaledThumbRadius)));
			ThumbInner.Add(FVector2f(ThumbPos + FVector2D(FMath::Cos(T) * (ScaledThumbRadius * 0.55f), FMath::Sin(T) * (ScaledThumbRadius * 0.55f))));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(),
			ThumbOuter,
			ESlateDrawEffect::None,
			ThumbColor,
			true,
			2.5f
		);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(),
			ThumbInner,
			ESlateDrawEffect::None,
			FLinearColor(ThumbColor.R, ThumbColor.G, ThumbColor.B, 0.55f),
			true,
			2.0f
		);

		const FVector2D HighlightCenter = ThumbPos + FVector2D(ScaledThumbRadius * 0.25f, -ScaledThumbRadius * 0.2f);
		TArray<FVector2f> HighlightRing;
		HighlightRing.Reserve(Segments + 1);
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float T = (2.0f * PI * i) / Segments;
			HighlightRing.Add(FVector2f(HighlightCenter + FVector2D(FMath::Cos(T) * (ScaledThumbRadius * 0.18f), FMath::Sin(T) * (ScaledThumbRadius * 0.18f))));
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			HighlightRing,
			ESlateDrawEffect::None,
			ThumbHighlight,
			true,
			1.5f
		);
	};

	// Draw left trackball ring (bottom-left) - positioned at left screen edge
	DrawTrackballRing(LeftCenter, ScaledTrackballRadius, LayerId + 1);
	DrawThumb(LeftThumbPos, LayerId + 2);

	// Draw right trackball ring (bottom-right) - positioned at right screen edge
	DrawTrackballRing(RightCenter, ScaledTrackballRadius, LayerId + 1);
	DrawThumb(RightThumbPos, LayerId + 2);

	const float DividerX = WidgetSize.X * 0.5f;
	const float DockCenterY = WidgetSize.Y * AssignedTokenRowY;
	const FLinearColor DividerColor(0.72f, 0.58f, 1.0f, 0.75f);
	const FLinearColor EnemyOutline(1.0f, 0.36f, 0.36f, 1.0f);
	const FLinearColor FriendlyOutline(0.18f, 1.0f, 0.34f, 1.0f);

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		{
			FVector2f(FVector2D(DividerX, WidgetSize.Y * 0.44f)),
			FVector2f(FVector2D(DividerX, WidgetSize.Y * 0.90f))
		},
		ESlateDrawEffect::None,
		DividerColor,
		true,
		2.0f
	);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(120.0f, 20.0f)),
			FSlateLayoutTransform(FVector2f(DividerX - 170.0f, DockCenterY - 118.0f))
		),
		TEXT("Enemy Zone"),
		FontInfo,
		ESlateDrawEffect::None,
		DividerColor
	);

	int32 EnemyCount = 0;
	int32 FriendlyCount = 0;
	for (int32 TokenIndex = 0; TokenIndex < ColorTokenColors.Num(); ++TokenIndex)
	{
		if (IsEnemySelectedForToken(TokenIndex))
		{
			++EnemyCount;
		}

		if (IsFriendlySelectedForToken(TokenIndex))
		{
			++FriendlyCount;
		}
	}

	const FString EnemyCountText = FString::Printf(TEXT("Enemy %d"), EnemyCount);
	const FString FriendlyCountText = FString::Printf(TEXT("Friendly %d"), FriendlyCount);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(120.0f, 18.0f)),
			FSlateLayoutTransform(FVector2f(DividerX - 170.0f, DockCenterY - 98.0f))
		),
		EnemyCountText,
		FontInfo,
		ESlateDrawEffect::None,
		FLinearColor(1.0f, 0.36f, 0.36f, 0.95f)
	);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(130.0f, 18.0f)),
			FSlateLayoutTransform(FVector2f(DividerX + 28.0f, DockCenterY - 98.0f))
		),
		FriendlyCountText,
		FontInfo,
		ESlateDrawEffect::None,
		FLinearColor(0.18f, 1.0f, 0.34f, 0.95f)
	);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(140.0f, 20.0f)),
			FSlateLayoutTransform(FVector2f(DividerX + 28.0f, DockCenterY - 118.0f))
		),
		TEXT("Friendly Zone"),
		FontInfo,
		ESlateDrawEffect::None,
		DividerColor
	);

	auto DrawColorToken = [&](const FVector2D& Center, const FLinearColor& Fill, bool bEnemySelected, bool bFriendlySelected, int32 DrawLayer)
	{
		TArray<FVector2f> OuterPoints;
		TArray<FVector2f> TokenRingPoints;
		TArray<FVector2f> FriendlyInnerPoints;
		constexpr int32 Segments = 36;
		OuterPoints.Reserve(Segments + 1);
		TokenRingPoints.Reserve(Segments + 1);
		FriendlyInnerPoints.Reserve(Segments + 1);

		for (int32 i = 0; i <= Segments; ++i)
		{
			const float T = (2.0f * PI * i) / Segments;
			const FVector2D Dir(FMath::Cos(T), FMath::Sin(T));
			OuterPoints.Add(FVector2f(Center + Dir * ScaledColorTokenRadius));
			TokenRingPoints.Add(FVector2f(Center + Dir * (ScaledColorTokenRadius * 0.76f)));
			FriendlyInnerPoints.Add(FVector2f(Center + Dir * (ScaledColorTokenRadius * 0.54f)));
		}

		if (bEnemySelected)
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				DrawLayer,
				AllottedGeometry.ToPaintGeometry(),
				OuterPoints,
				ESlateDrawEffect::None,
				EnemyOutline,
				true,
				ScaledColorTokenOutlineThickness
			);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			DrawLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			TokenRingPoints,
			ESlateDrawEffect::None,
			FLinearColor(Fill.R, Fill.G, Fill.B, 0.98f),
			true,
			5.6f
		);

		if (bFriendlySelected)
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				DrawLayer + 2,
				AllottedGeometry.ToPaintGeometry(),
				FriendlyInnerPoints,
				ESlateDrawEffect::None,
				FriendlyOutline,
				true,
				ScaledColorTokenOutlineThickness
			);
		}
	};

	for (int32 TokenIndex = 0; TokenIndex < ColorTokenColors.Num(); ++TokenIndex)
	{
		const FVector2D TokenCenter = GetColorTokenPaintPosition(TokenIndex, WidgetSize);

		bool bEnemySelected = IsEnemySelectedForToken(TokenIndex);
		bool bFriendlySelected = IsFriendlySelectedForToken(TokenIndex);

		for (const TPair<int32, int32>& Pair : ActiveColorTokenPointers)
		{
			if (Pair.Value == TokenIndex)
			{
				if (const FVector2D* DragPos = ActiveColorTokenPositions.Find(Pair.Key))
				{
					const EColorDockSide HoveredDock = ResolveColorDockSide(*DragPos, WidgetSize);
					if (HoveredDock == EColorDockSide::Enemy)
					{
						bEnemySelected = !bEnemySelected;
					}
					else if (HoveredDock == EColorDockSide::Friendly)
					{
						bFriendlySelected = !bFriendlySelected;
					}
				}

				break;
			}
		}

		DrawColorToken(TokenCenter, ColorTokenColors[TokenIndex], bEnemySelected, bFriendlySelected, LayerId + 2);
	}

	// Draw right-side speed slider (touch + mouse).
	const FVector2D SliderTopLeft(SliderX - SliderWidth * 0.5f, SliderTop);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(SliderWidth, SliderHeight),
			FSlateLayoutTransform(FVector2f(SliderTopLeft))
		),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		SliderTrackColor
	);

	const float FillHeight = SliderBottom - SliderThumbY;
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(SliderWidth, FillHeight),
			FSlateLayoutTransform(FVector2f(SliderTopLeft.X, SliderThumbY))
		),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		SliderFillColor
	);

	const FVector2D SliderThumbCenter(SliderX, SliderThumbY);
	TArray<FVector2f> SliderThumbRing;
	constexpr int32 SliderThumbSegments = 28;
	const float SliderThumbRadius = 16.0f;
	SliderThumbRing.Reserve(SliderThumbSegments + 1);
	for (int32 i = 0; i <= SliderThumbSegments; ++i)
	{
		const float T = (2.0f * PI * i) / SliderThumbSegments;
		SliderThumbRing.Add(FVector2f(SliderThumbCenter + FVector2D(FMath::Cos(T) * SliderThumbRadius, FMath::Sin(T) * SliderThumbRadius)));
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 3,
		AllottedGeometry.ToPaintGeometry(),
		SliderThumbRing,
		ESlateDrawEffect::None,
		SliderThumbColor,
		true,
		2.0f
	);

	// Draw labels
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(60.0f, 16.0f)),
			FSlateLayoutTransform(FVector2f(LeftCenter - FVector2D(30.0f, 8.0f)))
		),
		TEXT("Camera"),
		FontInfo,
		ESlateDrawEffect::None,
		FLinearColor::White
	);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(56.0f, 16.0f)),
			FSlateLayoutTransform(FVector2f(FVector2D(SliderX - 28.0f, SliderTop - 22.0f)))
		),
		TEXT("Speed"),
		FontInfo,
		ESlateDrawEffect::None,
		FLinearColor::White
	);

	const FString SpeedPercentText = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(SliderValue * 100.0f));
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(56.0f, 16.0f)),
			FSlateLayoutTransform(FVector2f(FVector2D(SliderX - 28.0f, SliderBottom + 10.0f)))
		),
		SpeedPercentText,
		FontInfo,
		ESlateDrawEffect::None,
		FLinearColor::White
	);

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 4,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(FVector2D(40.0f, 16.0f)),
			FSlateLayoutTransform(FVector2f(RightCenter - FVector2D(20.0f, 8.0f)))
		),
		TEXT("Ship"),
		FontInfo,
		ESlateDrawEffect::None,
		FLinearColor::White
	);

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
}

FVector2D UTrackballUI::ClampPointToTrackball(const FVector2D& Point, const FVector2D& Center) const
{
	// Calculate responsive scale based on smallest screen dimension
	const float BaseScreenSize = 1080.0f;
	const float ScreenScale = FMath::Min(Center.X * 2.0f, Center.Y * 2.0f) / BaseScreenSize;
	const float ScaledRadius = TrackballRadius * ScreenScale;

	const FVector2D Delta = Point - Center;
	const float Dist = Delta.Length();
	if (Dist <= ScaledRadius || Dist <= KINDA_SMALL_NUMBER)
	{
		return Point;
	}

	return Center + (Delta / Dist) * ScaledRadius;
}

ETrackballSide UTrackballUI::ResolveTrackballSide(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	// Calculate responsive scale based on smallest screen dimension
	const float BaseScreenSize = 1080.0f;
	const float ScreenScale = FMath::Min(WidgetSize.X, WidgetSize.Y) / BaseScreenSize;
	const float ScaledTrackballRadius = TrackballRadius * ScreenScale;

	const FVector2D LeftCenter(WidgetSize.X * 0.15f, WidgetSize.Y * 0.8f);
	const FVector2D RightCenter(WidgetSize.X * 0.85f, WidgetSize.Y * 0.8f);
	const float SliderX = WidgetSize.X * 0.93f;
	const float SliderTop = WidgetSize.Y * 0.2f;
	const float SliderBottom = WidgetSize.Y * 0.85f;
	const float SliderWidth = 52.0f * ScreenScale;  // 2x larger, responsive

	if (LocalPos.X >= (SliderX - SliderWidth * 0.5f) &&
		LocalPos.X <= (SliderX + SliderWidth * 0.5f) &&
		LocalPos.Y >= SliderTop &&
		LocalPos.Y <= SliderBottom)
	{
		return ETrackballSide::SpeedSlider;
	}

	if ((LocalPos - LeftCenter).SizeSquared() <= FMath::Square(ScaledTrackballRadius))
	{
		return ETrackballSide::Left;
	}

	if ((LocalPos - RightCenter).SizeSquared() <= FMath::Square(ScaledTrackballRadius))
	{
		return ETrackballSide::Right;
	}

	return ETrackballSide::None;
}

int32 UTrackballUI::ResolveColorTokenIndex(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	// Calculate responsive scale based on smallest screen dimension
	const float BaseScreenSize = 1080.0f;
	const float ScreenScale = FMath::Min(WidgetSize.X, WidgetSize.Y) / BaseScreenSize;
	const float ScaledColorTokenRadius = ColorTokenRadius * ScreenScale;

	for (int32 TokenIndex = 0; TokenIndex < ColorTokenColors.Num(); ++TokenIndex)
	{
		const FVector2D TokenCenter = GetColorTokenPaintPosition(TokenIndex, WidgetSize);
		if ((LocalPos - TokenCenter).SizeSquared() <= FMath::Square(ScaledColorTokenRadius * 1.25f))
		{
			return TokenIndex;
		}
	}

	return INDEX_NONE;
}

EColorDockSide UTrackballUI::ResolveColorDockSide(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	const float LeftThreshold = WidgetSize.X * (0.5f - ColorDockDeadZone);
	const float RightThreshold = WidgetSize.X * (0.5f + ColorDockDeadZone);

	if (LocalPos.X < LeftThreshold)
	{
		return EColorDockSide::Enemy;
	}

	if (LocalPos.X > RightThreshold)
	{
		return EColorDockSide::Friendly;
	}

	return EColorDockSide::None;
}

FVector2D UTrackballUI::GetColorTokenPaintPosition(int32 TokenIndex, const FVector2D& WidgetSize) const
{
	for (const TPair<int32, int32>& Pair : ActiveColorTokenPointers)
	{
		if (Pair.Value == TokenIndex)
		{
			if (const FVector2D* DragPos = ActiveColorTokenPositions.Find(Pair.Key))
			{
				return *DragPos;
			}
		}
	}

	if (ColorTokenColors.IsValidIndex(TokenIndex))
	{
		// Calculate responsive scale
		const float BaseScreenSize = 1080.0f;
		const float ScreenScale = FMath::Min(WidgetSize.X, WidgetSize.Y) / BaseScreenSize;
		const float ScaledTrackballRadius = TrackballRadius * ScreenScale;
		const float ScaledTokenRadius = ColorTokenRadius * ScreenScale;

		// Trackball exclusion zone - keep buttons away from trackball safe areas
		const float ExclusionZoneRadius = ScaledTrackballRadius * 1.4f;  // 40% larger than trackball for safe buffer
		const FVector2D LeftCenter(WidgetSize.X * 0.15f, WidgetSize.Y * 0.8f);
		const FVector2D RightCenter(WidgetSize.X * 0.85f, WidgetSize.Y * 0.8f);

		const float Spacing = ColorTokenRadius * 2.45f;
		const float TotalWidth = FMath::Max(0.0f, (ColorTokenColors.Num() - 1) * Spacing);
		const float StartX = (WidgetSize.X * 0.5f) - (TotalWidth * 0.5f);
		float RowY = WidgetSize.Y * UnassignedTokenRowY;
		FVector2D TokenPos = FVector2D(StartX + (TokenIndex * Spacing), RowY);

		// Check if button would collide with left or right trackball exclusion zones
		const float DistToLeftTrackball = FVector2D::Distance(TokenPos, LeftCenter);
		const float DistToRightTrackball = FVector2D::Distance(TokenPos, RightCenter);

		// If button is too close to either trackball, push it away
		if (DistToLeftTrackball < ExclusionZoneRadius + ScaledTokenRadius)
		{
			// Push button to the right, away from left trackball
			const float PushDistance = ExclusionZoneRadius + ScaledTokenRadius - DistToLeftTrackball + 20.0f;
			TokenPos.X += PushDistance;
		}
		else if (DistToRightTrackball < ExclusionZoneRadius + ScaledTokenRadius)
		{
			// Push button to the left, away from right trackball
			const float PushDistance = ExclusionZoneRadius + ScaledTokenRadius - DistToRightTrackball + 20.0f;
			TokenPos.X -= PushDistance;
		}

		return TokenPos;
	}

	return FVector2D::ZeroVector;
}

bool UTrackballUI::IsEnemySelectedForToken(int32 TokenIndex) const
{
	return ColorTokenEnemySelections.IsValidIndex(TokenIndex) ? ColorTokenEnemySelections[TokenIndex] : false;
}

bool UTrackballUI::IsFriendlySelectedForToken(int32 TokenIndex) const
{
	return ColorTokenFriendlySelections.IsValidIndex(TokenIndex) ? ColorTokenFriendlySelections[TokenIndex] : false;
}

void UTrackballUI::UpdateSpeedFromPointer(const FVector2D& LocalPos, const FVector2D& WidgetSize)
{
	const float SliderTop = WidgetSize.Y * 0.2f;
	const float SliderBottom = WidgetSize.Y * 0.85f;
	const float ClampedY = FMath::Clamp(LocalPos.Y, SliderTop, SliderBottom);

	// Top is max speed, bottom is zero speed.
	const float RawNormalized = 1.0f - ((ClampedY - SliderTop) / FMath::Max(1.0f, SliderBottom - SliderTop));
	const float ClampedRaw = FMath::Clamp(RawNormalized, 0.0f, 1.0f);

	const float MinOut = FMath::Clamp(FMath::Min(SliderMinThrottle, SliderMaxThrottle), 0.0f, 1.0f);
	const float MaxOut = FMath::Clamp(FMath::Max(SliderMinThrottle, SliderMaxThrottle), 0.0f, 1.0f);
	const float Expo = FMath::Max(0.01f, SliderResponseExponent);
	const float Shaped = FMath::Pow(ClampedRaw, Expo);

	SpeedSliderValueNormalized = FMath::Lerp(MinOut, MaxOut, Shaped);

	if (!CachedPawn.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			CachedPawn = Cast<AUFOPawn>(PC->GetPawn());
		}
	}

	if (CachedPawn.IsValid())
	{
		CachedPawn->SetThrottleNormalized(SpeedSliderValueNormalized);
	}
}

FReply UTrackballUI::HandlePointerPressed(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId)
{
	const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
	const int32 ColorTokenIndex = ResolveColorTokenIndex(LocalPos, InGeometry.GetLocalSize());
	if (ColorTokenIndex != INDEX_NONE)
	{
		ActiveColorTokenPointers.Add(PointerId, ColorTokenIndex);
		ActiveColorTokenPositions.Add(PointerId, LocalPos);
		return FReply::Handled();
	}

	const ETrackballSide Side = ResolveTrackballSide(LocalPos, InGeometry.GetLocalSize());

	if (Side != ETrackballSide::None)
	{
		ActivePointers.Add(PointerId, Side);
		PreviousPointerPositions.Add(PointerId, LocalPos);

		if (Side == ETrackballSide::SpeedSlider)
		{
			UpdateSpeedFromPointer(LocalPos, InGeometry.GetLocalSize());
		}

		if (bShowInputDebug && GEngine)
		{
			const FString SideLabel = (Side == ETrackballSide::Left) ? TEXT("LEFT") : TEXT("RIGHT");
			GEngine->AddOnScreenDebugMessage(
				500 + PointerId,
				1.5f,
				FColor::Green,
				FString::Printf(TEXT("Trackball Pointer Start: %s (Pointer %d)"), *SideLabel, PointerId)
			);
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply UTrackballUI::HandlePointerMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId)
{
	if (ActiveColorTokenPointers.Contains(PointerId))
	{
		ActiveColorTokenPositions.FindOrAdd(PointerId) = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	const ETrackballSide* Side = ActivePointers.Find(PointerId);
	FVector2D* PrevPos = PreviousPointerPositions.Find(PointerId);

	if (!Side || !PrevPos)
	{
		return FReply::Unhandled();
	}

	const FVector2D CurrentPos = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());

	// Add temporal smoothing to eliminate trackball jitter
	// Average current position with previous position for smoother motion
	FVector2D SmoothedPos = CurrentPos;
	if (PrevPos->SizeSquared() > 0)
	{
		// Blend: 60% current, 40% previous for smooth but responsive feel
		SmoothedPos = (CurrentPos * 0.6f) + (*PrevPos * 0.4f);
	}

	// Check minimum movement threshold to filter micro-movements/noise
	const FVector2D Delta = SmoothedPos - *PrevPos;
	const float MovementThreshold = 1.0f;  // 1-pixel threshold for noise filtering
	if (Delta.SizeSquared() < FMath::Square(MovementThreshold))
	{
		// Movement too small, likely noise - skip this update
		return FReply::Handled();
	}

	if (!CachedPawn.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			CachedPawn = Cast<AUFOPawn>(PC->GetPawn());
		}
	}

	if (CachedPawn.IsValid())
	{
		if (*Side == ETrackballSide::Left)
		{
			CachedPawn->ApplyLeftTrackballDrag(*PrevPos, SmoothedPos);
			if (bShowInputDebug && GEngine)
			{
				const FVector2D DebugDelta = CurrentPos - *PrevPos;
				GEngine->AddOnScreenDebugMessage(
					600 + PointerId,
					0.05f,
					FColor::Cyan,
					FString::Printf(TEXT("LEFT Drag dX=%.1f dY=%.1f"), DebugDelta.X, DebugDelta.Y)
				);
			}
		}
		else if (*Side == ETrackballSide::Right)
		{
			CachedPawn->ApplyRightTrackballDrag(*PrevPos, SmoothedPos);
			if (bShowInputDebug && GEngine)
			{
				const FVector2D DebugDelta = CurrentPos - *PrevPos;
				GEngine->AddOnScreenDebugMessage(
					700 + PointerId,
					0.05f,
					FColor::Yellow,
					FString::Printf(TEXT("RIGHT Drag dX=%.1f dY=%.1f"), DebugDelta.X, DebugDelta.Y)
				);
			}
		}
		else if (*Side == ETrackballSide::SpeedSlider)
		{
			UpdateSpeedFromPointer(SmoothedPos, InGeometry.GetLocalSize());
		}
	}
	else if (*Side == ETrackballSide::SpeedSlider)
	{
		UpdateSpeedFromPointer(SmoothedPos, InGeometry.GetLocalSize());
	}

	*PrevPos = SmoothedPos;
	return FReply::Handled();
}

FReply UTrackballUI::HandlePointerReleased(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId)
{
	if (const int32* TokenIndex = ActiveColorTokenPointers.Find(PointerId))
	{
		const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
		const EColorDockSide DockSide = ResolveColorDockSide(LocalPos, InGeometry.GetLocalSize());
		bool bSelectionStateChanged = false;
		if (DockSide == EColorDockSide::Enemy && ColorTokenEnemySelections.IsValidIndex(*TokenIndex))
		{
			ColorTokenEnemySelections[*TokenIndex] = !ColorTokenEnemySelections[*TokenIndex];
			bSelectionStateChanged = true;
			if (ColorTokenLastTouchedDock.IsValidIndex(*TokenIndex))
			{
				ColorTokenLastTouchedDock[*TokenIndex] = EColorDockSide::Enemy;
			}
		}
		else if (DockSide == EColorDockSide::Friendly && ColorTokenFriendlySelections.IsValidIndex(*TokenIndex))
		{
			ColorTokenFriendlySelections[*TokenIndex] = !ColorTokenFriendlySelections[*TokenIndex];
			bSelectionStateChanged = true;
			if (ColorTokenLastTouchedDock.IsValidIndex(*TokenIndex))
			{
				ColorTokenLastTouchedDock[*TokenIndex] = EColorDockSide::Friendly;
			}
		}

		if (bSelectionStateChanged)
		{
			if (AUFOPlayerController* PC = Cast<AUFOPlayerController>(GetOwningPlayer()))
			{
				PC->SelectNPCForToken(*TokenIndex, GetPreferredDockForToken(*TokenIndex));
			}
		}

		ActiveColorTokenPointers.Remove(PointerId);
		ActiveColorTokenPositions.Remove(PointerId);
		return FReply::Handled();
	}

	if (bShowInputDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			800 + PointerId,
			1.0f,
			FColor::Silver,
			FString::Printf(TEXT("Trackball Pointer End (Pointer %d)"), PointerId)
		);
	}

	ActivePointers.Remove(PointerId);
	PreviousPointerPositions.Remove(PointerId);
	return FReply::Handled();
}

FReply UTrackballUI::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ActiveTouchPointerIds.Num() > 0 || InMouseEvent.IsTouchEvent())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	FReply Reply = HandlePointerPressed(InGeometry, InMouseEvent, 0);
	if (Reply.IsEventHandled())
	{
		return Reply.CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UTrackballUI::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ActiveTouchPointerIds.Num() > 0 || InMouseEvent.IsTouchEvent())
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	FReply Reply = HandlePointerMoved(InGeometry, InMouseEvent, 0);
	if (Reply.IsEventHandled())
	{
		return Reply;
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UTrackballUI::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ActiveTouchPointerIds.Num() > 0 || InMouseEvent.IsTouchEvent())
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (ActivePointers.Contains(0))
	{
		return HandlePointerReleased(InGeometry, InMouseEvent, 0).ReleaseMouseCapture();
	}

	if (ActiveColorTokenPointers.Contains(0))
	{
		return HandlePointerReleased(InGeometry, InMouseEvent, 0).ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UTrackballUI::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const int32 TouchPointerId = 1000 + InGestureEvent.GetPointerIndex();
	ActiveTouchPointerIds.Add(TouchPointerId);
	const FReply Reply = HandlePointerPressed(InGeometry, InGestureEvent, TouchPointerId);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

FReply UTrackballUI::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const int32 TouchPointerId = 1000 + InGestureEvent.GetPointerIndex();
	const FReply Reply = HandlePointerMoved(InGeometry, InGestureEvent, TouchPointerId);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
}

FReply UTrackballUI::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const int32 TouchPointerId = 1000 + InGestureEvent.GetPointerIndex();
	const FReply Reply = HandlePointerReleased(InGeometry, InGestureEvent, TouchPointerId);
	ActiveTouchPointerIds.Remove(TouchPointerId);
	return Reply;
}

EColorDockSide UTrackballUI::GetDockAssignmentForToken(int32 TokenIndex) const
{
	if (IsEnemySelectedForToken(TokenIndex))
	{
		return EColorDockSide::Enemy;
	}

	if (IsFriendlySelectedForToken(TokenIndex))
	{
		return EColorDockSide::Friendly;
	}

	return EColorDockSide::None;
}

EColorDockSide UTrackballUI::GetPreferredDockForToken(int32 TokenIndex) const
{
	const bool bEnemySelected = IsEnemySelectedForToken(TokenIndex);
	const bool bFriendlySelected = IsFriendlySelectedForToken(TokenIndex);

	if (!bEnemySelected && !bFriendlySelected)
	{
		return EColorDockSide::None;
	}

	if (ColorTokenLastTouchedDock.IsValidIndex(TokenIndex))
	{
		const EColorDockSide LastTouchedDock = ColorTokenLastTouchedDock[TokenIndex];
		if (LastTouchedDock == EColorDockSide::Enemy && bEnemySelected)
		{
			return EColorDockSide::Enemy;
		}

		if (LastTouchedDock == EColorDockSide::Friendly && bFriendlySelected)
		{
			return EColorDockSide::Friendly;
		}
	}

	return bEnemySelected ? EColorDockSide::Enemy : EColorDockSide::Friendly;
}
