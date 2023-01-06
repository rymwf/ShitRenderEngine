#include "primitive.hpp"
#include "root.hpp"
#include "buffer.hpp"
#include "material.hpp"
#include "renderable.hpp"
#include "sceneNode.hpp"

#define ZERO_VERTEX_BUFFER_SIZE 65536

BufferView PrimitiveView::s_zeroVertexBufferView = {};

PrimitiveView::PrimitiveView(Renderable *parent, Primitive *primitive)
	: _parent(parent), _primitive(primitive)
{
	// max vertex count is 2^10;
	if (!s_zeroVertexBufferView.buffer)
	{
		auto buffer = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
			BufferPropertyDesciption{
				Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
				Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
				false});
		s_zeroVertexBufferView = buffer->allocate(1, ZERO_VERTEX_BUFFER_SIZE);
	}
	_curMaterialDataBlock = _primitive->materialDataBlock;

	//====================
	_pPosition = _primitive->position ? _primitive->position.get() : nullptr;
	_pNormal = _primitive->normal ? _primitive->normal.get() : nullptr;
	_pTangent = _primitive->tangent ? _primitive->tangent.get() : nullptr;
	_pColor = _primitive->colors.empty() ? nullptr : &_primitive->colors[_colorIndex];
	_pTexcoord = _primitive->texcoords.empty() ? nullptr : &_primitive->texcoords[_texcoordIndex];
}
void PrimitiveView::prepareBuffersInfo(uint32_t frameIndex)
{
	//
	auto bindings = Material::sGetCommonVertexBindings();

	_vertexBuffersInfo = {0, LOCATION_NUM};

	_vertexBuffers.resize(bindings.size(), s_zeroVertexBufferView.buffer->getGpuBufferByIndex(frameIndex));
	_vertexOffsets.resize(bindings.size(), s_zeroVertexBufferView.offset);

	// vertex buffers
	if (_primitive->position)
	{
		_pPosition->bufferView.buffer->upload(frameIndex);
		//_primitive->position->bufferView.buffer->upload(frameIndex);
		_vertexBuffers[LOCATION_POSITION] = _pPosition->bufferView.buffer->getGpuBufferByIndex(frameIndex);
		_vertexOffsets[LOCATION_POSITION] = _pPosition->bufferView.offset;

		if (_primitive->index)
		{
			// index buffer
			_indexBufferInfo = Shit::BindIndexBufferInfo{
				_primitive->index->bufferView.buffer->getGpuBufferByIndex(frameIndex),
				0, //_primitive->index->bufferView.offset,
				_primitive->index->indexType};

			// gl do not support indexbuffer offset, thus change buffer offset to first index
			uint32_t firstIndex = 0;
			if (_primitive->index->bufferView.offset)
			{
				firstIndex = _primitive->index->bufferView.offset / Shit::GetIndexTypeSize(_primitive->index->indexType);
			}
			_drawCommand = Shit::DrawIndexedIndirectCommand{
				(uint32_t)_primitive->index->bufferView.count, 1, firstIndex, 0, 0};
		}
		else
		{
			_drawCommand = Shit::DrawIndirectCommand{
				(uint32_t)_primitive->position->bufferView.count, 1, 0, 0};
		}
	}
	else
	{
		THROW("primitive must have positions")
	}
	if (_pNormal)
	{
		_pNormal->bufferView.buffer->upload(frameIndex);
		_vertexBuffers[LOCATION_NORMAL] = _primitive->normal->bufferView.buffer->getGpuBufferByIndex(0);
		_vertexOffsets[LOCATION_NORMAL] = _primitive->normal->bufferView.offset;
	}
	if (_pTangent)
	{
		_pTangent->bufferView.buffer->upload(frameIndex);
		_vertexBuffers[LOCATION_TANGENT] = _primitive->tangent->bufferView.buffer->getGpuBufferByIndex(0);
		_vertexOffsets[LOCATION_TANGENT] = _primitive->tangent->bufferView.offset;
	}
	if (_pColor)
	{
		_pColor->bufferView.buffer->upload(frameIndex);
		_vertexBuffers[LOCATION_COLOR0] = _pColor->bufferView.buffer->getGpuBufferByIndex(0);
		_vertexOffsets[LOCATION_COLOR0] = _pColor->bufferView.offset;
	}
	if (_pTexcoord)
	{
		_pTexcoord->bufferView.buffer->upload(frameIndex);
		_vertexBuffers[LOCATION_TEXCOORD0] = _pTexcoord->bufferView.buffer->getGpuBufferByIndex(0);
		_vertexOffsets[LOCATION_TEXCOORD0] = _pTexcoord->bufferView.offset;
	}
	if (!_primitive->joints.empty())
	{
		_vertexBuffers[LOCATION_JOINTS0] = _primitive->joints[_jointIndex].bufferView.buffer->getGpuBufferByIndex(0);
		_vertexOffsets[LOCATION_JOINTS0] = _primitive->joints[_jointIndex].bufferView.offset;
	}
	if (!_primitive->weights.empty())
	{
		_vertexBuffers[LOCATION_WEIGHTS0] = _primitive->weights[_weightIndex].bufferView.buffer->getGpuBufferByIndex(0);
		_vertexOffsets[LOCATION_WEIGHTS0] = _primitive->weights[_weightIndex].bufferView.offset;
	}
	_vertexBuffersInfo.ppBuffers = _vertexBuffers.data();
	_vertexBuffersInfo.pOffsets = _vertexOffsets.data();
}
Shit::PrimitiveTopology PrimitiveView::getTopology() const
{
	return _primitive->topology;
}
void PrimitiveView::prepare()
{
	getMaterialDataBlock()->prepare();
}
void PrimitiveView::updateGPUData(uint32_t frameIndex)
{
	prepareBuffersInfo(frameIndex);
	getMaterialDataBlock()->upload(frameIndex);
	_parent->updateGPUData(frameIndex);
}
void PrimitiveView::recordCommandBuffer(
	Shit::CommandBuffer *cmdBuffer,
	uint32_t frameIndex,
	MaterialDataBlock *materialDataBlock)
{
	if (!_parent->isEnable()) //|| !_parent->isRenderInLayer(layerIndex))
		return;
	// bind material block
	auto pSetData = getMaterialDataBlock()->getDescriptorSetData();
	auto pipelineLayout = getMaterialDataBlock()->getMaterial()->getPipelineWrapper()->getCreateInfoPtr()->pLayout;

	if (materialDataBlock)
	{
		pSetData = materialDataBlock->getDescriptorSetData();
		pipelineLayout = materialDataBlock->getMaterial()->getPipelineWrapper()->getCreateInfoPtr()->pLayout;
	}

	if (pSetData)
		pSetData->bind(cmdBuffer, Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, DESCRIPTORSET_MATERIAL, frameIndex);

	// bind node ubo
	_parent->recordCommandBuffer(cmdBuffer, frameIndex);

	// bind vertex buffer
	cmdBuffer->BindVertexBuffers(_vertexBuffersInfo);

	if (_primitive->index)
	{
		cmdBuffer->BindIndexBuffer(_indexBufferInfo);
	}
	// no draw count buffer
	if (auto a = std::get_if<Shit::DrawIndirectCommand>(&_drawCommand))
	{
		// draw vertex
		cmdBuffer->Draw(*a);
	}
	else if (auto b = std::get_if<Shit::DrawIndexedIndirectCommand>(&_drawCommand))
	{
		// draw index
		cmdBuffer->Draw(*b);
	}
}
//==============================================================================
MorphPrimitiveView::MorphPrimitiveView(Renderable *parent, Primitive *primitive)
	: PrimitiveView(parent, primitive)
{
	//allocate new storage for vertices data
	auto buffer = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
			Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
			true});

	if (_primitive->position)
	{
		auto &&a = _primitive->position;
		_pPosition = new Primitive::AttributeDescription{
			buffer->allocate(a->bufferView.stride, a->bufferView.count),
			a->dataType, a->minValues, a->maxValues};
	}
	if (_primitive->normal)
	{
		auto &&a = _primitive->normal;
		_pNormal = new Primitive::AttributeDescription{
			buffer->allocate(a->bufferView.stride, a->bufferView.count),
			a->dataType, a->minValues, a->maxValues};
	}
	if (_primitive->tangent)
	{
		auto &&a = _primitive->tangent;
		_pTangent = new Primitive::AttributeDescription{
			buffer->allocate(a->bufferView.stride, a->bufferView.count),
			a->dataType, a->minValues, a->maxValues};
	}
	if (!_primitive->colors.empty())
	{
		auto &&a = _primitive->colors[0];
		_pColor = new Primitive::AttributeDescription{
			buffer->allocate(a.bufferView.stride, a.bufferView.count),
			a.dataType, a.minValues, a.maxValues};
	}
	if (!_primitive->texcoords.empty())
	{
		auto &&a = _primitive->texcoords[0];
		_pTexcoord = new Primitive::AttributeDescription{
			buffer->allocate(a.bufferView.stride, a.bufferView.count),
			a.dataType, a.minValues, a.maxValues};
	}
}
MorphPrimitiveView::~MorphPrimitiveView()
{
	delete _pPosition;
	delete _pNormal;
	delete _pTangent;
	delete _pColor;
	delete _pTexcoord;
}

