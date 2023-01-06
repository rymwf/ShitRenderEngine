#include "material.hpp"
// #include "renderable.hpp"
#include "root.hpp"
#include "resourceGroup.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "descriptorManager.hpp"

// create vertex input state
std::vector<Shit::VertexBindingDescription> Material::s_commonVertexBindings = {
	{0, sizeof(float) * 3, 0}, // layout(location = 0) in vec3 inPos;
	{1, sizeof(float) * 3, 0}, // layout(location = 1) in vec3 inNormal;
	{2, sizeof(float) * 4, 0}, // layout(location = 2) in vec4 inTangent;
	{3, sizeof(float) * 2, 0}, // layout(location = 3) in vec2 inTexCoord0;
	{4, sizeof(float) * 4, 0}, // layout(location = 4) in vec4 inColor0;
	{5, sizeof(float) * 4, 0}, // layout(location = 5) in vec4 inJoints0;
	{6, sizeof(float) * 4, 0}, // layout(location = 6) in vec4 inWeights0;
							   //{7, sizeof(float) * 4, 1},	//layout(location = 11) in vec4 inInstanceColorFactor;
							   //{8, sizeof(float) * 16, 1}, //layout(location = 12) in mat4 inInstanceMatrix;
};
std::vector<Shit::VertexAttributeDescription> Material::s_commonVertexAttributes = {
	{0, 0, Shit::Format::R32G32B32_SFLOAT, 0},	  // layout(location = 0) in vec3 inPos;
	{1, 1, Shit::Format::R32G32B32_SFLOAT, 0},	  // layout(location = 1) in vec3 inNormal;
	{2, 2, Shit::Format::R32G32B32A32_SFLOAT, 0}, // layout(location = 2) in vec4 inTangent;
	{3, 3, Shit::Format::R32G32_SFLOAT, 0},		  // layout(location = 3) in vec2 inTexCoord0;
	{4, 4, Shit::Format::R32G32B32A32_SFLOAT, 0}, // layout(location = 4) in vec4 inColor0;
	{5, 5, Shit::Format::R32G32B32A32_SFLOAT, 0}, // layout(location = 5) in vec4 inJoints0;
	{6, 6, Shit::Format::R32G32B32A32_SFLOAT, 0}, // layout(location = 6) in vec4 inWeights0;

	//{11, 7, Shit::Format::R32G32B32A32_SFLOAT, 0},	 //layout(location = 11) in vec4 inInstanceColorFactor;

	//{12, 8, Shit::Format::R32G32B32A32_SFLOAT, 0},	 //layout(location = 12) in mat4 inInstanceMatrix;
	//{12, 9, Shit::Format::R32G32B32A32_SFLOAT, 16},	 //layout(location = 12) in mat4 inInstanceMatrix;
	//{12, 10, Shit::Format::R32G32B32A32_SFLOAT, 32}, //layout(location = 12) in mat4 inInstanceMatrix;
	//{12, 11, Shit::Format::R32G32B32A32_SFLOAT, 48}, //layout(location = 12) in mat4 inInstanceMatrix;
};
//===================================================
Material::Material(MaterialDataBlockManager *creator, int priority)
	: _creator(creator), _priority(priority)
{
}
Material::Material(MaterialDataBlockManager *creator, std::string_view name, int priority)
	: _creator(creator), _name(name), _priority(priority)
{
}
Material::~Material() {}

