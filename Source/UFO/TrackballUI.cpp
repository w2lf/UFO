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

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace TrackballUIConstants
{
	// Layout fractions (relative to widget size)
	constexpr float LeftCenterX          = 0.15f;
	constexpr float RightCenterX         = 0.85f;
	constexpr float TrackballCenterY     = 0.80f;
	constexpr float SliderFractX         = 0.93f;
	constexpr float SliderTopFractY      = 0.20f;
	constexpr float SliderBottomFractY   = 0.85f;

	// Base screen size used for all responsive scaling
	constexpr float BaseScreenSize       = 1080.0f;

	// Smoothing blend: 60% current + 40% previous
	constexpr float SmoothBlend          = 0.6f;
	constexpr float MovementThresholdPx  = 1.0f;

	// Geometry
	constexpr int32 RingSegments         = 48;
	constexpr int32 ThumbSegments        = 36;
	constexpr int32 TokenSegments        = 36;
	constexpr int32 SliderThumbSegments  = 28;
	constexpr float SliderThumbRadius    = 16.0f;

	// Mouse pointer ID
	constexpr int32 MousePointerId       = 0;
	constexpr int32 TouchPointerBase     = 1000;
}

// ---------------------------------------------------------------------------
// NativeConstruct
// ---------------------------------------------------------------------------

