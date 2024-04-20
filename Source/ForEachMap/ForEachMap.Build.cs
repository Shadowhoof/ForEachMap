// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ForEachMap : ModuleRules
{
	public ForEachMap(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"BlueprintGraph",
				"KismetCompiler",
				"UnrealEd"
			}
			);
	}
}
