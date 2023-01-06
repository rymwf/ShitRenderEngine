#include "root.hpp"
#include "mesh.hpp"
#include "buffer.hpp"
#include "archiveManager.hpp"
#include "sceneManager.hpp"
#include "configFile.hpp"
#include "shader.hpp"
#include "compositorWorkspace.hpp"
#include "texture.hpp"
#include "image.hpp"
#include "descriptorManager.hpp"
#include "resourceGroup.hpp"
#include "modelResource.hpp"
#include "pipeline.hpp"
#include "viewLayer.hpp"

const int Root::MAX_FRAMES_IN_FLIGHT = 2;
Root::Root()
{
}
Root::~Root() {}
void Root::init(Shit::RendererVersion rendererVersion, const wchar_t *windowTitle, Shit::SampleCountFlagBits sampleCount)
{
	_sampleCount = sampleCount;
	_rendererVersion = rendererVersion;

	_renderSystem = LoadRenderSystem(
		Shit::RenderSystemCreateInfo{
			_rendererVersion,
#ifndef NDEBUG
			Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT,
#endif
		});

	_screen = std::make_unique<Screen>(windowTitle);

	Shit::DeviceCreateInfo deviceCreateInfo{};
	if ((_rendererVersion & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::GL)
		deviceCreateInfo = {_screen->getWindow()};
	_device = _renderSystem->CreateDevice(deviceCreateInfo);

	_screen->createSwapchain();

	createQueues();
	createCommandPool();
	createSyncObjects();

	initRenderPasses();
	initDescriptorSetLayouts();
	initManagers();
}

void Root::initManagers()
{
	ConfigFile cf;
	cf.load(SHIT_SOURCE_DIR "/assets.cfg");
	auto &&settingSectionMap = cf.getSettingSectionMap();
	for (auto &&section : settingSectionMap)
	{
		LOG("===section===", section.first);
		for (auto &&e : section.second)
			LOG(e.first, e.second);
	}

	auto &&archiveMap = settingSectionMap.at("Archives");
	std::string str;
	for (auto &&e : archiveMap)
	{
		str = SHIT_SOURCE_DIR "/";
		str += e.second;
		auto arch = ArchiveManager::getSingleton().createOrRetrieve(e.first, str, ArchiveType::DIRECTORY);
		ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup()->addArchive(arch);
	}
	// create default empty archive
	auto arch = ArchiveManager::getSingleton().createOrRetrieve("", SHIT_OUTPUT_DIR, ArchiveType::DIRECTORY);
	ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup()->addArchive(arch);

	ResourceGroupManager::getSingleton().registerResourceCreator(ResourceType::IMAGE, ImageCreator());
	ResourceGroupManager::getSingleton().registerResourceCreator(ResourceType::SHADER, ShaderCreator());
	ResourceGroupManager::getSingleton().registerResourceCreator(ResourceType::MODEL, ModelResourceCreator());

	_descriptorManager = std::make_unique<DescriptorManager>();
	_meshManager = std::make_unique<MeshManager>();
	_bufferManager = std::make_unique<BufferManager>();
	_textureManager = std::make_unique<TextureManager>();
	_compositorManager = std::make_unique<CompositorManager>();
	_materialDataBlockManager = std::make_unique<MaterialDataBlockManager>();

	_sceneManager = std::make_unique<SceneManager>();
}
void Root::initDescriptorSetLayouts()
{
	Shit::DescriptorSetLayoutBinding bindings[]{
		// set0 binding 0
		{DESCRIPTOR_BINDING_CAMERA, Shit::DescriptorType::STORAGE_BUFFER, 1,
		 Shit::ShaderStageFlagBits::VERTEX_BIT | Shit::ShaderStageFlagBits::GEOMETRY_BIT | Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		// set1 binding 0
		{DESCRIPTOR_BINDING_NODE, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::VERTEX_BIT},
		// set2 binding 1
		{DESCRIPTOR_BINDING_SKIN, Shit::DescriptorType::STORAGE_BUFFER, 1, Shit::ShaderStageFlagBits::VERTEX_BIT},
		// set3 binding 2
		{DESCRIPTOR_BINDING_LIGHT, Shit::DescriptorType::STORAGE_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::GEOMETRY_BIT},
		{8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT}, // shadowmap 2d array
		{9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT}, // shadowmap cube
		// set 3 binding 1
		// {DESCRIPTOR_BINDING_MATERIAL_BUFFER, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		// {DESCRIPTOR_BINDING_MATERIAL_TEXTURE, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 8, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		// set4 prefiltered env map, brdf tex
		{10, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		{11, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
	};

	_descriptorSetLayoutCamera = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[0]});
	_descriptorSetLayoutNode = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[1]});
	_descriptorSetLayoutSkin = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[2]});
	_descriptorSetLayoutLight = _device->Create(Shit::DescriptorSetLayoutCreateInfo{3, &bindings[3]});
	_descriptorSetLayoutEnv = _device->Create(Shit::DescriptorSetLayoutCreateInfo{2, &bindings[6]});

	Shit::DescriptorSetLayout *setLayouts[]{
		_descriptorSetLayoutCamera,
		_descriptorSetLayoutNode,
		_descriptorSetLayoutSkin,
		_descriptorSetLayoutLight,
		_descriptorSetLayoutEnv,
	};
	_commonPipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)std::size(setLayouts), setLayouts});
}
void Root::addBeforeOneFrameCallback(const std::function<std::vector<Shit::CommandBuffer *>(uint32_t)> &func)
{
	_beforeOneFrameSignal.Connect(func);
}
void Root::createCommandPool()
{
	_shortLiveCommandPool = Root::getSingleton().getDevice()->Create(
		Shit::CommandPoolCreateInfo{
			Shit::CommandPoolCreateFlagBits::TRANSIENT_BIT |
				Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
			_graphicsQueue->GetFamilyIndex()});
	_longLiveCommandPool = _device->Create(
		Shit::CommandPoolCreateInfo{Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
									_graphicsQueue->GetFamilyIndex()});
}
void Root::createSyncObjects()
{
	_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		_imageAvailableSemaphores[i] = Root::getSingleton().getDevice()->Create(Shit::SemaphoreCreateInfo{});
		_renderFinishedSemaphores[i] = Root::getSingleton().getDevice()->Create(Shit::SemaphoreCreateInfo{});
		_inFlightFences[i] = Root::getSingleton().getDevice()->Create(Shit::FenceCreateInfo{Shit::FenceCreateFlagBits::SIGNALED_BIT});
	}
}
void Root::createQueues()
{
	// present queue
	auto presentQueueFamilyProprety = _device->GetPresentQueueFamilyProperty(_screen->getWindow());
	_presentQueue = _device->GetQueue(presentQueueFamilyProprety->index, 0);

	// graphics queue
	// 5
	auto graphicsQueueFamilyProperty = _device->GetQueueFamilyProperty(Shit::QueueFlagBits::GRAPHICS_BIT);
	if (!graphicsQueueFamilyProperty.has_value())
		THROW("failed to find a graphic queue");
	_graphicsQueue = _device->GetQueue(graphicsQueueFamilyProperty->index, 0);

	// transfer queue
	auto transferQueueFamilyProperty = _device->GetQueueFamilyProperty(Shit::QueueFlagBits::TRANSFER_BIT);
	if (!transferQueueFamilyProperty.has_value())
		THROW("failed to find a transfer queue");
	_transferQueue = _device->GetQueue(transferQueueFamilyProperty->index, 0);

	// compute queue
	auto computeQueueFamilyProperty = _device->GetQueueFamilyProperty(Shit::QueueFlagBits::COMPUTE_BIT);
	if (!computeQueueFamilyProperty.has_value())
		THROW("failed to find a compute queue");
	_computeQueue = _device->GetQueue(computeQueueFamilyProperty->index, 0);
}
void Root::executeOneTimeCommand(const std::function<void(Shit::CommandBuffer *)> &func)
{
	static auto queueFamilyProperty = _device->GetQueueFamilyProperty(
		Shit::QueueFlagBits::GRAPHICS_BIT | Shit::QueueFlagBits::TRANSFER_BIT | Shit::QueueFlagBits::COMPUTE_BIT);
	if (!queueFamilyProperty.has_value())
	{
		THROW("cannot find a grphics/transfer/compute queue ");
	}
	static auto queue = _device->GetQueue(queueFamilyProperty->index, 0);
	static auto pFence = _device->Create(Shit::FenceCreateInfo{});
	pFence->Reset();
	static auto commandPool = _device->Create(Shit::CommandPoolCreateInfo{
		Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
		queueFamilyProperty->index});
	static Shit::CommandBuffer *commandBuffer;
	if (!commandBuffer)
		commandPool->CreateCommandBuffers(
			Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, 1}, &commandBuffer);

	commandBuffer->Reset();
	commandBuffer->Begin(Shit::CommandBufferBeginInfo{Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
	func(commandBuffer);
	commandBuffer->End();

	Shit::SubmitInfo submitInfos[]{{0, nullptr, 1, &commandBuffer}};

	queue->Submit(submitInfos, pFence);
	pFence->WaitFor(UINT64_MAX);
	// queue->WaitIdle();
}
void Root::render()
{
	std::vector<Shit::CommandBuffer *> commandBuffers;
	_timer.Restart();
	while (_screen->getWindow()->PollEvents())
	{
		_inFlightFences[_currentFrame]->WaitFor(UINT64_MAX);

		// calc frame time
		static std::chrono::high_resolution_clock::time_point preTime = std::chrono::high_resolution_clock::now();
		static std::chrono::high_resolution_clock::time_point curTime;
		curTime = std::chrono::high_resolution_clock::now();
		_frameDeltaTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - preTime).count();
		preTime = curTime;

		uint32_t imageIndex{};
		Shit::GetNextImageInfo nextImageInfo{
			UINT64_MAX,
			_imageAvailableSemaphores[_currentFrame]};
		auto ret = _screen->getSwapchain()->GetNextImage(nextImageInfo, imageIndex);
		if (ret == Shit::Result::SHIT_ERROR_OUT_OF_DATE)
		{
			_presentQueue->WaitIdle();
			_graphicsQueue->WaitIdle();

			_screen->recreateSwapchain();

			continue;
		}
		else if (ret != Shit::Result::SUCCESS && ret != Shit::Result::SUBOPTIMAL)
		{
			THROW("failed to get next image");
		}
		_inFlightFences[_currentFrame]->Reset();

		auto ret2 = _beforeOneFrameSignal(imageIndex);
		commandBuffers.clear();
		for (auto &&e : ret2)
		{
			for (auto e2 : e)
				commandBuffers.emplace_back(e2);
		}

		std::vector<Shit::SubmitInfo> submitInfos{
			{1, &_imageAvailableSemaphores[_currentFrame],
			 (uint32_t)commandBuffers.size(),
			 commandBuffers.data(),
			 1, &_renderFinishedSemaphores[_currentFrame]}};
		_graphicsQueue->Submit(submitInfos, _inFlightFences[_currentFrame]);

		auto swapchain = _screen->getSwapchain();

		Shit::PresentInfo presentInfo{
			1, &_renderFinishedSemaphores[_currentFrame],
			1, &swapchain,
			&imageIndex};
		auto res = _presentQueue->Present(presentInfo);
		if (res == Shit::Result::SHIT_ERROR_OUT_OF_DATE || res == Shit::Result::SUBOPTIMAL)
		{
			_presentQueue->WaitIdle();
			_graphicsQueue->WaitIdle();
			_screen->recreateSwapchain();
		}
		else if (res != Shit::Result::SUCCESS)
		{
			THROW("failed to present swapchain image");
		}
		// if (_takeScreenShot)
		// {
		// 	takeScreenshot(_device, _swapchain->GetImageByIndex(imageIndex), Shit::ImageLayout::PRESENT_SRC);
		// 	_takeScreenShot = false;
		// }
		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	_device->WaitIdle();
}
Shit::RenderPass *Root::getRenderPass(RenderPassType type) const
{
	return _renderPasses.at((size_t)type);
}
void Root::initRenderPasses()
{
	{
		// BackGround renderpass not used
		Shit::AttachmentDescription attachmentDescriptions[]{
			{Shit::Format::R8G8B8A8_SRGB,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}, // color
														   //{Shit::Format::R8G8B8A8_SRGB,
														   // Shit::SampleCountFlagBits::BIT_1,
														   // Shit::AttachmentLoadOp::CLEAR,
														   // Shit::AttachmentStoreOp::DONT_CARE,
														   // Shit::AttachmentLoadOp::DONT_CARE,
														   // Shit::AttachmentStoreOp::DONT_CARE,
														   // Shit::ImageLayout::UNDEFINED,
														   // Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL}, // color
		};
		std::vector<Shit::AttachmentReference> colorAttachmentRefs{
			{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		};
		// std::vector<Shit::AttachmentReference> resolveAttachmentRefs{
		// };

		Shit::SubpassDescription subpasses[]{
			{
				Shit::PipelineBindPoint::GRAPHICS,
				0,
				nullptr,
				(uint32_t)colorAttachmentRefs.size(),
				colorAttachmentRefs.data(),
				// resolveAttachmentRefs.data(),
			},
		};
		_renderPasses[(size_t)RenderPassType::BACKGROUND] = _device->Create(Shit::RenderPassCreateInfo{
			(uint32_t)ranges::size(attachmentDescriptions),
			attachmentDescriptions,
			(uint32_t)ranges::size(subpasses),
			subpasses,
		});
	}
	//==========================================
	// renderpass Forward
	{
		Shit::AttachmentDescription attachmentDescriptions[]{
			{Shit::Format::R8G8B8A8_SRGB,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::LOAD,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// depth
			{Shit::Format::D24_UNORM_S8_UINT,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// resolve
			//{Shit::Format::R8G8B8A8_SRGB,
			//  Shit::SampleCountFlagBits::BIT_1,
			//  Shit::AttachmentLoadOp::CLEAR,
			//  Shit::AttachmentStoreOp::DONT_CARE,
			//  Shit::AttachmentLoadOp::DONT_CARE,
			//  Shit::AttachmentStoreOp::DONT_CARE,
			//  Shit::ImageLayout::UNDEFINED,
			//  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}, // color
		};
		std::vector<Shit::AttachmentReference> colorAttachmentRefs{
			{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		};
		Shit::AttachmentReference depthStencilAttachmentRef{
			1, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
		////std::vector<Shit::AttachmentReference> resolveAttachmentRefs{
		////	{2, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		//};

		Shit::SubpassDescription subpasses[]{
			{Shit::PipelineBindPoint::GRAPHICS,
			 0,
			 nullptr,
			 (uint32_t)colorAttachmentRefs.size(),
			 colorAttachmentRefs.data(),
			 0,
			 &depthStencilAttachmentRef},
		};
		_renderPasses[(size_t)RenderPassType::FORWARD] = _device->Create(Shit::RenderPassCreateInfo{
			(uint32_t)ranges::size(attachmentDescriptions),
			attachmentDescriptions,
			(uint32_t)ranges::size(subpasses),
			subpasses,
		});
	}
	{
		// render pass deferred pass
		std::vector<Shit::AttachmentDescription> attachmentDesc{
			// read color
			{Shit::Format::R8G8B8A8_SRGB,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::LOAD,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// write geometry pass attachments
			// postion
			//{Shit::Format::R8G8B8A8_SNORM,
			{Shit::Format::R16G16B16A16_SFLOAT,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// albedo
			{Shit::Format::R16G16B16A16_UNORM,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// normal
			{Shit::Format::R16G16B16A16_SNORM,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// MetallicRoughnessAO
			{Shit::Format::R16G16B16A16_UNORM,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// Emission
			{Shit::Format::R16G16B16A16_UNORM,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			// depth
			{Shit::Format::D24_UNORM_S8_UINT,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
		};
		Shit::AttachmentReference colorAttachments[]{
			{1, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
			{2, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
			{3, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
			{4, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
			{5, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		};
		Shit::AttachmentReference depthAttachment{6, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		Shit::AttachmentReference inputAttachments1[]{
			{1, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			{2, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			{3, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			{4, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
			{5, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
		};
		Shit::AttachmentReference colorAttachment1{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

		Shit::SubpassDescription subpassDescs[]{
			{Shit::PipelineBindPoint::GRAPHICS,
			 0,
			 0,
			 (uint32_t)std::size(colorAttachments),
			 colorAttachments,
			 0,
			 &depthAttachment},
			{Shit::PipelineBindPoint::GRAPHICS,
			 (uint32_t)std::size(inputAttachments1),
			 inputAttachments1,
			 1,
			 &colorAttachment1},
		};
		Shit::SubpassDependency subpassDependencies[]{
			{ST_SUBPASS_EXTERNAL,
			 0,
			 Shit::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
			 Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT |
				 Shit::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,
			 {},
			 Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
				 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
			{0,
			 1,
			 Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT |
				 Shit::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,
			 Shit::PipelineStageFlagBits::EARLY_FRAGMENT_TESTS_BIT |
				 Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
			 Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
				 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			 Shit::AccessFlagBits::INPUT_ATTACHMENT_READ_BIT |
				 Shit::AccessFlagBits::SHADER_READ_BIT},
			{1,
			 ST_SUBPASS_EXTERNAL,
			 Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
			 Shit::PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT,
			 Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT}};

		_renderPasses[(size_t)RenderPassType::DEFERRED] = _device->Create(
			Shit::RenderPassCreateInfo{
				(uint32_t)attachmentDesc.size(),
				attachmentDesc.data(),
				std::size(subpassDescs),
				subpassDescs,
				std::size(subpassDependencies),
				subpassDependencies,
			});
	}
	{
		// shadow pass
		Shit::AttachmentDescription attachmentDescriptions[]{
			// depth
			{Shit::Format::D24_UNORM_S8_UINT,
			 Shit::SampleCountFlagBits::BIT_1,
			 Shit::AttachmentLoadOp::CLEAR,
			 Shit::AttachmentStoreOp::STORE,
			 Shit::AttachmentLoadOp::DONT_CARE,
			 Shit::AttachmentStoreOp::DONT_CARE,
			 Shit::ImageLayout::UNDEFINED,
			 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
		};
		Shit::AttachmentReference depthStencilAttachmentRef{
			0, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		Shit::SubpassDescription subpasses[]{
			{Shit::PipelineBindPoint::GRAPHICS,
			 0,
			 0,
			 0,
			 0,
			 0,
			 &depthStencilAttachmentRef},
		};
		_renderPasses[(size_t)RenderPassType::SHADOW] = _device->Create(
			Shit::RenderPassCreateInfo{
				(uint32_t)ranges::size(attachmentDescriptions),
				attachmentDescriptions,
				(uint32_t)ranges::size(subpasses),
				subpasses,
			});
	}
}