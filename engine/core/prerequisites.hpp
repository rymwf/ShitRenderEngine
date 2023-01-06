#pragma once
#include <renderer/ShitRenderSystem.hpp>
#include "utility.hpp"
#include "enums.hpp"
#include "config.hpp"

#define NAME_NODE "NODE"
#define NAME_SCENENODE "SCENENODE"
#define NAME_CAMERA "CAMERA"

#define MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x100

#define LOCATION_POSITION 0
#define LOCATION_NORMAL 1
#define LOCATION_TANGENT 2
#define LOCATION_TEXCOORD0 3
#define LOCATION_COLOR0 4
#define LOCATION_JOINTS0 5
#define LOCATION_WEIGHTS0 6

#define LOCATION_NUM LOCATION_WEIGHTS0 + 1

// common descriptors
#define DESCRIPTORSET_CAMREA 0
#define DESCRIPTORSET_NODE 1
#define DESCRIPTORSET_SKIN 2
#define DESCRIPTORSET_LIGHT 3
#define DESCRIPTORSET_ENV 4
#define DESCRIPTORSET_MATERIAL 5

#define DESCRIPTOR_BINDING_CAMERA 0			  //buffer
#define DESCRIPTOR_BINDING_NODE 0			  //uniform
#define DESCRIPTOR_BINDING_SKIN 1			  //buffer
#define DESCRIPTOR_BINDING_LIGHT 2			  //buffer
#define DESCRIPTOR_BINDING_MATERIAL_BUFFER 1  //uniform
#define DESCRIPTOR_BINDING_MATERIAL_TEXTURE 0 //texture

#define DESCRIPTORSET_LAYOUT_CAMERA_NAME "ubo_camera"
#define DESCRIPTORSET_LAYOUT_NODE_NAME "ubo_node"
#define DESCRIPTORSET_LAYOUT_SKIN_NAME "ubo_skin"

class Component;

class Animation;
class AnimationTrack;
class KeyFrame;

class Resource;
class ResourceManager;

class Ray;
struct Sphere;
class Plane;
class Ellipsoid;
class Cylinder;
struct AxisAlignedBox;

struct BoundingVolume;

class Node;
class SceneNode;
class Joint;

class CompositorWorkspace;
class CompositorManager;

class Camera;
class Screen;

class Scene;
class SceneManager;
class Timer;
class ManualResourceLoader;

class Light;
class Image;
// class ImageManager;
class Texture;
class TextureManager;

class Buffer;
class BufferManager;

class Root;

class Mesh;
class MeshView;
class MeshManager;

class Material;
class MaterialDataBlock;
class MaterialDataBlockManager;

class ResourceGroup;
class ResourceGroupManager;

class RenderQueueGroup;

class Model;
class ModelResource;
// class ModelManager;

class Shader;
class RenderPass;
class PipelineWrapper;

class Renderable;
class Behaviour;

class DescriptorSetData;
class DescriptorManager;

class ViewLayer;

class Skin;

using ParameterMap = std::unordered_map<std::string, std::string>;

struct ResourceDeclaration
{
	std::string name;
	ResourceType type;
	ManualResourceLoader *loader;
	ParameterMap parameters;
};

// enum class TextureType
//{
//	TEX_1D,
//	TEX_2D,
//	TEX_3D,
//	TEX_CUBE,
//	TEX_1D_ARRAY,
//	TEX_2D_ARRAY,
//	TEX_CUBE_ARRAY,
// };

/**
 * @brief describe buffer object, one buffer view can only contain one type of data
 *
 */
struct BufferView // : public IdObject<BufferView>
{
	// IdType bufferId;
	Buffer *buffer;
	size_t offset;
	size_t stride;
	size_t count;
	size_t size() const { return stride * count; }
	// BufferView(IdType id, size_t offset_, size_t stride_, size_t count_)
	//	: bufferId(id), offset(offset_), stride(stride_), count(count_)
	//{
	// }
};

