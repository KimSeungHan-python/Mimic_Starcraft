// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Mimic_Starcraft : ModuleRules
{
	public Mimic_Starcraft(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"NavigationSystem",
			"UMG",
			"EnhancedInput",
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});

		DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
	}
}
