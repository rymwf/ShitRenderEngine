#include "compositor.hpp"
#include "root.hpp"
#include "appbase.hpp"
#include "texture.hpp"
#include "scene.hpp"
#include "buffer.hpp"
#include "compositorWorkspace.hpp"

#define COMPOSITOR_SHADER_UBOBUFFER_NAME "ubo_data"
#define COMPOSITOR_SHADER_TEXCOORD_NAME "uv"

//==============================================
// compositor node
CompositorNode::CompositorNode(CompositorWorkspace *workspace, const CompositorNodeDescription &desc)
	: _workspace(workspace), _desc(desc)
{
	_name = "compositor_node_";
	_name += std::to_string(_id);
}
// void CompositorNode::setInputChannelDefaultValue(size_t channelIndex, std::span<float const> values)
//{
//	//auto &&a = std::get<CompositorBufferDescription>(_inputChannels[channelIndex]->defaultVal);
//	//a.data.assign(values.begin(), values.end());
//	//if (a.uboIndex != -1)
//	//{
//	//	_workspace->setUBOBufferData(a.uboIndex * sizeof(float), a.data.size() * sizeof(float), a.data.data());
//	//}
// }
void CompositorNode::setParametersIndex(uint32_t &uboArraySize)
{
	for (auto &&e : _parameters)
	{
		if (e.second.uboIndex != -1)
			return;
		e.second.uboIndex = uboArraySize;
		uboArraySize += e.second.data.size();
	}
}
std::string CompositorNode::getOutputChannelName(int channelIndex) const
{
	return "_" + std::to_string((int)getId()) + "_" + std::to_string(channelIndex);
}
std::string CompositorNode::getOutputChannelName(IdType nodeId, int channelIndex)
{
	return "_" + std::to_string((int)nodeId) + "_" + std::to_string(channelIndex);
}
void CompositorNode::getUBOVariableShaderCode(std::string &str, DataType datatype, int startIndex)
{
	std::string prefix;
	std::string suffix;
	std::string token;
	int count{0};
	switch (datatype)
	{
	case DataType::FLOAT:
		count = 1;
		break;
	case DataType::VEC3:
		prefix = "vec3(";
		suffix = ")";
		token = ",";
		count = 3;
		break;
	default:
		THROW("error data type", (int)datatype);
	}
	str += prefix;
	for (int i = 0; i < count - 1; ++i)
	{
		str += COMPOSITOR_SHADER_UBOBUFFER_NAME "[";
		str += std::to_string(startIndex + i);
		str += "]";
		str += token;
	}
	str += COMPOSITOR_SHADER_UBOBUFFER_NAME "[";
	str += std::to_string(startIndex + count - 1);
	str += "]";
	str += suffix;
}
void CompositorNode::reset(CompositorGlobalPara &para)
{
	for (auto &&e : _parameters)
	{
		e.second.uboIndex = para.uboSize;
		para.uboSize += e.second.data.size();
		_workspace->allocateUBOBuffer(sizeof(float), e.second.data.size(), e.second.data.data());
		//_workspace->setUBOBufferData(e.second.uboIndex * sizeof(float), e.second.data.size() * sizeof(float), e.second.data.data());
	}
	for (auto &&e : _outputChannels)
	{
		std::visit(Shit::overloaded{
					   [&](CompositorBufferDescription &v)
					   {
						   v.uboIndex = para.uboSize;
						   para.uboSize += v.data.size();
						   _workspace->allocateUBOBuffer(sizeof(float), v.data.size(), v.data.data());
					   },
					   [&](CompositorTextureDescription &v)
					   {
						   if (v.textureIndex >= 0)
						   {
							   v.textureIndex = para.textureCount[v.uniformType]++;
							   _workspace->getDescriptorSetData()->setTexture(1, v.textureIndex, v.textureData);
						   }
					   },
					   [](auto &&) {},
				   },
				   e->value);
		e->code = "";
	}
}

