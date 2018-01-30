
#include "stdafx.h"

#include <Gdiplus.h>
#include <algorithm>

#include "glew.h"
#include "SOIL.h"

#include "../predefinition.h"

#include "ConversionProcessor.h"
#include "SceneControlVariables.h"

#include "../LogWriter.h"
#include "../util/GeometryUtility.h"
#include "../geometry/Quadtree.h"
#include "../geometry/LegoBlock.h"

ConversionProcessor::ConversionProcessor()
:thisSpatialOctree(NULL)
{
	scv = new SceneControlVariables();

	longitude = latitude = 0.0;
	altitude = 0.0f;

	legoTextureBitmap = NULL;
	legoTextureDimension[0] = legoTextureDimension[1] = 0;
}

ConversionProcessor::~ConversionProcessor()
{
	delete scv;

	clear();
}

bool ConversionProcessor::initialize(HDC hDC, int width, int height)
{
	// TODO(khj 20170210) : NYI 여기서 SceneControlVariables를 초기화 해야 한다.

	// dc와 연결
	scv->m_myhDC = hDC;
	scv->m_width = width;
	scv->m_height = height;

	// projection mode
	scv->tp_projection = PROJECTION_PERSPECTIVE;

	// // Init vars lumm openGL.***
	scv->ambientLight[0]=0.2f; scv->ambientLight[1]=0.2f; scv->ambientLight[2]=0.2f; scv->ambientLight[3]=1.0f;
	scv->diffuseLight[0]=0.7f; scv->diffuseLight[1]=0.7f; scv->diffuseLight[2]=0.7f; scv->diffuseLight[3]=1.0f; 
	scv->specular[0]=0.3f; scv->specular[1]=0.3f; scv->specular[2]=0.3f; scv->specular[3]=1.0f; 
	scv->lightPos[0]=-1000000.0f; scv->lightPos[1]=-1000000.0f; scv->lightPos[2]=2000000.0f; scv->lightPos[3]=1.0f; // Hem de ferlo fixe.***
	scv->specref[0]=0.7f; scv->specref[1]=0.7f; scv->specref[2]=0.7f; scv->specref[3]=1.0f; 
	scv->ClearColor[0]=1.0f; scv->ClearColor[1]=1.0f; scv->ClearColor[2]=1.0f; scv->ClearColor[3]=1.0f; 

	static PIXELFORMATDESCRIPTOR pfd= {
	sizeof(PIXELFORMATDESCRIPTOR),
	// Size Of This Pixel Format Descriptor
	1,
	// Version Number (?)
	PFD_DRAW_TO_WINDOW | // Format Must Support Window
	PFD_SUPPORT_OPENGL | // Format Must Support OpenGL
	PFD_DOUBLEBUFFER, // Must Support Double Buffering
	PFD_TYPE_RGBA, // Request An RGBA Format
	24, // Select A 24Bit Color Depth
	0, 0, 0, 0, 0, 0, // Color Bits Ignored (?)
	0, // No Alpha Buffer
	0, // Shift Bit Ignored (?)
	0, // No Accumulation Buffer
	0, 0, 0, 0, // Accumulation Bits Ignored (?)
	32, // 16Bit Z-Buffer (Depth Buffer)
	0, // No Stencil Buffer
	0, // No Auxiliary Buffer (?)
	PFD_MAIN_PLANE, // Main Drawing Layer
	0, // Reserved (?)
	0, 0, 0 // Layer Masks Ignored (?)
	};

	GLuint PixelFormat;
	PixelFormat = ChoosePixelFormat(scv->m_myhDC, &pfd);
	if(PixelFormat == 0)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CHOOSE_PF), true);
		return false;
	}

	if(SetPixelFormat(scv->m_myhDC,PixelFormat,&pfd) == FALSE)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_SET_PF), true);
		return false;
	}

	scv->m_hRC = wglCreateContext(scv->m_myhDC);
	if(scv->m_hRC == NULL)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_GL_CONTEXT), true);
		return false;
	}

	if(wglMakeCurrent(scv->m_myhDC, scv->m_hRC) == FALSE)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CONNECT_GLC_TO_DC), true);
		return false;
	}

	//// initialize gl
	glEnable(GL_TEXTURE_2D);    
	// Enables Depth Testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);  
	glClearDepth(1.0f); 

	// Reset the current projection matrix
	scv->m_perspective_far = 10000.0;
	glViewport(0, 0, scv->m_width, scv->m_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(scv->m_perspective_angle,(GLfloat)scv->m_width/(GLfloat)scv->m_height, scv->m_perspective_near, scv->m_perspective_far);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	  
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT, GL_FILL);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); 
	glEnable(GL_POLYGON_SMOOTH);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0,1.0);
	//InitGL();
	//End InitGL------------------------------------------------------------------------------------------------------------------------------------------------
	// SetUp lighting.******************************************************************************************************************************************
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);

	// Setup and enable light_0.***
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, scv->ambientLight);
	//glLightModelf (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);// Test.***
	glLightfv(GL_LIGHT0, GL_AMBIENT, scv->ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, scv->diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, scv->specular);
	glLightfv(GL_LIGHT0, GL_POSITION, scv->lightPos);
	glEnable(GL_LIGHT0);

	// Enable color tracking.***
	glEnable(GL_COLOR_MATERIAL);

	// Set material properties to follow glColor values.***
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// A partir d'aqui, tots els materials tenen reflectivitat especular total amb brillo.***
	glMaterialfv(GL_FRONT, GL_SPECULAR, scv->specref);
	glMateriali(GL_FRONT, GL_SHININESS, 64);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	//SetUpLighting();// Increiblement "SetUpLighting()" ha d'anar a darrera de "InitGL()".***
	// End SetUp lighting.---------------------------------------------------------------------------------------------------------------------------------
	GLenum result = glewInit();
	if(result != GLEW_OK)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_INITIALIZE_GLEW), true);
		return false;
	}

	return true;
}

void ConversionProcessor::clear()
{
	fullBbox.isInitialized = false;

	attributes.clear();

	size_t meshCount = allMeshes.size();
	for(size_t i = 0; i < meshCount; i++)
		delete allMeshes[i];
	allMeshes.clear();

	thisSpatialOctree.clear();

	std::map<size_t, gaia3d::TrianglePolyhedron*>::iterator itr;
	for(itr = legos.begin(); itr != legos.end(); itr++)
		delete itr->second;
	legos.clear();

	allTextureInfo.clear();

	if(legoTextureBitmap != NULL)
	{
		delete[] legoTextureBitmap;
		legoTextureBitmap = NULL;
		legoTextureDimension[0] = legoTextureDimension[1] = 0;
	}

	if (!resizedTextures.empty())
	{
		std::map<std::wstring, unsigned char*>::iterator itr = resizedTextures.begin();
		for (; itr != resizedTextures.end(); itr++)
			delete[] itr->second;

		resizedTextures.clear();

		allTextureWidths.clear();
		allTextureHeights.clear();
	}
}

void ConversionProcessor::changeSceneControlVariables()
{
	// TODO(khj 20170210) : NYI 여기서 SceneControlVariables를 바꿀 수 있다.
}

void ConversionProcessor::addAttribute(std::wstring key, std::wstring value)
{
	attributes.insert(std::map<std::wstring, std::wstring>::value_type(key, value));
}

bool ConversionProcessor::proceedConversion(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
											std::map<std::wstring, std::wstring>& originalTextureInfo,
											bool bExtractExterior,
											bool bOcclusionCulling)
{
	// copy data from original to this container
	allMeshes.insert(allMeshes.end(), originalMeshes.begin(), originalMeshes.end());

	// copy texture info
	if(!originalTextureInfo.empty())
		allTextureInfo.insert(originalTextureInfo.begin(), originalTextureInfo.end());

	// calculate plane normals and align them to their vertex normals
	trimVertexNormals(allMeshes);

	// calculate bounding box
	calculateBoundingBox(allMeshes, fullBbox);

	// determine  which surfaces are exteriors
	if(bExtractExterior)
		determineWhichSurfacesAreExterior(allMeshes, fullBbox);

	// make model-reference relationship
	determineModelAndReference(allMeshes);

	size_t modelCount = 0;
	size_t meshCount = allMeshes.size();
	for(size_t i = 0; i < meshCount; i++)
	{
		if(allMeshes[i]->getReferenceInfo().model == NULL)
			modelCount++;
	}
	// make VBO
	makeVboObjects(allMeshes);

	// make generic spatial octree
	assignReferencesIntoEachSpatialOctrees(thisSpatialOctree, allMeshes, fullBbox, false, LeafSpatialOctreeSize);

	// make upper LOD(rougher LOD)
	bool bMakeTextureCoordinate = allTextureInfo.empty() ? false : true;
	makeLegoStructure(thisSpatialOctree, MinLegoSize, legos, bMakeTextureCoordinate);

	// make visibility indices
	if(bOcclusionCulling)
	{
		// exterior extraction is necessary for occlusion culling
		// so, if it is not done, do it before occlusion culling
		if (!bExtractExterior)
			determineWhichSurfacesAreExterior(allMeshes, fullBbox);

		std::vector<gaia3d::TrianglePolyhedron*> interiors, exteriors;
		assignReferencesIntoExteriorAndInterior(allMeshes, interiors, exteriors);

		gaia3d::BoundingBox interiorBbox, exteriorBbox;
		calculateBoundingBox(interiors, interiorBbox);
		calculateBoundingBox(exteriors, exteriorBbox);
			 
		// make occlusion culling information
		gaia3d::VisionOctreeBox interiorOcclusionOctree(NULL), exteriorOcclusionOctree(NULL);
		makeOcclusionInformation(allMeshes, interiorOcclusionOctree, exteriorOcclusionOctree, interiorBbox, exteriorBbox);

		// finally, import these information into data groups and interior spatial octree boxes
		applyOcclusionInformationOnSpatialOctree(thisSpatialOctree, interiorOcclusionOctree, exteriorOcclusionOctree);
	}

	if (bMakeTextureCoordinate)
	{
		// make lego texture
		makeLegoTexture(allMeshes, allTextureInfo);

		// rebuild original texture
		normalizeTextures(allTextureInfo);
	}

	return true;
}

