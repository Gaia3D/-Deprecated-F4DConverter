
#include "stdafx.h"

#ifdef SHIJTFORMAT

#include "JtReader.h"

#include <time.h>
#include "../geometry/TrianglePolyhedron.h"
#include "../LogWriter.h"

// local function & magro defined
clock_t TimeOutLimit = 30000; // in mili-second
enum JtLoadingStatus {loadingFailed = -1, notYetLoaded, loadingDone};
int objectNodeDepth = 2;
JtLoadingStatus loadAllGeometryNodes(int trialNumber, JTData_NodePtr& node);
std::map<JTData_MeshNodePtr, unsigned long> meshBirthTimes;

JtReader::JtReader()
{
	geometrySource = NULL;

	thisHasUnloadableMesh = false;

	unitScaleFactor = 1.0;
}

JtReader::~JtReader()
{
	clear();
}

bool JtReader::readRawDataFile(std::wstring& filePath)
{
	std::string singleCharFilePath = std::string(CW2A(filePath.c_str()));
	geometrySource = JTData_GeometrySourcePtr(new JTData_GeometrySource());

	if ( !(geometrySource->Add(TCollection_ExtendedString(singleCharFilePath.c_str()), NULL)) )
	{
		return false;
	}

	
	JTData_SceneGraphPtr& sceneGraph = JTData_SceneGraphPtr();
	JtLoadingStatus loadingStatus = notYetLoaded;
	int trialNumber = 0;
	while(loadingStatus == notYetLoaded)
	{
		sceneGraph = geometrySource->SceneGraph();

		loadingStatus = loadAllGeometryNodes(trialNumber, sceneGraph->Tree());

		trialNumber++;
	}

	meshBirthTimes.clear();

	if(loadingStatus == loadingFailed)
		return false;

	bool bResult = extractGeometryInformation(0, sceneGraph->Tree(), std::wstring(L""), std::wstring(L""), JTData_TransformAttributePtr(NULL), JTData_MaterialAttributePtr(NULL));

	return bResult;
}

void JtReader::clear()
{
	geometrySource = NULL;

	container.clear();
}

