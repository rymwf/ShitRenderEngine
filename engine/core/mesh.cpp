#include "mesh.hpp"
#include "material.hpp"
#include "buffer.hpp"
#include "root.hpp"

Mesh::Mesh(MeshManager *creator, std::string_view name, std::string_view group)
	: GPUResource(creator, name, group)
{
}
Mesh::Mesh(MeshManager *creator, std::string_view group) : GPUResource(creator, group)
{
	_name = "mesh" + std::to_string((int)getId());
}
Primitive *Mesh::addPrimitive(Shit::PrimitiveTopology topology)
{
	return _primitives.emplace_back(std::make_unique<Primitive>(this, topology)).get();
}
void Mesh::init()
{
	int i = 0;
	for (auto &&e : _primitives)
	{
		if (e->position->minValues.empty() || e->position->maxValues.empty())
		{
			LOG("warning, primitive ", i, " donot have min max values")
		}
		else
		{
			_boundingVolume.box.merge({e->position->maxValues[0], e->position->maxValues[1], e->position->maxValues[2]});
			_boundingVolume.box.merge({e->position->minValues[0], e->position->minValues[1], e->position->minValues[2]});
		}
		++i;
	}


}
//===========================================
Mesh *MeshManager ::createMesh(std::string_view name, std::string_view group)
{
	Mesh* mesh;
	if (name.empty())
		mesh = new Mesh(this, group);
	else
		mesh = new Mesh(this, name, group);
	add(mesh);
	return mesh;
}