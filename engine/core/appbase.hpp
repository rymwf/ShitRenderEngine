#pragma once
#include "imgui-impl.hpp"
#include "camera.hpp"
#include "timer.hpp"
#include "root.hpp"
#include "sceneNode.hpp"
#include "configFile.hpp"
#include "shader.hpp"
#include "sceneManager.hpp"
#include "singleton.hpp"
#include "compositorWorkspace.hpp"
#include "image.hpp"
#include "modelResource.hpp"
#include "resourceGroup.hpp"
#include "texture.hpp"
#include "collider.hpp"
#include "light.hpp"

class App
{
private:
	Timer _timer;

	//===========================
	bool _startScreenshot{};

	//====================
	// gui
	std::vector<Shit::CommandBuffer *> _guiPrimaryCommandBuffers;

	////managers
	// std::unique_ptr<CameraManager> _cameraManager;

	void processEvent(const Shit::Event &ev);

	std::vector<Shit::CommandBuffer *> beforeOneFrame(uint32_t imageIndex);

	// gui
	void initGui();
	void guiRecreateSwapchainCallback();
	void updateGui(uint32_t imageIndex);
	void showMenuBar();
	void showExampleAppSimpleOverlay(bool *p_open);
	virtual void updateGuiImp(uint32_t imageIndex) {}

	virtual void prepare() {}
	virtual void recreateSwapchainCallback() {}

protected:
	CompositorWorkspace *_curWorkspace{};

	constexpr void setCurrentWorkspace(CompositorWorkspace *workspace) { _curWorkspace = workspace; };

	/**
	 * @brief Create a Shader object
	 *
	 * @param name
	 * @param isSpirv
	 * @return Shader*
	 */
	// Shader *createShader(std::string_view name, bool isSpirv = true);

public:
	App();
	virtual ~App();

	void run(const wchar_t *windowTitle, Shit::SampleCountFlagBits sampleCount = Shit::SampleCountFlagBits::BIT_1);
};