#include "PBR.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

GraphicsContext*   context = nullptr;
RendererDrawList*  drawList = nullptr;
RendererStorage*   storage = nullptr;

struct Chunk
{
	Ref<Material3D::Info>       Material = nullptr;
	glm::vec3                   Pos = glm::vec3(1.0f);
	glm::vec3                   Rot = glm::vec3(0.0f);
	glm::vec3                   Scale = glm::vec3(1.0f);

} chunk;

void GenerateMap(std::vector<Chunk>& map, std::vector<Ref<Material3D::Info>>& materials)
{
	for (uint32_t x = 0; x < 50; x += 2)
	{
		for (uint32_t z = 0; z < 50; z += 2)
		{
			float scale_y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			chunk.Pos = { x, 0, z };
			chunk.Rot = { 0, 0, 0 };
			chunk.Scale = { 0.5, scale_y, 0.5 };

			uint32_t rIndex = rand() % materials.size();
			chunk.Material = materials[rIndex];

			map.emplace_back(chunk);
		}
	}
}

void CreateDrawList(SceneViewProjection* viewProj, std::vector<Chunk>& chunks, Ref<Mesh>& cube)
{
	drawList->GetFrustum().SetRadius(1000.0f);

	drawList->BeginSubmit(viewProj);
	{
		for (const auto& c : chunks)
		{
			drawList->SubmitMesh(c.Pos, c.Rot, c.Scale, cube, c.Material);
		}
	}
	drawList->EndSubmit();

	chunks.clear();
}

void LoadMaterials(std::vector<Ref<Material3D::Info>>& materials)
{
	std::string albedroPath = "";
	std::string normalPath = "";
	std::string roughnessPath = "";
	std::string aoPath = "";
	std::string metalPath = "";

	Ref<Material3D> defaultMaterial = storage->GetDefaultMaterial();
	Material3D::CreateInfo materialCI = {};
	TextureCreateInfo textureCI = {};

	// Wood
	{
		materialCI.Metallness = 0.2f;

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Color.png";
		materialCI.SetTexture(Material3D::TextureType::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Normal.png";
		materialCI.SetTexture(Material3D::TextureType::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Roughness.png";
		materialCI.SetTexture(Material3D::TextureType::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_AmbientOcclusion.png";
		materialCI.SetTexture(Material3D::TextureType::AO, &textureCI);

		auto material = defaultMaterial->AddMaterial(&materialCI, "wood");
		materials.push_back(material);
	}

	// Stone
	{
		materialCI = {};
		materialCI.Roughness = 1.0f;

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Color.png";
		materialCI.SetTexture(Material3D::TextureType::Albedo, &textureCI);

		textureCI.FilePath = normalPath = "Assets/materials/stone/Tiles087_1K_Normal.png";
		materialCI.SetTexture(Material3D::TextureType::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Roughness.png";
		materialCI.SetTexture(Material3D::TextureType::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png";
		materialCI.SetTexture(Material3D::TextureType::AO, &textureCI);

		auto material = defaultMaterial->AddMaterial(&materialCI, "stone");
		materials.push_back(material);
	}

	// Metals 1
	{
		materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Color.png";
		materialCI.SetTexture(Material3D::TextureType::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Normal.png";
		materialCI.SetTexture(Material3D::TextureType::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Roughness.png";
		materialCI.SetTexture(Material3D::TextureType::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Metalness.png";
		materialCI.SetTexture(Material3D::TextureType::Metallic, &textureCI);

		auto material = defaultMaterial->AddMaterial(&materialCI, "metal1");
		materials.push_back(material);
	}

	// Metals 2
	{
		materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Color.png";
		materialCI.SetTexture(Material3D::TextureType::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Normal.png";
		materialCI.SetTexture(Material3D::TextureType::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Roughness.png";
		materialCI.SetTexture(Material3D::TextureType::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Metalness.png";
		materialCI.SetTexture(Material3D::TextureType::Metallic, &textureCI);

		auto material = defaultMaterial->AddMaterial(&materialCI, "metal2");
		materials.push_back(material);
	}

	// Don't forget update materials
	storage->OnUpdateMaterials();
}

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Frostium PBR";
	}

	Camera* camera = nullptr;
	{
		EditorCameraCreateInfo cameraCI = {};
		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
	}

	bool process = true;
	ClearInfo clearInfo = {};

	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) { process = false; }  camera->OnEvent(e); });

	storage = new RendererStorage();
	storage->Build();

	drawList = new RendererDrawList();

	auto cube = MeshPool::GetCube();

	std::vector<Ref<Material3D::Info>> materials;
	std::vector<Chunk> chunks;

	LoadMaterials(materials);
	GenerateMap(chunks, materials);

	RendererStateEX& state = storage->GetState();
	state.Bloom.Threshold = 0.7f;
	state.Bloom.Enabled = true;
	state.bDrawGrid = false;

	DynamicSkyProperties sky;
	storage->SetDynamicSkybox(sky, camera->GetProjection(), true);

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.IsCastShadows = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	drawList->SubmitDirLight(&dirLight);

	SceneViewProjection viewProj = SceneViewProjection(camera);
	CreateDrawList(&viewProj, chunks, cube);

	UICanvas canvas;
	canvas.Rect.x = 0;
	canvas.Rect.y = 0;
	canvas.Rect.z = 720;
	canvas.Rect.w = 480;

	UIButton button;
	button.SetLayout(100, 50, 1);
	button.SetOffset(500, 0);

	UIText text;
	text.SetFont("../resources/Fonts/Font1.ttf", 44.0f);
	text.SetLayout(600, 200, 1);
	text.SetAlignment(AlignmentFlags::MidCentered);
	text.Text = "Some text!";

	UITextField textField;
	textField.SetLayout(120, 30, 1);
	textField.Label = "Player name:";

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/* 
		    Calculates physics, update scripts, etc.
		*/

		camera->OnUpdate(deltaTime);
		viewProj.Update(camera);

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("PBR Sample");
			{
				ImGui::Text("Some Text");

				ImGui::DragFloat("Bloom", &storage->GetState().Bloom.Threshold);
			}
			ImGui::End();

			canvas.Draw([&]() 
			{
				button.Draw();
				text.Draw();
				textField.Draw();
			});

			RendererDeferred::DrawFrame(&clearInfo, storage, drawList);
		}
		context->SwapBuffers();
	}
}
