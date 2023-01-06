/**
 * @file light.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include "prerequisites.hpp"
#include "component.hpp"
#include "viewLayer.hpp"

enum LightRenderMode
{
	AUTO,
	VERTEX,
	PIXEL,
};

enum LightType
{
	LIGHT_DIRECTIONAL,
	LIGHT_POINT,
	LIGHT_SPHERE,
	LIGHT_SPOT,
	LIGHT_TUBE,
	LIGHT_DISK_SINGLE_FACE,
	LIGHT_QUAD_SINGLE_FACE,
	LIGHT_POLYGON_SINGLE_FACE, // vertex num <=10
};

struct LightPara
{
	alignas(16) glm::mat4 transformMatrix;
	float color[3]{1, 1, 1};
	int lightType{}; // 0 directional

	glm::vec3 radius{1, 1, 1};
	float rmax{100};

	float radiance{1.};
	float cosThetaU{0.5}; // umbra
	float cosThetaP{0.4}; // penumbra
	int vertexNum{0};

	glm::vec4 vertices[4]{};
	glm::mat4 P[6];
	glm::mat4 V[6];
};

class Light : public Component
{
	bool _enableShadow{true};
	bool _enable{true};

	// int _renderLayerMask{1};

	float _shadowBias{0};
	float _shadowNear{0.1};
	float _shadowFar{1000};
	float _shadowRadius{1.};

	Shit::Extent2D _shadowResolution;

	LightPara _lightPara{};
	BufferView _gpuBufferView;
	IdType _setDataId;

	std::vector<glm::vec3> _cascadeFrustumVertices;

	//
	Texture *_shadowTexture;
	std::vector<Shit::Framebuffer *> _shadowFramebuffers;

	void createShadowTextures();
	void createFramebuffers();

	void updateLightPVs();

	void prepareDescriptorSet();

public:
	Light(SceneNode *parent, LightType lightType = LIGHT_DIRECTIONAL);
	~Light() {}
	Light(Light const &other);

	NODISCARD Component *clone()
	{
		return new Light(*this);
	}

	DescriptorSetData *getDescriptorSetData() const;

	constexpr Shit::Framebuffer *getShadowFramebuffer(uint32_t frameIndex) const { return _shadowFramebuffers.at(frameIndex); };

	constexpr LightPara &getLightParaRef() { return _lightPara; }

	Shit::Extent2D getShadowResolution() const { return _shadowResolution; }

	void updateGPUData(uint32_t frameIndex) override;

	void prepare() override;

	void onNodeUpdated() override;

	void setCascadeFrustumVertices(std::span<glm::vec3 const> frustumVertices);

	void cameraListener(Camera const *camera);
};
