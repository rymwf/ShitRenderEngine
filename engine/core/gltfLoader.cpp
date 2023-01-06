#include "gltfLoader.hpp"
#include "root.hpp"
#include "scene.hpp"
#include "mesh.hpp"
#include "buffer.hpp"
#include "archiveManager.hpp"
#include "sceneManager.hpp"
#include "animation.hpp"
#include "modelResource.hpp"
#include "resourceGroup.hpp"
#include "image.hpp"
#include "texture.hpp"
#include "material.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include <tiny_gltf.h>

static std::string s_primitiveAttributeNames[]{
	"POSITION",
	"NORMAL",
	"TANGENT",
	"TEXCOORD_0",
	"COLOR_0",
	"JOINTS_0",
	"WEIGHTS_0",
};

static Buffer *s_vertexBuffer;
static Buffer *s_indexBuffer;
static tinygltf::Model *pGltfModel;
static BufferManager *s_bufferManager;
static ModelResource *s_dstModel;
static std::vector<SceneNode *> s_sceneNodes;
static bool s_isStatic = true;

bool loadModelGLTF(tinygltf::Model &model, const char *filename);
void loadModel();

// void parseBuffer();
/**
 * @brief because current gpu donot support VK_EXT_extended_dynamic_state,
 * 		set all vertex attributes to have the same data type
 */
void parseMeshes();
void parseNodes();
void parseNode(int nodeIndex, SceneNode *parentNode);
void parseAnimations();
void parseMaterials();
void parseImages();
void parseTextures();
void parseSkins();

static VertexAttributeType mapPrimitiveAttributeType(std::string_view name)
{
	for (int i = 0, len = sizeof(s_primitiveAttributeNames) / sizeof(s_primitiveAttributeNames[0]);
		 i < len; ++i)
	{
		if (name == s_primitiveAttributeNames[i])
			return static_cast<VertexAttributeType>(i);
	}
	THROW("unknown attribute", name);
	return {};
};

Shit::Format getFormat(uint32_t type, uint32_t componentNum, bool normalized)
{
	if (normalized)
	{
		switch (componentNum)
		{
		case 0:
		case 1:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8_UNORM;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16_UNORM;
			default:
				break;
			}
			break;
		case 2:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8G8_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8G8_UNORM;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16G16_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16G16_UNORM;
			default:
				break;
			}
			break;
		case 3:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8G8B8_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8G8B8_UNORM;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16G16B16_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16G16B16_UNORM;
			default:
				break;
			}
			break;
		case 4:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8G8B8A8_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8G8B8A8_UNORM;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16G16B16A16_SNORM;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16G16B16A16_UNORM;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	else
	{
		switch (componentNum)
		{
		case 0:
		case 1:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return Shit::Format::R32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return Shit::Format::R32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return Shit::Format::R32_SFLOAT;
			default:
				break;
			}
			break;
		case 2:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8G8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8G8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16G16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16G16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return Shit::Format::R32G32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return Shit::Format::R32G32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return Shit::Format::R32G32_SFLOAT;
			default:
				break;
			}
			break;
		case 3:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8G8B8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8G8B8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16G16B16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16G16B16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return Shit::Format::R32G32B32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return Shit::Format::R32G32B32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return Shit::Format::R32G32B32_SFLOAT;
			default:
				break;
			}
			break;
		case 4:
			switch (type)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				return Shit::Format::R8G8B8A8_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return Shit::Format::R8G8B8A8_UINT;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				return Shit::Format::R16G16B16A16_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return Shit::Format::R16G16B16A16_UINT;
			case TINYGLTF_COMPONENT_TYPE_INT:
				return Shit::Format::R32G32B32A32_SINT;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return Shit::Format::R32G32B32A32_UINT;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return Shit::Format::R32G32B32A32_SFLOAT;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	ST_THROW("failed to faind appropriate format");
}

