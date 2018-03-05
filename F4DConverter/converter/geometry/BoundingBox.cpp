
#include "stdafx.h"

#include "BoundingBox.h"

namespace gaia3d
{
	BoundingBox::BoundingBox()
	{
		minX = minY = minZ = 10E9;

		maxX = maxY = maxZ = -10E9;

		isInitialized = false;
	}

	BoundingBox::~BoundingBox()
	{
	}

	void BoundingBox::addBox(BoundingBox& bbox)
	{
		if(!bbox.isInitialized)
			return;

		addPoint(bbox.maxX, bbox.maxY, bbox.maxZ);
		addPoint(bbox.minX, bbox.minY, bbox.minZ);
	}

	void BoundingBox::addPoint(double x, double y, double z)
	{
		if(!isInitialized)
		{
			init(x, y, z);
			return;
		}

		if(x < minX) minX = x;
		else if(x > maxX) maxX = x;

		if(y < minY) minY = y;
		else if(y > maxY) maxY= y;

		if(z < minZ) minZ= z;
		else if(z > maxZ) maxZ = z;
	}

	void BoundingBox::getCenterPoint(double& x, double& y, double& z)
	{
		x = (minX + maxX) / 2.0;
		y = (minY + maxY) / 2.0;
		z = (minZ + maxZ) / 2.0;
	}

	double BoundingBox::getMaxLength()
	{
		double xlen = getXLength();
		double ylen = getYLength();
		double zlen = getZLength();
		return ( xlen > ylen ? (xlen > zlen ? xlen : zlen) : (ylen > zlen ? ylen : zlen));
	}

	double BoundingBox::getXLength()
	{
		return maxX - minX;
	}

	double BoundingBox::getYLength()
	{
		return maxY - minY;
	}

	double BoundingBox::getZLength()
	{
		return maxZ - minZ;
	}

	void BoundingBox::init(double x, double y, double z)
	{
		minX = maxX = x;
		minY = maxY = y;
		minZ = maxZ = z;

		isInitialized = true;
	}
}