void ConversionProcessor::trimVertexNormals(std::vector<gaia3d::TrianglePolyhedron*>& meshes)
{
	size_t meshCount = meshes.size();
	double anglePNormalAndVNormal0, anglePNormalAndVNormal1, anglePNormalAndVNormal2;
	for(size_t i = 0; i < meshCount; i++)
	{
		if (meshes[i]->doesThisHaveNormals())
		{
			std::vector<gaia3d::Surface*>& surfaces = meshes[i]->getSurfaces();
			size_t surfaceCount = surfaces.size();
			for (size_t j = 0; j < surfaceCount; j++)
			{
				std::vector<gaia3d::Triangle*>& triangles = surfaces[j]->getTriangles();
				size_t triangleCount = triangles.size();
				for (size_t k = 0; k < triangleCount; k++)
				{
					gaia3d::Triangle* triangle = triangles[k];
					gaia3d::Point3D planeNormal;
					gaia3d::GeometryUtility::calculatePlaneNormal(triangle->getVertices()[0]->position.x, triangle->getVertices()[0]->position.y, triangle->getVertices()[0]->position.z,
						triangle->getVertices()[1]->position.x, triangle->getVertices()[1]->position.y, triangle->getVertices()[1]->position.z,
						triangle->getVertices()[2]->position.x, triangle->getVertices()[2]->position.y, triangle->getVertices()[2]->position.z,
						planeNormal.x, planeNormal.y, planeNormal.z,
						true);

					

					anglePNormalAndVNormal0 = 180.0 / M_PI * gaia3d::GeometryUtility::angleBetweenTwoVectors(planeNormal.x, planeNormal.y, planeNormal.z,
						triangle->getVertices()[0]->normal.x,
						triangle->getVertices()[0]->normal.y,
						triangle->getVertices()[0]->normal.z);
					anglePNormalAndVNormal1 = 180.0 / M_PI * gaia3d::GeometryUtility::angleBetweenTwoVectors(planeNormal.x, planeNormal.y, planeNormal.z,
						triangle->getVertices()[1]->normal.x,
						triangle->getVertices()[1]->normal.y,
						triangle->getVertices()[1]->normal.z);
					anglePNormalAndVNormal2 = 180.0 / M_PI * gaia3d::GeometryUtility::angleBetweenTwoVectors(planeNormal.x, planeNormal.y, planeNormal.z,
						triangle->getVertices()[2]->normal.x,
						triangle->getVertices()[2]->normal.y,
						triangle->getVertices()[2]->normal.z);


					if (anglePNormalAndVNormal0 > 90.0 || anglePNormalAndVNormal0 > 90.0 || anglePNormalAndVNormal0 > 90.0)
					{
						size_t tempIndex = triangle->getVertexIndices()[0];
						triangle->getVertexIndices()[0] = triangle->getVertexIndices()[1];
						triangle->getVertexIndices()[1] = tempIndex;

						gaia3d::Vertex* tempVertex = triangle->getVertices()[0];
						triangle->getVertices()[0] = triangle->getVertices()[1];
						triangle->getVertices()[1] = tempVertex;

						triangle->setNormal(-planeNormal.x, -planeNormal.y, -planeNormal.z);
					}
					else
						*(triangle->getNormal()) = planeNormal;
				}
			}
		}
		else
		{
			std::vector<gaia3d::Surface*>& surfaces = meshes[i]->getSurfaces();
			size_t surfaceCount = surfaces.size();
			for (size_t j = 0; j < surfaceCount; j++)
			{
				std::vector<gaia3d::Triangle*>& triangles = surfaces[j]->getTriangles();
				size_t triangleCount = triangles.size();
				for (size_t k = 0; k < triangleCount; k++)
				{
					gaia3d::Triangle* triangle = triangles[k];
					gaia3d::Point3D planeNormal;
					gaia3d::GeometryUtility::calculatePlaneNormal(triangle->getVertices()[0]->position.x, triangle->getVertices()[0]->position.y, triangle->getVertices()[0]->position.z,
						triangle->getVertices()[1]->position.x, triangle->getVertices()[1]->position.y, triangle->getVertices()[1]->position.z,
						triangle->getVertices()[2]->position.x, triangle->getVertices()[2]->position.y, triangle->getVertices()[2]->position.z,
						planeNormal.x, planeNormal.y, planeNormal.z,
						true);

					*(triangle->getNormal()) = planeNormal;

					triangle->alignVertexNormalsToPlaneNormal();
				}
			}

			meshes[i]->setHasNormals(true);
		}
	}
}

void ConversionProcessor::calculateBoundingBox(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox)
{
	size_t meshCount = meshes.size();

	for(size_t i = 0; i < meshCount; i++)
	{
		calculateBoundingBox(meshes[i]);

		bbox.addBox(meshes[i]->getBoundingBox());
	}
}

void ConversionProcessor::determineWhichSurfacesAreExterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox)
{

	// allocate a unique color to each surface in all meshes
	std::vector<gaia3d::Surface*> allSurfaces;
	std::vector<gaia3d::ColorU4> allColors;
	size_t meshCount = meshes.size();
	size_t prevSurfaceCount, addedSurfaceCount;
	for(size_t i = 0; i < meshCount; i++)
	{
		prevSurfaceCount = allSurfaces.size();
		addedSurfaceCount = meshes[i]->getSurfaces().size();
		allSurfaces.insert(allSurfaces.end(), meshes[i]->getSurfaces().begin(), meshes[i]->getSurfaces().end());

		for(size_t j = prevSurfaceCount; j < prevSurfaceCount + addedSurfaceCount; j++)
		{
			allColors.push_back(MakeColorU4(GetRedValue(j), GetGreenValue(j), GetBlueValue(j)));
		}
	}

	glDisable( GL_TEXTURE_2D );
	glDisable(GL_CULL_FACE);
	scv->tp_projection = PROJECTION_ORTHO;

	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);

	scv->m_xRot=-60.0; scv->m_yRot=0.0; scv->m_zRot=22.5;
	for(int i=0; i<8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachSurfaceIsExterior(allSurfaces, allColors);
		scv->m_zRot+=45.0;
	}

	scv->m_xRot=-90.0; scv->m_yRot=0.0; scv->m_zRot=22.5;
	for(int i=0; i<8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachSurfaceIsExterior(allSurfaces, allColors);
		scv->m_zRot+=45.0;
	}

	scv->m_xRot=-120.0; scv->m_yRot=0.0; scv->m_zRot=22.5;
	for(int i=0; i<8; i++)
	{
		this->setupPerspectiveViewSetting(bbox);
		this->checkIfEachSurfaceIsExterior(allSurfaces, allColors);
		scv->m_zRot+=45.0;
	}

	scv->tp_projection= PROJECTION_PERSPECTIVE;
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
}

void ConversionProcessor::makeVboObjects(std::vector<gaia3d::TrianglePolyhedron*>& meshes, bool bBind)
{
	
	size_t meshCount = meshes.size();
	size_t surfaceCount, triangleCount;
	gaia3d::Vbo* vbo;
	size_t vertexInsertedCount;
	gaia3d::Vertex** vertices;
	bool bExist;
	size_t existingVertexIndex;
	std::vector<gaia3d::Triangle*> sortedTriangles;
	unsigned char sizeLevels = TriangleSizeLevels;
	double sizeThresholds[TriangleSizeLevels] = TriangleSizeThresholds;
	unsigned int eachSizeStartingIndices[TriangleSizeLevels];
	for(size_t i = 0; i < meshCount; i++)
	{
		if(meshes[i]->getReferenceInfo().model != NULL)
			continue;

		vbo = new gaia3d::Vbo;
		meshes[i]->getVbos().push_back(vbo);

		surfaceCount = meshes[i]->getSurfaces().size();
		for(size_t j = 0;j < surfaceCount; j++)
		{
			sortedTriangles.clear();
			sortTrianglesBySize(meshes[i]->getSurfaces()[j]->getTriangles(), sizeLevels, sizeThresholds, sortedTriangles, eachSizeStartingIndices);

			triangleCount = sortedTriangles.size();
			for(size_t k = 0; k < triangleCount; k++)
			{
				if(vbo->vertices.size() >= VboVertexMaxCount)
				{
					// vbo의 maximum vertex개수가 다 차면
					// 새로 만들어주어야 한다.
					// 새로 만들기 전에 size별로 sorting 해둔 index marker를 이용해
					// 기존의 vbo에 size별 index marker를 넣어야 한다.
					for(size_t m = 0; m < TriangleSizeLevels; m++)
					{
						if(eachSizeStartingIndices[m]*3 > vbo->indices.size())
						{
							for(size_t n=m; n < TriangleSizeLevels; n++)
							{
								vbo->indexMarker[n] = (unsigned int)vbo->indices.size();
								eachSizeStartingIndices[n] -= (unsigned int)vbo->indices.size() / 3;
							}
						}
						else
						{
							vbo->indexMarker[m] = eachSizeStartingIndices[m]*3;
							eachSizeStartingIndices[m] = 0;
						}
					}

					vbo = new gaia3d::Vbo;
					meshes[i]->getVbos().push_back(vbo);
				}

				vertices = sortedTriangles[k]->getVertices();
				for(size_t m = 0; m < 3; m++)
				{
					bExist = false;
					vertexInsertedCount = vbo->vertices.size();
					for(size_t n = 0; n < vertexInsertedCount; n++)
					{
						if(vbo->vertices[n] == vertices[m])
						{
							bExist = true;
							existingVertexIndex = n;
							break;
						}
					}

					if(bExist)
					{
						vbo->indices.push_back((unsigned short)existingVertexIndex);
					}
					else
					{
						vbo->indices.push_back((unsigned short)vbo->vertices.size());
						vbo->vertices.push_back(vertices[m]);
					}
				}
			}

			// 마지막 vbo에 size별 index marker를 넣어줘야 한다.
			for(size_t m = 0; m < TriangleSizeLevels; m++)
			{
				if(eachSizeStartingIndices[m]*3 > vbo->indices.size())
				{
					for(size_t n=m; n < TriangleSizeLevels; n++)
					{
						vbo->indexMarker[n] = (unsigned int)vbo->indices.size();
						eachSizeStartingIndices[n] -= (unsigned int)vbo->indices.size() / 3;
					}
				}
				else
				{
					vbo->indexMarker[m] = eachSizeStartingIndices[m]*3;
					eachSizeStartingIndices[m] = 0;
				}
			}
		}

		if(bBind)
		{
			// TODO(khj 20170323) : NYI 나중에 off-screen rendering 때문에 gpu를 사용해야 한다면 여기서 추가 구현 해야 한다.
		}
	}
}


void ConversionProcessor::determineModelAndReference(std::vector<gaia3d::TrianglePolyhedron*>& meshes)
{
	size_t meshCount = meshes.size();
	gaia3d::TrianglePolyhedron *mesh, *model, *candidate;
	gaia3d::Matrix4 matrix;
	size_t modelId = 0;
	for(size_t i = 0; i < meshCount; i++)
	{
		mesh = meshes[i];
		// check if this mesh can be a model
		if(mesh->getReferenceInfo().model != NULL)
			continue;

		// at this time, this mesh is a model
		model = mesh;
		model->getReferenceInfo().modelIndex = modelId;
		
		// using this model, find and mark all references
		for(size_t j = i+1; j < meshCount; j++)
		{
			mesh = meshes[j];
			// check if this mesh is refering other model
			if(mesh->getReferenceInfo().model != NULL)
				continue;

			// at this time, this mesh is a candidate which can refer this model
			candidate = mesh;

			if(gaia3d::GeometryUtility::areTwoCongruentWithEachOther(candidate, model, &matrix, 0.0, gaia3d::GeometryUtility::POLYHEDRON))
			{
				candidate->getReferenceInfo().model = model;
				candidate->getReferenceInfo().mat.set(&matrix);
				candidate->getReferenceInfo().modelIndex = modelId;
			}
		}

		modelId++;
	}
}

void ConversionProcessor::assignReferencesIntoExteriorAndInterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
																std::vector<gaia3d::TrianglePolyhedron*>& interiors,
																std::vector<gaia3d::TrianglePolyhedron*>& exteriors)
{
	size_t meshCount = meshes.size();
	gaia3d::TrianglePolyhedron* mesh;
	for(size_t i = 0; i < meshCount; i++)
	{
		mesh = meshes[i];

		if(mesh->doesHaveAnyExteriorSurface())
			exteriors.push_back(mesh);
		else
			interiors.push_back(mesh);
	}
}

void ConversionProcessor::assignReferencesIntoEachSpatialOctrees(gaia3d::SpatialOctreeBox& spatialOctree,
																std::vector<gaia3d::TrianglePolyhedron*>& meshes,
																gaia3d::BoundingBox& bbox,
																bool bFixedDepth,
																double leafBoxSize,
																bool bRefOnOnlyOneLeaf)
{
	// prepare spatial indexing
	if(bbox.isInitialized)
	{
		double maxLength = bbox.getMaxLength();
		spatialOctree.setSize(bbox.minX, bbox.minY, bbox.minZ, bbox.minX + maxLength, bbox.minY + maxLength, bbox.minZ + maxLength);
		spatialOctree.meshes.insert(spatialOctree.meshes.end(), meshes.begin(), meshes.end());

		if(bFixedDepth)
		{
			spatialOctree.makeTree(SpatialIndexingDepth);
			
			spatialOctree.distributeMeshesIntoEachChildren(bRefOnOnlyOneLeaf);
		}
		else
		{
			spatialOctree.makeTreeOfUnfixedDepth(leafBoxSize, bRefOnOnlyOneLeaf);
		}

		spatialOctree.setOctreeId();
	}
}

