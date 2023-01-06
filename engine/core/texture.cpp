#include "texture.hpp"
#include "root.hpp"
#include "resourceGroup.hpp"

//Texture::Texture(
//	GPUResourceManager *creator,
//	std::string_view name,
//	Shit::ImageCreateInfo const &imageCreateInfo,
//	bool frameDependent,
//	std::string_view group) : GPUResource(creator, name, group, frameDependent)
//{
//	_gpuImage = Root::getSingleton().getDevice()->Create(imageCreateInfo);
//}
Texture::Texture(GPUResourceManager *creator,
				 std::string_view name,
				 std::string_view group,
				 Shit::ImageCreateInfo const &createInfo,
				 SamplerInfo const &samplerInfo,
				 bool frameDependent)
	: GPUResource(creator, name, group, frameDependent),
	  _samplerInfo(samplerInfo), _imageCreateInfo(createInfo)
{
	//auto device = Root::getSingleton().getDevice();
	//for (uint32_t i = 0; i < _gpuResourceCount; ++i)
	//	_gpuImages.emplace_back(device->Create(createInfo));
	_gpuImages.resize(_gpuResourceCount);
}
Texture::Texture(
	GPUResourceManager *creator,
	std::string_view name,
	std::string_view group,
	Shit::ImageType type,
	Shit::ImageUsageFlagBits usage,
	std::span<Image *> images,
	SamplerInfo const &samplerInfo)
	: GPUResource(creator, name, group, false),
	  _images(images.begin(), images.end()), _samplerInfo(samplerInfo)
{
	_gpuImages.resize(_gpuResourceCount);

	//load images
	auto p = _images[0];

	LOG("creating gpu texture from image name:", p->getName(), "group:", p->getGroupName());

	auto depth = _images.size();
	if (depth == 1)
	{
		p->load();
	}
	else
	{
		for (uint32_t i = 0, s = _images.size(); i < s; ++i)
			_images[i]->load();
	}

	//setup image createinfo
	_imageCreateInfo = {
		Shit::ImageCreateFlagBits::MUTABLE_FORMAT_BIT,
		type,
		p->getFormat(),
		Shit::Extent3D{(uint32_t)p->getWidth(), (uint32_t)p->getHeight(), (uint32_t)depth},
		0,
		1,
		Shit::SampleCountFlagBits::BIT_1,
		Shit::ImageTiling::OPTIMAL,
		usage,
		Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
}
Texture::~Texture()
{
}
void Texture::saveImage(std::string_view name, uint32_t index)
{
	index = std::min(_gpuResourceCount - 1, index);

	// create temp texture
	auto device = Root::getSingleton().getDevice();
	if (!_stageImage)
	{
		auto createInfo = *_gpuImages[index]->GetCreateInfoPtr();
		createInfo.usageFlags = Shit::ImageUsageFlagBits::TRANSFER_DST_BIT;
		createInfo.memoryPropertyFlags = Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT |
										 Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT;
		createInfo.initialLayout = Shit::ImageLayout::TRANSFER_DST_OPTIMAL;
		_stageImage = device->Create(createInfo);
	}

	Root::getSingleton().executeOneTimeCommand(
		[&](Shit::CommandBuffer *cmdBuffer)
		{
			auto pImage = _gpuImages[index];
			auto srcImageLayout = pImage->GetCreateInfoPtr()->initialLayout;
			auto dstImageLayout = Shit::ImageLayout::TRANSFER_SRC_OPTIMAL;
			std::vector<Shit::ImageMemoryBarrier> barriers{
				{Shit::AccessFlagBits::SHADER_READ_BIT,
				 Shit::AccessFlagBits::TRANSFER_READ_BIT,
				 srcImageLayout,
				 dstImageLayout,
				 Root::getSingleton().getGraphicsQueue()->GetFamilyIndex(),
				 Root::getSingleton().getTransferQueue()->GetFamilyIndex(),
				 pImage,
				 {Shit::ImageAspectFlagBits::COLOR_BIT,
				  0,
				  pImage->GetCreateInfoPtr()->mipLevels,
				  0,
				  pImage->GetCreateInfoPtr()->arrayLayers}},
			};
			cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{
				Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
				Shit::PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				0,
				0,
				0,
				(uint32_t)barriers.size(),
				barriers.data()});
			auto layercount = pImage->GetCreateInfoPtr()->arrayLayers;
			Shit::ImageCopy copyregion{
				{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 0, layercount},
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 0, layercount},
				{},
				pImage->GetCreateInfoPtr()->extent};
			cmdBuffer->CopyImage(Shit::CopyImageInfo{
				pImage,
				_stageImage,
				1,
				&copyregion});
			barriers = std::vector<Shit::ImageMemoryBarrier>{
				{Shit::AccessFlagBits::TRANSFER_READ_BIT,
				 Shit::AccessFlagBits::SHADER_READ_BIT,
				 dstImageLayout,
				 srcImageLayout,
				 Root::getSingleton().getTransferQueue()->GetFamilyIndex(),
				 Root::getSingleton().getGraphicsQueue()->GetFamilyIndex(),
				 pImage,
				 {Shit::ImageAspectFlagBits::COLOR_BIT,
				  0,
				  pImage->GetCreateInfoPtr()->mipLevels,
				  0,
				  pImage->GetCreateInfoPtr()->arrayLayers}},
				{Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
				 Shit::AccessFlagBits::MEMORY_READ_BIT,
				 _stageImage->GetCreateInfoPtr()->initialLayout,
				 _stageImage->GetCreateInfoPtr()->initialLayout,
				 ST_QUEUE_FAMILY_IGNORED,
				 ST_QUEUE_FAMILY_IGNORED,
				 _stageImage,
				 {Shit::ImageAspectFlagBits::COLOR_BIT,
				  0,
				  _stageImage->GetCreateInfoPtr()->mipLevels,
				  0,
				  _stageImage->GetCreateInfoPtr()->arrayLayers}},
			};
			cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{
				Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
				Shit::PipelineStageFlagBits::TRANSFER_BIT,
				{},
				0,
				0,
				0,
				0,
				(uint32_t)barriers.size(),
				barriers.data()});
		});

	auto grp = ResourceGroupManager::getSingleton().getResourceGroup();
	if (_images.empty())
	{
		_images.emplace_back(
			static_cast<Image *>(grp->createResource({std::string(name), ResourceType::IMAGE})));
	}
	//_images[0]->updateData();
	// auto ss = _images[0]->open(std::ios::out);
	////ss-
	//_images[0]->close();
}
void Texture::uploadImpl(int index)
{
	auto device = Root::getSingleton().getDevice();
	if (_images.empty())
	{
		_gpuImages[index] = device->Create(_imageCreateInfo);
		_gpuImages[index]->GenerateMipmap(
			Shit::Filter::LINEAR, _imageCreateInfo.initialLayout, _imageCreateInfo.initialLayout);
		return;
	}

	auto p = _images[0];

	LOG("creating gpu texture from image name:", p->getName(), "group:", p->getGroupName());

	auto depth = _images.size();
	unsigned char const *data;
	if (depth == 1)
	{
		data = p->getColorData().data();
	}
	else
	{
		auto len = p->getSize();
		data = new unsigned char[len * depth];
		for (uint32_t i = 0, s = _images.size(); i < s; ++i)
		{
			memcpy((void *)&data[len * i], _images[i]->getColorData().data(), len);
		}
	}
	_gpuImages[index] = device->Create(_imageCreateInfo, Shit::ImageAspectFlagBits::COLOR_BIT, data);
	_gpuImages[index]->GenerateMipmap(
		Shit::Filter::LINEAR, _imageCreateInfo.initialLayout, _imageCreateInfo.initialLayout);
	if (depth > 1)
		delete data;
}
void Texture::destroyImpl(int index)
{
	auto device = Root::getSingleton().getDevice();
	for (auto &&e : _gpuImageViews)
	{
		if (e.first.pImage == _gpuImages[index])
			device->Destroy(e.second);
		e.second = nullptr;
	}
	device->Destroy(_gpuImages[index]);
	_gpuImages[index] = nullptr;
	device->Destroy(_stageImage);
	_stageImage = nullptr;
}