bool JtReader::extractGeometryInformation(int depth,
											JTData_NodePtr& node,
											std::wstring& blockId,
											std::wstring& objectId,
											JTData_TransformAttributePtr& transform,
											JTData_MaterialAttributePtr& material)
{
	/*
	if (std::dynamic_pointer_cast<JTData_PartitionNode> (node) != NULL)
	{
		JTData_PartitionNodePtr partition = std::dynamic_pointer_cast<JTData_PartitionNode> (node);

		if(partition->Name().IsEmpty())
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(NO_BLOCK_ID), false);
			return false;
		}

		blockId = TCollection_AsciiString(TCollection_AsciiString(node->Name()).Token()).ToCString();
	}
	*/

	if(std::dynamic_pointer_cast<JTData_GroupNode> (node) != NULL)
	{
		JTData_GroupNodePtr group = std::dynamic_pointer_cast<JTData_GroupNode> (node);
		
		if(depth == objectNodeDepth)
		{
			if(group->Name().IsEmpty())
			{
				LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
				LogWriter::getLogWriter()->addContents(std::wstring(NO_OBJECT_ID), false);
				return false;
			}
			else
			{
				std::string singleCharObjectId = TCollection_AsciiString(TCollection_AsciiString(node->Name()).Token()).ToCString();
				objectId = std::wstring(CA2W(singleCharObjectId));
			}
				

			transform = NULL;
			material = NULL;
			for(size_t i = 0; i < group->Attributes.size(); i++)
			{
				if(std::dynamic_pointer_cast<JTData_TransformAttribute>(node->Attributes.at (i)) != NULL)
					transform = std::dynamic_pointer_cast<JTData_TransformAttribute>(node->Attributes.at (i));

				if(std::dynamic_pointer_cast<JTData_MaterialAttribute>(node->Attributes.at (i)) != NULL)
					material = std::dynamic_pointer_cast<JTData_MaterialAttribute>(node->Attributes.at (i));
			}

			// Transform matrix가 필수 요소가 아님으로 변경
			/*
			if(transform == NULL)
			{
				LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
				LogWriter::getLogWriter()->addContents(std::string(NO_TRANSFORM_MATRIX), false);
				return false;
			}
			*/
		}

		for(size_t i = 0; i < group->Children.size(); i++)
		{
			if(!extractGeometryInformation(depth+1, group->Children[i], blockId, objectId, transform, material))
				return false;
		}
	}

	if(std::dynamic_pointer_cast<JTData_MeshNode> (node) != NULL)
	{
		JTData_MeshNodePtr mesh = std::dynamic_pointer_cast<JTData_MeshNode> (node);

		mesh->State();

		const JTData_TriangulationSourcePtr &meshSource = mesh->Source();
		meshSource->RequestTriangulation(0, NULL);

		int triangleCount = meshSource->TriangleCount();

		size_t verticesCount = meshSource->Vertices().size();
		const float* verticesData = meshSource->Vertices().data();

		size_t indicesCount = meshSource->Indices().size();
		const int* indicesData = meshSource->Indices().data();

		size_t normalsCount = meshSource->Normals().size();
		const float* normalsData = meshSource->Normals().data();

		if(triangleCount == 0)
		{
			this->thisHasUnloadableMesh = true;
			return true;
		}

		if(verticesCount == 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(NO_VERTEX_COUNT), false);
			return false;
		}

		if(verticesData == NULL)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(NO_VERTEX_ARRAY), false);
			return false;
		}

		if(indicesCount == 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(NO_INDEX_COUNT), false);
			return false;
		}

		if(indicesData == NULL)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(NO_INDEX_ARRAY), false);
			return false;
		}

		if(normalsCount == 0)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(NO_NORMAL_COUNT), false);
			return false;
		}

		if(normalsData == NULL)
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(NO_NORMAL_ARRAY), false);
			return false;
		}

		
		gaia3d::Matrix4 mat4, mat4Normal;
		mat4.identity();
		mat4Normal.identity();

		if(transform != NULL)
		{
			const float* matrix = transform->Transform().data();
			mat4.set(matrix[0], matrix[1], matrix[2], matrix[3], 
					matrix[4], matrix[5], matrix[6], matrix[7], 
					matrix[8], matrix[9], matrix[10], matrix[11], 
					matrix[12], matrix[13], matrix[14], matrix[15]);

			mat4Normal.set(matrix[0], matrix[1], matrix[2], matrix[3], 
							matrix[4], matrix[5], matrix[6], matrix[7], 
							matrix[8], matrix[9], matrix[10], matrix[11], 
							0.0, 0.0, 0.0, 1.0);
		}
		
		float color4[4];
		color4[0] = 0.8f;
		color4[1] = 0.8f;
		color4[2] = 0.8f;
		color4[3] = 1.0f;

		if(material != NULL)
		{
			const float* ambientColor = material->AmbientColor().data();
			const float* diffuseColor = material->DiffuseColor().data();
			const float* specularColor = material->SpecularColor().data();
			const float* emissionColor = material->EmissionColor().data();
			float shiness = material->Shininess();

			memcpy(color4, diffuseColor, sizeof(float)*4);
		}

		// create an instance for a mesh, TrianglePolyhedron
		gaia3d::TrianglePolyhedron* polyhedron = new gaia3d::TrianglePolyhedron;
		polyhedron->setColorMode(gaia3d::SingleColor);
		polyhedron->setSingleColor(MakeColorU4( (unsigned char)(color4[0]*255), (unsigned char)(color4[1]*255), (unsigned char)(color4[2]*255) ) );
		gaia3d::ColorU4 meshColor = polyhedron->getSingleColor();
		polyhedron->setHasNormals(true);
		polyhedron->addStringAttribute(std::wstring(L"objectId"), objectId);

		// create vertices and insert them into the created mesh
		size_t vertexCount = verticesCount / 3;
		gaia3d::Vertex* vertex;
		for(size_t i = 0; i < vertexCount; i++)
		{
			vertex = new gaia3d::Vertex;

			vertex->position.set(verticesData[i*3], verticesData[i*3 + 1], verticesData[i*3 + 2]);
			vertex->position = mat4 * vertex->position;

			// unit change
			vertex->position.x = vertex->position.x * unitScaleFactor;
			vertex->position.y = vertex->position.y * unitScaleFactor;
			vertex->position.z = vertex->position.z * unitScaleFactor;
			//

			vertex->normal.set(normalsData[i*3], normalsData[i*3 + 1], normalsData[i*3 + 2]);
			vertex->normal = mat4Normal * vertex->normal;

			//memcpy(vertex->color4, color4, sizeof(float)*4);

			polyhedron->getVertices().push_back(vertex);
		}

		// create a surface. only 1 surface
		gaia3d::Surface* surface = new gaia3d::Surface;
		polyhedron->getSurfaces().push_back(surface);
		size_t realTriangleCount = indicesCount / 3;
		gaia3d::Triangle* triangle;
		for(size_t i = 0; i < realTriangleCount; i++)
		{
			triangle = new gaia3d::Triangle;
			triangle->getVertexIndices()[0] = indicesData[i*3];
			triangle->getVertexIndices()[1] = indicesData[i*3 + 1];
			triangle->getVertexIndices()[2] = indicesData[i*3 + 2];
			triangle->getVertices()[0] = polyhedron->getVertices()[indicesData[i*3]];
			triangle->getVertices()[1] = polyhedron->getVertices()[indicesData[i*3 + 1]];
			triangle->getVertices()[2] = polyhedron->getVertices()[indicesData[i*3 + 2]];

			surface->getTriangles().push_back(triangle);
		}

		polyhedron->setId(container.size());
		container.push_back(polyhedron);
	}

	return true;
}