void UTrackballUI::NativeConstruct()
{
	Super::NativeConstruct();

	TrackballRadius               = 160.0f;
	TrackballColor                = FLinearColor(0.0f, 1.0f, 1.0f, 0.3f);
	TrackballOpacity              = 0.3f;
	ThumbRadius                   = 40.0f;
	RingThickness                 = 2.5f;
	bShowInputDebug               = false;
	SliderMinThrottle             = 0.0f;
	SliderMaxThrottle             = 1.0f;
	SliderResponseExponent        = 1.6f;
	ColorTokenRadius              = 55.0f;
	ColorTokenOutlineThickness    = 11.0f;
	AssignedTokenRowY             = 0.66f;
	UnassignedTokenRowY           = 0.82f;
	ColorDockDeadZone             = 0.08f;

	ColorTokenColors = {
		FLinearColor(0.24f, 0.85f, 0.38f, 1.0f),
		FLinearColor(1.0f,  0.58f, 0.16f, 1.0f),
		FLinearColor(0.26f, 0.65f, 1.0f,  1.0f),
		FLinearColor(1.0f,  0.56f, 0.62f, 1.0f),
		FLinearColor(0.83f, 0.52f, 0.13f, 1.0f),
		FLinearColor(0.74f, 0.68f, 0.64f, 1.0f),
		FLinearColor(0.73f, 0.57f, 1.0f,  1.0f),
	};

	ColorTokenEnemySelections   .Init(false,               ColorTokenColors.Num());
	ColorTokenFriendlySelections.Init(false,               ColorTokenColors.Num());
	ColorTokenLastTouchedDock   .Init(EColorDockSide::None, ColorTokenColors.Num());

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

// ---------------------------------------------------------------------------
// Layout helpers (called by both NativePaint and hit-testing)
// ---------------------------------------------------------------------------

namespace
{
	/** Returns the scaled trackball radius for a given widget size */
	float GetScreenScale(const FVector2D& WidgetSize)
	{
		return FMath::Min(WidgetSize.X, WidgetSize.Y) / TrackballUIConstants::BaseScreenSize;
	}

	/** Returns the widget-relative centre of the left trackball */
	FVector2D GetLeftCenter(const FVector2D& WidgetSize)
	{
		return FVector2D(WidgetSize.X * TrackballUIConstants::LeftCenterX,
						 WidgetSize.Y * TrackballUIConstants::TrackballCenterY);
	}

	/** Returns the widget-relative centre of the right trackball */
	FVector2D GetRightCenter(const FVector2D& WidgetSize)
	{
		return FVector2D(WidgetSize.X * TrackballUIConstants::RightCenterX,
						 WidgetSize.Y * TrackballUIConstants::TrackballCenterY);
	}

	/** Returns the slider geometry: X centre, top Y, bottom Y, half-width */
	struct FSliderLayout
	{
		float CentreX, TopY, BottomY, HalfWidth;
	};

	FSliderLayout GetSliderLayout(const FVector2D& WidgetSize, float ScreenScale)
	{
		FSliderLayout L;
		L.CentreX   = WidgetSize.X * TrackballUIConstants::SliderFractX;
		L.TopY      = WidgetSize.Y * TrackballUIConstants::SliderTopFractY;
		L.BottomY   = WidgetSize.Y * TrackballUIConstants::SliderBottomFractY;
		L.HalfWidth = 26.0f * ScreenScale;  // half of 52px
		return L;
	}
}

// ---------------------------------------------------------------------------
// NativePaint
// ---------------------------------------------------------------------------

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

	const float ScreenScale             = GetScreenScale(WidgetSize);
	const float ScaledTrackballRadius   = TrackballRadius * ScreenScale;
	const float ScaledThumbRadius       = ThumbRadius     * ScreenScale;
	const float ScaledTokenRadius       = ColorTokenRadius * ScreenScale;
	const float ScaledTokenOutline      = ColorTokenOutlineThickness * ScreenScale;

	const FVector2D LeftCenter  = GetLeftCenter(WidgetSize);
	const FVector2D RightCenter = GetRightCenter(WidgetSize);
	const FSliderLayout Slider  = GetSliderLayout(WidgetSize, ScreenScale);

	// ----- Colours -----
	const FLinearColor FillColor      (TrackballColor.R, TrackballColor.G, TrackballColor.B, TrackballOpacity);
	const FLinearColor RingColor      (TrackballColor.R, TrackballColor.G, TrackballColor.B, 0.9f);
	const FLinearColor InnerRingColor (TrackballColor.R, TrackballColor.G, TrackballColor.B, 0.35f);
	const FLinearColor ThumbColor     (0.85f, 0.95f, 1.0f, 0.95f);
	const FLinearColor ThumbHighlight (1.0f,  1.0f,  1.0f, 0.75f);
	const FLinearColor SliderTrackCol (0.1f,  0.75f, 0.95f, 0.3f);
	const FLinearColor SliderFillCol  (0.2f,  0.95f, 1.0f,  0.75f);
	const FLinearColor SliderThumbCol (0.95f, 1.0f,  1.0f,  0.95f);
	const FLinearColor DividerColor   (0.72f, 0.58f, 1.0f,  0.75f);
	const FLinearColor EnemyOutline   (1.0f,  0.36f, 0.36f, 1.0f);
	const FLinearColor FriendlyOutline(0.18f, 1.0f,  0.34f, 1.0f);

	// ----- Resolve thumb positions -----
	FVector2D LeftThumbPos  = LeftCenter;
	FVector2D RightThumbPos = RightCenter;
	for (const TPair<int32, ETrackballSide>& Pair : ActivePointers)
	{
		if (const FVector2D* Pos = PreviousPointerPositions.Find(Pair.Key))
		{
			if      (Pair.Value == ETrackballSide::Left)  { LeftThumbPos  = ClampPointToTrackball(*Pos, LeftCenter); }
			else if (Pair.Value == ETrackballSide::Right) { RightThumbPos = ClampPointToTrackball(*Pos, RightCenter); }
		}
	}

	const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 14);

	// =========================================================
	// Local draw lambdas
	// =========================================================

	// Draw a circle outline from a centre + radius
	auto DrawCircle = [&](const FVector2D& Centre, float Radius, const FLinearColor& Color,
							  float Thickness, int32 NumSegments, int32 Layer)
	{
		TArray<FVector2f> Pts;
		Pts.Reserve(NumSegments + 1);
		for (int32 i = 0; i <= NumSegments; ++i)
		{
			const float T = (2.0f * PI * i) / NumSegments;
			Pts.Add(FVector2f(Centre + FVector2D(FMath::Cos(T) * Radius, FMath::Sin(T) * Radius)));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, Color, true, Thickness);
	};

	auto DrawTrackballRing = [&](const FVector2D& Centre, int32 Layer)
	{
		DrawCircle(Centre, ScaledTrackballRadius,          RingColor,      RingThickness, TrackballUIConstants::RingSegments, Layer);
		DrawCircle(Centre, ScaledTrackballRadius * 0.55f,  InnerRingColor, 1.5f,          TrackballUIConstants::RingSegments, Layer);
		DrawCircle(Centre, ScaledTrackballRadius * 0.25f,  FillColor,      2.0f,          TrackballUIConstants::RingSegments, Layer);
	};

	auto DrawThumb = [&](const FVector2D& Pos, int32 Layer)
	{
		DrawCircle(Pos, ScaledThumbRadius,          ThumbColor,
				   2.5f, TrackballUIConstants::ThumbSegments, Layer);
		DrawCircle(Pos, ScaledThumbRadius * 0.55f,
				   FLinearColor(ThumbColor.R, ThumbColor.G, ThumbColor.B, 0.55f),
				   2.0f, TrackballUIConstants::ThumbSegments, Layer);
		// Highlight dot
		const FVector2D HL = Pos + FVector2D(ScaledThumbRadius * 0.25f, -ScaledThumbRadius * 0.2f);
		DrawCircle(HL, ScaledThumbRadius * 0.18f,
				   ThumbHighlight, 1.5f, TrackballUIConstants::ThumbSegments, Layer + 1);
	};

	auto DrawColorToken = [&](const FVector2D& Centre, const FLinearColor& Fill,
								  bool bEnemySel, bool bFriendlySel, int32 Layer)
	{
		if (bEnemySel)
		{
			DrawCircle(Centre, ScaledTokenRadius,          EnemyOutline,    ScaledTokenOutline, TrackballUIConstants::TokenSegments, Layer);
		}
		DrawCircle(Centre, ScaledTokenRadius * 0.76f,
				   FLinearColor(Fill.R, Fill.G, Fill.B, 0.98f),
				   5.6f, TrackballUIConstants::TokenSegments, Layer + 1);
		if (bFriendlySel)
		{
			DrawCircle(Centre, ScaledTokenRadius * 0.54f, FriendlyOutline, ScaledTokenOutline, TrackballUIConstants::TokenSegments, Layer + 2);
		}
	};

	// =========================================================
	// Draw trackballs
	// =========================================================
	DrawTrackballRing(LeftCenter,  LayerId + 1);
	DrawThumb(LeftThumbPos,        LayerId + 2);
	DrawTrackballRing(RightCenter, LayerId + 1);
	DrawThumb(RightThumbPos,       LayerId + 2);

	// =========================================================
	// Draw divider + zone labels
	// =========================================================
	const float DividerX   = WidgetSize.X * 0.5f;
	const float DockCentreY= WidgetSize.Y * AssignedTokenRowY;

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		{ FVector2f(DividerX, WidgetSize.Y * 0.44f), FVector2f(DividerX, WidgetSize.Y * 0.90f) },
		ESlateDrawEffect::None, DividerColor, true, 2.0f);

	auto DrawLabel = [&](const FString& Text, float X, float Y, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(FVector2D(200.0f, 20.0f)),
				FSlateLayoutTransform(FVector2f(X, Y))),
			Text, FontInfo, ESlateDrawEffect::None, Color);
	};

	// Count assigned tokens
	int32 EnemyCount = 0, FriendlyCount = 0;
	for (int32 t = 0; t < ColorTokenColors.Num(); ++t)
	{
		if (IsEnemySelectedForToken(t))    { ++EnemyCount; }
		if (IsFriendlySelectedForToken(t)) { ++FriendlyCount; }
	}

	DrawLabel(TEXT("Enemy Zone"),                        DividerX - 170.0f, DockCentreY - 118.0f, DividerColor);
	DrawLabel(FString::Printf(TEXT("Enemy %d"),    EnemyCount),  DividerX - 170.0f, DockCentreY - 98.0f,  FLinearColor(1.0f, 0.36f, 0.36f, 0.95f));
	DrawLabel(TEXT("Friendly Zone"),                     DividerX + 28.0f,  DockCentreY - 118.0f, DividerColor);
	DrawLabel(FString::Printf(TEXT("Friendly %d"), FriendlyCount), DividerX + 28.0f,  DockCentreY - 98.0f,  FLinearColor(0.18f, 1.0f, 0.34f, 0.95f));

	// =========================================================
	// Draw colour tokens
	// =========================================================
	for (int32 TokenIndex = 0; TokenIndex < ColorTokenColors.Num(); ++TokenIndex)
	{
		const FVector2D TokenCentre = GetColorTokenPaintPosition(TokenIndex, WidgetSize);
		bool bEnemySel    = IsEnemySelectedForToken(TokenIndex);
		bool bFriendlySel = IsFriendlySelectedForToken(TokenIndex);

		for (const TPair<int32, int32>& Pair : ActiveColorTokenPointers)
		{
			if (Pair.Value == TokenIndex)
			{
				if (const FVector2D* DragPos = ActiveColorTokenPositions.Find(Pair.Key))
				{
					const EColorDockSide Hovered = ResolveColorDockSide(*DragPos, WidgetSize);
					if (Hovered == EColorDockSide::Enemy)    { bEnemySel    = !bEnemySel; }
					if (Hovered == EColorDockSide::Friendly) { bFriendlySel = !bFriendlySel; }
				}
				break;
			}
		}

		DrawColorToken(TokenCentre, ColorTokenColors[TokenIndex], bEnemySel, bFriendlySel, LayerId + 2);
	}

	// =========================================================
	// Draw speed slider
	// =========================================================
	float SliderValue = SpeedSliderValueNormalized;
	if (CachedPawn.IsValid()) { SliderValue = CachedPawn->GetThrottleNormalized(); }

	const float SliderThumbY    = FMath::Lerp(Slider.BottomY, Slider.TopY, FMath::Clamp(SliderValue, 0.0f, 1.0f));
	const FVector2D SliderTL    (Slider.CentreX - Slider.HalfWidth, Slider.TopY);
	const float SliderW         = Slider.HalfWidth * 2.0f;
	const float SliderH         = Slider.BottomY - Slider.TopY;

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2f(SliderW, SliderH), FSlateLayoutTransform(FVector2f(SliderTL))),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, SliderTrackCol);

	const float FillH = Slider.BottomY - SliderThumbY;
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2f(SliderW, FillH), FSlateLayoutTransform(FVector2f(SliderTL.X, SliderThumbY))),
		FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, SliderFillCol);

	DrawCircle(FVector2D(Slider.CentreX, SliderThumbY),
			   TrackballUIConstants::SliderThumbRadius, SliderThumbCol, 2.0f,
			   TrackballUIConstants::SliderThumbSegments, LayerId + 3);

	// =========================================================
	// Draw UI labels
	// =========================================================
	DrawLabel(TEXT("Camera"), LeftCenter.X  - 30.0f,                    LeftCenter.Y  - 8.0f,           FLinearColor::White);
	DrawLabel(TEXT("Ship"),   RightCenter.X - 20.0f,                    RightCenter.Y - 8.0f,           FLinearColor::White);
	DrawLabel(TEXT("Speed"),  Slider.CentreX - 28.0f,                   Slider.TopY   - 22.0f,          FLinearColor::White);
	DrawLabel(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(SliderValue * 100.0f)),
			  Slider.CentreX - 28.0f, Slider.BottomY + 10.0f, FLinearColor::White);

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