Shit::IndexType getIndexType(int componentType)
{
	switch (componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		return Shit::IndexType::UINT8;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		return Shit::IndexType::UINT16;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		return Shit::IndexType::UINT32;
	}
	THROW("unknown index type")
	return Shit::IndexType::NONE;
}
InterpolationType getInterpolationType(std::string_view interpolation)
{
	if (interpolation == "LINEAR")
		return InterpolationType::LINEAR;
	else if (interpolation == "STEP")
		return InterpolationType::STEP;
	else if (interpolation == "CUBICSPLINE")
		return InterpolationType::CUBICSPLINE;
	LOG("unkown interpolation type ", interpolation)
	return InterpolationType::LINEAR;
}
TransformPath getTransformPath(std::string_view path)
{
	if (path == "translation")
		return TransformPath::TRANSLATION;
	else if (path == "scale")
		return TransformPath::SCALE;
	else if (path == "rotation")
		return TransformPath::ROTATION;
	else if (path == "weights")
		return TransformPath::WEIGTHS;
	THROW("unknown transform path ", path)
	return TransformPath::TRANSLATION;
}

TextureInterpolation getTextureInterpolation(int magFilter, int minFilter)
{
	if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
		return TextureInterpolation::NEAREST;
	else if (minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR)
		return TextureInterpolation::CUBIC;
	else
		return TextureInterpolation::LINEAR;
}
Shit::SamplerWrapMode getSamplerWrapMode(int wrap)
{
	switch (wrap)
	{
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		return Shit::SamplerWrapMode::CLAMP_TO_EDGE;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		return Shit::SamplerWrapMode::MIRRORED_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
		return Shit::SamplerWrapMode::REPEAT;
	}
	return Shit::SamplerWrapMode::CLAMP_TO_EDGE;
}

bool loadModelGLTF(tinygltf::Model &model, const char *filename)
{
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!warn.empty())
	{
		std::cout << "WARN: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cout << "ERR: " << err << std::endl;
	}

	if (!res)
		std::cout << "Failed to load glTF: " << filename << std::endl;
	else
		std::cout << "Loaded glTF: " << filename << std::endl;

	return res;
}

template <typename SrcT, typename DstT>
void getAccessorValuesHelper(const tinygltf::Model &model,
							 const tinygltf::Accessor &accessor,
							 std::vector<DstT> &values)
{
	auto &bufferView = model.bufferViews[accessor.bufferView];
	auto &buffer = model.buffers[bufferView.buffer];

	// auto len = values.size();
	auto componentNum = tinygltf::GetNumComponentsInType(accessor.type);
	values.resize(accessor.count * componentNum);
	using T = std::vector<DstT>::value_type;

	auto p = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
	auto p2 = p;
	for (size_t i = 0, j; i < accessor.count; ++i)
	{
		p2 = p;
		for (j = 0; j < componentNum; ++j)
		{
			if (accessor.normalized)
				values[i][j] = static_cast<const T>(intToFloat(*reinterpret_cast<const SrcT *>(p2)));
			else
				values[i][j] = *reinterpret_cast<const T *>(p2);
			p2 += sizeof(SrcT);
		}
		if (bufferView.byteStride)
			p += bufferView.byteStride;
		else
			p = p2;
	}
}

template <typename T>
void getAccessorValues(const tinygltf::Model &model,
					   const tinygltf::Accessor &accessor,
					   std::vector<T> &values)
{
	values.clear();
	switch (accessor.componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		getAccessorValuesHelper<char, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		getAccessorValuesHelper<unsigned char, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		getAccessorValuesHelper<short, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		getAccessorValuesHelper<unsigned short, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_INT:
		getAccessorValuesHelper<int, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		getAccessorValuesHelper<unsigned int, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_FLOAT:
		getAccessorValuesHelper<float, T>(model, accessor, values);
		break;
	case TINYGLTF_COMPONENT_TYPE_DOUBLE:
		getAccessorValuesHelper<double, T>(model, accessor, values);
		break;
	}
}

