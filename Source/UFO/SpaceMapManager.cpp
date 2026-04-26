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

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace SpaceMapConstants
{
	// Lighting
	constexpr float SkyLightIntensity          = 0.5f;
	constexpr float SkyLightIndirectIntensity  = 1.0f;
	const     FLinearColor SkyLightColor(0.10f, 0.12f, 0.16f);

	constexpr float PointLightIntensityEV      = 25.0f;
	constexpr float PointLightRadius           = 12000.0f;
	constexpr float PointLightIndirectIntensity= 1.5f;
	const     FLinearColor WarmSunColor(1.0f, 0.86f, 0.62f);

	// NPC spawning
	constexpr float NPCSpawnRadius             = 2000.0f;

	// Token palette — must match TrackballUI::ColorTokenColors
	const TArray<FLinearColor> TokenPalette = {
		FLinearColor(0.24f, 0.85f, 0.38f, 1.0f),
		FLinearColor(1.0f,  0.58f, 0.16f, 1.0f),
		FLinearColor(0.26f, 0.65f, 1.0f,  1.0f),
		FLinearColor(1.0f,  0.56f, 0.62f, 1.0f),
		FLinearColor(0.83f, 0.52f, 0.13f, 1.0f),
		FLinearColor(0.74f, 0.68f, 0.64f, 1.0f),
		FLinearColor(0.73f, 0.57f, 1.0f,  1.0f),
	};
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ASpaceMapManager::ASpaceMapManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SunDistance          = 0.0f;
	PlanetLeftDistance   = 3000.0f;
	PlanetRightDistance  = 3000.0f;
	PlanetVerticalOffset = 500.0f;
	SkyboxRadius         = 400000.0f;
	NPCUFOCount          = 10;
	SkyLightCubemap      = TSoftObjectPtr<UTextureCube>(FSoftObjectPath(
		TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// ---------------------------------------------------------------------------
// BeginPlay — orchestrates all spawn steps
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Spawn helpers
// ---------------------------------------------------------------------------

void ASpaceMapManager::SetupEnvironment()
{
	// Placeholder: override world fog / sky settings here if needed.
	// We currently rely on the material skybox for the space backdrop.
}

void ASpaceMapManager::SpawnAmbientSkyLight()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Owner = this;

	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(
		ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	USkyLightComponent* SkyComp = SkyLight ? SkyLight->GetLightComponent() : nullptr;
	if (!SkyComp) return;

#if WITH_EDITOR
	SkyLight->SetActorLabel(TEXT("AmbientFillSkyLight"));
#endif

	SkyComp->SetMobility(EComponentMobility::Stationary);
	SkyComp->SetIntensity(SpaceMapConstants::SkyLightIntensity);
	SkyComp->SetIndirectLightingIntensity(SpaceMapConstants::SkyLightIndirectIntensity);
	SkyComp->SetLightColor(SpaceMapConstants::SkyLightColor);
	SkyComp->SetCastShadows(false);

	if (UTextureCube* Cubemap = SkyLightCubemap.LoadSynchronous())
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

void ASpaceMapManager::SpawnSun()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Keep at most one sun actor — destroy any extras left over from previous PIE sessions
	ASunActor* Sun = nullptr;
	for (TActorIterator<ASunActor> It(World); It; ++It)
	{
		if (!Sun) { Sun = *It; }
		else       { It->Destroy(); }
	}

	if (!Sun)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Sun = World->SpawnActor<ASunActor>(
			ASunActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	if (Sun)
	{
#if WITH_EDITOR
		Sun->SetActorLabel(TEXT("Sun"));
#endif
		Sun->SetActorLocation(FVector::ZeroVector);
	}

	// Remove any stale managed point lights before spawning a fresh one
	for (TActorIterator<APointLight> It(World); It; ++It)
	{
		if (It->ActorHasTag(FName(TEXT("ManagedSunLight"))))
		{
			It->Destroy();
		}
	}

	FActorSpawnParameters LightParams;
	LightParams.Owner = this;
	APointLight* SunLight = World->SpawnActor<APointLight>(
		APointLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, LightParams);

	UPointLightComponent* PointComp = SunLight
		? Cast<UPointLightComponent>(SunLight->GetLightComponent())
		: nullptr;

	if (!PointComp) return;

#if WITH_EDITOR
	SunLight->SetActorLabel(TEXT("SunPointLight"));
#endif
	SunLight->Tags.AddUnique(FName(TEXT("ManagedSunLight")));
	PointComp->SetIntensityUnits(ELightUnits::EV);
	PointComp->SetIntensity(SpaceMapConstants::PointLightIntensityEV);
	PointComp->SetAttenuationRadius(SpaceMapConstants::PointLightRadius);
	PointComp->SetLightColor(SpaceMapConstants::WarmSunColor);
	PointComp->SetCastShadows(false);
	PointComp->SetIndirectLightingIntensity(SpaceMapConstants::PointLightIndirectIntensity);
}

void ASpaceMapManager::SpawnPlanets()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Owner = this;

	auto SpawnPlanet = [&](const FVector& Location, const TCHAR* Label)
	{
		APlanetActor* Planet = World->SpawnActor<APlanetActor>(
			APlanetActor::StaticClass(), Location, FRotator::ZeroRotator, Params);
#if WITH_EDITOR
		if (Planet) { Planet->SetActorLabel(Label); }
#endif
	};

	SpawnPlanet(FVector(-PlanetLeftDistance,  0.0f, PlanetVerticalOffset), TEXT("PlanetLeft"));
	SpawnPlanet(FVector( PlanetRightDistance,  0.0f, PlanetVerticalOffset), TEXT("PlanetRight"));
}

void ASpaceMapManager::SpawnSkybox()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Owner = this;

	ASkyBoxActor* SkyBox = World->SpawnActor<ASkyBoxActor>(
		ASkyBoxActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	if (SkyBox)
	{
#if WITH_EDITOR
		SkyBox->SetActorLabel(TEXT("Skybox"));
#endif
		SkyBox->SetSkyboxRadius(SkyboxRadius);
	}
}

void ASpaceMapManager::SpawnNPCUFOs()
{
	if (NPCUFOCount <= 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const TArray<FLinearColor>& Palette = SpaceMapConstants::TokenPalette;

	for (int32 i = 0; i < NPCUFOCount; ++i)
	{
		const FVector SpawnLoc = FMath::VRand() * FMath::FRandRange(
			SpaceMapConstants::NPCSpawnRadius * 0.4f,
			SpaceMapConstants::NPCSpawnRadius);

		ANPCUFOPawn* NPC = World->SpawnActor<ANPCUFOPawn>(
			ANPCUFOPawn::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);

		if (!NPC) continue;

#if WITH_EDITOR
		NPC->SetActorLabel(FString::Printf(TEXT("NPCUFO_%d"), i + 1));
#endif
		if (Palette.Num() > 0)
		{
			const int32 TokenIndex = i % Palette.Num();
			NPC->SetColorToken(TokenIndex, Palette[TokenIndex]);
		}
	}
}
