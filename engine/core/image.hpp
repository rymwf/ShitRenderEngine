#pragma once
#include "prerequisites.hpp"
#include "resourceManager.hpp"

class ImageManager;

enum class ImageType
{
	PNG,
	BMP,
	TGA,
	JPG,
	HDR,
};

class Image : public Resource
{
	friend class ImageManager;

	std::vector<unsigned char> _colorData;

	int _width;
	int _height;
	int _requestComponentNum{4};
	bool _hdr{};
	Shit::Format _imageFormat;

	ImageType _imageType;

	void prepareImpl() override;
	void loadImpl() override;
	void unloadImpl() override;

public:
	Image(std::string_view name,
		  std::string_view group,
		  bool manuallyLoad = false,
		  ManualResourceLoader *loader = nullptr);

	Image(std::string_view name,
		  std::string_view group,
		  size_t size,
		  char const *fileData, // data is file data
		  bool manuallyLoad = false,
		  ManualResourceLoader *loader = nullptr);

	Image(std::string_view name,
		  std::string_view group,
		  ImageType type,
		  int width,
		  int height,
		  Shit::Format format,
		  unsigned char const *colorData = nullptr, // data is color data
		  bool manuallyLoad = false,
		  ManualResourceLoader *loader = nullptr);

	~Image();

	constexpr Image &setRequestComponentNum(int requestComponentNum)
	{
		_requestComponentNum = requestComponentNum;
		return *this;
	}
	std::span<unsigned char const> getColorData() const { return _colorData; }

	// void updateData(void const *data);
	void save() override;

	/**
	 * @brief need to be loaded first
	 *
	 * @return constexpr Shit::Format
	 */
	constexpr Shit::Format getFormat() const { return _imageFormat; }

	constexpr int getWidth() const { return _width; }
	constexpr int getHeight() const { return _height; }
	int getComponentSize() const;
	int getComponentNum() const;
	int getFormatSize() const;

	// around Y axis
	void flipX();

	// around X axis
	void flipY();
	// NODISCARD void *getLuminance();
};

struct ImageCreator
{
	/**
	 * @brief
	 *
	 * @param name
	 * @param parameterMap parameters
	 *	"image_type","integer value of PNG,BMP,TGA,JPG,HDR"
	 * 	"width","(integer value)"
	 * 	"height","(integer value)"
	 *	"format", "(integer value of Shit::Format)"
	 *  "is_file_data","0"
	 * @param group
	 * @param manullyLoad
	 * @param loader
	 * @param size
	 * @param data color data or filedata
	 * @return Resource*
	 */
	Resource *operator()(
		std::string_view name,
		const ParameterMap *parameterMap,
		std::string_view group,
		bool manullyLoad,
		ManualResourceLoader *loader,
		size_t size,
		void const *data) const
	{
		if (size > 0)
		{
			auto it = parameterMap->find("is_file_data");
			if (it == parameterMap->cend() || it->second == "0")
			{
				// data is color
				return new Image(
					name,
					group,
					(ImageType)std::atoi(parameterMap->at("image_type").data()),
					std::atoi(parameterMap->at("width").data()),
					std::atoi(parameterMap->at("height").data()),
					parameterMap->contains("format") ? (Shit::Format)std::atoi(parameterMap->at("format").data()) : Shit::Format::R8G8B8A8_UNORM,
					(unsigned char const *)data,
					manullyLoad,
					loader);
			}
			else
			{
				return new Image(
					name,
					group,
					size,
					(char const *)data,
					manullyLoad,
					loader);
			}
		}
		return new Image(name, group, manullyLoad, loader);
	};
};