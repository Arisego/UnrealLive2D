// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;


public class UnrealLive2D : ModuleRules
{
	public UnrealLive2D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "ThirdParty/CubismLibrary/include",
				"UnrealLive2D/Private/FrameWork/src"
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Engine",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
                "CoreUObject",
                "Projects",
                "RenderCore",
                "SlateCore",
                "RHI",
                "Projects",
			}
            );
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		bUseUnity = false;
		PrivateDefinitions.Add("CSM_CORE_WIN32_DLL=0");

		var ThirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/"));
        var CubismLibPath = ThirdPartyPath + "CubismLibrary/lib/";

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            CubismLibPath += "windows/x86_64/142";
            PublicAdditionalLibraries.Add(String.Format(CubismLibPath + "/Live2DCubismCore_MTd.lib"));
        }

    }
}
