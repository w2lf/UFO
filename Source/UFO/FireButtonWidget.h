// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FireButtonWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class AUFOPlayerController;

/**
 * UFireButtonWidget
 *
 * A simple HUD widget rendered in the bottom-right corner.
 * Contains one large FIRE button. Tapping it calls
 * AUFOPlayerController::FireLaserAtSelectedNPC().
 */
UCLASS()
class UFO_API UFireButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY()
	UButton* FireButton = nullptr;

	UPROPERTY()
	UTextBlock* FireLabel = nullptr;

	UPROPERTY()
	UBorder* RootBorder = nullptr;

	UFUNCTION()
	void OnFireButtonClicked();

	static constexpr float ButtonWidth  = 140.0f;
	static constexpr float ButtonHeight =  72.0f;
	static constexpr float ButtonMargin =  24.0f;
};
