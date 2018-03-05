#pragma once

#ifdef SHIJTFORMAT

#include "aReader.h"

#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKernel.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKMath.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKBRep.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKG2d.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKG3d.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKTopAlgo.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKGeomBase.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKShHealing.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKPrim.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKGeomAlgo.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKOffset.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKBool.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKXSBase.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKMesh.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKV3d.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKService.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKHLR.lib")
#pragma comment(lib, "../3rdparty/opencascade-6.9.1/win64/vc10/lib/TKOpenGl.lib")
#pragma comment(lib, "../3rdparty/tbb42_20140416oss/lib/intel64/vc10/tbb.lib")
#pragma comment(lib, "../3rdparty/tbb42_20140416oss/lib/intel64/vc10/tbbmalloc.lib")
#pragma comment(lib, "../3rdparty/zlib-1.2.8-vc10-64/lib/zlib.lib")
#pragma comment(lib, "../3rdparty/freetype-2.5.5-vc10-64/lib/freetype.lib")
#pragma comment(lib, "../3rdparty/JTFramework/lib/x64/JTFramework.lib")
#pragma comment(lib, "../3rdparty/TKJT/lib/x64/TKJT.lib")

#include <JTData/JTData_GeometrySource.hxx>
#include <JTData/JTData_TransformAttribute.hxx>
#include <JTData/JTData_MaterialAttribute.hxx>


class JtReader : public aReader
{
public:
	JtReader();

	virtual ~JtReader();

	bool thisHasUnloadableMesh;

public:
	virtual bool readRawDataFile(std::wstring& filePath);

	virtual void clear();

private:
	JTData_GeometrySourcePtr geometrySource;


private:
	bool extractGeometryInformation(int depth,
									JTData_NodePtr& node,
									std::wstring& blockId,
									std::wstring& objectId,
									JTData_TransformAttributePtr& transform,
									JTData_MaterialAttributePtr& material);
};

#endif
