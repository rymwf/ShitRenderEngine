#include "screen.hpp"
#include "root.hpp"

static Shit::WindowPixelFormat chooseSwapchainFormat(
	const std::vector<Shit::WindowPixelFormat> &candidates,
	Shit::Device *pDevice,
	Shit::Window *pWindow)
{
	std::vector<Shit::WindowPixelFormat> supportedFormats;
	pDevice->GetWindowPixelFormats(pWindow, supportedFormats);
	ST_LOG("supported formats:");
	for (auto &&e : supportedFormats)
	{
		ST_LOG_VAR(static_cast<int>(e.format));
		ST_LOG_VAR(static_cast<int>(e.colorSpace));
		for (auto &&a : candidates)
		{
			if (e.format == a.format && e.colorSpace == a.colorSpace)
				return e;
		}
	}
	return supportedFormats[0];
}
static Shit::PresentMode choosePresentMode(
	const std::vector<Shit::PresentMode> &candidates,
	Shit::Device *pDevice,
	Shit::Window *window)
{
	std::vector<Shit::PresentMode> modes;
	pDevice->GetPresentModes(window, modes);
	for (auto &&a : candidates)
	{
		for (auto &&e : modes)
		{
			if (static_cast<int>(e) == static_cast<int>(a))
				return e;
		}
	}
	return modes[0];
}

Screen::Screen(const wchar_t *windowTitle) : _windowTitle(windowTitle)
{
	// 1. create window
	//		std::make_shared<std::function<void(const Shit::Event &)>>(std::bind(&AppBase::processBaseEvent, this, std::placeholders::_1))};
	_window = Root::getSingleton().getRenderSystem()->CreateRenderWindow(
		Shit::WindowCreateInfo{
			Shit::WindowCreateFlagBits::FIXED_SIZE,
			_windowTitle,
			{{80, 40},
			 {1280, 720}},
		});
}

void Screen::recreateSwapchain()
{
	// destroy window resources
	for (auto e : _swapchainImageViews)
		Root::getSingleton().getDevice()->Destroy(e);
	Root::getSingleton().getDevice()->Destroy(_swapchain);
	createSwapchain();
	_swapchainRecreateSignal();
}
void Screen::addSwapchainRecreateCallback(const std::function<void()> &func, int group)
{
	_swapchainRecreateSignal.Connect(group, func);
}

void Screen::createSwapchain()
{
	auto swapchainFormat = chooseSwapchainFormat(
		{
			{Shit::Format::B8G8R8A8_SRGB, Shit::ColorSpace::SRGB_NONLINEAR},
			{Shit::Format::R8G8B8A8_SRGB, Shit::ColorSpace::SRGB_NONLINEAR},
		},
		Root::getSingleton().getDevice(), _window);
	LOG_VAR(static_cast<int>(swapchainFormat.format));
	LOG_VAR(static_cast<int>(swapchainFormat.colorSpace));

	auto presentMode = choosePresentMode(
		{Shit::PresentMode::IMMEDIATE, Shit::PresentMode::MAILBOX, Shit::PresentMode::FIFO},
		Root::getSingleton().getDevice(), _window);
	ST_LOG("selected present mode:", static_cast<int>(presentMode));

	uint32_t framebufferWidth, framebufferHeight;
	_window->GetFramebufferSize(framebufferWidth, framebufferHeight);
	while (framebufferWidth == 0 || framebufferHeight == 0)
	{
		_window->WaitEvents();
		_window->GetFramebufferSize(framebufferWidth, framebufferHeight);
	}
	_swapchain = Root::getSingleton().getDevice()->Create(
		Shit::SwapchainCreateInfo{
			2, // min image count
			swapchainFormat.format,
			swapchainFormat.colorSpace,
			{framebufferWidth, framebufferHeight},
			1,
			Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::TRANSFER_SRC_BIT,
			presentMode,
			_window});

	// create swapchain imageView
	auto imageCount = _swapchain->GetImageCount();
	_swapchainImageViews.resize(imageCount);
	Shit::ImageViewCreateInfo imageViewCreateInfo{
		{},
		Shit::ImageViewType::TYPE_2D,
		_swapchain->GetCreateInfoPtr()->format,
		{},
		{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		imageViewCreateInfo.pImage = _swapchain->GetImageByIndex(i);
		_swapchainImageViews[i] = Root::getSingleton().getDevice()->Create(imageViewCreateInfo);
	}
}