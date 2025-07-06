// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DungeonGen : ModuleRules
{
	public DungeonGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