void ConversionProcessor::makeOcclusionInformation(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
													gaia3d::VisionOctreeBox& interiorOcclusionOctree,
													gaia3d::VisionOctreeBox& exteriorOcclusionOctree,
													gaia3d::BoundingBox& interiorBbox,
													gaia3d::BoundingBox& exteriorBbox)
{
	// allocate a unique color to each mesh
	std::vector<gaia3d::ColorU4> allColors;
	size_t meshCount = meshes.size();
	for(size_t i = 0; i < meshCount; i++)
		allColors.push_back(MakeColorU4(GetRedValue(i), GetGreenValue(i), GetBlueValue(i)));

	// 2 occlusion culling processes should be performed.
	// one for interior objects and the other for exterior objects.

	// before occlusion culling processes, make display list of all meshes with index colors
	glDisable( GL_TEXTURE_2D );
	makeDisplayListOfMeshes(meshes, allColors);
		
	// occlusion culling for interior
	if(interiorBbox.isInitialized)
	{
		interiorOcclusionOctree.setSize(interiorBbox.minX, interiorBbox.minY, interiorBbox.minZ, interiorBbox.maxX, interiorBbox.maxY, interiorBbox.maxZ);
		interiorOcclusionOctree.makeTree(InteriorOcclusionCullingDepth);
		makeVisibilityIndices(interiorOcclusionOctree, meshes, InteriorOcclusionCullingStep, InteriorOcclusionCullingStep, InteriorOcclusionCullingStep, NULL);
	}

	// occlusion culling for exterior
	if(exteriorBbox.isInitialized)
	{
		double bufferLength = exteriorBbox.getMaxLength() * 0.5;
		exteriorOcclusionOctree.setSize(exteriorBbox.minX - bufferLength, exteriorBbox.minY - bufferLength, exteriorBbox.minZ - bufferLength,
										exteriorBbox.maxX + bufferLength, exteriorBbox.maxY + bufferLength, exteriorBbox.maxZ + bufferLength);
		exteriorOcclusionOctree.makeTree(ExteriorOcclusionCullingDepth);
		makeVisibilityIndices(exteriorOcclusionOctree, meshes, ExteriorOcclusionCullingStep, ExteriorOcclusionCullingStep, ExteriorOcclusionCullingStep, &interiorOcclusionOctree);
	}

	glEnable( GL_TEXTURE_2D );
}

void ConversionProcessor::applyOcclusionInformationOnSpatialOctree(gaia3d::SpatialOctreeBox& spatialOctree,
																	gaia3d::VisionOctreeBox& interiorOcclusionOctree,
																	gaia3d::VisionOctreeBox& exteriorOcclusionOctree)
{
	gaia3d::VisionOctreeBox* interiorOcclusionInfoOfEachGroup, *exteriorOcclusionInfoOfEachGroup;

	std::vector<gaia3d::OctreeBox*> leafBoxes;
	spatialOctree.getAllLeafBoxes(leafBoxes);
	size_t leafBoxCount = leafBoxes.size();
	for(size_t i = 0; i < leafBoxCount; i++)
	{
		interiorOcclusionInfoOfEachGroup = ((gaia3d::SpatialOctreeBox*)leafBoxes[i])->interiorOcclusionInfo;
		interiorOcclusionInfoOfEachGroup->copyDimensionsFromOtherOctreeBox(interiorOcclusionOctree);
		extractMatchedReferencesFromOcclusionInfo(interiorOcclusionInfoOfEachGroup, interiorOcclusionOctree, leafBoxes[i]->meshes);

		exteriorOcclusionInfoOfEachGroup = ((gaia3d::SpatialOctreeBox*)leafBoxes[i])->exteriorOcclusionInfo;
		exteriorOcclusionInfoOfEachGroup->copyDimensionsFromOtherOctreeBox(exteriorOcclusionOctree);
		extractMatchedReferencesFromOcclusionInfo(exteriorOcclusionInfoOfEachGroup, exteriorOcclusionOctree, leafBoxes[i]->meshes);
	}
}

void ConversionProcessor::makeLegoStructure(gaia3d::SpatialOctreeBox& spatialOctree, double minLegoSize, std::map<size_t, gaia3d::TrianglePolyhedron*>& result, bool bMakeTexutreCoordinate)
{
	std::vector<gaia3d::OctreeBox*> leafBoxes;
	spatialOctree.getAllLeafBoxes(leafBoxes, true);

	size_t leafBoxCount = leafBoxes.size();
	gaia3d::BoundingBox bbox;
	gaia3d::TrianglePolyhedron* lego;
	for(size_t i = 0; i < leafBoxCount; i++)
	{
		lego = NULL;
		bbox.isInitialized = false;
		bbox.addPoint(leafBoxes[i]->minX, leafBoxes[i]->minY, leafBoxes[i]->minZ);
		bbox.addPoint(leafBoxes[i]->maxX, leafBoxes[i]->maxY, leafBoxes[i]->maxZ);
		lego = makeLegoStructure(leafBoxes[i]->meshes, bbox, minLegoSize, bMakeTexutreCoordinate);
		if(lego != NULL)
			result.insert(std::map<size_t, gaia3d::TrianglePolyhedron*>::value_type(((gaia3d::SpatialOctreeBox*)leafBoxes[i])->octreeId, lego));
	}
}

void ConversionProcessor::makeLegoTexture(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::wstring, std::wstring>& textureInfo)
{
	// prepare shaders for texture drawing
	unsigned int shaderProgram = makeShaders();
	if (shaderProgram == 0)
		return;

	// load and bind textures
	std::map<std::wstring, unsigned int> bindingResult;
	loadAndBindTextures(textureInfo, bindingResult);

	// let's draw full meshes and extract lego textures
	extractLegoTextures(meshes, bindingResult, shaderProgram);

	// delete shaders
	deleteShaders(shaderProgram);

	// unbind textures
	unbindTextures(bindingResult);
}

void ConversionProcessor::normalizeTextures(std::map<std::wstring, std::wstring>& textureInfo)
{
	Gdiplus::GdiplusStartupInput gdiplusstartupinput;
	unsigned long long gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusstartupinput, NULL);

	std::map<std::wstring, std::wstring>::iterator itr = textureInfo.begin();
	std::wstring fileExt;
	std::wstring fileName, fullPath;
	for (; itr != textureInfo.end(); itr++)
	{
		fileName = itr->first;
		fullPath = itr->second;

		std::wstring::size_type dotPosition = fileName.rfind(L".");
		if (dotPosition == std::wstring::npos)
			continue;

		std::wstring fileExt = fileName.substr(dotPosition + 1, fileName.length() - dotPosition - 1);
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

		if (fileExt.compare(L"jpg") != 0 && fileExt.compare(L"jpeg") != 0 && fileExt.compare(L"jpe") != 0 &&
			fileExt.compare(L"png") != 0 && fileExt.compare(L"gif") != 0 && fileExt.compare(L"bmp") != 0 &&
			fileExt.compare(L"tif") != 0 && fileExt.compare(L"tiff") != 0)
			continue;

		// texture file full path를 이용해 bitmap을 만든다.
		Gdiplus::Bitmap bmp(fullPath.c_str());

		unsigned int bmpWidth =  bmp.GetWidth();
		unsigned int bmpHeight = bmp.GetHeight();

		if (bmpWidth == 0 || bmpHeight == 0)
			continue;

		unsigned int bmpWidthResized, bmpHeightResized;

		// width가 2^n 형태가 아니면 2^n 형태로 resize 해야 한다.
		if ((bmpWidth & (bmpWidth - 1)) == 0)
			bmpWidthResized = bmpWidth;
		else
		{
			unsigned int prevPower = 1;
			while(bmpWidth >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			bmpWidthResized = ((bmpWidth - (1 << prevPower)) > ((1 << nextPower) - bmpWidth)) ? 1 << nextPower : 1 << prevPower;
		}

		// height가 2^n 형태가 아니면 2^n 형태로 resize 해야 한다.
		if ((bmpHeight & (bmpHeight - 1)) == 0)
			bmpHeightResized = bmpHeight;
		else
		{
			unsigned int prevPower = 1;
			while (bmpHeight >> prevPower != 1)
				prevPower++;

			unsigned int nextPower = prevPower + 1;
			bmpHeightResized = ((bmpHeight - (1 << prevPower)) > ((1 << nextPower) - bmpHeight)) ? 1 << nextPower : 1 << prevPower;
		}

		// resize 된 width / height를 이용해 resized bitmap을 만든다.
		Gdiplus::PixelFormat sourcePixelFormat = bmp.GetPixelFormat();
		Gdiplus::Bitmap resizedBmp(bmpWidthResized, bmpHeightResized, sourcePixelFormat);

		Gdiplus::Graphics graphics(&resizedBmp);
		graphics.DrawImage(&bmp, 0, 0, bmpWidthResized, bmpHeightResized);

		// resized bitmap으로부터 bitmap array를 얻는다.
		Gdiplus::Rect rect(0, 0, bmpWidthResized, bmpHeightResized);
		Gdiplus::BitmapData bmpData;
		resizedBmp.LockBits(&rect, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);

		unsigned char* pSource = (unsigned char*)bmpData.Scan0;
		if (pSource == NULL)
		{
			resizedBmp.UnlockBits(&bmpData);
			continue;
		}

		unsigned char* pDest = new unsigned char[rect.Width * rect.Height * 4];
		memset(pDest, 0x00, sizeof(unsigned char) * rect.Width * rect.Height * 4);
		memcpy(pDest, pSource, sizeof(unsigned char) * rect.Width * rect.Height * 4);

		resizedBmp.UnlockBits(&bmpData);

		// 최종 결과물을 container에 넣는다.
		resizedTextures[itr->first] = pDest;
		allTextureWidths[itr->first] = bmpWidthResized;
		allTextureHeights[itr->first] = bmpHeightResized;
	}

	Gdiplus::GdiplusShutdown(gdiplusToken);
}

void ConversionProcessor::calculateBoundingBox(gaia3d::TrianglePolyhedron* mesh)
{
	if(mesh->getBoundingBox().isInitialized)
		return;

	std::vector<gaia3d::Vertex*>& vertices = mesh->getVertices();
	size_t vertexCount = vertices.size();
	for(size_t j = 0; j < vertexCount; j++)
	{
		
		mesh->getBoundingBox().addPoint(vertices[j]->position.x, vertices[j]->position.y, vertices[j]->position.z);
		//bbox.addPoint(vertices[j]->position.x, vertices[j]->position.y, vertices[j]->position.z);
	}
}

void ConversionProcessor::defaultSpaceSetupForVisualization(int width, int height)
{
	scv->m_width = width;
	scv->m_height = height;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//wglMakeCurrent(scv->m_myhDC, scv->m_hRC);

	glViewport (0, 0, (GLsizei) scv->m_width, (GLsizei) scv->m_height); // Set the viewport 

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if(scv->tp_projection==PROJECTION_PERSPECTIVE)
	{
		gluPerspective(scv->m_perspective_angle, (GLfloat) scv->m_width /(GLfloat) scv->m_height ,scv->m_perspective_near, scv->m_perspective_far);
	}
	else if(scv->tp_projection==PROJECTION_ORTHO)
	{
		if(scv->m_width <= scv->m_height)
			glOrtho (-(scv->m_nRange), scv->m_nRange,
					scv->m_nRange*scv->m_height/scv->m_width, scv->m_nRange*scv->m_height/scv->m_width,
					-(scv->m_nRange)*5000.0f, (scv->m_nRange)*5000.0f);
		else
			glOrtho (-(scv->m_nRange)*scv->m_width/scv->m_height, scv->m_nRange*scv->m_width/scv->m_height,
					-(scv->m_nRange), scv->m_nRange,
					-(scv->m_nRange)*5000.0f, (scv->m_nRange)*5000.0f);
	}
	
    glMatrixMode(GL_MODELVIEW);
}