void MorphPrimitiveView::setMorphWeights(std::span<float> weights)
{
	auto count = weights.size();
	if (count != _primitive->targets.size())
	{
		LOG("morph mesh weight count ", weights.size(), " is not equal to target count ", _primitive->targets.size())
		return;
	}

	//TODO: put this in gpu??
	std::vector<float> values;
	if (_pPosition)
	{
		auto p0 = reinterpret_cast<float const *>(_primitive->position->bufferView.buffer->data() +
												  _primitive->position->bufferView.offset);

		auto l = _primitive->position->bufferView.size() / sizeof(float);
		values.resize(l);
		for (size_t j = 0; j < l; ++j)
		{
			values[j] = p0[j];
			for (size_t i = 0; i < count; ++i)
			{
				auto &&bufferView = _primitive->targets[i]->position->bufferView;
				auto p = reinterpret_cast<float const *>(bufferView.buffer->data() + bufferView.offset);
				values[j] += weights[i] * p[j];
			}
		}
		_pPosition->bufferView.buffer->setData(_pPosition->bufferView.offset, _pPosition->bufferView.size(), values.data());
	}
	if (_pNormal)
	{
		auto p0 = reinterpret_cast<float const *>(_primitive->normal->bufferView.buffer->data() +
												  _primitive->normal->bufferView.offset);

		auto l = _primitive->normal->bufferView.size() / sizeof(float);
		values.resize(l);
		for (size_t j = 0; j < l; ++j)
		{
			values[j] = p0[j];
			for (size_t i = 0; i < count; ++i)
			{
				auto &&bufferView = _primitive->targets[i]->normal->bufferView;
				auto p = reinterpret_cast<float const *>(bufferView.buffer->data() + bufferView.offset);
				values[j] += weights[i] * p[j];
			}
		}
		_pNormal->bufferView.buffer->setData(_pNormal->bufferView.offset, _pNormal->bufferView.size(), values.data());
	}
	if (_pTangent)
	{
		auto p0 = reinterpret_cast<float const *>(_primitive->tangent->bufferView.buffer->data() +
												  _primitive->tangent->bufferView.offset);

		auto l = _primitive->tangent->bufferView.size() / sizeof(float);
		values.resize(l);
		for (size_t j = 0; j < l; ++j)
		{
			values[j] = p0[j];
			for (size_t i = 0; i < count; ++i)
			{
				auto &&bufferView = _primitive->targets[i]->tangent->bufferView;
				auto p = reinterpret_cast<float const *>(bufferView.buffer->data() + bufferView.offset);
				values[j] += weights[i] * p[j];
			}
		}
		_pTangent->bufferView.buffer->setData(_pTangent->bufferView.offset, _pTangent->bufferView.size(), values.data());
	}
	if (_pColor)
	{
		auto p0 = reinterpret_cast<float const *>(_primitive->colors[0].bufferView.buffer->data() +
												  _primitive->colors[0].bufferView.offset);

		auto l = _primitive->colors[0].bufferView.size() / sizeof(float);
		values.resize(l);
		for (size_t j = 0; j < l; ++j)
		{
			values[j] = p0[j];
			for (size_t i = 0; i < count; ++i)
			{
				auto &&bufferView = _primitive->targets[i]->colors[0].bufferView;
				auto p = reinterpret_cast<float const *>(bufferView.buffer->data() + bufferView.offset);
				values[j] += weights[i] * p[j];
			}
		}
		_pColor->bufferView.buffer->setData(_pColor->bufferView.offset, _pColor->bufferView.size(), values.data());
	}
	if (_pTexcoord)
	{
		auto p0 = reinterpret_cast<float const *>(_primitive->texcoords[0].bufferView.buffer->data() +
												  _primitive->texcoords[0].bufferView.offset);

		auto l = _primitive->texcoords[0].bufferView.size() / sizeof(float);
		values.resize(l);
		for (size_t j = 0; j < l; ++j)
		{
			values[j] = p0[j];
			for (size_t i = 0; i < count; ++i)
			{
				auto &&bufferView = _primitive->targets[i]->texcoords[0].bufferView;
				auto p = reinterpret_cast<float const *>(bufferView.buffer->data() + bufferView.offset);
				values[j] += weights[i] * p[j];
			}
		}
		_pTexcoord->bufferView.buffer->setData(_pTexcoord->bufferView.offset, _pTexcoord->bufferView.size(), values.data());
	}
}