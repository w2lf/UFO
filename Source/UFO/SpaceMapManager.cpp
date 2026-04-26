// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceMapManager.h"
#include "SunActor.h"
#include "PlanetActor.h"
#include "SkyBoxActor.h"
#include "NPCUFOPawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/TextureCube.h"

ASpaceMapManager::ASpaceMapManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	// Set defaults
	SunDistance = 0.0f; // Sun at origin
	PlanetLeftDistance = 3000.0f;
	PlanetRightDistance = 3000.0f;
	PlanetVerticalOffset = 500.0f;
	SkyboxRadius = 400000.0f;
	NPCUFOCount = 10;
	SkyLightCubemap = TSoftObjectPtr<UTextureCube>(FSoftObjectPath(TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));

	// Root is scene component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASpaceMapManager::BeginPlay()
{
	Super::BeginPlay();

	SetupEnvironment();
	SpawnAmbientSkyLight();
	SpawnSun();
	SpawnPlanets();
	SpawnSkybox();
	SpawnNPCUFOs();
}

void ASpaceMapManager::SetupEnvironment()
{
	// Set world to black background
	if (UWorld* World = GetWorld())
	{
		// Set fog to black
		World->Modify();
		// We'll rely on material skybox instead of UE's default skybox
	}
}

void ASpaceMapManager::SpawnAmbientSkyLight()
{
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		ASkyLight* FillSkyLight = World->SpawnActor<ASkyLight>(
			ASkyLight::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (FillSkyLight && FillSkyLight->GetLightComponent())
		{
			USkyLightComponent* SkyComp = FillSkyLight->GetLightComponent();
			const float SkyLightIntensity = 0.5f;
			const float SkyLightIndirectIntensity = 1.0f;
			const FLinearColor SkyLightColor(0.10f, 0.12f, 0.16f);
			#if WITH_EDITOR
			FillSkyLight->SetActorLabel(FString(TEXT("AmbientFillSkyLight")));
			#endif
			SkyComp->SetMobility(EComponentMobility::Stationary);
			SkyComp->SetIntensity(SkyLightIntensity);
			SkyComp->SetIndirectLightingIntensity(SkyLightIndirectIntensity);
			SkyComp->SetLightColor(SkyLightColor);
			SkyComp->SetCastShadows(false);

			UTextureCube* Cubemap = SkyLightCubemap.LoadSynchronous();
			if (Cubemap)
			{
				SkyComp->SourceType = SLS_SpecifiedCubemap;
				SkyComp->SetCubemap(Cubemap);
				SkyComp->SetCubemapBlend(Cubemap, Cubemap, 0.0f);
			}
			else
			{
				SkyComp->SourceType = SLS_CapturedScene;
				SkyComp->RecaptureSky();
			}
		}
	}
}