// convert all value to float, only apply to vertex data
void getAccessorValues(tinygltf::Model *pModel,
					   const tinygltf::Accessor &accessor,
					   std::vector<float> &values,
					   int dstComponentNum,
					   std::vector<float> &minValues,
					   std::vector<float> &maxValues)
{
	auto &bufferView = pModel->bufferViews[accessor.bufferView];
	auto &buffer = pModel->buffers[bufferView.buffer];
	auto pSrcData = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
	auto srcComponentNum = tinygltf::GetNumComponentsInType(accessor.type);
	auto srcComponentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	auto stride = std::max((size_t)srcComponentNum * srcComponentSize, bufferView.byteStride);

	values.resize(accessor.count * dstComponentNum, 0);
	minValues.clear();
	maxValues.clear();

	for (auto e : accessor.maxValues)
		maxValues.emplace_back((float)e);
	for (auto e : accessor.minValues)
		minValues.emplace_back((float)e);

	auto componenntNum = (std::min)(srcComponentNum, dstComponentNum);
	auto p = pSrcData;
	for (size_t i = 0; i < accessor.count; ++i, pSrcData += stride)
	{
		p = pSrcData;
		for (int j = 0; j < componenntNum; ++j, p += srcComponentSize)
		{
			auto &val = values[i * dstComponentNum + j];
			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				val = accessor.normalized ? intToFloat(*(char *)p) : *(char *)p;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				val = accessor.normalized ? intToFloat(*(unsigned char *)p) : *(unsigned char *)p;
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				val = accessor.normalized ? intToFloat(*(short *)p) : *(short *)p;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				val = accessor.normalized ? intToFloat(*(unsigned short *)p) : *(unsigned short *)p;
				break;
			case TINYGLTF_COMPONENT_TYPE_INT:
				val = accessor.normalized ? intToFloat(*(int *)p) : *(int *)p;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				val = accessor.normalized ? intToFloat(*(unsigned int *)p) : *(unsigned int *)p;
				THROW("gltfLoader::getAccessorValues, current accessor may refer to indices, do not use this function")
				break;
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				val = *(float *)p;
				break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
				val = *(double *)p;
				break;
			default:
			{
				THROW("unknown accessor type", (int)accessor.componentType)
			}
			}
		}
	}
}

Shit::PrimitiveTopology mapTopology(int mode)
{
	static Shit::PrimitiveTopology a[]{
		Shit::PrimitiveTopology::POINT_LIST,	 // TINYGLTF_MODE_POINTS (0)
		Shit::PrimitiveTopology::LINE_LIST,		 // TINYGLTF_MODE_LINE (1)
		Shit::PrimitiveTopology::LINE_LIST,		 // TINYGLTF_MODE_LINE_LOOP (2)
		Shit::PrimitiveTopology::LINE_STRIP,	 // TINYGLTF_MODE_LINE_STRIP (3)
		Shit::PrimitiveTopology::TRIANGLE_LIST,	 // TINYGLTF_MODE_TRIANGLES (4)
		Shit::PrimitiveTopology::TRIANGLE_STRIP, // TINYGLTF_MODE_TRIANGLE_STRIP (5)
		Shit::PrimitiveTopology::TRIANGLE_FAN,	 // TINYGLTF_MODE_TRIANGLE_FAN (6)
	};
	return a[mode];
}

void loadModel()
{
	auto &&resourceName = s_dstModel->getName();
	// auto s = s_resource->open();
	auto fullpath = s_dstModel->getFullPath();

	// s->seekg(0, std::ios::end);
	// auto len = s->tellg();
	// char *data;
	// s->get(data, len);
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(pGltfModel, &err, &warn, s_dstModel->getFullPath());

	if (!warn.empty())
	{
		std::cout << "WARN: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cout << "ERR: " << err << std::endl;
	}

	if (!res)
		std::cout << "Failed to load glTF: " << fullpath << std::endl;
	else
		std::cout << "Loaded glTF: " << fullpath << std::endl;
}
void GLTFModelLoader::loadResource(Resource *resource)
{
	s_dstModel = static_cast<ModelResource *>(resource);
	tinygltf::Model gltfModel;
	pGltfModel = &gltfModel;

	loadModel();

	// s_isStatic = pGltfModel->animations.empty();
	s_isStatic = false;
	s_bufferManager = Root::getSingleton().getBufferManager();

	s_vertexBuffer = s_bufferManager->createOrRetriveBuffer(
		{Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
		 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT});
	s_indexBuffer = s_bufferManager->createOrRetriveBuffer(
		{Shit::BufferUsageFlagBits::INDEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
		 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT});

	parseImages();
	parseTextures();
	parseMaterials();
	parseMeshes();
	parseNodes();
	parseSkins();
	parseAnimations();
}

