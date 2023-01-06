/**
 * @file myMesh.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "myMesh.hpp"
#include "mesh.hpp"
#include "meshView.hpp"
#include "root.hpp"
#include "buffer.hpp"
#include "modelResource.hpp"
#include "sceneManager.hpp"

std::shared_ptr<MyMesh> MyMeshFactory::buildAxis()
{
	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();
	a.vertices = {
		{{0, 0, 0}, {}, {}, {0, 0, 0, 1}},
		{{1, 0, 0}, {}, {}, {1, 0, 0, 1}},

		{{0, 0, 0}, {}, {}, {0, 0, 0, 1}},
		{{0, 1, 0}, {}, {}, {0, 1, 0, 1}},

		{{0, 0, 0}, {}, {}, {0, 0, 0, 1}},
		{{0, 0, 1}, {}, {}, {0, 0, 1, 1}},
	};
	a.topology = Shit::PrimitiveTopology::LINE_LIST;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory::buildTriangle()
{
	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();
	a.vertices = {
		{{0, 1, 0}, {}, {0, 1}, {1, 0, 0, 1}},
		{{0, 0, 0}, {}, {0, 0}, {0, 1, 0, 1}},
		{{1, 0, 0}, {}, {1, 0}, {0, 0, 1, 1}},
	};
	a.topology = Shit::PrimitiveTopology::TRIANGLE_LIST;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory ::buildQuad()
{
	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();
	a.vertices = {
		{{0, 1, 0}, {}, {0, 1}, {0, 1, 0, 1}},
		{{0, 0, 0}, {}, {0, 0}, {0, 0, 0, 1}},
		{{1, 1, 0}, {}, {1, 1}, {1, 1, 0, 1}},

		{{1, 1, 0}, {}, {1, 1}, {1, 1, 0, 1}},
		{{0, 0, 0}, {}, {0, 0}, {0, 0, 0, 1}},
		{{1, 0, 0}, {}, {1, 0}, {1, 0, 0, 1}},
	};
	a.topology = Shit::PrimitiveTopology::TRIANGLE_LIST;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory ::buildCube()
{
	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();
	a.vertices = {
		// position			normal		 texture coordinate		tangent		bitangent
		// back face x,y
		{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, //-1.0f,  0.0f, 0.0f,	  0.0f,  1.0f,  0.0f,// bottom-left
		{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},	//-1.0f,  0.0f, 0.0f,	  0.0f,  1.0f,  0.0f,// top-right
		{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},	//-1.0f,  0.0f, 0.0f,	  0.0f,  1.0f,  0.0f,// bottom-right
		{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},	//-1.0f,  0.0f, 0.0f,	  0.0f,  1.0f,  0.0f,// top-right
		{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, //-1.0f,  0.0f, 0.0f,	  0.0f,  1.0f,  0.0f,// bottom-left
		{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},	//-1.0f,  0.0f, 0.0f,	  0.0f,  1.0f,  0.0f,// top-left
		// front face
		{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 1.0f,  0.0f,  0.0f,	  0.0f,  1.0f,  0.0f,// bottom-left
		{{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // 1.0f,  0.0f,  0.0f,	  0.0f,  1.0f,  0.0f,// bottom-right
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},	  // 1.0f,  0.0f,  0.0f,	  0.0f,  1.0f,  0.0f,// top-right
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},	  // 1.0f,  0.0f,  0.0f,	  0.0f,  1.0f,  0.0f,// top-right
		{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // 1.0f,  0.0f,  0.0f,	  0.0f,  1.0f,  0.0f,// top-left
		{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 1.0f,  0.0f,  0.0f,	  0.0f,  1.0f,  0.0f,// bottom-left
		// left face
		{{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},	// 0.0f,  0.0f,  1.0f,	  0.0f,  1.0f,  0.0f,// top-right
		{{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},	// 0.0f,  0.0f,  1.0f,	  0.0f,  1.0f,  0.0f,// top-left
		{{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // 0.0f,  0.0f,  1.0f,	  0.0f,  1.0f,  0.0f,// bottom-left
		{{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // 0.0f,  0.0f,  1.0f,	  0.0f,  1.0f,  0.0f,// bottom-left
		{{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},	// 0.0f,  0.0f,  1.0f,	  0.0f,  1.0f,  0.0f,// bottom-right
		{{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},	// 0.0f,  0.0f,  1.0f,	  0.0f,  1.0f,  0.0f,// top-right
		// right face
		{{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},	  // 0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,// top-left
		{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // 0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,// bottom-right
		{{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // 0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,// top-right
		{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // 0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,// bottom-right
		{{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},	  // 0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,// top-left
		{{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // 0.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,// bottom-left
		// bottom face
		{{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // 1.0f,  0.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// top-right
		{{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},	// 1.0f,  0.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// top-left
		{{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},	// 1.0f,  0.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// bottom-left
		{{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},	// 1.0f,  0.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// bottom-left
		{{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},	// 1.0f,  0.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// bottom-right
		{{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // 1.0f,  0.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// top-right
		// top face
		{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // 1.0f,  1.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// top-left
		{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},	  // 1.0f,  1.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// bottom-right
		{{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // 1.0f,  1.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// top-right
		{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},	  // 1.0f,  1.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// bottom-right
		{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // 1.0f,  1.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// top-left
		{{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},  // 1.0f,  1.0f,  0.0f,	  0.0f,  0.0f,  1.0f,// bottom-left
	};
	for (auto &e : a.vertices)
	{
		e.color[0] = e.position[0];
		e.color[1] = e.position[1];
		e.color[2] = e.position[2];
		e.color[3] = 1;
	}
	a.topology = Shit::PrimitiveTopology::TRIANGLE_LIST;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory ::buildCircle()
{
	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();

	const unsigned int SEGMENT = 64;
	a.vertices.resize(SEGMENT + 2);
	a.vertices[0].position = {};
	for (int i = 0; i < SEGMENT; ++i)
	{
		float theta = float(i * 2 * PI) / float(SEGMENT);
		a.vertices[i + 1] = {{glm::cos(theta), glm::sin(theta), 0}, {0, 0, 1}};
	}
	a.vertices[SEGMENT + 1] = {{1, 0, 0}, {0, 0, 1}, {}};
	for (auto &e : a.vertices)
	{
		e.color[0] = e.position[0];
		e.color[1] = e.position[1];
		e.color[2] = e.position[2];
		e.color[3] = 1;
	}
	a.topology = Shit::PrimitiveTopology::TRIANGLE_FAN;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory ::buildSphere()
{
	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();
	const unsigned int X_SEGMENTS = 64;
	const unsigned int Y_SEGMENTS = X_SEGMENTS / 2;
	a.vertices.resize(X_SEGMENTS * (Y_SEGMENTS + 1));

	for (int j = 0; j <= Y_SEGMENTS; ++j)
	{
		for (int i = 0; i < X_SEGMENTS; ++i)
		{
			float theta = float(i * 2 * PI) / X_SEGMENTS;
			float phi = float(j * PI) / Y_SEGMENTS;

			float cosTheta = glm::cos(theta);
			float sinTheta = glm::sin(theta);
			float cosPhi = -glm::cos(phi); // from bottom to top
			float sinPhi = glm::sin(phi);

			int index = j * X_SEGMENTS + i;

			a.vertices[index].normal = a.vertices[index].position = {sinPhi * cosTheta, cosPhi, sinPhi * sinTheta};
			a.vertices[index].texcoord = {theta, phi};
		}
	}

	// a.indices
	a.indices.resize(2 * X_SEGMENTS * (Y_SEGMENTS + 1));
	for (int j = 0; j <= Y_SEGMENTS; ++j)
	{
		for (int i = 0; i < X_SEGMENTS - 1; ++i)
		{
			a.indices[i * 2 * (Y_SEGMENTS + 1) + 2 * j] = j * (X_SEGMENTS) + i + 1;
			a.indices[i * 2 * (Y_SEGMENTS + 1) + 2 * j + 1] = j * (X_SEGMENTS) + i;
		}
		a.indices[(X_SEGMENTS - 1) * 2 * (Y_SEGMENTS + 1) + 2 * j] = j * (X_SEGMENTS);
		a.indices[(X_SEGMENTS - 1) * 2 * (Y_SEGMENTS + 1) + 2 * j + 1] = j * (X_SEGMENTS) + X_SEGMENTS - 1;
	}
	for (auto &e : a.vertices)
	{
		e.color[0] = e.position[0];
		e.color[1] = e.position[1];
		e.color[2] = e.position[2];
		e.color[3] = 1;
	}
	a.topology = Shit::PrimitiveTopology::TRIANGLE_STRIP;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory ::buildTerrain()
{
	/**
	 * 500x500 terrain
	 *
	 */

	auto ret = std::make_shared<MyMesh>();
	auto &a = ret->primitives.emplace_back();
	const unsigned int X_SEGMENTS = 500;
	const unsigned int Y_SEGMENTS = 500;

	a.vertices.reserve((X_SEGMENTS + 1) * (Y_SEGMENTS + 1));
	float startX = -X_SEGMENTS / 2;
	float startY = -Y_SEGMENTS / 2;
	float px;
	for (int i = 0; i <= X_SEGMENTS; ++i)
	{
		px = startX + i;
		for (int j = 0; j <= Y_SEGMENTS; ++j)
		{
			a.vertices.emplace_back(
				Vertex::position_type{px, 0.f, startY + j},
				Vertex::normal_type{0.f, 1.f, 0.f},
				Vertex::texcoord_type{px, startY + j});
		}
	}
	// a.indices.resize();
	for (int i = 0; i < X_SEGMENTS; ++i)
	{
		for (int j = 0; j <= Y_SEGMENTS; ++j)
		{
		}
	}

	a.topology = Shit::PrimitiveTopology::TRIANGLE_STRIP;
	return ret;
}
std::shared_ptr<MyMesh> MyMeshFactory ::buildMesh(MyMeshType type)
{
	auto wp = _meshes[static_cast<size_t>(type)];
	if (wp.expired())
	{
		std::shared_ptr<MyMesh> ret;
		switch (type)
		{
		case MyMeshType::TRIANGLE:
			ret = buildTriangle();
			break;
		case MyMeshType::QUAD:
			ret = buildQuad();
			break;
		case MyMeshType::CIRCLE:
			ret = buildCircle();
			break;
		case MyMeshType::CUBE:
			ret = buildCube();
			break;
		case MyMeshType::SPHERE:
			ret = buildSphere();
			break;
		case MyMeshType::AXIS:
			ret = buildAxis();
			break;
		default:
			THROW("unknow MyMeshType:", (int)type);
		}
		_meshes[static_cast<size_t>(type)] = ret;
		return ret;
	}
	else
		return wp.lock();
}
MyModel *MyModelFactory::buildAxis(std::string_view name)
{
	return new MyModel{
		std::string(name),
		{MyMeshFactory::getSingleton().buildMesh(MyMeshType::AXIS)},
		{{0, -1}}};
}
MyModel *MyModelFactory ::buildTriangle(std::string_view name)
{
	return new MyModel{
		std::string(name),
		{MyMeshFactory::getSingleton().buildMesh(MyMeshType::TRIANGLE)},
		{{0, -1}}};
}
MyModel *MyModelFactory::buildQuad(std::string_view name)
{
	return new MyModel{
		std::string(name),
		{MyMeshFactory::getSingleton().buildMesh(MyMeshType::QUAD)},
		{{0, -1}}};
}
MyModel *MyModelFactory::buildCube(std::string_view name)
{
	return new MyModel{
		std::string(name),
		{MyMeshFactory::getSingleton().buildMesh(MyMeshType::CUBE)},
		{{0, -1}}};
}
MyModel *MyModelFactory::buildCircle(std::string_view name)
{
	return new MyModel{
		std::string(name),
		{MyMeshFactory::getSingleton().buildMesh(MyMeshType::CIRCLE)},
		{{0, -1}}};
}
MyModel *MyModelFactory::buildSphere(std::string_view name)
{
	return new MyModel{
		std::string(name),
		{MyMeshFactory::getSingleton().buildMesh(MyMeshType::SPHERE)},
		{{0, -1}}};
}
std::unique_ptr<MyModel> MyModelFactory::buildModel(std::string_view name, MyModelType type)
{
	MyModel *ret;
	switch (type)
	{
	case MyModelType::NONE:
		ret = new MyModel{std::string(name)};
		break;
	case MyModelType::TRIANGLE:
		ret = buildTriangle(name);
		break;
	case MyModelType::QUAD:
		ret = buildQuad(name);
		break;
	case MyModelType::CIRCLE:
		ret = buildCircle(name);
		break;
	case MyModelType::CUBE:
		ret = buildCube(name);
		break;
	case MyModelType::SPHERE:
		ret = buildSphere(name);
		break;
	case MyModelType::AXIS:
		ret = buildAxis(name);
		break;
	default:
		THROW("unknow MyModelType:", (int)type);
	}
	return std::unique_ptr<MyModel>(ret);
}