inline std::string getGLSLUniformTypeName(int type)
{
	static std::string strGLSLUniformTypeName[]{
		"sampler1D",
		"sampler1DShadow",
		"sampler1DArray",
		"sampler1DArrayShadow",
		"isampler1D",
		"isampler1DArray",
		"usampler1D",
		"usampler1DArray",
		"sampler2D",
		"sampler2DShadow",
		"sampler2DArray",
		"sampler2DArrayShadow",
		"isampler2D",
		"isampler2DArray",
		"usampler2D",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"sampler3D",
		"isampler3D",
		"usampler3D",
		"samplerCube",
		"samplerCubeShadow",
		"isamplerCube",
		"usamplerCube",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"image1D",
		"iimage1D",
		"uimage1D",
		"image1DArray",
		"iimage1DArray",
		"uimage1DArray",
		"image2D",
		"iimage2D",
		"uimage2D",
		"image2DArray",
		"iimage2DArray",
		"uimage2DArray",
		"image2DRect",
		"iimage2DRect",
		"uimage2DRect",
		"image2DMS",
		"iimage2DMS",
		"uimage2DMS",
		"image2DMSArray",
		"iimage2DMSArray",
		"uimage2DMSArray",
		"image3D",
		"iimage3D",
		"uimage3D",
		"imageCube",
		"iimageCube",
		"uimageCube",
		"imageCubeArray",
		"iimageCubeArray",
		"uimageCubeArray",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"struct",

		// vulkan
		"texture1D",
		"texture1DArray",
		"itexture1D",
		"itexture1DArray",
		"utexture1D",
		"utexture1DArray",
		"texture2D",
		"texture2DArray",
		"itexture2D",
		"itexture2DArray",
		"utexture2D",
		"utexture2DArray",
		"texture2DRect",
		"itexture2DRect",
		"utexture2DRect",
		"texture2DMS",
		"itexture2DMS",
		"utexture2DMS",
		"texture2DMSArray",
		"itexture2DMSArray",
		"utexture2DMSArray",
		"texture3D",
		"itexture3D",
		"utexture3D",
		"textureCube",
		"itextureCube",
		"utextureCube",
		"textureCubeArray",
		"itextureCubeArray",
		"utextureCubeArray",
		"textureBuffer",
		"itextureBuffer",
		"utextureBuffer",
		"sampler",
		"samplerShadow",
		"subpassInput",
		"isubpassInput",
		"usubpassInput",
		"subpassInputMS",
		"isubpassInputMS",
		"usubpassInputMS",
	};
	return strGLSLUniformTypeName[(size_t)(type)];
}

template <>
struct std::hash<Shit::ImageViewCreateInfo>
{
	std::size_t operator()(Shit::ImageViewCreateInfo const &s) const noexcept
	{
		size_t h = std::hash<Shit::ImageViewType>{}(s.viewType);
		hashCombine(h, std::hash<Shit::Format>{}(s.format));
		hashCombine(h, std::hash<Shit::ComponentSwizzle>{}(s.components.r));
		hashCombine(h, std::hash<Shit::ComponentSwizzle>{}(s.components.g));
		hashCombine(h, std::hash<Shit::ComponentSwizzle>{}(s.components.b));
		hashCombine(h, std::hash<Shit::ComponentSwizzle>{}(s.components.a));
		hashCombine(h, std::hash<uint32_t>{}(s.subresourceRange.baseMipLevel));
		hashCombine(h, std::hash<uint32_t>{}(s.subresourceRange.levelCount));
		hashCombine(h, std::hash<uint32_t>{}(s.subresourceRange.baseArrayLayer));
		hashCombine(h, std::hash<uint32_t>{}(s.subresourceRange.layerCount));
		return h;
	}
};
template <>
struct std::hash<Shit::BufferCreateInfo>
{
	std::size_t operator()(Shit::BufferCreateInfo const &s) const noexcept
	{
		size_t h = std::hash<Shit::BufferCreateFlagBits>{}(s.flags);
		hashCombine(h, std::hash<uint64_t>{}(s.size));
		hashCombine(h, std::hash<Shit::BufferUsageFlagBits>{}(s.usage));
		hashCombine(h, std::hash<Shit::MemoryPropertyFlagBits>{}(s.memoryPropertyFlags));
		return h;
	}
};

