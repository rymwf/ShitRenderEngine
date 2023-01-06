#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"

enum class VertexAttributeType
{
	POSITION,
	NORMAL,
	TANGENT,
	TEXCOORD,
	COLOR,
	JOINTS,
	WEIGHTS,
	INDEX,

	INSTANCE,
	DRAW_COMMAND_BUFFER,
	DRAW_COUNT_BUFFER,
};
/**
 * @brief basic render unit, 
 * TODO: generate tangents, bitangents automotically
 *
 */
struct Primitive
{
	struct AttributeDescription
	{
		// IdType bufferViewId;
		BufferView bufferView;
		DataType dataType;
		std::vector<float> minValues; // optional
		std::vector<float> maxValues; // optional
	};
	struct IndexDescription
	{
		// IdType bufferViewId;
		BufferView bufferView;
		Shit::IndexType indexType;
		uint32_t minValue;
		uint32_t maxValue;
	};

	Mesh *parent;
	Shit::PrimitiveTopology topology;

	std::unique_ptr<IndexDescription> index;

	std::unique_ptr<AttributeDescription> position;
	std::unique_ptr<AttributeDescription> normal;
	std::unique_ptr<AttributeDescription> tangent;
	std::vector<AttributeDescription> colors;
	std::vector<AttributeDescription> texcoords;
	std::vector<AttributeDescription> joints;
	std::vector<AttributeDescription> weights; //used for skin

	struct Target
	{
		std::unique_ptr<AttributeDescription> position;
		std::unique_ptr<AttributeDescription> normal;
		std::unique_ptr<AttributeDescription> tangent;
		std::vector<AttributeDescription> colors;
		std::vector<AttributeDescription> texcoords;
	};
	std::vector<std::unique_ptr<Target>> targets;

	//IdType materialId;
	MaterialDataBlock *materialDataBlock{};
};

struct InstanceAttribute
{
	//glm::mat4 *matrix;
	static std::vector<Shit::VertexAttributeDescription>
	getVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
	{
		return std::vector<Shit::VertexAttributeDescription>{
			{startLocation + 0, binding, Shit::Format::R32G32B32A32_SFLOAT, 0},
			{startLocation + 1, binding, Shit::Format::R32G32B32A32_SFLOAT, 16},
			{startLocation + 2, binding, Shit::Format::R32G32B32A32_SFLOAT, 32},
			{startLocation + 3, binding, Shit::Format::R32G32B32A32_SFLOAT, 48},
		};
	}
};
struct InstanceAttributeAccessor
{
	IdType bufferViewId;
	InstanceAttribute instance;
};

class PrimitiveView
{
protected:
	friend class Renderable;
	friend class MeshView;

	static BufferView s_zeroVertexBufferView;

	Renderable *_parent;
	Primitive *_primitive{};
	uint32_t _firstVertexOrIndex{};

	//IdType _materialDataBlockId;
	MaterialDataBlock *_curMaterialDataBlock;

	int _colorIndex{};
	int _texcoordIndex{};
	int _jointIndex{};
	int _weightIndex{};

	InstanceAttributeAccessor _instanceAttribute;

	Primitive::AttributeDescription _drawCommandBuffer;
	Primitive::AttributeDescription _drawCountBuffer;

	std::variant<Shit::DrawIndexedIndirectCommand, Shit::DrawIndirectCommand> _drawCommand;

	//========================
	Primitive::AttributeDescription *_pPosition{};
	Primitive::AttributeDescription *_pNormal{};
	Primitive::AttributeDescription *_pTangent{};
	Primitive::AttributeDescription *_pColor{};
	Primitive::AttributeDescription *_pTexcoord{};

	//===========
	std::vector<Shit::Buffer *> _vertexBuffers;
	std::vector<uint64_t> _vertexOffsets;

	Shit::BindVertexBuffersInfo _vertexBuffersInfo;
	Shit::BindIndexBufferInfo _indexBufferInfo;

	//void init();

	void prepareBuffersInfo(uint32_t frameIndex);

public:
	PrimitiveView(Renderable *parent, Primitive *primitive);
	virtual ~PrimitiveView() {}

	Shit::PrimitiveTopology getTopology() const;

	/**
	 * @brief called by renderqueue
	 * 
	 */
	void prepare();
	void updateGPUData(uint32_t frameIndex);
	void recordCommandBuffer(
		Shit::CommandBuffer *cmdBuffer,
		uint32_t frameIndex,
		MaterialDataBlock *materialDataBlock);

	constexpr void setFirstVertexOrIndex(uint32_t first)
	{
		_firstVertexOrIndex = first;
	}
	constexpr void setColorIndex(int index)
	{
		_colorIndex = index;
		_pColor = &_primitive->colors[_colorIndex];
	}
	constexpr void setTexcoordIndex(int index)
	{
		_weightIndex = index;
		_pTexcoord = &_primitive->texcoords[_weightIndex];
	}
	constexpr void setJointIndex(int index)
	{
		_jointIndex = index;
	}
	constexpr void setWeightIndex(int index)
	{
		_weightIndex = index;
	}
	constexpr void setMaterialDataBlock(MaterialDataBlock *materialDataBlock)
	{
		_curMaterialDataBlock = materialDataBlock;
	}
	constexpr void resetToDefaultMaterialDataBlock()
	{
		_curMaterialDataBlock = _primitive->materialDataBlock;
	}
	constexpr MaterialDataBlock *getMaterialDataBlock() const
	{
		return _curMaterialDataBlock;
	}
};

class MorphPrimitiveView : public PrimitiveView
{

public:
	MorphPrimitiveView(Renderable *parent, Primitive *primitive);
	~MorphPrimitiveView();

	void setMorphWeights(std::span<float> weights);
};
