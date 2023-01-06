#pragma once
#include "gpuResource.hpp"
#include "pipeline.hpp"
#include "descriptorManager.hpp"

// class GraphicPipelineWrapper;
// struct MaterialElement
//{
//	std::string name;
//	DataType dataType;
//	void *pValue;
// };

// struct MaterialUBOView
//{
//	// bool std430;
//	BufferView bufferView;
//	// size_t alignment;
//	std::vector<MaterialElement> elements;
// };

//==============================
class Material : public IdObject<Material>
{
public:
	Material(MaterialDataBlockManager *creator, int priority = 0);
	Material(MaterialDataBlockManager *creator, std::string_view name, int priority = 0);

	constexpr std::string_view getName() const { return _name; }

	virtual ~Material();
	constexpr int getPriority() const { return _priority; }

	Shit::GraphicsPipeline *getPipeline(GraphicPipelineSpec const &spec)
	{
		return _pipelineWrapper->getHandle(spec);
	}
	virtual Shit::GraphicsPipeline *getPipelineLight(GraphicPipelineSpec const &spec) { return nullptr; }

	GraphicPipelineWrapper *getPipelineWrapper() const { return _pipelineWrapper.get(); }

	static constexpr std::span<Shit::VertexBindingDescription> sGetCommonVertexBindings()
	{
		return s_commonVertexBindings;
	}
	static constexpr std::span<Shit::VertexAttributeDescription> sGetCommonVertexAttributes()
	{
		return s_commonVertexAttributes;
	}

protected:
	void init();

	std::string _name{};

	MaterialDataBlockManager *_creator;

	std::unique_ptr<GraphicPipelineWrapper> _pipelineWrapper;

	// all pipelines have the same vertex input state
	static std::vector<Shit::VertexBindingDescription> s_commonVertexBindings;
	static std::vector<Shit::VertexAttributeDescription> s_commonVertexAttributes;

	int _priority;

	Shit::PolygonMode _polygonMode{Shit::PolygonMode::FILL};

	friend class MaterialDataBlock;
};

class MaterialUnlitColor : public Material
{
public:
	struct Para
	{
		std::array<float, 4> color;
	};
	MaterialUnlitColor(MaterialDataBlockManager *creator);
};

class MaterialGPass : public Material
{
public:
	MaterialGPass(MaterialDataBlockManager *creator);
};
class MaterialDeferred : public Material
{

	std::unique_ptr<GraphicPipelineWrapper> _pipelineWrapperLight;

	void createPipelineEnv();
	void createPipelineLight();

public:
	MaterialDeferred(MaterialDataBlockManager *creator);

	Shit::GraphicsPipeline *getPipelineLight(GraphicPipelineSpec const &spec) override
	{
		if (!_pipelineWrapperLight)
			createPipelineLight();
		return _pipelineWrapperLight->getHandle(spec);
	}
};

class MaterialPBR : public Material
{
	std::unique_ptr<GraphicPipelineWrapper> _pipelineWrapperLight;

	void createPipeline();
	void createPipelineLight();

public:
	struct Para
	{
		int baseColorTextureIndex{-1};		   // srgb
		int metallicRoughnessTextureIndex{-1}; // b metallic, g roughness, [r occlussion]
		int normalTextureIndex{-1};			   // rgb
		int occlusionTextureIndex{-1};		   // r occlussion

		// base color
		float baseColorFactor[4]{1, 1, 1, 1};

		// emissive
		float emissiveFactor[3]{};
		int emissiveTextureIndex{-1}; // srgb

		//
		int alphaMode{0}; // 0 opaque, 1 cutoff, 2 blend
		float alphaCutoff{0.5};
		float normalTextureScale{1};
		float occlusionTextureStrength{1};

		float uvOffset[2];
		float uvScale[2];

		// sheen
		float sheenColorFactor[3]; // linear
		float sheenRoughnessFactor{0};

		int sheenColorTextureIndex{-1};
		int sheenRoughnessTextureIndex{-1};
		float metallicFactor{1};
		float roughnessFactor{1};

		// clear coat
		float clearcoatFactor{0};
		float clearcoatRoughnessFactor{0};
		int clearcoatTextureIndex{-1};
		int clearcoatRoughnessTextureIndex{-1};

		int clearcoatNormalTextureIndex{-1};
	};

	MaterialPBR(MaterialDataBlockManager *creator);

	Shit::GraphicsPipeline *getPipelineLight(GraphicPipelineSpec const &spec) override
	{
		if (!_pipelineWrapperLight)
			createPipelineLight();
		return _pipelineWrapperLight->getHandle(spec);
	}
};