FVector2D UTrackballUI::ClampPointToTrackball(const FVector2D& Point, const FVector2D& Centre) const
{
	const float Scale  = FMath::Min(Centre.X * 2.0f, Centre.Y * 2.0f) / TrackballUIConstants::BaseScreenSize;
	const float Radius = TrackballRadius * Scale;
	const FVector2D Delta = Point - Centre;
	const float Dist = Delta.Length();
	return (Dist <= Radius || Dist <= KINDA_SMALL_NUMBER) ? Point : Centre + (Delta / Dist) * Radius;
}

ETrackballSide UTrackballUI::ResolveTrackballSide(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	const float Scale  = GetScreenScale(WidgetSize);
	const float Radius = TrackballRadius * Scale;
	const FSliderLayout Slider = GetSliderLayout(WidgetSize, Scale);

	const float SliderL = Slider.CentreX - Slider.HalfWidth;
	const float SliderR = Slider.CentreX + Slider.HalfWidth;
	if (LocalPos.X >= SliderL && LocalPos.X <= SliderR &&
		LocalPos.Y >= Slider.TopY  && LocalPos.Y <= Slider.BottomY)
	{
		return ETrackballSide::SpeedSlider;
	}

	if ((LocalPos - GetLeftCenter(WidgetSize)).SizeSquared()  <= FMath::Square(Radius)) return ETrackballSide::Left;
	if ((LocalPos - GetRightCenter(WidgetSize)).SizeSquared() <= FMath::Square(Radius)) return ETrackballSide::Right;

	return ETrackballSide::None;
}

