#include "environment.hpp"
#include "texture.hpp"
#include "root.hpp"
#include "resourceGroup.hpp"
#include "shader.hpp"
#include "descriptorManager.hpp"
#include "material.hpp"

#define SKYBOX_WIDTH 1024
#define SAMPLE_NUM 256
#define MAX_ROUGHNESS_LEVEL 7

Shit::Pipeline *Environment::s_eq2cubePipeline = 0;
Shit::Pipeline *Environment::s_prefilteredComputePipeline = 0;

Shit::PipelineLayout *Environment::s_eq2cubePipelineLayout = 0;
Shit::PipelineLayout *Environment::s_prefilteredComputePipelineLayout = 0;

Texture *Environment::s_brdfTex = 0;
Texture *Environment::s_brdfSheenTex = 0;

void Environment::loadBrdfTex()
{
	static const char *brdfTexName = "images/envBRDF.hdr";
	static const char *brdfSheenTexName = "images/sheenEnvBRDF.hdr";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
	auto brdfImage = static_cast<Image *>(grp->createOrRetrieveResource(ResourceDeclaration{brdfTexName, ResourceType::IMAGE}));
	auto brdfSheenImage = static_cast<Image *>(grp->createOrRetrieveResource(ResourceDeclaration{brdfSheenTexName, ResourceType::IMAGE}));

	auto texMgr = Root::getSingleton().getTextureManager();
	s_brdfTex = texMgr->create(
		Shit::ImageType::TYPE_2D,
		Shit::ImageUsageFlagBits::SAMPLED_BIT,
		{&brdfImage, 1},
		SamplerInfo{
			TextureInterpolation::LINEAR,
			Shit::SamplerWrapMode::CLAMP_TO_EDGE,
			false});

	s_brdfSheenTex = texMgr->create(
		Shit::ImageType::TYPE_2D,
		Shit::ImageUsageFlagBits::SAMPLED_BIT,
		{&brdfSheenImage, 1},
		SamplerInfo{
			TextureInterpolation::LINEAR,
			Shit::SamplerWrapMode::CLAMP_TO_EDGE,
			false});
}
void Environment::createEq2cubePipeline()
{
	auto device = Root::getSingleton().getDevice();
	// create pipline layout
	Shit::DescriptorSetLayoutBinding bindings[]{
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
		{1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
	};

	auto descriptorSetLayout = device->Create(
		Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});

	s_eq2cubePipelineLayout = device->Create(
		Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

	// create pipeline
	auto archiveName = getShaderArchiveName();
	static auto comp_file_spv = archiveName + "/equirectangular2cube.comp.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "COMP"}, {"ShadingLanguage", "SPV"}};
	auto compShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{comp_file_spv, ResourceType::SHADER, nullptr, paras}));
	compShader->load();

	//========
	auto stage = Shit::PipelineShaderStageCreateInfo{
		Shit::ShaderStageFlagBits::COMPUTE_BIT,
		compShader->getHandle(),
		"main"};
	s_eq2cubePipeline = device->Create(
		Shit::ComputePipelineCreateInfo{stage, s_eq2cubePipelineLayout});
}
void Environment::createPrefilteredEnvPipeline()
{
	auto device = Root::getSingleton().getDevice();
	// create pipline layout
	Shit::DescriptorSetLayoutBinding bindings[]{
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
		{1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
	};

	auto descriptorSetLayout = device->Create(
		Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});

	Shit::PushConstantRange range{Shit::ShaderStageFlagBits::COMPUTE_BIT, 2, 0, sizeof(float) * 2};
	s_prefilteredComputePipelineLayout = device->Create(
		Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout, 1, &range});

	//=================
	auto archiveName = getShaderArchiveName();
	static auto comp_file_spv = archiveName + "/prefilteredEnv2D.comp.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "COMP"}, {"ShadingLanguage", "SPV"}};
	auto compShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{comp_file_spv, ResourceType::SHADER, nullptr, paras}));
	compShader->load();

	auto stage = Shit::PipelineShaderStageCreateInfo{
		Shit::ShaderStageFlagBits::COMPUTE_BIT,
		compShader->getHandle(),
		"main"};

	s_prefilteredComputePipeline = device->Create(
		Shit::ComputePipelineCreateInfo{stage, s_prefilteredComputePipelineLayout});
}
Environment::Environment(Scene *scene) : _scene(scene), _clearValue{0.2, 0.2, 0.2, 1.f}
{
	if (!s_eq2cubePipeline)
	{
		loadBrdfTex();
		createEq2cubePipeline();
		createPrefilteredEnvPipeline();
	}
	_skyboxDescriptorSetData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		Root::getSingleton().getDescriptorSetLayoutEnv());

	_skyboxDescriptorSetData->setTexture(
		11, 0,
		DescriptorTextureData{
			s_brdfTex,
			Shit::ImageViewType::TYPE_2D,
			s_brdfTex->getImageCreateInfo()->format,
			{},
			{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
			Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			s_brdfTex->getSamplerInfo()});
}
Environment::~Environment()
{
	auto texmgr = Root::getSingleton().getTextureManager();
	if (_eqTex)
		texmgr->remove(_eqTex);
	if (_prefilteredSkyboxTex)
		texmgr->remove(_prefilteredSkyboxTex);
}
void Environment::generatePrefilteredSkyboxTex()
{
	// create skybox tex
	if (!_prefilteredSkyboxTex)
	{
		_prefilteredSkyboxTex = Root::getSingleton().getTextureManager()->create(
			Shit::ImageCreateInfo{
				Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
				Shit::ImageType::TYPE_2D,
				Shit::Format::R32G32B32A32_SFLOAT,
				Shit::Extent3D{1024, 1024, 1},
				0,
				6,
				Shit::SampleCountFlagBits::BIT_1,
				Shit::ImageTiling::OPTIMAL,
				Shit::ImageUsageFlagBits::STORAGE_BIT |
					Shit::ImageUsageFlagBits::SAMPLED_BIT |
					Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
				Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
				Shit::ImageLayout::GENERAL},
			false,
			SamplerInfo{TextureInterpolation::CUBIC,
						Shit::SamplerWrapMode::CLAMP_TO_EDGE,
						false});
	}

	auto levels = _prefilteredSkyboxTex->getGpuImage()->GetCreateInfoPtr()->mipLevels;
	std::vector<DescriptorSetData *> setDatas(levels);
	for (uint32_t i = 0; i < levels; ++i)
	{
		setDatas[i] = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
			s_prefilteredComputePipelineLayout->GetCreateInfoPtr()->pSetLayouts[0]);

		setDatas[i]->setTexture(
			0, 0,
			DescriptorTextureData{
				_eqTex,
				Shit::ImageViewType::TYPE_2D,
				_eqTex->getImageCreateInfo()->format,
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, i, 1, 0, 1},
				Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				_eqTex->getSamplerInfo()});

		setDatas[i]->setTexture(
			1, 0,
			DescriptorTextureData{
				_prefilteredSkyboxTex,
				Shit::ImageViewType::TYPE_CUBE,
				_prefilteredSkyboxTex->getImageCreateInfo()->format,
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, i, 1, 0, _prefilteredSkyboxTex->getImageCreateInfo()->arrayLayers},
				Shit::ImageLayout::GENERAL,
				_prefilteredSkyboxTex->getSamplerInfo()});
		setDatas[i]->prepare();
	}

	Root::getSingleton().executeOneTimeCommand(
		[&](Shit::CommandBuffer *cmdBuffer)
		{
			// Shit::ImageMemoryBarrier barrier{
			// 	Shit::AccessFlagBits::MEMORY_WRITE_BIT,
			// 	Shit::AccessFlagBits::SHADER_WRITE_BIT,
			// 	Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			// 	Shit::ImageLayout::GENERAL,
			// 	ST_QUEUE_FAMILY_IGNORED,
			// 	ST_QUEUE_FAMILY_IGNORED,
			// 	_prefilteredSkyboxTex->getGpuImage(),
			// 	{Shit::ImageAspectFlagBits::COLOR_BIT,
			// 	 0,
			// 	 _prefilteredSkyboxTex->getGpuImage()->GetCreateInfoPtr()->mipLevels,
			// 	 0,
			// 	 _prefilteredSkyboxTex->getImageCreateInfo()->arrayLayers}};
			// cmdBuffer->PipelineBarrier(
			// 	Shit::PipelineBarrierInfo{
			// 		Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			// 		Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
			// 		{},
			// 		0,
			// 		0,
			// 		0,
			// 		0,
			// 		1,
			// 		&barrier});

			cmdBuffer->BindPipeline(Shit::BindPipelineInfo{
				Shit::PipelineBindPoint::COMPUTE,
				s_eq2cubePipeline});

			setDatas[0]->bind(cmdBuffer, s_eq2cubePipeline->GetBindPoint(), s_eq2cubePipeline->GetPipelineLayout(), 0, 0);

			cmdBuffer->Dispatch({SKYBOX_WIDTH, 6, 1});

			//=========================
			cmdBuffer->BindPipeline(Shit::BindPipelineInfo{
				Shit::PipelineBindPoint::COMPUTE, s_prefilteredComputePipeline});

			auto pipeline = dynamic_cast<Shit::ComputePipeline *>(s_prefilteredComputePipeline);
			uint32_t width = _prefilteredSkyboxTex->getImageCreateInfo()->extent.width;

			int sampleNum = SAMPLE_NUM;
			float a[2];
			memcpy(&a[1], &sampleNum, sizeof(int));
			int i = 1;
			for (; i < MAX_ROUGHNESS_LEVEL + 1; ++i)
			{
				width /= 2;
				a[0] = float(i) / MAX_ROUGHNESS_LEVEL;
				cmdBuffer->PushConstants(Shit::PushConstantInfo{
					pipeline->GetCreateInfoPtr()->pLayout,
					Shit::ShaderStageFlagBits::COMPUTE_BIT,
					0,
					sizeof(float) * 2,
					a});
				setDatas[i]->bind(cmdBuffer, pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), 0, 0);
				cmdBuffer->Dispatch({width, 6, 1});
			}
			// transfer imagelayout to shder readonly
			auto barrier = Shit::ImageMemoryBarrier{
				Shit::AccessFlagBits::SHADER_WRITE_BIT,
				Shit::AccessFlagBits::SHADER_READ_BIT,
				Shit::ImageLayout::GENERAL,
				Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				ST_QUEUE_FAMILY_IGNORED,
				ST_QUEUE_FAMILY_IGNORED,
				_prefilteredSkyboxTex->getGpuImage(),
				{Shit::ImageAspectFlagBits::COLOR_BIT,
				 0,
				 _prefilteredSkyboxTex->getGpuImage()->GetCreateInfoPtr()->mipLevels,
				 0,
				 _prefilteredSkyboxTex->getImageCreateInfo()->arrayLayers}};
			cmdBuffer->PipelineBarrier(
				Shit::PipelineBarrierInfo{
					Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
					Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
					{},
					0,
					0,
					0,
					0,
					1,
					&barrier});
		});
}
void Environment::destroySkyboxTex()
{
	Root::getSingleton().getTextureManager()->remove(_prefilteredSkyboxTex);
}
void Environment::setSkyboxEqTex(Texture *tex)
{
	_eqTex = tex;
	setEnvironmentMode(EnvironmentMode::EQRECT);
	prepare();
}
void Environment::prepare()
{
	if (_envMode == EnvironmentMode::EQRECT)
	{
		generatePrefilteredSkyboxTex();
		_skyboxDescriptorSetData->setTexture(
			10, 0,
			DescriptorTextureData{
				_prefilteredSkyboxTex,
				Shit::ImageViewType::TYPE_CUBE,
				_prefilteredSkyboxTex->getImageCreateInfo()->format,
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, 0, _prefilteredSkyboxTex->getGpuImage()->GetCreateInfoPtr()->mipLevels, 0, 6},
				Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				_prefilteredSkyboxTex->getSamplerInfo()});
		_skyboxDescriptorSetData->prepare();
	}
}
void Environment::drawSkybox(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex, Shit::Rect2D rect) const
{
	switch (_envMode)
	{
	case EnvironmentMode::COLOR:
	{
		Shit::ClearAttachment clearAttachment{Shit::ImageAspectFlagBits::COLOR_BIT, 0, _clearValue};
		Shit::ClearRect clearRect{rect, 0, 1};
		cmdBuffer->ClearAttachments(Shit::ClearAttachmentsInfo{
			1, &clearAttachment, 1, &clearRect});
	}
	break;
	case EnvironmentMode::EQRECT:
	{
		// _skyboxDescriptorSetData ->bind(cmdBuffer, pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), DESCRIPTORSET_ENV, 0);
		cmdBuffer->Draw(Shit::DrawIndirectCommand{36, 1, 0, 0});
	}
	break;
	case EnvironmentMode::PROCEDURAL:
	{
	}
	break;
	}
}
void Environment::bindTex(Shit::CommandBuffer *cmdBuffer) const
{
}