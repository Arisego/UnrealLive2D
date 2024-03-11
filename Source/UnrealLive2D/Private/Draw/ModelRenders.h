#include "CubismFramework.hpp"
#include "Model/CubismModel.hpp"
#include "Type/CubismBasicType.hpp"
#include "RHIResources.h"
#include "RenderResource.h"
#include "RHICommandList.h"
#include "RHIDefinitions.h"
#include "UObject/NoExportTypes.h"

//////////////////////////////////////////////////////////////////////////
// See CommonRenderResources.cpp

/** The vertex data used to filter a texture. */
struct FCubismVertex
{
    FVector2f Position;
    FVector2f UV;

    FCubismVertex(float x, float y, float z, float w)
        : Position(x, y), UV(z, w)
    {
    }
};

/** The filter vertex declaration resource type. */
class FCubismVertexDeclaration : public FRenderResource
{
public:
    FVertexDeclarationRHIRef VertexDeclarationRHI;

    /** Destructor. */
    virtual ~FCubismVertexDeclaration() {}

    virtual void InitRHI(FRHICommandListBase &RHICmdList) override;

    virtual void ReleaseRHI();
};

extern TGlobalResource<FCubismVertexDeclaration> GCubismVertexDeclaration;
class FRHITexture;

/** Test Texture Draw */
class FCubismVertexBuffer : public FVertexBuffer
{
public:
    /**
     * Initialize the RHI for this rendering resource
     */
    virtual void InitRHI(FRHICommandListBase &RHICmdList) override;
};

extern TGlobalResource<FCubismVertexBuffer> GCubismVertexScreenBuffer;

//////////////////////////////////////////////////////////////////////////
struct FCubismRenderState;

struct FModelRenders
{
    static void SetUpBlendMode(
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        FGraphicsPipelineStateInitializer &GraphicsPSOInit);

    static void FillVertexBuffer(
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        FBufferRHIRef ScratchVertexBufferRHI,
        FCubismRenderState *tp_States,
        FRHICommandListImmediate &RHICmdList);

    static class UTexture2D *GetTexture(
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        FCubismRenderState *tp_States);

    static FMatrix44f ConvertCubismMatrix(Csm::CubismMatrix44 &InCubismMartix);

    /** Normal Drawable draw */
    static void DrawSepNormal(
        class FTextureRenderTargetResource *OutTextureRenderTargetResource,
        FRHICommandListImmediate &RHICmdList,
        ERHIFeatureLevel::Type FeatureLevel,
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        struct FCubismRenderState *tp_States);

    //////////////////////////////////////////////////////////////////////////
    /** Masked Drawable Draw */
    static void DrawSepMask(
        FTextureRenderTargetResource *OutTextureRenderTargetResource,
        FRHICommandListImmediate &RHICmdList, ERHIFeatureLevel::Type FeatureLevel,
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        FCubismRenderState *tp_States,
        class CubismClippingContext *clipContext);

    static void _DrawSepMask_Normal(
        FTextureRenderTargetResource *OutTextureRenderTargetResource,
        FRHICommandListImmediate &RHICmdList, ERHIFeatureLevel::Type FeatureLevel,
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        FCubismRenderState *tp_States,
        class CubismClippingContext *clipContext);

    static void _DrawSepMask_Invert(
        FTextureRenderTargetResource *OutTextureRenderTargetResource,
        FRHICommandListImmediate &RHICmdList, ERHIFeatureLevel::Type FeatureLevel,
        Csm::CubismModel *tp_Model,
        const Csm::csmInt32 drawableIndex,
        FCubismRenderState *tp_States,
        class CubismClippingContext *clipContext);

    //////////////////////////////////////////////////////////////////////////
    /** Render Mask to buffer, low precise version */
    static void RenderMask_Full(
        FCubismRenderState *tp_States,
        FRHICommandListImmediate &RHICmdList,
        class CubismClippingManager_UE *_clippingManager,
        ERHIFeatureLevel::Type FeatureLevel,
        Csm::CubismModel *tp_Model);

    /** Render Mask to buffer, high precise version */
    static void RenderMask_Single(
        FCubismRenderState *tp_States,
        FRHICommandListImmediate &RHICmdList,
        ERHIFeatureLevel::Type FeatureLevel,
        Csm::CubismModel *tp_Model,
        class CubismClippingContext *clipContext);

    /** Draw Texture, for debug */
    static void DrawTestTexture(
        FTextureRenderTargetResource *OutTextureRenderTargetResource,
        FRHICommandListImmediate &RHICmdList,
        ERHIFeatureLevel::Type FeatureLevel,
        struct FCubismRenderState *tp_States);

    /**
     * Copied from RHICommandList.h, as TransitionResource has been deprecated
     *
     * @see: RHICommandList.h, UE_DEPRECATED(5.1, "TransitionResource has been deprecated. Use Transition instead.")
     */
    static TArray<FRHITransitionInfo, TInlineAllocator<2>> ConvertTransitionResource(FExclusiveDepthStencil DepthStencilMode, FRHITexture *DepthTexture);
};
