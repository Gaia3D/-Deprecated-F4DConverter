// IfcLoader.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "IfcLoader.h"

#include "ifcpp/model/IfcPPModel.h"
#include "ifcpp/geometry/GeometryConverter.h"
#include "ifcpp/reader/IfcPPReaderSTEP.h"
#include "ifcpp/IFC4/include/IfcSite.h"
#include "ifcpp/IFC4/include/IfcSpace.h"



class MessageWrapper
{
public:
	static void slotMessageWrapper(void* obj_ptr, shared_ptr<StatusCallback::Message> m)
	{
		if (m)
		{
			if (m->m_message_type != StatusCallback::MESSAGE_TYPE_PROGRESS_VALUE && m->m_message_type != StatusCallback::MESSAGE_TYPE_PROGRESS_TEXT)
			{
				std::wcout << m->m_message_text << std::endl;
			}
		}
	}
};

class IfcLoader : public aIfcLoader
{
public:
	IfcLoader();
	virtual ~IfcLoader();

public:
	virtual bool loadIfcFile(std::wstring& filePath);

	virtual void setVertexReductionMode(bool bOn);

	virtual size_t getPolyhedronCount();
	virtual float* getRepresentativeColor(size_t polyhedronIndex);
	virtual std::wstring getGuid(size_t polyhedronIndex);
	virtual size_t getVertexCount(size_t polyhedronIndex);
	virtual double* getVertexPositions(size_t polyhedronIndex);
	virtual size_t getSurfaceCount(size_t polyhedronIndex);
	virtual size_t getTrialgleCount(size_t polyhedronIndex, size_t surfaceIndex);
	virtual size_t* getTriangleIndices(size_t polyhedronIndex, size_t surfaceIndex);

private:

	bool bVertexReduction;

	struct Surface
	{
		size_t triangleCount;
		size_t* triangleIndices;
	};

	struct Polyhedron
	{
		size_t vertexCount;
		double* vertices;
		float color[4];
		std::vector<Surface*> surfaces;
		std::wstring guid;
	};

	std::vector<Polyhedron*> polyhedrons;
};

IfcLoader::IfcLoader()
{
	bVertexReduction = false;
}

IfcLoader::~IfcLoader()
{
	size_t polyhedronCount = polyhedrons.size();
	size_t surfaceCount;
	for (size_t i = 0; i < polyhedronCount; i++)
	{
		delete[] polyhedrons[i]->vertices;
		surfaceCount = polyhedrons[i]->surfaces.size();
		for (size_t j = 0; j < surfaceCount; j++)
		{
			delete[] polyhedrons[i]->surfaces[j]->triangleIndices;
			delete polyhedrons[i]->surfaces[j];
		}

		polyhedrons[i]->surfaces.clear();

		delete polyhedrons[i];
	}

	polyhedrons.clear();
}

