// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CubismUeTypes.generated.h"

UENUM(BlueprintType)
enum class EMotionPriority : uint8 
{
    EMP_None = 0 UMETA(DisplayName = "None"),
    EMP_Idle = 1 UMETA(DisplayName = "Idle"),
    EMP_Normal = 2 UMETA(DisplayName = "Normal"),
    EMP_Force = 3 UMETA(DisplayName = "Force")
};