#include "Render/CustomMeshElement.h"
#include "MeshPassProcessor.h"
#include "MeshBatch.h"
#include "PrimitiveSceneInfo.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderGraphBuilder.h"
#include "EngineModule.h"
#include "SceneRendererInterface.h"
#include "InstanceCulling/InstanceCullingContext.h"
#include "RenderCaptureInterface.h"
#include "MeshPassProcessor.h"
#include "MeshPassProcessor.inl"
#include "Slate/CustomMeshWidget.h"
#include "SceneView.h"

#include "CanvasRender.h"

#include "SceneRendering.h"
#include "BasePassRendering.h"
#include "MobileBasePassRendering.h"
#include "VT/VirtualTextureSystem.h"

#if (!UE_BUILD_SHIPPING && !UE_BUILD_TEST)
static int32 GCustomMeshWidgetCaptureDraws = 0;
static FAutoConsoleVariableRef CVarCustomMeshWidgetCaptureDraws(
	TEXT("s.CustomMeshWidget.CaptureDraws"),
	GCustomMeshWidgetCaptureDraws,
	TEXT("Enable capturing of custom mesh widget passes"));
#endif

BEGIN_SHADER_PARAMETER_STRUCT(FDrawTileMeshPassParameters, )
SHADER_PARAMETER_STRUCT_INCLUDE(FViewShaderParameters, View)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FInstanceCullingGlobalUniforms, InstanceCulling)
SHADER_PARAMETER_STRUCT_REF(FReflectionCaptureShaderData, ReflectionCapture)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FDebugViewModePassUniformParameters, DebugViewMode)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FTranslucentBasePassUniformParameters, TranslucentBasePass)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FOpaqueBasePassUniformParameters, OpaqueBasePass)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FMobileBasePassUniformParameters, MobileBasePass)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()


FCustomMeshSlateElement::FCustomMeshSlateElement(const SCustomMeshWidget& Creator)
	: StaticMesh(Creator.GetStaticMesh())
	, Material(Creator.GetMaterial())
	, MaterialRenderProxy(nullptr)
	, StaticMeshRenderData(nullptr)
	, IsLocalToWorldDeterminantNegative(false)
	, CurrentTime(0.0)
	, DeltaTime(0.0f)
{
	LocalBounds = StaticMesh->GetBounds();
	LocalToWS = FMatrix::Identity;
}

void FCustomMeshSlateElement::ResetGameThreadInfo()
{
	Material = nullptr;
	StaticMesh = nullptr;
}

FMatrix ComputeProjectionMatrix(const FIntRect& InCanvasRect, float InFOV, float InDPIScale)
{
	const float RadianFOV = FMath::DegreesToRadians(InFOV);
	const float HalfFOV = FMath::Max(0.001f, RadianFOV * 0.5f);
	const float FOVScale = FMath::Tan(HalfFOV) / InDPIScale;
	const float OrthoWidth = 0.5f * FOVScale * InCanvasRect.Width();
	const float OrthoHeight = 0.5f * FOVScale * InCanvasRect.Height();
	const FMatrix::FReal ZRange = UE_OLD_WORLD_MAX;
	if ((bool)ERHIZBuffer::IsInverted)
	{
		return FReversedZOrthoMatrix(OrthoWidth, OrthoHeight, 0.5f / ZRange, ZRange);
	}
	else
	{
		return FOrthoMatrix(OrthoWidth, OrthoHeight, 0.5f / ZRange, ZRange);
	}
}

FMatrix Compute2DScreenTransformMatrix(const FMatrix2x2f& In2DTransformMatrix)
{
	FMatrix M = FMatrix::Identity;
	In2DTransformMatrix.GetMatrix(M.M[0][0], M.M[0][1], M.M[1][0], M.M[1][1]);
	M.M[0][1] *= -1.0f;
	M.M[1][0] *= -1.0f;
	return M;
}