void ASpaceMapManager::SpawnSun()
{
	if (UWorld* World = GetWorld())
	{
		// Keep exactly one sun actor in the world.
		ASunActor* Sun = nullptr;
		for (TActorIterator<ASunActor> It(World); It; ++It)
		{
			if (!Sun)
			{
				Sun = *It;
			}
			else
			{
				It->Destroy();
			}
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = nullptr;

		if (!Sun)
		{
			Sun = World->SpawnActor<ASunActor>(
				ASunActor::StaticClass(),
				FVector(0.0f, 0.0f, 0.0f),
				FRotator::ZeroRotator,
				SpawnParams
			);
		}

		if (Sun)
		{
			#if WITH_EDITOR
			Sun->SetActorLabel(FString(TEXT("Sun")));
			#endif
			Sun->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		}

		// Single point light at the sun center.
		FActorSpawnParameters LightSpawnParams;
		LightSpawnParams.Owner = this;

		// Remove previously managed sun lights to avoid duplicates.
		for (TActorIterator<APointLight> It(World); It; ++It)
		{
			if (It->ActorHasTag(FName(TEXT("ManagedSunLight"))))
			{
				It->Destroy();
			}
		}

		const FLinearColor WarmSunLight(1.0f, 0.86f, 0.62f);

		APointLight* SunPointLight = World->SpawnActor<APointLight>(
			APointLight::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			LightSpawnParams
		);

		UPointLightComponent* PointComp = SunPointLight ? Cast<UPointLightComponent>(SunPointLight->GetLightComponent()) : nullptr;
		if (SunPointLight && PointComp)
		{
			#if WITH_EDITOR
			SunPointLight->SetActorLabel(FString(TEXT("SunPointLight")));
			#endif
			SunPointLight->Tags.AddUnique(FName(TEXT("ManagedSunLight")));
			PointComp->SetIntensityUnits(ELightUnits::EV);
			PointComp->SetIntensity(25.0f);
			PointComp->SetAttenuationRadius(12000.0f);
			PointComp->SetLightColor(WarmSunLight);
			PointComp->SetCastShadows(false);
			PointComp->SetIndirectLightingIntensity(1.5f);
		}

	}
}

void ASpaceMapManager::SpawnPlanets()
{
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = nullptr;

		// Left planet
		APlanetActor* LeftPlanet = World->SpawnActor<APlanetActor>(
			APlanetActor::StaticClass(),
			FVector(-PlanetLeftDistance, 0.0f, PlanetVerticalOffset),
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (LeftPlanet)
		{
			#if WITH_EDITOR
			LeftPlanet->SetActorLabel(FString(TEXT("PlanetLeft")));
			#endif
		}

		// Right planet
		APlanetActor* RightPlanet = World->SpawnActor<APlanetActor>(
			APlanetActor::StaticClass(),
			FVector(PlanetRightDistance, 0.0f, PlanetVerticalOffset),
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (RightPlanet)
		{
			#if WITH_EDITOR
			RightPlanet->SetActorLabel(FString(TEXT("PlanetRight")));
			#endif
		}
	}
}

void ASpaceMapManager::SpawnSkybox()
{
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = nullptr;

		// Spawn skybox at origin - it will render as background
		ASkyBoxActor* SkyBox = World->SpawnActor<ASkyBoxActor>(
			ASkyBoxActor::StaticClass(),
			FVector(0.0f, 0.0f, 0.0f),
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (SkyBox)
		{
			#if WITH_EDITOR
			SkyBox->SetActorLabel(FString(TEXT("Skybox")));
			#endif
			SkyBox->SetSkyboxRadius(SkyboxRadius);
		}
	}
}

void ASpaceMapManager::SpawnNPCUFOs()
{
	if (NPCUFOCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const float SpawnRadius = 2000.0f;
	const TArray<FLinearColor> TokenPalette = {
		FLinearColor(0.24f, 0.85f, 0.38f, 1.0f),
		FLinearColor(1.0f, 0.58f, 0.16f, 1.0f),
		FLinearColor(0.26f, 0.65f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.56f, 0.62f, 1.0f),
		FLinearColor(0.83f, 0.52f, 0.13f, 1.0f),
		FLinearColor(0.74f, 0.68f, 0.64f, 1.0f),
		FLinearColor(0.73f, 0.57f, 1.0f, 1.0f)
	};

	for (int32 i = 0; i < NPCUFOCount; ++i)
	{
		// Spread NPCs in a ring around the arena at varying heights
		FVector SpawnLocation = FMath::VRand() * FMath::FRandRange(SpawnRadius * 0.4f, SpawnRadius);

		ANPCUFOPawn* NPC = World->SpawnActor<ANPCUFOPawn>(
			ANPCUFOPawn::StaticClass(),
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (NPC)
		{
			#if WITH_EDITOR
			NPC->SetActorLabel(FString::Printf(TEXT("NPCUFO_%d"), i + 1));
			#endif
			if (TokenPalette.Num() > 0)
			{
				const int32 TokenIndex = i % TokenPalette.Num();
				NPC->SetColorToken(TokenIndex, TokenPalette[TokenIndex]);
			}
		}
	}
}
