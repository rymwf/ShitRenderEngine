/**
 * @file descriptorManager.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include "gpuResource.hpp"
#include "texture.hpp"

struct DescriptorTextureData
{
	//bool needRewrite;
	Texture *texture;

	// imageview
	Shit::ImageViewType viewType;
	Shit::Format format;
	Shit::ComponentMapping components;
	Shit::ImageSubresourceRange subresourceRange;
	Shit::ImageLayout imageLayout;

	// sampler
	SamplerInfo samplerInfo;
	//TextureInterpolation textureInterpolation;
	//Shit::SamplerWrapMode wrapmode;
	//bool compareEnable;
};
//struct DescriptorBufferData
//{
//	bool needRewrite;
//	BufferView bufferView;
//};
struct DescriptorData
{
	bool needRewrite; /// need update descriptor set
	uint32_t bindingSlot;
	uint32_t descriptorCount;
	Shit::DescriptorType descriptorType;
	//std::variant<std::monostate, std::vector<DescriptorBufferData>, std::vector<DescriptorTextureData>> data;
	//note!!! descriptor array must not have hole
	std::variant<std::monostate, std::vector<BufferView>, std::vector<DescriptorTextureData>> data;
};

// class DescriptorSetData : public IdObject<DescriptorSetData>
class DescriptorSetData : public GPUResource
{
public:
	DescriptorSetData(
		DescriptorManager *creator,
		Shit::DescriptorSetLayout const *setLayout,
		std::string_view group,
		bool frameDependent);

	void setBuffer(uint32_t binding, uint32_t startIndex, BufferView const &bufferView);
	void setBuffers(uint32_t binding, uint32_t startIndex, std::span<BufferView const> bufferViews);

	void setTexture(uint32_t binding, uint32_t startIndex, DescriptorTextureData const &texture);
	void setTextures(uint32_t binding, uint32_t startIndex, std::span<DescriptorTextureData const> textures);

	Shit::DescriptorSet *getHandleByIndex(uint32_t index) const;

	// constexpr uint32_t getSetSlot() const { return _setSlot; }
	// constexpr Shit::PipelineLayout const *getCompatiblePipelineLayout() const { return _compatiblePipelineLayout; }
	Shit::DescriptorSetLayoutCreateInfo const *getDescriptorSetLayoutCreateInfo() const;

	void bind(Shit::CommandBuffer *cmdBuffer,
			  Shit::PipelineBindPoint bindPoint,
			  Shit::PipelineLayout const *pipelineLayout,
			  uint32_t setSlot,
			  uint32_t frameIndex) const;

	void prepareImpl(int index) override;

	/**
	 * @brief TODO: prepare should not interact with gpu
	 * 
	 * @param index 
	 */
	//void prepare(int index = -1) override;
	void upload(int index = -1) override;

private:
	std::vector<Shit::DescriptorSet *> _handles;

	mutable std::vector<DescriptorData> _descriptorsData;

	// uint32_t _setSlot;
	// Shit::PipelineBindPoint _bindPoint;
	// Shit::PipelineLayout const *_compatiblePipelineLayout;

	// std::vector<Shit::WriteDescriptorSet> _descriptorWrites;

	void descriptorStatusListener(GPUResource::Status status, int index);
};

// class DescriptorSetDataFactory
//{
// };
struct DescriptorSetLayoutInfo
{
	// std::string name;
	uint32_t setSlot;
	Shit::PipelineBindPoint bindPoint;
	Shit::PipelineLayout *pipelineLayout;
};

class DescriptorManager : public GPUResourceManager
{
	friend class DescriptorSetData;

	// one big descriptor pool
	Shit::DescriptorPool *_descriptorPool;

public:
	DescriptorManager();

	void allocateDescriptorSets(const Shit::DescriptorSetAllocateInfo &allocateInfo, Shit::DescriptorSet **ppDst);
	void freeDescriptorSets(std::span<Shit::DescriptorSet const *const> sets);

	// void bindDescriptorSetData(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex, IdType descriptorSetDataId);

	DescriptorSetData *createDescriptorSetData(
		Shit::DescriptorSetLayout const *descriptorSetLayout,
		bool frameDependent = false,
		std::string_view group = DEFAULT_GROUP_NAME);
};
