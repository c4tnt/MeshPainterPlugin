#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "RenderResource.h"
#include "ShaderParameters.h"
#include "ShaderParameterStruct.h"
#include "Shader.h"
#include "MeshMaterialShader.h"
#include "RHI.h"
#include "RHIDefinitions.h"
#include "SceneView.h"
#include "MeshDrawShaderBindings.h"
#include "InstanceCulling/InstanceCullingContext.h"

BEGIN_SHADER_PARAMETER_STRUCT(FMeshPaintShaderParameters, MESHPAINTERSHADERCORE_API)
SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FInstanceCullingGlobalUniforms, InstanceCulling)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

class FMeshPaintShaderElementData : public FMeshMaterialShaderElementData
{
public:
	FVector4 UVTileMapping;
};

bool CheckMeshPaintVertexFactoryType(const FVertexFactoryType* VertexFactoryType);

class FMeshPaintShaderVS : public FMeshMaterialShader
{
public:
	DECLARE_SHADER_TYPE(FMeshPaintShaderVS, MeshMaterial);

	FMeshPaintShaderVS() { }
	FMeshPaintShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer)	
	{
		UVTileMapping.Bind(Initializer.ParameterMap, TEXT("UVTileMapping"), SPF_Mandatory);
	}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5)
			&& CheckMeshPaintVertexFactoryType(Parameters.VertexFactoryType);
	}

	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const FMeshPaintShaderElementData& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);

		ShaderBindings.Add(UVTileMapping, FVector4f(ShaderElementData.UVTileMapping));
	}

private:
	LAYOUT_FIELD(FShaderParameter, UVTileMapping);
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FMeshPaintShaderVS, TEXT("/Plugin/RuntimeMeshPainter/Private/MeshPaintShaders.usf"), TEXT("MeshPaintShaderVS"), SF_Vertex);

enum class EMeshPaintShaderOutputBits : int32
{
	None = 0x00,
	BaseColor = 0x01,
	Emissive = 0x02,
	Normal = 0x04
};
ENUM_CLASS_FLAGS(EMeshPaintShaderOutputBits)

class FMeshPaintShaderPS : public FMeshMaterialShader
{
public:
	class FOutputBits : SHADER_PERMUTATION_INT("OUTPUT_BITS", 8);
	using FPermutationDomain = TShaderPermutationDomain<FOutputBits>;

	DECLARE_SHADER_TYPE(FMeshPaintShaderPS, MeshMaterial);

	FMeshPaintShaderPS() { }
	FMeshPaintShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer)	{}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		FPermutationDomain Permutation(Parameters.PermutationId);
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5)
			&& Permutation.Get<FOutputBits>() != 0
			&& CheckMeshPaintVertexFactoryType(Parameters.VertexFactoryType);
	}

	static void ModifyCompilationEnvironment(const FMeshMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FPermutationDomain Permutation(Parameters.PermutationId);
		FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		int32 MRTIndex = 0;
		if (Permutation.Get<FOutputBits>() & (int32)EMeshPaintShaderOutputBits::BaseColor)
		{
			OutEnvironment.SetDefine(TEXT("MRT_BASE_COLOR"), MRTIndex);
			MRTIndex++;
		}
		if (Permutation.Get<FOutputBits>() & (int32)EMeshPaintShaderOutputBits::Emissive)
		{
			OutEnvironment.SetDefine(TEXT("MRT_EMISSIVE"), MRTIndex);
			MRTIndex++;
		}
		if (Permutation.Get<FOutputBits>() & (int32)EMeshPaintShaderOutputBits::Normal)
		{
			OutEnvironment.SetDefine(TEXT("MRT_NORMAL"), MRTIndex);
			MRTIndex++;
		}
		OutEnvironment.SetDefine(TEXT("MRT_MAX"), MRTIndex);
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FMeshPaintShaderPS, TEXT("/Plugin/RuntimeMeshPainter/Private/MeshPaintShaders.usf"), TEXT("MeshPaintShaderPS"), SF_Pixel);