//================================================
MaterialUnlitColor::MaterialUnlitColor(MaterialDataBlockManager *creator)
	: Material(creator, "UnlitColor")
{
	auto pDevice = Root::getSingleton().getDevice();

	auto pipelineLayout = Root::getSingleton().getCommonPipelineLayout();

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/common.vert.spv";
	static auto frag_file_spv = archiveName + "/unlitColor.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));
	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	//==========
	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::FORWARD);

	Shit::PipelineColorBlendAttachmentState blendAttachmentStates[]{
		{true,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits ::R_BIT |
			 Shit::ColorComponentFlagBits ::G_BIT |
			 Shit::ColorComponentFlagBits ::B_BIT |
			 Shit::ColorComponentFlagBits ::A_BIT},
		//{true,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::ColorComponentFlagBits ::R_BIT |
		//	 Shit::ColorComponentFlagBits ::G_BIT |
		//	 Shit::ColorComponentFlagBits ::B_BIT |
		//	 Shit::ColorComponentFlagBits ::A_BIT}

	};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapper = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages), // PipelineShaderStageCreateInfo
			{
				(uint32_t)ranges::size(s_commonVertexBindings),
				ranges::data(s_commonVertexBindings),
				(uint32_t)ranges::size(s_commonVertexAttributes),
				ranges::data(s_commonVertexAttributes),
			},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)ranges::size(blendAttachmentStates),
				ranges::data(blendAttachmentStates),
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			0					  // subpass;
		});
}
MaterialGPass::MaterialGPass(MaterialDataBlockManager *creator) : Material(creator, "GPass")
{
	auto pDevice = Root::getSingleton().getDevice();

	// create descriptor setlayout set3
	Shit::DescriptorSetLayoutBinding bindings[]{
		{1, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 8, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
	};
	std::vector<Shit::DescriptorSetLayout const *> setLayouts{
		Root::getSingleton().getDescriptorSetLayoutCamera(),
		Root::getSingleton().getDescriptorSetLayoutNode(),
		Root::getSingleton().getDescriptorSetLayoutSkin(),
		Root::getSingleton().getDescriptorSetLayoutLight(),
		Root::getSingleton().getDescriptorSetLayoutEnv(),
		pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings}),
	};

	// create pipeline layout
	auto pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)setLayouts.size(), setLayouts.data()});

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/common.vert.spv";
	static auto frag_file_spv = archiveName + "/gpass.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));
	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	//==========
	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::DEFERRED);

	std::vector<Shit::PipelineColorBlendAttachmentState> colorBlendAttachmentStates(
		5, {false,
			{},
			{},
			{},
			{},
			{},
			{},
			Shit::ColorComponentFlagBits::R_BIT |
				Shit::ColorComponentFlagBits::G_BIT |
				Shit::ColorComponentFlagBits::B_BIT |
				Shit::ColorComponentFlagBits::A_BIT});

	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapper = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages), // PipelineShaderStageCreateInfo
			{
				(uint32_t)ranges::size(s_commonVertexBindings),
				ranges::data(s_commonVertexBindings),
				(uint32_t)ranges::size(s_commonVertexAttributes),
				ranges::data(s_commonVertexAttributes),
			},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)colorBlendAttachmentStates.size(),
				colorBlendAttachmentStates.data()}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			0					  // subpass;
		});
}
MaterialDeferred::MaterialDeferred(MaterialDataBlockManager *creator)
	: Material(creator, "Deferred")
{
	createPipelineEnv();
	createPipelineLight();
}
void MaterialDeferred::createPipelineEnv()
{
	auto pDevice = Root::getSingleton().getDevice();

	// create pipeline layout
	Shit::DescriptorSetLayoutBinding bindings[]{
		// set 1
		{0, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		{1, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		{2, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		{3, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
		{4, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
	};
	std::vector<Shit::DescriptorSetLayout const *> setLayouts{
		Root::getSingleton().getDescriptorSetLayoutCamera(),
		pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings}),
		pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{}),
		Root::getSingleton().getDescriptorSetLayoutLight(),
		Root::getSingleton().getDescriptorSetLayoutEnv(),
	};
	auto pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)setLayouts.size(), setLayouts.data()});

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/triangle.vert.spv";
	static auto frag_file_spv = archiveName + "/IBLdeferred.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));
	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::DEFERRED);

	Shit::PipelineColorBlendAttachmentState blendAttachmentStates[]{
		{true,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits ::R_BIT |
			 Shit::ColorComponentFlagBits ::G_BIT |
			 Shit::ColorComponentFlagBits ::B_BIT |
			 Shit::ColorComponentFlagBits ::A_BIT},
		//{true,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::ColorComponentFlagBits ::R_BIT |
		//	 Shit::ColorComponentFlagBits ::G_BIT |
		//	 Shit::ColorComponentFlagBits ::B_BIT |
		//	 Shit::ColorComponentFlagBits ::A_BIT}

	};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapper = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages),					  // PipelineShaderStageCreateInfo
			{},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)ranges::size(blendAttachmentStates),
				ranges::data(blendAttachmentStates),
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			1					  // subpass;
		});
}
void MaterialDeferred::createPipelineLight()
{
	auto pipelineLayout = _pipelineWrapper->getCreateInfoPtr()->pLayout;

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/triangle.vert.spv";
	static auto frag_file_spv = archiveName + "/pbrDeferred.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));
	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::DEFERRED);

	Shit::PipelineColorBlendAttachmentState blendAttachmentStates[]{
		{true,
		 Shit::BlendFactor::ONE,
		 Shit::BlendFactor::ONE,
		//  Shit::BlendFactor::SRC_ALPHA,
		//  Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits ::R_BIT |
			 Shit::ColorComponentFlagBits ::G_BIT |
			 Shit::ColorComponentFlagBits ::B_BIT |
			 Shit::ColorComponentFlagBits ::A_BIT},
		//{true,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::ColorComponentFlagBits ::R_BIT |
		//	 Shit::ColorComponentFlagBits ::G_BIT |
		//	 Shit::ColorComponentFlagBits ::B_BIT |
		//	 Shit::ColorComponentFlagBits ::A_BIT}

	};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapperLight = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages),					  // PipelineShaderStageCreateInfo
			{},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)ranges::size(blendAttachmentStates),
				ranges::data(blendAttachmentStates),
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			1					  // subpass;
		});
}
//==================
MaterialPBR::MaterialPBR(MaterialDataBlockManager *creator)
	: Material(creator, "PBR")
{
	createPipeline();
}
void MaterialPBR::createPipeline()
{
	auto pDevice = Root::getSingleton().getDevice();

	// create descriptor setlayout set3
	Shit::DescriptorSetLayoutBinding bindings[]{
		{1, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 8, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
	};
	std::vector<Shit::DescriptorSetLayout const *> setLayouts{
		Root::getSingleton().getDescriptorSetLayoutCamera(),
		Root::getSingleton().getDescriptorSetLayoutNode(),
		Root::getSingleton().getDescriptorSetLayoutSkin(),
		Root::getSingleton().getDescriptorSetLayoutLight(),
		Root::getSingleton().getDescriptorSetLayoutEnv(),
		pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings}),
	};

	// create pipeline layout
	auto pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)setLayouts.size(), setLayouts.data()});

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/common.vert.spv";
	static auto frag_file_spv = archiveName + "/IBL.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));
	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::FORWARD);

	Shit::PipelineColorBlendAttachmentState blendAttachmentStates[]{
		{true,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits ::R_BIT |
			 Shit::ColorComponentFlagBits ::G_BIT |
			 Shit::ColorComponentFlagBits ::B_BIT |
			 Shit::ColorComponentFlagBits ::A_BIT},
		//{true,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::ColorComponentFlagBits ::R_BIT |
		//	 Shit::ColorComponentFlagBits ::G_BIT |
		//	 Shit::ColorComponentFlagBits ::B_BIT |
		//	 Shit::ColorComponentFlagBits ::A_BIT}

	};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapper = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages), // PipelineShaderStageCreateInfo
			{
				(uint32_t)ranges::size(s_commonVertexBindings),
				ranges::data(s_commonVertexBindings),
				(uint32_t)ranges::size(s_commonVertexAttributes),
				ranges::data(s_commonVertexAttributes),
			},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)ranges::size(blendAttachmentStates),
				ranges::data(blendAttachmentStates),
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			0					  // subpass;
		});
}
void MaterialPBR::createPipelineLight()
{
	auto pDevice = Root::getSingleton().getDevice();

	// create descriptor setlayout set3
	Shit::DescriptorSetLayoutBinding bindings[]{
		{1, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 8, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
	};
	std::vector<Shit::DescriptorSetLayout const *> setLayouts{
		Root::getSingleton().getDescriptorSetLayoutCamera(),
		Root::getSingleton().getDescriptorSetLayoutNode(),
		Root::getSingleton().getDescriptorSetLayoutSkin(),
		Root::getSingleton().getDescriptorSetLayoutLight(),
		Root::getSingleton().getDescriptorSetLayoutEnv(),
		pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings}),
	};

	// create pipeline layout
	auto pipelineLayout = _pipelineWrapper->getCreateInfoPtr()->pLayout;

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/common.vert.spv";
	static auto frag_file_spv = archiveName + "/pbr.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));
	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::FORWARD);

	Shit::PipelineColorBlendAttachmentState blendAttachmentStates[]{
		{true,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits ::R_BIT |
			 Shit::ColorComponentFlagBits ::G_BIT |
			 Shit::ColorComponentFlagBits ::B_BIT |
			 Shit::ColorComponentFlagBits ::A_BIT},
		//{true,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::ColorComponentFlagBits ::R_BIT |
		//	 Shit::ColorComponentFlagBits ::G_BIT |
		//	 Shit::ColorComponentFlagBits ::B_BIT |
		//	 Shit::ColorComponentFlagBits ::A_BIT}

	};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapperLight = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages), // PipelineShaderStageCreateInfo
			{
				(uint32_t)ranges::size(s_commonVertexBindings),
				ranges::data(s_commonVertexBindings),
				(uint32_t)ranges::size(s_commonVertexAttributes),
				ranges::data(s_commonVertexAttributes),
			},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)ranges::size(blendAttachmentStates),
				ranges::data(blendAttachmentStates),
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			0					  // subpass;
		});
}
MaterialShadow::MaterialShadow(MaterialDataBlockManager *creator): Material(creator)
{
	auto pDevice = Root::getSingleton().getDevice();

	// create descriptor setlayout set3
	Shit::DescriptorSetLayoutBinding bindings[]{
		{1, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
		{0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 8, Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::VERTEX_BIT},
	};
	std::vector<Shit::DescriptorSetLayout const *> setLayouts{
		Root::getSingleton().getDescriptorSetLayoutCamera(),
		Root::getSingleton().getDescriptorSetLayoutNode(),
		Root::getSingleton().getDescriptorSetLayoutSkin(),
		Root::getSingleton().getDescriptorSetLayoutLight(),
		Root::getSingleton().getDescriptorSetLayoutEnv(),
		pDevice->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings}),
	};

	// create pipeline layout
	auto pipelineLayout = pDevice->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)setLayouts.size(), setLayouts.data()});

	//========================
	auto archiveName = getShaderArchiveName();
	// if(Root::getS)
	static auto vert_file_spv = archiveName + "/shadow.vert.spv";
	static auto geom_file_spv = archiveName + "/shadow.geom.spv";
	static auto frag_file_spv = archiveName + "/shadow.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);
	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));

	paras = ParameterMap{{"ShaderStage", "GEOM"}, {"ShadingLanguage", "SPV"}};
	auto geomShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{geom_file_spv, ResourceType::SHADER, nullptr, paras}));

	paras = ParameterMap{{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));
	vertShader->load();
	geomShader->load();
	fragShader->load();

	Shit::PipelineShaderStageCreateInfo stages[]{
		{
			Shit::ShaderStageFlagBits::VERTEX_BIT,
			vertShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::GEOMETRY_BIT,
			geomShader->getHandle(),
			"main",
		},
		{
			Shit::ShaderStageFlagBits::FRAGMENT_BIT,
			fragShader->getHandle(),
			"main",
		}};

	auto compatibleRenderPass = Root::getSingleton().getRenderPass(RenderPassType::SHADOW);

	Shit::PipelineColorBlendAttachmentState blendAttachmentStates[]{
		{false,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::BlendFactor::SRC_ALPHA,
		 Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		 Shit::BlendOp::ADD,
		 Shit::ColorComponentFlagBits ::R_BIT |
			 Shit::ColorComponentFlagBits ::G_BIT |
			 Shit::ColorComponentFlagBits ::B_BIT |
			 Shit::ColorComponentFlagBits ::A_BIT},
		//{true,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::BlendFactor::SRC_ALPHA,
		// Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
		// Shit::BlendOp::ADD,
		// Shit::ColorComponentFlagBits ::R_BIT |
		//	 Shit::ColorComponentFlagBits ::G_BIT |
		//	 Shit::ColorComponentFlagBits ::B_BIT |
		//	 Shit::ColorComponentFlagBits ::A_BIT}

	};
	Shit::DynamicState dynamicStates[]{
		Shit::DynamicState::VIEWPORT,
		Shit::DynamicState::SCISSOR};

	_pipelineWrapper = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)ranges::size(stages),
			ranges::data(stages), // PipelineShaderStageCreateInfo
			{
				(uint32_t)ranges::size(s_commonVertexBindings),
				ranges::data(s_commonVertexBindings),
				(uint32_t)ranges::size(s_commonVertexAttributes),
				ranges::data(s_commonVertexAttributes),
			},										  // VertexInputStateCreateInfo
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{},										  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				true,
				true,
				Shit::CompareOp::LESS,
				false,
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				(uint32_t)ranges::size(blendAttachmentStates),
				ranges::data(blendAttachmentStates),
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)ranges::size(dynamicStates),
				ranges::data(dynamicStates),
			},					  // PipelineDynamicStateCreateInfo
			pipelineLayout,		  // PipelineLayout
			compatibleRenderPass, // RenderPass
			0					  // subpass;
		});
}
MaterialSkybox::MaterialSkybox(MaterialDataBlockManager *creator) : Material(creator)
{
	auto device = Root::getSingleton().getDevice();

	auto pipelineLayout = Root::getSingleton().getCommonPipelineLayout();
	// device->Create(
	// 	Shit::PipelineLayoutCreateInfo{(uint32_t)std::size(setLayouts), setLayouts});

	auto archiveName = getShaderArchiveName();
	static auto vert_file_spv = archiveName + "/skybox.vert.spv";
	static auto frag_file_spv = archiveName + "/skybox.frag.spv";

	auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup(ResourceManager::DEFAULT_GROUP_NAME);

	ParameterMap paras{{"ShaderStage", "VERT"}, {"ShadingLanguage", "SPV"}};
	auto vertShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{vert_file_spv, ResourceType::SHADER, nullptr, paras}));

	paras = {{"ShaderStage", "FRAG"}, {"ShadingLanguage", "SPV"}};
	auto fragShader = static_cast<Shader *>(grp->createOrRetrieveResource(
		ResourceDeclaration{frag_file_spv, ResourceType::SHADER, nullptr, paras}));

	vertShader->load();
	fragShader->load();

	auto renderPass = Root::getSingleton().getRenderPass(RenderPassType::BACKGROUND);

	//========
	Shit::PipelineShaderStageCreateInfo stages[] = {
		{Shit::ShaderStageFlagBits::VERTEX_BIT, vertShader->getHandle(), "main"},
		{Shit::ShaderStageFlagBits::FRAGMENT_BIT, fragShader->getHandle(), "main"}};

	Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
		false,
	};
	colorBlendAttachmentState.colorWriteMask =
		Shit::ColorComponentFlagBits::R_BIT |
		Shit::ColorComponentFlagBits::G_BIT |
		Shit::ColorComponentFlagBits::B_BIT |
		Shit::ColorComponentFlagBits::A_BIT;

	Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
	Shit::Viewport viewport{};
	Shit::Rect2D scissor{};

	_pipelineWrapper = std::make_unique<GraphicPipelineWrapper>(
		Shit::GraphicsPipelineCreateInfo{
			(uint32_t)std::size(stages),
			stages,
			{},
			{Shit::PrimitiveTopology::TRIANGLE_LIST}, // PipelineInputAssemblyStateCreateInfo
			{1, &viewport, 1, &scissor},			  // PipelineViewportStateCreateInfo
			{},										  // PipelineTessellationStateCreateInfo
			{
				false,
				false,
				Shit::PolygonMode::FILL,
				Shit::CullMode::BACK,
				Shit::FrontFace::COUNTER_CLOCKWISE,
				false,
				0,
				0,
				0,
				1.f},							// PipelineRasterizationStateCreateInfo
			{Shit::SampleCountFlagBits::BIT_1}, // PipelineMultisampleStateCreateInfo
			{
				false,
			}, // PipelineDepthStencilStateCreateInfo
			{
				false,
				{},
				1,
				&colorBlendAttachmentState,
			}, // PipelineColorBlendStateCreateInfo
			{
				(uint32_t)std::size(dynamicStates),
				dynamicStates,
			}, // PipelineDynamicStateCreateInfo
			pipelineLayout,
			renderPass,
			0});
}