void transformPrimitive(tinygltf::Primitive *pGltfPrimitive, Primitive *pDstPrimitive)
{
	std::vector<float> values;
	std::vector<float> minValues;
	std::vector<float> maxValues;
	static std::string keyname;
	keyname = "POSITION";
	if (pGltfPrimitive->attributes.contains(keyname))
	{
		auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
		getAccessorValues(pGltfModel, accessor, values, 3, minValues, maxValues);

		pDstPrimitive->position = std::make_unique<Primitive::AttributeDescription>(
			s_vertexBuffer->allocate(3 * sizeof(float), accessor.count, values.data()),
			DataType::VEC3, minValues, maxValues);
		// Shit::Format::R32G32B32_SFLOAT);
	}
	keyname = "NORMAL";
	if (pGltfPrimitive->attributes.contains(keyname))
	{
		auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
		getAccessorValues(pGltfModel, accessor, values, 3, minValues, maxValues);
		pDstPrimitive->normal = std::make_unique<Primitive::AttributeDescription>(
			s_vertexBuffer->allocate(3 * sizeof(float), accessor.count, values.data()),
			DataType::VEC3, minValues, maxValues);
		// Shit::Format::R32G32B32_SFLOAT);
	}
	keyname = "TANGENT";
	if (pGltfPrimitive->attributes.contains(keyname))
	{
		auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
		getAccessorValues(pGltfModel, accessor, values, 4, minValues, maxValues);
		pDstPrimitive->tangent = std::make_unique<Primitive::AttributeDescription>(
			s_vertexBuffer->allocate(4 * sizeof(float), accessor.count, values.data()),
			DataType::VEC4, minValues, maxValues);
		// Shit::Format::R32G32B32A32_SFLOAT);
	}
	for (int i = 0, j = 0; i < 10; ++i)
	{
		j = 10;
		keyname = "TEXCOORD_" + std::to_string(i);
		if (pGltfPrimitive->attributes.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 2, minValues, maxValues);

			static bool flipY = (Root::getSingleton().getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::GL;
			if (flipY)
			{
				for (size_t k = 0, l = values.size(); k < l; k += 2)
				{
					values[k + 1] = 1 - values[k + 1];
				}
			}
			pDstPrimitive->texcoords.emplace_back(Primitive::AttributeDescription(
				s_vertexBuffer->allocate(2 * sizeof(float), accessor.count, values.data()),
				DataType::VEC2, minValues, maxValues));
			// Shit::Format::R32G32_SFLOAT));
			j = 0;
		}
		keyname = "COLOR_" + std::to_string(i);
		if (pGltfPrimitive->attributes.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 4, minValues, maxValues);
			pDstPrimitive->colors.emplace_back(Primitive::AttributeDescription(
				s_vertexBuffer->allocate(4 * sizeof(float), accessor.count, values.data()),
				DataType::VEC4, minValues, maxValues));
			// Shit::Format::R32G32B32A32_SFLOAT));
			j = 0;
		}
		keyname = "JOINTS_" + std::to_string(i);
		if (pGltfPrimitive->attributes.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 4, minValues, maxValues);
			pDstPrimitive->joints.emplace_back(Primitive::AttributeDescription(
				s_vertexBuffer->allocate(4 * sizeof(float), accessor.count, values.data()),
				DataType::VEC4, minValues, maxValues));
			// Shit::Format::R32G32B32A32_SFLOAT));
			j = 0;
		}
		keyname = "WEIGHTS_" + std::to_string(i);
		if (pGltfPrimitive->attributes.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[pGltfPrimitive->attributes.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 4, minValues, maxValues);
			pDstPrimitive->weights.emplace_back(Primitive::AttributeDescription(
				s_vertexBuffer->allocate(4 * sizeof(float), accessor.count, values.data()),
				DataType::VEC4, minValues, maxValues));
			// Shit::Format::R32G32B32A32_SFLOAT));
			j = 0;
		}
		i += j;
	}
	if (pGltfPrimitive->indices >= 0)
	{
		auto accessor = pGltfModel->accessors[pGltfPrimitive->indices];
		auto &bufferView = pGltfModel->bufferViews[accessor.bufferView];
		auto &buffer = pGltfModel->buffers[bufferView.buffer];
		auto pSrcData = buffer.data.data() + accessor.byteOffset + bufferView.byteOffset;
		uint32_t maxValue{}, minValue{};
		maxValue = accessor.maxValues.empty() ? 0 : accessor.maxValues[0];
		minValue = accessor.minValues.empty() ? 0 : accessor.minValues[0];
		if (getIndexType(accessor.componentType) == Shit::IndexType::UINT8)
		{
			std::vector<uint16_t> data(accessor.count);
			for (size_t i = 0; i < accessor.count; ++i)
				data[i] = pSrcData[i];
			pDstPrimitive->index = std::make_unique<Primitive::IndexDescription>(
				s_indexBuffer->allocate(2, accessor.count, pSrcData), Shit::IndexType::UINT16, minValue, maxValue);
		}
		else
		{
			pDstPrimitive->index = std::make_unique<Primitive::IndexDescription>(
				s_indexBuffer->allocate(
					tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type),
					accessor.count, pSrcData),
				getIndexType(accessor.componentType), minValue, maxValue);
		}
	}
	// targets
	pDstPrimitive->targets.reserve(pGltfPrimitive->targets.size());
	for (auto &&e : pGltfPrimitive->targets)
	{
		auto pDstTarget = pDstPrimitive->targets.emplace_back(std::make_unique<Primitive::Target>()).get();
		keyname = "POSITION";
		if (e.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[e.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 3, minValues, maxValues);

			pDstTarget->position = std::make_unique<Primitive::AttributeDescription>(
				s_vertexBuffer->allocate(3 * sizeof(float), accessor.count, values.data()),
				DataType::VEC3, minValues, maxValues);
			// Shit::Format::R32G32B32_SFLOAT);
		}
		keyname = "NORMAL";
		if (e.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[e.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 3, minValues, maxValues);
			pDstTarget->normal = std::make_unique<Primitive::AttributeDescription>(
				s_vertexBuffer->allocate(3 * sizeof(float), accessor.count, values.data()),
				DataType::VEC3, minValues, maxValues);
			// Shit::Format::R32G32B32_SFLOAT);
		}
		keyname = "TANGENT";
		if (e.contains(keyname))
		{
			auto accessor = pGltfModel->accessors[e.at(keyname)];
			getAccessorValues(pGltfModel, accessor, values, 4, minValues, maxValues);
			pDstTarget->tangent = std::make_unique<Primitive::AttributeDescription>(
				s_vertexBuffer->allocate(4 * sizeof(float), accessor.count, values.data()),
				DataType::VEC4, minValues, maxValues);
			// Shit::Format::R32G32B32A32_SFLOAT);
		}
		for (int i = 0, j = 0; i < 10; ++i)
		{
			j = 10;
			keyname = "TEXCOORD_" + std::to_string(i);
			if (e.contains(keyname))
			{
				auto accessor = pGltfModel->accessors[e.at(keyname)];
				getAccessorValues(pGltfModel, accessor, values, 2, minValues, maxValues);
				pDstTarget->texcoords.emplace_back(Primitive::AttributeDescription(
					s_vertexBuffer->allocate(2 * sizeof(float), accessor.count, values.data()),
					DataType::VEC2, minValues, maxValues));
				// Shit::Format::R32G32_SFLOAT));
				j = 0;
			}
			keyname = "COLOR_" + std::to_string(i);
			if (e.contains(keyname))
			{
				auto accessor = pGltfModel->accessors[e.at(keyname)];
				getAccessorValues(pGltfModel, accessor, values, 4, minValues, maxValues);
				pDstTarget->colors.emplace_back(Primitive::AttributeDescription(
					s_vertexBuffer->allocate(4 * sizeof(float), accessor.count, values.data()),
					DataType::VEC4, minValues, maxValues));
				// Shit::Format::R32G32B32A32_SFLOAT));
				j = 0;
			}
			i += j;
		}
	}

	if (pGltfPrimitive->material >= 0)
	{
		// has material
		pDstPrimitive->materialDataBlock = s_dstModel->getMaterialDataBlockByIndex(pGltfPrimitive->material);
	}
	else
	{
		pDstPrimitive->materialDataBlock = Root::getSingleton().getMaterialDataBlockManager()->getDefaultMaterialBlock(MaterialType::PBR);
	}
}

