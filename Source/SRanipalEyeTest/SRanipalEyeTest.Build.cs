// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SRanipalEyeTest : ModuleRules
{
	private bool IsWindows(ReadOnlyTargetRules Target)
	{
		return (Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32);
	}
	public SRanipalEyeTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Add module for SteamVR support with UE4
		PublicDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

		// SRanipal plugin for Windows
		if (IsWindows(Target)){ // SRanipal unfortunately only works on Windows
			bEnableExceptions = true; // enable unwind semantics for C++-style exceptions
			PrivateDependencyModuleNames.AddRange(new string[] { "SRanipalEye" });
			PrivateIncludePathModuleNames.AddRange(new string[] { "SRanipalEye" });
			// add for eye enums header file
			PublicIncludePaths.AddRange(new string[] {"../Plugins/SRanipal/Source/SRanipal/Public/Eye/"});
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
