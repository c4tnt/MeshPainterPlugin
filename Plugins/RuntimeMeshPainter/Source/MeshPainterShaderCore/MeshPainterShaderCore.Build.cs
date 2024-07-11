//

using System.IO;
using UnrealBuildTool;

public class MeshPainterShaderCore : ModuleRules
{
	public MeshPainterShaderCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(new string[] { Path.Combine(GetModuleDirectory("Renderer"), "Private"), });
        PublicDependencyModuleNames.AddRange(new string[] { "Core" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine", "Projects", "RenderCore", "Renderer", "RHI" });
		DynamicallyLoadedModuleNames.AddRange(new string[] { });
    }
}