void ConversionProcessor::setupPerspectiveViewSetting(gaia3d::BoundingBox& bbox)
{
	gaia3d::Point3D centerPoint;
	bbox.getCenterPoint(centerPoint.x, centerPoint.y, centerPoint.z);

	// Ara calculem la posicio de la camara tenint en compte xRot, yRot i zRot.***
	gaia3d::Point3D rotation_vector, new_cam_pos;
	double max_length_bc = bbox.getMaxLength();

	if(scv->tp_projection == PROJECTION_PERSPECTIVE)
	{
		double dist_aux = (max_length_bc/2.0)/tan(scv->m_perspective_angle/2.0*scv->MPI_Div_180);// provisional.***

		gaia3d::Matrix4 mat_xrot, mat_yrot, mat_zrot;
		mat_xrot.rotation(scv->m_xRot*scv->MPI_Div_180, 1.0, 0.0, 0.0);
		mat_yrot.rotation(scv->m_yRot*scv->MPI_Div_180, 0.0, 1.0, 0.0);
		mat_zrot.rotation(scv->m_zRot*scv->MPI_Div_180, 0.0, 0.0, 1.0);
		scv->mat_rot= (mat_xrot*mat_yrot)*mat_zrot;

		scv->m_viewing_direction = scv->mat_rot*scv->m_viewing_direction;

		rotation_vector.set(0.0, 0.0, -1.0);
		rotation_vector = scv->mat_rot*rotation_vector;

		new_cam_pos.set(centerPoint.x-rotation_vector.x*dist_aux, 
			centerPoint.y-rotation_vector.y*dist_aux, 
			centerPoint.z-rotation_vector.z*dist_aux);

		scv->m_xPos = -new_cam_pos.x;
		scv->m_yPos = -new_cam_pos.y;
		scv->m_zPos = -new_cam_pos.z;
	}
	else if(scv->tp_projection== PROJECTION_ORTHO)
	{
		scv->m_nRange = (float)(max_length_bc*0.9f)/2.0f;
		scv->m_xPos = -centerPoint.x;
		scv->m_yPos = -centerPoint.y;
		scv->m_zPos = -centerPoint.z;
		defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
	}
}

void ConversionProcessor::checkIfEachSurfaceIsExterior(std::vector<gaia3d::Surface*>& surfaces, std::vector<gaia3d::ColorU4>& colors)
{

	size_t last_idx_triSurface = 4294967295;// Max value for an unsigned int.***

	int data_size = scv->m_width * scv->m_height;
	GLuint *data = new GLuint[data_size];
	memset(data, 0x00, data_size*sizeof(GLuint));

	glDisable(GL_LIGHTING);
	this->drawSurfacesWithIndexColor(surfaces, colors);
	//SwapBuffers(scv->m_myhDC);
	//this->drawSurfacesWithIndexColor(surfaces, colors);

	glReadPixels(0,0, scv->m_width, scv->m_height, GL_RGBA, GL_UNSIGNED_BYTE, data);

	int pixels_count = data_size;
	size_t triSurfaces_count = surfaces.size();

	// Make a colors counter. if an objects is visible by a little count of pixels, then, this object is not visible.***
	std::map<gaia3d::Surface*, int> map_triSurf_pixelsCount;
	std::map<gaia3d::Surface*, int>::iterator it;

	for(int i=0; i<pixels_count; i++)
	{
		unsigned int idx_triSurf = (data[i] & 0x00ffffff);// Original by Hak.***
		if(idx_triSurf < triSurfaces_count)
		{
			if(idx_triSurf != last_idx_triSurface && idx_triSurf != 0x00ffffff)
			{
				last_idx_triSurface = idx_triSurf;

				gaia3d::Surface *triSurf = surfaces[idx_triSurf];

				// now, search the triSurf inside of the map.***
				it = map_triSurf_pixelsCount.find(triSurf);
				if(it != map_triSurf_pixelsCount.end())
				{
					// found.***
					int current_value = it->second;
					map_triSurf_pixelsCount[triSurf] = current_value+1;
				}
				else
				{
					// no found.***
					map_triSurf_pixelsCount[triSurf] = 1;
				}
			}
		}
	}

	// Now, for each triSurf in the map, set as visible if his value is bigger than minValue.***
	for(std::map<gaia3d::Surface*, int>::iterator itera = map_triSurf_pixelsCount.begin(); itera != map_triSurf_pixelsCount.end(); itera++)
	{
		gaia3d::Surface *triSurf = itera->first;
		int pixels_counted = itera->second;

		if(pixels_counted > ExteriorDetectionThreshold)
		{
			triSurf->setIsExterior(true);

			size_t surfaceCount = surfaces.size();
			for(size_t i = 0; i < surfaceCount; i++)
			{
				if(triSurf == surfaces[i])
				{
					colors[i] = MakeColorU4(255, 255, 255);
					break;
				}
			}
		}
	}

	delete []data;
	glEnable(GL_LIGHTING);
}

void ConversionProcessor::drawSurfacesWithIndexColor(std::vector<gaia3d::Surface*>& surfaces, std::vector<gaia3d::ColorU4>& colors)
{
	// Clear the screen and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Reset the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();

			glRotatef((float)scv->m_xRot_aditional, 1.0f, 0.0f, 0.0f); 
			glRotatef((float)scv->m_yRot_aditional, 0.0f, 1.0f, 0.0f); 
			glRotatef((float)scv->m_zRot_aditional, 0.0f, 0.0f, 1.0f); 

			glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
			glRotatef((float)scv->m_yRot, 0.0f, 1.0f, 0.0f);
			glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);

			// Apliquem les traslacions de l'escena.***
			glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);

	//**********************************************************
	size_t triSurfaces_count = surfaces.size();
	GLubyte ubyte_r, ubyte_g, ubyte_b;

	ubyte_r = 0;
	ubyte_g = 0;
	ubyte_b = 0;

	glBegin(GL_TRIANGLES);
	for(size_t i=0; i<triSurfaces_count; i++)
	{
		gaia3d::Surface *triSurf = surfaces[i];

		ubyte_r = GetRedValue(colors[i]);
		ubyte_g = GetGreenValue(colors[i]);;
		ubyte_b = GetBlueValue(colors[i]);;

		glColor3ub(ubyte_r, ubyte_g, ubyte_b);

		size_t triangles_count = triSurf->getTriangles().size();
		for(size_t j=0; j<triangles_count; j++)
		{
			gaia3d::Triangle *tri = triSurf->getTriangles()[j];

			glVertex3f((float)tri->getVertices()[0]->position.x, (float)tri->getVertices()[0]->position.y, (float)tri->getVertices()[0]->position.z);
			glVertex3f((float)tri->getVertices()[1]->position.x, (float)tri->getVertices()[1]->position.y, (float)tri->getVertices()[1]->position.z);
			glVertex3f((float)tri->getVertices()[2]->position.x, (float)tri->getVertices()[2]->position.y, (float)tri->getVertices()[2]->position.z);
		}
	}
	glEnd();
	//-----------------------------------------------------------------------------------
	
 	glPopMatrix();
	glFlush();
}

void ConversionProcessor::makeVisibilityIndices(gaia3d::VisionOctreeBox& octree, std::vector<gaia3d::TrianglePolyhedron*>& meshes,
												float scanStepX, float scanStepY, float scanStepZ,
												gaia3d::VisionOctreeBox* excludedBox)
{
	// Save the current state to return at the final of the process.***
	double currentXPos = scv->m_xPos;
	double currentYPos = scv->m_yPos;
	double currentZPos = scv->m_zPos;

	double currentXRot = scv->m_xRot;
	double currentYRot = scv->m_yRot;
	double currentZRot = scv->m_zRot;

	
	// temporary setup for space of visualization for offscreen rendering for occlusion culling
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	int screenSize = OcclusionCullingScreenSize; // edge length of screen for offscreen rendering for occlusion culling. we set the screen squared
	//int pixels_count = screenSize * screenSize;
	scv->m_perspective_angle = 90.0;

	glViewport (0, 0, (GLsizei) screenSize, (GLsizei) screenSize); // Set the viewport 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(scv->m_perspective_angle, (GLfloat)1.0f , scv->m_perspective_near, scv->m_perspective_far);
    glMatrixMode(GL_MODELVIEW);

	glReadBuffer(GL_BACK);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	// temporary setup - end

	std::vector<gaia3d::OctreeBox*> leafBoxes;
	octree.getAllLeafBoxes(leafBoxes);

	size_t meshCount = meshes.size();
	size_t leafBoxCount = leafBoxes.size();
	gaia3d::VisionOctreeBox* leafBox;
	std::vector<gaia3d::Point3D> scanningPoints;
	size_t pointCount;
	gaia3d::Point3D targetPoint;
	std::map<size_t, size_t> indices;
	for(size_t i = 0; i < leafBoxCount; i++)
	{
		leafBox = (gaia3d::VisionOctreeBox*)leafBoxes[i];
		leafBox->getInternalDivisionPoints(scanningPoints,  scanStepX,  scanStepY,  scanStepZ);
		pointCount = scanningPoints.size();
		for(size_t j = 0; j < pointCount; j++)
		{
			targetPoint = scanningPoints[j];

			if(excludedBox != NULL)
			{
				if( gaia3d::GeometryUtility::isInsideBox(targetPoint.x, targetPoint.y, targetPoint.z,
														excludedBox->minX, excludedBox->minY, excludedBox->minZ,
														excludedBox->maxX, excludedBox->maxY, excludedBox->maxZ) )
					continue;
			}

			scv->m_xPos = -targetPoint.x;
			scv->m_yPos = -targetPoint.y;
			scv->m_zPos = -targetPoint.z;

			// FRONT.***
			scv->m_xRot= 0.0; scv->m_yRot= 0.0; scv->m_zRot= 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// TOP.***
			scv->m_xRot= -90.0; scv->m_yRot= 0.0; scv->m_zRot= 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// LEFT.***
			scv->m_xRot= 0.0; scv->m_yRot= -90.0; scv->m_zRot= 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// RIGHT.***
			scv->m_xRot= 0.0; scv->m_yRot= 90.0; scv->m_zRot= 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// REAR.***
			scv->m_xRot= 180.0; scv->m_yRot= 0.0; scv->m_zRot= 0.0;
			drawAndDetectVisibleColorIndices(indices);

			// BOTTOM.***
			scv->m_xRot= 90.0; scv->m_yRot= 0.0; scv->m_zRot= 0.0;
			drawAndDetectVisibleColorIndices(indices);
		}

		leafBox->meshes.clear();

		std::map<size_t, size_t>::iterator iter;
		for(iter = indices.begin(); iter != indices.end(); iter++)
		{
			if(iter->first >= meshCount)
				continue;

			leafBox->meshes.push_back(meshes[iter->first]);
		}

		scanningPoints.clear();
		indices.clear();
	}


	// Finally, return the normal perspective view.*******************************************************************
	scv->ClearColor[0] =1.0;
	scv->ClearColor[1] =1.0;
	scv->ClearColor[2] =1.0;

	scv->m_xPos = currentXPos;
	scv->m_yPos = currentYPos;
	scv->m_zPos = currentZPos;

	scv->m_xRot = currentXRot;
	scv->m_yRot = currentYRot;
	scv->m_zRot = currentZRot;

	scv->m_xRot_aditional = 0.0;
	scv->m_yRot_aditional = 0.0;
	scv->m_zRot_aditional = 0.0;

	scv->m_perspective_angle = 90.0;
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
	setupPerspectiveViewSetting(fullBbox);

	glReadBuffer(GL_BACK);
}

