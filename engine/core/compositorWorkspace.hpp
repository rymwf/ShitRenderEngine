/**
 * @file compositorWorkspace.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once
#include "compositor.hpp"

//===================================================================
class CompositorWorkspace : public IdObject<CompositorWorkspace>
{
	friend class CompositorNode;
	friend class CNComposite;
	friend class CompositorManager;

	static Shader *s_vertShader;

	std::string _mainCode;

	CompositorGlobalPara _globalPara;

	CompositorManager *_creator;

	std::string _name;
	CompositorWorkspaceDescription _desc;

	std::unordered_map<size_t, size_t> _routeIndexMap;

	std::vector<std::unique_ptr<CompositorNode>> _compositorNodes;
	CNComposite *_outputCompoite{};

	DescriptorSetData *_descriptorSetData{};

	Buffer *_uboBuffer;

	Shit::Pipeline *_pipeline;
	//================
	uint32_t _imageCount;

	bool _needRebuild{true}; //if true call outputcomposite to rebuild pipeline

	std::vector<Shit::CommandBuffer *> _commandBuffers;
	std::vector<bool> _needUpdateCommandBuffers{true};

	void validateChannelRoute(const ChannelRoute &route);
	void validateDescription();

	void createPipeline();
	void createDescriptorSets();

	void destroyOutDatedResources();

	static void initVertexShader();

	size_t channelRouteHash(const ChannelRoute &route) const;

	void connectRoute(const ChannelRoute &route);

	size_t getChannelRouteIndex(const ChannelRoute &route) const
	{
		return _routeIndexMap.at(channelRouteHash(route));
	}

	void updateUBOBuffer(uint32_t frameIndex);

	void recordCommandBuffer(uint32_t frameIndex);

	void init();

	constexpr void needRebuild() { _needRebuild = true; }

	constexpr void needUpdateCommandBuffer(uint32_t frameIndex) { _needUpdateCommandBuffers[frameIndex] = true; }
	constexpr void needUpdateAllCommandBuffers()
	{
		_needUpdateCommandBuffers.assign(_needUpdateCommandBuffers.size(), true);
	}

public:
	CompositorWorkspace(CompositorManager *creator, const CompositorWorkspaceDescription &desc);
	CompositorWorkspace(CompositorManager *creator, const CompositorWorkspaceDescription &desc, std::string_view name);
	~CompositorWorkspace() {}

	constexpr std::string &getMainCode() { return _mainCode; };
	constexpr CompositorGlobalPara &getGlobalPara() { return _globalPara; }

	void build();
	void update(uint32_t frameIndex);

	CompositorNode *createCompositorNode(const CompositorNodeDescription &desc);
	CompositorNode *getCompositorNodeByIndex(uint32_t index) const
	{
		return _compositorNodes.at(index).get();
	}
	constexpr std::span<std::unique_ptr<CompositorNode>> getCompositorNodes() { return _compositorNodes; }

	constexpr std::string_view getName() const { return _name; }

	void connect(const ChannelRoute &route);
	//void disconnectByIndex(uint32_t index);
	void disconnect(const ChannelRoute &route);

	constexpr Shit::CommandBuffer *getCommandBuffer(uint32_t frameIndex) const
	{
		return _commandBuffers.at(frameIndex);
	}

	constexpr DescriptorSetData *getDescriptorSetData() const { return _descriptorSetData; }

	void setUBOBufferData(size_t offset, size_t size, void const *data);
	void setUBOBufferData(size_t offset, size_t size, int val);

	BufferView allocateUBOBuffer(size_t stride, size_t count, void const *data);
	BufferView allocateUBOBuffer(size_t stride, size_t count, int val = 0);
};

//=======================================
class CompositorManager //: public Singleton<CompositorManager>
{
	std::unordered_map<std::string, std::unique_ptr<CompositorWorkspace>> _compositorWorkspaces;

	Shit::RenderPass *_renderPass;
	std::vector<Shit::Framebuffer *> _framebuffersForSwapchain;

	void initRenderPass();
	void initFramebuffers();

public:
	CompositorManager();
	void recreateSwapchainCallback();

	CompositorWorkspace *createCompositorWorkspace(const CompositorWorkspaceDescription &desc, std::string_view name = "");
	CompositorWorkspace *getCompositorWorkspace(std::string_view name) const;
	bool contains(std::string_view name) const;
	constexpr Shit::RenderPass *getRenderPass() const { return _renderPass; }
	constexpr Shit::Framebuffer *getFramebuffer(uint32_t frameIndex) const { return _framebuffersForSwapchain.at(frameIndex); }
};

