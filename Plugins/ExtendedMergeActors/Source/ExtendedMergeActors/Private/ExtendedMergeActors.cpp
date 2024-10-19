// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExtendedMergeActors.h"
#include "ExtendedMergeActorsStyle.h"
#include "ExtendedMeshInstancingTool.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "IMergeActorsModule.h"

static const FName ExtendedMergeActorsTabName("ExtendedMergeActors");

#define LOCTEXT_NAMESPACE "FExtendedMergeActorsModule"

void FExtendedMergeActorsModule::StartupModule()
{
	FExtendedMergeActorsStyle::Initialize();
	FExtendedMergeActorsStyle::ReloadTextures();

	TUniquePtr<IMergeActorsTool> Tool = MakeUnique<FWrappedMeshInstancingTool>();
	WrappedMeshInstancingTool = Tool.Get();
	IMergeActorsModule& MergeActorsModule = IMergeActorsModule::Get();
	ensure(MergeActorsModule.RegisterMergeActorsTool(MoveTemp(Tool)));
}

void FExtendedMergeActorsModule::ShutdownModule()
{
	if (IMergeActorsModule* MergeActorsModule = FModuleManager::LoadModulePtr<IMergeActorsModule>("MergeActors"))
	{
		MergeActorsModule->UnregisterMergeActorsTool(WrappedMeshInstancingTool);
	}
	FExtendedMergeActorsStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FExtendedMergeActorsModule, ExtendedMergeActors)