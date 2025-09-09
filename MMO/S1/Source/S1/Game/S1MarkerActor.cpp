// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1MarkerActor.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
AS1MarkerActor::AS1MarkerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	RootComponent = DecalComp;
	DecalComp->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void AS1MarkerActor::Init(UMaterialInterface* InMaterial, FVector InSize, float InLifeTime, bool bFollowOwner)
{
    if (InMaterial)
    {
        DecalComp->SetDecalMaterial(InMaterial);
        DecalComp->DecalSize = InSize;
    }

    LifeTime = InLifeTime;
    bShouldFollowOwner = bFollowOwner;
}

// Called when the game starts or when spawned
void AS1MarkerActor::BeginPlay()
{
	Super::BeginPlay();
	
    DynMat = DecalComp->CreateDynamicMaterialInstance();
    if (DynMat)
        DynMat->SetScalarParameterValue("Fade", 1.f);

    if (LifeTime > 0.f)
    {
        const float TickInterval = 0.02f;
        GetWorldTimerManager().SetTimer(FadeTimer, this, &AS1MarkerActor::TickFade, TickInterval, true);
    }
}

void AS1MarkerActor::Tick(float DeltaTime)
{
    if (bShouldFollowOwner && FollowActor.IsValid())
    {
        SetActorLocation(FollowActor->GetActorLocation());
    }
}

// Called every frame
void AS1MarkerActor::TickFade()
{
    if (LifeTime <= 0.f)
        return;

    const float TickInterval = 0.02f;
    Elapsed += TickInterval;

    if (!DynMat)
    {
        Destroy();
        return;
    }

    float Alpha = FMath::Clamp(1.f - (Elapsed / LifeTime), 0.f, 1.f);
    DynMat->SetScalarParameterValue("Fade", Alpha);

    if (Alpha <= 0.f)
        Destroy();
}