//========================================
static Buffer *s_vertexBuffer;
static Buffer *s_indexBuffer;
void transformPrimitive(MyPrimitive *srcPrimitive, Primitive *dstPrimitve)
{
	dstPrimitve->topology = srcPrimitive->topology;
	auto count = srcPrimitive->vertices.size();
	auto positionBufferView = s_vertexBuffer->allocate(sizeof(Vertex::position_type), count);
	auto normalBufferView = s_vertexBuffer->allocate(sizeof(Vertex::normal_type), count);
	auto texcoordBufferView = s_vertexBuffer->allocate(sizeof(Vertex::texcoord_type), count);
	auto colorBufferView = s_vertexBuffer->allocate(sizeof(Vertex::color_type), count);

	for (size_t i = 0; i < count; ++i)
	{
		auto &&e = srcPrimitive->vertices[i];
		s_vertexBuffer->setData(positionBufferView.offset + i * sizeof(Vertex::position_type), sizeof(Vertex::position_type), e.position.data());
		s_vertexBuffer->setData(normalBufferView.offset + i * sizeof(Vertex::normal_type), sizeof(Vertex::normal_type), e.normal.data());
		s_vertexBuffer->setData(texcoordBufferView.offset + i * sizeof(Vertex::texcoord_type), sizeof(Vertex::texcoord_type), e.texcoord.data());
		s_vertexBuffer->setData(colorBufferView.offset + i * sizeof(Vertex::color_type), sizeof(Vertex::color_type), e.color.data());
	}
	BufferView indexBufferView;
	if (!srcPrimitive->indices.empty())
	{
		indexBufferView = s_indexBuffer->allocate(sizeof(MyPrimitive::index_type), srcPrimitive->indices.size(), srcPrimitive->indices.data());
		dstPrimitve->index = std::make_unique<Primitive::IndexDescription>(indexBufferView, Shit::IndexType::UINT16);
	}
	dstPrimitve->position = std::make_unique<Primitive::AttributeDescription>(positionBufferView, DataType::VEC3);
	dstPrimitve->normal = std::make_unique<Primitive::AttributeDescription>(normalBufferView, DataType::VEC3);
	dstPrimitve->texcoords.emplace_back(Primitive::AttributeDescription(texcoordBufferView, DataType::VEC2));
	dstPrimitve->colors.emplace_back(Primitive::AttributeDescription(colorBufferView, DataType::VEC4));

	dstPrimitve->materialDataBlock = Root::getSingleton().getMaterialDataBlockManager()->getDefaultMaterialBlock();
}