int32 UTrackballUI::ResolveColorTokenIndex(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	const float ScaledRadius = ColorTokenRadius * GetScreenScale(WidgetSize);

	for (int32 i = 0; i < ColorTokenColors.Num(); ++i)
	{
		if ((LocalPos - GetColorTokenPaintPosition(i, WidgetSize)).SizeSquared() <= FMath::Square(ScaledRadius * 1.25f))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

EColorDockSide UTrackballUI::ResolveColorDockSide(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	const float L = WidgetSize.X * (0.5f - ColorDockDeadZone);
	const float R = WidgetSize.X * (0.5f + ColorDockDeadZone);

	if (LocalPos.X < L) return EColorDockSide::Enemy;
	if (LocalPos.X > R) return EColorDockSide::Friendly;
	return EColorDockSide::None;
}

FVector2D UTrackballUI::GetColorTokenPaintPosition(int32 TokenIndex, const FVector2D& WidgetSize) const
{
	// If this token is being dragged, follow the finger
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

	if (!ColorTokenColors.IsValidIndex(TokenIndex)) return FVector2D::ZeroVector;

	const float Scale           = GetScreenScale(WidgetSize);
	const float ScaledTBRadius  = TrackballRadius  * Scale;
	const float ScaledTokRadius = ColorTokenRadius * Scale;
	const float ExclusionRadius = ScaledTBRadius * 1.4f;

	const FVector2D LC = GetLeftCenter(WidgetSize);
	const FVector2D RC = GetRightCenter(WidgetSize);

	const float Spacing    = ColorTokenRadius * 2.45f;
	const float TotalWidth = FMath::Max(0.0f, (ColorTokenColors.Num() - 1) * Spacing);
	const float StartX     = (WidgetSize.X * 0.5f) - (TotalWidth * 0.5f);

	FVector2D Pos(StartX + TokenIndex * Spacing, WidgetSize.Y * UnassignedTokenRowY);

	// Push away from trackball exclusion zones
	const float DistL = FVector2D::Distance(Pos, LC);
	const float DistR = FVector2D::Distance(Pos, RC);

	if (DistL < ExclusionRadius + ScaledTokRadius)
	{
		Pos.X += ExclusionRadius + ScaledTokRadius - DistL + 20.0f;
	}
	else if (DistR < ExclusionRadius + ScaledTokRadius)
	{
		Pos.X -= ExclusionRadius + ScaledTokRadius - DistR + 20.0f;
	}

	return Pos;
}

// ---------------------------------------------------------------------------
// Speed slider
// ---------------------------------------------------------------------------

void UTrackballUI::UpdateSpeedFromPointer(const FVector2D& LocalPos, const FVector2D& WidgetSize)
{
	const float Top    = WidgetSize.Y * TrackballUIConstants::SliderTopFractY;
	const float Bottom = WidgetSize.Y * TrackballUIConstants::SliderBottomFractY;
	const float Height = FMath::Max(1.0f, Bottom - Top);

	const float RawNorm  = 1.0f - ((FMath::Clamp(LocalPos.Y, Top, Bottom) - Top) / Height);
	const float MinOut   = FMath::Clamp(FMath::Min(SliderMinThrottle, SliderMaxThrottle), 0.0f, 1.0f);
	const float MaxOut   = FMath::Clamp(FMath::Max(SliderMinThrottle, SliderMaxThrottle), 0.0f, 1.0f);
	const float Shaped   = FMath::Pow(RawNorm, FMath::Max(0.01f, SliderResponseExponent));

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

// ---------------------------------------------------------------------------
// Pointer helpers
// ---------------------------------------------------------------------------

FReply UTrackballUI::HandlePointerPressed(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId)
{
	const FVector2D LocalPos  = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
	const FVector2D WidgetSize = InGeometry.GetLocalSize();

	// Token drag takes priority over trackball
	const int32 TokenIdx = ResolveColorTokenIndex(LocalPos, WidgetSize);
	if (TokenIdx != INDEX_NONE)
	{
		ActiveColorTokenPointers.Add(PointerId, TokenIdx);
		ActiveColorTokenPositions.Add(PointerId, LocalPos);
		return FReply::Handled();
	}

	const ETrackballSide Side = ResolveTrackballSide(LocalPos, WidgetSize);
	if (Side == ETrackballSide::None) return FReply::Unhandled();

	ActivePointers.Add(PointerId, Side);
	PreviousPointerPositions.Add(PointerId, LocalPos);

	if (Side == ETrackballSide::SpeedSlider)
	{
		UpdateSpeedFromPointer(LocalPos, WidgetSize);
	}

	if (bShowInputDebug && GEngine)
	{
		const TCHAR* Label = (Side == ETrackballSide::Left) ? TEXT("LEFT") : TEXT("RIGHT");
		GEngine->AddOnScreenDebugMessage(500 + PointerId, 1.5f, FColor::Green,
			FString::Printf(TEXT("Trackball Start: %s (Ptr %d)"), Label, PointerId));
	}

	return FReply::Handled();
}

FReply UTrackballUI::HandlePointerMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId)
{
	const FVector2D LocalPos   = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
	const FVector2D WidgetSize = InGeometry.GetLocalSize();

	// Token being dragged
	if (ActiveColorTokenPointers.Contains(PointerId))
	{
		ActiveColorTokenPositions.FindOrAdd(PointerId) = LocalPos;
		return FReply::Handled();
	}

	const ETrackballSide* Side = ActivePointers.Find(PointerId);
	FVector2D*            Prev = PreviousPointerPositions.Find(PointerId);
	if (!Side || !Prev) return FReply::Unhandled();

	// Temporal smoothing: 60/40 blend
	const FVector2D Smoothed = (Prev->SizeSquared() > 0)
		? (LocalPos * TrackballUIConstants::SmoothBlend) + (*Prev * (1.0f - TrackballUIConstants::SmoothBlend))
		: LocalPos;

	if ((Smoothed - *Prev).SizeSquared() < FMath::Square(TrackballUIConstants::MovementThresholdPx))
	{
		return FReply::Handled(); // Below noise threshold
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
			CachedPawn->ApplyLeftTrackballDrag(*Prev, Smoothed);
			if (bShowInputDebug && GEngine)
			{
				const FVector2D D = LocalPos - *Prev;
				GEngine->AddOnScreenDebugMessage(600 + PointerId, 0.05f, FColor::Cyan,
					FString::Printf(TEXT("LEFT  dX=%.1f dY=%.1f"), D.X, D.Y));
			}
		}
		else if (*Side == ETrackballSide::Right)
		{
			CachedPawn->ApplyRightTrackballDrag(*Prev, Smoothed);
			if (bShowInputDebug && GEngine)
			{
				const FVector2D D = LocalPos - *Prev;
				GEngine->AddOnScreenDebugMessage(700 + PointerId, 0.05f, FColor::Yellow,
					FString::Printf(TEXT("RIGHT dX=%.1f dY=%.1f"), D.X, D.Y));
			}
		}
		else if (*Side == ETrackballSide::SpeedSlider)
		{
			UpdateSpeedFromPointer(Smoothed, WidgetSize);
		}
	}
	else if (*Side == ETrackballSide::SpeedSlider)
	{
		UpdateSpeedFromPointer(Smoothed, WidgetSize);
	}

	*Prev = Smoothed;
	return FReply::Handled();
}

FReply UTrackballUI::HandlePointerReleased(const FGeometry& InGeometry, const FPointerEvent& InEvent, int32 PointerId)
{
	if (const int32* TokenIndex = ActiveColorTokenPointers.Find(PointerId))
	{
		const FVector2D LocalPos  = InGeometry.AbsoluteToLocal(InEvent.GetScreenSpacePosition());
		const EColorDockSide Dock = ResolveColorDockSide(LocalPos, InGeometry.GetLocalSize());
		bool bChanged = false;

		if (Dock == EColorDockSide::Enemy && ColorTokenEnemySelections.IsValidIndex(*TokenIndex))
		{
			ColorTokenEnemySelections[*TokenIndex] = !ColorTokenEnemySelections[*TokenIndex];
			if (ColorTokenLastTouchedDock.IsValidIndex(*TokenIndex))
			{
				ColorTokenLastTouchedDock[*TokenIndex] = EColorDockSide::Enemy;
			}
			bChanged = true;
		}
		else if (Dock == EColorDockSide::Friendly && ColorTokenFriendlySelections.IsValidIndex(*TokenIndex))
		{
			ColorTokenFriendlySelections[*TokenIndex] = !ColorTokenFriendlySelections[*TokenIndex];
			if (ColorTokenLastTouchedDock.IsValidIndex(*TokenIndex))
			{
				ColorTokenLastTouchedDock[*TokenIndex] = EColorDockSide::Friendly;
			}
			bChanged = true;
		}

		if (bChanged)
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
		GEngine->AddOnScreenDebugMessage(800 + PointerId, 1.0f, FColor::Silver,
			FString::Printf(TEXT("Pointer End (Ptr %d)"), PointerId));
	}

	ActivePointers.Remove(PointerId);
	PreviousPointerPositions.Remove(PointerId);
	return FReply::Handled();
}

// ---------------------------------------------------------------------------
// Mouse events (desktop)
// ---------------------------------------------------------------------------

FReply UTrackballUI::NativeOnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (ActiveTouchPointerIds.Num() > 0 || Ev.IsTouchEvent() || !Ev.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return Super::NativeOnMouseButtonDown(Geo, Ev);
	}

	FReply R = HandlePointerPressed(Geo, Ev, TrackballUIConstants::MousePointerId);
	return R.IsEventHandled() ? R.CaptureMouse(TakeWidget()) : Super::NativeOnMouseButtonDown(Geo, Ev);
}

