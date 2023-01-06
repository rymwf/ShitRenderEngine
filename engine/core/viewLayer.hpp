#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"

class ViewLayer
{
	std::string _name;
	// Shit::SampleCountFlagBits _sampleCount;

	// std::vector<Texture*> _attachments;
	IdType _colorTextureId;
	IdType _depthTextureId;

	IdType _positionTextureId;
	IdType _albedoTextureId;
	IdType _normalTextureId;
	IdType _MRATextureId;
	IdType _emissionTextureId;

	IdType _shadowTextureId;

	std::vector<Shit::ClearValue> _clearValues;

	Shit::Signal<void(ViewLayer *)> _resizeViewLayerSignal;

	std::vector<std::array<Shit::Framebuffer *, (size_t)RenderPassType::Num>> _framebuffers;

	RenderPath _renderPath;

	Shit::Extent2D _extent;

	Shit::Format _colorFormat{Shit::Format::R8G8B8A8_SRGB};

	IdType _gpassDescriptorSetDataId;

	void createFramebuffers(RenderPassType renderPassType);

	void destroyFramebuffers();

	void init();

public:
	ViewLayer(Screen *screen);
	ViewLayer(Shit::Extent2D ext, Shit::Format colorFormat);
	~ViewLayer();

	Shit::Framebuffer *getFramebuffer(RenderPassType passType, uint32_t frameIndex);

	DescriptorSetData *getDeferredDescriptorSetData() const;

	void resize(Shit::Extent2D ext);

	constexpr Shit::Extent2D const &getExtent() const { return _extent; }

	constexpr Shit::Format getColorFormat() const { return _colorFormat; }

	constexpr IdType getColorTextureId() const
	{
		return _colorTextureId;
	}
	constexpr IdType getDepthTExtureId() const
	{
		return _depthTextureId;
	}

	void addResizeListener(Shit::Slot<void(ViewLayer *)> const &slot);
	void removeResizeListener(Shit::Slot<void(ViewLayer *)> const &slot);
};