#include "core/entry.hpp"

const char *testImageName[] = {
	"images/0.jpg",
	"images/1.jpg",
};

class Hello : public App
{
	std::vector<Image *> imagejpg;

	void loadResources()
	{
		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();

		auto l = ranges::size(testImageName);
		for (size_t i = 0; i < l; ++i)
			grp->declareResource(ResourceDeclaration{testImageName[i], ResourceType::IMAGE});

		grp->createDeclaredResources();
		grp->loadDeclaredResources();

		for (size_t i = 0; i < l; ++i)
			imagejpg.emplace_back(static_cast<Image *>(grp->getResource(testImageName[i])));
	}
	void prepareCompositor()
	{
		auto compositorManager = Root::getSingleton().getCompositorManager();
// example 0
#if 0
		CompositorWorkspaceDescription compositorWorkspaceDescription{
			{
				CNCompositeDescription{},
				CNColorDescription{{1.f, 0.f, 0.f}},
			},
			{
				//
				{1, 0, 0, 0},
			}};
#endif
// example 1
#if 0
		CompositorWorkspaceDescription compositorWorkspaceDescription{
			{
				CNCompositeDescription{},
				CNImageDescription{imagejpg[0], Shit::Format::R8G8B8A8_UNORM},
			},
			{
				//
				{1, 0, 0, 0},
			}};
#endif
// example 2
#if 1
		CompositorWorkspaceDescription compositorWorkspaceDescription{
			{
				CNCompositeDescription{},
				CNImageDescription{imagejpg[0], Shit::Format::R8G8B8A8_UNORM},
				CNImageDescription{imagejpg[1], Shit::Format::R8G8B8A8_SRGB},
				CNColorMixDescription{},
				CNControlValueDescription{0.5},
			},
			{
				//
				{1, 0, 3, 1},
				{2, 0, 3, 2},
				{3, 0, 0, 0},
				{4, 0, 3, 0},
			}};
#endif
		auto compositorWorkspace = compositorManager->createCompositorWorkspace(compositorWorkspaceDescription, "aa");
		setCurrentWorkspace(compositorWorkspace);
	}
	void prepare() override
	{
		loadResources();
		prepareCompositor();
	}
	void recreateSwapchainCallback() override
	{
	}
	void updateGuiImp(uint32_t imageIndex) override
	{
		if (ImGui::Begin("aaa"))
		{
			static float v = 0.5;
			ImGui::Text("compositor node mix factor");
			if (ImGui::DragFloat("mix factor", &v, 0.01, 0, 1))
			{
				static_cast<CNControlValue*>(
					Root::getSingleton()
						.getCompositorManager()
						->getCompositorWorkspace("aa")
						->getCompositorNodeByIndex(4))
					->setValue(v);
			}
			ImGui::End();
		}
	};
};
EXAMPLE_MAIN(Hello)
