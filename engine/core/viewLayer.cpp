#include "viewLayer.hpp"
#include "root.hpp"
#include "texture.hpp"
#include "pipeline.hpp"
#include "descriptorManager.hpp"
#include "material.hpp"

ViewLayer::ViewLayer(Screen *screen) : _renderPath(RenderPath::DEFERRED)
{
	auto frameCount = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();
	_framebuffers.resize(frameCount, {});
	init();
}
ViewLayer::ViewLayer(Shit::Extent2D ext, Shit::Format colorFormat)
	: _colorFormat(colorFormat), _extent(ext), _renderPath(RenderPath::DEFERRED)
{
	auto frameCount = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();
	_framebuffers.resize(frameCount, {});
	init();
}
ViewLayer::~ViewLayer()
{
	destroyFramebuffers();
}
void ViewLayer::init()
{
	auto count = _framebuffers.size();
	Shit::Format tempDepthFormats[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};

	auto depthFormat = Root::getSingleton().getDevice()->GetSuitableImageFormat(tempDepthFormats, Shit::ImageTiling::OPTIMAL,
																				Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

	auto textureMgr = Root::getSingleton().getTextureManager();

	SamplerInfo samplerInfo{
		TextureInterpolation::LINEAR,
		Shit::SamplerWrapMode::MIRRORED_REPEAT,
		false};
	SamplerInfo shadowSamplerInfo{
		TextureInterpolation::LINEAR,
		Shit::SamplerWrapMode::MIRRORED_REPEAT,
		true};

	// create color attachments
	Shit::ImageCreateInfo colorImageCI{
		{},
		Shit::ImageType::TYPE_2D,
		_colorFormat,
		{_extent.width, _extent.height, 1},
		1,
		1,
		Shit::SampleCountFlagBits::BIT_1,
		Shit::ImageTiling::OPTIMAL,
		Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT |
			Shit::ImageUsageFlagBits::SAMPLED_BIT,
		Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
	};
	_colorTextureId = textureMgr->create(colorImageCI, true, samplerInfo)->getId();

	// create depth attachments
	Shit::ImageCreateInfo depthImageCI{
		{},
		Shit::ImageType::TYPE_2D,
		depthFormat,
		{_extent.width, _extent.height, 1},
		1,
		1,
		Shit::SampleCountFlagBits::BIT_1,
		Shit::ImageTiling::OPTIMAL,
		Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT |
			Shit::ImageUsageFlagBits::SAMPLED_BIT,
		Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
	};
	_depthTextureId = textureMgr->create(depthImageCI, true, samplerInfo)->getId();

	if (_renderPath == RenderPath::DEFERRED)
	{
		// create position attachments
		// create normal attachments
		// create mra attachments
		auto imageCI = Shit::ImageCreateInfo{
			{},
			Shit::ImageType::TYPE_2D,
			Shit::Format::R16G16B16A16_SFLOAT,
			{_extent.width, _extent.height, 1},
			1,
			1,
			Shit::SampleCountFlagBits::BIT_1,
			Shit::ImageTiling::OPTIMAL,
			Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT |
				Shit::ImageUsageFlagBits::SAMPLED_BIT |
				Shit::ImageUsageFlagBits::INPUT_ATTACHMENT_BIT,
			Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		};
		_positionTextureId = textureMgr->create(imageCI, true, samplerInfo)->getId();

		imageCI.format = Shit::Format::R16G16B16A16_UNORM;

		_albedoTextureId = textureMgr->create(imageCI, true, samplerInfo)->getId();
		_MRATextureId = textureMgr->create(imageCI, true, samplerInfo)->getId();
		_emissionTextureId = textureMgr->create(imageCI, true, samplerInfo)->getId();

		imageCI.format = Shit::Format::R16G16B16A16_SNORM;
		_normalTextureId = textureMgr->create(imageCI, true, samplerInfo)->getId();
	}
}
void ViewLayer::createFramebuffers(RenderPassType renderPassType)
{
	Shit::RenderPass *renderPass = Root::getSingleton().getRenderPass(renderPassType);
	auto device = Root::getSingleton().getDevice();

	auto count = _framebuffers.size();

	auto colorAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_colorTextureId));
	auto depthAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_depthTextureId));

	for (uint32_t i = 0; i < count; ++i)
	{
		switch (renderPassType)
		{
		case RenderPassType::BACKGROUND:
		{
			Shit::ImageView *images[]{
				colorAttachment->getGpuImageView(
					Shit::ImageViewType::TYPE_2D,
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
			};
			_framebuffers[i][size_t(RenderPassType::BACKGROUND)] = device->Create(
				Shit::FramebufferCreateInfo{
					renderPass,
					std::size(images),
					images,
					_extent,
					1});
		}
		break;
		case RenderPassType::FORWARD:
		{
			Shit::ImageView *images[]{
				colorAttachment->getGpuImageView(
					Shit::ImageViewType::TYPE_2D,
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				depthAttachment->getGpuImageView(
					Shit::ImageViewType::TYPE_2D,
					{Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}, {}, i),
			};
			_framebuffers[i][size_t(RenderPassType::FORWARD)] = device->Create(
				Shit::FramebufferCreateInfo{
					renderPass,
					std::size(images),
					images,
					_extent,
					1});
		}
		break;
		case RenderPassType::DEFERRED:
		{
			auto positionAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_positionTextureId));
			auto albedoAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_albedoTextureId));
			auto normalAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_normalTextureId));
			auto mraAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_MRATextureId));
			auto emissionAttachment = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(_emissionTextureId));

			Shit::ImageView *images[]{
				colorAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				positionAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				albedoAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				normalAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				mraAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				emissionAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}, {}, i),
				depthAttachment->getGpuImageView(Shit::ImageViewType::TYPE_2D, {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}, {}, i),
			};
			_framebuffers[i][size_t(RenderPassType::DEFERRED)] = device->Create(
				Shit::FramebufferCreateInfo{renderPass, std::size(images), images, _extent, 1});

			auto mgr = Root::getSingleton().getDescriptorManager();

			auto pipelineLayout = Root::getSingleton()
									  .getMaterialDataBlockManager()
									  ->getMaterial(MaterialType::DEFERRED)
									  ->getPipeline({Shit::PrimitiveTopology::TRIANGLE_LIST})
									  ->GetPipelineLayout();

			auto setData = mgr->createDescriptorSetData(pipelineLayout->GetCreateInfoPtr()->pSetLayouts[1], true);
			_gpassDescriptorSetDataId = setData->getId();

			setData->setTexture(
				0, 0,
				DescriptorTextureData{
					positionAttachment,
					Shit::ImageViewType::TYPE_2D,
					positionAttachment->getImageCreateInfo()->format,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					positionAttachment->getSamplerInfo()});
			setData->setTexture(
				1, 0,
				DescriptorTextureData{
					albedoAttachment,
					Shit::ImageViewType::TYPE_2D,
					albedoAttachment->getImageCreateInfo()->format,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					albedoAttachment->getSamplerInfo()});
			setData->setTexture(
				2, 0,
				DescriptorTextureData{
					normalAttachment,
					Shit::ImageViewType::TYPE_2D,
					normalAttachment->getImageCreateInfo()->format,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					normalAttachment->getSamplerInfo()});
			setData->setTexture(
				3, 0,
				DescriptorTextureData{
					mraAttachment,
					Shit::ImageViewType::TYPE_2D,
					mraAttachment->getImageCreateInfo()->format,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					mraAttachment->getSamplerInfo()});
			setData->setTexture(
				4, 0,
				DescriptorTextureData{
					emissionAttachment,
					Shit::ImageViewType::TYPE_2D,
					emissionAttachment->getImageCreateInfo()->format,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					emissionAttachment->getSamplerInfo()});

			setData->prepare();
		}
		break;
		// case RenderPassType::DEFERRED_GEOMETRY:
		// 	break;
		// case RenderPassType::DEFERRED_SHADING:
		// 	break;
		case RenderPassType::ALPHA_TEST:
			break;
		}
	}
}
void ViewLayer::destroyFramebuffers()
{
	auto textureMgr = Root::getSingleton().getTextureManager();
	textureMgr->remove(_colorTextureId);
	textureMgr->remove(_depthTextureId);
	auto device = Root::getSingleton().getDevice();
	for (auto &&e : _framebuffers)
	{
		for (auto p : e)
		{
			if (p)
				device->Destroy(p);
		}
	}
}
void ViewLayer::resize(Shit::Extent2D ext)
{
	destroyFramebuffers();
	init();
	_extent = ext;
	_resizeViewLayerSignal(this);
}
Shit::Framebuffer *ViewLayer::getFramebuffer(RenderPassType passType, uint32_t frameIndex)
{
	auto p = _framebuffers[frameIndex][(size_t)passType];
	if (p)
		return p;
	else
	{
		createFramebuffers(passType);
		return _framebuffers[frameIndex][(size_t)passType];
	}
}
DescriptorSetData *ViewLayer::getDeferredDescriptorSetData() const
{
	return static_cast<DescriptorSetData *>(Root::getSingleton().getDescriptorManager()->get(_gpassDescriptorSetDataId));
}
void ViewLayer::addResizeListener(Shit::Slot<void(ViewLayer *)> const &slot)
{
	_resizeViewLayerSignal.Connect(slot);
}
void ViewLayer::removeResizeListener(Shit::Slot<void(ViewLayer *)> const &slot)
{
	_resizeViewLayerSignal.Disconnect(slot);
}