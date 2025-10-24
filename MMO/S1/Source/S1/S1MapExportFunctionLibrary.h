// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "S1MapExportFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class S1_API US1MapExportFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()	

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Server Map Export")
	static void ExportCollisionMap(int32 MapId = 1, float CellSize = 100.f);
#endif
};