bool IfcLoader::loadIfcFile(std::wstring& filePath)
{
	// initializing
	shared_ptr<MessageWrapper> mw(new MessageWrapper());
	shared_ptr<IfcPPModel> ifc_model(new IfcPPModel());
	shared_ptr<GeometryConverter> geometry_converter(new GeometryConverter(ifc_model));
	shared_ptr<IfcPPReaderSTEP> reader(new IfcPPReaderSTEP());

	reader->setMessageCallBack(mw.get(), &MessageWrapper::slotMessageWrapper);
	geometry_converter->setMessageCallBack(mw.get(), &MessageWrapper::slotMessageWrapper);

	// loading
	reader->loadModelFromFile(filePath, ifc_model);

	// conversion raw data into geometries of OSG type
	osg::ref_ptr<osg::Switch> model_switch = new osg::Switch();
	geometry_converter->createGeometryOSG(model_switch);

	// contains the VEF graph for each IfcProduct:
	std::map<int, shared_ptr<ProductShapeInputData> >& map_vef_data = geometry_converter->getShapeInputData();
	double volume_all_products = 0;

	std::map<int, shared_ptr<ProductShapeInputData> >::iterator it;
	for (it = map_vef_data.begin(); it != map_vef_data.end(); ++it)
	//for (auto it = map_vef_data.begin(); it != map_vef_data.end(); ++it)
	{
		// STEP entity id:
		int entity_id = it->first;

		// shape data
		shared_ptr<ProductShapeInputData>& shape_data = it->second;

		// IfcProduct(abstract type)
		shared_ptr<IfcProduct> ifc_product(shape_data->m_ifc_product);

		// filtering out IfcProduct of IfcSpace type
		shared_ptr<IfcSpace> Ifc_Space = dynamic_pointer_cast<IfcSpace>(ifc_product);
		if (Ifc_Space != NULL)
			continue;

		// filtering out IfcProduct of IfcSite type
		shared_ptr<IfcSite> site_elem = dynamic_pointer_cast<IfcSite>(ifc_product);
		if (site_elem  != NULL)
			continue;

		// for each IfcProduct, there can be mulitple geometric representation items:
		std::vector<shared_ptr<ProductRepresentationData> >& vec_representations = shape_data->m_vec_representations;
		for (size_t i_representation = 0; i_representation < vec_representations.size(); ++i_representation)
		{
			shared_ptr<ProductRepresentationData>& representation_data = vec_representations[i_representation];

			// a representation item can have multiple item shapes
			std::vector<shared_ptr<ItemShapeInputData> >& vec_item_data = representation_data->m_vec_item_data;
			for (size_t i_item = 0; i_item < vec_item_data.size(); ++i_item)
			{
				shared_ptr<ItemShapeInputData>& item_data = vec_item_data[i_item];

				// appearance data for getting color
				std::vector<shared_ptr<AppearanceData> > vec_items_appearances = item_data->m_vec_item_appearances;

				std::vector<shared_ptr<carve::mesh::MeshSet<3> > > allMeshsets;
				allMeshsets.insert(allMeshsets.end(), item_data->m_meshsets.begin(), item_data->m_meshsets.end());
				allMeshsets.insert(allMeshsets.end(), item_data->m_meshsets_open.begin(), item_data->m_meshsets_open.end());
				Polyhedron* polyhedron;
				Surface* surface;
				for (size_t i_meshset = 0; i_meshset < allMeshsets.size(); ++i_meshset)
				{
					// A meshset == A Polyhedron
					shared_ptr<carve::mesh::MeshSet<3> >& meshset = allMeshsets[i_meshset];
					polyhedron = NULL;

					////////////////////////////////////
					if (bVertexReduction)
					{
						// vertices of this meshset(to add all vertices to a Polyhedron)
						std::map<carve::mesh::Vertex<3> *, size_t> vertices;
						std::map<carve::mesh::Vertex<3> *, size_t>::iterator vertexIter;

						// A meshset is composed of multiple meshes.( mesh = surface)
						std::vector<carve::mesh::Mesh<3>* >& vec_meshes = meshset->meshes;
						for (size_t i_mesh = 0; i_mesh < vec_meshes.size(); ++i_mesh)
						{
							// Mesh == Surface
							carve::mesh::Mesh<3>* mesh = vec_meshes[i_mesh];
							surface = NULL;

							// vertex indices to be used to compose of triangles
							std::vector<size_t> indices;

							// A mesh is composed of multiple faces. (face == triangle)
							std::vector<carve::mesh::Face<3>* >& vec_faces = mesh->faces;
							for (size_t i_face = 0; i_face < vec_faces.size(); ++i_face)
							{
								// Face == Triangle
								carve::mesh::Face<3>* face = vec_faces[i_face];

								// iterate through edges:
								carve::mesh::Edge<3>* edge = face->edge;
								size_t vertexIndex;
								do
								{
									// start vertices of each edge
									// 각 edge의 start vertext만 instance화 한다.
									// 왜냐하면 한 edge의 end vertex는 다음 edge의 start vertex이므로.
									carve::mesh::Vertex<3>* vertex_begin = edge->v1();

									vertexIter = vertices.find(vertex_begin);
									if (vertexIter == vertices.end())
									{
										vertexIndex = vertices.size();
										vertices.insert(std::map<carve::mesh::Vertex<3>*, size_t>::value_type(vertex_begin, vertexIndex));
									}
									else
										vertexIndex = vertexIter->second;

									indices.push_back(vertexIndex);

									// 각 edge의 start vertext만 instance화 한다.
									// 왜냐하면 한 edge의 end vertex는 다음 edge의 start vertex이므로.

									edge = edge->next;
								} while (edge != face->edge);
							}

							// fill triangle count and triangle vertex indices
							if (indices.size() / 3 == 0)
								continue;

							surface = new Surface;

							surface->triangleCount = indices.size() / 3;
							surface->triangleIndices = new size_t[3 * surface->triangleCount];
							memset(surface->triangleIndices, 0x00, sizeof(size_t) * 3 * surface->triangleCount);
							for (size_t iIndex = 0; iIndex < 3 * surface->triangleCount; iIndex++)
								surface->triangleIndices[iIndex] = indices[iIndex];

							// add this surface to this polyhedron
							if (polyhedron == NULL)
								polyhedron = new Polyhedron;

							polyhedron->surfaces.push_back(surface);
						}

						if (polyhedron == NULL)
							continue;

						// fill vertex count and vertex position information
						std::map<size_t, carve::mesh::Vertex<3> *> verticesSorted;
						for (vertexIter = vertices.begin(); vertexIter != vertices.end(); vertexIter++)
							verticesSorted.insert(std::map<size_t, carve::mesh::Vertex<3> *>::value_type(vertexIter->second, vertexIter->first));

						polyhedron->vertexCount = verticesSorted.size();
						polyhedron->vertices = new double[3 * polyhedron->vertexCount];
						memset(polyhedron->vertices, 0x00, sizeof(double) * 3 * polyhedron->vertexCount);
						for (size_t vIndex = 0; vIndex < polyhedron->vertexCount; vIndex++)
						{
							polyhedron->vertices[3 * vIndex] = verticesSorted[vIndex]->v.x;
							polyhedron->vertices[3 * vIndex + 1] = verticesSorted[vIndex]->v.y;
							polyhedron->vertices[3 * vIndex + 2] = verticesSorted[vIndex]->v.z;
						}
					}
					////////////////////////
					else
					{
						// vertices of this meshset(to add all vertices to a Polyhedron)
						std::vector<carve::mesh::Vertex<3> *> vertices;

						// A meshset is composed of multiple meshes.( mesh = surface)
						std::vector<carve::mesh::Mesh<3>* >& vec_meshes = meshset->meshes;
						for (size_t i_mesh = 0; i_mesh < vec_meshes.size(); ++i_mesh)
						{
							// Mesh == Surface
							carve::mesh::Mesh<3>* mesh = vec_meshes[i_mesh];
							surface = NULL;

							// A mesh is composed of multiple faces. (face == triangle)
							std::vector<carve::mesh::Face<3>* >& vec_faces = mesh->faces;
							for (size_t i_face = 0; i_face < vec_faces.size(); ++i_face)
							{
								// Face == Triangle
								carve::mesh::Face<3>* face = vec_faces[i_face];

								// iterate through edges:
								carve::mesh::Edge<3>* edge = face->edge;
								do
								{
									// start vertices of each edge
									// 각 edge의 start vertext만 instance화 한다.
									// 왜냐하면 한 edge의 end vertex는 다음 edge의 start vertex이므로.
									carve::mesh::Vertex<3>* vertex_begin = edge->v1();

									vertices.push_back(vertex_begin);

									edge = edge->next;
								} while (edge != face->edge);
							}

							if (vertices.size() / 3 == 0)
								continue;

							surface = new Surface;

							// fill triangle count and triangle vertex indices
							surface->triangleCount = vertices.size() / 3;
							surface->triangleIndices = new size_t[3 * surface->triangleCount];
							memset(surface->triangleIndices, 0x00, sizeof(size_t) * 3 * surface->triangleCount);
							for (size_t iIndex = 0; iIndex < 3 * surface->triangleCount; iIndex++)
								surface->triangleIndices[iIndex] = iIndex;

							// add this surface to this polyhedron
							if (polyhedron == NULL)
								polyhedron = new Polyhedron;

							polyhedron->surfaces.push_back(surface);
						}

						if (polyhedron == NULL)
							continue;

						polyhedron->vertexCount = vertices.size();
						polyhedron->vertices = new double[3 * polyhedron->vertexCount];
						memset(polyhedron->vertices, 0x00, sizeof(double) * 3 * polyhedron->vertexCount);
						for (size_t vIndex = 0; vIndex < polyhedron->vertexCount; vIndex++)
						{
							polyhedron->vertices[3 * vIndex] = vertices[vIndex]->v.x;
							polyhedron->vertices[3 * vIndex + 1] = vertices[vIndex]->v.y;
							polyhedron->vertices[3 * vIndex + 2] = vertices[vIndex]->v.z;
						}
					}

					// get representative color of a Polyhedron, if exist
					if (vec_items_appearances.size() > 0)
					{
						polyhedron->color[0] = vec_items_appearances[0]->m_color_diffuse.x;
						polyhedron->color[1] = vec_items_appearances[0]->m_color_diffuse.y;
						polyhedron->color[2] = vec_items_appearances[0]->m_color_diffuse.z;
						polyhedron->color[3] = vec_items_appearances[0]->m_color_diffuse.w;
					}
					else
						memset(polyhedron->color, 0x00, sizeof(float) * 4);

					// extract and allocate guid into this polyhedron
					polyhedron->guid = ifc_product->m_GlobalId->m_value;

					// add this polyhedron
					polyhedrons.push_back(polyhedron);
				}
			}
		}
	}

	return true;
}

void IfcLoader::setVertexReductionMode(bool bOn)
{
	bVertexReduction = bOn;
}

size_t IfcLoader::getPolyhedronCount()
{
	return polyhedrons.size();
}

float* IfcLoader::getRepresentativeColor(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->color;
}

std::wstring IfcLoader::getGuid(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->guid;
}

size_t IfcLoader::getVertexCount(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->vertexCount;
}

double* IfcLoader::getVertexPositions(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->vertices;
}

size_t IfcLoader::getSurfaceCount(size_t polyhedronIndex)
{
	return polyhedrons[polyhedronIndex]->surfaces.size();
}

size_t IfcLoader::getTrialgleCount(size_t polyhedronIndex, size_t surfaceIndex)
{
	return polyhedrons[polyhedronIndex]->surfaces[surfaceIndex]->triangleCount;
}

size_t* IfcLoader::getTriangleIndices(size_t polyhedronIndex, size_t surfaceIndex)
{
	return polyhedrons[polyhedronIndex]->surfaces[surfaceIndex]->triangleIndices;
}

aIfcLoader* createIfcLoader()
{
	return new IfcLoader;
}

void destroyIfcLoader(aIfcLoader* aLoader)
{
	delete static_cast<IfcLoader*>(aLoader);
}
