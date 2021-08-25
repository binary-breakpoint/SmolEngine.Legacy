#include "PBR.h"

#include <FrostiumCore.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif

GraphicsContext*   context = nullptr;
RendererDrawList*  drawList = nullptr;
RendererStorage*   storage = nullptr;

struct Chunk
{
	uint32_t       MaterialID = 0;
	glm::vec3      Pos = glm::vec3(1.0f);
	glm::vec3      Rot = glm::vec3(0.0f);
	glm::vec3      Scale = glm::vec3(1.0f);

} chunk;

void GenerateMap(std::vector<Chunk>& map, std::vector<uint32_t>& materialIDs)
{
	for (uint32_t x = 0; x < 50; x += 2)
	{
		for (uint32_t z = 0; z < 50; z += 2)
		{
			float scale_y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			chunk.Pos = { x, 0, z };
			chunk.Rot = { 0, 0, 0 };
			chunk.Scale = { 0.5, scale_y, 0.5 };
			chunk.MaterialID = rand() % materialIDs.size();

			map.emplace_back(chunk);
		}
	}
}

void CreateDrawList(SceneViewProjection* viewProj, std::vector<Chunk>& chunks, Mesh* cube)
{
	drawList->GetFrustum().SetRadius(1000.0f);

	drawList->BeginSubmit(viewProj);
	{
		for (const auto& c : chunks)
		{
			drawList->SubmitMesh(c.Pos, c.Rot, c.Scale, cube, c.MaterialID);
		}
	}
	drawList->EndSubmit();

	chunks.clear();
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
	storage->Initilize();
	context->PushStorage(storage);

	drawList = new RendererDrawList();

	auto cube = context->GetDefaultMeshes()->Cube;

	std::vector<uint32_t> materialIDs;
	std::vector<Chunk> chunks;
	LoadMaterials(materialIDs);
	GenerateMap(chunks, materialIDs);

	RendererStateEX& state = storage->GetState();
	state.Bloom.Strength = 0.1f;
	state.Bloom.Enabled = true;
	state.bDrawGrid = false;

	DynamicSkyProperties sky;
	storage->SetDynamicSkybox(sky, camera->GetProjection(), true);

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	drawList->SubmitDirLight(&dirLight);

	SceneViewProjection viewProj = SceneViewProjection(camera);
	CreateDrawList(&viewProj, chunks, cube.get());

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
			}
			ImGui::End();

			RendererDeferred::DrawFrame(&clearInfo, storage, drawList);
		}
		context->SwapBuffers();
	}
}

void LoadMaterials(std::vector<uint32_t>& materialsIDs)
{
	MaterialLibrary* lib = &storage->GetMaterialLibrary();

	std::string albedroPath = "";
	std::string normalPath = "";
	std::string roughnessPath = "";
	std::string aoPath = "";
	std::string metalPath = "";

	MaterialCreateInfo materialCI = {};
	TextureCreateInfo textureCI = {};

	// Wood
	{
		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Color.png";
		materialCI.SetTexture(MaterialTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Normal.png";
		materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_Roughness.png";
		materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/wood/WoodFloor041_1K_AmbientOcclusion.png";
		materialCI.SetTexture(MaterialTexture::AO, &textureCI);

		materialCI.SetMetalness(0.2f);
		uint32_t id = lib->Add(&materialCI, "wood");
		materialsIDs.push_back(id);
	}

	// Stone
	{
		materialCI = {};

		textureCI.FilePath  = "Assets/materials/stone/Tiles087_1K_Color.png";
		materialCI.SetTexture(MaterialTexture::Albedo, &textureCI);

		textureCI.FilePath = normalPath = "Assets/materials/stone/Tiles087_1K_Normal.png";
		materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Roughness.png";
		materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png";
		materialCI.SetTexture(MaterialTexture::AO, &textureCI);

		materialCI.SetMetalness(1.0f);
		uint32_t id = lib->Add(&materialCI, "stone");
		materialsIDs.push_back(id);
	}

	// Metals 1
	{
		materialCI = {};

		textureCI.FilePath  = "Assets/materials/metal_1/Metal033_1K_Color.png";
		materialCI.SetTexture(MaterialTexture::Albedo, &textureCI);

		textureCI.FilePath =  "Assets/materials/metal_1/Metal033_1K_Normal.png";
		materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Roughness.png";
		materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Metalness.png";
		materialCI.SetTexture(MaterialTexture::Metallic, &textureCI);

		uint32_t id = lib->Add(&materialCI, "metal1");
		materialsIDs.push_back(id);
	}

	// Metals 2
	{
		materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Color.png";
		materialCI.SetTexture(MaterialTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Normal.png";
		materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Roughness.png";
		materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Metalness.png";
		materialCI.SetTexture(MaterialTexture::Metallic, &textureCI);

		uint32_t id = lib->Add(&materialCI, "metal2");
		materialsIDs.push_back(id);
	}

	// Don't forget update materials
	storage->OnUpdateMaterials();
}