//==================================================
MaterialDataBlock::MaterialDataBlock(
	MaterialDataBlockManager *creator,
	MaterialType materialType,
	std::string_view group)
	: _materialType(materialType), GPUResource(creator, group, false)
{
}
MaterialDataBlock::MaterialDataBlock(
	MaterialDataBlockManager *creator,
	MaterialType materialType,
	std::string_view name,
	std::string_view group)
	: _materialType(materialType), GPUResource(creator, name, group, false)
{
}
void MaterialDataBlock::init()
{
}
MaterialDataBlock::~MaterialDataBlock()
{
}
Material *MaterialDataBlock::getMaterial()
{
	return static_cast<MaterialDataBlockManager *>(_creator)->getMaterial(_materialType);
}
void MaterialDataBlock::prepare(int index)
{
	//_descriptorSetData->prepare(index);
}
void MaterialDataBlock::upload(int index)
{
	if (_descriptorSetData)
		_descriptorSetData->upload(index);
}
void MaterialDataBlock::destroy(int index)
{
	if (_descriptorSetData)
		_descriptorSetData->destroy(index);
}
// size_t MaterialDataBlock::addTexture(Texture *texture)
//{
//	_textures.emplace_back(texture);
//	return _textures.size() - 1;
// }
void MaterialDataBlock::setDescriptorTexture(uint32_t startIndex, DescriptorTextureData const &texture)
{
	_descriptorSetData->setTexture(DESCRIPTOR_BINDING_MATERIAL_TEXTURE, startIndex, texture);
}
////========================================
MaterialDataBlockUnlitColor::MaterialDataBlockUnlitColor(MaterialDataBlockManager *creator, std::string_view group)
	: MaterialDataBlock(creator, MaterialType::UNLIT_COLOR, group)
{
	// init();
}
MaterialDataBlockUnlitColor::MaterialDataBlockUnlitColor(
	MaterialDataBlockManager *creator, std::string_view name, std::string_view group)
	: MaterialDataBlock(creator, MaterialType::UNLIT_COLOR, name, group)
{
	// init();
}
void MaterialDataBlockUnlitColor::init()
{
	// init set 3
	// auto buffer1 = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
	// 	BufferPropertyDesciption{
	// 		Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT |
	// 			Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
	// 		Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
	// 		false},
	// 	_group);
	// _para = {};

	// _paraBufferView = buffer1->allocate(sizeof(_para), 1, &_para);

	// _descriptorSetData->setBuffer(DESCRIPTOR_BINDING_MATERIAL_BUFFER, 0, _paraBufferView);

	// setColorFactor(0, 0, 0, 0);
}
void MaterialDataBlockUnlitColor::updateGpuBuffer()
{
	_paraBufferView.buffer->setData(
		_paraBufferView.offset, _paraBufferView.size(), &_para);
}
MaterialDataBlockUnlitColor *MaterialDataBlockUnlitColor::setColorFactor(float colorR, float colorG, float colorB, float colorA)
{
	_para.color = {colorR, colorG, colorB, colorA};
	return this;
}
MaterialDataBlockUnlitColor *MaterialDataBlockUnlitColor::setColorFactor(std::span<float const> color)
{
	ranges::copy(color, _para.color.begin());
	return this;
}
void MaterialDataBlockUnlitColor::setMaterial(MaterialUnlitColor::Para const &ubo)
{
	_para = ubo;
	updateGpuBuffer();
}
//====================
MaterialDataBlockPBR::MaterialDataBlockPBR(MaterialDataBlockManager *creator, std::string_view group)
	: MaterialDataBlock(creator, MaterialType::PBR, group)
{
	init();
}
MaterialDataBlockPBR::MaterialDataBlockPBR(MaterialDataBlockManager *creator, std::string_view name, std::string_view group)
	: MaterialDataBlock(creator, MaterialType::PBR, name, group)
{
	init();
}
void MaterialDataBlockPBR::init()
{
	auto material = static_cast<MaterialDataBlockManager *>(_creator)->getMaterial(_materialType);

	// 3 is default set number
	_descriptorSetData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		material->getPipelineWrapper()->getCreateInfoPtr()->pLayout->GetCreateInfoPtr()->pSetLayouts[DESCRIPTORSET_MATERIAL], false, _group);

	// init set 3
	auto buffer1 = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT |
				Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
			Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			false},
		_group);
	_para = {};

	_paraBufferView = buffer1->allocate(sizeof(_para), 1, &_para);

	//_paraView = {
	//	_paraBufferView,
	//	{
	//		{"Color Factor", DataType::VEC4, &_para.color},
	//	}};

	_descriptorSetData->setBuffer(
		DESCRIPTOR_BINDING_MATERIAL_BUFFER,
		// _descriptorSetData->getDescriptorSetLayoutCreateInfo()->pDescriptorSetLayoutBindings[0].binding,
		0, _paraBufferView);
}
void MaterialDataBlockPBR::updateGpuBuffer()
{
	_paraBufferView.buffer->setData(
		_paraBufferView.offset, _paraBufferView.size(), &_para);
}
void MaterialDataBlockPBR::setMaterial(MaterialPBR::Para const &para)
{
	_para = para;
	updateGpuBuffer();
}

