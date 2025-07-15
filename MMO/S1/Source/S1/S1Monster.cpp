// Fill out your copyright notice in the Description page of Project Settings.


#include "S1Monster.h"

// Sets default values
AS1Monster::AS1Monster()
{
	PrimaryActorTick.bCanEverTick = true;
}

AS1Monster::~AS1Monster()
{
}

void AS1Monster::BeginPlay()
{
	Super::BeginPlay();
	
	{
		FVector Location = GetActorLocation();
		MonsterInfo.set_x(Location.X);
		MonsterInfo.set_y(Location.Y);
		MonsterInfo.set_z(Location.Z);
		MonsterInfo.set_yaw(GetControlRotation().Yaw);
	}
}

// Called every frame
void AS1Monster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AS1Monster::SetMonsterInfo(const Protocol::PosInfo& Info)
{

}

