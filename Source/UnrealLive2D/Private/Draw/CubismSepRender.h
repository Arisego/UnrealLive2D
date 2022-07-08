// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "CubismFramework.hpp"
#include "Type/csmVector.hpp"
#include "CubismConfig.h"
#include "RHIResources.h"


class UTexture2D;

struct FCubismRenderState
{
    Csm::csmVector<Csm::csmInt32>                 _sortedDrawableIndexList;       ///< 描画オブジェクトのインデックスを描画順に並べたリスト

    TMap<int32, FBufferRHIRef> IndexBuffers;
    TMap<int32, FBufferRHIRef> VertexBuffers;
    TMap<int32, int32> VertexCount;

    TArray<UTexture2D*> Textures;

    TSharedPtr<class CubismClippingManager_UE> _ClippingManager;

    FTexture2DRHIRef MaskBuffer;

    FModelConfig RenderModelConfig;

    bool bIsRendering = false;
    bool bRenderOver = false;

public:
    void ClearStates();

    void NoLowPreciseMask(bool val) { bNoLowPreciseMask = val; }
    bool Get_UseHighPreciseMask() const;

private:
    bool bNoLowPreciseMask = false;
};

/** 
 * 
 */
struct FCubismSepRender
{ 
public:

	void DrawSeparateToRenderTarget(
		class UWorld* World,
		class UTextureRenderTarget2D* OutputRenderTarget) const;

    void InitRender(TSharedPtr<class FRawModel> InModel, const FModelConfig& InModelConfig);

private:
    void LoadTextures();

    void UnLoadTextures();


private:
	TSharedPtr<class FRawModel> _Model;

public:
    /** TODO: We are cheating */
    mutable FCubismRenderState RenderStates;

};