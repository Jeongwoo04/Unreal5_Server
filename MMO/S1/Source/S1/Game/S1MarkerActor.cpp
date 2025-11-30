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

    LifeTime = 0.f;
    Elapsed = 0.f;
    bShouldFollowOwner = false;
}

void AS1MarkerActor::Init(UMaterialInterface* InMaterial, FVector InSize, float InLifeTime, bool bFollowOwner)
{
    if (InMaterial)
    {
        DecalComp->SetDecalMaterial(InMaterial);
        DecalComp->DecalSize = InSize;
    }
    else
    {
        ;
    }


    LifeTime = InLifeTime;
    bShouldFollowOwner = bFollowOwner;
}

// Called when the game starts or when spawned
void AS1MarkerActor::BeginPlay()
{
	Super::BeginPlay();
    	
    if (DecalComp)
    {
        DynMat = DecalComp->CreateDynamicMaterialInstance();
        if (DynMat)
            DynMat->SetScalarParameterValue("Fade", 1.f);
    }

    if (LifeTime > 0.f)
    {
        const float TickInterval = 0.02f;

        // Timer Delegate 안전하게 람다로 바인딩
        FTimerDelegate FadeDelegate;
        FadeDelegate.BindLambda([this]()
            {
                if (!IsValid(this))
                    return;

                TickFade();
            });

        GetWorldTimerManager().SetTimer(FadeTimer, FadeDelegate, TickInterval, true);
    }
}

void AS1MarkerActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bShouldFollowOwner && FollowActor.IsValid())
    {
        SetActorLocation(FollowActor->GetActorLocation());
    }
}

// Called every frame
void AS1MarkerActor::TickFade()
{
    if (!IsValid(this) || !DynMat)
    {
        GetWorldTimerManager().ClearTimer(FadeTimer);
        SetLifeSpan(0.1f);
        return;
    }

    const float TickInterval = 0.02f;
    Elapsed += TickInterval;

    float Alpha = FMath::Clamp(1.f - (Elapsed / LifeTime), 0.f, 1.f);
    DynMat->SetScalarParameterValue("Fade", Alpha);

    if (Alpha <= 0.f)
    {
        GetWorldTimerManager().ClearTimer(FadeTimer);
        SetLifeSpan(0.1f);
    }
}

