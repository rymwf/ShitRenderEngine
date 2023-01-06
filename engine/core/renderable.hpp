#pragma once
#include "prerequisites.hpp"
#include "component.hpp"
#include "primitive.hpp"
#include "boundingVolume.hpp"

enum class RenderableType
{
	MESHVIEW,
};

//enum class RenderQueueMode
//{
//	BACKGROUND,
//	GEOMETRY,
//	ALPHATEST,
//	GEOMETRY_LAST,
//	TRANSPARENT,
//	OVERLAY
//};
//enum class RenderType
//{
//	OPAQUE,					 //: most of the shaders (Normal, Self Illuminated, Reflective, terrain shaders).
//	TRANSPARENT,			 //: most semitransparent shaders (Transparent, Particle, Font, terrain additive pass shaders).
//	TRANSPARENT_CUTOUT,		 //: masked transparency shaders (Transparent Cutout, two pass vegetation shaders).
//	BACKGROUND,				 //: Skybox shaders.
//	OVERLAY,				 //: Halo, Flare shaders.
//	TREE_OPAQUE,			 //: terrain engine tree bark.
//	TREE_TRANSPARENT_CUTOUT, //: terrain engine tree leaves.
//	TREE_BILLBOARD,			 //: terrain engine billboarded trees.
//	GRASS,					 //: terrain engine grass.
//	GRASS_BILLBOARD,		 //: terrain engine billboarded grass.
//};

class Renderable : public Component //public IdObject<Renderable>
{
public:
	enum class Event
	{
		TRANSFORMATION,
	};

	Renderable(SceneNode *parent, RenderableType renderableType);
	Renderable(SceneNode *parent, RenderableType renderableType, std::string_view name);
	Renderable(Renderable const &other);
	//Renderable &operator=(Renderable const &other);

	virtual ~Renderable() {}

	//virtual Shit::PrimitiveTopology getTopology() const = 0;
	void getAllPrimitiveViews(std::vector<PrimitiveView *> &primitiveViews);

	void setCurMaterialDataBlock(MaterialDataBlock *materialDataBlock)
	{
		for (auto &&p : _primitiveViews)
		{
			p->setMaterialDataBlock(materialDataBlock);
		}
	}
	void resetToDefaultMaterialDataBlock()
	{
		for (auto &&p : _primitiveViews)
		{
			p->resetToDefaultMaterialDataBlock();
		}
	}
	MaterialDataBlock *getCurMaterialDataBlock() const
	{
		return _primitiveViews.at(0)->getMaterialDataBlock();
	}

	bool isRenderInLayer(uint32_t layerIndex) const
	{
		if (layerIndex > 32)
		{
			THROW("layer index should be in range[0, 32], current layerIndex", layerIndex)
		}
		return (_renderLayerMask >> layerIndex) & 1;
	}

	constexpr int64_t getRenderInLayerFlags() const { return _renderLayerMask; }

	constexpr void enableRenderLayer(uint32_t layerIndex)
	{
		_renderLayerMask |= (1 << layerIndex);
	}
	constexpr void disableRenderLayer(uint32_t layerIndex)
	{
		_renderLayerMask &= (~(1 << layerIndex));
	}
	/**
	 * @brief allocate all the gpu memories, not in main loop
	 * 
	 */
	void prepare() override;

	void onNodeUpdated() override;

	/**
	 * @brief called by renderqueue
	 * 
	 * @param frameIndex 
	 */
	void updateGPUData(uint32_t frameIndex);

	/**
	 * @brief bind node ubo
	 * 
	 * @param cmdBuffer 
	 * @param frameIndex 
	 */
	virtual void recordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex);

	//after submit
	//virtual void postRender(uint32_t frameIndex);

	constexpr void setRenderPriority(int priority) { _renderPriority = priority; }

	void addSignalListener(Shit::Slot<void(Renderable *, Event)> slot)
	{
		_signal.Connect(slot);
	}
	void removeSignalListener(Shit::Slot<void(Renderable *, Event)> slot)
	{
		_signal.Disconnect(slot);
	}

	constexpr BoundingVolume const &getBoundingVolume() const { return _boundingVolume; }

	constexpr RenderableType getRenderableType() const { return _renderableType; }

protected:
	RenderableType _renderableType;

	BoundingVolume _boundingVolume{};

	std::vector<std::unique_ptr<PrimitiveView>> _primitiveViews;

	// bool _visible{true};
	//MaterialDataBlock *_defaultMaterialDataBlock;
	//MaterialDataBlock *_curMaterialDataBlock;

	std::vector<std::pair<Light *, bool>> _lightMasks;

	bool _enable{true};
	int _renderPriority{0};

	uint32_t _renderLayerMask{1}; //only render in first layer

	bool _receiveShadow{false};
	bool _castShadow{false};
	bool _staticShadowCaster{false};

	//Shit::Signal<void(Renderable *)> _materialChangedSignal;
	Shit::Signal<void(Renderable *, Event)> _signal;

	virtual void onBecameVisible() {}
	virtual void onBecameInvisible() {}
};
