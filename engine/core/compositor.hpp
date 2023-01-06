#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include "singleton.hpp"
#include "compositorBase.hpp"
#include "buffer.hpp"

enum class CNMathFunction
{
	ADD,		// x+y
	MULTIPLY,	// x*y
	DIVIDE,		// x/y
	RECIPROCAL, // 1/x
	POWER,		// x^y
	EXP,		// exp(x)
	LOGARITHM,	// logx y;//default x is 10
	LOG10,		// log10 x
	LOG2,		// log2(x)
};

class CompositorWorkspace;

struct CompositorDescriptorWriteInfo
{
	std::vector<Shit::DescriptorImageInfo> imagesInfo;
	std::vector<Shit::DescriptorBufferInfo> buffersInfo;
};

struct CNInputChannelOutputSignalType
{
	// uint32_t uniformVariablesNum[GLSL_UNIFORM_TYPE_Num];
	// uint32_t uboBufferSize;
	// std::string shaderCodeBody1;
	// std::string shaderCodeBody2;
	IdType id;
	int channelIndex;
	DataType dataType;
};

struct ChannelRoute
{
	uint32_t outputNodeIndex;
	uint32_t outputChannelIndex;
	uint32_t inputNodeIndex;
	uint32_t inputChannelIndex;
};

//===============compositor node descriptions=================
struct CNRenderLayerDescription
{
	std::string sceneName;
	uint32_t viewLayerIndex;
};
struct CNCompositeDescription
{
	// Shit::Viewport viewport;
};
struct CNImageDescription
{
	Image *image;
	Shit::Format format;
};
struct CNColorDescription
{
	std::array<float, 3> value;
};

//========control variables
// stored in ubo
struct CNControlColorDescription
{
	std::array<float, 3> value;
};
struct CNControlValueDescription
{
	float value;
};

//=======
struct CNMathDescription
{
	CNMathFunction func;
};
struct CNColorMixDescription
{
};

using CompositorNodeDescription = std::variant<
	CNRenderLayerDescription,
	CNCompositeDescription,
	CNImageDescription,
	CNMathDescription,
	CNColorDescription,
	CNColorMixDescription,
	CNControlColorDescription,
	CNControlValueDescription>;

//============================================================
struct CompositorWorkspaceDescription
{
	std::vector<CompositorNodeDescription> compositorNodes;
	std::vector<ChannelRoute> channelRoutes;
};

//================================================================
class CompositorNode
	: public CompositorNodeBase<CNInputChannelOutputSignalType>
{
protected:
	CompositorWorkspace *_workspace;
	CompositorNodeDescription _desc;

	void setParametersIndex(uint32_t &uboArraySize);

public:
	// using signal_argument_type = const CNInputChannelOutputSignalType &;
	using signal_argument_type = void;

	CompositorNode(CompositorWorkspace *workspace, const CompositorNodeDescription &desc);
	virtual ~CompositorNode() {}

	constexpr CompositorNodeDescription &getDescription() { return _desc; }
	constexpr const CompositorNodeDescription &getDescription() const { return _desc; }

	void getUBOVariableShaderCode(std::string &str, DataType datatype, int startIndex);

	void reset(CompositorGlobalPara &para) override;

	virtual void refresh() {}

	std::string getOutputChannelName(int channelIndex) const;

	static std::string getOutputChannelName(IdType nodeId, int channelIndex);
};

template <typename T_dec>
class CompositorNodeExt : public CompositorNode
{
public:
	CompositorNodeExt(CompositorWorkspace *workspace, const CompositorNodeDescription &desc)
		: CompositorNode(workspace, desc)
	{
	}
	T_dec &getDescriptionExt()
	{
		return std::get<T_dec>(_desc);
	}
	const T_dec &getDescriptionExt() const
	{
		return std::get<T_dec>(_desc);
	}
};

//================================
// compositor workspace
//======================
// compositorNodes
/**
 * @brief output channel
 * 1 image
 * 2 alpha
 * 3 z
 * 4 shadow
 * 5 ao
 *
 */
