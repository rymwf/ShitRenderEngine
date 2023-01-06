#include "shader.hpp"
#include "root.hpp"
#include <shaderc/shaderc.hpp>

Shader::Shader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage)
	: Resource(name, group, false, nullptr), _stage(stage)
{
}
void Shader::loadImpl()
{
	_shader = Root::getSingleton().getDevice()->Create(Shit::ShaderCreateInfo{
		Shit::ShadingLanguage::SPIRV, _data.size(), _data.data()});
}
Shit::PipelineShaderStageCreateInfo Shader::getPipelineShaderStageCreateInfo(
	const Shit::SpecializationInfo *spec)
{
	load();
	if (spec)
		return Shit::PipelineShaderStageCreateInfo{_stage, _shader, "main", *spec};
	else
		return Shit::PipelineShaderStageCreateInfo{_stage, _shader, "main"};
}
SPVShader::SPVShader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage)
	: Shader(name, group, stage)
{
}
void SPVShader::prepareImpl()
{
	if (_data.empty())
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
GLSLShader::GLSLShader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage)
	: Shader(name, group, stage)
{
}
GLSLShader::GLSLShader(
	std::string_view name,
	std::string_view group,
	Shit::ShaderStageFlagBits stage,
	size_t size,
	void const *data)
	: Shader(name, group, stage)
{
	updateData(0, size, data);
}
std::tuple<int, int, int> GLSLShader::getTargetEnvironment()
{
	auto rendererVersion = Root::getSingleton().getRendererVersion();
	auto type = rendererVersion & Shit::RendererVersion::TypeBitmask;
	//auto version = rendererVersion & Shit::RendererVersion::VersionBitmask;
	int res_type, res_version, res_spv_version;
	switch (type)
	{
	case Shit::RendererVersion::GL:
	{
		res_type = shaderc_target_env_opengl;
		res_version = shaderc_env_version_opengl_4_5;
		res_spv_version = shaderc_spirv_version_1_0;
		break;
	}
	case Shit::RendererVersion::VULKAN:
	default:
	{
		res_type = shaderc_target_env_vulkan;
		switch (rendererVersion)
		{
		case Shit::RendererVersion::VULKAN_100:
			res_version = shaderc_env_version_vulkan_1_0;
			// res_spv_version=
			break;
		case Shit::RendererVersion::VULKAN_110:
			res_version = shaderc_env_version_vulkan_1_1;
			res_spv_version = shaderc_spirv_version_1_3;
			break;
		case Shit::RendererVersion::VULKAN_120:
			res_version = shaderc_env_version_vulkan_1_2;
			res_spv_version = shaderc_spirv_version_1_5;
			break;
		case Shit::RendererVersion::VULKAN_130:
			res_version = shaderc_env_version_vulkan_1_3;
			res_spv_version = shaderc_spirv_version_1_6;
			break;
		}
		break;
	}
	}
	return {res_type, res_version, res_spv_version};
}
int GLSLShader::getStage()
{
	switch (_stage)
	{
	case Shit::ShaderStageFlagBits::VERTEX_BIT:
		return shaderc_vertex_shader;
	case Shit::ShaderStageFlagBits::TESSELLATION_CONTROL_BIT:
		return shaderc_tess_control_shader;
	case Shit::ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT:
		return shaderc_tess_evaluation_shader;
	case Shit::ShaderStageFlagBits::GEOMETRY_BIT:
		return shaderc_geometry_shader;
	case Shit::ShaderStageFlagBits::FRAGMENT_BIT:
		return shaderc_fragment_shader;
	case Shit::ShaderStageFlagBits::COMPUTE_BIT:
		return shaderc_compute_shader;
	case Shit::ShaderStageFlagBits::MESH_BIT:
		return shaderc_mesh_shader;
	case Shit::ShaderStageFlagBits::TASK_BIT:
		return shaderc_task_shader;
	case Shit::ShaderStageFlagBits::RAYGEN_BIT:
		return shaderc_raygen_shader;
	default:
		break;
	}
	THROW("unknow shader stage", (int)_stage);
	return 0;
}
void GLSLShader::prepareImpl()
{
	Shader::prepareImpl();
	// compile glsl to spv
	static shaderc::Compiler compiler;
	static shaderc::CompileOptions compileOptions;

	compileOptions.SetSourceLanguage(shaderc_source_language_glsl);
	auto targetEnv = getTargetEnvironment();
	compileOptions.SetTargetEnvironment((shaderc_target_env)std::get<0>(targetEnv), (shaderc_env_version)std::get<1>(targetEnv));
	compileOptions.SetTargetSpirv((shaderc_spirv_version)std::get<2>(targetEnv));
	compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

	auto retCode = compiler.CompileGlslToSpv(_data.data(), _data.size(), (shaderc_shader_kind)getStage(), "shader.glsl", compileOptions);
	if (retCode.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		THROW("failed to compile shader", _name, "group", _group, "error message", retCode.GetErrorMessage());
	}
	_spvCode.assign(retCode.cbegin(), retCode.cend());
}
void GLSLShader::loadImpl()
{
	_shader = Root::getSingleton().getDevice()->Create(Shit::ShaderCreateInfo{
		Shit::ShadingLanguage::SPIRV, _spvCode.size() * sizeof(uint32_t), _spvCode.data()});
}
//=============================================
Resource *ShaderCreator::operator()(
	std::string_view name,
	const ParameterMap *parameterMap,
	std::string_view group,
	bool manullyLoad,
	ManualResourceLoader *loader,
	size_t size,
	void const *data) const
{
	auto stagestr = parameterMap->at("ShaderStage");
	Shit::ShaderStageFlagBits stage;
	if (stagestr == "VERT")
		stage = Shit::ShaderStageFlagBits::VERTEX_BIT;
	else if (stagestr == "TESC")
		stage = Shit::ShaderStageFlagBits::TESSELLATION_CONTROL_BIT;
	else if (stagestr == "TESE")
		stage = Shit::ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT;
	else if (stagestr == "GEOM")
		stage = Shit::ShaderStageFlagBits::GEOMETRY_BIT;
	else if (stagestr == "FRAG")
		stage = Shit::ShaderStageFlagBits::FRAGMENT_BIT;
	else if (stagestr == "COMP")
		stage = Shit::ShaderStageFlagBits::COMPUTE_BIT;
	else
		THROW("wrong shader stage", stagestr);

	auto shadingLanguage = parameterMap->at("ShadingLanguage");
	if (shadingLanguage == "GLSL")
	{
		if (size > 0)
		{
			return new GLSLShader(name, group, stage, size, data);
		}
		else
			return new GLSLShader(name, group, stage);
	}
	else if (shadingLanguage == "SPV")
		return new SPVShader(name, group, stage);
	THROW("not support shading language", shadingLanguage);
	return nullptr;
}