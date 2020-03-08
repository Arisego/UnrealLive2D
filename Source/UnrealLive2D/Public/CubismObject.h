// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <Tickable.h>
#include "Templates/SharedPointer.h"
#include "Components/ActorComponent.h"
#include "Draw/CubismConfig.h"
#include "CubismUeTypes.h"
#include "CubismObject.generated.h"

class UCubismDrawAble;
class UCubismAsset;
struct FModelInfo;

/**
 * 
 */
UCLASS(Blueprintable,BlueprintType)
class UNREALLIVE2D_API UCubismObject : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
    UCubismObject();
    ~UCubismObject();

    //////////////////////////////////////////////////////////////////////////
    /**	Tick object */
    virtual bool IsTickableInEditor() const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;
    //////////////////////////////////////////////////////////////////////////


	void BeginDestroy() override;

    //////////////////////////////////////////////////////////////////////////

public:
	UFUNCTION(BlueprintCallable, Category = "CubismModel")
	void LoadFromPath(const FString& InPath);

    UFUNCTION(BlueprintCallable)
    void SetUpdatePaused(bool InIsPaused);

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Render")
	class UTextureRenderTarget2D* TargetRender = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Render")
    FModelConfig ModelConfig;

private:
    TSharedPtr<class FRawModel> msp_RawModel;
    TSharedPtr<struct FCubismSepRender> msp_Render;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cubism|Update")
    int32 TickRate = 30;

private:
	float _TimeSeconds = 0.0f;
	float _TimeNextUpdate = 0.0f;
    float _TimeLastUpdate = 0.0f;

    bool _bPauseUpdate = false;

	//////////////////////////////////////////////////////////////////////////
public:
    /** Event called while motion play ends */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void OnMotionPlayeEnd();

    UFUNCTION(BlueprintCallable)
    bool OnTap(const FVector2D& InPos);

    /**
     * Motion control
     *
     * @param InName Group name of motion
     * @param InNo Group number of motion
     * @param InPriority Priori of motion to play
     */
    UFUNCTION(BlueprintCallable)
    void PlayMotion(const FString& InName, const int32 InNo, const EMotionPriority InPriority = EMotionPriority::EMP_Normal);
};
