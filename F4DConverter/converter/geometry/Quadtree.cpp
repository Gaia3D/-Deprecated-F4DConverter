
#include "stdafx.h"

#include "Quadtree.h"

namespace gaia3d
{
	Quadtree::Quadtree(Quadtree* owner)
	:parent(owner)
	{
		level = (parent == NULL) ? 0 : parent->level + 1;
		
	}

	Quadtree::~Quadtree()
	{
		size_t childCount = children.size();
		for(size_t i = 0; i < childCount; i++)
			delete children[i];
		children.clear();

		legos.clear();
	}

	void Quadtree::getAllLeafQuadtrees(std::vector<Quadtree*>& leafContainer, bool onlyNotEmptyLeaf)
	{
		size_t childCount = children.size();
		if(childCount > 0)
		{
			for(size_t i = 0; i < childCount; i++)
				children[i]->getAllLeafQuadtrees(leafContainer, onlyNotEmptyLeaf);
		}
		else
		{
			if(onlyNotEmptyLeaf)
			{
				if(legos.size() > 0)
					leafContainer.push_back(this);
			}
			else
				leafContainer.push_back(this);
		}
	}

	void Quadtree::makeTreeOfUnfixedDepth(double minLeafSize, bool isObjectInOnlyOneLeaf)
	{
		double xLength = maxX - minX, yLength = maxY - minY;
		double maxEdgeLength = (xLength > yLength) ? xLength : yLength;
		double tolerance = minLeafSize * 0.4;

		if(maxEdgeLength > minLeafSize + tolerance)
		{
			for(size_t i = 0; i < 4; i++)
			{
				children.push_back(new Quadtree(this));
			}

			double halfX, halfY;
			halfX= (maxX+minX)/2.0;
			halfY= (maxY+minY)/2.0;
			children[0]->setSize(minX, minY, halfX, halfY);
			children[1]->setSize(halfX, minY, maxX, halfY);
			children[2]->setSize(halfX, halfY,maxX, maxY);
			children[3]->setSize(minX, halfY, halfX, maxY);

			distributeLegosIntoEachChildren(isObjectInOnlyOneLeaf);

			for(size_t i = 0; i < 4; i++)
			{
				if(children[i]->legos.size() == 0)
					continue;

				children[i]->makeTreeOfUnfixedDepth(minLeafSize, isObjectInOnlyOneLeaf);
			}
		}
	}

	void Quadtree::setQuadtreeId(size_t parentId, size_t orderOfChild)
	{
		if(level == 0)
			quadtreeId = 0;
		else
			quadtreeId = parentId*10 + (orderOfChild+1);

		size_t childCount = children.size();
		for(size_t i = 0; i < childCount; i++)
		{
			children[i]->setQuadtreeId(quadtreeId, i);
		}
	}

	void Quadtree::distributeLegosIntoEachChildren(bool isObjectInOnlyOneLeaf)
	{
		size_t childCount = children.size();
		if(childCount == 0)
			return;

		size_t legoCount = legos.size();
		double centerX, centerY;
		for(size_t i = 0; i < legoCount; i++)
		{
			centerX = (legos[i]->minX + legos[i]->maxX)*0.5;
			centerY = (legos[i]->minY + legos[i]->maxY)*0.5;

			for(size_t j = 0; j < childCount; j++)
			{
				if( centerX >= children[j]->maxX || centerX <= children[j]->minX ||
					centerY >= children[j]->maxY || centerY <= children[j]->minY )
					continue;

				children[j]->legos.push_back(legos[i]);

				if(isObjectInOnlyOneLeaf)
				{
					legos.erase(legos.begin() + i);
					legoCount--;
					i--;
				}
			}
		}
	}
}