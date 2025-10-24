// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1Field.h"

// Sets default values
AS1Field::AS1Field()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

AS1Field::~AS1Field()
{

}

// Called when the game starts or when spawned
void AS1Field::BeginPlay()
{
	Super::BeginPlay();
	
	{
		FVector Location = GetActorLocation();
		PosInfo.set_x(Location.X);
		PosInfo.set_y(Location.Y);
		PosInfo.set_z(Location.Z);
		PosInfo.set_yaw(GetActorRotation().Yaw);
		PosInfo.set_state(Protocol::STATE_MACHINE_IDLE);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Field Is Spawn. Pos %f , %f , %f"), PosInfo.x(), PosInfo.y(), PosInfo.z()));
	}
}

// Called every frame
void AS1Field::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AS1Field::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (PosInfo.object_id() != 0)
	{
		assert(PosInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);
}

void AS1Field::SetCaster(AS1Creature* InCaster)
{
	Caster = InCaster;
}

void AS1Field::OnHit()
{

}

