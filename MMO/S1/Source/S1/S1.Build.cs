// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class S1 : ModuleRules
{
	public S1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking", "EnhancedInput", "NavigationSystem" });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "ProtobufCore"
        });

        // Editor 전용 모듈은 Editor 빌드일 때만 추가
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
                "Blutility",
                "EditorSubsystem",
                "EditorScriptingUtilities"
            });
        }

        PrivateIncludePaths.AddRange(new string[]
		{
			"S1/",
			"S1/Network/",
			"S1/Game/",
			"S1/Data/",
            "C:/Users/jeson/vcpkg/installed/x64-windows/include/"
        });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