JtLoadingStatus loadAllGeometryNodes(int trialNumber, JTData_NodePtr& node)
{
	if (std::dynamic_pointer_cast<JTData_GroupNode> (node) != NULL)
	{
		JtLoadingStatus loadingStatus = loadingDone, childLoadingStatus;
		JTData_GroupNodePtr group = std::dynamic_pointer_cast<JTData_GroupNode> (node);
		for(size_t i = 0; i < group->Children.size(); i++)
		{
			childLoadingStatus = loadAllGeometryNodes(trialNumber, group->Children[i]);

			if(childLoadingStatus == loadingFailed)
				return loadingFailed;

			if(childLoadingStatus == notYetLoaded)
				loadingStatus = notYetLoaded;
		}

		return loadingStatus;
	}
	else if( std::dynamic_pointer_cast<JTData_MeshNode> (node) != NULL)
	{
		JTData_MeshNodePtr mesh = std::dynamic_pointer_cast<JTData_MeshNode> (node);

		const JTData_TriangulationSourcePtr &meshSource = mesh->Source();
		meshSource->RequestTriangulation(0, NULL);

		if(trialNumber == 0)
		{
			clock_t birthTime = clock();
			meshBirthTimes.insert(std::map<JTData_MeshNodePtr, long>::value_type(mesh, birthTime));
			return notYetLoaded;
		}

		int triangleCount = meshSource->TriangleCount();

		if(triangleCount > 0)
			return loadingDone;
		else if(triangleCount == 0)
		{
			clock_t elapsedTime = clock() - meshBirthTimes[mesh];
			if(elapsedTime > TimeOutLimit)
				return loadingDone;
		
			return notYetLoaded;
		}
		else
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(INVALID_TRIANGLE_COUNT), false);
			return loadingFailed;
		}
		
	}
	else
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(UNKNOWN_NODE_TYPE), false);
		return loadingFailed;
	}
}

#endif

