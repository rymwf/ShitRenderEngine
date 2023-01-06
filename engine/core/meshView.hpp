#pragma once
#include "mesh.hpp"
#include "renderable.hpp"

class MeshView : public Renderable
{
	// SceneManager *_creator;
	Mesh *_mesh;

	Skin *_skin{};

	void initialise();

public:
	MeshView(SceneNode *parent, Mesh *mesh);
	MeshView(SceneNode *parent, Mesh *mesh, std::string_view name);
	MeshView(MeshView const &other);

	void prepare();

	void setMorphWeights(std::span<float> weights);

	NODISCARD Component *clone() override
	{
		return new MeshView(*this);
	}

	Mesh *getMesh() const { return _mesh; }

	constexpr BoundingVolume const &getBoundingVolume() const { return _boundingVolume; }

	void recordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex) override;

	constexpr void setSkin(Skin *skin)
	{
		_skin = skin;
	}
	constexpr Skin *getSkin() const { return _skin; }
};