namespace Shit
{
	constexpr bool operator==(const BufferCreateInfo &lhs, const BufferCreateInfo &rhs) noexcept
	{
		return lhs.flags == rhs.flags &&
			   lhs.size == rhs.size &&
			   lhs.usage == rhs.usage &&
			   lhs.memoryPropertyFlags == rhs.memoryPropertyFlags;
	}
	constexpr bool operator==(const ImageViewCreateInfo &lhs, const ImageViewCreateInfo &rhs) noexcept
	{
		return lhs.pImage == rhs.pImage &&
			   lhs.viewType == rhs.viewType &&
			   lhs.format == rhs.format &&
			   lhs.components.r == rhs.components.r &&
			   lhs.components.g == rhs.components.g &&
			   lhs.components.b == rhs.components.b &&
			   lhs.components.a == rhs.components.a &&
			   lhs.subresourceRange.baseArrayLayer == rhs.subresourceRange.baseArrayLayer &&
			   lhs.subresourceRange.baseMipLevel == rhs.subresourceRange.baseMipLevel &&
			   lhs.subresourceRange.layerCount == rhs.subresourceRange.layerCount &&
			   lhs.subresourceRange.levelCount == rhs.subresourceRange.levelCount;
	}
}

template <typename T = float>
struct Offset2D
{
	T x;
	T y;
};

template <typename T = float>
struct Offset3D
{
	T x;
	T y;
	T z;
};

template <typename T = float>
struct Extent2D
{
	T width;
	T height;
};
template <typename T = float>
struct Extent3D
{
	T width;
	T height;
	T depth;
};

template <typename T = float>
struct Rect2D
{
	Offset2D<T> offset;
	Extent2D<T> extent;
};

template <typename T = float>
struct Rect3D
{
	Offset3D<T> offset;
	Extent3D<T> extent;
};

using Offset2DF = Offset2D<float>;
using Offset3DF = Offset3D<float>;
using Extent2DF = Extent2D<float>;
using Extent3DF = Extent3D<float>;
using Rect2DF = Rect2D<float>;
using Rect3DF = Rect3D<float>;

inline size_t getDataTypeSizeInBytes(DataType type)
{
	static const size_t a[]{
		1, // INT8,
		1, // UINT8,
		2, // INT16,
		2, // UINT16,
		4, // INT32,
		4, // UINT32,
		8, // INT64,
		8, // UINT64,
		4, // FLOAT,
		8, // DOUBLE,

		8,	// BVEC2,
		12, // BVEC3,
		16, // BVEC4,
		8,	// IVEC2,
		12, // IVEC3,
		16, // IVEC4,
		8,	// VEC2,
		12, // VEC3,
		16, // VEC4,
		8,	// DVEC2,
		12, // DVEC3,
		16, // DVEC4,

		16, // MAT2,
		24, // MAT2x3,
		32, // MAT2x4,
		24, // MAT3x2,
		36, // MAT3,
		48, // MAT3x4,
		64, // MAT4,
		32, // MAT4x2,
		48, // MAT4x3,
		16, // DMAT2,
		24, // DMAT2x3,
		32, // DMAT2x4,
		24, // DMAT3x2,
		36, // DMAT3,
		48, // DMAT3x4,
		64, // DMAT4,
		32, // DMAT4x2,
		48, // DMAT4x3,
		1,	// INT8_NORM,
		1,	// UINT8_NORM,
	};
	return a[(size_t)type];
}