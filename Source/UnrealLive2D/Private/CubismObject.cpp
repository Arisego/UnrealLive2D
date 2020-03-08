// Fill out your copyright notice in the Description page of Project Settings.

#include "CubismObject.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "FWPort/RawModel.h"
#include "Draw/CubismSepRender.h"

#include "UnrealLive2D.h"
#include "CubismBpLib.h"


UCubismObject::UCubismObject()
{
	msp_RawModel = nullptr;
	msp_Render = MakeShared<FCubismSepRender>();

}

UCubismObject::~UCubismObject()
{
}

void UCubismObject::Tick(float DeltaTime)
{
    if (_bPauseUpdate)
    {
        return;
    }

	_TimeSeconds += DeltaTime;
	if (_TimeSeconds < _TimeNextUpdate)
	{
		return;
	}

	const float tf_UpdateDelta = ((TickRate > 0) ? (1.0f / TickRate) : 0.05f);

    if (msp_RawModel.IsValid()&& IsValid(TargetRender))
    {
		/** 
		 * The update control is a bit dirty 
		 * If you want to change this logic, take care of GameThread/RenderThread yourself
		 * The render result will be flipping if GameThread updated vertex buffer while render is taking place.
		 */
		if (!msp_Render->RenderStates.bIsRendering)
		{
            check(!msp_Render->RenderStates.bIsRendering);
            msp_Render->RenderStates.bIsRendering = true;
            msp_Render->RenderStates.bRenderOver = false;

            msp_RawModel->OnUpdate(_TimeSeconds - _TimeLastUpdate);
            _TimeLastUpdate = _TimeSeconds;

            msp_Render->DrawSeparateToRenderTarget(GetWorld(), TargetRender);
            UE_LOG(LogCubism, Verbose, TEXT("Update: %f at %f || %f %f"), DeltaTime, GetWorld()->GetTimeSeconds(), _TimeSeconds, _TimeNextUpdate);
		}
		else
		{
            if (msp_Render->RenderStates.bRenderOver)
            {
                msp_Render->RenderStates.bIsRendering = false;
				msp_Render->RenderStates.bRenderOver = false;
				return;
            }
		}
    }

    _TimeNextUpdate = _TimeSeconds + tf_UpdateDelta;
}


bool UCubismObject::IsTickableInEditor() const
{
    return true;
}

TStatId UCubismObject::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCubismViewer, STATGROUP_Tickables);
}

bool UCubismObject::IsTickable() const
{
	if (HasAnyFlags(RF_ClassDefaultObject)) return  false;

	return true;
}

void UCubismObject::BeginDestroy()
{
	Super::BeginDestroy();

	msp_RawModel.Reset();
	msp_Render.Reset();
}

void UCubismObject::LoadFromPath(const FString& InPath)
{
	/** Try init cubism, init with module startup will break cook */
    UCubismBpLib::InitCubism();

	/** Load model */
	msp_RawModel.Reset();

	msp_RawModel = MakeShared<FRawModel>();
	const bool tb_ReadSuc = msp_RawModel->LoadAsset(InPath);
	if (!tb_ReadSuc)
	{
		msp_RawModel.Reset();
	}

	msp_Render->InitRender(msp_RawModel, ModelConfig);
    msp_RawModel->OnMotionPlayEnd.BindUObject(this, &UCubismObject::OnMotionPlayeEnd);
}

void UCubismObject::SetUpdatePaused(bool InIsPaused)
{
    _bPauseUpdate = InIsPaused;
}

//////////////////////////////////////////////////////////////////////////

void UCubismObject::OnMotionPlayeEnd_Implementation()
{
	UE_LOG(LogCubism, Log, TEXT("UCubismObject::OnMotionPlayeEnd"));
}

bool UCubismObject::OnTap(const FVector2D& InPos)
{
    if (!msp_RawModel.IsValid())
    {
        UE_LOG(LogCubism, Error, TEXT("UCubismObject::OnTap: No valid model"));
        return false;
    }

	bool tb_Handled = msp_RawModel->OnTap(InPos.X, InPos.Y);
	UE_LOG(LogCubism, Log, TEXT("UCubismObject::OnTap: %s %d"),
		*InPos.ToString(),
		tb_Handled ? 1 : 0
	);

	return tb_Handled;
}

void UCubismObject::PlayMotion(const FString& InName, const int32 InNo, const EMotionPriority InPriority /*= EMotionPriority::EMP_Normal*/)
{
    if (!msp_RawModel.IsValid())
    {
        UE_LOG(LogCubism, Error, TEXT("UCubismObject::PlayMotion: No valid model"));
        return;
    }

	UE_LOG(LogCubism, Log, TEXT("UCubismObject::PlayMotion: %s_%d | %d"), *InName, InNo, (int32)InPriority);
    msp_RawModel->PlayMotion(InName, InNo, (int32)InPriority);
}
