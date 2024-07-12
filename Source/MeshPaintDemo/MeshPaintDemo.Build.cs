// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MeshPaintDemo : ModuleRules
{
	public MeshPaintDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
	}
}
