#include "MaterialExpressionMeshPaintUVOutput.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "FRuntimeMeshPainterModule"

UMaterialExpressionMeshPaintUVOutput::UMaterialExpressionMeshPaintUVOutput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText NAME_MeshPaintUV;
		FConstructorStatics()
			: NAME_MeshPaintUV(LOCTEXT("Shading", "Shading"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.NAME_MeshPaintUV);
#endif

#if WITH_EDITOR
	Outputs.Reset();
#endif
}

#if WITH_EDITOR
int32 UMaterialExpressionMeshPaintUVOutput::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	int32 CodeInput = INDEX_NONE;
	if (OutputIndex == 0)
	{
		CodeInput = UV.IsConnected() ? UV.Compile(Compiler) : Compiler->Constant2(0.5f, 0.5f);
	}
	return Compiler->CustomOutput(this, OutputIndex, CodeInput);
}

void UMaterialExpressionMeshPaintUVOutput::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(FString(TEXT("Mesh Paint UV")));
}
#endif

int32 UMaterialExpressionMeshPaintUVOutput::GetNumOutputs() const
{
	return 1;
}

FString UMaterialExpressionMeshPaintUVOutput::GetFunctionName() const
{
	return TEXT("GetMeshPaintUVOutput");
}

FString UMaterialExpressionMeshPaintUVOutput::GetDisplayName() const
{
	return TEXT("Mesh Paint UV");
}

#undef LOCTEXT_NAMESPACE