bool FCustomMeshSlateElement::BeginRenderingCanvas(const FIntRect& InCanvasRect, const FIntRect& InClippingRect, const FMatrix& MeshTransform, const FSlateRenderTransform& SlateTransform)
{
	if (IsCulledAway(InCanvasRect, InClippingRect) || !IsValid(StaticMesh) || !IsValid(Material)) 
		return false;

	FMaterialRenderProxy* MaterialProxyPtr = Material->GetRenderProxy();
	FStaticMeshRenderData* StaticMeshRenderDataPtr = StaticMesh->GetRenderData();
	FMatrix OrthoProjectionMatrix = ComputeProjectionMatrix(InCanvasRect, 90.0f, 1.0f) * Compute2DScreenTransformMatrix(SlateTransform.GetMatrix());
	
	const FVector ViewPivot = FVector(0.5f, 0.5f, 0.0f);
	const FBox BoundsBox = LocalBounds.GetBox();
	FVector ViewPoint = FMath::Lerp(BoundsBox.Min, BoundsBox.Max, ViewPivot);

	if (!MaterialProxyPtr || !StaticMeshRenderDataPtr)
		return false;

	ENQUEUE_RENDER_COMMAND(SetupCustomMeshSlateRendering)([=, this](FRHICommandListImmediate& RHICmdList)
	{
		RenderTarget.SetClippingRect(InClippingRect);
		RenderTarget.SetViewRect(InCanvasRect);
		MaterialRenderProxy = MaterialProxyPtr;
		StaticMeshRenderData = StaticMeshRenderDataPtr;
		WSProjection = OrthoProjectionMatrix;
		LocalToWS = MeshTransform;
		ViewOrigin = LocalToWS.TransformPosition(ViewPoint);
		WSBounds = LocalBounds.TransformBy(LocalToWS);
		IsLocalToWorldDeterminantNegative = LocalToWS.Determinant() < 0.0f;
	});

	return true;
}

void FCustomMeshSlateElement::SetAnimationTime(double InCurrentTime, float InDeltaTime)
{
	ENQUEUE_RENDER_COMMAND(UpdateCustomMeshSlateTimers)([=, this](FRHICommandListImmediate& RHICmdList)
	{
		CurrentTime = InCurrentTime;
		DeltaTime = InDeltaTime;
	});
}

void FCustomMeshSlateElement::DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* InWindowBackBuffer)
{
	if (!MaterialRenderProxy || !StaticMeshRenderData) return;

	RenderTarget.SetRenderTargetTexture(*(FTexture2DRHIRef*)InWindowBackBuffer);

	FIntPoint ViewSize = RenderTarget.GetSizeXY();

	FMatrix PreProjectionMatrix = FScaleMatrix(LocalToWS.GetScaleVector());

	IRendererModule& Renderer = GetRendererModule();
	
	int32 RenderLOD = StaticMeshRenderData->GetFirstValidLODIdx(0);

	FRDGBuilder GraphBuilder(RHICmdList);

#if (!UE_BUILD_SHIPPING && !UE_BUILD_TEST)
	RenderCaptureInterface::FScopedCapture RenderCapture(GCustomMeshWidgetCaptureDraws > 0, GraphBuilder);
	GCustomMeshWidgetCaptureDraws = FMath::Max(0, GCustomMeshWidgetCaptureDraws - 1);
#endif

	FCanvasRenderContext RenderContext(GraphBuilder, &RenderTarget, RenderTarget.GetViewRect(), RenderTarget.GetClippingRect(), false);

	FEngineShowFlags ShowFlags(ESFIM_Game);
	ApplyViewMode(VMI_Unlit, false, ShowFlags);

	const FSceneViewFamily& ViewFamily = *RenderContext.Alloc<FSceneViewFamily>(
		FSceneViewFamily::ConstructionValues(&RenderTarget, nullptr, ShowFlags)
		.SetTime(FGameTime::GetTimeSinceAppStart())
		.SetGammaCorrection(1.0f));
	
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ProjectionMatrix = WSProjection;
	ViewInitOptions.ViewRotationMatrix = FRotationMatrix(FRotator::ZeroRotator);
	ViewInitOptions.ViewOrigin = ViewOrigin;
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.SetViewRectangle(FIntRect(FIntPoint::ZeroValue, ViewSize));
	ViewInitOptions.bIsSceneCapture = true;
	FSceneView& View = *RenderContext.Alloc<FSceneView>(ViewInitOptions);
	View.FinalPostProcessSettings.bOverride_ToneCurveAmount = 1;
	View.FinalPostProcessSettings.ToneCurveAmount = 0.0;

	DynamicPrimitiveUniformBuffer.Set(LocalToWS, LocalToWS, WSBounds, LocalBounds, LocalBounds, false, false, false, &CustomPrimitiveData);

	const auto FeatureLevel = View.GetFeatureLevel();
	const EShadingPath ShadingPath = FSceneInterface::GetShadingPath(FeatureLevel);

	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	DrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());

	DrawMesh(RenderContext, Renderer, View, RenderLOD);
	
	TRefCountPtr<IPooledRenderTarget> ExtractedRTPointer;
	GraphBuilder.QueueTextureExtraction(RenderContext.GetRenderTarget(), &ExtractedRTPointer);
	GraphBuilder.Execute();
	RenderTarget.ClearRenderTargetTexture();
}

