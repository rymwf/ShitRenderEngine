#include "appbase.hpp"
extern Shit::RendererVersion s_rendererVersion;

#ifdef _WIN32

#ifndef UNICODE
#define UNICODE 1
#endif
#include <windows.h>
#include <combaseapi.h>

inline void parseArgument(int ac, LPWSTR *av)
{
	// for (int i = 0; i < ac; ++i)
	//{
	//	LOG(av[i]);
	// }
	if (ac > 1)
	{
		if (wcscmp(av[1], L"GL") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::GL;
		}
		else if (wcscmp(av[1], L"VK") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN; // latest
		}
		else if (wcscmp(av[1], L"VK110") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN_110;
		}
		else if (wcscmp(av[1], L"VK120") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN_120;
		}
		else if (wcscmp(av[1], L"VK130") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN_130;
		}
	}
}

#define EXAMPLE_MAIN(A, ...)                                                          \
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) \
	{                                                                                 \
		LPWSTR *argv;                                                                 \
		int argc;                                                                     \
		argv = CommandLineToArgvW(GetCommandLineW(), &argc);                          \
		parseArgument(argc, argv);                                                    \
		LocalFree(argv);                                                              \
		try                                                                           \
		{                                                                             \
			A app;                                                                    \
			app.run(__WFILE__, __VA_ARGS__);                                          \
		}                                                                             \
		catch (const std::exception &e)                                               \
		{                                                                             \
			std::cout << e.what() << std::endl;                                       \
		}                                                                             \
	}
#else
inline void parseArgument(int ac, char **av)
{
	for (int i = 0; i < ac; ++i)
	{
		std::cout << av[i] << std::endl;
	}
	if (ac > 1)
	{
		if (strcmp(av[1], "GL") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::GL;
		}
		else if (strcmp(av[1], "VK") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN;
		}
		else if (strcmp(av[1], "VK110") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN_110;
		}
		else if (strcmp(av[1], "VK120") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN_120;
		}
		else if (strcmp(av[1], "VK130") == 0)
		{
			s_rendererVersion = Shit::RendererVersion::VULKAN_130;
		}
	}
}
#define EXAMPLE_MAIN(A, ...)                    \
	int main(int argc, char **argv)             \
	{                                           \
		parseArgument(argc, argv);              \
		try                                     \
		{                                       \
			A app;                              \
			app.run(__WFILE__, __VA_ARGS__);    \
		}                                       \
		catch (const std::exception &e)         \
		{                                       \
			std::cout << e.what() << std::endl; \
		}                                       \
	}
#endif