#include "MeshPainterShadersModule.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "MeshPainterShaderCore"

void FMeshPainterShadersModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("RuntimeMeshPainter"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/RuntimeMeshPainter"), PluginShaderDir);
}

void FMeshPainterShadersModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMeshPainterShadersModule, MeshPainterShaderCore)