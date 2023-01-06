#include "renderQueue.hpp"
#include "root.hpp"
#include "scene.hpp"
#include "primitive.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "sceneNode.hpp"

//===============
// RenderQueue::RenderQueue(RenderQueueGroup *renderQueueGroup, RenderQueueType queueType)
//	: _renderQueueGroup(renderQueueGroup), _renderQueueType(queueType)
//{
//}
// void RenderQueue::addPrimitiveView(PrimitiveView *primitiveView)
//{
//	auto m = primitiveView->getMaterialDataBlock()->getMaterial();
//	auto pipelineSpec = GraphicPipelineSpec{primitiveView->getTopology()};
//	_primitiveViewGroups[m][pipelineSpec].emplace_back(primitiveView);
//}
// void RenderQueue::removePrimitiveView(PrimitiveView *primitiveView)
//{
//	auto m = primitiveView->getMaterialDataBlock()->getMaterial();
//	auto pipelineSpec = GraphicPipelineSpec{primitiveView->getTopology()};
//	std::erase(_primitiveViewGroups[m][pipelineSpec], primitiveView);
//}
// void RenderQueue::addPrimitiveViews(std::span<PrimitiveView *> primitiveViews)
//{
//	for (auto p : primitiveViews)
//		addPrimitiveView(p);
//}
// void RenderQueue::removePrimitiveViews(std::span<PrimitiveView *> primitiveViews)
//{
//	for (auto p : primitiveViews)
//		removePrimitiveView(p);
//}
// void RenderQueue::clear()
//{
//	_primitiveViewGroups.clear();
//}

// void RenderQueue::prepare()
//{
//	for (auto &&e : _primitiveViewGroups)
//	{
//		for (auto &&e2 : e.second)
//		{
//			for (auto primitiveView : e2.second)
//			{
//				primitiveView->prepare();
//			}
//		}
//	}
// }
//  void RenderQueue::updateData()
//{
//	//for (auto &&e : _primitiveViewGroups)
//	//{
//	//	for (auto &&e2 : e.second)
//	//	{
//	//		//std::erase_if(e2.second, [](auto p)
//	//		//			  { return p.expired(); });
//	//		for (auto primitiveView : e2.second)
//	//		{
//	//			primitiveView->updateData();
//	//		}
//	//	}
//	//}
//  }

// void RenderQueue::updateGPUData(uint32_t frameIndex)
//{
//	for (auto &&e : _primitiveViewGroups)
//	{
//		for (auto &&e2 : e.second)
//		{
//			for (auto primitiveView : e2.second)
//			{
//				primitiveView->updateGPUData(frameIndex);
//			}
//		}
//	}
// }
// void RenderQueue::recordCommandBuffer(
//	uint32_t frameIndex, bool drawShadow, MaterialDataBlock *materialDataBlock)
//{
//	//if (_framePrimaryCommandBuffers.size() == 1 && !_needRecord)
//	//	return;

//	_needRecord = false;

//	auto cmdBuffer = _framePrimaryCommandBuffers[frameIndex];

//	//============
//	auto camera = _renderQueueGroup->_camera;

//	auto renderPath = camera->getRenderPath();
//	auto viewLayer = camera->getViewLayer();
//	auto ext = viewLayer->getExtent();

//	auto clearColor = camera->getClearColorValue();

//	std::vector<Shit::ClearValue> clearValues{clearColor};

//	Shit::RenderPass *renderPass;
//	Shit::Framebuffer *framebuffer;

//	switch (_renderQueueType)
//	{
//	case RenderQueueType::BACKGROUND:
//	{
//		renderPass = Root::getSingleton().getRenderPass(RenderPassType::BACKGROUND);
//		framebuffer = viewLayer->getFramebuffer(RenderPassType::BACKGROUND, frameIndex);
//	}
//	break;
//	case RenderQueueType::GEOMETRY:
//		if (renderPath == RenderPath::FORWARD)
//		{
//			renderPass = Root::getSingleton().getRenderPass(RenderPassType::FORWARD);
//			framebuffer = viewLayer->getFramebuffer(RenderPassType::FORWARD, frameIndex);
//			clearValues.emplace_back(Shit::ClearDepthStencilValue{1.f, 0});
//		}
//		else if (renderPath == RenderPath::DEFERRED)
//		{
//		}
//		break;
//	case RenderQueueType::ALPHATEST:
//		return;
//		break;
//	case RenderQueueType::OVERLAY:
//		return;
//		break;
//	}

