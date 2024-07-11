#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionMeshPaintUVOutput.generated.h"

/** Material output expression for setting absorption properties of solid refractive glass (for the Path Tracer Only). */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionMeshPaintUVOutput : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** Texture coordinates to use with mesh painting functionality. */
	UPROPERTY()
	FExpressionInput UV;

public:
#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual EShaderFrequency GetShaderFrequency() { return SF_Vertex; }
	//~ End UMaterialExpression Interface
#endif

	//~ Begin UMaterialExpressionCustomOutput Interface
	virtual int32 GetNumOutputs() const override;
	virtual FString GetFunctionName() const override;
	virtual FString GetDisplayName() const override;
	//~ End UMaterialExpressionCustomOutput Interface
};
