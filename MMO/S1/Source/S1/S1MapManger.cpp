// Fill out your copyright notice in the Description page of Project Settings.


#include "S1MapManger.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

void US1MapManger::LoadMap(int32 MapId, float InCellSize)
{
	CellSize = InCellSize;

	FString FilePath = FPaths::ProjectDir() + FString::Printf(TEXT("Saved/CollisionMap/Map_%03d.txt"), MapId);
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
		return;

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);

	if (Lines.Num() < 1)
		return;

	TArray<FString> Header;
	Lines[0].ParseIntoArray(Header, TEXT(" "));
	MinX = FCString::Atoi(*Header[0]);
	MaxX = FCString::Atoi(*Header[1]);
	MinY = FCString::Atoi(*Header[2]);
	MaxY = FCString::Atoi(*Header[3]);

	int32 SizeX = MaxX - MinX + 1;
	int32 SizeY = MaxY - MinY + 1;

	Collisions.SetNum(SizeY);
	for (int32 y = 0; y < SizeY; ++y)
	{
		Collisions[y].SetNum(SizeX);
		for (int32 x = 0; x < SizeX; ++x)
		{
			Collisions[y][x] = Lines[y + 1][x] == '1';
		}
	}
}

bool US1MapManger::IsBlocked(float WorldX, float WorldY)
{
	int32 GridX = FMath::FloorToInt(WorldX / CellSize);
	int32 GridY = FMath::FloorToInt(WorldY / CellSize);

	int32 ArrayX = GridX - MinX;
	int32 ArrayY = GridY - MinY;

	if (Collisions.IsValidIndex(ArrayY) && Collisions[ArrayY].IsValidIndex(ArrayX))
		return Collisions[ArrayY][ArrayX];

	return true;
}
