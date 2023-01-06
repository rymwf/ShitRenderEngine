/**
 * @file descriptorManager.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "descriptorManager.hpp"
#include "root.hpp"
#include "texture.hpp"
#include "buffer.hpp"

DescriptorSetData::DescriptorSetData(
	DescriptorManager *creator,
	Shit::DescriptorSetLayout const *setLayout,
	std::string_view group,
	bool frameDependent)
	: GPUResource(creator, {}, group, frameDependent)
{
	// default textures
	auto whiteTex = Root::getSingleton().getTextureManager()->getDefaultTextureWhite();
	//
	_handles.resize(_gpuResourceCount);
	std::vector<Shit::DescriptorSetLayout const *> setLayouts(_gpuResourceCount, setLayout);

	// create descriptor set
	creator->allocateDescriptorSets(
		Shit::DescriptorSetAllocateInfo{(uint32_t)ranges::size(setLayouts), ranges::data(setLayouts)},
		ranges::data(_handles));

	auto count = setLayout->GetCreateInfoPtr()->descriptorSetLayoutBindingCount;
	auto pBindings = setLayout->GetCreateInfoPtr()->pDescriptorSetLayoutBindings;
	_descriptorsData.resize(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		auto &&e = pBindings[i];
		if (e.descriptorType == Shit::DescriptorType::STORAGE_BUFFER ||
			e.descriptorType == Shit::DescriptorType::STORAGE_BUFFER_DYNAMIC ||
			e.descriptorType == Shit::DescriptorType::UNIFORM_BUFFER ||
			e.descriptorType == Shit::DescriptorType::UNIFORM_BUFFER_DYNAMIC)
		{
			_descriptorsData[i] = {true,
								   e.binding,
								   e.descriptorCount,
								   e.descriptorType,
								   std::vector<BufferView>(pBindings[i].descriptorCount)};
		}
		else if (e.descriptorType == Shit::DescriptorType::COMBINED_IMAGE_SAMPLER ||
				 e.descriptorType == Shit::DescriptorType::INPUT_ATTACHMENT ||
				 e.descriptorType == Shit::DescriptorType::SAMPLED_IMAGE ||
				 e.descriptorType == Shit::DescriptorType::STORAGE_IMAGE)
		{
			_descriptorsData[i] = {true,
								   e.binding,
								   e.descriptorCount,
								   e.descriptorType,
								   std::vector<DescriptorTextureData>(pBindings[i].descriptorCount,
																	  {whiteTex,
																	   Shit::ImageViewType::TYPE_2D,
																	   Shit::Format::R8G8B8A8_SRGB,
																	   {},
																	   {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
																	   Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
																	   {TextureInterpolation::LINEAR,
																		Shit::SamplerWrapMode::CLAMP_TO_EDGE,
																		false}})};
		}
		else
		{
			THROW("unknown descriptor type", (int)e.descriptorType);
		}
	}
}

void DescriptorSetData::bind(Shit::CommandBuffer *cmdBuffer,
							 Shit::PipelineBindPoint bindPoint,
							 Shit::PipelineLayout const *pipelineLayout,
							 uint32_t setSlot,
							 uint32_t frameIndex) const
{
	// upload(frameIndex);
	frameIndex = std::min((uint32_t)_handles.size() - 1, frameIndex);
	cmdBuffer->BindDescriptorSets(
		Shit::BindDescriptorSetsInfo{
			bindPoint,
			pipelineLayout,
			setSlot,
			1,
			&_handles.at(frameIndex)});
}
Shit::DescriptorSet *DescriptorSetData::getHandleByIndex(uint32_t index) const
{
	index = (std::max)(_gpuResourceCount - 1, index);
	return _handles.at(index);
}
void DescriptorSetData::setBuffer(uint32_t binding, uint32_t startIndex, BufferView const &bufferViews)
{
	setBuffers(binding, startIndex, {&bufferViews, 1});
}
void DescriptorSetData::setBuffers(uint32_t binding, uint32_t startIndex, std::span<BufferView const> bufferViews)
{
	auto count = _handles[0]->GetSetLayoutPtr()->GetCreateInfoPtr()->descriptorSetLayoutBindingCount;
	auto pBindings = _handles[0]->GetSetLayoutPtr()->GetCreateInfoPtr()->pDescriptorSetLayoutBindings;
	auto bufferViewsCount = bufferViews.size();

	for (uint32_t i = 0; i < count; ++i)
	{
		if (pBindings[i].binding == binding)
		{
			_descriptorsData[i].needRewrite = true;
			auto &&a = std::get<std::vector<BufferView>>(_descriptorsData[i].data);
			ranges::copy(bufferViews, a.begin() + startIndex);
			break;
		}
	}
	// for (auto &&e : bufferViews)
	//	e.buffer->addStatusListener(
	//		std::bind(&DescriptorSetData::descriptorStatusListener,
	//				  this, std::placeholders::_1, std::placeholders::_2));
	setAllStatus(GPUResource::Status::DESTROYED);
}
void DescriptorSetData::setTexture(
	uint32_t binding, uint32_t startIndex, DescriptorTextureData const &texture)
{
	setTextures(binding, startIndex, {&texture, 1});
}
void DescriptorSetData::setTextures(
	uint32_t binding, uint32_t startIndex, std::span<DescriptorTextureData const> textures)
{
	auto count = _handles[0]->GetSetLayoutPtr()->GetCreateInfoPtr()->descriptorSetLayoutBindingCount;
	auto pBindings = _handles[0]->GetSetLayoutPtr()->GetCreateInfoPtr()->pDescriptorSetLayoutBindings;

	for (uint32_t i = 0; i < count; ++i)
	{
		if (pBindings[i].binding == binding)
		{
			_descriptorsData[i].needRewrite = true;
			auto &&a = std::get<std::vector<DescriptorTextureData>>(_descriptorsData[i].data);
			ranges::copy(textures, a.begin() + startIndex);
			break;
		}
	}
	// for (auto &&e : textures)
	//{
	//	e.texture->addStatusListener(
	//		std::bind(&DescriptorSetData::descriptorStatusListener,
	//				  this, std::placeholders::_1, std::placeholders::_2));
	// }
	setAllStatus(GPUResource::Status::DESTROYED);
}
void DescriptorSetData::prepareImpl(int index)
{
	// index >= 0
	index = std::min(index, (int)_gpuResourceCount - 1);

	auto descriptorCount = _descriptorsData.size();

	// write data
	std::vector<Shit::WriteDescriptorSet> writes;
	std::vector<std::vector<Shit::DescriptorImageInfo>> imagesInfo(descriptorCount);
	std::vector<std::vector<Shit::DescriptorBufferInfo>> buffersInfo(descriptorCount);

	for (size_t j = 0; j < descriptorCount; ++j)
	{
		auto &&descriptorData = _descriptorsData[j];
		if (!descriptorData.needRewrite)
			return;

		if (auto p1 = std::get_if<std::vector<BufferView>>(&descriptorData.data))
		{
			auto count = p1->size();
			buffersInfo[j].resize(count);
			for (size_t i = 0; i < count; ++i)
			{
				auto &&bufferView = (*p1)[i];
				bufferView.buffer->upload(index);
				buffersInfo[j][i] = {
					bufferView.buffer->getGpuBufferByIndex(index),
					bufferView.offset,
					bufferView.size()};
			}
			writes.emplace_back(Shit::WriteDescriptorSet{
				_handles.at(index),
				descriptorData.bindingSlot,
				0, // descriptorData.startIndex,
				(uint32_t)count,
				descriptorData.descriptorType,
				0,
				buffersInfo[j].data()});
		}
		else if (auto p2 = std::get_if<std::vector<DescriptorTextureData>>(&descriptorData.data))
		{
			auto count = p2->size();
			imagesInfo[j].resize(count);
			for (size_t i = 0; i < count; ++i)
			{
				auto &&tex = (*p2)[i];
				tex.texture->upload(index);
				imagesInfo[j][i] = {
					Root::getSingleton().getTextureManager()->createOrRetrieveSampler(
						tex.texture->getSamplerInfo()),
					tex.texture->getGpuImageView(
						tex.viewType,
						tex.subresourceRange,
						tex.format,
						tex.components),
					tex.imageLayout};
				// tex.texture->getImageCreateInfo()->initialLayout};
			}
			writes.emplace_back(Shit::WriteDescriptorSet{
				_handles.at(index),
				descriptorData.bindingSlot,
				0, // descriptorData.startIndex,
				(uint32_t)count,
				descriptorData.descriptorType,
				imagesInfo[j].data()});
		}
	}
	if (!writes.empty())
	{
		Root::getSingleton().getDevice()->UpdateDescriptorSets(writes);
	}
}
void DescriptorSetData::upload(int index)
{
	if (index < 0)
	{
		for (uint32_t i = 0; i < _gpuResourceCount; ++i)
		{
			upload(i);
		}
		return;
	}
	index = (std::min)(_gpuResourceCount - 1, (uint32_t)index);
	if (_statuses[index] != GPUResource::Status::PREPARED)
		prepare(index);

	// upload all buffers and textures
	ranges::for_each(
		_descriptorsData,
		[index](DescriptorData const &data)
		{
			if (auto p = std::get_if<std::vector<BufferView>>(&data.data))
			{
				ranges::for_each(*p, [index](BufferView const &bufferView)
								 {
									 if (bufferView.buffer)
										 bufferView.buffer->upload(index); });
			}
			else if (auto p2 = std::get_if<std::vector<DescriptorTextureData>>(&data.data))
			{
				ranges::for_each(*p2, [index](DescriptorTextureData const &tex)
								 {
									 if (tex.texture)
										 tex.texture->upload(index); });
			}
		});
}
void DescriptorSetData::descriptorStatusListener(GPUResource::Status status, int index)
{
	if (index == -1)
	{
		if (status == GPUResource::Status::PREPARED && _statuses[0] != GPUResource::Status::DESTROYED)
			setAllStatus(GPUResource::Status::PREPARED);
	}
	else
	{
		if (status == GPUResource::Status::PREPARED && _statuses[index] != GPUResource::Status::DESTROYED)
			setStatus(index, GPUResource::Status::PREPARED);
	}
}
// void DescriptorSetData::descriptorUploadCallback(GPUResource const *)
//{
// }
//========================================
DescriptorManager::DescriptorManager()
{
	// ranges::fill(_curBindingDescriptorSetData, INVALID_ID);

	// create descriptor pool
	// create a large descriptor pool, donot recommend
	Shit::DescriptorPoolSize poolsize[]{
		{Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1000},
		{Shit::DescriptorType::STORAGE_IMAGE, 500},
		{Shit::DescriptorType::UNIFORM_BUFFER, 1000},
		{Shit::DescriptorType::UNIFORM_BUFFER_DYNAMIC, 1000},
	};
	_descriptorPool = Root::getSingleton().getDevice()->Create(Shit::DescriptorPoolCreateInfo{
		500, std::size(poolsize), poolsize});
}
// void DescriptorManager::bindDescriptorSetData(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex, IdType descriptorSetDataId)
// {
// 	static_cast<DescriptorSetData const *>(get(descriptorSetDataId))->bind(cmdBuffer, frameIndex);
// }
void DescriptorManager::allocateDescriptorSets(
	const Shit::DescriptorSetAllocateInfo &allocateInfo, Shit::DescriptorSet **ppDst)
{
	_descriptorPool->Allocate(allocateInfo, ppDst);
}
void DescriptorManager::freeDescriptorSets(std::span<Shit::DescriptorSet const *const> sets)
{
	_descriptorPool->Free((uint32_t)ranges::size(sets), ranges::data(sets));
}
DescriptorSetData *DescriptorManager::createDescriptorSetData(
	Shit::DescriptorSetLayout const *descriptorSetLayout,
	bool frameDependent,
	std::string_view group)
{
	auto ret = new DescriptorSetData(this, descriptorSetLayout, group, frameDependent);
	add(ret);
	return ret;
}