void FCustomMeshSlateElement::DrawMesh(FCanvasRenderContext& RenderContext, IRendererModule& Renderer, FSceneView& SceneView, int32 RenderLOD)
{
	const FStaticMeshLODResources& LODResources = StaticMeshRenderData->LODResources[RenderLOD];
	const FStaticMeshVertexFactories& VertexFactory = StaticMeshRenderData->LODVertexFactories[RenderLOD];
	
	check(MaterialRenderProxy);
	for (const FStaticMeshSection& Section : LODResources.Sections)
	{
		int32 MaterialIndex = Section.MaterialIndex;

		FMeshBatch* OutMeshBatch = RenderContext.Alloc<FMeshBatch>();
		OutMeshBatch->LODIndex = RenderLOD;
		OutMeshBatch->VertexFactory = &VertexFactory.VertexFactory;
		OutMeshBatch->LCI = nullptr;
		OutMeshBatch->ReverseCulling = IsLocalToWorldDeterminantNegative;
		OutMeshBatch->CastShadow = false;
		OutMeshBatch->DepthPriorityGroup = SDPG_World;
		OutMeshBatch->Type = PT_TriangleList;
		OutMeshBatch->bDisableBackfaceCulling = true;
		OutMeshBatch->MaterialRenderProxy = MaterialRenderProxy;

		FMeshBatchElement& BatchElement = OutMeshBatch->Elements[0];
		BatchElement.VertexFactoryUserData = VertexFactory.VertexFactory.GetUniformBuffer();
		BatchElement.IndexBuffer = &LODResources.IndexBuffer;
		BatchElement.FirstIndex = Section.FirstIndex;
		BatchElement.MinVertexIndex = Section.MinVertexIndex;
		BatchElement.MaxVertexIndex = Section.MaxVertexIndex;
		BatchElement.NumPrimitives = Section.NumTriangles;
		BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
		BatchElement.PrimitiveIdMode = PrimID_ForceZero;

		Renderer.DrawTileMesh(RenderContext, DrawRenderState, SceneView, *OutMeshBatch, false, FHitProxyId(), false);
	}
}

bool FCustomMeshSlateElement::IsCulledAway(const FIntRect& InCanvasRect, const FIntRect& InClippingRect) const
{
	return InCanvasRect.Size().X <= 0 || InCanvasRect.Size().Y <= 0 || InClippingRect.Size().X <= 0 || InClippingRect.Size().Y <= 0;
}

FInstancedCustomMeshElement::FInstancedCustomMeshElement(const SCustomMeshWidget& Creator)
	: FCustomMeshSlateElement(Creator) 
{
}

void FInstancedCustomMeshElement::DrawMesh(FCanvasRenderContext& RenderContext, IRendererModule& Renderer, FSceneView& SceneView, int32 RenderLOD)
{
	const FStaticMeshLODResources& LODResources = StaticMeshRenderData->LODResources[RenderLOD];
	const FStaticMeshVertexFactories& VertexFactory = StaticMeshRenderData->LODVertexFactories[RenderLOD];

	for (const FStaticMeshSection& Section : LODResources.Sections)
	{
		int32 MaterialIndex = Section.MaterialIndex;

		FMeshBatch* OutMeshBatch = RenderContext.Alloc<FMeshBatch>();
		OutMeshBatch->LODIndex = 0;
		OutMeshBatch->VertexFactory = &VertexFactory.VertexFactory;
		OutMeshBatch->LCI = nullptr;
		OutMeshBatch->ReverseCulling = IsLocalToWorldDeterminantNegative;
		OutMeshBatch->CastShadow = false;
		OutMeshBatch->DepthPriorityGroup = SDPG_World;
		OutMeshBatch->Type = PT_TriangleList;
		OutMeshBatch->bDisableBackfaceCulling = true;
		OutMeshBatch->MaterialRenderProxy = MaterialRenderProxy;

		FMeshBatchElement& BatchElement = OutMeshBatch->Elements[0];
		BatchElement.VertexFactoryUserData = VertexFactory.VertexFactory.GetUniformBuffer();
		BatchElement.IndexBuffer = &LODResources.IndexBuffer;
		BatchElement.FirstIndex = Section.FirstIndex;
		BatchElement.MinVertexIndex = Section.MinVertexIndex;
		BatchElement.MaxVertexIndex = Section.MaxVertexIndex;
		BatchElement.NumPrimitives = Section.NumTriangles;
		BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
		BatchElement.PrimitiveIdMode = PrimID_DynamicPrimitiveShaderData;

		//BatchElement.bIsInstancedMesh = true;
		//BatchElement.NumInstances = InstanceCount;
		//BatchElement.UserData = (void*)WaterMeshUserDataBuffers->GetUserData(RenderGroup);
		//BatchElement.UserIndex = InstanceDataOffset * InstanceFactor;

		Renderer.DrawTileMesh(RenderContext, DrawRenderState, SceneView, *OutMeshBatch, false, FHitProxyId(), false);
	}
}

void FInstancedCustomMeshElement::SendInstanceUpdateRange(TArrayView<const FCustomMeshWidgetInstanceData> Instances)
{
}