void parseMeshes()
{
	auto meshManager = Root::getSingleton().getMeshManager();

	for (auto &&mesh : pGltfModel->meshes)
	{
		auto newMesh = meshManager->createMesh(mesh.name);
		s_dstModel->addMesh(newMesh);
		newMesh->setWeights(mesh.weights);

		for (auto &&primitive : mesh.primitives)
		{
			if (primitive.attributes.contains("POSITION"))
			{
				auto newPrimitive = newMesh->addPrimitive(mapTopology(primitive.mode));
				transformPrimitive(&primitive, newPrimitive);
			}
		}
		newMesh->init();
	}
}

void transformSceneNode(tinygltf::Node *pNode, SceneNode *pDstNode)
{
	Transform t{};
	if (!pNode->translation.empty())
		t.translation = {pNode->translation[0], pNode->translation[1], pNode->translation[2]};
	if (!pNode->scale.empty())
		t.scale = {pNode->scale[0], pNode->scale[1], pNode->scale[2]};
	if (!pNode->rotation.empty())
		t.rotation = {(float)pNode->rotation[3], (float)pNode->rotation[0], (float)pNode->rotation[1], (float)pNode->rotation[2]}; // xyzw
	if (!pNode->matrix.empty())
	{
		glm::mat4 m;
		for (int i = 0, j = 0; i < 4; ++i)
		{
			for (j = 0; j < 4; ++j)
				m[i][j] = pNode->matrix[i * 4 + j];
		}
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(m, t.scale, t.rotation, t.translation, skew, perspective);
	}
	pDstNode->setInitialState(t);
	pDstNode->resetToInitialState();
}
void parseNodes()
{
	s_sceneNodes.resize(pGltfModel->nodes.size(), 0);
	auto rootNode = s_dstModel->getModel();

	//==================
	for (int i = 0, l = pGltfModel->skins.size(); i < l; ++i)
	{
		auto &&gltfSkin = pGltfModel->skins[i];
		rootNode->createSkin(gltfSkin.joints.size(), gltfSkin.joints.data());
	}

	//======
	for (size_t i = 0; i < pGltfModel->scenes.size(); ++i)
	{
		std::string name;
		auto &&e = pGltfModel->scenes[i];
		if (!e.name.empty())
		{
			name = e.name;
		}
		else
		{
			name = s_dstModel->getName();
			name += std::to_string(i);
		}

		for (auto &&nodeIndex : e.nodes)
			parseNode(nodeIndex, rootNode);

		if (pGltfModel->defaultScene >= 0 && i != pGltfModel->defaultScene)
		{
			static_cast<SceneNode *>(rootNode->getChild(i))->enable(false);
		}
	}
}
void parseNode(int nodeIndex, SceneNode *parentNode)
{
	auto node = pGltfModel->nodes[nodeIndex];
	auto sceneNode = static_cast<SceneNode *>(parentNode->createChild(node.name, !s_isStatic));

	s_dstModel->getModel()->setChildIndex(sceneNode, nodeIndex);

	transformSceneNode(&node, sceneNode);
	if (node.mesh >= 0)
	{
		auto meshView = sceneNode->addComponent<MeshView>(s_dstModel->getMeshByIndex(node.mesh));
		if (node.skin >= 0)
		{
			meshView->setSkin(s_dstModel->getModel()->getSkin(node.skin));
		}
	}
	s_sceneNodes[nodeIndex] = sceneNode;
	for (auto &&childIndex : node.children)
	{
		parseNode(childIndex, sceneNode);
	}
}

