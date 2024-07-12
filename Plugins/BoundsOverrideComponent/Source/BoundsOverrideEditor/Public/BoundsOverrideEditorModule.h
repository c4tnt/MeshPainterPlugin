// Copyright Dod Games, Inc. All Rights Reserved.

#pragma once
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "ComponentVisualizer.h"

class FBoundsOverrideEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void OnPostEngineInit();

protected:
	void RegisterComponentVisualizer(FName ComponentClassName, TSharedRef<FComponentVisualizer> Visualizer);

private:
	TSet<FName> RegisteredComponentClassNames;
};
