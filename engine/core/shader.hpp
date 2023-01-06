#pragma once
#include "resourceManager.hpp"

class Shader : public Resource
{
public:
	Shader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage);
	~Shader() override
	{
	}
	constexpr Shit::Shader *getHandle() const { return _shader; }
	constexpr Shit::ShaderStageFlagBits getShaderStage() const { return _stage; }

	Shit::PipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(
		const Shit::SpecializationInfo *spec = nullptr);

protected:
	void loadImpl() override;

	Shit::ShaderStageFlagBits _stage;

	Shit::Shader *_shader;
};

class SPVShader : public Shader
{
	void prepareImpl() override;

public:
	SPVShader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage);
};

class GLSLShader : public Shader
{
	void prepareImpl() override;
	void loadImpl() override;

	int getStage();

	/**
	 * @brief Get the Target Environment object
	 * 
	 * @return std::tuple<int, int, int> 
	 * 0: shaderc_target_env
	 * 1: shaderc_env_version
	 * 2: shaderc_spirv_version
	 */
	std::tuple<int, int, int> getTargetEnvironment();

	std::vector<uint32_t> _spvCode;

public:
	GLSLShader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage);
	GLSLShader(std::string_view name, std::string_view group, Shit::ShaderStageFlagBits stage,
			   size_t size, void const *data);
	~GLSLShader() override
	{
	}
};

struct ShaderCreator
{
	Resource *operator()(
		std::string_view name,
		const ParameterMap *parameterMap,
		std::string_view group,
		bool manullyLoad,
		ManualResourceLoader *loader,
		size_t size,
		void const *data) const;
};