void ConversionProcessor::drawAndDetectVisibleColorIndices(std::map<size_t, size_t>& container)
{
	int pixelCount = OcclusionCullingScreenSize * OcclusionCullingScreenSize;
	GLubyte* buffer = new GLubyte[pixelCount * 3];
	memset(buffer, 0xff, sizeof(GLubyte)*pixelCount*3);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Reset the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();

	glRotatef((float)scv->m_xRot_aditional, 1.0f, 0.0f, 0.0f); 
	glRotatef((float)scv->m_yRot_aditional, 0.0f, 1.0f, 0.0f); 
	glRotatef((float)scv->m_zRot_aditional, 0.0f, 0.0f, 1.0f); 

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_yRot, 0.0f, 1.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);

	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);

	//**********************************************************
			//if(this->m_one_displayList_id > 0)
	glCallList(DisplayListIdForOcclusionCulling);
	
 	glPopMatrix();
	glFlush();

	glReadPixels(0,0, OcclusionCullingScreenSize, OcclusionCullingScreenSize, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	
	for(int j=0; j<pixelCount; j++)
	{
		GLubyte pixel_data_R = buffer[j*3];
		GLubyte pixel_data_G = buffer[j*3 + 1];
		GLubyte pixel_data_B = buffer[j*3 + 2];

		if(pixel_data_R == 255 && pixel_data_G == 255 && pixel_data_B == 255)
			continue;
		// Max color = (254, 254, 254).***
		// i= 64516*R + 254*G + B.***
		unsigned long index = MakeColorU4(pixel_data_R, pixel_data_G, pixel_data_B);
		container[index] = index;
	}
	
	delete[] buffer;
}

void ConversionProcessor::makeDisplayListOfMeshes(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::vector<gaia3d::ColorU4>& colors)
{
	glNewList(DisplayListIdForOcclusionCulling, GL_COMPILE);

	size_t meshCount = meshes.size();
	gaia3d::TrianglePolyhedron* mesh;
	for(size_t i=0; i < meshCount; i++)
	{
		mesh = meshes[i];

		glColor3ub(GetRedValue(colors[i]), GetGreenValue(colors[i]), GetBlueValue(colors[i]));
		renderMesh(mesh, false, false);
	}
	glEndList();
}

void ConversionProcessor::renderMesh(gaia3d::TrianglePolyhedron* mesh, bool bNormal, bool bTextureCoordinate)
{
	char renderingMode = (bNormal ? 2 : 0) + (bTextureCoordinate ? 1 : 0);
	
	size_t surfaceCount = mesh->getSurfaces().size();
	gaia3d::Surface* surface;
	size_t triangleCount;
	gaia3d::Triangle* triangle;
	gaia3d::Vertex** vertices;

	switch(renderingMode)
	{
	case 0: // non normal, non texture coordinate
		{
			for(size_t i = 0; i < surfaceCount; i++)
			{
				surface = mesh->getSurfaces()[i];
				triangleCount = surface->getTriangles().size();

				glBegin(GL_TRIANGLES);
				for(size_t j = 0; j < triangleCount; j++)
				{
					triangle = surface->getTriangles()[j];
					vertices = triangle->getVertices();

					// 1rst vertex.***************************************************************
					glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

					// 2nd vertex.***************************************************************
					glVertex3f((float)vertices[1]->position.x,(float) vertices[1]->position.y, (float)vertices[1]->position.z);

					// 3rd vertex.***************************************************************
					glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
				}
				glEnd();
			}
		}
		break;
	case 1: // only texture coordiate
		{
			for(size_t i = 0; i < surfaceCount; i++)
			{
				surface = mesh->getSurfaces()[i];
				triangleCount = surface->getTriangles().size();

				glBegin(GL_TRIANGLES);
				for(size_t j = 0; j < triangleCount; j++)
				{
					triangle = surface->getTriangles()[j];
					vertices = triangle->getVertices();

					// 1rst vertex.***************************************************************
					glTexCoord2d(vertices[0]->textureCoordinate[0], vertices[0]->textureCoordinate[1]);
					glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

					// 2nd vertex.***************************************************************
					glTexCoord2d(vertices[1]->textureCoordinate[0], vertices[1]->textureCoordinate[1]);
					glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

					// 3rd vertex.***************************************************************
					glTexCoord2d(vertices[2]->textureCoordinate[0], vertices[2]->textureCoordinate[1]);
					glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
				}
				glEnd();
			}
		}
		break;
	case 2: // only normal
		{
			for(size_t i = 0; i < surfaceCount; i++)
			{
				surface = mesh->getSurfaces()[i];
				triangleCount = surface->getTriangles().size();

				glBegin(GL_TRIANGLES);
				for(size_t j = 0; j < triangleCount; j++)
				{
					triangle = surface->getTriangles()[j];
					vertices = triangle->getVertices();

					// 1rst vertex.***************************************************************
					glNormal3f((float)vertices[0]->normal.x, (float)vertices[0]->normal.y, (float)vertices[0]->normal.z);
					glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

					// 2nd vertex.***************************************************************
					glNormal3f((float)vertices[1]->normal.x, (float)vertices[1]->normal.y, (float)vertices[1]->normal.z);
					glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

					// 3rd vertex.***************************************************************
					glNormal3f((float)vertices[2]->normal.x, (float)vertices[2]->normal.y, (float)vertices[2]->normal.z);
					glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
				}
				glEnd();
			}
		}
		break;
	case 3: // both normal and texture coordinate
		{
			for(size_t i = 0; i < surfaceCount; i++)
			{
				surface = mesh->getSurfaces()[i];
				triangleCount = surface->getTriangles().size();

				glBegin(GL_TRIANGLES);
				for(size_t j = 0; j < triangleCount; j++)
				{
					triangle = surface->getTriangles()[j];
					vertices = triangle->getVertices();

					// 1rst vertex.***************************************************************
					glNormal3f((float)vertices[0]->normal.x, (float)vertices[0]->normal.y, (float)vertices[0]->normal.z);
					glTexCoord2d(vertices[0]->textureCoordinate[0], vertices[0]->textureCoordinate[1]);
					glVertex3f((float)vertices[0]->position.x, (float)vertices[0]->position.y, (float)vertices[0]->position.z);

					// 2nd vertex.***************************************************************
					glNormal3f((float)vertices[1]->normal.x, (float)vertices[1]->normal.y, (float)vertices[1]->normal.z);
					glTexCoord2d(vertices[1]->textureCoordinate[0], vertices[1]->textureCoordinate[1]);
					glVertex3f((float)vertices[1]->position.x, (float)vertices[1]->position.y, (float)vertices[1]->position.z);

					// 3rd vertex.***************************************************************
					glNormal3f((float)vertices[2]->normal.x, (float)vertices[2]->normal.y, (float)vertices[2]->normal.z);
					glTexCoord2d(vertices[2]->textureCoordinate[0], vertices[2]->textureCoordinate[1]);
					glVertex3f((float)vertices[2]->position.x, (float)vertices[2]->position.y, (float)vertices[2]->position.z);
				}
				glEnd();
			}
		}
		break;
	}
}

void ConversionProcessor::extractMatchedReferencesFromOcclusionInfo(gaia3d::VisionOctreeBox* receiver,
																	gaia3d::VisionOctreeBox& info,
																	std::vector<gaia3d::TrianglePolyhedron*>& meshesToBeCompared)
{
	// at this time, dimensions of receiver and info are same with each other.
	size_t childCount = receiver->children.size();
	if(childCount > 0)
	{
		for(size_t i = 0; i < childCount; i++)
		{
			extractMatchedReferencesFromOcclusionInfo( ((gaia3d::VisionOctreeBox*)receiver->children[i]),
														*((gaia3d::VisionOctreeBox*)info.children[i]),
														meshesToBeCompared);
		}
	}
	else
	{
		receiver->meshes.clear();
		size_t meshCountInInfoBox = info.meshes.size();
		size_t meshCountInToBecompared = meshesToBeCompared.size();
		bool existsInToBeCompared;
		for(size_t i = 0; i < meshCountInInfoBox; i++)
		{
			existsInToBeCompared = false;
			for(size_t j = 0; j < meshCountInToBecompared; j++)
			{
				if(info.meshes[i] == meshesToBeCompared[j])
				{
					existsInToBeCompared = true;
					break;
				}
			}

			if(existsInToBeCompared)
				receiver->meshes.push_back(info.meshes[i]);
		}
	}
}

