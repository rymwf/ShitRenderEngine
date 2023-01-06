#pragma once
#include "prerequisites.hpp"

class Screen
{
	const wchar_t *_windowTitle;

	Shit::Window *_window;

	// swapchains
	Shit::Swapchain *_swapchain;
	std::vector<Shit::ImageView *> _swapchainImageViews;

	// signals
	Shit::Signal<void()> _swapchainRecreateSignal;

public:
	Screen(const wchar_t *windowTitle);

	void createSwapchain();

	void addSwapchainRecreateCallback(const std::function<void()> &func, int group = 0);

	void addWindowEventCallback(std::function<void(const Shit::Event &)> func)
	{
		_window->AddEventListener(func);
	}
	constexpr Shit::Window *getWindow() const { return _window; }

	constexpr Shit::Swapchain *getSwapchain() const { return _swapchain; }
	constexpr Shit::ImageView *getSwapchainImageViewByIndex(size_t index) const
	{
		return _swapchainImageViews.at(index);
	}

	Shit::Extent2D const &getFramebufferSize() const
	{
		Shit::Extent2D ret;
		_window->GetFramebufferSize(ret.width, ret.height);
		return ret;
	}

	void recreateSwapchain();
};