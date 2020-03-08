#pragma once

#include "CoreMinimal.h"

#include "CubismFramework.hpp"
#include "Model/CubismUserModel.hpp"
#include "Motion/CubismMotionQueueManager.hpp"
#include "CubismUeTypes.h"


class FRawModel : public Csm::CubismUserModel
{
public:
    FRawModel();
    ~FRawModel();

public:
    bool LoadAsset(const FString& InPath);

    void OnUpdate(float InDeltaTime);
	
public:
    Csm::ICubismModelSetting* Get_ModelSetting() const { return _modelSetting; };
    const FString& Get_HomeDir() const { return _HomeDir; };

private:
    Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority);
    Csm::CubismMotionQueueEntryHandle StartRandomMotion(const Csm::csmChar* group, Csm::csmInt32 priority);

    void SetExpression(const Csm::csmChar* expressionID);
    void SetRandomExpression();

private:
    FString _HomeDir;

    Csm::ICubismModelSetting* _modelSetting;
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   _expressions;

    const Csm::CubismId* _idParamAngleX; ///< パラメータID: ParamAngleX
    const Csm::CubismId* _idParamAngleY; ///< パラメータID: ParamAngleX
    const Csm::CubismId* _idParamAngleZ; ///< パラメータID: ParamAngleX
    const Csm::CubismId* _idParamBodyAngleX; ///< パラメータID: ParamBodyAngleX

    Csm::csmVector<Csm::CubismIdHandle> _eyeBlinkIds; ///<　モデルに設定されたまばたき機能用パラメータID

    Csm::csmVector<Csm::CubismIdHandle> _lipSyncIds; ///< モデルに設定されたリップシンク機能用パラメータID

    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*>   _motions; ///< 読み込まれているモーションのリスト

    const Csm::CubismId* _idParamEyeBallX; ///< パラメータID: ParamEyeBallX
    const Csm::CubismId* _idParamEyeBallY; ///< パラメータID: ParamEyeBallXY

private:
    /**
     * @brief   モーションデータをグループ名から一括でロードする。<br>
     *           モーションデータの名前は内部でModelSettingから取得する。
     *
     * @param[in]   group  モーションデータのグループ名
     */
    void PreloadMotionGroup(const Csm::csmChar* group);

    void ReleaseMotions();
    void ReleaseExpressions();
    float _userTimeSeconds;

    bool HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y);

    //////////////////////////////////////////////////////////////////////////
    /** Link to UE4 */
public:
    FSimpleDelegate OnMotionPlayEnd;

    void PlayMotion(const FString& InName, const int32 InNo, const int32 InPriority);

    bool OnTap(float x, float y);
};