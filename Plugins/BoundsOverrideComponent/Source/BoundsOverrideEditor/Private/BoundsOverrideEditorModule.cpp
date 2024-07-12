#include "BoundsOverrideEditorModule.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "AssetToolsModule.h"
#include "ComponentVisualizer.h"
#include "ComponentVisualizers/BoundsOverrideVisualizer.h"
#include "Components/BoundsOverrideComponent.h"

void FBoundsOverrideEditorModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FBoundsOverrideEditorModule::OnPostEngineInit);
}

void FBoundsOverrideEditorModule::ShutdownModule()
{
	// Unregister component visualizers
	if (GUnrealEd != NULL)
	{
		// Iterate over all class names we registered for
		for (FName ClassName : RegisteredComponentClassNames)
		{
			GUnrealEd->UnregisterComponentVisualizer(ClassName);
		}
	}
}

void FBoundsOverrideEditorModule::OnPostEngineInit()
{
	if (GUnrealEd != NULL)
	{
		RegisterComponentVisualizer(UBoundsOverrideComponent::StaticClass()->GetFName(), MakeShareable(new FBoundsOverrideComponentVisualizer));
	}
}

void FBoundsOverrideEditorModule::RegisterComponentVisualizer(FName ComponentClassName, TSharedRef<FComponentVisualizer> Visualizer)
{
	check(GUnrealEd != NULL);
	GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
	RegisteredComponentClassNames.Add(ComponentClassName);
	Visualizer->OnRegister();
}

IMPLEMENT_MODULE( FBoundsOverrideEditorModule, BoundsOverrideEditor );
