
#include "stdafx.h"

#include <gdiplus.h>

#include "F4DWriter.h"
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <algorithm>
#include "../process/ConversionProcessor.h"
#include "../LogWriter.h"
#include "../geometry/ColorU4.h"


F4DWriter::F4DWriter(ConversionProcessor* conversionResult)
:processor(conversionResult)
{
	version = "v.0.0";
	guid = "abcdefghi";
	guidLength = 9;
}

F4DWriter::~F4DWriter()
{}

bool F4DWriter::write()
{
	bool outputFolderExist = false;
	std::wstring resultPath = folder + L"/F4D_" + processor->getAttribute(L"id");
	if (_waccess(resultPath.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(resultPath.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (_wmkdir(resultPath.c_str()) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	std::wstring headerPath = resultPath + L"/HeaderAsimetric.hed";
	FILE* file = _wfopen(headerPath.c_str(), L"wb");
	// write header
	writeHeader(file);
	fclose(file);

	// create reference directory
	outputFolderExist = false;
	std::wstring referencePath = resultPath + L"/References";
	if (_waccess(referencePath.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(referencePath.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (_wmkdir(referencePath.c_str()) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	// create lego block directory
	outputFolderExist = false;
	std::wstring legoBlockPath = resultPath + L"/Bricks";
	if (_waccess(legoBlockPath.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(legoBlockPath.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (_wmkdir(legoBlockPath.c_str()) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	// create model directory
	outputFolderExist = false;
	std::wstring modelPath = resultPath + L"/Models";
	if (_waccess(modelPath.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(modelPath.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (_wmkdir(modelPath.c_str()) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	writeReferencesAndModels(referencePath, modelPath);
	writeLegoBlocks(legoBlockPath);

	// create image directory
	if (!processor->getTextureInfo().empty())
	{
		outputFolderExist = false;
		std::wstring imagePath = resultPath + L"/Images_Resized";
		if (_waccess(imagePath.c_str(), 0) == 0)
		{
			struct _stat64i32 status;
			_wstat(imagePath.c_str(), &status);
			if (status.st_mode & S_IFDIR)
				outputFolderExist = true;
		}
		if (!outputFolderExist)
		{
			if (_wmkdir(imagePath.c_str()) != 0)
			{
				LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
				LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_DIRECTORY), false);
				return false;
			}
		}

		writeLegoTexture(resultPath);
		writeTextures(imagePath);
	}

	return true;
}

bool F4DWriter::writeHeader(FILE* f)
{
	// versoin div : start(20170323)
	// version
	fwrite(version.c_str(), sizeof(char), 5, f);
	// guid length
	fwrite(&guidLength, sizeof(int), 1, f);
	// guid
	fwrite(guid.c_str(), sizeof(char), guidLength, f);
	// representative longitude, latitude, altitude
	double longitude = processor->getLongitude(), latitude = processor->getLatitude();
	float altitude = processor->getAltitude();
	fwrite(&longitude, sizeof(double), 1, f);
	fwrite(&latitude, sizeof(double), 1, f);
	fwrite(&altitude, sizeof(float), 1, f);
	// bounding box
	float minX = (float)processor->getBoundingBox().minX, minY = (float)processor->getBoundingBox().minY, minZ = (float)processor->getBoundingBox().minZ;
	float maxX = (float)processor->getBoundingBox().maxX, maxY = (float)processor->getBoundingBox().maxY, maxZ = (float)processor->getBoundingBox().maxZ;
	fwrite(&minX, sizeof(float), 1, f); fwrite(&minY, sizeof(float), 1, f); fwrite(&minZ, sizeof(float), 1, f);
	fwrite(&maxX, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);

	// assymetric octree info
	writeOctreeInfo(processor->getSpatialOctree(), f);
	// versoin div : end

	return true;
}

bool F4DWriter::writeModels(FILE* f, std::vector<gaia3d::TrianglePolyhedron*>& models)
{
	unsigned int modelCount = (unsigned int)models.size();
	fwrite(&modelCount, sizeof(unsigned int), 1, f);

	gaia3d::TrianglePolyhedron* model;
	unsigned int modelIndex;
	float minX, minY, minZ, maxX, maxY, maxZ;
	unsigned int vboCount, vertexCount, indexCount;
	unsigned char sizeLevels;
	float thresholds[TriangleSizeLevels];
	float x, y, z;
	char nx, ny, nz;
	bool bInterleavedMode = false;
	char padding = 0;
	unsigned short index;
	for(size_t i = 0; i < modelCount; i++)
	{
		model = models[i];
		modelIndex = (unsigned int)model->getReferenceInfo().modelIndex;
		fwrite(&modelIndex, sizeof(unsigned int), 1, f);

		// bounding box
		minX = (float)model->getBoundingBox().minX; minY = (float)model->getBoundingBox().minY; minZ = (float)model->getBoundingBox().minZ;
		maxX = (float)model->getBoundingBox().maxX; maxY = (float)model->getBoundingBox().maxY; maxZ = (float)model->getBoundingBox().maxZ;
		fwrite(&minX, sizeof(float), 1, f); fwrite(&minY, sizeof(float), 1, f); fwrite(&minZ, sizeof(float), 1, f);
		fwrite(&maxX, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);

		// vbo count
		vboCount = (unsigned int)model->getVbos().size();
		fwrite(&vboCount, sizeof(unsigned int), 1, f);

		// vbo
		for(unsigned int j = 0; j < vboCount; j++)
		{
			vertexCount = (unsigned int)model->getVbos()[j]->vertices.size();
			// vertex count
			fwrite(&vertexCount, sizeof(unsigned int), 1, f);

			// vertex positions
			for(unsigned int k = 0; k < vertexCount; k++)
			{
				x = (float)model->getVbos()[j]->vertices[k]->position.x;
				y = (float)model->getVbos()[j]->vertices[k]->position.y;
				z = (float)model->getVbos()[j]->vertices[k]->position.z;
				fwrite(&x, sizeof(float), 1, f); fwrite(&y, sizeof(float), 1, f); fwrite(&z, sizeof(float), 1, f);
			}

			// normal count
			fwrite(&vertexCount, sizeof(unsigned int), 1, f);

			// normals
			for(unsigned int k = 0; k < vertexCount; k++)
			{
				nx = (char)(127.0f * model->getVbos()[j]->vertices[k]->normal.x);
				ny = (char)(127.0f * model->getVbos()[j]->vertices[k]->normal.y);
				nz = (char)(127.0f * model->getVbos()[j]->vertices[k]->normal.z);
				fwrite(&nx, sizeof(char), 1, f); fwrite(&ny, sizeof(char), 1, f); fwrite(&nz, sizeof(char), 1, f);
				if(bInterleavedMode)
					fwrite(&padding, sizeof(char), 1, f);
			}

			// index count
			indexCount =  (unsigned int)model->getVbos()[j]->indices.size();
			fwrite(&indexCount, sizeof(unsigned int), 1, f); // 전체 index개수
			sizeLevels = TriangleSizeLevels; // 삼각형을 크기별로 정렬할 때 적용된 크기 개수
			fwrite(&sizeLevels, sizeof(unsigned char), 1, f);
			for(unsigned char k = 0; k < sizeLevels; k++)
				thresholds[k] = (float)model->getVbos()[j]->triangleSizeThresholds[k]; // 삼각형을 크기별로 정렬할 때 적용된 크기들
			fwrite(thresholds, sizeof(float), sizeLevels, f);
			fwrite(model->getVbos()[j]->indexMarker, sizeof(unsigned int), sizeLevels, f); // 정렬된 vertex들의 index에서 크기 기준이 변경되는 최초 삼각형의 vertex의 index 위치 marker
			for(size_t k = 0; k < indexCount; k++)
			{
				index = models[i]->getVbos()[j]->indices[k];
				fwrite(&index, sizeof(unsigned short), 1, f);
			}
		}
	}

	return true;
}

bool F4DWriter::writeReferencesAndModels(std::wstring& referencePath, std::wstring& modelPath)
{
	std::vector<gaia3d::OctreeBox*> leafBoxes;
	gaia3d::OctreeBox* leafBox;
	processor->getSpatialOctree()->getAllLeafBoxes(leafBoxes, true);
	size_t leafCount = leafBoxes.size();
	std::wstring referenceFilePath; // each reference file full path
	std::wstring modelFilePath; // each model file full path
	std::vector<gaia3d::TrianglePolyhedron*> models;
	size_t modelCount;
	FILE* file = NULL;
	unsigned int referenceCount; // total reference count
	size_t meshCount = processor->getAllMeshes().size();
	unsigned int referenceId, modelId; // referenc id, block id which reference is refering to
	unsigned char objectIdLength;
	std::string objectId;
	gaia3d::TrianglePolyhedron* reference;
	gaia3d::TrianglePolyhedron* model;
	bool bFound;
	unsigned int vertexCount; // vertex count in a reference
	float m;	// element of transform matrix by which referend block is transformed into current reference position
	bool bColor; // if color exists
	unsigned short valueType; // array value type
	unsigned char colorDimension; // color channel count
	bool bTextureCoordinate; // if texture coordinate exists
	//float u, v;	// texture coordinate
	unsigned int textureCount = 0;
	for(size_t i = 0; i < leafCount; i++)
	{
		leafBox = leafBoxes[i];

		referenceFilePath = referencePath + L"/" + std::to_wstring((long long)((gaia3d::SpatialOctreeBox*)leafBoxes[i])->octreeId) + std::wstring(L"_Ref");
		file = _wfopen(referenceFilePath.c_str(), L"wb");

		// reference count in each octrees
		referenceCount = (unsigned int)leafBox->meshes.size();
		fwrite(&referenceCount, sizeof(unsigned int), 1, file);

		for(unsigned int j = 0; j < referenceCount; j++)
		{
			reference = leafBox->meshes[j];

			// extract models
			if(reference->getReferenceInfo().model == NULL)
				model = reference;
			else
				model = reference->getReferenceInfo().model;
			bFound = false;
			modelCount = models.size();
			for(size_t k = 0; k < modelCount; k++)
			{
				if(models[k] == model)
				{
					bFound = true;
					break;
				}
			}

			if(!bFound)
				models.push_back(model);

			// reference id
			referenceId = (unsigned int)reference->getId();
			fwrite(&referenceId, sizeof(unsigned int), 1, file);

			// reference object id
			if(reference->doesStringAttributeExist(std::wstring(ObjectGuid)))
			{
				std::wstring wObjectId = reference->getStringAttribute(std::wstring(ObjectGuid));
				objectId = std::string(CW2A(wObjectId.c_str()));
				objectIdLength = (unsigned char)objectId.length();
				fwrite(&objectIdLength, sizeof(unsigned char), 1, file);
				if (objectIdLength > 0)
					fwrite(objectId.c_str(), sizeof(char), objectIdLength, file);
			}
			else
			{
				objectIdLength = (unsigned char)0;
				fwrite(&objectIdLength, sizeof(unsigned char), 1, file);
			}
			

			// model id
			modelId = (unsigned int)reference->getReferenceInfo().modelIndex;
			fwrite(&modelId, sizeof(unsigned int), 1, file);

			// transform matrix
			for(size_t c = 0; c < 4; c++)
			{
				for(size_t r = 0; r < 4; r++)
				{
					m = (float)reference->getReferenceInfo().mat.m[c][r];
					fwrite(&m, sizeof(float), 1, file);
				}
			}

			vertexCount = (unsigned int)reference->getVertices().size();

			// representative color of this reference
			if(reference->getColorMode() == gaia3d::SingleColor)
			{
				bColor = true;
				fwrite(&bColor, sizeof(bool), 1, file);
				valueType = 5121; // (5120 signed byte), (5121 unsigned byte), (5122 signed short), (5123 unsigned short), (5126 float)
				fwrite(&valueType, sizeof(unsigned short), 1, file);
				colorDimension = 4;
				fwrite(&colorDimension, sizeof(unsigned char), 1, file);
				writeColor(reference->getSingleColor(), valueType, true, file);
			}
			else
			{
				bColor = false;
				fwrite(&bColor, sizeof(bool), 1, file);
			}

			// if vertex color & texture coordinate exist
			bColor = (reference->getColorMode() == gaia3d::ColorsOnVertices) ? true : false;
			fwrite(&bColor, sizeof(bool), 1, file);
			bTextureCoordinate = reference->doesThisHaveTextureCoordinates();
			fwrite(&bTextureCoordinate, sizeof(bool), 1, file);

			// save vertex color & texture coordinate on vbo count loop
			if (bColor || bTextureCoordinate)
			{
				unsigned int vboCount = (unsigned int)reference->getVbos().size();
				fwrite(&vboCount, sizeof(unsigned int), 1, file);

				for (unsigned int k = 0; k < vboCount; k++)
				{
					gaia3d::Vbo* vbo = reference->getVbos()[k];
					unsigned int vboVertexCount = (unsigned int)vbo->vertices.size();

					if (bColor)
					{
						valueType = 5121;
						fwrite(&valueType, sizeof(unsigned short), 1, file);
						colorDimension = 3;
						fwrite(&colorDimension, sizeof(unsigned char), 1, file);

						fwrite(&vboVertexCount, sizeof(unsigned int), 1, file);
						for (unsigned int j = 0; j < vboVertexCount; j++)
							writeColor(vbo->vertices[j]->color, valueType, false, file);
					}

					if (bTextureCoordinate)
					{
						valueType = 5126;
						fwrite(&valueType, sizeof(unsigned short), 1, file);

						fwrite(&vboVertexCount, sizeof(unsigned int), 1, file);
						for (unsigned int j = 0; j < vboVertexCount; j++)
						{
							float tx = (float)vbo->vertices[j]->textureCoordinate[0];
							float ty = (float)vbo->vertices[j]->textureCoordinate[1];
							fwrite(&tx, sizeof(float), 1, file);
							fwrite(&ty, sizeof(float), 1, file);
						}
					}
				}
			}

			// texture file info if exists
			if(bTextureCoordinate)
			{
				textureCount = 1;
				fwrite(&textureCount, sizeof(unsigned int), 1, file);

				std::string textureType("diffuse");
				unsigned int typeLength = (unsigned int)textureType.length();
				fwrite(&typeLength, sizeof(unsigned int), 1, file);
				fwrite(textureType.c_str(), sizeof(char), typeLength, file);

				std::string textureName(CW2A(reference->getStringAttribute(TextureName).c_str(), CP_UTF8));
				unsigned int nameLength = (unsigned int)textureName.length();
				fwrite(&nameLength, sizeof(unsigned int), 1, file);
				fwrite(textureName.c_str(), sizeof(char), nameLength, file);
			}
			else
			{
				textureCount = 0;
				fwrite(&textureCount, sizeof(unsigned int), 1, file);
			}
		}

		writeVisibilityIndices(file, (static_cast<gaia3d::SpatialOctreeBox*>(leafBox))->exteriorOcclusionInfo);
		writeVisibilityIndices(file, (static_cast<gaia3d::SpatialOctreeBox*>(leafBox))->interiorOcclusionInfo);
		fclose(file);

		modelFilePath = modelPath + L"/" + std::to_wstring((long long)((gaia3d::SpatialOctreeBox*)leafBoxes[i])->octreeId) + L"_Model";
		file = _wfopen(modelFilePath.c_str(), L"wb");
		this->writeModels(file, models);
		fclose(file);
		models.clear();
	}

	return true;
}

bool F4DWriter::writeVisibilityIndices(FILE* f, gaia3d::OctreeBox* octree)
{
	bool bRoot = octree->parent == NULL ? true : false;
	fwrite(&bRoot, sizeof(bool), 1, f);
	if (bRoot)
	{
		float minX, minY, minZ, maxX, maxY, maxZ;

		minX = (float)octree->minX;
		minY = (float)octree->minY;
		minZ = (float)octree->minZ;
		maxX = (float)octree->maxX;
		maxY = (float)octree->maxY;
		maxZ = (float)octree->maxZ;
		fwrite(&minX, sizeof(float), 1, f); fwrite(&maxX, sizeof(float), 1, f);
		fwrite(&minY, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f);
		fwrite(&minZ, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);
	}

	unsigned int childCount = (unsigned int)octree->children.size();
	fwrite(&childCount, sizeof(unsigned int), 1, f);

	if (childCount == 0)
	{
		unsigned int referenceCount = (unsigned int)octree->meshes.size();
		fwrite(&referenceCount, sizeof(unsigned int), 1, f);

		unsigned int referenceId;
		for (unsigned int i = 0; i < referenceCount; i++)
		{
			referenceId = (unsigned int)octree->meshes[i]->getId();
			fwrite(&referenceId, sizeof(unsigned int), 1, f);
		}
	}
	else
	{
		for (unsigned int i = 0; i < childCount; i++)
		{
			writeVisibilityIndices(f, octree->children[i]);
		}
	}

	return true;
}

bool F4DWriter::writeLegoBlocks(std::wstring& legoBlockPath)
{
	std::map<size_t, gaia3d::TrianglePolyhedron*>::iterator itr = processor->getLegos().begin();
	std::wstring octreeLegoFilePath;
	FILE* file = NULL;
	size_t key;
	gaia3d::TrianglePolyhedron* lego;
	float minX, minY, minZ, maxX, maxY, maxZ; // bounding box
	unsigned int vertexCount;	// vertex count of each lego
	float x, y, z;
	bool bNormal;
	char nx, ny, nz;
	bool bInterleavedMode = false;
	char padding = 0;
	bool bColor;
	bool bTexture;
	for(; itr != processor->getLegos().end(); itr++)
	{
		// octree lego file
		key = itr->first;
		lego = itr->second;

		octreeLegoFilePath = legoBlockPath + L"/"+ std::to_wstring((unsigned long long)key) + std::wstring(L"_Brick");
		file = _wfopen(octreeLegoFilePath.c_str(), L"wb");

		// bounding box
		minX = (float)lego->getBoundingBox().minX; minY = (float)lego->getBoundingBox().minY; minZ = (float)lego->getBoundingBox().minZ;
		maxX = (float)lego->getBoundingBox().maxX; maxY = (float)lego->getBoundingBox().maxY; maxZ = (float)lego->getBoundingBox().maxZ;
		fwrite(&minX, sizeof(float), 1, file); fwrite(&minY, sizeof(float), 1, file); fwrite(&minZ, sizeof(float), 1, file);
		fwrite(&maxX, sizeof(float), 1, file); fwrite(&maxY, sizeof(float), 1, file); fwrite(&maxZ, sizeof(float), 1, file);

		// vertices count
		vertexCount = (unsigned int)lego->getVertices().size();
		fwrite(&vertexCount, sizeof(unsigned int), 1, file);
		
		// vertex positions
		for(unsigned int i = 0; i < vertexCount; i++)
		{
			x = (float)lego->getVertices()[i]->position.x; y = (float)lego->getVertices()[i]->position.y; z = (float)lego->getVertices()[i]->position.z;
			fwrite(&x, sizeof(float), 1, file); fwrite(&y, sizeof(float), 1, file); fwrite(&z, sizeof(float), 1, file);
		}

		// normals
		bNormal = lego->doesThisHaveNormals();
		fwrite(&bNormal, sizeof(bool), 1, file);
		if(bNormal)
		{
			fwrite(&vertexCount, sizeof(unsigned int), 1, file);
			for(unsigned int i = 0; i < vertexCount; i++)
			{
				nx = (char)(127.0f * lego->getVertices()[i]->normal.x);
				ny = (char)(127.0f * lego->getVertices()[i]->normal.y);
				nz = (char)(127.0f * lego->getVertices()[i]->normal.z);
				fwrite(&nx, sizeof(char), 1, file); fwrite(&ny, sizeof(char), 1, file); fwrite(&nz, sizeof(char), 1, file);
				if(bInterleavedMode)
					fwrite(&padding, sizeof(char), 1, file);
			}
		}

		// colors
		if(lego->getColorMode() == gaia3d::ColorsOnVertices)
		{
			bColor = true;
			fwrite(&bColor, sizeof(bool), 1, file);
			fwrite(&vertexCount, sizeof(unsigned int), 1, file);
			for(unsigned int i = 0; i < vertexCount; i++)
			{
				writeColor(lego->getVertices()[i]->color, 5121, true, file);
			}
		}
		else
		{
			bColor = false;
			fwrite(&bColor, sizeof(bool), 1, file);
		}

		// texture coordinate and resource
		bTexture = lego->doesThisHaveTextureCoordinates();
		fwrite(&bTexture, sizeof(bool), 1, file);
		if (bTexture)
		{
			// save the data type.***
			// (5120 signed byte), (5121 unsigned byte), (5122 signed short), (5123 unsigned short), (5126 float)
			unsigned short type = 5126;
			fwrite(&type, sizeof(unsigned short), 1, file);

			// vertex count
			fwrite(&vertexCount, sizeof(unsigned int), 1, file);
			// texture coordinates of each vertex
			for (unsigned int i = 0; i < vertexCount; i++)
			{
				float tx = (float)lego->getVertices()[i]->textureCoordinate[0];
				float ty = (float)lego->getVertices()[i]->textureCoordinate[1];
				fwrite(&tx, sizeof(float), 1, file);
				fwrite(&ty, sizeof(float), 1, file);
			}
		}
		fclose(file);
	}

	return true;
}

bool F4DWriter::writeOctreeInfo(gaia3d::OctreeBox* octree, FILE* f)
{
	unsigned int level = octree->level;
	fwrite(&level, sizeof(unsigned int), 1, f);

	float minX, minY, minZ, maxX, maxY, maxZ;
	if(level == 0)
	{
		minX = (float)octree->minX;
		minY = (float)octree->minY;
		minZ = (float)octree->minZ;
		maxX = (float)octree->maxX;
		maxY = (float)octree->maxY;
		maxZ = (float)octree->maxZ;
		fwrite(&minX, sizeof(float), 1, f); fwrite(&maxX, sizeof(float), 1, f);
		fwrite(&minY, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f);
		fwrite(&minZ, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);
	}

	unsigned char childCount = (unsigned char)octree->children.size();
	fwrite(&childCount, sizeof(unsigned char), 1, f);

	unsigned int triangleCount = 0;
	size_t meshCount = octree->meshes.size();
	size_t surfaceCount;
	for(size_t i = 0; i < meshCount; i++)
	{
		surfaceCount = octree->meshes[i]->getSurfaces().size();
		for(size_t j = 0; j < surfaceCount; j++)
			triangleCount += (unsigned int)octree->meshes[i]->getSurfaces()[j]->getTriangles().size();
	}
	fwrite(&triangleCount, sizeof(unsigned int), 1, f);

	for(unsigned char i = 0; i < childCount; i++)
	{
		writeOctreeInfo(octree->children[i], f);
	}

	return true;
}

void F4DWriter::writeColor(unsigned long color, unsigned short type, bool bAlpha, FILE* file)
{
	// color type : (5120 signed byte), (5121 unsigned byte), (5122 signed short), (5123 unsigned short), (5126 float)

	unsigned char red = GetRedValue(color);
	unsigned char green = GetGreenValue(color);
	unsigned char blue = GetBlueValue(color);
	unsigned char alpha = 255;

	switch(type)
	{
	case 5121: // unsigned char mode
		{
			fwrite(&red, sizeof(unsigned char), 1, file);
			fwrite(&green, sizeof(unsigned char), 1, file);
			fwrite(&blue, sizeof(unsigned char), 1, file);
			if(bAlpha)
				fwrite(&alpha, sizeof(unsigned char), 1, file);
		}
		break;
	case 5126: // float mode
		{
			float fRed = red / 255.0f;
			float fGreen = green / 255.0f;
			float fBlue = blue / 255.0f;
			fwrite(&fRed, sizeof(float), 1, file);
			fwrite(&fGreen, sizeof(float), 1, file);
			fwrite(&fBlue, sizeof(float), 1, file);
			if(bAlpha)
			{
				float fAlpha = 1.0f;
				fwrite(&fAlpha, sizeof(float), 1, file);
			}
		}
		break;
	default: // TODO(khj 20170324) : NYI the other color type
		break;
	}
}

bool F4DWriter::writeIndexFile()
{
	_wfinddata64_t fd;
    long long handle;
    int result = 1;
	std::wstring structureJtFilter = folder + std::wstring(L"/*.*");
	handle = _wfindfirsti64(structureJtFilter.c_str(), &fd);

	if(handle == -1)
	{
		return false;
	}

	std::vector<std::wstring> convertedDataFolders;
	while(result != -1)
	{
		if((fd.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			if(std::wstring(fd.name) != L"." && std::wstring(fd.name) != L"..")
				convertedDataFolders.push_back(std::wstring(fd.name));
		}
		result = _wfindnexti64(handle, &fd);
	}

	_findclose(handle);

	if(convertedDataFolders.size() == 0)
		return false;

	std::wstring targetFilePath = folder + L"/objectIndexFile.ihe";
	FILE* f;
	f = _wfopen(targetFilePath.c_str(), L"wb");
	
	unsigned int dataFolderCount = (unsigned int)convertedDataFolders.size();
	fwrite(&dataFolderCount, sizeof(unsigned int), 1, f);

	std::wstring eachDataHeader;
	char version[6];	
	int guidLength;
	char guid[256];
	memset(guid, 0x00, 256);
	double longitude, latitude;
	float altitude;
	float minX, minY, minZ, maxX, maxY, maxZ;
	unsigned int dataFolderNameLength;
	for(size_t i = 0; i < dataFolderCount; i++)
	{
		eachDataHeader = folder + L"/" + convertedDataFolders[i] + L"/HeaderAsimetric.hed";
		FILE* header;
		header = _wfopen(eachDataHeader.c_str(), L"rb");

		// version
		memset(version, 0x00, 6);
		fread(version, sizeof(char), 5, header);
		// guid length
		fread(&guidLength, sizeof(int), 1, header);
		// guid
		memset(guid, 0x00, 256);
		fread(guid, sizeof(char), guidLength, header);
		// representative longitude, latitude, altitude
		fread(&longitude, sizeof(double), 1, header);
		fread(&latitude, sizeof(double), 1, header);
		fread(&altitude, sizeof(float), 1, header);
		// bounding box
		fread(&minX, sizeof(float), 1, header); fread(&minY, sizeof(float), 1, header); fread(&minZ, sizeof(float), 1, header);
		fread(&maxX, sizeof(float), 1, header); fread(&maxY, sizeof(float), 1, header); fread(&maxZ, sizeof(float), 1, header);

		fclose(header);

		dataFolderNameLength = (unsigned int)convertedDataFolders[i].length();
		fwrite(&dataFolderNameLength, sizeof(unsigned int), 1, f);
		std::string singleConvertedDataFolder(CW2A(convertedDataFolders[i].c_str()));
		fwrite(singleConvertedDataFolder.c_str(), sizeof(char), dataFolderNameLength, f);

		fwrite(&longitude, sizeof(double), 1, f);
		fwrite(&latitude, sizeof(double), 1, f);
		fwrite(&altitude, sizeof(float), 1, f);

		fwrite(&minX, sizeof(float), 1, f); fwrite(&minY, sizeof(float), 1, f); fwrite(&minZ, sizeof(float), 1, f);
		fwrite(&maxX, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);
	}


	fclose(f);

	return true;
}

void F4DWriter::writeLegoTexture(std::wstring resultPath)
{
	Gdiplus::GdiplusStartupInput gdiplusstartupinput;
	unsigned long long gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusstartupinput, NULL);

	// at first, let's get png encoder from OS
	unsigned int num = 0, size = 0;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (num == 0 || size == 0)
	{
		Gdiplus::GdiplusShutdown(gdiplusToken);
		return;
	}

	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)new char[size];
	memset(pImageCodecInfo, 0x00, size);
	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	bool bFoundCodec = false;
	std::wstring mimeType = L"image/png";
	CLSID encId;
	for (unsigned int i = 0; i < num; i++)
	{
		std::wstring codec(pImageCodecInfo[i].MimeType);
		if (codec.compare(mimeType) == 0)
		{
			encId = pImageCodecInfo[i].Clsid;
			bFoundCodec = true;
			break;
		}
	}

	if (!bFoundCodec)
	{
		delete[](char*)pImageCodecInfo;
		Gdiplus::GdiplusShutdown(gdiplusToken);
		return;
	}

	unsigned int* legoTextureDimension = processor->getLegoTextureDimension();
	unsigned char* legoTextureByteArray = processor->getLegoTextureBitmapArray();
	unsigned int stride = legoTextureDimension[0] * 4;
	/*
	Gdiplus::Bitmap legoTexture(legoTextureDimension[0], legoTextureDimension[1], stride, PixelFormat32bppARGB, legoTextureByteArray);

	std::wstring legoTextureFullPath = resultPath + L"/SimpleBuildingTexture3x3.png";
	legoTexture.Save(legoTextureFullPath.c_str(), &encId);
	*/

	Gdiplus::Bitmap* legoTexture = new Gdiplus::Bitmap(legoTextureDimension[0], legoTextureDimension[1], stride, PixelFormat32bppARGB, legoTextureByteArray);
	std::wstring legoTextureFullPath = resultPath + L"/SimpleBuildingTexture3x3.png";
	legoTexture->Save(legoTextureFullPath.c_str(), &encId);

	delete legoTexture;

	delete[] (char*)pImageCodecInfo;

	Gdiplus::GdiplusShutdown(gdiplusToken);
}

void F4DWriter::writeTextures(std::wstring imagePath)
{
	Gdiplus::GdiplusStartupInput gdiplusstartupinput;
	unsigned long long gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusstartupinput, NULL);

	unsigned int num = 0, size = 0;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (num == 0 || size == 0)
	{
		Gdiplus::GdiplusShutdown(gdiplusToken);
		return;
	}

	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)new char[size];
	memset(pImageCodecInfo, 0x00, size);
	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	bool bFoundCodec = false;
	std::wstring mimeTypeBmp = L"image/bmp",  mimeTypePng = L"image/png", mimeTypeJpg = L"image/jpeg", mimeTypeGif = L"image/gif", mimeTypeTif = L"image/tiff";
	CLSID encIdBmp, encIdPng, encIdJpg, encIdGif, encIdTif;
	for (unsigned int i = 0; i < num; i++)
	{
		std::wstring codec(pImageCodecInfo[i].MimeType);
		if (codec.compare(mimeTypeBmp) == 0)
		{
			encIdBmp = pImageCodecInfo[i].Clsid;
			continue;
		}
		if (codec.compare(mimeTypePng) == 0)
		{
			encIdPng = pImageCodecInfo[i].Clsid;
			continue;
		}
		if (codec.compare(mimeTypeJpg) == 0)
		{
			encIdJpg = pImageCodecInfo[i].Clsid;
			continue;
		}
		if (codec.compare(mimeTypeGif) == 0)
		{
			encIdGif = pImageCodecInfo[i].Clsid;
			continue;
		}
		if(codec.compare(mimeTypeTif) == 0)
		{
			encIdTif = pImageCodecInfo[i].Clsid;
			continue;
		}
	}

	CLSID encId;
	std::map<std::wstring, std::wstring>::iterator itr = processor->getTextureInfo().begin();
	for (; itr != processor->getTextureInfo().end(); itr++)
	{
		std::wstring fileName = itr->first;

		std::wstring::size_type dotPosition = fileName.rfind(L".");
		if (dotPosition == std::wstring::npos)
			continue;

		std::wstring fileExt = fileName.substr(dotPosition + 1, fileName.length() - dotPosition - 1);
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

		if (fileExt.compare(L"tga") == 0 || fileExt.compare(L"dds") == 0)
		{
			std::wstring orginalImagePath = itr->second;
			FILE* file = NULL;
			file = _wfopen(orginalImagePath.c_str(), L"rb");
			if (file == NULL)
				continue;
			
			fseek(file, 0, SEEK_END);
			long fileSize = ftell(file);
			if (fileSize <= 0)
				continue;

			rewind(file);
			unsigned char* fileContents = new unsigned char[fileSize];
			memset(fileContents, 0x00, sizeof(unsigned char)*fileSize);

			fread(fileContents, sizeof(unsigned char), fileSize, file);
			fclose(file);
			file = NULL;

			std::wstring fullPath = imagePath + L"/" + fileName;
			file = _wfopen(fullPath.c_str(), L"wb");
			if (file == NULL)
			{
				delete[] fileContents;
				continue;
			}

			fwrite(fileContents, sizeof(unsigned char), fileSize, file);
			fclose(file);
			delete[] fileContents;
		}
		else
		{
			std::wstring mimeType;
			if (fileExt.compare(L"jpg") == 0 || fileExt.compare(L"jpeg") == 0 || fileExt.compare(L"jpe") == 0)
			{
				encId = encIdJpg;
			}
			else if (fileExt.compare(L"png") == 0)
			{
				encId = encIdPng;
			}
			else if (fileExt.compare(L"gif") == 0)
			{
				encId = encIdGif;
			}
			else if (fileExt.compare(L"tif") == 0 || fileExt.compare(L"tiff") == 0)
			{
				encId = encIdTif;
			}
			else if (fileExt.compare(L"bmp"))
			{
				encId = encIdPng;
			}
			else
				continue;

			unsigned int width = processor->getAllTextureWidths()[fileName];
			unsigned int height = processor->getAllTextureHeights()[fileName];
			unsigned char* bmpArray = processor->getResizedTextures()[fileName];
			Gdiplus::Bitmap* resizedBmp = new Gdiplus::Bitmap(width, height, width * 4, PixelFormat32bppARGB, bmpArray);

			std::wstring fullPath = imagePath + L"/" + fileName;
			resizedBmp->Save(fullPath.c_str(), &encId);
			delete resizedBmp;
		}
	}

	delete[](char*)pImageCodecInfo;
	Gdiplus::GdiplusShutdown(gdiplusToken);
}