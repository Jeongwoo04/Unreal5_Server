// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S1MarkerActor.generated.h"

UCLASS()
class S1_API AS1MarkerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AS1MarkerActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Marker")
    class UDecalComponent* DecalComp;

    void Init(UMaterialInterface* InMaterial, FVector InSize, float InLifeTime, bool bFollowOwner = false);

    // Follow ±â´É: ex) SkillRangeMarker
    void SetFollowActor(AActor* InActor) { FollowActor = InActor; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UMaterialInstanceDynamic* DynMat = nullptr;
    FTimerHandle FadeTimer;
    float LifeTime = 1.f;
    float Elapsed = 0.f;

    bool bShouldFollowOwner = false;
    TWeakObjectPtr<AActor> FollowActor;

    void TickFade();
};