gaia3d::TrianglePolyhedron* ConversionProcessor::makeLegoStructure(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox, double minLegoSize, bool bMakeTexutreCoordinate)
{
	// octree for making lego blocks
	gaia3d::SpatialOctreeBox legoOctree(NULL);
	double maxLength = bbox.getMaxLength();
	legoOctree.setSize(bbox.minX, bbox.minY, bbox.minZ, bbox.minX + maxLength, bbox.minY + maxLength, bbox.minZ + maxLength);
	legoOctree.meshes.insert(legoOctree.meshes.end(), meshes.begin(), meshes.end());
	legoOctree.makeTreeOfUnfixedDepth(minLegoSize, false);

	// change leaf octrees to lego blocks and insert them into quadtree
	std::vector<gaia3d::OctreeBox*> leafOctrees;
	legoOctree.getAllLeafBoxes(leafOctrees, true);
	gaia3d::Quadtree quadtree(NULL);
	quadtree.setSize(legoOctree.minX, legoOctree.minY, legoOctree.maxX, legoOctree.maxY);
	gaia3d::LegoBlock* legoBlock;
	size_t leafCount = leafOctrees.size();
	for(size_t i = 0; i < leafCount; i++)
	{
		legoBlock = new gaia3d::LegoBlock;
		legoBlock->setSize(leafOctrees[i]->minX, leafOctrees[i]->minY, leafOctrees[i]->minZ, leafOctrees[i]->maxX, leafOctrees[i]->maxY, leafOctrees[i]->maxZ);
		if (leafOctrees[i]->meshes[0]->getColorMode() == gaia3d::SingleColor)
			legoBlock->color = leafOctrees[i]->meshes[0]->getSingleColor();
		else
			legoBlock->color = leafOctrees[i]->meshes[0]->getVertices()[0]->color;

		quadtree.legos.push_back(legoBlock);
	}

	quadtree.makeTreeOfUnfixedDepth(minLegoSize, true);
	quadtree.setQuadtreeId();

	// merge lego blocks along z-axis using quadtree
	std::vector<gaia3d::LegoBlock*> joinedLegoBlocks;
	std::vector<gaia3d::Quadtree*> leafQuadtrees;
	quadtree.getAllLeafQuadtrees(leafQuadtrees);
	leafCount = leafQuadtrees.size();
	for(size_t i = 0; i < leafCount; i++)
	{
		gaia3d::GeometryUtility::mergeLegoBlocksAlongZAxis(leafQuadtrees[i]->legos, false);
		joinedLegoBlocks.insert(joinedLegoBlocks.begin(), leafQuadtrees[i]->legos.begin(), leafQuadtrees[i]->legos.end());
	}

	// merge lego blocks along y-axsis
	size_t joinedLegoCount = joinedLegoBlocks.size();
	for(size_t i = 0; i < joinedLegoCount; i++)
		gaia3d::GeometryUtility::mergeLegoBlocksAlongYAxis(joinedLegoBlocks, true);

	// merge lego blocks along x-axis
	joinedLegoCount = joinedLegoBlocks.size();
	for(size_t i = 0; i < joinedLegoCount; i++)
		gaia3d::GeometryUtility::mergeLegoBlocksAlongXAxis(joinedLegoBlocks, true);

	// convert lego blocks into a triangle polyhedron
	gaia3d::TrianglePolyhedron* trianglePolyhedron = new gaia3d::TrianglePolyhedron;
	trianglePolyhedron->setColorMode(gaia3d::ColorsOnVertices);
	trianglePolyhedron->setHasNormals(true);
	trianglePolyhedron->setHasTextureCoordinates(false);
	gaia3d::Surface* surface = new gaia3d::Surface;
	gaia3d::Triangle* triangle[12];
	joinedLegoCount = joinedLegoBlocks.size();
	double minS, maxS, minT, maxT, s, t;
	for(size_t i = 0; i < joinedLegoCount; i++)
	{
		gaia3d::LegoBlock* lego = joinedLegoBlocks[i];

		double pos[8][3] = {
			{lego->minX, lego->minY, lego->minZ},
			{lego->maxX, lego->minY, lego->minZ},
			{lego->maxX, lego->maxY, lego->minZ},
			{lego->minX, lego->maxY, lego->minZ},
			{lego->minX, lego->minY, lego->maxZ},
			{lego->maxX, lego->minY, lego->maxZ},
			{lego->maxX, lego->maxY, lego->maxZ},
			{lego->minX, lego->maxY, lego->maxZ}
		};

		// create 12 triangles
		for(size_t j = 0; j < 12; j++)
		{
			triangle[j] = new gaia3d::Triangle;
			triangle[j]->setVertices(new gaia3d::Vertex, new gaia3d::Vertex, new gaia3d::Vertex);
			for(size_t k = 0; k < 3; k++)
			{
				triangle[j]->getVertices()[k]->color = lego->color;
				trianglePolyhedron->getVertices().push_back(triangle[j]->getVertices()[k]);
			}
		}
		// bottom faces position setup
		triangle[0]->getVertices()[0]->position.set(pos[0][0], pos[0][1], pos[0][2]);
		triangle[0]->getVertices()[1]->position.set(pos[2][0], pos[2][1], pos[2][2]);
		triangle[0]->getVertices()[2]->position.set(pos[1][0], pos[1][1], pos[1][2]);
		triangle[1]->getVertices()[0]->position.set(pos[0][0], pos[0][1], pos[0][2]);
		triangle[1]->getVertices()[1]->position.set(pos[3][0], pos[3][1], pos[3][2]);
		triangle[1]->getVertices()[2]->position.set(pos[2][0], pos[2][1], pos[2][2]);
		triangle[0]->setNormal(0.0, 0.0, -1.0);
		triangle[1]->setNormal(0.0, 0.0, -1.0);
		// top faces position setup
		triangle[2]->getVertices()[0]->position.set(pos[4][0], pos[4][1], pos[4][2]);
		triangle[2]->getVertices()[1]->position.set(pos[5][0], pos[5][1], pos[5][2]);
		triangle[2]->getVertices()[2]->position.set(pos[6][0], pos[6][1], pos[6][2]);
		triangle[3]->getVertices()[0]->position.set(pos[4][0], pos[4][1], pos[4][2]);
		triangle[3]->getVertices()[1]->position.set(pos[6][0], pos[6][1], pos[6][2]);
		triangle[3]->getVertices()[2]->position.set(pos[7][0], pos[7][1], pos[7][2]);
		triangle[2]->setNormal(0.0, 0.0, 1.0);
		triangle[3]->setNormal(0.0, 0.0, 1.0);
		// front faces position setup
		triangle[4]->getVertices()[0]->position.set(pos[0][0], pos[0][1], pos[0][2]);
		triangle[4]->getVertices()[1]->position.set(pos[1][0], pos[1][1], pos[1][2]);
		triangle[4]->getVertices()[2]->position.set(pos[5][0], pos[5][1], pos[5][2]);
		triangle[5]->getVertices()[0]->position.set(pos[0][0], pos[0][1], pos[0][2]);
		triangle[5]->getVertices()[1]->position.set(pos[5][0], pos[5][1], pos[5][2]);
		triangle[5]->getVertices()[2]->position.set(pos[4][0], pos[4][1], pos[4][2]);
		triangle[4]->setNormal(0.0, -1.0, 0.0);
		triangle[5]->setNormal(0.0, -1.0, 0.0);
		// rear faces position setup
		triangle[6]->getVertices()[0]->position.set(pos[2][0], pos[2][1], pos[2][2]);
		triangle[6]->getVertices()[1]->position.set(pos[3][0], pos[3][1], pos[3][2]);
		triangle[6]->getVertices()[2]->position.set(pos[6][0], pos[6][1], pos[6][2]);
		triangle[7]->getVertices()[0]->position.set(pos[3][0], pos[3][1], pos[3][2]);
		triangle[7]->getVertices()[1]->position.set(pos[7][0], pos[7][1], pos[7][2]);
		triangle[7]->getVertices()[2]->position.set(pos[6][0], pos[6][1], pos[6][2]);
		triangle[6]->setNormal(0.0, 1.0, 0.0);
		triangle[7]->setNormal(0.0, 1.0, 0.0);
		// left faces position setup
		triangle[8]->getVertices()[0]->position.set(pos[0][0], pos[0][1], pos[0][2]);
		triangle[8]->getVertices()[1]->position.set(pos[7][0], pos[7][1], pos[7][2]);
		triangle[8]->getVertices()[2]->position.set(pos[3][0], pos[3][1], pos[3][2]);
		triangle[9]->getVertices()[0]->position.set(pos[0][0], pos[0][1], pos[0][2]);
		triangle[9]->getVertices()[1]->position.set(pos[4][0], pos[4][1], pos[4][2]);
		triangle[9]->getVertices()[2]->position.set(pos[7][0], pos[7][1], pos[7][2]);
		triangle[8]->setNormal(-1.0, 0.0, 0.0);
		triangle[9]->setNormal(-1.0, 0.0, 0.0);
		// right faces position setup
		triangle[10]->getVertices()[0]->position.set(pos[1][0], pos[1][1], pos[1][2]);
		triangle[10]->getVertices()[1]->position.set(pos[2][0], pos[2][1], pos[2][2]);
		triangle[10]->getVertices()[2]->position.set(pos[6][0], pos[6][1], pos[6][2]);
		triangle[11]->getVertices()[0]->position.set(pos[1][0], pos[1][1], pos[1][2]);
		triangle[11]->getVertices()[1]->position.set(pos[6][0], pos[6][1], pos[6][2]);
		triangle[11]->getVertices()[2]->position.set(pos[5][0], pos[5][1], pos[5][2]);
		triangle[10]->setNormal(1.0, 0.0, 0.0);
		triangle[11]->setNormal(1.0, 0.0, 0.0);

		if (bMakeTexutreCoordinate)
		{
			//---------------------------------------
			//            |            |            |
			//     TOP    |   FRONT    |    LEFT    |
			//            |            |            |
			//---------------------------------------
			//            |            |            |
			//    BOTTOM  |   REAR     |   RIGHT    |
			//            |            |            |
			//---------------------------------------
			//            |            |            |
			//            |            |            |
			//            |            |            |
			//---------------------------------------

			for (size_t j = 0; j < 12; j++)
			{
				switch (j/2)
				{
				case 0: // bottom;
				{
					minS = lego->minX, maxS = lego->maxX, minT = lego->minY, maxT = lego->maxY;
					for (size_t k = 0; k < 3; k++)
					{
						s = (triangle[j]->getVertices()[k]->position.x - minS) / (maxS - minS);
						t = (triangle[j]->getVertices()[k]->position.y - minT) / (maxT - minT);
						s = s / 3.0;
						t = t / 3.0 + 1.0 / 3.0;
						triangle[j]->getVertices()[k]->textureCoordinate[0] = s;
						triangle[j]->getVertices()[k]->textureCoordinate[1] = 1.0 - t;
					}
				}
				break;
				case 1: // top
				{
					minS = lego->minX, maxS = lego->maxX, minT = lego->minY, maxT = lego->maxY;
					for (size_t k = 0; k < 3; k++)
					{
						s = (triangle[j]->getVertices()[k]->position.x - minS) / (maxS - minS);
						t = (triangle[j]->getVertices()[k]->position.y - minT) / (maxT - minT);
						s = s / 3.0;
						t = t / 3.0 + 2.0 / 3.0;
						triangle[j]->getVertices()[k]->textureCoordinate[0] = s;
						triangle[j]->getVertices()[k]->textureCoordinate[1] = t;
					}
				}
				break;
				case 2: // front
				{
					minS = lego->minX, maxS = lego->maxX, minT = lego->minZ, maxT = lego->maxZ;
					for (size_t k = 0; k < 3; k++)
					{
						s = (triangle[j]->getVertices()[k]->position.x - minS) / (maxS - minS);
						t = (triangle[j]->getVertices()[k]->position.z - minT) / (maxT - minT);
						s = s / 3.0 + 1.0 / 3.0;
						t = t / 3.0 + 2.0 / 3.0;
						triangle[j]->getVertices()[k]->textureCoordinate[0] = s;
						triangle[j]->getVertices()[k]->textureCoordinate[1] = t;
					}
				}
				break;
				case 3: // rear
				{
					minS = lego->minX, maxS = lego->maxX, minT = lego->minZ, maxT = lego->maxZ;
					for (size_t k = 0; k < 3; k++)
					{
						s = (triangle[j]->getVertices()[k]->position.x - minS) / (maxS - minS);
						t = (triangle[j]->getVertices()[k]->position.z - minT) / (maxT - minT);
						s = s / 3.0 + 1.0 / 3.0;
						t = t / 3.0 + 1.0 / 3.0;
						triangle[j]->getVertices()[k]->textureCoordinate[0] = 1.0 - s;
						triangle[j]->getVertices()[k]->textureCoordinate[1] = t;
					}
				}
				break;
				case 4: // left
				{
					minS = lego->minY, maxS = lego->maxY, minT = lego->minZ, maxT = lego->maxZ;
					for (size_t k = 0; k < 3; k++)
					{
						s = (triangle[j]->getVertices()[k]->position.y - minS) / (maxS - minS);
						t = (triangle[j]->getVertices()[k]->position.z - minT) / (maxT - minT);
						s = 1.0 - s;
						s = s / 3.0 + 2.0 / 3.0;
						t = t / 3.0 + 2.0 / 3.0;
						triangle[j]->getVertices()[k]->textureCoordinate[0] = s;
						triangle[j]->getVertices()[k]->textureCoordinate[1] = t;
					}
				}
				break;
				case 5: // right
				{
					minS = lego->minY, maxS = lego->maxY, minT = lego->minZ, maxT = lego->maxZ;
					for (size_t k = 0; k < 3; k++)
					{
						s = (triangle[j]->getVertices()[k]->position.y - minS) / (maxS - minS);
						t = (triangle[j]->getVertices()[k]->position.z - minT) / (maxT - minT);
						s = s / 3.0 + 2.0 / 3.0;
						t = t / 3.0 + 1.0 / 3.0;
						triangle[j]->getVertices()[k]->textureCoordinate[0] = s;
						triangle[j]->getVertices()[k]->textureCoordinate[1] = t;
					}
				}
				break;
				}
			}
		}

		// calculate normal of plane & 3 vertices
		for(size_t j = 0; j < 12; j++)
		{
			triangle[j]->alignVertexNormalsToPlaneNormal();
			surface->getTriangles().push_back(triangle[j]);
		}
	}

	trianglePolyhedron->getSurfaces().push_back(surface);

	if (bMakeTexutreCoordinate)
		trianglePolyhedron->setHasTextureCoordinates(true);

	this->calculateBoundingBox(trianglePolyhedron);

	for(size_t i = 0; i < joinedLegoCount; i++)
		delete joinedLegoBlocks[i];

	return trianglePolyhedron;
}

