#include "RawModel.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "CubismModelSettingJson.hpp"
#include "CubismFramework.hpp"
#include "CubismDefaultParameterId.hpp"
#include "Id/CubismIdManager.hpp"
#include "Utils/CubismString.hpp"
#include "Motion/CubismMotion.hpp"

#include "UnrealLive2D.h"

using namespace Live2D::Cubism::Framework;

// 外部定義ファイル(json)と合わせる
const csmChar* MotionGroupIdle = "Idle"; // アイドリング
const csmChar* MotionGroupTapBody = "TapBody"; // 体をタップしたとき

// モーションの優先度定数
const csmInt32 PriorityNone = 0;
const csmInt32 PriorityIdle = 1;
const csmInt32 PriorityNormal = 2;
const csmInt32 PriorityForce = 3;

// 外部定義ファイル(json)と合わせる
const csmChar* HitAreaNameHead = "Head";
const csmChar* HitAreaNameBody = "Body";

FRawModel::FRawModel()
{
    _idParamAngleX = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamAngleX);
    _idParamAngleY = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamAngleY);
    _idParamAngleZ = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamAngleZ);
    _idParamBodyAngleX = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamBodyAngleX);
    _idParamEyeBallX = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamEyeBallX);
    _idParamEyeBallY = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamEyeBallY);
}

FRawModel::~FRawModel()
{
    UE_LOG(LogCubism, Log, TEXT("FRawModel::~FRawModel"));
	
	ReleaseMotions();
    ReleaseExpressions();

    // This seams not done in official sample
    //for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); i++)
    //{
    //    const csmChar* group = _modelSetting->GetMotionGroupName(i);
    //    ReleaseMotionGroup(group);
    //}

    delete _modelSetting;
}

