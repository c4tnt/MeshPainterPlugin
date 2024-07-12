#include "BoundsOverrideRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FBoundsOverrideRuntimeModule"

void FBoundsOverrideRuntimeModule::StartupModule()
{
}

void FBoundsOverrideRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBoundsOverrideRuntimeModule, BoundsOverrideRuntime)