void ConversionProcessor::sortTrianglesBySize(std::vector<gaia3d::Triangle*>& inputTriangles,
											unsigned char sizeLevels,
											double* sizeArray,
											std::vector<gaia3d::Triangle*>& outputTriangles,
											unsigned int* sizeIndexMarkers)
{
	
	double* squaredSizeArray = new double[sizeLevels];
	for(unsigned char i = 0; i < sizeLevels; i++)
		squaredSizeArray[i] = sizeArray[i]*sizeArray[i];

	std::vector<std::vector<gaia3d::Triangle*>*> eachSizeTriangles;
	for(unsigned char i = 0; i <= sizeLevels; i++)
		eachSizeTriangles.push_back(new std::vector<gaia3d::Triangle*>);

	size_t triangleCount = inputTriangles.size();
	double diffX, diffY, diffZ;
	double squaredEdgeLength0, squaredEdgeLength1, squaredEdgeLength2, minSquaredEdgeLength;
	for(size_t i = 0; i < triangleCount; i++)
	{
		diffX = inputTriangles[i]->getVertices()[0]->position.x - inputTriangles[i]->getVertices()[1]->position.x;
		diffY = inputTriangles[i]->getVertices()[0]->position.y - inputTriangles[i]->getVertices()[1]->position.y;
		diffZ = inputTriangles[i]->getVertices()[0]->position.z - inputTriangles[i]->getVertices()[1]->position.z;
		squaredEdgeLength0 = diffX*diffX + diffY*diffY + diffZ*diffZ;

		diffX = inputTriangles[i]->getVertices()[2]->position.x - inputTriangles[i]->getVertices()[1]->position.x;
		diffY = inputTriangles[i]->getVertices()[2]->position.y - inputTriangles[i]->getVertices()[1]->position.y;
		diffZ = inputTriangles[i]->getVertices()[2]->position.z - inputTriangles[i]->getVertices()[1]->position.z;
		squaredEdgeLength1 = diffX*diffX + diffY*diffY + diffZ*diffZ;

		diffX = inputTriangles[i]->getVertices()[2]->position.x - inputTriangles[i]->getVertices()[0]->position.x;
		diffY = inputTriangles[i]->getVertices()[2]->position.y - inputTriangles[i]->getVertices()[0]->position.y;
		diffZ = inputTriangles[i]->getVertices()[2]->position.z - inputTriangles[i]->getVertices()[0]->position.z;
		squaredEdgeLength2 = diffX*diffX + diffY*diffY + diffZ*diffZ;

		minSquaredEdgeLength = (squaredEdgeLength0 > squaredEdgeLength1) ?
			((squaredEdgeLength1 > squaredEdgeLength2) ? squaredEdgeLength2 : squaredEdgeLength1) :
			((squaredEdgeLength0 > squaredEdgeLength2) ? squaredEdgeLength2 : squaredEdgeLength0);

		bool bSorted = false;
		for(unsigned char j = 0; j < sizeLevels; j++)
		{
			if(minSquaredEdgeLength > squaredSizeArray[j])
			{
				eachSizeTriangles[j]->push_back(inputTriangles[i]);
				bSorted = true;
				break;
			}
		}
		if(!bSorted)
			eachSizeTriangles[sizeLevels]->push_back(inputTriangles[i]);
	}

	for(unsigned char i = 0; i < sizeLevels; i++)
	{
		outputTriangles.insert(outputTriangles.end(), eachSizeTriangles[i]->begin(), eachSizeTriangles[i]->end());
		sizeIndexMarkers[i] = (unsigned int)outputTriangles.size();
		delete eachSizeTriangles[i];
	}
	outputTriangles.insert(outputTriangles.end(), eachSizeTriangles[sizeLevels]->begin(), eachSizeTriangles[sizeLevels]->end());
	delete eachSizeTriangles[sizeLevels];

	delete[] squaredSizeArray;
}

void ConversionProcessor::loadAndBindTextures(std::map<std::wstring, std::wstring>& textureInfo, std::map<std::wstring, unsigned int>& bindingResult)
{
	std::map<std::wstring, std::wstring>::iterator itr = textureInfo.begin();
	std::wstring fileName, fullPath;
	for (; itr != textureInfo.end(); itr++)
	{
		fileName = itr->first;
		fullPath = itr->second;

		std::wstring::size_type dotPosition = fileName.rfind(L".");
		if (dotPosition == std::wstring::npos)
			continue;

		std::wstring fileExt = fileName.substr(dotPosition + 1, fileName.length() - dotPosition - 1);
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

		unsigned char* pDest = NULL;
		int width, height;
		unsigned int idTextureBound;
		if (fileExt.compare(L"jpg") == 0 || fileExt.compare(L"jpeg") == 0 || fileExt.compare(L"jpe") == 0 ||
			fileExt.compare(L"png") == 0 || fileExt.compare(L"gif") == 0 || fileExt.compare(L"tif") == 0 || fileExt.compare(L"tiff") == 0)
		{
			// loading
			std::string singleFullPath(CW2A(fullPath.c_str()));
			pDest = SOIL_load_image(singleFullPath.c_str(), &width, &height, NULL, SOIL_LOAD_RGBA);

			// binding
			glGenTextures(1, &idTextureBound);
			glBindTexture(GL_TEXTURE_2D, idTextureBound);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pDest);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBindTexture(GL_TEXTURE_2D, 0);
			SOIL_free_image_data(pDest);
		}
		else if (fileExt.compare(L"tga") == 0)
		{
			std::string singleFullPath(CW2A(fullPath.c_str()));
			idTextureBound = SOIL_load_OGL_texture(singleFullPath.c_str(), SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);
		}
		else if (fileExt.compare(L"dds") == 0)
		{
			std::string singleFullPath(CW2A(fullPath.c_str()));
			idTextureBound = SOIL_load_OGL_texture(singleFullPath.c_str(), SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_DDS_LOAD_DIRECT);
		}
		else
			continue;
	
		bindingResult.insert(std::map<std::wstring, unsigned int>::value_type(fileName, idTextureBound));
	}
}

void ConversionProcessor::extractLegoTextures(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::wstring, unsigned int>& bindingResult,unsigned int shaderProgram)
{
	scv->ClearColor[0] = 188.0f / 255.0f;
	scv->ClearColor[1] = 188.0f / 255.0f;
	scv->ClearColor[2] = 222.0f / 255.0f;
	unsigned int screen_size = MemoryDeviceContextEdgeLength;

	scv->tp_projection = PROJECTION_ORTHO;
	glViewport(0, 0, screen_size, screen_size);

	double max_length_bc = fullBbox.getMaxLength();
	scv->m_nRange = (float)(max_length_bc / 2.0 * 3.0);
	double cx, cy, cz;
	fullBbox.getCenterPoint(cx, cy, cz);

	unsigned int wa = screen_size;
	unsigned int ha = screen_size;

	// Mini define_space_of_visualitzation.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Make the rendering context current
	wglMakeCurrent(scv->m_myhDC, scv->m_hRC);
	glViewport(0, 0, (GLsizei)screen_size, (GLsizei)screen_size); // Set the viewport 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-scv->m_nRange*wa / ha, scv->m_nRange*wa / ha, -scv->m_nRange, scv->m_nRange, -scv->m_nRange*5000.0f, scv->m_nRange*5000.0f); // Note that wa/ha = 1.***
	glMatrixMode(GL_MODELVIEW);

	// Clear the screen and the depth buffer
	glClearColor(scv->ClearColor[0], scv->ClearColor[1], scv->ClearColor[2], scv->ClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset the model matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// 1) TOP.Col[0] row[0].***************************************************************************************
	glPushMatrix();
	scv->m_xRot = 0.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
	scv->m_xPos = -cx - max_length_bc;
	scv->m_yPos = -cy + max_length_bc;
	scv->m_zPos = -cz;
	/*
	scv->m_xRot = 0.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
	scv->m_xPos = 0.0;
	scv->m_yPos = 0.0;
	scv->m_zPos = -cz;
	*/

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);
	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	glPopMatrix();

	// 2) BOTTOM.Col[0] row[1].***************************************************************************************
	glPushMatrix();
	scv->m_xRot = 180.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
	scv->m_xPos = -cx - max_length_bc;
	scv->m_yPos = -cy;
	scv->m_zPos = -cz;

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);
	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	glPopMatrix();

	// 3) FRONT.Col[1] row[0].***************************************************************************************
	glPushMatrix();
	scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = 0.0;
	scv->m_xPos = -cx;
	scv->m_yPos = -cy;
	scv->m_zPos = -cz + max_length_bc;

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);
	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	glPopMatrix();

	// 4) REAR.Col[1] row[1].***************************************************************************************
	glPushMatrix();
	scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = 180.0;
	scv->m_xPos = -cx;
	scv->m_yPos = -cy;
	scv->m_zPos = -cz;

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);
	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	glPopMatrix();

	// 5) LEFT.Col[2] row[0].***************************************************************************************
	glPushMatrix();
	scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = 90.0;
	scv->m_xPos = -cx;
	scv->m_yPos = -cy - max_length_bc;
	scv->m_zPos = -cz + max_length_bc;

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);
	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	glPopMatrix();

	// 6) RIGHT.Col[2] row[1].***************************************************************************************
	glPushMatrix();
	scv->m_xRot = -90.0; scv->m_yRot = 0.0; scv->m_zRot = -90.0;
	scv->m_xPos = -cx;
	scv->m_yPos = -cy + max_length_bc;
	scv->m_zPos = -cz;

	glRotatef((float)scv->m_xRot, 1.0f, 0.0f, 0.0f);
	glRotatef((float)scv->m_zRot, 0.0f, 0.0f, 1.0f);
	// Apliquem les traslacions de l'escena.***
	glTranslatef((float)scv->m_xPos, (float)scv->m_yPos, (float)scv->m_zPos);
	drawMeshesWithTextures(meshes, bindingResult, shaderProgram);
	glPopMatrix();
	//-------------------------------------------------------------------------------------------------------------

	glFlush();

	SwapBuffers(scv->m_myhDC);

	// extract bitmap array from drawn result
	GLint viewPort[4];
	glGetIntegerv(GL_VIEWPORT, viewPort);
	//int imageSize = ((viewPort[2] + ((4 - (viewPort[2] % 4)) % 4))*viewPort[3] * 3) + 2;
	int imageSize = viewPort[2]*viewPort[3]*4;
	unsigned char* tempBitmap = new unsigned char[imageSize];
	memset(tempBitmap, 0x00, sizeof(unsigned char)*imageSize);
	legoTextureDimension[0] = viewPort[2];
	legoTextureDimension[1] = viewPort[3];
	GLenum lastBuffer;
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_PACK_SWAP_BYTES, 1);
	glGetIntegerv(GL_READ_BUFFER, (GLint*)&lastBuffer);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, viewPort[2], viewPort[3], GL_BGRA, GL_UNSIGNED_BYTE, tempBitmap);
	//legoTextureBitmap[imageSize - 1] = 0;
	//legoTextureBitmap[imageSize - 2] = 0;
	glReadBuffer(lastBuffer);

	legoTextureBitmap = new unsigned char[imageSize];
	memset(legoTextureBitmap, 0x00, sizeof(unsigned char)*imageSize);
	int width = viewPort[2], height = viewPort[3];
	for (int i = 0; i < height; i++)
	{
		memcpy(legoTextureBitmap + width * (height - i - 1) * 4, tempBitmap + width * i * 4, sizeof(unsigned char)*width*4 );
	}
	delete[] tempBitmap;

	// Return the normal values.***
	scv->ClearColor[0] = 1.0;
	scv->ClearColor[1] = 1.0;
	scv->ClearColor[2] = 1.0;
	scv->tp_projection = PROJECTION_PERSPECTIVE;
	defaultSpaceSetupForVisualization(scv->m_width, scv->m_height);
}

