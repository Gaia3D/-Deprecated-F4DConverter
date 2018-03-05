#pragma once

#include <vector>

#define M_PI 3.14159265358979323846

namespace gaia3d
{
	class LegoBlock;

	class GeometryUtility
	{
	public:

		static void calculatePlaneNormal(double& x0, double& y0, double& z0,
										double& x1, double& y1, double& z1,
										double& x2, double& y2, double& z2,
										double& nX, double& nY, double& nZ,
										bool bNormalize);

		static void crossProduct(double x0, double y0, double z0,
								double x1, double y1, double z1,
								double& nX, double& nY, double& nZ);

		enum GeomType {POLYHEDRON, TRIANGLE, POINT};
		static bool areTwoCongruentWithEachOther(void* geom1, void* geom2, void* transform, double tolerance, GeomType geomType);

		static double angleBetweenTwoVectors(double x1, double y1, double z1, double x2, double y2, double z2);

		static bool isInsideBox(double x, double y, double z,
								double minX, double minY, double minZ,
								double maxX, double maxY, double maxZ);

		static bool doesTriangleIntersectWithBox(double& x1, double& y1, double& z1,
												double& x2, double& y2, double& z2,
												double& x3, double& y3, double& z3,
												double& minX, double& minY, double& minZ,
												double& maxX, double& maxY, double& maxZ);

		static void mergeLegoBlocksAlongZAxis(std::vector<LegoBlock*>& legos, bool mustSameColor);

		static void mergeLegoBlocksAlongYAxis(std::vector<LegoBlock*>& legos, bool mustSameColor);

		static void mergeLegoBlocksAlongXAxis(std::vector<LegoBlock*>& legos, bool mustSameColor);
	};
}