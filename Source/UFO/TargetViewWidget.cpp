// Copyright Epic Games, Inc. All Rights Reserved.

#include "TargetViewWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UTargetViewWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	// Root border — dark semi-transparent background
	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetPadding(FMargin(8.0f));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f));
	WidgetTree->RootWidget = RootBorder;

	// Vertical stack: title label on top, capture image below
	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
	RootBorder->SetContent(VBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Selected Target")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.9f, 1.0f, 1.0f)));
	VBox->AddChildToVerticalBox(TitleText);

	PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PreviewImage"));
	{
		FSlateBrush EmptyBrush;
		EmptyBrush.ImageSize = FVector2D(PanelSize, PanelSize);
		PreviewImage->SetBrush(EmptyBrush);
	}
	VBox->AddChildToVerticalBox(PreviewImage);

	SetPreviewVisible(false);

	return RootBorder->TakeWidget();
}

void UTargetViewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetPositionInViewport(FVector2D(PanelMargin, PanelMargin));
	SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
}

void UTargetViewWidget::SetRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	if (!PreviewImage) return;

	if (!InRenderTarget)
	{
		PreviewImage->SetBrush(FSlateBrush());
		SetPreviewVisible(false);
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(InRenderTarget);
	Brush.ImageSize = FVector2D(PanelSize, PanelSize);
	Brush.DrawAs    = ESlateBrushDrawType::Image;
	PreviewImage->SetBrush(Brush);
	SetPreviewVisible(true);
}

void UTargetViewWidget::SetPreviewVisible(bool bVisible)
{
	const ESlateVisibility V = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	SetVisibility(V);
	if (RootBorder) RootBorder->SetVisibility(V);
}
