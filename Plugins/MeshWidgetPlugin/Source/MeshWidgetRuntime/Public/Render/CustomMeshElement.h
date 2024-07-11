#pragma once

#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "MeshWidgetTypes.h"
#include "MeshPassProcessor.h"
#include "SceneManagement.h"

class SCustomMeshWidget;
struct FMeshPassProcessorRenderState;
class FDynamicPrimitiveUniformBuffer;

class FCustomMeshElementRenderTarget : public FRenderTarget
{
public:
	/** FRenderTarget interface */
	virtual FIntPoint GetSizeXY() const { return ClippingRect.Size(); }

	/** Sets the texture that this target renders to */
	void SetRenderTargetTexture( FTexture2DRHIRef& InRHIRef ) { RenderTargetTextureRHI = InRHIRef; }

	/** Clears the render target texture */
	void ClearRenderTargetTexture() { RenderTargetTextureRHI.SafeRelease(); }

	/** Sets the viewport rect for the render target */
	void SetViewRect( const FIntRect& InViewRect ) { ViewRect = InViewRect; }

	/** Gets the viewport rect for the render target */
	const FIntRect& GetViewRect() const { return ViewRect; }

	/** Sets the clipping rect for the render target */
	void SetClippingRect( const FIntRect& InClippingRect ) { ClippingRect = InClippingRect; }

	/** Gets the clipping rect for the render target */
	const FIntRect& GetClippingRect() const { return ClippingRect; }

private:
	FIntRect ViewRect;
	FIntRect ClippingRect;
};


class MESHWIDGETRUNTIME_API FCustomMeshSlateElement : public ICustomSlateElement
{
public:
	FCustomMeshSlateElement(const SCustomMeshWidget& Creator);

	virtual bool BeginRenderingCanvas(const FIntRect& InCanvasRect, const FIntRect& InClippingRect, const FMatrix& MeshTransform, const FSlateRenderTransform& SlateTransform);
	virtual void DrawRenderThread(class FRHICommandListImmediate& RHICmdList, const void* InWindowBackBuffer) override final;
	virtual void SetAnimationTime(double InCurrentTime, float InDeltaTime);

	virtual bool SupportInstanceData() { return false; }
	virtual void SendInstanceUpdateRange(TArrayView<const FCustomMeshWidgetInstanceData> Instances) {}

	virtual void ResetGameThreadInfo();

	virtual void DrawMesh(FCanvasRenderContext& RenderContext, IRendererModule& Renderer, FSceneView& SceneView, int32 RenderLOD);

protected:
	bool IsCulledAway(const FIntRect& InCanvasRect, const FIntRect& InClippingRect) const;
	
	// Game thread info
	UStaticMesh* StaticMesh;
	UMaterialInterface* Material;

	// Constant data for GT and RT
	FBoxSphereBounds LocalBounds;

	// Render thread info
	FCustomMeshElementRenderTarget RenderTarget;
	FMaterialRenderProxy* MaterialRenderProxy;
	FStaticMeshRenderData* StaticMeshRenderData;
	bool IsLocalToWorldDeterminantNegative;
	FCustomPrimitiveData CustomPrimitiveData;
	FMatrix WSProjection;
	FVector ViewOrigin;
	FMatrix LocalToWS;
	FBoxSphereBounds WSBounds;

	// Inner render thread states
	FMeshPassProcessorRenderState DrawRenderState;
	FDynamicPrimitiveUniformBuffer DynamicPrimitiveUniformBuffer;

	double CurrentTime;
	float DeltaTime;
};

class MESHWIDGETRUNTIME_API FInstancedCustomMeshElement : public FCustomMeshSlateElement
{
public:
	FInstancedCustomMeshElement(const SCustomMeshWidget& Creator);

	virtual void DrawMesh(FCanvasRenderContext& RenderContext, IRendererModule& Renderer, FSceneView& SceneView, int32 RenderLOD) override;

	virtual bool SupportInstanceData() override { return true; };
	virtual void SendInstanceUpdateRange(TArrayView<const FCustomMeshWidgetInstanceData> Instances);

protected:
	// Render thread info

};
