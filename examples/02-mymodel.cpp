#include "core/entry.hpp"
#include "core/myMesh.hpp"

const char *testImageName[] = {
	"images/0.jpg",
};
class Hello : public App
{
	std::vector<Image *> imagejpg;

	std::unique_ptr<MyModel> axisModelSrc;
	ModelResource *axisModelResource;
	// Model *axisModel;

	std::vector<std::unique_ptr<MyModel>> testModelSrcs;
	std::vector<ModelResource *> testModelResources;
	std::vector<Model *> testModels;

	Scene *scene;

	using SelectedObject_T = std::variant<
		SceneNode *>;
	// SelectedObject_T selectedObject;

	void loadResources()
	{
		// create model
		axisModelSrc = MyModelFactory::getSingleton().buildModel("axis", MyModelType::AXIS);

		testModelSrcs.emplace_back(MyModelFactory::getSingleton().buildModel("triangle", MyModelType::TRIANGLE));
		testModelSrcs.emplace_back(MyModelFactory::getSingleton().buildModel("quad", MyModelType::QUAD));
		testModelSrcs.emplace_back(MyModelFactory::getSingleton().buildModel("sphere", MyModelType::SPHERE));
		testModelSrcs.emplace_back(MyModelFactory::getSingleton().buildModel("circle", MyModelType::CIRCLE));
		testModelSrcs.emplace_back(MyModelFactory::getSingleton().buildModel("cube", MyModelType::CUBE));

		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();
		for (auto &&e : testModelSrcs)
		{
			auto p = e.get();
			auto pModelResource = static_cast<ModelResource *>(grp->createResource(
				ResourceDeclaration{
					e->name,
					ResourceType::MODEL,
					&MyModelLoader::getSingleton()},
				sizeof(ptrdiff_t), &p));

			pModelResource->load();
			testModelResources.emplace_back(pModelResource);
		}
		auto p = axisModelSrc.get();
		axisModelResource = static_cast<ModelResource *>(grp->createResource(
			ResourceDeclaration{
				p->name,
				ResourceType::MODEL,
				&MyModelLoader::getSingleton()},
			sizeof(ptrdiff_t), &p));
		axisModelResource->load();

		//=====================================
		// auto l = std::size(testImageName);
		// for (size_t i = 0; i < l; ++i)
		//	grp->declareResource(ResourceDeclaration{testImageName[i], ResourceType::IMAGE});
		// grp->createDeclaredResources();
		// grp->loadDeclaredResources();

		// for (size_t i = 0; i < l; ++i)
		//	imagejpg.emplace_back(static_cast<Image *>(grp->getResource(testImageName[i])));
	}
	void createScene()
	{
		auto sceneManager = Root::getSingleton().getSceneManager();
		scene = sceneManager->createScene("testscene");

		auto axisModel = scene->createModel(axisModelResource)->scale(glm::vec3(20.f, 20.f, 20.f));

		for (auto p : testModelResources)
			testModels.emplace_back(scene->createModel(p));

		auto pos = glm::vec3(-5, 0, 0);
		for (auto p : testModels)
		{
			p->translate(pos);
			pos += glm::vec3(2, 0, 0);
		}
		scene->prepare(); // must
	}
	void prepareCompositor()
	{
		auto compositorManager = Root::getSingleton().getCompositorManager();
		CompositorWorkspaceDescription compositorWorkspaceDescription{
			{
				CNCompositeDescription{},
				CNRenderLayerDescription{"testscene", 0},
				// CompositorNodeImageDescription{imagejpg[0], Shit::Format::R8G8B8A8_SRGB},
				//  CompositorNodeImageDescription{imagejpg[1], Shit::Format::R8G8B8A8_SRGB},
				// CompositorNodeMixDescription{},
			},
			{
				{1, 0, 0, 0},
				//{1, 0, 3, 1},
				//{2, 0, 3, 2},
				//{3, 0, 0, 0},
			}};
		auto compositorWorkspace = compositorManager->createCompositorWorkspace(compositorWorkspaceDescription, "aa");
		setCurrentWorkspace(compositorWorkspace);
		static_cast<CNComposite *>(compositorWorkspace->getCompositorNodeByIndex(0))->setAlpha(1.);
	}
	void prepare() override
	{
		loadResources();
		createScene();
		prepareCompositor();
	}
	void recreateSwapchainCallback() override
	{
	}
	void updateGuiImp(uint32_t imageIndex) override
	{
		showCameraControlWindow();
		bool isOpen;
		showExampleAppSimpleOverlay(&isOpen);

		// node hierarchy
		showNodeHierarchy();
	}
	void showExampleAppSimpleOverlay(bool *p_open)
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
			ImGui::Text("frame time: %d ms", Root::getSingleton().getFrameDeltaTimeMs());
			ImGui::End();
		}
	}
	void showCameraControlWindow()
	{
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 windowPos{300.f, viewport->WorkPos.y};

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		static bool isOpen = true;
		if (ImGui::Begin("camera control", &isOpen, window_flags))
		{
			auto buttonSize = ImVec2(60, 30);

			if (ImGui::Button("scene", buttonSize))
			{
				scene->renderEditorCamera();
			}
			ImGui::SameLine();
			if (ImGui::Button("game", buttonSize))
			{
				scene->renderActiveCamera();
			}
			ImGui::End();
		}
	}
	void showNodeHierarchy()
	{
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 windowPos{viewport->WorkPos};
		ImVec2 windowSize{300.f, viewport->WorkSize.y * 2.f / 3.f};

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize);
		ImGui::Begin("Hierarchy");

		showNodeHierarchyHelper(scene->getRootNode());

		showInspector();
		ImGui::End();
	}
	void showNodeHierarchyHelper(SceneNode *sceneNode)
	{
		ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		static bool test_drag_and_drop = true;

		if (sceneNode->childrenNum() == 0)
			base_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		auto nodeId = sceneNode->getId();

		// const bool is_selected = (selection_mask & (1 << index)) != 0;
		if (scene->containSelectedNode(sceneNode))
		{
			base_flags |= ImGuiTreeNodeFlags_Selected;
		}
		// 'selection_mask' is dumb representation of what may be user-side selection state.
		//  You may retain selection state inside or outside your objects in whatever format you see fit.
		// 'node_clicked' is temporary storage of what node we have clicked to process selection at the end
		/// of the loop. May be a pointer to your own node type, etc.
		// int node_clicked = -1;

		// intptr_t;
		// bool isOpen=ImGui::TreeNodeEx((void *)(intptr_t)index, base_flags, "%s", name);
		auto name = sceneNode->getName().data();

		auto isOpen = ImGui::TreeNodeEx((void *)nodeId, base_flags, "%s", name);

		if (ImGui::IsItemClicked())
		{
			// sceneNode->setSelected(true);
			// selectedNodeId = sceneNode->getId();
			scene->setSelectedNode(sceneNode);
		}

		if (isOpen)
		{
			for (auto it = sceneNode->childBegin(); it < sceneNode->childEnd(); ++it)
				showNodeHierarchyHelper(static_cast<SceneNode *>(it->get()));

			if (!(base_flags & ImGuiTreeNodeFlags_Leaf))
				ImGui::TreePop();
		}
	}

	void showInspector()
	{
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 windowPos{viewport->WorkSize.x - 300.f, viewport->WorkPos.y};
		ImVec2 windowSize{300.f, viewport->WorkSize.y * 2.f / 3.f};

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize);
		ImGui::Begin("inspector");
		auto selectedNodes = scene->getSeletectedNodes();
		if (!selectedNodes.empty())
		{
			auto a = *selectedNodes.rbegin();
			showInspectorSceneNode(a);
		}
		ImGui::End();
	}
	void showInspectorSceneNode(SceneNode *sceneNode)
	{
		if (!sceneNode)
			return;

		ImGui::Text(sceneNode->getName().data());

		static bool flag = false;
		static bool flag2 = false;
		flag = false;

		auto &&transform = sceneNode->getLocalTransform();

		glm::vec3 translation = transform.translation;
		glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform.rotation));
		glm::vec3 scale = transform.scale;

		if (ImGui::DragFloat3("position", (float *)&translation, 0.1f))
		{
			sceneNode->setLocalTranslation(translation);
		}
		if (ImGui::DragFloat3("rotation", (float *)&rotation, 1.f))
		{
			sceneNode->setLocalRotation(glm::quat(glm::radians(rotation)));
		}
		if (ImGui::DragFloat3("scale", (float *)&scale, 1.f, 0.1f))
		{
			sceneNode->setLocalScale(scale);
		}
		std::vector<Component *> components;
		sceneNode->getComponents(components);
		
		//for (auto e : components)
		//{
		//	switch (e->getComponentType())
		//	{
		//	case ComponentType::FRUSTUM:
		//		showInspectorFrustum(static_cast<Frustum *>(e));
		//		break;
		//	case ComponentType::RENDERABLE:
		//	{
		//		showInspectorRenderable(static_cast<Renderable *>(e));
		//	}
		//	break;
		//	default:
		//		break;
		//	}
		//}
	}
	void showInspectorRenderable(Renderable *renderable)
	{
		ImGui::Text("MeshView: %s", renderable->getName().data());
		showInspectorMaterialDataBlock(renderable->getCurMaterialDataBlock());
	}
	void showInspectorMaterialDataBlock(MaterialDataBlock *materialDataBlock)
	{
		ImGui::Text(materialDataBlock->getMaterial()->getName().data());
		switch (materialDataBlock->getMaterialType())
		{
		case MaterialType::UNLIT_COLOR:
			showInspectorMaterialDataBlockUnlitColor(static_cast<MaterialDataBlockUnlitColor *>(materialDataBlock));
			break;
		}
	}
	void showInspectorMaterialDataBlockUnlitColor(MaterialDataBlockUnlitColor *unlitColorDataBlock)
	{
		static float color[4]{};
		ImGuiColorEditFlags misc_flags =
			ImGuiColorEditFlags_NoDragDrop |
			ImGuiColorEditFlags_AlphaPreviewHalf |
			ImGuiColorEditFlags_NoOptions |
			ImGuiColorEditFlags_AlphaBar;
		if (ImGui::ColorEdit4("color factor", color, ImGuiColorEditFlags_NoLabel | misc_flags))
		{
			unlitColorDataBlock->setColorFactor(color);
		}
	}
};
EXAMPLE_MAIN(Hello)
