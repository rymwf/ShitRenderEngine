#pragma once
#include "resourceManager.hpp"
#include "boundingVolume.hpp"
#include "animation.hpp"
#include "model.hpp"

enum class ModelType
{
	GLTF,
	MYMODEL,
};

class ModelResource : public Resource
{
	friend class Model;

public:
	ModelResource(std::string_view name,
				  std::string_view groupName,
				  bool manuallyLoad = false,
				  ManualResourceLoader *loader = nullptr,
				  size_t size = 0,
				  void const *data = nullptr);

	virtual ~ModelResource() {}

	void addMesh(Mesh *mesh);

	// return model 
	Model *getModel() const { return _model.get(); }

	void addMateralDataBlock(MaterialDataBlock *materialDatablock);
	void addImage(Image *image);
	void addTexture(Texture *texture);

	constexpr Mesh *getMeshByIndex(uint32_t index) const { return _meshes.at(index); }
	constexpr Image *getImageByIndex(uint32_t index) const { return _images.at(index); }
	constexpr Texture *getTextureByIndex(uint32_t index) const { return _textures.at(index); }
	constexpr MaterialDataBlock *getMaterialDataBlockByIndex(uint32_t index) const { return _materialDataBlocks.at(index); }

	void loadAllImages();

	AnimationClip *createAnimationClip(std::string_view name = {});
	AnimationClip *getAnimationClipByIndex(size_t index) const { return _animationClips.at(index).get(); }
	constexpr size_t getAnimationClipCount() const { return _animationClips.size(); }

	constexpr BoundingVolume const &getBoundingVolume() const
	{
		return _boundingVolume;
	}

protected:
	friend class ModelCreator;

	BoundingVolume _boundingVolume{};

	std::unique_ptr<Model> _model;

	std::vector<Mesh *> _meshes;

	std::vector<std::unique_ptr<AnimationClip>> _animationClips;

	std::vector<MaterialDataBlock *> _materialDataBlocks;
	std::vector<Texture *> _textures;
	std::vector<Image *> _images;

	BoundingVolume const &getBoundingVolume(SceneNode const *node) const;

	void initBoundingVolume();

	void postLoadImpl() override;
};

struct ModelResourceCreator
{
	/**
	 * @brief
	 *
	 * @param name
	 * @param parameterMap
	 * key value
	 * "ModelType","MyModel","GLTF"
	 *
	 * @param group
	 * @param manullyLoad
	 * @param loader
	 * @param size
	 * @param data
	 * @return Resource*
	 */
	Resource *operator()(
		std::string_view name,
		const ParameterMap *parameterMap,
		std::string_view group,
		bool manullyLoad,
		ManualResourceLoader *loader,
		size_t size,
		void const *data) const
	{
		return new ModelResource(name, group, manullyLoad, loader, size, data);
	}
};