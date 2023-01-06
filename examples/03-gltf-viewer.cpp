#include "core/entry.hpp"
#include "core/myMesh.hpp"
#include "core/gltfLoader.hpp"

const char *test_gltf_models[] = {
	"models/plane.gltf",
	//"gltf2/",
	//"gltf2/Triangle/glTF/Triangle.gltf",
	// "gltf2/BoomBoxWithAxes/glTF/BoomBoxWithAxes.gltf",
	//"gltf2/Cube/glTF/Cube.gltf",
	//"gltf2/WaterBottle/glTF/WaterBottle.gltf",
	//"gltf2/SimpleMeshes/glTF/SimpleMeshes.gltf",
	"gltf2/DamagedHelmet/glTF/DamagedHelmet.gltf",
	// "gltf2/FlightHelmet/glTF/FlightHelmet.gltf",
	// "gltf2/BoxAnimated/glTF/BoxAnimated.gltf",//skeleton
	"gltf2/BrainStem/glTF/BrainStem.gltf", // skin
	"gltf2/CesiumMan/glTF/CesiumMan.gltf", // skin
										   //"gltf2/AnimatedMorphSphere/glTF/AnimatedMorphSphere.gltf", //morph
};
// const char* groundModelName="models/plane.gltf";
// const char *skyboxImageName = "images/Mt-Washington-Gold-Room_Ref.hdr";
const char *skyboxImageName = "images/GCanyon_C_YumaPoint_3k.hdr";

class Hello : public App
{
	std::vector<Image *> imagejpg;

	std::unique_ptr<MyModel> axisModelSrc;
	ModelResource *axisModelResource;

	std::vector<ModelResource *> testModelResources;
	std::vector<Model *> testModels;

	Scene *scene;

	void loadResources()
	{
		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();

		// create axis
		axisModelSrc = MyModelFactory::getSingleton().buildModel("axis", MyModelType::AXIS);

		auto p = axisModelSrc.get();
		axisModelResource = static_cast<ModelResource *>(grp->createResource(
			ResourceDeclaration{
				p->name,
				ResourceType::MODEL,
				&MyModelLoader::getSingleton()},
			sizeof(ptrdiff_t), &p));
		axisModelResource->load();

		//=====================================
		auto l = std::size(test_gltf_models);
		for (size_t i = 0; i < l; ++i)
			grp->declareResource(ResourceDeclaration{test_gltf_models[i], ResourceType::MODEL, &GLTFModelLoader::getSingleton()});
		grp->createDeclaredResources();
		grp->loadDeclaredResources();

		for (size_t i = 0; i < l; ++i)
			testModelResources.emplace_back(static_cast<ModelResource *>(grp->getResource(test_gltf_models[i])));
	}
	void createScene()
	{
		auto sceneManager = Root::getSingleton().getSceneManager();
		scene = sceneManager->createScene("testscene");

		// scene->createModel(axisModelResource)->scale(glm::vec3(20.f));

		// for (auto p : testModelResources)
		for (uint32_t i = 0; i < testModelResources.size(); ++i)
		{
			testModels.emplace_back(scene->createModel(testModelResources[i]));
		}

		float interval = 5;
		auto pos = glm::vec3(-interval * (std::size(test_gltf_models) - 1) / 2, 0, 0);
		// for (auto p : testModels)
		for (uint32_t i = 1; i < testModels.size(); ++i)
		{
			testModels[i]->translate(pos);
			pos += glm::vec3(interval, 0, 0);
		}
		scene->prepare();

		// scale models
		// for (auto p : testModels)
		for (uint32_t i = 1; i < testModels.size(); ++i)
		{
			auto b = testModels[i]->getBoundingVolume().box;
			if (b.extent == BoundingVolumeExtentType::FINITE)
			{
				auto s = b.getSize();
				testModels[i]->scale(glm::vec3(2.f / (std::max)((std::max)(s.x, s.y), s.z)));
			}
		}
		testModels[1]->translate({0, 2, 0});

		// add directional light
		auto light = scene->createLight("light");
		auto &&lightPara = light->getLightParaRef();
		lightPara.lightType = LIGHT_DIRECTIONAL;
		light->getParentNode()->translate({0, 10, 0})->pitch(glm::radians(-90.f), SceneNode::TransformSpace::LOCAL);

		// set skybox
		auto grp = ResourceGroupManager::getSingleton().createOrRetrieveResourceGroup();

		auto skyboxImage = static_cast<Image *>(grp->createOrRetrieveResource(ResourceDeclaration{skyboxImageName, ResourceType::IMAGE}));
		auto texMgr = Root::getSingleton().getTextureManager();
		auto skyboxEqTex = texMgr->create(
			Shit::ImageType::TYPE_2D,
			Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
			{&skyboxImage, 1});
		scene->getEnvironment()->setSkyboxEqTex(skyboxEqTex);
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
			showInspectorSceneNode(*selectedNodes.rbegin());
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
		for (auto e : components)
		{
			if (auto p = dynamic_cast<Animation *>(e))
			{
				showAnimation(p);
			}
			// 	switch (e->getComponentType())
			// 	{
			// 	case ComponentType::FRUSTUM:
			// 		showInspectorFrustum(static_cast<Frustum *>(e));
			// 		break;
			// 	case ComponentType::RENDERABLE:
			// 		showInspectorRenderable(static_cast<Renderable *>(e));
			// 		break;
			// 	default:
			// 		break;
			// 	}
		}
	}
	void showAnimation(Animation *animation)
	{
		ImGui::Text("Animation: %s", animation->getName().data());

		auto count = std::distance(animation->clipViewCbegin(), animation->clipViewCend());

		static int clipIndex = 0;
		if (ImGui::BeginCombo("input attachment", animation->getCurClipView()->getClip()->getName().data()))
		{
			for (int i = 0; i < count; ++i)
			{
				const bool is_selected = clipIndex == i;
				// auto clipView = std::advance(animation->clipViewBegin(), i);
				auto a = animation->clipViewBegin();
				std::advance(a, i);
				if (ImGui::Selectable(a->first.data(), is_selected))
				{
					animation->setCurClipView(a->second.get());
				}
			}
			ImGui::EndCombo();
		}

		static float curTime;
		curTime = float(animation->getTimer().getElapsedTimeInMs()) / 1000;
		if (ImGui::SliderFloat("time", &curTime, 0, animation->getPeriod()))
		{
			animation->setTime(curTime);
		}

		static bool pause;
		pause = !animation->isRunning();
		if (ImGui::Checkbox("pause", &pause))
		{
			if (pause)
				animation->pause();
			else
				animation->play();
		}
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
			showInspectorMaterialUnlitColorDataBlock(static_cast<MaterialDataBlockUnlitColor *>(materialDataBlock));
			break;
		}
	}
	void showInspectorMaterialUnlitColorDataBlock(MaterialDataBlockUnlitColor *unlitColorDataBlock)
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
