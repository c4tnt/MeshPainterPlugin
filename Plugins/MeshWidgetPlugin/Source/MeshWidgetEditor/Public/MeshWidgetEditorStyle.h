#pragma once

#include "Styling/SlateStyle.h"

class FMeshWidgetEditorStyle : public FSlateStyleSet
{
private:	
	FMeshWidgetEditorStyle();

public:
	static void Register();
	static void Unregister();

	static const FMeshWidgetEditorStyle& Get();
};

namespace MeshWidgetEditorStyleConstants
{
	//const FName Entry = TEXT("...");
}