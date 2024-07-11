#include "MeshPainterShader.h"

bool CheckMeshPaintVertexFactoryType(const FVertexFactoryType* VertexFactoryType)
{
	return 
		VertexFactoryType == FindVertexFactoryType(TEXT("FLocalVertexFactory")) ||
		VertexFactoryType == FindVertexFactoryType(TEXT("FSplineMeshVertexFactory")) ||
		VertexFactoryType == FindVertexFactoryType(FName(TEXT("TGPUSkinVertexFactoryDefault"), FNAME_Find)) ||
		VertexFactoryType == FindVertexFactoryType(FName(TEXT("TGPUSkinVertexFactoryUnlimited"), FNAME_Find));
}
