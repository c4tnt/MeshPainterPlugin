#include "MeshWidgetEditor.h"
#include "MeshWidgetEditorStyle.h"

#define LOCTEXT_NAMESPACE "FMeshWidgetEditorModule"

void FMeshWidgetEditorModule::StartupModule()
{
	FMeshWidgetEditorStyle::Register();
}

void FMeshWidgetEditorModule::ShutdownModule()
{
	FMeshWidgetEditorStyle::Unregister();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshWidgetEditorModule, MeshWidgetEditor)