class MaterialShadow : public Material
{
public:
	MaterialShadow(MaterialDataBlockManager *creator);
};

class MaterialSkybox : public Material
{
public:
	MaterialSkybox(MaterialDataBlockManager *creator);
	~MaterialSkybox() {}
};

//===========================================================
/**
 * @brief  the descriptor set number is 3
 *
 */
class MaterialDataBlock : public GPUResource
{
public:
	MaterialDataBlock(MaterialDataBlockManager *creator, MaterialType type, std::string_view group);
	MaterialDataBlock(MaterialDataBlockManager *creator, MaterialType type, std::string_view name, std::string_view group);
	virtual ~MaterialDataBlock();

	DescriptorSetData *getDescriptorSetData() const { return _descriptorSetData; }

	Material *getMaterial();
	constexpr MaterialType getMaterialType() const { return _materialType; }
	void setMaterialType();

	constexpr bool isEditable() const { return _isEditable; }
	constexpr void setEditable(bool val) { _isEditable = val; }

	// constexpr MaterialUBOView const &getUboView() const { return _paraView; }

	virtual void updateGpuBuffer() = 0;

	void prepare(int index = -1) override;
	void upload(int index = -1) override;
	void destroy(int index = -1) override;

	constexpr std::span<Texture *> getAllTextures() { return _textures; }
	constexpr Texture *getTextureByIndex(size_t index) const { return _textures.at(index); }

	void setDescriptorTexture(uint32_t startIndex, DescriptorTextureData const &texture);

	virtual RenderQueueType getRenderQueueType() = 0;

protected:
	virtual void init();

	MaterialType _materialType;
	bool _isEditable = true;

	// MaterialUBOView _paraView;

	BufferView _paraBufferView;

	std::vector<Texture *> _textures;

	// std::vector<std::vector<Shit::DescriptorSet *>> _frameDescriptorSets;

	// dynamic states
	// float _lineWidth{};
	// float _depthBias{};
	// float _blendConstants[4];
	// float _minDepthBounds{0};
	// float _maxDepthBounds{1};

	// Shit::StencilMaskInfo _stencilCompareMask;
	// Shit::StencilMaskInfo _stencilWriteMask;
	// Shit::StencilMaskInfo _stencilReference;

	DescriptorSetData *_descriptorSetData{};
};
class MaterialDataBlockUnlitColor : public MaterialDataBlock
{
	MaterialUnlitColor::Para _para;

	void init() override;

public:
	MaterialDataBlockUnlitColor(MaterialDataBlockManager *creator, std::string_view group);
	MaterialDataBlockUnlitColor(MaterialDataBlockManager *creator, std::string_view name, std::string_view group);

	constexpr MaterialUnlitColor::Para const &getUBO() const { return _para; }
	MaterialDataBlockUnlitColor *setColorFactor(float colorR, float colorG, float colorB, float colorA);
	MaterialDataBlockUnlitColor *setColorFactor(std::span<float const> color);
	void updateGpuBuffer() override;
	void setMaterial(MaterialUnlitColor::Para const &para);

	RenderQueueType getRenderQueueType() override { return RenderQueueType::GEOMETRY; }
};
class MaterialDataBlockPBR : public MaterialDataBlock
{
	MaterialPBR::Para _para;

	void init() override;

public:
	MaterialDataBlockPBR(MaterialDataBlockManager *creator, std::string_view group);
	MaterialDataBlockPBR(MaterialDataBlockManager *creator, std::string_view name, std::string_view group);
	void updateGpuBuffer() override;
	void setMaterial(MaterialPBR::Para const &para);
	constexpr MaterialPBR::Para const &getUBO() const { return _para; }

	RenderQueueType getRenderQueueType() override { return RenderQueueType::GEOMETRY; }
};

//===============================================================
class MaterialDataBlockManager : public GPUResourceManager
{
	mutable std::array<std::unique_ptr<Material>, static_cast<size_t>(MaterialType::Num)> _materials;

	std::array<std::unique_ptr<MaterialDataBlock>, static_cast<size_t>(MaterialType::Num)> _defaultMaterialDataBlocks;

	std::array<std::vector<Material *>, (size_t)RenderPassType::Num> _materialGroups;

public:
	MaterialDataBlockManager();
	~MaterialDataBlockManager() {}

	Material *getMaterial(MaterialType materialType);

	MaterialDataBlock *createMaterialDataBlock(
		MaterialType materialType, std::string_view name = {}, std::string_view group = DEFAULT_GROUP_NAME);

	MaterialDataBlock *getDefaultMaterialBlock(MaterialType materialType = MaterialType::UNLIT_COLOR) const;
};