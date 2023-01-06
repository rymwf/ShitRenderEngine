#pragma once
#include "prerequisites.hpp"
#include "timer.hpp"
#include "screen.hpp"

class Root
{
	static const int MAX_FRAMES_IN_FLIGHT;
	Shit::Device *_device;

	Shit::RenderSystem *_renderSystem;
	Shit::RendererVersion _rendererVersion;

	Shit::Queue *_graphicsQueue;
	Shit::Queue *_presentQueue;
	Shit::Queue *_transferQueue;
	Shit::Queue *_computeQueue;

	Shit::CommandPool *_shortLiveCommandPool;
	Shit::CommandPool *_longLiveCommandPool;

	std::unique_ptr<Screen> _screen;

	// only include gpu resource managers
	std::unique_ptr<MeshManager> _meshManager;
	std::unique_ptr<BufferManager> _bufferManager;
	std::unique_ptr<TextureManager> _textureManager;
	std::unique_ptr<CompositorManager> _compositorManager;
	// std::unique_ptr<RenderableManager> _renderableManager;
	std::unique_ptr<MaterialDataBlockManager> _materialDataBlockManager;
	std::unique_ptr<DescriptorManager> _descriptorManager;

	std::unique_ptr<SceneManager> _sceneManager;

	uint32_t _currentFrame{};
	std::vector<Shit::Semaphore *> _imageAvailableSemaphores;
	std::vector<Shit::Semaphore *> _renderFinishedSemaphores;
	std::vector<Shit::Fence *> _inFlightFences;

	Timer _timer;
	uint64_t _frameDeltaTimeMs;

	//=====================================
	// arguments are imageIndex, commandbuffers need to submit
	Shit::Signal<std::vector<Shit::CommandBuffer *>(uint32_t),
				 Shit::all_values<std::vector<Shit::CommandBuffer *>>>
		_beforeOneFrameSignal;

	Shit::SampleCountFlagBits _sampleCount{Shit::SampleCountFlagBits::BIT_1};

	std::array<Shit::RenderPass *, (size_t)RenderPassType::Num> _renderPasses;

	Shit::DescriptorSetLayout *_descriptorSetLayoutCamera; // set 0
	Shit::DescriptorSetLayout *_descriptorSetLayoutNode;   // set 1
	Shit::DescriptorSetLayout *_descriptorSetLayoutLight;  // set 2
	Shit::DescriptorSetLayout *_descriptorSetLayoutEnv;	   // set 3
	Shit::DescriptorSetLayout *_descriptorSetLayoutSkin;   // set 4

	Shit::PipelineLayout* _commonPipelineLayout;

	void initDescriptorSetLayouts();

	void initRenderPasses();

	void initManagers();

	void createQueues();

	void createCommandPool();

	void createSyncObjects();

public:
	Root();
	~Root();

	void init(Shit::RendererVersion rendererVersion, const wchar_t *windowTitle, Shit::SampleCountFlagBits sampleCount);

	void addBeforeOneFrameCallback(const std::function<std::vector<Shit::CommandBuffer *>(uint32_t)> &func);

	void render();

	static inline Root *s_instance = 0;

	static Root *getSingletonPtr()
	{
		if (!s_instance)
			s_instance = new Root();
		return s_instance;
	}
	static Root &getSingleton()
	{
		if (!s_instance)
			s_instance = new Root();
		return *s_instance;
	}
	static void releaseSingleton()
	{
		if (s_instance)
		{
			delete s_instance;
			s_instance = 0;
		}
	}
	constexpr uint64_t getFrameDeltaTimeMs() const { return _frameDeltaTimeMs; }

	constexpr Shit::Device *getDevice() const { return _device; }
	constexpr Shit::RenderSystem *getRenderSystem() const { return _renderSystem; }
	Shit::RendererVersion getRendererVersion() const { return _renderSystem->GetCreateInfo()->version; }
	constexpr Shit::CommandPool *getCommandPool() const { return _longLiveCommandPool; }

	Screen *getScreen() const { return _screen.get(); }

	constexpr Shit::Queue *getGraphicsQueue() const { return _graphicsQueue; }
	constexpr Shit::Queue *getTransferQueue() const { return _transferQueue; }
	constexpr Shit::Queue *getComputeQueue() const { return _computeQueue; }
	constexpr Shit::Queue *getPresentQueue() const { return _presentQueue; }

	MeshManager *getMeshManager() const { return _meshManager.get(); }
	BufferManager *getBufferManager() const { return _bufferManager.get(); }
	SceneManager *getSceneManager() const { return _sceneManager.get(); }
	TextureManager *getTextureManager() const { return _textureManager.get(); }
	CompositorManager *getCompositorManager() const { return _compositorManager.get(); }
	// RenderableManager *getRenderableManager() const { return _renderableManager.get(); }
	MaterialDataBlockManager *getMaterialDataBlockManager() const { return _materialDataBlockManager.get(); }
	DescriptorManager *getDescriptorManager() const { return _descriptorManager.get(); }

	Shit::RenderPass *getRenderPass(RenderPassType type) const;

	void executeOneTimeCommand(const std::function<void(Shit::CommandBuffer *)> &func);
	void presentCommandBuffers(const std::vector<Shit::CommandBuffer *> &commandBuffers);

	void saveImage();

	constexpr Shit::SampleCountFlagBits getSampleCount() const { return _sampleCount; }

	constexpr Shit::DescriptorSetLayout const *getDescriptorSetLayoutCamera() const { return _descriptorSetLayoutCamera; }
	constexpr Shit::DescriptorSetLayout const *getDescriptorSetLayoutNode() const { return _descriptorSetLayoutNode; }
	constexpr Shit::DescriptorSetLayout const *getDescriptorSetLayoutLight() const { return _descriptorSetLayoutLight; }
	constexpr Shit::DescriptorSetLayout const *getDescriptorSetLayoutEnv() const { return _descriptorSetLayoutEnv; }
	constexpr Shit::DescriptorSetLayout const *getDescriptorSetLayoutSkin() const { return _descriptorSetLayoutSkin; }
	constexpr Shit::PipelineLayout const *getCommonPipelineLayout() const { return _commonPipelineLayout; }
};