bool FRawModel::LoadAsset(const FString& InPath)
{
    FString tstr_FileName;
    FString tstr_FileExtentsion;
    FPaths::Split(InPath, _HomeDir, tstr_FileName, tstr_FileExtentsion);

    UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Path info [%s]/[%s].[%s]"), 
        *_HomeDir,
        *tstr_FileName,
        *tstr_FileExtentsion
    );

    UE_LOG(LogCubism,Log, TEXT("%s"), *FPaths::ProjectPluginsDir());


    TArray<uint8> tarr_FileData;
    if (!FFileHelper::LoadFileToArray(tarr_FileData, *InPath))
    {
        UE_LOG(LogCubism, Error, TEXT("FRawModel::LoadAsset: Read <%s> failed!"));
        return false;
    }
    
    _modelSetting = new CubismModelSettingJson(tarr_FileData.GetData(), tarr_FileData.Num());
    //delete Reader;


    csmString _modelHomeDir = TCHAR_TO_UTF8(*_HomeDir);


    //Cubism Model
    if (strcmp(_modelSetting->GetModelFileName(), "") != 0)
    {
        csmString path = _modelSetting->GetModelFileName();

        FString tstr_ModelPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

        TArray<uint8> tarr_ModelFile;
        const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_ModelPath);
        if (tb_ReadSuc)
        {
            LoadModel(tarr_ModelFile.GetData(), tarr_ModelFile.Num());
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Read model <%s> res=%d"),
            *tstr_ModelPath,
            tb_ReadSuc ? 1 : 0
        );

        UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: %f, %f"), _model->GetCanvasWidth(), _model->GetCanvasHeight());

    }

    //Expression
    if (_modelSetting->GetExpressionCount() > 0)
    {
        const csmInt32 count = _modelSetting->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++)
        {
            csmString name = _modelSetting->GetExpressionName(i);
            csmString path = _modelSetting->GetExpressionFileName(i);

            FString tstr_ExpressionPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

            TArray<uint8> tarr_ModelFile;
            const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_ExpressionPath);
            if (tb_ReadSuc)
            {
                ACubismMotion* motion = LoadExpression(tarr_ModelFile.GetData(), tarr_ModelFile.Num(), name.GetRawString());

                if (_expressions[name] != NULL)
                {
                    ACubismMotion::Delete(_expressions[name]);
                    _expressions[name] = NULL;
                }
                _expressions[name] = motion;


                UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Read expression <%s><%s> res=%d"),
                    *tstr_ExpressionPath,
                    UTF8_TO_TCHAR(name.GetRawString()),
                    tb_ReadSuc ? 1 : 0
                );
            }
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Expression readed %d"),count);
    }

    //Physics
    if (strcmp(_modelSetting->GetPhysicsFileName(), "") != 0)
    {
        csmString path = _modelSetting->GetPhysicsFileName();

        FString tstr_PhysicsPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

        TArray<uint8> tarr_ModelFile;
        const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_PhysicsPath);
        if (tb_ReadSuc)
        {
            LoadPhysics(tarr_ModelFile.GetData(), tarr_ModelFile.Num());
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Read Physics <%s> res=%d"),
            *tstr_PhysicsPath,
            tb_ReadSuc ? 1 : 0
        );
    }

    //Pose
    if (strcmp(_modelSetting->GetPoseFileName(), "") != 0)
    {
        csmString path = _modelSetting->GetPoseFileName();
        FString tstr_TempReadPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

        TArray<uint8> tarr_ModelFile;
        const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_TempReadPath);

        if (tb_ReadSuc)
        {
            LoadPose(tarr_ModelFile.GetData(), tarr_ModelFile.Num());
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Read Pose <%s> res=%d"),
            *tstr_TempReadPath,
            tb_ReadSuc ? 1 : 0
        );
    }

    //EyeBlink
    if (_modelSetting->GetEyeBlinkParameterCount() > 0)
    {
        _eyeBlink = CubismEyeBlink::Create(_modelSetting);
    }

    //Breath
    {
        _breath = CubismBreath::Create();

        csmVector<CubismBreath::BreathParameterData> breathParameters;

        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleX, 0.0f, 15.0f, 6.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleY, 0.0f, 8.0f, 3.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleZ, 0.0f, 10.0f, 5.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamBodyAngleX, 0.0f, 4.0f, 15.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamBreath), 0.5f, 0.5f, 3.2345f, 0.5f));

        _breath->SetParameters(breathParameters);
    }

    //UserData
    if (strcmp(_modelSetting->GetUserDataFile(), "") != 0)
    {
        csmString path = _modelSetting->GetUserDataFile();
        FString tstr_TempReadPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

        TArray<uint8> tarr_ModelFile;
        const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_TempReadPath);

        if (tb_ReadSuc)
        {
            LoadUserData(tarr_ModelFile.GetData(), tarr_ModelFile.Num());
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Read UserData <%s> res=%d"),
            *tstr_TempReadPath,
            tb_ReadSuc ? 1 : 0
        );
    }

    // EyeBlinkIds
    {
        csmInt32 eyeBlinkIdCount = _modelSetting->GetEyeBlinkParameterCount();
        for (csmInt32 i = 0; i < eyeBlinkIdCount; ++i)
        {
            _eyeBlinkIds.PushBack(_modelSetting->GetEyeBlinkParameterId(i));
        }
    }

    // LipSyncIds
    {
        csmInt32 lipSyncIdCount = _modelSetting->GetLipSyncParameterCount();
        for (csmInt32 i = 0; i < lipSyncIdCount; ++i)
        {
            _lipSyncIds.PushBack(_modelSetting->GetLipSyncParameterId(i));
        }
    }

    //Layout
    csmMap<csmString, csmFloat32> layout;
    _modelSetting->GetLayoutMap(layout);
    _modelMatrix->SetupFromLayout(layout);

    _model->SaveParameters();

    for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); i++)
    {
        const csmChar* group = _modelSetting->GetMotionGroupName(i);
        PreloadMotionGroup(group);
    }

    _motionManager->StopAllMotions();

    //////////////////////////////////////////////////////////////////////////

    UE_LOG(LogCubism, Log, TEXT("FRawModel::LoadAsset: Done"));
    return true;
}

