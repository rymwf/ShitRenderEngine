#pragma once
#include "prerequisites.hpp"
#include "material.hpp"

// all renderables in group use the same material
struct MaterialPtrComp
{
	bool operator()(Material const *lhs, Material const *rhs) const
	{
		if (lhs->getPriority() == rhs->getPriority())
			return lhs->getId() < rhs->getId();
		return lhs->getPriority() == rhs->getPriority();
	};
};

/**
 * @brief render steps
 * 1. update all renderables
 * 2. record commandbuffers
 * 3. submit commandbuffers
 *
 */
//class RenderQueue
//{

//public:
//	RenderQueue(RenderQueueGroup *renderQueueGroup, RenderQueueType queueType);

//	void addPrimitiveView(PrimitiveView *primitiveView);
//	void removePrimitiveView(PrimitiveView *primitiveView);

//	void addPrimitiveViews(std::span<PrimitiveView *> primitiveViews);
//	void removePrimitiveViews(std::span<PrimitiveView *> primitiveViews);

//	void clear();

//private:
//	RenderQueueGroup *_renderQueueGroup;
//	RenderQueueType _renderQueueType;

//	mutable bool _needRecord = true;

//	std::map<Material *,
//			 std::unordered_map<GraphicPipelineSpec,
//								std::vector<PrimitiveView *>, GraphicPipelineSpecHash>,
//			 MaterialPtrComp>
//		_primitiveViewGroups;

//	void updateGPUData(uint32_t frameIndex);

//	void recordCommandBuffer(uint32_t frameIndex,
//							 bool noPipeline = false,
//							 MaterialDataBlock *materialDataBlock = nullptr);
//};

class RenderQueueGroup
{
	using PrimitiveContainer = std::map<Material *, std::unordered_map<GraphicPipelineSpec, std::vector<PrimitiveView *>, GraphicPipelineSpecHash>, MaterialPtrComp>;

	PrimitiveContainer _primitiveGroups[(size_t)RenderQueueType::Num];

	Camera *_camera;

	std::vector<Light *> _lights;

	std::vector<Shit::CommandBuffer *> _framePrimaryCommandBuffers;

	//std::vector<Shit::Semaphore *> _renderFinishedSemaphores;
	std::vector<Shit::Fence*> _fences;

	void renderShadow(uint32_t frameIndex);

	void draw(RenderQueueType queueType, Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex, Material *material = nullptr);

	void drawShadow(Shit::CommandBuffer *cmdBuffer, Light *light, uint32_t frameIndex);
	void drawBackground(uint32_t frameIndex);
	void drawGeometry(uint32_t frameIndex);

	void recordCommandBuffer(
		uint32_t frameIndex, bool drawShadow = false, MaterialDataBlock *materialDataBlock = nullptr);

	void submit(uint32_t frameIndex);

public:
	RenderQueueGroup(Camera *camera);

	void preRender(uint32_t frameIndex);
	void render(uint32_t frameIndex);
	void postRender(uint32_t frameIndex);

	//constexpr RenderQueue *getRenderQueue(RenderQueueType renderQueueType) const
	//{
	//	return _renderQueues.at((size_t)renderQueueType).get();
	//}

	void addRenderables(std::span<Renderable *> renderables);
	void removeRenderables(std::span<Renderable *> renderables);
	void clear();

	void setLights(std::span<Light *> lights);
};