FReply UTrackballUI::NativeOnMouseMove(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (ActiveTouchPointerIds.Num() > 0 || Ev.IsTouchEvent() || !Ev.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return Super::NativeOnMouseMove(Geo, Ev);
	}

	FReply R = HandlePointerMoved(Geo, Ev, TrackballUIConstants::MousePointerId);
	return R.IsEventHandled() ? R : Super::NativeOnMouseMove(Geo, Ev);
}

FReply UTrackballUI::NativeOnMouseButtonUp(const FGeometry& Geo, const FPointerEvent& Ev)
{
	if (ActiveTouchPointerIds.Num() > 0 || Ev.IsTouchEvent()) return Super::NativeOnMouseButtonUp(Geo, Ev);

	const int32 ID = TrackballUIConstants::MousePointerId;
	if (ActivePointers.Contains(ID) || ActiveColorTokenPointers.Contains(ID))
	{
		return HandlePointerReleased(Geo, Ev, ID).ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(Geo, Ev);
}

// ---------------------------------------------------------------------------
// Touch events (mobile)
// ---------------------------------------------------------------------------

FReply UTrackballUI::NativeOnTouchStarted(const FGeometry& Geo, const FPointerEvent& Ev)
{
	const int32 ID = TrackballUIConstants::TouchPointerBase + Ev.GetPointerIndex();
	ActiveTouchPointerIds.Add(ID);
	FReply R = HandlePointerPressed(Geo, Ev, ID);
	return R.IsEventHandled() ? R : Super::NativeOnTouchStarted(Geo, Ev);
}

FReply UTrackballUI::NativeOnTouchMoved(const FGeometry& Geo, const FPointerEvent& Ev)
{
	const int32 ID = TrackballUIConstants::TouchPointerBase + Ev.GetPointerIndex();
	FReply R = HandlePointerMoved(Geo, Ev, ID);
	return R.IsEventHandled() ? R : Super::NativeOnTouchMoved(Geo, Ev);
}

FReply UTrackballUI::NativeOnTouchEnded(const FGeometry& Geo, const FPointerEvent& Ev)
{
	const int32 ID = TrackballUIConstants::TouchPointerBase + Ev.GetPointerIndex();
	FReply R = HandlePointerReleased(Geo, Ev, ID);
	ActiveTouchPointerIds.Remove(ID);
	return R;
}

// ---------------------------------------------------------------------------
// Dock state queries
// ---------------------------------------------------------------------------

bool UTrackballUI::IsEnemySelectedForToken(int32 TokenIndex) const
{
	return ColorTokenEnemySelections.IsValidIndex(TokenIndex) ? ColorTokenEnemySelections[TokenIndex] : false;
}

bool UTrackballUI::IsFriendlySelectedForToken(int32 TokenIndex) const
{
	return ColorTokenFriendlySelections.IsValidIndex(TokenIndex) ? ColorTokenFriendlySelections[TokenIndex] : false;
}

EColorDockSide UTrackballUI::GetDockAssignmentForToken(int32 TokenIndex) const
{
	if (IsEnemySelectedForToken(TokenIndex))    return EColorDockSide::Enemy;
	if (IsFriendlySelectedForToken(TokenIndex)) return EColorDockSide::Friendly;
	return EColorDockSide::None;
}

EColorDockSide UTrackballUI::GetPreferredDockForToken(int32 TokenIndex) const
{
	const bool bEnemy    = IsEnemySelectedForToken(TokenIndex);
	const bool bFriendly = IsFriendlySelectedForToken(TokenIndex);

	if (!bEnemy && !bFriendly) return EColorDockSide::None;

	if (ColorTokenLastTouchedDock.IsValidIndex(TokenIndex))
	{
		const EColorDockSide Last = ColorTokenLastTouchedDock[TokenIndex];
		if (Last == EColorDockSide::Enemy    && bEnemy)    return EColorDockSide::Enemy;
		if (Last == EColorDockSide::Friendly && bFriendly) return EColorDockSide::Friendly;
	}

	return bEnemy ? EColorDockSide::Enemy : EColorDockSide::Friendly;
}