//	auto a = camera->getRelativeViewport();
//	auto b = camera->getRelativeScissor();

//	Shit::Viewport viewport{
//		a.offset.x * ext.width,
//		a.offset.y * ext.height,
//		a.extent.width * ext.width,
//		a.extent.height * ext.height,
//		0.f,
//		1.f};
//	Shit::Rect2D scissor{
//		{uint32_t(b.offset.x * ext.width), uint32_t(b.offset.y * ext.height)},
//		{uint32_t(b.extent.width * ext.width), uint32_t(b.extent.height * ext.height)},
//	};

//	if (!drawShadow)
//	{
//		cmdBuffer->Reset();
//		cmdBuffer->Begin();
//		cmdBuffer->BeginRenderPass(
//			Shit::BeginRenderPassInfo{
//				renderPass,
//				framebuffer,
//				{{}, framebuffer->GetCreateInfoPtr()->extent},
//				(uint32_t)clearValues.size(),
//				clearValues.data(),
//				Shit::SubpassContents::INLINE});
//		cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
//		cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});
//	}

//	// bind camera descriptor set
//	camera->recordCommandBuffer(cmdBuffer, frameIndex);

//	for (auto &&e : _primitiveViewGroups)
//	{
//		for (auto &&e2 : e.second)
//		{
//			// pipline
//			if (!drawShadow)
//				e.first->bindPipeline(cmdBuffer, frameIndex, e2.first);
//			//
//			for (auto primitiveView : e2.second)
//			{
//				primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, materialDataBlock);
//			}
//		}
//	}

