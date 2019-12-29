// Fill out your copyright notice in the Description page of Project Settings.

#include "CubismBpLib.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "CubismObject.h"
#include "CubisimInclude.h"

#include "FWPort/CubismFrameWorkAllocator.h"
#include "Interfaces/IPluginManager.h"
#include "Templates/SharedPointer.h"

#include "UnrealLive2D.h"

using namespace Live2D::Cubism::Core;
using namespace Live2D::Cubism::Framework;

CubismFrameWorkAllocator ls_Allocator;

/**	Log out func register to cubism */
static void Cubism_Log(const char* message)
{
	printf("%s\n", message);
	UE_LOG(LogCubism, Log, TEXT("[UCubismBpLib] %s"), ANSI_TO_TCHAR(message));
}

void AnimCallback(float time, const char* value)
{
	UE_LOG(LogCubism, Log, TEXT("[UCubismBpLib] AnimCallback: %f -> %s"), time, ANSI_TO_TCHAR(value));
}

/////////////////////////////////////////////////////////////////////////////////////
static bool lb_IsInited = false;

void UCubismBpLib::InitCubism()
{
	if (lb_IsInited)
	{
		UE_LOG(LogTemp, Log, TEXT("UCubismBpLib::InitCubism: Already inited."));
		return;
	}

	csmVersion version = csmGetVersion();
	UE_LOG(LogCubism, Log, TEXT("csm Version is: %u"), version);

	Csm::CubismFramework::Option _cubismOption;  ///< Cubism SDK Option
        //setup cubism
    _cubismOption.LogFunction = Cubism_Log;
    _cubismOption.LoggingLevel = Live2D::Cubism::Framework::CubismFramework::Option::LogLevel_Verbose;
    Csm::CubismFramework::StartUp(&ls_Allocator, &_cubismOption);

    //Initialize cubism
    CubismFramework::Initialize();

	lb_IsInited = true;
}

FString UCubismBpLib::Get_CubismPath()
{
	TSharedPtr<IPlugin> tsp_Plugin = IPluginManager::Get().FindPlugin(TEXT("UnrealLive2D"));
	if (tsp_Plugin.IsValid())
	{
        return tsp_Plugin->GetBaseDir();
	}
	else
	{
		return FPaths::ProjectPluginsDir() / TEXT("UnrealLive2D");
	}
}
