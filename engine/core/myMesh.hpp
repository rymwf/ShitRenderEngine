/**
 * @file myMesh.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include "prerequisites.hpp"
#include "resource.hpp"
#include "singleton.hpp"

struct Vertex
{
	using position_type = std::array<float, 3>;
	using normal_type = std::array<float, 3>;
	using texcoord_type = std::array<float, 2>;
	using color_type= std::array<float, 4>;

	position_type position;
	normal_type normal;
	texcoord_type texcoord;
	color_type color;
};

struct MyPrimitive
{
	using index_type = uint16_t;
	std::vector<Vertex> vertices;
	std::vector<index_type> indices;
	Shit::PrimitiveTopology topology;
};

struct MyMesh
{
	std::vector<MyPrimitive> primitives;
};

struct MyModel
{
	struct NodeAttribute
	{
		int meshIndex;
		int parentNodeIndex; //-1 mean no parent
							 // transformations
							 // std::array<float, 3> scale;
							 // std::array<float, 4> rotation;
							 // std::array<float, 3> transform;
	};
	std::string name;
	std::vector<std::shared_ptr<MyMesh>> meshes;
	// index is mesh index, value is parent node, -1 means no parent
	std::vector<NodeAttribute> nodes;
};

enum class MyMeshType
{
	TRIANGLE,
	QUAD,
	CIRCLE,
	CUBE,
	CUBE2,
	SPHERE,
	TERRAIN,
	AXIS,
	Num,
};
enum class MyModelType
{
	NONE, // emtpy, selfdefin
	TRIANGLE,
	QUAD,
	CIRCLE,
	CUBE,
	SPHERE,
	TERRAIN,
	AXIS,
	Num
};

class MyMeshFactory : public Singleton<MyMeshFactory>
{
	std::array<std::weak_ptr<MyMesh>, static_cast<size_t>(MyMeshType::Num)> _meshes;

	std::shared_ptr<MyMesh> buildAxis();
	std::shared_ptr<MyMesh> buildTriangle();
	std::shared_ptr<MyMesh> buildQuad();
	std::shared_ptr<MyMesh> buildCube();
	std::shared_ptr<MyMesh> buildCircle();
	std::shared_ptr<MyMesh> buildSphere();
	std::shared_ptr<MyMesh> buildTerrain();

public:
	std::shared_ptr<MyMesh> buildMesh(MyMeshType type);
};

class MyModelFactory : public Singleton<MyModelFactory>
{
	MyModel *buildAxis(std::string_view name);
	MyModel *buildTriangle(std::string_view name);
	MyModel *buildQuad(std::string_view name);
	MyModel *buildCube(std::string_view name);
	MyModel *buildCircle(std::string_view name);
	MyModel *buildSphere(std::string_view name);

public:
	std::unique_ptr<MyModel> buildModel(std::string_view name, MyModelType type = MyModelType::NONE);
};

class MyModelLoader : public ManualResourceLoader, public Singleton<MyModelLoader>
{
public:
	~MyModelLoader() {}
	void loadResource(Resource *resource);
};