#include "image.hpp"
#include "resourceGroup.hpp"
#include "root.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

Image::Image(std::string_view name,
			 std::string_view group,
			 bool manuallyLoad,
			 ManualResourceLoader *loader)
	: Resource(name, group, manuallyLoad, loader)
{
}
Image::Image(std::string_view name,
			 std::string_view group,
			 size_t size,
			 char const *fileData,
			 bool manuallyLoad,
			 ManualResourceLoader *loader)
	: Resource(name, group, manuallyLoad, loader)
{
	updateData(0, size, fileData);
}
Image::Image(std::string_view name,
			 std::string_view group,
			 ImageType type,
			 int width,
			 int height,
			 Shit::Format format,
			 unsigned char const *colorData,
			 bool manuallyLoad,
			 ManualResourceLoader *loader)
	: Resource(name, group, manuallyLoad, loader),
	  _width(width),
	  _height(height),
	  _imageFormat(format)
{
	if (!colorData)
		return;
	auto size = _width * _height * Shit::GetFormatSize(_imageFormat);
	_colorData.assign(colorData, colorData + size);
}
Image::~Image()
{
}
int Image::getComponentSize() const
{
	return Shit::GetFormatAttribute(_imageFormat).componentSizeR;
}
int Image::getComponentNum() const
{
	return Shit::GetFormatComponentNum(_imageFormat);
}
int Image::getFormatSize() const
{
	return Shit::GetFormatSize(_imageFormat);
}
void Image::prepareImpl()
{
	if (_colorData.empty() && _data.empty())
	{
		auto a = open(std::ios::in | std::ios::binary);
		a->seekg(0, std::ios::end);
		auto size = a->tellg();
		a->seekg(0, std::ios::beg);
		_data.resize(size);
		a->read(_data.data(), size);
		close();
	}
}
void Image::loadImpl()
{
	if (!_colorData.empty())
		return;
	int componentNum;
	int componentSize;

	if ((Root::getSingleton().getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::GL)
		stbi_set_flip_vertically_on_load(true);

	if (stbi_is_hdr_from_memory((unsigned char *)_data.data(), _data.size()))
	{
		auto p = stbi_loadf_from_memory((unsigned char *)_data.data(), _data.size(), &_width, &_height, &componentNum, _requestComponentNum);
		//_data = (char*)stbi_loadf(path.data(), &_width, &_height, &componentNum, _requestComponentNum);
		auto size = _width * _height * _requestComponentNum * sizeof(float);
		_colorData.resize(size);
		memcpy(_colorData.data(), p, size);
		stbi_image_free(p);

		_hdr = true;
		componentSize = 4;
	}
	else
	{
		auto p = stbi_load_from_memory((unsigned char *)_data.data(), _data.size(), &_width, &_height, &componentNum, _requestComponentNum);
		//_data = (char *)stbi_load(path.data(), &_width, &_height, &componentNum, _requestComponentNum);
		auto size = _width * _height * _requestComponentNum;
		_colorData.resize(size);
		memcpy(_colorData.data(), p, size);
		componentSize = 1;
	}

	//
	switch (_requestComponentNum)
	{
	case 1:
		switch (componentSize)
		{
		case 1:
			_imageFormat = Shit::Format::R8_UNORM;
			return;
		case 2:
			_imageFormat = Shit::Format::R16_UNORM;
			return;
		case 4:
			_imageFormat = Shit::Format::R32_SFLOAT;
			return;
		default:
			break;
		}
		break;
	case 2:
		switch (componentSize)
		{
		case 1:
			_imageFormat = Shit::Format::R8G8_UNORM;
			return;
		case 2:
			_imageFormat = Shit::Format::R16G16_UNORM;
			return;
		case 4:
			_imageFormat = Shit::Format::R32G32_SFLOAT;
			return;
		default:
			break;
		}
		break;
	case 3:
		switch (componentSize)
		{
		case 1:
			_imageFormat = Shit::Format::R8G8B8_UNORM;
			return;
		case 2:
			_imageFormat = Shit::Format::R16G16B16_UNORM;
			return;
		case 4:
			_imageFormat = Shit::Format::R32G32B32_SFLOAT;
			return;
		default:
			break;
		}
		break;
	case 4:
		switch (componentSize)
		{
		case 1:
			_imageFormat = Shit::Format::R8G8B8A8_UNORM;
			return;
		case 2:
			_imageFormat = Shit::Format::R16G16B16A16_UNORM;
			return;
		case 4:
			_imageFormat = Shit::Format::R32G32B32A32_SFLOAT;
			return;
		default:
			break;
		}
		break;
	default:
		break;
	}
	THROW("failed to find suitable format, componentNum:", componentNum, "componentSize:", componentSize);
}
void Image::unloadImpl()
{
	// stbi_image_free(_data);
	//_data = nullptr;
}
void Image::save()
{
	switch (_imageType)
	{
	case ImageType::PNG:
		stbi_write_png(getFullPath().data(), _width, _height, getComponentNum(), _colorData.data(), getFormatSize());
		break;
	case ImageType::BMP:
		// stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
		stbi_write_bmp(getFullPath().data(), _width, _height, getComponentNum(), _colorData.data());
		break;
	case ImageType::TGA:
		stbi_write_tga(getFullPath().data(), _width, _height, getComponentNum(), _colorData.data());
		break;
	case ImageType::JPG:
		stbi_write_jpg(getFullPath().data(), _width, _height, getComponentNum(), _colorData.data(), 90);
		break;
	case ImageType::HDR:
		stbi_write_hdr(getFullPath().data(), _width, _height, getComponentNum(), (float *)_colorData.data());
		break;
	default:
		THROW("unknown imagetype:", int(_imageType));
	};
}
void Image::flipX()
{
	auto a = _requestComponentNum * getComponentSize();
	char *temp = new char[a];
	auto p = _colorData.data();
	for (int j = 0; j < _height; ++j)
	{
		for (int i = 0, l = _width * a / 2; i <= l; i += a)
		{
			memcpy(temp, &p[j * _width + i], a);
			memcpy(&p[j * _width + i], &p[(j + 1) * _width * a - i - 1], a);
			memcpy(&p[(j + 1) * _width * a - i - 1], temp, a);
		}
	}
	delete[] temp;
}
void Image::flipY()
{
	auto a = _width * _requestComponentNum * getComponentSize();
	char *temp = new char[a];
	auto p = _colorData.data();
	for (int j = 0; j < _height; ++j)
	{
		memcpy(temp, &p[j], a);
		memcpy(&p[j], &p[(_height - j - 1) * a], a);
		memcpy(&p[(_height - j - 1) * a], temp, a);
	}
	delete[] temp;
}
// NODISCARD void *Image::getLuminance()
//{
//	void *ret =new char[_width * _height * _componentSize];
//	if (_componentSize = 1)
//	{
//		auto p = reinterpret_cast<unsigned char *>(ret);
//		for (int i = 0, j = 0; i < _width; ++i)
//		{
//			for (j = 0; j < _height; ++j)
//			{
//				p[i];
//			}
//		}
//	}
//	else
//	{
//	}
//	return ret;
// }

//======================
// ImageManager::ImageManager()
//{
//}
// ImageManager::~ImageManager() {}
// Resource *ImageManager::createImpl(
//	std::string_view name,
//	const ParameterMap *parameterMap,
//	std::string_view group,
//	bool manullyLoad,
//	ManualResourceLoader *loader)
//{
//	return new Image(this, name, group, manullyLoad, loader);
//}