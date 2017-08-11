#pragma once

#include <vector>

#include "LegoBlock.h"

namespace gaia3d
{
	class Quadtree
	{
	public:
		Quadtree(Quadtree* owner);

		virtual ~Quadtree();

	public:
		Quadtree* parent;

		std::vector<Quadtree*> children;

		std::vector<LegoBlock*> legos;

		unsigned char level;

		size_t quadtreeId;

		double minX, minY, maxX, maxY;

		void setSize(double xMin, double yMin, double xMax, double yMax) {minX = xMin, minY = yMin, maxX = xMax, maxY = yMax;}

		void getAllLeafQuadtrees(std::vector<Quadtree*>& leafContainer, bool onlyNotEmptyLeaf = true);

		void makeTreeOfUnfixedDepth(double minLeafSize, bool isObjectInOnlyOneLeaf);

		void setQuadtreeId(size_t parentId = 0, size_t orderOfChild = 0);

		void distributeLegosIntoEachChildren(bool isObjectInOnlyOneLeaf);
	};
}