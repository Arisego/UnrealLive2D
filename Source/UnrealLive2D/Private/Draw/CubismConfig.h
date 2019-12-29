#pragma once

#include "CubismConfig.generated.h"

/**
 * Model config goes here
 */
USTRUCT(BlueprintType)
struct FModelConfig
{
	GENERATED_USTRUCT_BODY()

public:
	FModelConfig();

public:

	/**
	 * If Opens
	 * We will try to render all mask in a single texture if possible
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTryLowPreciseMask = true;

};