#pragma once
#include "gpuResource.hpp"
#include "boundingVolume.hpp"
#include "primitive.hpp"

class Mesh : public GPUResource
{
	friend class MeshView;
	std::vector<std::unique_ptr<Primitive>> _primitives;
	std::vector<double> _morphWeights;

	BoundingVolume _boundingVolume;

public:
	template <typename T>
	void addVertexAttributes(const std::vector<T> &attributes);
	constexpr void setWeights(const std::vector<double> &weights)
	{
		_morphWeights = weights;
	}
	Mesh(MeshManager *creator, std::string_view name, std::string_view group);
	Mesh(MeshManager *creator, std::string_view group);

	Primitive *addPrimitive(Shit::PrimitiveTopology topology);

	/**
	 * @brief initialize bounding volume etc.
	 *
	 */
	void init();

	size_t size() const { return 0; }
};

class MeshManager : public GPUResourceManager
{
public:
	MeshManager() {}

	Mesh *createMesh(std::string_view name = {}, std::string_view group = DEFAULT_GROUP_NAME);
};