//	cmdBuffer->EndRenderPass();
//	cmdBuffer->End();
//}
//===============================
RenderQueueGroup::RenderQueueGroup(Camera *camera) : _camera(camera)
{
	// create renderqueue
	// for (int i = 0; i < (int)RenderQueueType::Num; ++i)
	//{
	//	_renderQueues[i] = std::make_unique<RenderQueue>(this, RenderQueueType(i));
	// }

	//
	uint32_t count = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();

	_framePrimaryCommandBuffers.resize(count);

	Root::getSingleton().getCommandPool()->CreateCommandBuffers(
		Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, count},
		_framePrimaryCommandBuffers.data());

	for (uint32_t i = 0; i < count; ++i)
	{
		_fences.emplace_back(Root::getSingleton().getDevice()->Create(
			Shit::FenceCreateInfo{
				Shit::FenceCreateFlagBits::SIGNALED_BIT}));
	}
}
void RenderQueueGroup::renderShadow(uint32_t frameIndex)
{
	// for(auto light:_lights)
	//{
	//	_renderQueues[size_t(RenderQueueType::GEOMETRY)]->render(frameIndex, true);
	//	_renderQueues[size_t(RenderQueueType::ALPHATEST)]->render(frameIndex, true);
	// }
}
void RenderQueueGroup::preRender(uint32_t frameIndex)
{
	_camera->updateGPUData(frameIndex);
	for (auto p : _lights)
	{
		p->updateGPUData(frameIndex);
	}
}
void RenderQueueGroup::draw(RenderQueueType queueType, Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex, Material *material)
{
	for (auto &&e : _primitiveGroups[(size_t)queueType])
	{
		for (auto &&e2 : e.second)
		{
			auto pipeline = material ? material->getPipeline(e2.first) : e.first->getPipeline(e2.first);
			cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});
			for (auto primitiveView : e2.second)
			{
				primitiveView->updateGPUData(frameIndex);
				primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, nullptr);
			}
		}
	}
}
void RenderQueueGroup::drawBackground(uint32_t frameIndex)
{
	auto cmdBuffer = _framePrimaryCommandBuffers[frameIndex];

	auto viewLayer = _camera->getViewLayer();
	auto renderPass = Root::getSingleton().getRenderPass(RenderPassType::BACKGROUND);
	auto framebuffer = viewLayer->getFramebuffer(RenderPassType::BACKGROUND, frameIndex);
	auto ext = viewLayer->getExtent();

	auto clearColor = _camera->getClearColorValue();
	std::vector<Shit::ClearValue> clearValues{clearColor};

	auto a = _camera->getRelativeViewport();
	auto b = _camera->getRelativeScissor();

	Shit::Viewport viewport{
		a.offset.x * ext.width,
		a.offset.y * ext.height,
		a.extent.width * ext.width,
		a.extent.height * ext.height,
		0.f,
		1.f};
	Shit::Rect2D scissor{
		{uint32_t(b.offset.x * ext.width), uint32_t(b.offset.y * ext.height)},
		{uint32_t(b.extent.width * ext.width), uint32_t(b.extent.height * ext.height)},
	};

	cmdBuffer->BeginRenderPass(
		Shit::BeginRenderPassInfo{
			renderPass,
			framebuffer,
			{{}, framebuffer->GetCreateInfoPtr()->extent},
			(uint32_t)clearValues.size(),
			clearValues.data(),
			Shit::SubpassContents::INLINE});

	cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
	cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

	// draw skybox
	auto pipeline = Root::getSingleton().getMaterialDataBlockManager()->getMaterial(MaterialType::SKYBOX)->getPipeline({Shit::PrimitiveTopology::TRIANGLE_LIST});
	cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});
	auto env = _camera->getParentNode()->getParentScene()->getEnvironment();
	env->getDescriptorSetData()->bind(cmdBuffer, pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), DESCRIPTORSET_ENV, frameIndex);
	env->drawSkybox(cmdBuffer, 0, scissor);

	for (auto &&e : _primitiveGroups[(size_t)RenderQueueType::BACKGROUND])
	{
		for (auto &&e2 : e.second)
		{
			auto pipeline = e.first->getPipeline(e2.first);
			cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});
			for (auto primitiveView : e2.second)
			{
				primitiveView->updateGPUData(frameIndex);
				primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, nullptr);
			}
		}
	}
	cmdBuffer->EndRenderPass();
}
void RenderQueueGroup::drawShadow(Shit::CommandBuffer *cmdBuffer, Light *light, uint32_t frameIndex)
{
	// geometry queue
	auto renderPass = Root::getSingleton().getRenderPass(RenderPassType::SHADOW);
	auto framebuffer = light->getShadowFramebuffer(frameIndex);

	Shit::ClearValue clearValue = Shit::ClearDepthStencilValue{1.0, 0};

	auto material = Root::getSingleton().getMaterialDataBlockManager()->getMaterial(MaterialType::SHADOW);

	// light->getShadowResolution();
	//bind camera pv


	cmdBuffer->BeginRenderPass(
		Shit::BeginRenderPassInfo{
			renderPass,
			framebuffer,
			{{}, framebuffer->GetCreateInfoPtr()->extent},
			1,
			&clearValue,
			Shit::SubpassContents::INLINE});

	auto ext = framebuffer->GetCreateInfoPtr()->extent;
	Shit::Viewport viewport{0, 0, (float)ext.width, (float)ext.height, 0, 1};
	Shit::Rect2D scissor{{}, ext};
	if ((Root::getSingleton().getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::VULKAN)
	{
		viewport.y = -viewport.height - viewport.y;
		viewport.height = -viewport.height;
	}
	cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
	cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

	light->getDescriptorSetData()->bind(
		cmdBuffer, Shit::PipelineBindPoint::GRAPHICS,
		material->getPipelineWrapper()->getCreateInfoPtr()->pLayout,
		DESCRIPTORSET_LIGHT, frameIndex);

	for (auto &&e : _primitiveGroups[(size_t)RenderQueueType::GEOMETRY])
	{
		for (auto &&e2 : e.second)
		{
			auto pipeline = material->getPipeline(e2.first);
			if (pipeline)
			{
				cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});
				for (auto primitiveView : e2.second)
				{
					primitiveView->updateGPUData(frameIndex);
					primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, nullptr);
				}
			}
		}
	}
	cmdBuffer->EndRenderPass();
}
void RenderQueueGroup::drawGeometry(uint32_t frameIndex)
{
	auto cmdBuffer = _framePrimaryCommandBuffers[frameIndex];
	auto renderPath = _camera->getRenderPath();
	auto viewLayer = _camera->getViewLayer();

	auto clearColor = _camera->getClearColorValue();
	std::vector<Shit::ClearValue> clearValues{clearColor};

	if (renderPath == RenderPath::FORWARD)
	{
		auto renderPass = Root::getSingleton().getRenderPass(RenderPassType::FORWARD);
		auto framebuffer = viewLayer->getFramebuffer(RenderPassType::FORWARD, frameIndex);
		clearValues.emplace_back(Shit::ClearDepthStencilValue{1.f, 0});

		// env light + emissions
		cmdBuffer->BeginRenderPass(
			Shit::BeginRenderPassInfo{
				renderPass,
				framebuffer,
				{{}, framebuffer->GetCreateInfoPtr()->extent},
				(uint32_t)clearValues.size(),
				clearValues.data(),
				Shit::SubpassContents::INLINE});

		for (auto &&e : _primitiveGroups[(size_t)RenderQueueType::GEOMETRY])
		{
			for (auto &&e2 : e.second)
			{
				auto pipeline = e.first->getPipeline(e2.first);
				if (pipeline)
				{
					cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});
					for (auto primitiveView : e2.second)
					{
						primitiveView->updateGPUData(frameIndex);
						primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, nullptr);
					}
				}
			}
		}
		cmdBuffer->EndRenderPass();

		// lights
		for (auto light : _lights)
		{
			light->getDescriptorSetData()->bind(
				cmdBuffer,
				Shit::PipelineBindPoint::GRAPHICS,
				Root::getSingleton().getCommonPipelineLayout(),
				DESCRIPTORSET_LIGHT,
				frameIndex);

			cmdBuffer->BeginRenderPass(
				Shit::BeginRenderPassInfo{
					renderPass,
					framebuffer,
					{{}, framebuffer->GetCreateInfoPtr()->extent},
					(uint32_t)clearValues.size(),
					clearValues.data(),
					Shit::SubpassContents::INLINE});

			for (auto &&e : _primitiveGroups[(size_t)RenderPassType::FORWARD])
			{
				for (auto &&e2 : e.second)
				{
					auto pipeline = e.first->getPipelineLight(e2.first);
					if (pipeline)
					{
						cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});

						for (auto primitiveView : e2.second)
						{
							primitiveView->updateGPUData(frameIndex);
							primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, nullptr);
						}
					}
				}
			}
			cmdBuffer->EndRenderPass();
		}
	}
	else if (renderPath == RenderPath::DEFERRED)
	{
		auto renderPass = Root::getSingleton().getRenderPass(RenderPassType::DEFERRED);
		auto framebuffer = viewLayer->getFramebuffer(RenderPassType::DEFERRED, frameIndex);

		clearValues = {
			Shit::ClearColorValueFloat{},
			Shit::ClearColorValueFloat{},
			Shit::ClearColorValueFloat{},
			Shit::ClearColorValueFloat{},
			Shit::ClearColorValueFloat{},
			Shit::ClearColorValueFloat{},
			Shit::ClearDepthStencilValue{1.f, 0}};

		// gpass
		cmdBuffer->BeginRenderPass(
			Shit::BeginRenderPassInfo{
				renderPass,
				framebuffer,
				{{}, framebuffer->GetCreateInfoPtr()->extent},
				(uint32_t)clearValues.size(),
				clearValues.data(),
				Shit::SubpassContents::INLINE});

		auto material = Root::getSingleton().getMaterialDataBlockManager()->getMaterial(MaterialType::GPASS);
		for (auto &&e : _primitiveGroups[(size_t)RenderQueueType::GEOMETRY])
		{
			for (auto &&e2 : e.second)
			{
				auto pipeline = material->getPipeline(e2.first);
				if (pipeline)
				{
					cmdBuffer->BindPipeline(Shit::BindPipelineInfo{pipeline->GetBindPoint(), pipeline});
					for (auto primitiveView : e2.second)
					{
						primitiveView->updateGPUData(frameIndex);
						primitiveView->recordCommandBuffer(cmdBuffer, frameIndex, nullptr);
					}
				}
			}
		}
		cmdBuffer->NextSubpass(Shit::SubpassContents::INLINE);

		// shading pass
		material = Root::getSingleton().getMaterialDataBlockManager()->getMaterial(MaterialType::DEFERRED);
		auto pipeline = material->getPipeline({Shit::PrimitiveTopology::TRIANGLE_LIST});

		auto env = _camera->getParentNode()->getParentScene()->getEnvironment();
		env->getDescriptorSetData()->bind(cmdBuffer, pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), DESCRIPTORSET_ENV, frameIndex);

		{
			// IBL
			cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline});
			viewLayer->getDeferredDescriptorSetData()->bind(
				cmdBuffer, Shit::PipelineBindPoint::GRAPHICS, pipeline->GetPipelineLayout(), 1, frameIndex);
			cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
		}

		// lights
		for (auto light : _lights)
		{
			pipeline = material->getPipelineLight({Shit::PrimitiveTopology::TRIANGLE_LIST});
			light->getDescriptorSetData()->bind(
				cmdBuffer,
				Shit::PipelineBindPoint::GRAPHICS,
				pipeline->GetPipelineLayout(),
				DESCRIPTORSET_LIGHT,
				frameIndex);

			viewLayer->getDeferredDescriptorSetData()->bind(
				cmdBuffer, Shit::PipelineBindPoint::GRAPHICS, pipeline->GetPipelineLayout(), 1, frameIndex);

			env->getDescriptorSetData()->bind(cmdBuffer, pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), DESCRIPTORSET_ENV, frameIndex);

			cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline});
			cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
		}
		cmdBuffer->EndRenderPass();
	}
}
void RenderQueueGroup::render(uint32_t frameIndex)
{
	auto cmdBuffer = _framePrimaryCommandBuffers[frameIndex];

	cmdBuffer->Reset();
	cmdBuffer->Begin();

	// bind camera descriptor set
	// _camera->recordCommandBuffer(cmdBuffer, frameIndex);
	_camera->getDescriptorSetData()->bind(
		cmdBuffer,
		Shit::PipelineBindPoint::GRAPHICS,
		Root::getSingleton().getCommonPipelineLayout(),
		0,
		frameIndex);

	//draw shadow
	for (auto light : _lights)
	{
		drawShadow(cmdBuffer, light, frameIndex);
	}

	// draw background
	drawBackground(frameIndex);
	drawGeometry(frameIndex);

	cmdBuffer->End();

	submit(frameIndex);
}
void RenderQueueGroup::postRender(uint32_t frameIndex)
{
}
void RenderQueueGroup::submit(uint32_t frameIndex)
{
	_fences[frameIndex]->WaitFor(UINT64_MAX);
	_fences[frameIndex]->Reset();
	Shit::SubmitInfo submitInfos[]{
		{
			0,
			0,
			1,
			&_framePrimaryCommandBuffers[frameIndex],
		}};
	Root::getSingleton().getGraphicsQueue()->Submit(submitInfos, _fences[frameIndex]);
}
void RenderQueueGroup::addRenderables(std::span<Renderable *> renderables)
{
	std::vector<PrimitiveView *> primitiveViews;
	for (auto p : renderables)
	{
		p->getAllPrimitiveViews(primitiveViews);
	}
	for (auto primitiveView : primitiveViews)
	{
		auto m = primitiveView->getMaterialDataBlock()->getMaterial();
		auto pipelineSpec = GraphicPipelineSpec{primitiveView->getTopology()};
		auto renderQueueType = primitiveView->getMaterialDataBlock()->getRenderQueueType();
		_primitiveGroups[(size_t)renderQueueType][m][pipelineSpec].emplace_back(primitiveView);
	}
}
void RenderQueueGroup::removeRenderables(std::span<Renderable *> renderables)
{
	std::vector<PrimitiveView *> primitiveViews;
	for (auto p : renderables)
	{
		p->getAllPrimitiveViews(primitiveViews);
	}
	for (auto primitiveView : primitiveViews)
	{
		auto m = primitiveView->getMaterialDataBlock()->getMaterial();
		auto pipelineSpec = GraphicPipelineSpec{primitiveView->getTopology()};
		auto renderQueueType = primitiveView->getMaterialDataBlock()->getRenderQueueType();
		std::erase(_primitiveGroups[(size_t)renderQueueType][m][pipelineSpec], primitiveView);
	}
}
void RenderQueueGroup::clear()
{
	for (int i = 0; i < (int)RenderQueueType::Num; ++i)
	{
		_primitiveGroups[i].clear();
	}
}
void RenderQueueGroup::setLights(std::span<Light *> lights)
{
	_lights.clear();
	_lights.insert(_lights.end(), lights.begin(), lights.end());
}