class CNRenderLayer : public CompositorNodeExt<CNRenderLayerDescription>
{
	CNInputChannelOutputSignalType processOutputImageViewColor(int index);
	CNInputChannelOutputSignalType processOutputImageViewAlpha(int index);
	CNInputChannelOutputSignalType processOutputImageViewDepth(int index);
	CNInputChannelOutputSignalType processOutputImageViewShadow(int index);
	CNInputChannelOutputSignalType processOutputImageViewAO(int index);

	Scene *getScene() const;

	void updateImpl(uint32_t frameIndex) override;

public:
	CNRenderLayer(CompositorWorkspace *workspace, const CNRenderLayerDescription &desc);

	void setScene(std::string_view sceneName, uint32_t viewLayerIndex);

	void refresh();

	void reset(CompositorGlobalPara &para) override;

	void needUpdate(uint32_t frameIndex);
};
/**
 * @brief
 * input channel:
 * 1: image, rgb
 * 2: alpha
 * 3: z
 *
 */
class CNComposite : public CompositorNodeExt<CNCompositeDescription>
{
	static std::string s_fragShaderCodeHeader;
	static std::string s_fragShaderCodeBody1;
	static std::string s_fragShaderCodeBody2;
	static std::string s_fragShaderCodeEnd;

	Shader *_fragShader{};
	Shit::DescriptorSetLayout *_descriptorSetLayout{};
	Shit::PipelineLayout *_pipelineLayout{};

	float _color[4]{0, 0, 0, 1};
	float _z;

	void destroyGPUResources();

public:
	CNComposite(CompositorWorkspace *workspace, const CompositorNodeDescription &desc);
	~CNComposite() {}

	void setColor(float v[3]);
	void setAlpha(float v);
	void setZ(float v);

	constexpr Shader *getFragShader() const { return _fragShader; }
	constexpr Shit::PipelineLayout *getPipelineLayout() const { return _pipelineLayout; }
	constexpr Shit::DescriptorSetLayout *getDescriptorSetLayout() const { return _descriptorSetLayout; }
	/**
	 * @brief rebuild fragshader and pipelinelayout
	 *
	 */
	void rebuild();

	void refresh();
};

/**
 * @brief
 * output channel:
 * 1 image
 *
 */
class CNImage : public CompositorNodeExt<CNImageDescription>
{
	Texture *_texture{};

	void createTexture();
	void destroyTexture();

	CNInputChannelOutputSignalType processOutputImageViewColor(int channelIndex);
	CNInputChannelOutputSignalType processOutputImageViewAlpha(int channelIndex);

	void updateImpl(uint32_t frameIndex) override;

public:
	CNImage(CompositorWorkspace *workspace, const CNImageDescription &desc);

	void reset(CompositorGlobalPara &para) override;

	void setImage(Image *image);
	void setFormat(Shit::Format format);
};
class CNColor : public CompositorNodeExt<CNColorDescription>
{
	CNInputChannelOutputSignalType processOutputVector(int channelIndex);

	void setVector(std::array<float, 3> val);

public:
	CNColor(CompositorWorkspace *workspace, const CNColorDescription &desc);
};
//============================================

class CNControlColor : public CompositorNodeExt<CNControlColorDescription>
{
	CNInputChannelOutputSignalType processOutputColor(int index);

public:
	CNControlColor(CompositorWorkspace *workspace, const CNControlColorDescription &desc);
	void setColor(std::array<float, 3> color);
};

class CNControlValue : public CompositorNodeExt<CNControlValueDescription>
{
	CNInputChannelOutputSignalType processOutputValue(int index);

public:
	CNControlValue(CompositorWorkspace *workspace, const CNControlValueDescription &desc);
	void setValue(float value);
};

//============================================
/**
 * @brief 2 input channels and 1 output channel
 *
 */
class CNMath : public CompositorNodeExt<CNMathDescription>
{
	CNInputChannelOutputSignalType processOutputColor();

public:
	CNMath(CompositorWorkspace *workspace, const CNMathDescription &desc);

	void setInput1(float val);
	void setInput2(float val);
	void setFunction(CNMathFunction func);
};

class CNColorMix : public CompositorNodeExt<CNColorMix>
{
	CNInputChannelOutputSignalType processOutputImage(int index);

	float _factor{0.5};
	std::array<float, 3> _color0{};
	std::array<float, 3> _color1{};

public:
	CNColorMix(CompositorWorkspace *workspace, const CNColorMixDescription &desc);
	void setFactor(float v);
};