Shit::ImageView *Texture::getGpuImageView(
	Shit::ImageViewType viewType,
	const Shit::ImageSubresourceRange &subresourceRange,
	Shit::Format format,
	const Shit::ComponentMapping &components,
	uint32_t frameIndex)
{
	upload();
	auto imageViewCreateInfo =
		Shit::ImageViewCreateInfo{
			_gpuImages[frameIndex],
			viewType,
			format,
			components,
			subresourceRange};

	if (_gpuImageViews.contains(imageViewCreateInfo))
		return _gpuImageViews[imageViewCreateInfo];

	return _gpuImageViews.emplace(
							 imageViewCreateInfo,
							 Root::getSingleton().getDevice()->Create(
								 imageViewCreateInfo))
		.first->second;
}
Shit::ImageView *Texture::getGpuImageView(
	Shit::ImageViewType viewType,
	const Shit::ImageSubresourceRange &subresourceRange,
	const Shit::ComponentMapping &components,
	uint32_t frameIndex)
{
	upload();
	auto imageViewCreateInfo =
		Shit::ImageViewCreateInfo{
			_gpuImages[frameIndex],
			viewType,
			_gpuImages[frameIndex]->GetCreateInfoPtr()->format,
			components,
			subresourceRange};

	if (_gpuImageViews.contains(imageViewCreateInfo))
		return _gpuImageViews[imageViewCreateInfo];

	return _gpuImageViews.emplace(
							 imageViewCreateInfo,
							 Root::getSingleton().getDevice()->Create(
								 imageViewCreateInfo))
		.first->second;
}
Shit::Sampler *Texture::getSampler()
{
	return static_cast<TextureManager *>(_creator)->createOrRetrieveSampler(_samplerInfo);
}
//==========================================
TextureManager::TextureManager()
{
}
TextureManager::~TextureManager()
{
}
Texture *TextureManager::create(
	Shit::ImageType type,
	Shit::ImageUsageFlagBits usage,
	std::span<Image *> images,
	SamplerInfo const &samplerInfo,
	std::string_view name,
	std::string_view group)
{
	auto a = new Texture(this, name, group, type, usage, images, samplerInfo);
	add(a);
	return a;
}
Texture *TextureManager::create(
	Shit::ImageCreateInfo const &imageCreateInfo,
	bool frameDependent,
	SamplerInfo const &samplerInfo,
	std::string_view name,
	std::string_view group)
{
	auto a = new Texture(this, name, group, imageCreateInfo, samplerInfo, frameDependent);
	add(a);
	return a;
}
Texture *TextureManager::getDefaultTextureBlack()
{
	static Texture *ret = nullptr;
	if (!ret)
	{
		static int blackColor = 0;
		static std::string blackColorName = "black";
		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
		ParameterMap paras{
			{"image_type", std::to_string((int)ImageType::JPG)},
			{"width", "1"},
			{"height", "1"},
		};
		Image *images[1];
		images[0] = static_cast<Image *>(grp->createResource(
			ResourceDeclaration{
				std::string(blackColorName),
				ResourceType::IMAGE,
				nullptr,
				paras},
			4,
			&blackColor));
		ret = create(Shit::ImageType::TYPE_2D, Shit::ImageUsageFlagBits::SAMPLED_BIT, images,
					 {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, false},
					 blackColorName);
		ret->upload();
	}
	return ret;
}
Texture *TextureManager::getDefaultTextureWhite()
{
	static Texture *ret = nullptr;
	if (!ret)
	{
		static int whiteColor = 0xffffffff;
		static std::string whiteColorName = "white";
		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
		ParameterMap paras{
			{"image_type", std::to_string((int)ImageType::JPG)},
			{"width", "1"},
			{"height", "1"},
		};
		Image *images[1];
		images[0] = static_cast<Image *>(grp->createResource(
			ResourceDeclaration{
				std::string(whiteColorName),
				ResourceType::IMAGE,
				nullptr,
				paras},
			4,
			&whiteColor));
		ret = create(Shit::ImageType::TYPE_2D,
					 Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
					 images,
					 {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, false},
					 whiteColorName);
		ret->upload();
	}
	return ret;
}
Shit::Sampler *TextureManager::createOrRetrieveSampler(SamplerInfo const& samplerInfo)
{
	//auto a = SamplerInfo{texInterpolation, wrapmode, compareEnable};
	if (_samplers.contains(samplerInfo))
		return _samplers[samplerInfo];

	Shit::Filter magFilter = Shit::Filter::NEAREST;
	Shit::Filter minFilter = Shit::Filter::NEAREST;
	Shit::SamplerMipmapMode mipmapMode = Shit::SamplerMipmapMode::LINEAR;
	if (samplerInfo.texInterpolation == TextureInterpolation::LINEAR)
	{
		magFilter = Shit::Filter::LINEAR;
		minFilter = Shit::Filter::LINEAR;
		mipmapMode = Shit::SamplerMipmapMode::NEAREST;
	}
	else if (samplerInfo.texInterpolation == TextureInterpolation::NEAREST)
	{
		magFilter = Shit::Filter::NEAREST;
		minFilter = Shit::Filter::NEAREST;
		mipmapMode = Shit::SamplerMipmapMode::NEAREST;
	}
	else if (samplerInfo.texInterpolation == TextureInterpolation::CUBIC)
	{
		magFilter = Shit::Filter::LINEAR;
		minFilter = Shit::Filter::LINEAR;
		mipmapMode = Shit::SamplerMipmapMode::LINEAR;
	}

	Shit::SamplerCreateInfo createInfo{
		magFilter,
		minFilter,
		mipmapMode,
		samplerInfo.wrapmode,
		samplerInfo.wrapmode,
		samplerInfo.wrapmode,
		0,
		false,
		0,
		samplerInfo.compareEnable,
		Shit::CompareOp::LESS_OR_EQUAL,
		-1000,
		1000,
		Shit::BorderColor::FLOAT_TRANSPARENT_BLACK};
	return _samplers.emplace(samplerInfo, Root::getSingleton().getDevice()->Create(createInfo)).first->second;
}