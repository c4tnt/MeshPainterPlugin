#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class MESHPAINTERSHADERCORE_API FMeshPainterShadersModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