void FRawModel::OnUpdate(float InDeltaTime)
{
    _userTimeSeconds += InDeltaTime;

    _dragManager->Update(InDeltaTime);
    _dragX = _dragManager->GetX();
    _dragY = _dragManager->GetY();

    // モーションによるパラメータ更新の有無
    csmBool motionUpdated = false;

    //-----------------------------------------------------------------
    _model->LoadParameters(); // 前回セーブされた状態をロード
    if (_motionManager->IsFinished())
    {
        OnMotionPlayEnd.ExecuteIfBound();

        // モーションの再生がない場合、待機モーションの中からランダムで再生する
        StartRandomMotion(MotionGroupIdle, PriorityIdle);
    }
    else
    {
        motionUpdated = _motionManager->UpdateMotion(_model, InDeltaTime); // モーションを更新
    }
    _model->SaveParameters(); // 状態を保存
    //-----------------------------------------------------------------

    // まばたき
    if (!motionUpdated)
    {
        if (_eyeBlink != NULL)
        {
            // メインモーションの更新がないとき
            _eyeBlink->UpdateParameters(_model, InDeltaTime); // 目パチ
        }
    }

    if (_expressionManager != NULL)
    {
        _expressionManager->UpdateMotion(_model, InDeltaTime); // 表情でパラメータ更新（相対変化）
    }

    //ドラッグによる変化
    //ドラッグによる顔の向きの調整
    _model->AddParameterValue(_idParamAngleX, _dragX * 30); // -30から30の値を加える
    _model->AddParameterValue(_idParamAngleY, _dragY * 30);
    _model->AddParameterValue(_idParamAngleZ, _dragX * _dragY * -30);

    //ドラッグによる体の向きの調整
    _model->AddParameterValue(_idParamBodyAngleX, _dragX * 10); // -10から10の値を加える

    //ドラッグによる目の向きの調整
    _model->AddParameterValue(_idParamEyeBallX, _dragX); // -1から1の値を加える
    _model->AddParameterValue(_idParamEyeBallY, _dragY);

    // 呼吸など
    if (_breath != NULL)
    {
        _breath->UpdateParameters(_model, InDeltaTime);
    }

    // 物理演算の設定
    if (_physics != NULL)
    {
        _physics->Evaluate(_model, InDeltaTime);
    }

    // リップシンクの設定
    if (_lipSync)
    {
        csmFloat32 value = 0; // リアルタイムでリップシンクを行う場合、システムから音量を取得して0〜1の範囲で値を入力します。

        for (csmUint32 i = 0; i < _lipSyncIds.GetSize(); ++i)
        {
            _model->AddParameterValue(_lipSyncIds[i], value, 0.8f);
        }
    }

    // ポーズの設定
    if (_pose != NULL)
    {
        _pose->UpdateParameters(_model, InDeltaTime);
    }

    _model->Update();
}

void FRawModel::PreloadMotionGroup(const Csm::csmChar* group)
{
    const csmInt32 count = _modelSetting->GetMotionCount(group);

    for (csmInt32 i = 0; i < count; i++)
    {
        //ex) idle_0
        csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
        csmString path = _modelSetting->GetMotionFileName(group, i);


        FString tstr_TempReadPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

        TArray<uint8> tarr_ModelFile;
        const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_TempReadPath);
        if (tb_ReadSuc)
        {
            CubismMotion* tmpMotion = static_cast<CubismMotion*>(LoadMotion(tarr_ModelFile.GetData(), tarr_ModelFile.Num(), name.GetRawString()));

            csmFloat32 fadeTime = _modelSetting->GetMotionFadeInTimeValue(group, i);
            if (fadeTime >= 0.0f)
            {
                tmpMotion->SetFadeInTime(fadeTime);
            }

            fadeTime = _modelSetting->GetMotionFadeOutTimeValue(group, i);
            if (fadeTime >= 0.0f)
            {
                tmpMotion->SetFadeOutTime(fadeTime);
            }
            tmpMotion->SetEffectIds(_eyeBlinkIds, _lipSyncIds);

            if (_motions[name] != NULL)
            {
                ACubismMotion::Delete(_motions[name]);
            }
            _motions[name] = tmpMotion;
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::PreloadMotionGroup: Read <%s><%s> res=%d"),
            *tstr_TempReadPath,
            UTF8_TO_TCHAR(name.GetRawString()),
            tb_ReadSuc ? 1 : 0
        );
    }

    UE_LOG(LogCubism, Log, TEXT("FRawModel::PreloadMotionGroup: Complete with %d"), count);
}

