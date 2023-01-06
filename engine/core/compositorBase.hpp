#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include "buffer.hpp"
#include "root.hpp"
#include "descriptorManager.hpp"

// struct CompositorInputChannelDefaultValueType
//{
//	std::vector<char> data;
//	BufferView bufferView;
// };

struct CompositorGlobalPara
{
	int textureCount[GLSL_UNIFORM_TYPE_Num]{};
	int uboSize{};
	int ssboSize{};
};

struct CompositorBufferDescription
{
	int uboIndex{-1};
	std::vector<float> data;
};
struct CompositorTextureDescription
{
	GLSLUniformType uniformType;
	int textureIndex{-1};
	// std::vector<Shit::DescriptorImageInfo> frameImageInfo;
	DescriptorTextureData textureData;
};

using CompositorChannelValue = std::variant<
	std::monostate,
	CompositorBufferDescription,
	CompositorTextureDescription>;

enum class ChannelType
{
	IMAGE,	  // vec3
	CONSTANT, // float
};

template <typename R, typename... Args>
class CompositorNodeBase : public IdObject<CompositorNodeBase<R, Args...>>
{
public:
	using input_channel_return_type = R;
	using signal_signature = R(Args...);

	struct OutputChannel
	{
		std::string name;
		Shit::Slot<R(Args...)> slot;//return id
		CompositorChannelValue value; // default settings
		std::string code;
	};

	struct InputChannel
	{
		std::string name;
		Shit::Signal<R(Args...)> signal; // will generat a signal
		//CompositorChannelValue defaultVal;
		//std::string code;
	};

	CompositorNodeBase()
	{
		_needUpdate.resize(Root::getSingleton().getScreen()->getSwapchain()->GetImageCount(), true);
	}
	virtual ~CompositorNodeBase() {}

	CompositorBufferDescription &createParameter(
		std::string_view name, std::span<float> values, int uboIndex = -1)
	{
		auto &&ret = _parameters.emplace(
									std::string(name),
									CompositorBufferDescription{uboIndex})
						 .first->second;
		ret.data.assign(values.begin(), values.end());
		return ret;
	}
	CompositorBufferDescription &getParameter(std::string_view name) const
	{
		return _parameters.at(std::string(name));
	}
	void setParamter(std::string_view name, std::span<float> values)
	{
		auto &&a = _parameters[std::string(name)];
		ranges::copy(values, a.data.begin());
	}
	void createInputChannel(std::string_view name)
	{
		auto a = std::make_shared<InputChannel>(InputChannel{std::string(name)});
		//a->defaultVal = CompositorBufferDescription{
		//	-1,
		//	std::vector<float>(values.begin(), values.end())};
		_inputChannels.emplace_back(a);
	}
	void createOutputChannel(const OutputChannel &channel)
	{
		auto a = std::make_shared<OutputChannel>(channel);
		a->slot.Track(a);
		_outputChannels.emplace_back(a);
	}
	void createOutputChannels(std::span<OutputChannel> channels)
	{
		for (auto e : channels)
		{
			auto a = std::make_shared<OutputChannel>(e);
			a->slot.Track(a);
			_outputChannels.emplace_back(a);
		}
	}

	void connectTo(size_t outputChannelIndex, CompositorNodeBase *dst, size_t inputChannelIndex)
	{
		auto &&outputChannel = _outputChannels[outputChannelIndex];
		auto &&inputChannel = dst->_inputChannels[inputChannelIndex];
		inputChannel->signal.Connect(outputChannel->slot);
	}
	void discconectFrom(size_t outputChannelIndex, CompositorNodeBase *dst, size_t inputChannelIndex)
	{
		auto &&outputChannel = _outputChannels[outputChannelIndex];
		auto &&inputChannel = dst->_inputChannels[inputChannelIndex];
		inputChannel->signal.Disconnect(outputChannel->slot);
	}

	InputChannel *getInputChannelByIndex(uint32_t index) const
	{
		return _inputChannels.at(index).get();
	}
	OutputChannel *getOutputChannelByIndex(uint32_t index) const
	{
		return _outputChannels.at(index).get();
	}
	constexpr uint32_t getInputChannelCount() const { return _inputChannels.size(); }
	constexpr uint32_t getOutputChannelCount() const { return _outputChannels.size(); }

	//virtual void setInputChannelDefaultValue(size_t channelIndex, std::span<float> values)
	//{
	//}

	virtual void reset(CompositorGlobalPara &para) = 0;

	void update(uint32_t frameIndex)
	{
		if (_needUpdate[frameIndex])
		{
			updateImpl(frameIndex);
			_needUpdate[frameIndex] = false;
		}
	}

protected:
	virtual void updateImpl(uint32_t frameIndex) {}
	std::string _name;

	std::vector<bool> _needUpdate;

	std::vector<std::shared_ptr<InputChannel>> _inputChannels;
	std::vector<std::shared_ptr<OutputChannel>> _outputChannels;
	std::unordered_map<std::string, CompositorBufferDescription> _parameters;
};