//==============================================================
void MyModelLoader::loadResource(Resource *resource)
{
	auto pModelRes = static_cast<ModelResource *>(resource);
	auto pModelSrc = *reinterpret_cast<MyModel const *const *>(resource->getData().data());

	// allocat buffer for meshes
	auto bufferManager = Root::getSingleton().getBufferManager();
	s_vertexBuffer = bufferManager->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::TRANSFER_DST_BIT |
				Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT,
			//Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
				Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			false});
	s_indexBuffer = bufferManager->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::TRANSFER_DST_BIT |
				Shit::BufferUsageFlagBits::INDEX_BUFFER_BIT,
			//Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
				Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			false});

	// create meshes
	auto meshManager = Root::getSingleton().getMeshManager();
	for (auto &&srcMesh : pModelSrc->meshes)
	{
		auto mesh = meshManager->createMesh();
		pModelRes->addMesh(mesh);
		for (auto &&srcPrimitive : srcMesh->primitives)
		{
			auto primitive = mesh->addPrimitive(srcPrimitive.topology);
			transformPrimitive(&srcPrimitive, primitive);
		}
		mesh->init();
	}
	auto pModel= pModelRes->getModel();

	// auto &&subscene = pModel->addSubscene({});
	//std::vector<SceneNode *> sceneNodes;
	// for (auto &&srcNode : pModelSrc->nodes)
	for (size_t i = 0; i < pModelSrc->nodes.size(); ++i)
	{
		auto &&srcNode = pModelSrc->nodes[i];
		auto node = new SceneNode(true);
		node->addComponent<MeshView>(pModelRes->getMeshByIndex(srcNode.meshIndex));
		pModel->setChildIndex(node, i);
	}
	for (size_t i = 0; i < pModelSrc->nodes.size(); ++i)
	{
		auto &&srcNode = pModelSrc->nodes[i];
		if (srcNode.parentNodeIndex == -1)
		{
			pModel->addChild(pModel->getChildByIndex(i));
		}
		else
		{
			pModel->getChildByIndex(srcNode.parentNodeIndex)->addChild(pModel->getChildByIndex(i));
		}
	}
}