//============================
MaterialDataBlockManager::MaterialDataBlockManager()
{
	for (size_t i = 0; i < _materials.size(); ++i)
	{
		auto a = (MaterialType)i;
		switch (a)
		{
		case MaterialType::UNLIT_COLOR:
			_materials[i] = std::make_unique<MaterialUnlitColor>(this);
			_defaultMaterialDataBlocks[i] = std::make_unique<MaterialDataBlockUnlitColor>(this, DEFAULT_GROUP_NAME);
			break;
		case MaterialType::PBR:
			_materials[i] = std::make_unique<MaterialPBR>(this);
			_defaultMaterialDataBlocks[i] = std::make_unique<MaterialDataBlockPBR>(this, DEFAULT_GROUP_NAME);
			break;
		}
	}
}
Material *MaterialDataBlockManager::getMaterial(MaterialType materialType)
{
	auto &p = _materials.at(static_cast<size_t>(materialType));
	if (!p)
	{
		switch (materialType)
		{
		case MaterialType::UNLIT_COLOR:
			p = std::make_unique<MaterialUnlitColor>(this);
			break;
		case MaterialType::PBR:
			p = std::make_unique<MaterialPBR>(this);
			break;
		case MaterialType::SKYBOX:
			p = std::make_unique<MaterialSkybox>(this);
			break;
		case MaterialType::GPASS:
			p = std::make_unique<MaterialGPass>(this);
			break;
		case MaterialType::DEFERRED:
			p = std::make_unique<MaterialDeferred>(this);
			break;
		case MaterialType::SHADOW:
			p = std::make_unique<MaterialShadow>(this);
			break;
		}
	}
	return p.get();
}
MaterialDataBlock *MaterialDataBlockManager ::createMaterialDataBlock(
	MaterialType materialType, std::string_view name, std::string_view group)
{
	MaterialDataBlock *ret;
	switch (materialType)
	{
	case MaterialType::UNLIT_COLOR:
		ret = new MaterialDataBlockUnlitColor(this, name, group);
		break;
	case MaterialType::PBR:
		ret = new MaterialDataBlockPBR(this, name, group);
		break;
	default:
		return 0;
	}
	add(ret);
	return ret;
}
MaterialDataBlock *MaterialDataBlockManager::getDefaultMaterialBlock(MaterialType materialType) const
{
	return _defaultMaterialDataBlocks.at((size_t)materialType).get();
}