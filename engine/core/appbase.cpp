#include "appbase.hpp"
#include "compositor.hpp"

Shit::RendererVersion s_rendererVersion{Shit::RendererVersion::VULKAN};
static glm::vec3 ambientColor = glm::vec3(0.2);

App::App()
{
}
App::~App()
{
	Root::releaseSingleton();
}
void App::processEvent(const Shit::Event &ev)
{
	ImGuiIO &io = ImGui::GetIO();
	std::visit(
		Shit::overloaded{
			[&](const Shit::KeyEvent &value)
			{
				if (value.keyCode == Shit::KeyCode::KEY_ESCAPE)
					ev.pWindow->Close();
				_startScreenshot = io.KeysDown[Shit::KeyCode::KEY_P] = value.keyCode == Shit::KeyCode::KEY_P && value.state;
				io.KeysDown[Shit::KeyCode::KEY_BACK] = value.keyCode == Shit::KeyCode::KEY_BACK && value.state;
				if (value.keyCode == Shit::KeyCode::KEY_W)
				{
					io.KeysDown[Shit::KeyCode::KEY_W] = value.state;
				}
				if (value.keyCode == Shit::KeyCode::KEY_S)
				{
					io.KeysDown[Shit::KeyCode::KEY_S] = value.state;
				}
				if (value.keyCode == Shit::KeyCode::KEY_A)
				{
					io.KeysDown[Shit::KeyCode::KEY_A] = value.state;
				}
				if (value.keyCode == Shit::KeyCode::KEY_D)
				{
					io.KeysDown[Shit::KeyCode::KEY_D] = value.state;
				}
				//_cameraManager.getMainCameraController()
				//	->keyBoardEventCallback(value, _timer.getElapsedTime<Timer::Ms>());
			},
			[&](const Shit::CharEvent &value)
			{
				io.AddInputCharacter(value.codepoint);
			},
			[](auto &&) {},
			[&](const Shit::WindowResizeEvent &value)
			{
				uint32_t width, height;
				ev.pWindow->GetFramebufferSize(width, height);
				io.DisplaySize.x = width;
				io.DisplaySize.y = height;
			},
			[&](const Shit::MouseButtonEvent &value)
			{
				if (value.state)
				{
					io.MouseDown[0] = value.button == Shit::MouseButton::MOUSE_L;
					io.MouseDown[1] = value.button == Shit::MouseButton::MOUSE_R;
					io.MouseDown[2] = value.button == Shit::MouseButton::MOUSE_M;
				}
				else
				{
					io.MouseDown[0] = 0;
					io.MouseDown[1] = 0;
					io.MouseDown[2] = 0;
				}
				//_cameraManager.getMainCameraController()
				//	->mouseButtonEventCallback(value, _timer.getElapsedTime<Timer::Ms>());
			},
			[&](const Shit::MouseMoveEvent &value)
			{
				// imgui
				io.MousePos.x = value.xpos;
				io.MousePos.y = value.ypos;
				//_cameraManager.getMainCameraController()
				//	->mouseMoveEventCallback(value);
			},
			[&](const Shit::MouseWheelEvent &value)
			{
				// imgui
				io.MouseWheelH += value.xoffset;
				io.MouseWheel += value.yoffset;
				if (!io.WantCaptureMouse)
				{
					//_cameraManager.getMainCameraController()
					//	->mouseWheelEventCallback(value);
				}
			},
		},
		ev.value);
}
void App::initGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	uint32_t frameWidth, frameHeight;
	Root::getSingleton().getScreen()->getWindow()->GetFramebufferSize(frameWidth, frameHeight);
	io.DisplaySize = ImVec2{float(frameWidth), float(frameHeight)};
	auto imageCount = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();
	_guiPrimaryCommandBuffers.resize(imageCount);
	Root::getSingleton().getCommandPool()->CreateCommandBuffers(
		Shit::CommandBufferCreateInfo{
			Shit::CommandBufferLevel::PRIMARY,
			imageCount},
		_guiPrimaryCommandBuffers.data());

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsClassic();

	std::vector<Shit::Image *> images(imageCount);
	Root::getSingleton().getScreen()->getSwapchain()->GetImages(images.data());

	ImGui_ImplShitRenderer_InitInfo imguiInitInfo{
		s_rendererVersion,
		Root::getSingleton().getDevice(),
		images,
		_guiPrimaryCommandBuffers,
		Shit::ImageLayout::PRESENT_SRC,
		Shit::ImageLayout::PRESENT_SRC};

	ImGui_ImplShitRenderer_Init(&imguiInitInfo);

	Root::getSingleton().executeOneTimeCommand(
		[](Shit::CommandBuffer *commandBuffer)
		{ ImGui_ImplShitRenderer_CreateFontsTexture(commandBuffer); });
	ImGui_ImplShitRenderer_DestroyFontUploadObjects();

	Root::getSingleton().getScreen()->addSwapchainRecreateCallback(std::bind(&App::guiRecreateSwapchainCallback, this));
}
void App::guiRecreateSwapchainCallback()
{
	auto swapchain = Root::getSingleton().getScreen()->getSwapchain();
	std::vector<Shit::Image *> images(swapchain->GetImageCount());
	swapchain->GetImages(images.data());
	ImGui_ImplShitRenderer_RecreateFrameBuffers(images);
}
void App::updateGui(uint32_t imageIndex)
{
	ImGui_ImplShitRenderer_NewFrame();
	ImGui::NewFrame();

	showMenuBar();

	updateGuiImp(imageIndex);
	ImGui::Render();
	ImGui_ImplShitRenderer_RecordCommandBuffer(imageIndex);
}
void App::showMenuBar()
{
	static bool demoWindow = false;
	static bool metricsWindow = false;
	static bool simpleOverlay = true;
	// main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			// if (ImGui::MenuItem("Exit"))
			//{
			//	g_App->getWindow()->Close();
			// }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("GameObject"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Component"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Demo Window"))
			{
				demoWindow = true;
			}
			if (ImGui::MenuItem("Metrics Window"))
			{
				metricsWindow = true;
			}
			if (ImGui::MenuItem("Simple Overlay"))
			{
				simpleOverlay = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	//==========================
	if (demoWindow)
		ImGui::ShowDemoWindow(&demoWindow);

	if (metricsWindow)
		ImGui::ShowMetricsWindow(&metricsWindow);

	// if (simpleOverlay)
	//	showExampleAppSimpleOverlay(&simpleOverlay);
}
void App::showExampleAppSimpleOverlay(bool *p_open)
{
	static const float PAD = 10.0f;
	static int corner = 3;
	ImGuiIO &io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	if (corner != -1)
	{
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
		window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
		window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
		window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		window_flags |= ImGuiWindowFlags_NoMove;
	}
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	if (ImGui::Begin("Example: Simple overlay", p_open, window_flags))
	{
		ImGui::Text("press tab to switch camera mode");
		//		switch (_scene->GetMainCamera()->GetMode())
		//		{
		//		case 0:
		//		default:
		//     			ImGui::Text("\
//mouse left: rotate\n\
//mouse wheel: zoom \n\
//ctrl+mouse left/mouse middle: drag scene\
//		");
		//			break;
		//		case 1:
		//   			ImGui::Text("\
//Key WASD: move\
//		");
		//			break;
		//		}
		//		//ImGui::Text("Simple overlay\n" "in the corner of the screen.\n" "(right-click to change position)");
		//		//ImGui::Separator();
		//		if (ImGui::IsMousePosValid())
		//			ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
		//		else
		//			ImGui::Text("Mouse Position: <invalid>");
		//		ImGui::Text("FPS(wrong): %.1f", io.Framerate);
		//		if (ImGui::BeginPopupContextWindow())
		//		{
		//			if (ImGui::MenuItem("Custom", NULL, corner == -1))
		//				corner = -1;
		//			if (ImGui::MenuItem("Top-left", NULL, corner == 0))
		//				corner = 0;
		//			if (ImGui::MenuItem("Top-right", NULL, corner == 1))
		//				corner = 1;
		//			if (ImGui::MenuItem("Bottom-left", NULL, corner == 2))
		//				corner = 2;
		//			if (ImGui::MenuItem("Bottom-right", NULL, corner == 3))
		//				corner = 3;
		//			if (p_open && ImGui::MenuItem("Close"))
		//				*p_open = false;
		//			ImGui::EndPopup();
		//		}
	}
	ImGui::End();
}
void App::run(const wchar_t *windowTitle, Shit::SampleCountFlagBits sampleCount)
{
	Root::getSingleton().init(s_rendererVersion, windowTitle, sampleCount);
	initGui();
	Root::getSingleton().getScreen()->getWindow()->AddEventListener(std::bind(&App::processEvent, this, std::placeholders::_1));
	Root::getSingleton().addBeforeOneFrameCallback(
		std::bind(&App::beforeOneFrame, this, std::placeholders::_1));
	Root::getSingleton().getScreen()->addSwapchainRecreateCallback(std::bind(&App::recreateSwapchainCallback, this));

	prepare();

	_curWorkspace->build();
	Root::getSingleton().render();
}
std::vector<Shit::CommandBuffer *> App::beforeOneFrame(uint32_t imageIndex)
{
	std::vector<Shit::CommandBuffer *> ret;
	if (_curWorkspace)
	{
		_curWorkspace->update(imageIndex);
		ret = {_curWorkspace->getCommandBuffer(imageIndex)};
	}
	updateGui(imageIndex);
	ret.emplace_back(_guiPrimaryCommandBuffers[imageIndex]);
	return ret;
}