#pragma once

#include <string>
#include <vector>

class ConversionProcessor;

namespace gaia3d
{
	class OctreeBox;
	class TrianglePolyhedron;
}

class F4DWriter
{
public:
	F4DWriter(ConversionProcessor* conversionResult);

	virtual ~F4DWriter();

public:
	ConversionProcessor* processor;

	std::wstring folder;

	std::string version;

	std::string guid;

	int guidLength;


public:
	void setWriteFolder(std::wstring folderPath) {folder = folderPath;}

	bool write();

	bool writeIndexFile();

protected:
	bool writeHeader(FILE* f);

	bool writeVisibilityIndices(FILE* f, gaia3d::OctreeBox* octree);

	bool writeModels(FILE* f, std::vector<gaia3d::TrianglePolyhedron*>& models);

	bool writeReferencesAndModels(std::wstring& referencePath, std::wstring& modelPath);

	bool writeLegoBlocks(std::wstring& legoBlockPath);

	bool writeOctreeInfo(gaia3d::OctreeBox* octree, FILE* f);

	void writeColor(unsigned long color, unsigned short type, bool bAlpha, FILE* file);

	void writeLegoTexture(std::wstring resultPath);
	void writeTextures(std:: wstring imagePath);
};