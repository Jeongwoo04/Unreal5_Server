// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "S1MapManger.generated.h"

/**
 * 
 */
UCLASS()
class S1_API US1MapManger : public UObject
{
	GENERATED_BODY()
	
public:
	void LoadMap(int32 MapId, float InCellSize);
	bool IsBlocked(float WorldX, float WorldY);

private:
	TArray<TArray<bool>> Collisions;
	int32 MinX, MaxX, MinY, MaxY;
	float CellSize;
};
