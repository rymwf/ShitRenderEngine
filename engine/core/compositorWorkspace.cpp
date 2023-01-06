/**
 * @file compositorWorkspace.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "compositorWorkspace.hpp"
#include "root.hpp"
#include "descriptorManager.hpp"
#include "shader.hpp"
#include "resourceGroup.hpp"

Shader *CompositorWorkspace ::s_vertShader = nullptr;
void CompositorWorkspace::initVertexShader()
{
	//create vertexShader
	if (!s_vertShader)
	{
		//default vertex shader
		static std::string vertCode = "\
#version 460\n\
const vec2 inPos[3] = {{-1, -1}, {3, -1},{-1, 3}};\n\
#ifdef VULKAN\n\
#define VertexIndex gl_VertexIndex\n\
const vec2 inTexcoord[3] = {{0, 1}, {2, 1},{0, -1}};\n\
#else\n\
#define VertexIndex gl_VertexID\n\
const vec2 inTexcoord[3] = {{0, 0}, {2, 0},{0, 2}};\n\
#endif\n\
struct VS_OUT{\n\
    vec2 uv;\n\
};\n\
layout(location=0)out VS_OUT vs_out;\n\
void main() { \n\
	gl_Position = vec4(inPos[VertexIndex], 0, 1);\n\
	vs_out.uv=inTexcoord[VertexIndex];\n\
}";
		static ParameterMap para{
			{"ShadingLanguage", "GLSL"},
			{"ShaderStage", "VERT"},
		};

		std::string vertShaderName = "compositor.vert";
		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
		s_vertShader = static_cast<Shader *>(grp->createResource(
			ResourceDeclaration{
				vertShaderName,
				ResourceType::SHADER,
				nullptr,
				para},
			ranges::size(vertCode), ranges::cdata(vertCode)));
#ifndef NDEBUG
		s_vertShader->save();
#endif
	}
}
CompositorWorkspace::CompositorWorkspace(CompositorManager *creator, const CompositorWorkspaceDescription &desc)
	: _creator(creator), _desc(desc)
{
	static int i = 0;
	_name = "compositor_workspace" + std::to_string(i++);
	init();
}
CompositorWorkspace::CompositorWorkspace(
	CompositorManager *creator,
	const CompositorWorkspaceDescription &desc,
	std::string_view name)
	: _creator(creator), _desc(desc), _name(name)
{
	init();
}
void CompositorWorkspace::init()
{
	initVertexShader();

	//create framebuffers
	auto imageCount = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();
	_commandBuffers.resize(imageCount);

	_needUpdateCommandBuffers.resize(imageCount, true);

	Root::getSingleton().getCommandPool()->CreateCommandBuffers(
		Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, imageCount}, _commandBuffers.data());

	//create uboBuffer
	_uboBuffer = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT |
				Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
			Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			true},
		_name);

	//create nodes
	for (auto &&e : _desc.compositorNodes)
	{
		createCompositorNode(e);
	}
	//connect routes
	for (size_t i = 0, l = _desc.channelRoutes.size(); i < l; ++i)
	{
		_routeIndexMap.emplace(channelRouteHash(_desc.channelRoutes[i]), i);
		connectRoute(_desc.channelRoutes[i]);
	}
}
void CompositorWorkspace::validateChannelRoute(const ChannelRoute &route)
{
}
void CompositorWorkspace::validateDescription()
{
}
CompositorNode *CompositorWorkspace::createCompositorNode(const CompositorNodeDescription &desc)
{
	CompositorNode *ret;
	std::visit(Shit::overloaded{
				   [&](const CNRenderLayerDescription &v)
				   {
					   ret = new CNRenderLayer(this, v);
				   },
				   [&](const CNCompositeDescription &v)
				   {
					   ret = _outputCompoite = new CNComposite(this, v);
				   },
				   [&](const CNImageDescription &v)
				   {
					   ret = new CNImage(this, v);
				   },
				   [&](const CNMathDescription &v)
				   {
					   ret = new CNMath(this, v);
				   },
				   [&](const CNColorDescription &v)
				   {
					   ret = new CNColor(this, v);
				   },
				   [&](const CNColorMixDescription &v)
				   {
					   ret = new CNColorMix(this, v);
				   },
				   [&](const CNControlColorDescription &v)
				   {
					   ret = new CNControlColor(this, v);
				   },
				   [&](const CNControlValueDescription &v)
				   {
					   ret = new CNControlValue(this, v);
				   },
				   [](auto &&)
				   {
					   THROW("failed to find suitable compositor node class ");
				   },
			   },
			   desc);
	return _compositorNodes.emplace_back(std::unique_ptr<CompositorNode>(ret)).get();
}
size_t CompositorWorkspace::channelRouteHash(const ChannelRoute &route) const
{
	size_t ret = std::hash<uint32_t>{}(route.outputNodeIndex);
	hashCombine(ret, std::hash<uint32_t>{}(route.outputChannelIndex));
	hashCombine(ret, std::hash<uint32_t>{}(route.inputNodeIndex));
	hashCombine(ret, std::hash<uint32_t>{}(route.inputChannelIndex));
	return ret;
}
void CompositorWorkspace::connectRoute(const ChannelRoute &route)
{
	_compositorNodes[route.outputNodeIndex]->connectTo(
		route.outputChannelIndex,
		_compositorNodes[route.inputNodeIndex].get(),
		route.inputChannelIndex);
}
void CompositorWorkspace::connect(const ChannelRoute &route)
{
	_desc.channelRoutes.emplace_back(route);
	_routeIndexMap.emplace(channelRouteHash(route), _desc.channelRoutes.size() - 1);
	connectRoute(route);
}
//void CompositorWorkspace::disconnectByIndex(uint32_t index)
//{
//	auto it = _desc.channelRoutes.begin() + index;
//	_compositorNodes[it->outputNodeIndex]->discconectFrom(it->outputChannelIndex,
//														  _compositorNodes[it->inputNodeIndex].get(),
//														  it->inputChannelIndex);
//	_routeIndexMap.erase(channelRouteHash(*it));
//	_desc.channelRoutes.erase(it);
//}
void CompositorWorkspace::disconnect(const ChannelRoute &route)
{
	_compositorNodes[route.outputNodeIndex]->discconectFrom(
		route.outputChannelIndex,
		_compositorNodes[route.inputNodeIndex].get(),
		route.inputChannelIndex);
	auto h = channelRouteHash(route);
	_desc.channelRoutes.erase(_desc.channelRoutes.begin() + _routeIndexMap[h]);
	_routeIndexMap.erase(h);
}
void CompositorWorkspace::recordCommandBuffer(uint32_t frameIndex)
{
	static Shit::ClearValue clearValue = Shit::ClearColorValueFloat{0, 0, 0, 0};

	auto cmdBuffer = _commandBuffers[frameIndex];
	cmdBuffer->Reset({});
	cmdBuffer->Begin(
		Shit::CommandBufferBeginInfo{
			//Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT,
		});
	auto frameBufferExtent = Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->imageExtent;
	Shit::Viewport viewports[]{
		{0, 0, frameBufferExtent.width, frameBufferExtent.height, 0, 1},
	};
	Shit::Rect2D scissors[]{
		{0, 0, frameBufferExtent.width, frameBufferExtent.height},
	};
	cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, viewports});
	cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, scissors});

	cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{
		_creator->getRenderPass(),
		_creator->getFramebuffer(frameIndex),
		{
			{},
			Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->imageExtent,
		},
		1,
		&clearValue,
		Shit::SubpassContents::INLINE});
	cmdBuffer->BindPipeline(Shit::BindPipelineInfo{
		Shit::PipelineBindPoint::GRAPHICS,
		_pipeline});

	_descriptorSetData->bind(
		cmdBuffer,
		_pipeline->GetBindPoint(),
		_pipeline->GetPipelineLayout(),
		0,
		frameIndex);

	//cmdBuffer->BindDescriptorSets(
	//	Shit::BindDescriptorSetsInfo{
	//		Shit::PipelineBindPoint::GRAPHICS,
	//		dynamic_cast<Shit::GraphicsPipeline *>(_pipeline)->GetCreateInfoPtr()->pLayout,
	//		0,
	//		1,
	//		&frameContent.descriptorSet,
	//	});

	cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
	cmdBuffer->EndRenderPass();
	cmdBuffer->End();
}
void CompositorWorkspace::createPipeline()
{
	//recreate pipeline
	Shit::PipelineShaderStageCreateInfo shaderStagesInfo[]{
		s_vertShader->getPipelineShaderStageCreateInfo(),
		_outputCompoite->getFragShader()->getPipelineShaderStageCreateInfo()};

	Shit::PipelineColorBlendAttachmentState blendStates[]{
		{true,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits::R_BIT |
			 Shit::ColorComponentFlagBits::G_BIT |
			 Shit::ColorComponentFlagBits::B_BIT |
			 Shit::ColorComponentFlagBits::A_BIT}};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR,
	};
	_pipeline = Root::getSingleton().getDevice()->Create(Shit::GraphicsPipelineCreateInfo{
		std::size(shaderStagesInfo),
		shaderStagesInfo,
		Shit::VertexInputStateCreateInfo{},
		Shit::PipelineInputAssemblyStateCreateInfo{Shit::PrimitiveTopology::TRIANGLE_LIST},
		Shit::PipelineViewportStateCreateInfo{},
		Shit::PipelineTessellationStateCreateInfo{},
		Shit::PipelineRasterizationStateCreateInfo{
			false,
			false,
			Shit::PolygonMode::FILL,
			Shit::CullMode::BACK,
			Shit::FrontFace::COUNTER_CLOCKWISE,
			false, 0, 0, 0,
			1.0 //line with
		},
		Shit::PipelineMultisampleStateCreateInfo{
			Shit::SampleCountFlagBits::BIT_1,
		},
		Shit::PipelineDepthStencilStateCreateInfo{},
		Shit::PipelineColorBlendStateCreateInfo{
			false,
			{},
			std::size(blendStates),
			blendStates,
		},
		Shit::PipelineDynamicStateCreateInfo{
			std::size(dynamicStates),
			dynamicStates},
		_outputCompoite->getPipelineLayout(),
		_creator->getRenderPass(),
		0});
}
void CompositorWorkspace::destroyOutDatedResources()
{
	auto device = Root::getSingleton().getDevice();
	//destroy old pipeline
	device->Destroy(_pipeline);
	if (_descriptorSetData)
		_descriptorSetData->destroy();
	_uboBuffer->destroy();
}
void CompositorWorkspace::build()
{
	destroyOutDatedResources();

	_descriptorSetData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		_outputCompoite->getPipelineLayout()->GetCreateInfoPtr()->pSetLayouts[0], true);

	for (auto &&e : _compositorNodes)
		e->reset(_globalPara);

	if (_uboBuffer->size() > 0)
	{
		//_uboBuffer->upload();
		_descriptorSetData->setBuffer(0, 0, {_uboBuffer, 0, _uboBuffer->size(), 1});
	}

	//set texture
	_descriptorSetData->prepare();

	if (_outputCompoite)
	{
		//update composite node
		_outputCompoite->rebuild();
		createPipeline();

		createDescriptorSets();

		_needRebuild = false;
		return;
	}
	LOG("Error, OuptuComposite node not exist");
}
void CompositorWorkspace::createDescriptorSets()
{

}
void CompositorWorkspace::updateUBOBuffer(uint32_t frameIndex)
{
	_uboBuffer->upload(frameIndex);
}
void CompositorWorkspace::update(uint32_t frameIndex)
{
	//render scene
	for (auto &&e : _compositorNodes)
		e->update(frameIndex);

	updateUBOBuffer(frameIndex);
	if (_needUpdateCommandBuffers[frameIndex])
	{
		recordCommandBuffer(frameIndex);
		_needUpdateCommandBuffers[frameIndex] = false;
	}
}
void CompositorWorkspace::setUBOBufferData(size_t offset, size_t size, void const *data)
{
	_uboBuffer->setData(offset, size, data);
}
void CompositorWorkspace::setUBOBufferData(size_t offset, size_t size, int val)
{
	_uboBuffer->setData(offset, size, val);
}
BufferView CompositorWorkspace::allocateUBOBuffer(size_t stride, size_t count, void const *data)
{
	return _uboBuffer->allocate(stride, count, data);
}
BufferView CompositorWorkspace::allocateUBOBuffer(size_t stride, size_t count, int val)
{
	return _uboBuffer->allocate(stride, count, val);
}

//====================================================================
CompositorManager::CompositorManager()
{
	initRenderPass();
	initFramebuffers();
	Root::getSingleton().getScreen()->addSwapchainRecreateCallback(std::bind(&CompositorManager::recreateSwapchainCallback, this));
}
void CompositorManager::recreateSwapchainCallback()
{
	auto device = Root::getSingleton().getDevice();
	for (auto e : _framebuffersForSwapchain)
		device->Destroy(e);
	initFramebuffers();
	for (auto &&e : _compositorWorkspaces)
	{
		//render layer node
		for (auto &&e2 : e.second->getCompositorNodes())
		{
			e2->refresh();
		}
		e.second->getDescriptorSetData()->prepare();

		e.second->needUpdateAllCommandBuffers();

		auto &&framebufferSize = Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->imageExtent;
		float viewportSize[2]{framebufferSize.width, framebufferSize.height};
		e.second->setUBOBufferData(0, sizeof(float) * 2, viewportSize);
	}
}
void CompositorManager::initRenderPass()
{
	//render to swapchain
	Shit::AttachmentDescription attachments[]{
		{
			Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->format,
			Shit::SampleCountFlagBits::BIT_1,
			Shit::AttachmentLoadOp::CLEAR,
			Shit::AttachmentStoreOp::STORE,
			Shit::AttachmentLoadOp::DONT_CARE,
			Shit::AttachmentStoreOp::DONT_CARE,
			Shit::ImageLayout::UNDEFINED,
			Shit::ImageLayout::PRESENT_SRC,
		}};
	Shit::AttachmentReference colorAttachmentReferences[]{
		{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
	};
	Shit::SubpassDescription subpassDescs[]{
		{
			Shit::PipelineBindPoint::GRAPHICS,
			{},
			{},
			std::size(colorAttachmentReferences),
			colorAttachmentReferences,
		},
	};
	_renderPass = Root::getSingleton().getDevice()->Create(
		Shit::RenderPassCreateInfo{
			std::size(attachments),
			attachments,
			std::size(subpassDescs),
			subpassDescs,
		});
}
void CompositorManager::initFramebuffers()
{
	//create swapchain imageView
	auto swapchain = Root::getSingleton().getScreen()->getSwapchain();
	auto imageCount = swapchain->GetImageCount();
	auto device = Root::getSingleton().getDevice();

	Shit::FramebufferCreateInfo createInfo{
		_renderPass,
		1,
		{},
		swapchain->GetCreateInfoPtr()->imageExtent,
		1,
	};
	_framebuffersForSwapchain.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		auto a = Root::getSingleton().getScreen()->getSwapchainImageViewByIndex(i);
		createInfo.pAttachments = &a;
		_framebuffersForSwapchain[i] = device->Create(createInfo);
	}
}
CompositorWorkspace *CompositorManager::createCompositorWorkspace(
	const CompositorWorkspaceDescription &desc, std::string_view name)
{
	if (name.empty())
	{
		auto a = new CompositorWorkspace(this, desc);
		_compositorWorkspaces.emplace(a->getName(), std::unique_ptr<CompositorWorkspace>(a));
		return a;
	}
	return _compositorWorkspaces.emplace(name, std::make_unique<CompositorWorkspace>(this, desc, name)).first->second.get();
}
CompositorWorkspace *CompositorManager::getCompositorWorkspace(std::string_view name) const
{
	return _compositorWorkspaces.at(std::string(name)).get();
}
bool CompositorManager::contains(std::string_view name) const
{
	return _compositorWorkspaces.contains(std::string(name));
}