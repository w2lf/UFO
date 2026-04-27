// Copyright Epic Games, Inc. All Rights Reserved.

#include "FireButtonWidget.h"
#include "UFOPlayerController.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UFireButtonWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	// Dark rounded background border
	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetPadding(FMargin(0.0f));
	RootBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));  // transparent wrapper
	WidgetTree->RootWidget = RootBorder;

	// The fire button
	FireButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FireButton"));

	FButtonStyle ButtonStyle;
	auto MakeBrush = [](FLinearColor Color) -> FSlateBrush
	{
		FSlateBrush Brush;
		Brush.TintColor = FSlateColor(Color);
		Brush.DrawAs    = ESlateBrushDrawType::RoundedBox;
		FSlateBrushOutlineSettings Outline;
		Outline.CornerRadii = FVector4(12.0f, 12.0f, 12.0f, 12.0f);
		Outline.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings = Outline;
		return Brush;
	};

	ButtonStyle.Normal   = MakeBrush(FLinearColor(0.65f, 0.05f, 0.05f, 0.92f));
	ButtonStyle.Hovered  = MakeBrush(FLinearColor(0.85f, 0.10f, 0.10f, 1.00f));
	ButtonStyle.Pressed  = MakeBrush(FLinearColor(0.40f, 0.02f, 0.02f, 1.00f));
	FireButton->SetStyle(ButtonStyle);
	FireButton->SetClickMethod(EButtonClickMethod::PreciseClick);
	FireButton->OnClicked.AddDynamic(this, &UFireButtonWidget::OnFireButtonClicked);
	RootBorder->SetContent(FireButton);

	// Label inside the button
	FireLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FireLabel"));
	FireLabel->SetText(FText::FromString(TEXT("FIRE")));
	FireLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	{
		FSlateFontInfo Font = FireLabel->GetFont();
		Font.Size = 22;
		Font.OutlineSettings.OutlineSize = 1;
		FireLabel->SetFont(Font);
	}
	FireButton->SetContent(FireLabel);

	return RootBorder->TakeWidget();
}

void UFireButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Anchor to bottom-right corner
	SetAnchorsInViewport(FAnchors(1.0f, 1.0f));
	SetAlignmentInViewport(FVector2D(1.0f, 1.0f));
	SetPositionInViewport(FVector2D(-ButtonMargin, -ButtonMargin));
	SetDesiredSizeInViewport(FVector2D(ButtonWidth, ButtonHeight));
}

void UFireButtonWidget::OnFireButtonClicked()
{
	if (AUFOPlayerController* PC = Cast<AUFOPlayerController>(GetOwningPlayer()))
	{
		PC->FireLaserAtSelectedNPC();
	}
}
