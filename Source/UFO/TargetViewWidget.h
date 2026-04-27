// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetViewWidget.generated.h"

class UImage;
class UBorder;
class UTextBlock;
class UVerticalBox;
class UTextureRenderTarget2D;

/**
 * UTargetViewWidget
 *
 * Top-left HUD panel that displays a live 3-D scene capture of the
 * currently selected NPC target via ATargetCaptureActor.
 */
UCLASS()
class UFO_API UTargetViewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/**
	 * Feed in the render target from ATargetCaptureActor.
	 * Pass nullptr to collapse the panel.
	 */
	void SetRenderTarget(UTextureRenderTarget2D* InRenderTarget);

	/** Show or hide the entire panel. */
	void SetPreviewVisible(bool bVisible);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY()
	UBorder* RootBorder = nullptr;

	UPROPERTY()
	UTextBlock* TitleText = nullptr;

	UPROPERTY()
	UImage* PreviewImage = nullptr;

	static constexpr float PanelSize   = 220.0f;
	static constexpr float PanelMargin =  16.0f;
};
