// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1Projectile.h"

// Sets default values
AS1Projectile::AS1Projectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

AS1Projectile::~AS1Projectile()
{

}

void AS1Projectile::BeginPlay()
{
	Super::BeginPlay();
	
	{
		FVector Location = GetActorLocation();
		PosInfo.set_x(Location.X);
		PosInfo.set_y(Location.Y);
		PosInfo.set_z(Location.Z);
		PosInfo.set_yaw(GetActorRotation().Yaw);
		PosInfo.set_state(Protocol::STATE_MACHINE_MOVING);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Projectile Is Spawn. Pos %f , %f , %f"), PosInfo.x(), PosInfo.y(), PosInfo.z()));
	}
}

// Called every frame
void AS1Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector PreviousLoc = GetActorLocation();

	SetActorRotation(TargetRot);

	FVector NewLoc = FMath::VInterpConstantTo(PreviousLoc, TargetPos, DeltaTime, PosInfo.speed());
	SetActorLocation(NewLoc);

	float DistToTarget = FVector::Dist(NewLoc, TargetPos);
	if (DistToTarget <= PosInfo.speed() * DeltaTime)
	{
		NewLoc = TargetPos;
	}

	SetActorLocation(NewLoc);
}

void AS1Projectile::Move(const Protocol::PosInfo& Info)
{
	TargetPos = FVector(Info.x(), Info.y(), Info.z());
	TargetRot = FRotator(0.f, Info.yaw(), 0.f);
}

void AS1Projectile::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (PosInfo.object_id() != 0)
	{
		assert(PosInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);

	Move(Info);
}

void AS1Projectile::SetCaster(AS1Creature* InCaster)
{
	Caster = InCaster;
}

void AS1Projectile::OnHit()
{

}
