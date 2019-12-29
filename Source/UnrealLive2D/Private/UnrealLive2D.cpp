// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "UnrealLive2D.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "CubismBpLib.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FUnrealLive2DModule"

DEFINE_LOG_CATEGORY(LogCubism);

void FUnrealLive2DModule::StartupModule()
{
    TSharedPtr<IPlugin> tsp_Plugin = IPluginManager::Get().FindPlugin(TEXT("UnrealLive2D"));
    if (tsp_Plugin.IsValid())
    {
        FString PluginShaderDir = FPaths::Combine(tsp_Plugin->GetBaseDir(), TEXT("Shaders"));
        AddShaderSourceDirectoryMapping(TEXT("/Plugin/UnrealLive2D"), PluginShaderDir);
    }
    else
    {
        UE_LOG(LogCubism, Log, TEXT("FUnrealLive2DModule::StartupModule: Faild to find plugin <UnrealLive2D>, please check plugin config."));
        checkNoEntry();
    }
}

void FUnrealLive2DModule::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealLive2DModule, UnrealLive2D)