void ConversionProcessor::drawMeshesWithTextures(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::wstring, unsigned int>& bindingResult, unsigned int shaderProgram)
{
	// do this only once.
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Shader.
	int oneVertexBuffSize = 3 * 4 + 3 * 4 + 2 * 4;
	GLfloat mv_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv_matrix);

	GLfloat proj_matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, proj_matrix);

	GLfloat mvProj_matrix[16];

	gaia3d::Matrix4 mv_mat, proj_mat, mvProj_mat, mv_invMat, mv_invTransposedMat;

	mv_mat.set(mv_matrix[0], mv_matrix[4], mv_matrix[8], mv_matrix[12],
				mv_matrix[1], mv_matrix[5], mv_matrix[9], mv_matrix[13],
				mv_matrix[2], mv_matrix[6], mv_matrix[10], mv_matrix[14],
				mv_matrix[3], mv_matrix[7], mv_matrix[11], mv_matrix[15]);

	proj_mat.set(proj_matrix[0], proj_matrix[4], proj_matrix[8], proj_matrix[12],
				proj_matrix[1], proj_matrix[5], proj_matrix[9], proj_matrix[13],
				proj_matrix[2], proj_matrix[6], proj_matrix[10], proj_matrix[14],
				proj_matrix[3], proj_matrix[7], proj_matrix[11], proj_matrix[15]);

	mvProj_mat = proj_mat*mv_mat;
	mvProj_mat.getFloatArray(mvProj_matrix);

	mv_invMat = mv_mat.inverse();
	mv_invTransposedMat = mv_invMat.transpose();
	float normalMat3[9];
	mv_invMat.getOnlyRotationFloatArray(normalMat3);

	glUseProgram(shaderProgram);

	int normalMatrix_location = glGetUniformLocation(shaderProgram, "uNMatrix");
	int mvMatrix_location = glGetUniformLocation(shaderProgram, "ModelViewProjectionMatrix");

	glUniformMatrix4fv(mvMatrix_location, 1, false, mvProj_matrix);
	glUniformMatrix3fv(normalMatrix_location, 1, false, normalMat3);

	int position_location = glGetAttribLocation(shaderProgram, "position");
	int texCoord_location = glGetAttribLocation(shaderProgram, "aTextureCoord");
	//int normal_location = glGetAttribLocation(shaderProgram, "aVertexNormal");

	glEnableVertexAttribArray(position_location);
	glEnableVertexAttribArray(texCoord_location);

	int samplerUniform = glGetUniformLocation(shaderProgram, "uSampler");
	glUniform1i(samplerUniform, 0);

	int hasTexture_location = glGetUniformLocation(shaderProgram, "hasTexture");
	int colorAux_location = glGetUniformLocation(shaderProgram, "vColorAux");
	//int bUseLighting_location = glGetUniformLocation(shaderProgram, "useLighting");

	//glUniform1i(bUseLighting_location, false);

	//glUniform1i(hasTexture_location, true);

	size_t meshCount = meshes.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		gaia3d::TrianglePolyhedron* polyhedron = meshes[i];

		size_t vboCount = polyhedron->getVbos().size();
		// 이럴 일은 없겠지만 vbo가 없다는 것은 실체가 없는 객체이므로
		// filtering 한다.
		if (vboCount == 0)
			continue;

		GLfloat* vertices, *textureCoordinates;
		GLushort* indices;
		unsigned int textureCoordinateArrayId;
		for (size_t j = 0; j < vboCount; j++)
		{
			vertices = textureCoordinates = NULL;
			indices = NULL;

			gaia3d::Vbo* vbo = polyhedron->getVbos()[j];

			// make vertex array
			size_t vertexCount = vbo->vertices.size();
			if (vertexCount == 0)
				continue;

			vertices = new GLfloat[vertexCount * 3];
			memset(vertices, 0x00, sizeof(GLfloat) * 3 * vertexCount);

			for (size_t k = 0; k < vertexCount; k++)
			{
				vertices[k * 3] = float(vbo->vertices[k]->position.x);
				vertices[k * 3 + 1] = float(vbo->vertices[k]->position.y);
				vertices[k * 3 + 2] = float(vbo->vertices[k]->position.z);
			}

			// make index array
			size_t indexCount = vbo->indices.size();
			indices = new GLushort[indexCount];
			memset(indices, 0x00, sizeof(GLushort)*indexCount);
			for (size_t k = 0; k < indexCount; k++)
				indices[k] = vbo->indices[k];

			// at this point, all necessary arrays are made

			// bind vertex array
			unsigned int vertexArrayId;
			glGenBuffers(1, &vertexArrayId);
			glBindBuffer(GL_ARRAY_BUFFER, vertexArrayId);
			glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * 4, vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 3 * 4, (void*)0);    //send positions on pipe 0
			delete[] vertices;

			// bind index array
			unsigned int indexArrayId;
			glGenBuffers(1, &indexArrayId);// generate a new VBO and get the associated ID
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArrayId);// bind VBO in order to use
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * 2, indices, GL_STATIC_DRAW);// upload data to VBO
			delete[] indices;

			if (polyhedron->doesThisHaveTextureCoordinates())
			{
				// 이 polyhedron을 그릴 때 필요한 texture를 activate 시킨다.
				std::wstring textureName = polyhedron->getStringAttribute(std::wstring(TextureName));
				if (bindingResult.find(textureName) != bindingResult.end())
				{
					unsigned int textureId = bindingResult[textureName];
					glBindTexture(GL_TEXTURE_2D, textureId);
				}

				glUniform1i(hasTexture_location, true);

				// make texture coordinate array
				textureCoordinates = new GLfloat[vertexCount * 2];
				memset(textureCoordinates, 0x00, sizeof(GLfloat) * 2 * vertexCount);
				for (size_t k = 0; k < vertexCount; k++)
				{
					textureCoordinates[k * 2] = float(vbo->vertices[k]->textureCoordinate[0]);
					textureCoordinates[k * 2 + 1] = float(vbo->vertices[k]->textureCoordinate[1]);
				}

				// bind texture coordinate array
				glGenBuffers(1, &textureCoordinateArrayId);
				glBindBuffer(GL_ARRAY_BUFFER, textureCoordinateArrayId);
				glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * 4, textureCoordinates, GL_STATIC_DRAW);
				glVertexAttribPointer(texCoord_location, 2, GL_FLOAT, GL_FALSE, 2 * 4, (void*)0);
				delete[] textureCoordinates;
			}
			else
			{
				glUniform1i(hasTexture_location, false);
				float colorR, colorG, colorB;

				if (polyhedron->getColorMode() == gaia3d::SingleColor)
				{
					colorR = GetRedValue(polyhedron->getSingleColor()) / 255.0f;
					colorG = GetGreenValue(polyhedron->getSingleColor()) / 255.0f;
					colorB = GetBlueValue(polyhedron->getSingleColor()) / 255.0f;
				}
				else
				{
					colorR = GetRedValue(polyhedron->getVertices()[0]->color) / 255.0f;
					colorG = GetGreenValue(polyhedron->getVertices()[0]->color) / 255.0f;
					colorB = GetBlueValue(polyhedron->getVertices()[0]->color) / 255.0f;
				}

				glUniform3f(colorAux_location, colorR, colorG, colorB);
			}

			glDrawElements(GL_TRIANGLES, (int)indexCount, GL_UNSIGNED_SHORT, 0);

			glDeleteBuffers(1, &vertexArrayId);
			glDeleteBuffers(1, &indexArrayId);
			if (polyhedron->doesThisHaveTextureCoordinates())
				glDeleteBuffers(1, &textureCoordinateArrayId);
		}
	}

	glDisableVertexAttribArray(position_location);
	glDisableVertexAttribArray(texCoord_location);

	glUseProgram(0);
}

void ConversionProcessor::unbindTextures(std::map<std::wstring, unsigned int>& bindingResult)
{
	std::map<std::wstring, unsigned int>::iterator itr;
	for (itr = bindingResult.begin(); itr != bindingResult.end(); itr++)
	{
		unsigned int textureId = itr->second;
		glDeleteTextures(1, &textureId);
	}

	bindingResult.clear();
}

unsigned int ConversionProcessor::makeShaders()
{
	// vertex shader source
	GLchar vs_source[] =
	{
		"attribute vec3 position;\n"
		"attribute vec4 aVertexColor;\n"
		"attribute vec2 aTextureCoord;\n"
		"uniform mat4 ModelViewProjectionMatrix;\n"
		"varying vec4 vColor;\n"
		"varying vec2 vTextureCoord;\n"
		"attribute vec3 aVertexNormal;\n"
		"varying vec3 uAmbientColor;\n"
		"varying vec3 vLightWeighting;\n"
		"//uniform vec3 uLightingDirection;\n"
		"uniform mat3 uNMatrix;\n"
		"void main(void) { //pre-built function\n"
		"vec4 pos = vec4(position.xyz, 1.0);\n"
		"gl_Position = ModelViewProjectionMatrix * pos;\n"
		"vColor = aVertexColor;\n"
		"vTextureCoord = aTextureCoord;\n"
		"\n"
		"vLightWeighting = vec3(1.0, 1.0, 1.0);\n"
		"uAmbientColor = vec3(0.6, 0.6, 0.6);\n"
		"vec3 uLightingDirection = vec3(0.2, 0.2, -0.9);\n"
		"vec3 directionalLightColor = vec3(0.6, 0.6, 0.6);\n"
		"vec3 transformedNormal = uNMatrix * aVertexNormal;\n"
		"float directionalLightWeighting = max(dot(transformedNormal, uLightingDirection), 0.0);\n"
		"vLightWeighting = uAmbientColor + directionalLightColor * directionalLightWeighting;\n"
		"};\n"
	};

	const GLchar* const vertexShaderSource = vs_source;

	// create vertex shader object
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// attatch & compile vertex shader source
	glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
	glCompileShader(vertexShader);
	GLint isCompiled = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		glDeleteShader(vertexShader);
		return 0;
	}

	// fragment shader source
	GLchar fs_source[] =
	{
		"precision mediump float;\n"
		"varying vec4 vColor;\n"
		"varying vec2 vTextureCoord;\n"
		"uniform sampler2D uSampler;\n"
		"uniform bool hasTexture;\n"
		"uniform bool useLighting;\n"
		"uniform vec3 vColorAux;\n"
		"varying vec3 vLightWeighting;\n"
		"void main(void) {\n"
		"if(hasTexture)\n"
		"{\n"
		"vec4 textureColor = texture2D(uSampler, vec2(vTextureCoord.s, vTextureCoord.t));\n"
		"if(useLighting)\n"
		"{\n"
		"gl_FragColor = vec4(textureColor.rgb * vLightWeighting, textureColor.a);\n"
		"}\n"
		"else\n"
		"{\n"
		"gl_FragColor = textureColor;\n"
		"}\n"
		"}\n"
		"else \n"
		"{\n"
		"vec4 textureColor = vec4(vColorAux.xyz, 1.0);\n"
		"if(useLighting)\n"
		"{\n"
		"gl_FragColor = vec4(textureColor.rgb * vLightWeighting, textureColor.a);\n"
		"}\n"
		"else\n"
		"{\n"
		"gl_FragColor = textureColor;\n"
		"}\n"
		"}\n"

		"}\n"
	};

	const GLchar* const fragmentShaderSource = fs_source;

	// create fragment shader object
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// attatch & compile fragment shader source
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	//Vertex and fragment shaders are successfully compiled.
	//Now time to link them together into a program.
	//Get a program object.
	GLuint program = glCreateProgram();

	//Attach our shaders to our program
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	//Link our program
	glLinkProgram(program);
	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
	if (isLinked == GL_FALSE)
	{
		glDeleteProgram(program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return 0;
	}

	//Always detach shaders after a successful link.
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

void ConversionProcessor::deleteShaders(unsigned int programId)
{
	glUseProgram(0);
	glDeleteProgram(programId);
}