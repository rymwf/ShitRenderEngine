#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"

enum class EnvironmentMode
{
	COLOR,
	EQRECT,
	PROCEDURAL,
};

class Environment
{
	IdType _setDataId;

	static Texture *s_brdfTex;
	static Texture *s_brdfSheenTex;

	Texture *_eqTex{};
	//Texture *_skyboxTex{};
	Texture *_prefilteredSkyboxTex{};

	Scene *_scene{};

	static Shit::Pipeline *s_eq2cubePipeline;
	static Shit::PipelineLayout *s_eq2cubePipelineLayout;

	static Shit::Pipeline *s_prefilteredComputePipeline;
	static Shit::PipelineLayout *s_prefilteredComputePipelineLayout;

	DescriptorSetData *_skyboxDescriptorSetData;

	void destroySkyboxTex();

	void generatePrefilteredSkyboxTex();

	static void loadBrdfTex();

	static void createPrefilteredEnvPipeline();
	static void createEq2cubePipeline();

	EnvironmentMode _envMode{EnvironmentMode::COLOR};
	Shit::ClearColorValueFloat _clearValue;

	void prepare();

public:
	Environment(Scene *scene);

	~Environment();

	constexpr void setEnvironmentMode(EnvironmentMode mode)
	{
		_envMode = mode;
	}
	void setSkyboxEqTex(Texture* tex);

	void drawSkybox(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex, Shit::Rect2D rect) const;
	void bindTex(Shit::CommandBuffer *cmdBuffer) const;

	constexpr DescriptorSetData *getDescriptorSetData() const { return _skyboxDescriptorSetData; }
};
