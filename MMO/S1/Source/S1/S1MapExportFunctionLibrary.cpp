// Fill out your copyright notice in the Description page of Project Settings.

#include "S1MapExportFunctionLibrary.h"

#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "DrawDebugHelpers.h"
#include "Editor.h"

void US1MapExportFunctionLibrary::ExportCollisionMap(int32 MapId, float CellSize)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("World is null."));
        return;
    }

    // 1단계: BaseFloor (가장 낮은 Floor) 찾기 및 위치/범위 저장
    AActor* BaseFloor = nullptr;
    FVector BaseOrigin, BaseExtent;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor->ActorHasTag(TEXT("Floor")))
            continue;

        FVector Origin, Extent;
        Actor->GetActorBounds(false, Origin, Extent);
        float BottomZ = Origin.Z - Extent.Z;

        if (!BaseFloor || BottomZ < BaseOrigin.Z - BaseExtent.Z)
        {
            BaseFloor = Actor;
            BaseOrigin = Origin;
            BaseExtent = Extent;
        }
    }

    if (!BaseFloor)
    {
        UE_LOG(LogTemp, Warning, TEXT("No Floor actor found."));
        return;
    }

    // 2단계: BaseFloor 영역에 padding 1칸씩 추가해서 그리드 범위 계산
    int32 BaseMinX = FMath::FloorToInt((BaseOrigin.X - BaseExtent.X) / CellSize);
    int32 BaseMaxX = FMath::FloorToInt((BaseOrigin.X + BaseExtent.X) / CellSize);
    int32 BaseMinY = FMath::FloorToInt((BaseOrigin.Y - BaseExtent.Y) / CellSize);
    int32 BaseMaxY = FMath::FloorToInt((BaseOrigin.Y + BaseExtent.Y) / CellSize);

    int32 MinX = BaseMinX - 1;
    int32 MaxX = BaseMaxX + 1;
    int32 MinY = BaseMinY - 1;
    int32 MaxY = BaseMaxY + 1;

    int32 SizeX = MaxX - MinX + 1;
    int32 SizeY = MaxY - MinY + 1;

    // 3단계: CollisionData 배열 초기화 (true = 충돌, false = 이동 가능)
    TArray<TArray<bool>> CollisionData;
    CollisionData.SetNum(SizeY);
    for (int32 y = 0; y < SizeY; ++y)
        CollisionData[y].Init(true, SizeX);

    // 4단계: BaseFloor 영역만 이동 가능(false)로 설정
    for (int32 y = BaseMinY; y <= BaseMaxY; ++y)
    {
        for (int32 x = BaseMinX; x <= BaseMaxX; ++x)
        {
            int32 ArrayX = x - MinX;
            int32 ArrayY = y - MinY;

            if (ArrayX >= 0 && ArrayX < SizeX && ArrayY >= 0 && ArrayY < SizeY)
                CollisionData[ArrayY][ArrayX] = false;
        }
    }

    // 5단계: BaseFloor보다 높은 Floor에 대해 라인트레이스 검사하여 충돌 업데이트
    const float BaseFloorBottomZ = BaseOrigin.Z - BaseExtent.Z;
    const float TraceStartZ = BaseFloorBottomZ + 1000.f;
    const float TraceEndZ = BaseFloorBottomZ - 100.f;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor->ActorHasTag(TEXT("Floor")))
            continue;

        // BaseFloor 제외
        if (Actor == BaseFloor)
            continue;

        FVector Origin, Extent;
        Actor->GetActorBounds(false, Origin, Extent);
        float FloorBottomZ = Origin.Z - Extent.Z;

        // BaseFloor보다 10cm 이상 높은 Floor만 검사
        if (FloorBottomZ <= BaseFloorBottomZ + 10.f)
            continue;

        int32 FloorMinX = FMath::FloorToInt((Origin.X - Extent.X) / CellSize);
        int32 FloorMaxX = FMath::FloorToInt((Origin.X + Extent.X) / CellSize);
        int32 FloorMinY = FMath::FloorToInt((Origin.Y - Extent.Y) / CellSize);
        int32 FloorMaxY = FMath::FloorToInt((Origin.Y + Extent.Y) / CellSize);

        for (int32 y = FloorMinY; y <= FloorMaxY; ++y)
        {
            if (y < MinY || y > MaxY) continue;
            int32 ArrayY = y - MinY;

            for (int32 x = FloorMinX; x <= FloorMaxX; ++x)
            {
                if (x < MinX || x > MaxX) continue;
                int32 ArrayX = x - MinX;

                FVector TraceStart(x * CellSize + CellSize * 0.5f, y * CellSize + CellSize * 0.5f, TraceStartZ);
                FVector TraceEnd(x * CellSize + CellSize * 0.5f, y * CellSize + CellSize * 0.5f, TraceEndZ);

                FHitResult HitResult;
                bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility);

                if (bHit && HitResult.Location.Z > BaseFloorBottomZ + 50.f)
                {
                    CollisionData[ArrayY][ArrayX] = true; // 충돌 처리
                }
            }
        }
    }

    // 6단계: CollisionMap 파일 저장
    FString SaveDir = FPaths::ProjectDir() + TEXT("Saved/CollisionMap");
    IFileManager::Get().MakeDirectory(*SaveDir, true);

    FString FileName = FString::Printf(TEXT("/Map_%03d.txt"), MapId);
    FString SavePath = SaveDir + FileName;

    FString Output;
    Output += FString::Printf(TEXT("%d %d %d %d\n"), MinX, MaxX, MinY, MaxY);

    for (int32 y = 0; y < SizeY; ++y)
    {
        for (int32 x = 0; x < SizeX; ++x)
        {
            Output += CollisionData[y][x] ? TEXT("1") : TEXT("0");
        }
        Output += TEXT("\n");
    }

    if (FFileHelper::SaveStringToFile(Output, *SavePath))
    {
        UE_LOG(LogTemp, Log, TEXT("Collision map saved to: %s"), *SavePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save collision map to: %s"), *SavePath);
    }
}