void parseAnimations()
{
	if (pGltfModel->animations.empty())
		return;
	auto animation = s_dstModel->getModel()->addComponent<Animation>();

	// process animations
	std::vector<float> times;
	std::vector<float> values;
	std::vector<float> maxValues;
	std::vector<float> minValues;

	for (auto &&gltfAnimation : pGltfModel->animations)
	{
		auto pAnimationClip = s_dstModel->createAnimationClip(gltfAnimation.name);
		auto pAnimationClipView = animation->addClip(pAnimationClip);

		for (auto &&gltfChannel : gltfAnimation.channels)
		{
			auto sceneNode = s_sceneNodes[gltfChannel.target_node];
			if (!sceneNode)
			{
				THROW("scenenode ", gltfChannel.target_node, " is null")
			}
			auto transformPath = getTransformPath(gltfChannel.target_path);
			auto &&sampler = gltfAnimation.samplers[gltfChannel.sampler];
			auto interpolationType = getInterpolationType(sampler.interpolation);
			if (interpolationType == InterpolationType::CUBICSPLINE)
			{
				THROW("interpolation type is cubic spline")
			}

			getAccessorValues(pGltfModel, pGltfModel->accessors[sampler.input], times, 1, minValues, maxValues);
			switch (transformPath)
			{
			case TransformPath::TRANSLATION:
				getAccessorValues(pGltfModel, pGltfModel->accessors[sampler.output], values, 3, minValues, maxValues);
				break;
			case TransformPath::ROTATION:
				getAccessorValues(pGltfModel, pGltfModel->accessors[sampler.output], values, 4, minValues, maxValues);
				break;
			case TransformPath::SCALE:
				getAccessorValues(pGltfModel, pGltfModel->accessors[sampler.output], values, 3, minValues, maxValues);
				break;
			case TransformPath::WEIGTHS:
				getAccessorValues(pGltfModel, pGltfModel->accessors[sampler.output], values, 1, minValues, maxValues);
				break;
			}
			pAnimationClipView->createAnimationTrackChannel(
				sceneNode, transformPath, interpolationType, times, values);
		}
	}
}
void parseSkins()
{
	// mat4 in gltf is column major
	std::vector<float> values;
	std::vector<float> minValues;
	std::vector<float> maxValues;
	glm::mat4 m;
	auto pModel = s_dstModel->getModel();

	for (int i = 0, l = pGltfModel->skins.size(); i < l; ++i)
	{
		auto &&gltfSkin = pGltfModel->skins[i];

		getAccessorValues(pGltfModel, pGltfModel->accessors[gltfSkin.inverseBindMatrices], values, 16, minValues, maxValues);
		for (int j = 0, len = gltfSkin.joints.size(); j < len; ++j)
		{
			memcpy(&m, &values[j * 16], sizeof(float) * 16);
			s_sceneNodes[gltfSkin.joints[j]]->addComponent<Joint>(j, m, i);
		}
	}
}
void parseImages()
{
	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
	auto &&parentPath = std::filesystem::path(s_dstModel->getName()).parent_path().string();
	for (auto &&e : pGltfModel->images)
	{
		if (!e.uri.empty())
		{
			s_dstModel->addImage(static_cast<Image *>(grp->createResource(
				ResourceDeclaration{
					parentPath + "/" + e.uri,
					ResourceType::IMAGE})));
		}
		else
		{
			auto bufferView = pGltfModel->bufferViews[e.bufferView];
			auto buffer = pGltfModel->buffers[bufferView.buffer];
			auto p = buffer.data.data() + bufferView.byteOffset;
			auto size = bufferView.byteLength;

			ParameterMap paras{
				{"is_file_data", "1"},
			};
			auto pImage = static_cast<Image *>(grp->createResource(
				ResourceDeclaration{
					e.name,
					ResourceType::IMAGE,
					0,
					paras},
				size, p));
			s_dstModel->addImage(pImage);
		}
	}
}
void parseTextures()
{
	auto mgr = Root::getSingleton().getTextureManager();

	for (auto &&e : pGltfModel->textures)
	{
		auto samplerInfo = SamplerInfo{TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, false};
		if (e.sampler >= 0)
		{
			auto sampler = pGltfModel->samplers[e.sampler];
			samplerInfo.texInterpolation = getTextureInterpolation(sampler.magFilter, sampler.minFilter);
			samplerInfo.wrapmode = getSamplerWrapMode(sampler.wrapS);
		}
		//
		auto image = s_dstModel->getImageByIndex(e.source);
		auto tex = mgr->create(
			Shit::ImageType::TYPE_2D,
			Shit::ImageUsageFlagBits::SAMPLED_BIT |
				Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
			{&image, 1},
			samplerInfo,
			e.name);
		s_dstModel->addTexture(tex);
	}
}
void parseMaterials()
{
	static const std::string alphaModes[]{"OPAQUE", "MASK", "BLEND"};
	auto mgr = Root::getSingleton().getMaterialDataBlockManager();

	for (auto &&e : pGltfModel->materials)
	{
		MaterialPBR::Para materialPara{};
		auto materialDataBlock = static_cast<MaterialDataBlockPBR *>(mgr->createMaterialDataBlock(MaterialType::PBR));
		Texture *tex;
		size_t textureIndex = 0;
		// RGB space, no A
		if (e.normalTexture.index >= 0)
		{
			//
			materialPara.normalTextureScale = e.normalTexture.scale;
			materialPara.normalTextureIndex = textureIndex;
			++textureIndex;
			tex = s_dstModel->getTextureByIndex(e.normalTexture.index);
			materialDataBlock->setDescriptorTexture(
				materialPara.normalTextureIndex,
				DescriptorTextureData{
					tex,
					Shit::ImageViewType::TYPE_2D,
					Shit::Format::R8G8B8A8_UNORM,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					tex->getSamplerInfo()});
		}
		// R channel linear
		if (e.occlusionTexture.index >= 0)
		{
			materialPara.occlusionTextureStrength = e.occlusionTexture.strength;
			materialPara.occlusionTextureIndex = textureIndex;
			++textureIndex;

			tex = s_dstModel->getTextureByIndex(e.occlusionTexture.index);
			materialDataBlock->setDescriptorTexture(
				materialPara.occlusionTextureIndex,
				DescriptorTextureData{
					tex,
					Shit::ImageViewType::TYPE_2D,
					Shit::Format::R8G8B8A8_UNORM,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					tex->getSamplerInfo()});
		}
		// SRGB space, RGB, no A
		if (e.emissiveTexture.index >= 0)
		{
			materialPara.emissiveTextureIndex = textureIndex;
			++textureIndex;

			tex = s_dstModel->getTextureByIndex(e.emissiveTexture.index);
			materialDataBlock->setDescriptorTexture(
				materialPara.emissiveTextureIndex,
				DescriptorTextureData{
					tex,
					Shit::ImageViewType::TYPE_2D,
					Shit::Format::R8G8B8A8_SRGB,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					tex->getSamplerInfo()});
		}
		for (int i = 0; i < 3; ++i)
			materialPara.emissiveFactor[i] = e.emissiveFactor[i];
		if (e.alphaMode == alphaModes[0])
			materialPara.alphaMode = 0; // disable blend
		else if (e.alphaMode == alphaModes[1])
			materialPara.alphaMode = 1;
		else if (e.alphaMode == alphaModes[2])
			materialPara.alphaMode = 2;

		materialPara.alphaCutoff = e.alphaCutoff;

		// pbr metallic and roughness
		for (int i = 0; i < 4; ++i)
			materialPara.baseColorFactor[i] = e.pbrMetallicRoughness.baseColorFactor[i];
		if (e.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			// srgb space
			materialPara.baseColorTextureIndex = textureIndex;
			++textureIndex;

			tex = s_dstModel->getTextureByIndex(e.pbrMetallicRoughness.baseColorTexture.index);
			auto &&samplerInfo = tex->getSamplerInfo();
			materialDataBlock->setDescriptorTexture(
				materialPara.baseColorTextureIndex,
				DescriptorTextureData{
					tex,
					Shit::ImageViewType::TYPE_2D,
					Shit::Format::R8G8B8A8_SRGB,
					{},
					{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
					Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
					tex->getSamplerInfo()});
		}
		materialPara.metallicFactor = e.pbrMetallicRoughness.metallicFactor;
		materialPara.roughnessFactor = e.pbrMetallicRoughness.roughnessFactor;

		if (e.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		{
			// metallic B channel, roughness G channel,linear, can equal to occlusion texture index
			if (e.pbrMetallicRoughness.metallicRoughnessTexture.index == e.occlusionTexture.index)
			{
				materialPara.metallicRoughnessTextureIndex = materialPara.occlusionTextureIndex;
			}
			else
			{
				materialPara.metallicRoughnessTextureIndex = textureIndex;
				++textureIndex;
				tex = s_dstModel->getTextureByIndex(e.pbrMetallicRoughness.metallicRoughnessTexture.index);
				materialDataBlock->setDescriptorTexture(
					materialPara.metallicRoughnessTextureIndex,
					DescriptorTextureData{
						tex,
						Shit::ImageViewType::TYPE_2D,
						Shit::Format::R8G8B8A8_UNORM,
						{},
						{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
						Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
						tex->getSamplerInfo()});
			}
		}
		materialDataBlock->setMaterial(materialPara);
		s_dstModel->addMateralDataBlock(materialDataBlock);
	}
}