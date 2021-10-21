#include "PBR.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

Ref<GraphicsContext> context = nullptr;

struct Chunk
{
	Ref<MeshView> View = nullptr;
	glm::vec3 Pos = glm::vec3(1.0f);
	glm::vec3 Rot = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);

} chunk;

void GenerateMap(std::vector<Chunk>& map, std::vector<Ref<PBRHandle>>& materials, Ref<Mesh>& mesh)
{
	for (uint32_t x = 0; x < 50; x += 2)
	{
		for (uint32_t z = 0; z < 50; z += 2)
		{
			float scale_y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			chunk.Pos = { x, 0, z };
			chunk.Rot = { 0, 0, 0 };
			chunk.Scale = { 0.5, scale_y, 0.5 };
			chunk.View = mesh->CreateMeshView();

			uint32_t rIndex = rand() % materials.size();
			chunk.View->SetPBRHandle(materials[rIndex]);

			map.emplace_back(chunk);
		}
	}
}

void LoadMaterials(std::vector<Ref<PBRHandle>>& materials)
{
	std::string albedroPath = "";
	std::string normalPath = "";
	std::string roughnessPath = "";
	std::string aoPath = "";
	std::string metalPath = "";

	PBRCreateInfo materialCI = {};
	TextureCreateInfo textureCI = {};
	Ref<MaterialPBR> defaultMaterial = RendererStorage::GetDefaultMaterial();

	// Wood
	{
		materialCI.Metallness = 0.2f;

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Color.png";
		materialCI.SetTexture(PBRTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Normal.png";
		materialCI.SetTexture(PBRTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Roughness.png";
		materialCI.SetTexture(PBRTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_AmbientOcclusion.png";
		materialCI.SetTexture(PBRTexture::AO, &textureCI);

		auto material = PBRFactory::AddMaterial(&materialCI, "wood");
		materials.push_back(material);
	}

	// Stone
	{
		materialCI = {};
		materialCI.Roughness = 1.0f;

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Color.png";
		materialCI.SetTexture(PBRTexture::Albedo, &textureCI);

		textureCI.FilePath = normalPath = "Assets/materials/stone/Tiles087_1K_Normal.png";
		materialCI.SetTexture(PBRTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Roughness.png";
		materialCI.SetTexture(PBRTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png";
		materialCI.SetTexture(PBRTexture::AO, &textureCI);

		auto material = PBRFactory::AddMaterial(&materialCI, "stone");
		materials.push_back(material);
	}

	// Metals 1
	{
		materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Color.png";
		materialCI.SetTexture(PBRTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Normal.png";
		materialCI.SetTexture(PBRTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Roughness.png";
		materialCI.SetTexture(PBRTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Metalness.png";
		materialCI.SetTexture(PBRTexture::Metallic, &textureCI);

		auto material = PBRFactory::AddMaterial(&materialCI, "metal1");
		materials.push_back(material);
	}

	// Metals 2
	{
		materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Color.png";
		materialCI.SetTexture(PBRTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Normal.png";
		materialCI.SetTexture(PBRTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Roughness.png";
		materialCI.SetTexture(PBRTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Metalness.png";
		materialCI.SetTexture(PBRTexture::Metallic, &textureCI);

		auto material = PBRFactory::AddMaterial(&materialCI, "metal2");
		materials.push_back(material);
	}

	// Don't forget update materials
	PBRFactory::UpdateMaterials();
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

	GraphicsContextCreateInfo info = {};
	{
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolder = "../resources/";
		info.pWindowCI = &windoInfo;
	}

	ClearInfo clearInfo = {};
	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });

	context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e) 
	{ 
		camera->OnEvent(e); 
	});

	auto& [cube, view] = MeshPool::GetCube();

	std::vector<Ref<PBRHandle>> materials;
	std::vector<Chunk> chunks;

	LoadMaterials(materials);
	GenerateMap(chunks, materials, cube);

	RendererStateEX& state = RendererStorage::GetState();
	state.Bloom.Threshold = 0.7f;
	state.Bloom.Enabled = true;
	state.bDrawGrid = false;

	DynamicSkyProperties sky;
	RendererStorage::SetDynamicSkybox(sky, camera->GetProjection(), true);

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.IsCastShadows = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	RendererDrawList::SubmitDirLight(&dirLight);

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

	while (context->IsOpen())
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/* 
		    Calculates physics, update scripts, etc.
		*/

		camera->OnUpdate(deltaTime);

		RendererDrawList::BeginSubmit(camera->GetSceneViewProjection());
		{
			RendererDrawList::SetVCTMesh(cube);
			RendererDrawList::SubmitMesh({ 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, cube, chunks[0].View);
		}
		RendererDrawList::EndSubmit();

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("PBR Sample");
			{
				ImGui::Text("Some Text");

				ImGui::DragFloat("Bloom", &RendererStorage::GetState().Bloom.Threshold);
			}
			ImGui::End();

			canvas.Draw([&]() 
			{
				button.Draw();
				text.Draw();
				textField.Draw();
			});

			RendererDeferred::DrawFrame(&clearInfo);
		}
		context->SwapBuffers();
	}
}
