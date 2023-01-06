#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include "image.hpp"
#include "gpuResource.hpp"

enum class TextureType
{
	DEFAULT,
	NORMAL,
	EDITOR_GUI,
	SPRITE,
	CURSOR,
	COOKIE,
	LIGHTMAP,
	DIRECTIONAL_LIGHTMAP,
	SHADOW_MASK,
	SINGLE_CHANNEL
};
using TextureShape = Shit::ImageType;

struct SamplerInfo
{
	TextureInterpolation texInterpolation;
	Shit::SamplerWrapMode wrapmode;
	bool compareEnable;
};
struct SamplerInfoHash
{
	size_t operator()(const SamplerInfo &s) const noexcept
	{
		auto h = std::hash<TextureInterpolation>{}(s.texInterpolation);
		hashCombine(h, std::hash<Shit::SamplerWrapMode>{}(s.wrapmode));
		hashCombine(h, std::hash<bool>{}(s.compareEnable));
		return h;
	}
};
inline bool operator==(const SamplerInfo &l, const SamplerInfo &r)
{
	return l.texInterpolation == r.texInterpolation &&
		   l.wrapmode == r.wrapmode &&
		   l.compareEnable == r.compareEnable;
}

class Texture : public GPUResource
{
	std::vector<Image *> _images{};

	mutable std::vector<Shit::Image *> _gpuImages{};

	std::unordered_map<Shit::ImageViewCreateInfo, Shit::ImageView *> _gpuImageViews{};

	Shit::Image *_stageImage{};

	Shit::ImageCreateInfo _imageCreateInfo{};

	SamplerInfo _samplerInfo{};

	void initialize();

	void uploadImpl(int index) override;
	void destroyImpl(int index) override;

public:
	Texture(GPUResourceManager *creator,
			std::string_view name,
			std::string_view group,
			Shit::ImageCreateInfo const &createInfo,
			SamplerInfo const &samplerInfo,
			bool frameDependent);

	Texture(
		GPUResourceManager *creator,
		std::string_view name,
		std::string_view group,
		Shit::ImageType type,
		Shit::ImageUsageFlagBits usage,
		std::span<Image *> images,
		SamplerInfo const &samplerInfo);

	~Texture();

	Shit::Image *getGpuImage(uint32_t frameIndex = 0)
	{
		upload();
		return _gpuImages.at(frameIndex);
	}
	constexpr Shit::ImageCreateInfo *getImageCreateInfo() { return &_imageCreateInfo; }

	/**
	 * @brief if texture is created without image, create one according to texturetype
	 *
	 * @param name
	 */
	void saveImage(std::string_view name, uint32_t index);

	Shit::ImageView *getGpuImageView(
		Shit::ImageViewType viewType,
		const Shit::ImageSubresourceRange &subresourceRange,
		Shit::Format format,
		const Shit::ComponentMapping &components = {},
		uint32_t frameIndex = 0);

	Shit::ImageView *getGpuImageView(
		Shit::ImageViewType viewType,
		const Shit::ImageSubresourceRange &subresourceRange,
		const Shit::ComponentMapping &components = {},
		uint32_t frameIndex = 0);

	Shit::Sampler *getSampler();
	constexpr SamplerInfo const &getSamplerInfo() const { return _samplerInfo; }
};

class TextureManager : public GPUResourceManager
{
public:
	TextureManager();
	~TextureManager();

	Texture *create(
		Shit::ImageType type,
		Shit::ImageUsageFlagBits usage,
		std::span<Image *> images,
		SamplerInfo const &samplerInfo = {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, false},
		std::string_view name = {},
		std::string_view group = DEFAULT_GROUP_NAME);

	Texture *create(
		Shit::ImageCreateInfo const &imageCreateInfo,
		bool frameDependent,
		SamplerInfo const &samplerInfo = {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, false},
		std::string_view name = {},
		std::string_view group = DEFAULT_GROUP_NAME);

	Texture *getDefaultTextureBlack();
	Texture *getDefaultTextureWhite();

	Shit::Sampler *createOrRetrieveSampler(SamplerInfo const &samplerInfo);

	//Shit::Sampler *createOrRetrieveSampler(Shit::SamplerCreateInfo const &samplerInfo);

	// Shit::Sampler *getSampler(std::string_view name) const;
private:
	std::unordered_map<SamplerInfo, Shit::Sampler *, SamplerInfoHash> _samplers;
};