CubismMotionQueueEntryHandle FRawModel::StartMotion(const csmChar* group, csmInt32 no, csmInt32 priority)
{
    if (priority == PriorityForce)
    {
        _motionManager->SetReservePriority(priority);
    }
    else if (!_motionManager->ReserveMotion(priority))
    {
        UE_LOG(LogCubism, Log, TEXT("FRawModel::StartMotion: Can't start motion."));
        return InvalidMotionQueueEntryHandleValue;
    }

    const csmString fileName = _modelSetting->GetMotionFileName(group, no);

    //ex) idle_0
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
    CubismMotion* motion = static_cast<CubismMotion*>(_motions[name.GetRawString()]);
    csmBool autoDelete = false;

    if (motion == NULL)
    {
        csmString path = fileName;


        FString tstr_TempReadPath = _HomeDir / UTF8_TO_TCHAR(path.GetRawString());

        TArray<uint8> tarr_ModelFile;
        const bool tb_ReadSuc = FFileHelper::LoadFileToArray(tarr_ModelFile, *tstr_TempReadPath);

        if (tb_ReadSuc)
        {

            motion = static_cast<CubismMotion*>(LoadMotion(tarr_ModelFile.GetData(), tarr_ModelFile.Num(), NULL));
            csmFloat32 fadeTime = _modelSetting->GetMotionFadeInTimeValue(group, no);
            if (fadeTime >= 0.0f)
            {
                motion->SetFadeInTime(fadeTime);
            }

            fadeTime = _modelSetting->GetMotionFadeOutTimeValue(group, no);
            if (fadeTime >= 0.0f)
            {
                motion->SetFadeOutTime(fadeTime);
            }
            motion->SetEffectIds(_eyeBlinkIds, _lipSyncIds);
            autoDelete = true; // 終了時にメモリから削除
        }

        UE_LOG(LogCubism, Log, TEXT("FRawModel::StartMotion: Read <%s> res=%d"),
            *tstr_TempReadPath,
            tb_ReadSuc ? 1 : 0
        );


    }

    //voice is not supported
    //csmString voice = _modelSetting->GetMotionSoundFileName(group, no);
    //if (strcmp(voice.GetRawString(), "") != 0)
    //{
    //    //csmString path = voice;
    //    //path = _modelHomeDir + path;
    //}

    UE_LOG(LogCubism, Log,  TEXT("FRawModel::StartMotion: Start [%s_%d]"), UTF8_TO_TCHAR(group), no);
    return  _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

CubismMotionQueueEntryHandle FRawModel::StartRandomMotion(const csmChar* group, csmInt32 priority)
{
    if (_modelSetting->GetMotionCount(group) == 0)
    {
        return InvalidMotionQueueEntryHandleValue;
    }

    csmInt32 no = rand() % _modelSetting->GetMotionCount(group);

    return StartMotion(group, no, priority);
}

void FRawModel::SetExpression(const Csm::csmChar* expressionID)
{
    ACubismMotion* motion = _expressions[expressionID];
    check(_expressionManager);

    if (motion != NULL)
    {
        _expressionManager->StartMotionPriority(motion, false, PriorityForce);
    }
    else
    {
        UE_LOG(LogCubism, Warning, TEXT("FRawModel::SetExpression: No expression found [%s]"), UTF8_TO_TCHAR(expressionID));
    }
}

void FRawModel::SetRandomExpression()
{
    if (_expressions.GetSize() == 0)
    {
        return;
    }

    csmInt32 no = rand() % _expressions.GetSize();
    csmMap<csmString, ACubismMotion*>::const_iterator map_ite;
    csmInt32 i = 0;
    for (map_ite = _expressions.Begin(); map_ite != _expressions.End(); map_ite++)
    {
        if (i == no)
        {
            csmString name = (*map_ite).First;
            SetExpression(name.GetRawString());
            return;
        }
        i++;
    }
}

void FRawModel::ReleaseMotions()
{
    for (csmMap<csmString, ACubismMotion*>::const_iterator iter = _motions.Begin(); iter != _motions.End(); ++iter)
    {
        ACubismMotion::Delete(iter->Second);
    }

    _motions.Clear();
}

void FRawModel::ReleaseExpressions()
{
    for (csmMap<csmString, ACubismMotion*>::const_iterator iter = _expressions.Begin(); iter != _expressions.End(); ++iter)
    {
        ACubismMotion::Delete(iter->Second);
    }

    _expressions.Clear();
}

bool FRawModel::HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y)
{
    // 透明時は当たり判定なし。
    if (_opacity < 1)
    {
        return false;
    }


    const csmInt32 count = _modelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++)
    {
        if (strcmp(_modelSetting->GetHitAreaName(i), hitAreaName) == 0)
        {
            const CubismIdHandle drawID = _modelSetting->GetHitAreaId(i);
            return IsHit(drawID, x, y);
        }
    }
    return false; // 存在しない場合はfalse
}

void FRawModel::PlayMotion(const FString& InName, const int32 InNo, const int32 InPriority)
{
    StartMotion(TCHAR_TO_UTF8(*InName), InNo, (int32)InPriority);
}

bool FRawModel::OnTap(csmFloat32 x, csmFloat32 y)
{

    if (HitTest(HitAreaNameHead, x, y))
    {
        SetRandomExpression();
        return true;
    }
    else if (HitTest(HitAreaNameBody, x, y))
    {
        StartRandomMotion(MotionGroupTapBody, PriorityNormal);
        return true;
    }

    return false;
}