//=========================
// CNRenderLayer
CNRenderLayer::CNRenderLayer(CompositorWorkspace *workspace, const CNRenderLayerDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	OutputChannel channels[] = {
		{"Color",
		 std::bind(&CNRenderLayer::processOutputImageViewColor, this, 0),
		 CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, 0}},
		{"Alpha",
		 std::bind(&CNRenderLayer::processOutputImageViewAlpha, this, 1),
		 CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, -1}},
		//{"Depth",
		// std::bind(&CNRenderLayer::processOutputImageViewDepth, this, std::placeholders::_1, 2),
		// CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, -1}},
		//{"Shadow",
		// std::bind(&CNRenderLayer::processOutputImageViewShadow, this, std::placeholders::_1, 3),
		// CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, -1}},
		//{"AO",
		// std::bind(&CNRenderLayer::processOutputImageViewAO, this, std::placeholders::_1, 4),
		// CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, -1}},
	};
	createOutputChannels(channels);
	setScene(desc.sceneName, desc.viewLayerIndex);
}
Scene *CNRenderLayer::getScene() const
{
	return Root::getSingleton().getSceneManager()->getScene(getDescriptionExt().sceneName);
}
CNInputChannelOutputSignalType
CNRenderLayer::processOutputImageViewColor(int index)
{
	auto &&code = _outputChannels[index]->code;
	if (!code.empty())
		return CNInputChannelOutputSignalType{getId(), index, DataType::VEC3};

	auto &&outputValue = std::get<CompositorTextureDescription>(_outputChannels[index]->value);

	std::string textureName = "tex_";
	textureName += getGLSLUniformTypeName(outputValue.uniformType);
	textureName += "[";
	textureName += std::to_string(outputValue.textureIndex);
	textureName += "]";

	std::string name = "_";
	name += std::to_string(int(getId()));

	std::string str;
	str = "vec4 ";
	str += name;
	str += "=texture(";
	str += textureName;
	str += ",getTexUV(textureSize(";
	str += textureName;
	str += ",0)));\n";

	code = "vec3 ";
	code += getOutputChannelName(index);
	code += "=";
	code += name;
	code += ".rgb;";
	//
	_workspace->getMainCode() += str;
	_workspace->getMainCode() += code;

	return CNInputChannelOutputSignalType{getId(), index, DataType::VEC3};
}
CNInputChannelOutputSignalType CNRenderLayer::processOutputImageViewAlpha(int index)
{
	auto &&code = _outputChannels[index]->code;
	if (!code.empty())
		return CNInputChannelOutputSignalType{getId(), index, DataType::FLOAT};

	if (_outputChannels[0]->code.empty())
	{
		processOutputImageViewColor(0);
	}
	std::string name = "_";
	name += std::to_string(int(getId()));

	code = "float ";
	code += getOutputChannelName(index);
	code += "=";
	code += name;
	code += ".a;";
	_workspace->getMainCode() += code;
	return CNInputChannelOutputSignalType{getId(), index, DataType::FLOAT};
}
// IdType CNRenderLayer::processOutputImageViewDepth(size_t index)
//{
//	CNInputChannelOutputSignalType val2 = val;
//	setParametersIndex(val2.uboBufferSize);
//	auto &&outputValue = std::get<CompositorTextureDescription>(_outputChannels[index]->value);
//	if (_depthTextureIndex == -1)
//	{
//		outputValue.textureIndex = _depthTextureIndex = val2.uniformVariablesNum[outputValue.uniformType]++;
//		val2.uniformVariablesNum[outputValue.uniformType]++;
//		val2.shaderCodeBody1 += "vec4 ";
//		val2.shaderCodeBody1 += _name;
//		val2.shaderCodeBody1 += "_depth=texture(tex_";
//		val2.shaderCodeBody1 += getGLSLUniformTypeName(outputValue.uniformType);
//		val2.shaderCodeBody1 += "[";
//		val2.shaderCodeBody1 += std::to_string(_depthTextureIndex);
//		val2.shaderCodeBody1 += "],vec3(" COMPOSITOR_SHADER_TEXCOORD_NAME ",0));\n";
//	}
//	else
//	{
//		outputValue.textureIndex = _depthTextureIndex;
//	}
//	val2.shaderCodeBody2 = _name;
//	val2.shaderCodeBody2 += "_depth.r";
//	return val2;
// }
// IdType CNRenderLayer::processOutputImageViewShadow(size_t index)
//{
//	CNInputChannelOutputSignalType val2 = val;
//	setParametersIndex(val2.uboBufferSize);
//	auto &&outputValue = std::get<CompositorTextureDescription>(_outputChannels[index]->value);
//	if (_shadowAOTextureIndex == -1)
//	{
//		outputValue.textureIndex = _shadowAOTextureIndex = val2.uniformVariablesNum[outputValue.uniformType]++;
//		val2.shaderCodeBody1 += "vec4 ";
//		val2.shaderCodeBody1 += _name;
//		val2.shaderCodeBody1 += "_shadow=texture(tex_";
//		val2.shaderCodeBody1 += getGLSLUniformTypeName(outputValue.uniformType);
//		val2.shaderCodeBody1 += "[";
//		val2.shaderCodeBody1 += std::to_string(_depthTextureIndex);
//		val2.shaderCodeBody1 += "],vec3(" COMPOSITOR_SHADER_TEXCOORD_NAME ",0));\n";
//	}
//	else
//	{
//		outputValue.textureIndex = _shadowAOTextureIndex;
//	}
//	val2.shaderCodeBody2 = _name;
//	val2.shaderCodeBody2 += "_shadow.g";
//	return val2;
// }
// IdType CNRenderLayer::processOutputImageViewAO(size_t index)
//{
//	CNInputChannelOutputSignalType val2;
//	setParametersIndex(val2.uboBufferSize);
//	// if (_shadowAOTextureIndex == -1)
//	//{
//	//	_shadowAOTextureIndex = val.textureNum;
//	//	auto imageCount = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();
//	//	for (uint32_t i = 0; i < imageCount; ++i)
//	//	{
//	//		updateOutputImageViewShadowAO(i);
//	//	}
//	//	val.textureNum++;
//	// }
//	// val.shaderCode = "texture(" COMPOSITOR_SHADER_TEXTURES_NAME;
//	// val.shaderCode += "[";
//	// val.shaderCode += _shadowAOTextureIndex;
//	// val.shaderCode += "]," COMPOSITOR_SHADER_TEXCOORD_NAME ").g";
//	return val2;
// }
void CNRenderLayer::setScene(std::string_view sceneName, uint32_t viewLayerIndex)
{
	getDescriptionExt() = CNRenderLayerDescription{std::string(sceneName), viewLayerIndex};
	refresh();
}
void CNRenderLayer::refresh()
{
	auto scene = getScene();
	auto renderCamera = scene->getRenderCamera();

	auto viewLayer = renderCamera->getViewLayer();
	auto colorTexture = static_cast<Texture *>(Root::getSingleton().getTextureManager()->get(viewLayer->getColorTextureId()));

	DescriptorTextureData a{
		colorTexture,
		Shit::ImageViewType::TYPE_2D,
		viewLayer->getColorFormat(),
		{},
		{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
		Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		{TextureInterpolation::LINEAR,
		 Shit::SamplerWrapMode::CLAMP_TO_EDGE,
		 false}};
	std::get<CompositorTextureDescription>(_outputChannels[0]->value).textureData = a;

	// channel1 alpha
	std::get<CompositorTextureDescription>(_outputChannels[1]->value).textureData = a;

	// channel2 depth
	// pImageView = scene->getOutputImageViewDepth(getDescriptionExt().viewLayerIndex, 0);
	// a = {scene->getColorAttachmentTexture(getDescriptionExt().viewLayerIndex),
	//	 Shit::ImageViewType::TYPE_2D,
	//	 pImageView->GetCreateInfoPtr()->format,
	//	 pImageView->GetCreateInfoPtr()->components,
	//	 pImageView->GetCreateInfoPtr()->subresourceRange,
	//	 TextureInterpolation::LINEAR,
	//	 Shit::SamplerWrapMode::CLAMP_TO_EDGE,
	//	 true};
	// std::get<CompositorTextureDescription>(_outputChannels[2]->value).textureData = a;

	//// channel3 shadow
	// pImageView = scene->getOutputImageViewShadowAO(getDescriptionExt().viewLayerIndex, 0);
	// a = {scene->getColorAttachmentTexture(getDescriptionExt().viewLayerIndex),
	//	 Shit::ImageViewType::TYPE_2D,
	//	 pImageView->GetCreateInfoPtr()->format,
	//	 pImageView->GetCreateInfoPtr()->components,
	//	 pImageView->GetCreateInfoPtr()->subresourceRange,
	//	 TextureInterpolation::LINEAR,
	//	 Shit::SamplerWrapMode::CLAMP_TO_EDGE,
	//	 false};
	// std::get<CompositorTextureDescription>(_outputChannels[3]->value).textureData = a;
	//// channel4 ao
	// std::get<CompositorTextureDescription>(_outputChannels[4]->value).textureData = a;

	// for (auto &&e : _outputChannels)
	//{
	//	if (auto p = std::get_if<CompositorTextureDescription>(&(e->value)))
	//	{
	//		if (p->textureIndex != -1)
	//			_workspace->getDescriptorSetData()->setTexture(1, p->textureIndex, p->textureData);
	//	}
	// }
}
void CNRenderLayer::updateImpl(uint32_t frameIndex)
{
	getScene()->render(frameIndex, getDescriptionExt().viewLayerIndex);
	needUpdate((frameIndex + 1) % Root::getSingleton().getScreen()->getSwapchain()->GetImageCount()); // canbe called by scene
}
void CNRenderLayer::reset(CompositorGlobalPara &para)
{
	CompositorNode::reset(para);
	getScene()->prepare();
}
void CNRenderLayer::needUpdate(uint32_t frameIndex)
{
	_needUpdate[frameIndex] = true;
}

//=========================
// CNComposite
std::string CNComposite::s_fragShaderCodeHeader = "\
#version 460\n\
#extension GL_EXT_scalar_block_layout: enable\n\
#ifdef VULKAN\n\
#define SET(x) ,set=x\n\
#else\n\
#define SET(x)\n\
#endif\n\
struct VS_OUT{\n\
    vec2 uv;\n\
};\n\
layout(location=0) in VS_OUT fs_in;\n\
layout(location=0) out vec4 fragColor;\n\
";

std::string CNComposite::s_fragShaderCodeBody1 = "\
vec2 getTexUV(vec2 texSize)\n\
{\n\
	//vec2 Sv=vec2(" COMPOSITOR_SHADER_UBOBUFFER_NAME "[0]," COMPOSITOR_SHADER_UBOBUFFER_NAME "[1]);\n\
	//vec2 K=Sv/texSize;\n\
	//vec2 S1= texSize*min(min(K.x,K.y),1);\n\
	//#ifdef VULKAN\n\
	//return (gl_FragCoord.xy-(Sv-S1)*0.5)/S1;\n\
	//#else\n\
	//return (vec2(gl_FragCoord.x,Sv.y-gl_FragCoord.y)-(Sv-S1)*0.5)/S1;\n\
	//#endif\n\
	return fs_in.uv;\n\
}\n\
void main()\n\
{\n\
//vec2 uv=fs_in.uv;\n\
";

std::string CNComposite::s_fragShaderCodeBody2 = "fragColor=vec4(";
std::string CNComposite::s_fragShaderCodeEnd = ");\n}";

CNComposite::CNComposite(CompositorWorkspace *workspace, const CompositorNodeDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	// create default input channels
	createInputChannel("Image"); // vec3
	createInputChannel("Alpha"); // float, default 1
	createInputChannel("Z");	 // not used

	auto extent = Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->imageExtent;
	float viewportSize[]{(float)extent.width, (float)extent.height};
	createParameter("viewport_size", viewportSize);

	auto device = Root::getSingleton().getDevice();
	Shit::DescriptorSetLayoutBinding bindings[]{
		{0u, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		//{1u, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, (uint32_t)temp[GLSL_UNIFORM_TYPE_USAMPLERBUFFER], Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		{1u, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 16u, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
	};
	_descriptorSetLayout = device->Create(
		Shit::DescriptorSetLayoutCreateInfo{
			(uint32_t)std::size(bindings),
			bindings});
	_pipelineLayout = device->Create(
		Shit::PipelineLayoutCreateInfo{1, &_descriptorSetLayout});
}
void CNComposite::refresh()
{
	auto extent = Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->imageExtent;
	float viewportSize[]{(float)extent.width, (float)extent.height};
	auto &&a = _parameters[std::string("viewport_size")];
	ranges::copy(viewportSize, a.data.begin());
	_workspace->setUBOBufferData(a.uboIndex * sizeof(float), a.data.size() * sizeof(float), a.data.data());
}
void CNComposite::setColor(float v[3])
{
	_color[0] = v[0];
	_color[1] = v[1];
	_color[2] = v[2];
}
void CNComposite::setAlpha(float v)
{
	_color[3] = v;
}
void CNComposite::setZ(float v)
{
	_z = v;
}
void CNComposite::destroyGPUResources()
{
	auto device = Root::getSingleton().getDevice();
	device->WaitIdle();
	if (_fragShader)
	{
		ResourceGroupManager::getSingleton()
			.getResourceGroup(_fragShader->getGroupName())
			->removeResource(_fragShader->getName());
	}
	device->Destroy(_descriptorSetLayout);
	// device->Destroy(_pipelineLayout);
}
void CNComposite::rebuild()
{
	destroyGPUResources();

	CNInputChannelOutputSignalType val;
	// setParametersIndex(inputSignalArg.uboBufferSize);
	// auto &&framebufferSize = Root::getSingleton().getScreen()->getSwapchain()->GetCreateInfoPtr()->imageExtent;

	std::string str[3];

	for (uint32_t i = 0, l = _inputChannels.size(); i < l; ++i)
	{
		auto &&c = _inputChannels[i];
		if (c->signal.Empty())
		{
			// auto &defaultVal = std::get<CompositorBufferDescription>(c->defaultVal);
			// defaultVal.uboIndex = inputSignalArg.uboBufferSize;
			// inputSignalArg.uboBufferSize += defaultVal.data.size();

			// add ubo data, set as default value
			switch (i)
			{
			case 0:
				str[0] = "vec3(";
				str[0] += std::to_string(_color[0]);
				str[0] += ",";
				str[0] += std::to_string(_color[1]);
				str[0] += ",";
				str[0] += std::to_string(_color[2]);
				str[0] += ")";
				// getUBOVariableShaderCode(_inputColorCode, DataType::VEC3, defaultVal.uboIndex);
				break;
			case 1:
				// getUBOVariableShaderCode(_inputAlphaCode, DataType::FLOAT, defaultVal.uboIndex);
				str[1] = std::to_string(_color[3]);
				break;
			case 2:
				str[2] = std::to_string(_z);
				break;
			default:
				break;
			}
		}
		else
		{
			val = c->signal().value();
			str[i] += CompositorNode::getOutputChannelName(val.id, val.channelIndex);
		}
	}

	auto &&globalPara = _workspace->getGlobalPara();

	// sum
	int temp[GLSL_UNIFORM_TYPE_Num];
	memcpy(&temp[0], globalPara.textureCount, sizeof(int) * GLSL_UNIFORM_TYPE_Num);
	for (int i = 1; i < GLSL_UNIFORM_TYPE_Num; ++i)
		temp[i] += temp[i - 1];

	// create fragshader
	std::string fragCode = s_fragShaderCodeHeader; // add header

	// set UBO
	if (globalPara.uboSize > 0)
	{
		fragCode += "layout(binding=0,std430) uniform UBO{float " COMPOSITOR_SHADER_UBOBUFFER_NAME "[";
		fragCode += std::to_string(globalPara.uboSize);
		fragCode += "];};\n";
	}

	auto frameCount = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();

	// set uniform bindings
	int binding = 1;
	for (int i = 0; i < GLSL_UNIFORM_TYPE_Num; ++i)
	{
		if (globalPara.textureCount[i] > 0)
		{
			fragCode += "layout(binding=";
			// for gl, each  each subsequent element takes the next consecutive binding point
			fragCode += std::to_string(binding);
			fragCode += ")uniform ";
			fragCode += getGLSLUniformTypeName(i);
			fragCode += " tex_";
			fragCode += getGLSLUniformTypeName(i);
			fragCode += "[";
			fragCode += std::to_string(globalPara.textureCount[i]);
			fragCode += "];\n";
			binding = temp[i];
		}
	}

	// fragCode += "layout(binding=";
	// fragCode += std::to_string();
	// fragCode +=") uniform sampler2D " COMPOSITOR_SHADER_TEXTURES_NAME "[";
	// fragCode += std::to_string(val.uniformVariablesNum[GLSL_UNIFORM_TYPE_SAMPLER2D]);
	// fragCode += "];};";

	fragCode += s_fragShaderCodeBody1;	   // add common functions
	fragCode += _workspace->getMainCode(); // generated code 1, canbe null
	fragCode += s_fragShaderCodeBody2;	   // output fragcolor
	fragCode += str[0];					   // set fragcolor rgb
	fragCode += ",";
	fragCode += str[1]; // set fragcolor alpha
	fragCode += s_fragShaderCodeEnd;

	auto fragShaderName = std::string(_workspace->getName()) + ".frag";
	ParameterMap paras{
		{"ShadingLanguage", "GLSL"},
		{"ShaderStage", "FRAG"},
		//{"Data", fragCode},
	};
	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
	_fragShader = static_cast<Shader *>(grp->createResource(
		ResourceDeclaration{
			fragShaderName,
			ResourceType::SHADER,
			nullptr,
			paras},
		ranges::size(fragCode),
		ranges::data(fragCode)));

#ifndef NDEBUG
	_fragShader->save();
#endif
}
//=============================================
CNImage::CNImage(CompositorWorkspace *workspace, const CNImageDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	createOutputChannel(OutputChannel{
		"Image",
		std::bind(&CNImage::processOutputImageViewColor, this, 0),
		CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, 0}});
	createOutputChannel(OutputChannel{
		"Alpha",
		std::bind(&CNImage::processOutputImageViewAlpha, this, 1),
		CompositorTextureDescription{GLSL_UNIFORM_TYPE_SAMPLER2D, -1}});

	setImage(getDescriptionExt().image);
}
void CNImage::reset(CompositorGlobalPara &para)
{
	auto &&a = std::get<CompositorTextureDescription>(_outputChannels[0]->value);
	a.textureData = DescriptorTextureData{
		_texture,
		Shit::ImageViewType::TYPE_2D,
		getDescriptionExt().format,
		{},
		{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
		Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		{TextureInterpolation::LINEAR,
		 Shit::SamplerWrapMode::CLAMP_TO_BORDER,
		 false}};

	CompositorNode::reset(para);
}
void CNImage::setImage(Image *image)
{
	getDescriptionExt().image = image;
	destroyTexture();
	createTexture();

	// auto &&a = std::get<CompositorTextureDescription>(_outputChannels[0]->value);
	// a.textureData = DescriptorTextureData{
	//	_texture,
	//	Shit::ImageViewType::TYPE_2D,
	//	getDescriptionExt().format,
	//	{},
	//	{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
	//	TextureInterpolation::LINEAR,
	//	Shit::SamplerWrapMode::CLAMP_TO_BORDER,
	//	false};
	// if (a.textureIndex != -1)
	//	_workspace->getDescriptorSetData()->setTexture(1, a.textureIndex, a.textureData);
}
void CNImage::setFormat(Shit::Format format)
{
	if (auto p = std::get_if<CompositorTextureDescription>(&(_outputChannels[0]->value)))
	{
		p->textureData.format = getDescriptionExt().format = format;
		// if (p->textureIndex != -1)
		//	_workspace->getDescriptorSetData()->setTexture(1, p->textureIndex, p->textureData);
	}
}
void CNImage::createTexture()
{
	auto textureManager = Root::getSingleton().getTextureManager();
	Image *image[]{getDescriptionExt().image};

	_texture = textureManager->create(
		Shit::ImageType::TYPE_2D,
		Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
		image,
		{TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_BORDER, false},
		image[0]->getName(),
		image[0]->getGroupName());
}
void CNImage::destroyTexture()
{
	if (_texture)
	{
		Root::getSingleton().getTextureManager()->remove(_texture);
		_texture = nullptr;
	}
}
void CNImage::updateImpl(uint32_t frameIndex)
{
}
CNInputChannelOutputSignalType CNImage::processOutputImageViewColor(int channelIndex)
{
	auto &&c = _outputChannels[channelIndex];
	if (!c->code.empty())
		return {getId(), channelIndex, DataType::VEC3};

	auto &textureDesc = std::get<CompositorTextureDescription>(c->value);

	std::string textureName = "tex_";
	textureName += getGLSLUniformTypeName(textureDesc.uniformType);
	textureName += "[";
	textureName += std::to_string(textureDesc.textureIndex);
	textureName += "]";

	auto name = "_" + std::to_string(int(getId()));

	std::string str;

	str = "vec4 ";
	str += name;
	str += "=textureLod(";
	str += textureName;
	str += ",getTexUV(textureSize(";
	str += textureName;
	str += ",0)),0);\n";

	c->code = "vec3 ";
	c->code += getOutputChannelName(channelIndex);
	c->code += "=";
	c->code += name;
	c->code += ".rgb;\n";

	_workspace->getMainCode() += str;
	_workspace->getMainCode() += c->code;

	return {getId(), channelIndex, DataType::VEC3};
}
CNInputChannelOutputSignalType CNImage::processOutputImageViewAlpha(int channelIndex)
{
	auto &&c = _outputChannels[channelIndex];
	if (!c->code.empty())
		return {getId(), channelIndex, DataType::FLOAT};

	processOutputImageViewColor(0);

	auto name = "_" + std::to_string(int(getId()));

	c->code = "float ";
	c->code += getOutputChannelName(channelIndex);
	c->code += "=";
	c->code += name;
	c->code += ".a;\n";
	_workspace->getMainCode() += c->code;
	return {getId(), channelIndex, DataType::FLOAT};
}

//========================
CNColor::CNColor(CompositorWorkspace *workspace, const CNColorDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	createOutputChannel(OutputChannel{
		"Color",
		std::bind(&CNColor::processOutputVector, this, 0)});
	setVector(getDescriptionExt().value);
}
void CNColor::setVector(std::array<float, 3> val)
{
	getDescriptionExt().value = val;
}
CNInputChannelOutputSignalType CNColor::processOutputVector(int channelIndex)
{
	auto &&c = _outputChannels[channelIndex];
	if (!c->code.empty())
		return {getId(), channelIndex, DataType::VEC3};

	auto p = getDescriptionExt().value;

	c->code = "vec3 _";
	c->code += std::to_string(int(getId()));
	c->code += "_";
	c->code += std::to_string(channelIndex);
	c->code += "=vec4(";
	c->code = std::to_string(p[0]);
	c->code += ",";
	c->code += std::to_string(p[1]);
	c->code += ",";
	c->code += std::to_string(p[2]);
	c->code += ")\n";

	_workspace->getMainCode() += c->code;
	return {getId(), channelIndex, DataType::VEC3};
}
//===============
CNControlColor::CNControlColor(CompositorWorkspace *workspace, const CNControlColorDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	createOutputChannel(OutputChannel{
		"Color",
		std::bind(&CNControlColor::processOutputColor, this, 0),
		CompositorBufferDescription{0, {desc.value[0], desc.value[1], desc.value[2]}}});
	setColor(getDescriptionExt().value);
}
void CNControlColor::setColor(std::array<float, 3> color)
{
	auto &&c = std::get<CompositorBufferDescription>(_outputChannels[0]->value);
	_workspace->setUBOBufferData(sizeof(float) * c.uboIndex, sizeof(float) * 3, color.data());
}
CNInputChannelOutputSignalType CNControlColor::processOutputColor(int index)
{
	auto &&c = _outputChannels[index];
	if (!c->code.empty())
		return {getId(), index, DataType::VEC3};

	auto &&a = std::get<CompositorBufferDescription>(c->value);

	c->code = "vec3 ";
	c->code += getOutputChannelName(index);
	c->code += "=";
	getUBOVariableShaderCode(c->code, DataType::VEC3, a.uboIndex);
	c->code += ";";
	_workspace->getMainCode() += c->code;
	return {getId(), index, DataType::VEC3};
}
CNControlValue::CNControlValue(CompositorWorkspace *workspace, const CNControlValueDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	createOutputChannel(OutputChannel{
		"Value",
		std::bind(&CNControlValue::processOutputValue, this, 0),
		CompositorBufferDescription{0, {desc.value}}});
}
void CNControlValue::setValue(float value)
{
	auto &&c = std::get<CompositorBufferDescription>(_outputChannels[0]->value);
	_workspace->setUBOBufferData(sizeof(float) * c.uboIndex, sizeof(float), &value);
}
CNInputChannelOutputSignalType CNControlValue::processOutputValue(int index)
{
	auto &&c = _outputChannels[index];
	if (!c->code.empty())
		return {getId(), index, DataType::FLOAT};

	auto &&a = std::get<CompositorBufferDescription>(c->value);

	c->code = "float ";
	c->code += getOutputChannelName(index);
	c->code += "=";
	getUBOVariableShaderCode(c->code, DataType::FLOAT, a.uboIndex);
	c->code += ";";
	_workspace->getMainCode() += c->code;

	return {getId(), index, DataType::FLOAT};
}

//=============================================
CNMath::CNMath(CompositorWorkspace *workspace, const CNMathDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	// create channel
	createInputChannel("input1");
	createInputChannel("input2");
	// createOutputChannel("output", std::bind(&CNMath::processOutputColor, this, std::placeholders::_1));

	// switch (desc.func)
	//{
	// case CNMathFunction::ADD: //x+y
	//	break;
	// case CNMathFunction::MULTIPLY: //x*y
	//	break;
	// case CNMathFunction::DIVIDE: //x/y
	//	break;
	// case CNMathFunction::RECIPROCAL: //1/x
	//	break;
	// case CNMathFunction::POWER: //x^y
	//	break;
	// case CNMathFunction::EXP: //exp(x)
	//	break;
	// case CNMathFunction::LOGARITHM: //logx y;//default x is 10
	//	break;
	// case CNMathFunction::LOG10: //log10 x
	//	break;
	// case CNMathFunction::LOG2: //log2(x)
	//	break;
	// default:
	//	break;
	// }
}
CNInputChannelOutputSignalType CNMath::processOutputColor()
{
	return {getId(), 0};
}
void CNMath::setInput1(float val)
{
	for (uint32_t i = 0, l = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount(); i < l; ++i)
	{
		//_workspace->addDescriptorWriteInfo(
		//	CompositorDescriptorWriteInfo{
		//		i,
		//		0,
		//		Shit::DescriptorType::UNIFORM_BUFFER,
		//		{}

		//	});
	}
}
void CNMath::setInput2(float val)
{
}
void CNMath::setFunction(CNMathFunction func)
{
}
//========================================
CNColorMix::CNColorMix(CompositorWorkspace *workspace, const CNColorMixDescription &desc)
	: CompositorNodeExt(workspace, desc)
{
	createInputChannel("factor");
	createInputChannel("image1");
	createInputChannel("image2");
	createOutputChannel(OutputChannel{"image", std::bind(&CNColorMix::processOutputImage, this, 0)});
}
CNInputChannelOutputSignalType CNColorMix::processOutputImage(int index)
{
	auto och = _outputChannels[index];
	if (!och->code.empty())
		return {getId(), index, DataType::VEC3};

	CNInputChannelOutputSignalType val;
	std::string str[3];

	for (int i = 0; i < 3; ++i)
	{
		auto &&c = _inputChannels[i];
		if (c->signal.Empty())
		{
			switch (i)
			{
			case 0:
				// getUBOVariableShaderCode(_inputChannels[i]->code, DataType::FLOAT, defaultVal.uboIndex);
				//  factor
				str[0] = std::to_string(_factor);
				break;
			case 1:
				// image 1
				str[1] = "vec4(";
				str[1] += std::to_string(_color0[0]);
				str[1] += ",";
				str[1] += std::to_string(_color0[1]);
				str[1] += ",";
				str[1] += std::to_string(_color0[2]);
				str[1] += ")";
				break;
			case 2:
				// image 2
				str[2] = "vec4(";
				str[2] += std::to_string(_color1[0]);
				str[2] += ",";
				str[2] += std::to_string(_color1[1]);
				str[2] += ",";
				str[2] += std::to_string(_color1[2]);
				str[2] += ")";
				break;
			default:
				break;
			}
		}
		else
		{
			// auto
			val = c->signal().value();
			str[i] = CompositorNode::getOutputChannelName(val.id, val.channelIndex);
		}
	}

	och->code = "vec3 ";
	och->code += getOutputChannelName(index);
	och->code += "=mix(";
	och->code += str[1];
	och->code += ",";
	och->code += str[2];
	och->code += ",";
	och->code += str[0];
	och->code += ");";
	_workspace->getMainCode() += och->code;
	return {getId(), index, DataType::VEC3};
}
void CNColorMix::setFactor(float v)
{
	_factor = v;
}