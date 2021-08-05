#include "PBR.h"

#include <FrostiumCore.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif

struct Chunk
{
	uint32_t   MaterialID = 0;
	glm::vec3 Pos = glm::vec3(1.0f);
	glm::vec3 Rot = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
} chunk = {};

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

int main(int argc, char** argv)
{
	GraphicsContext* context = nullptr;
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
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags;
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
	}

	context = new GraphicsContext(&info);

	ClearInfo clearInfo = {};
	auto cube = context->GetDefaultMeshes()->Cube;
	bool process = true;

	context->SetEventCallback([&](Event& e) 
	{
		if(e.IsType(EventType::WINDOW_CLOSE))
			process = false;
	});

	std::vector<uint32_t> materialIDs;
	std::vector<Chunk> chunks;
	LoadMaterials(materialIDs);
	GenerateMap(chunks, materialIDs);


	RendererStateEX& state = DeferredRenderer::GetState();
	state.Bloom.Strength = 0.2f;
	state.bBloom = true;
	state.bDrawGrid = false;

	DynamicSkyProperties sky;
	DeferredRenderer::SetDynamicSkyboxProperties(sky);

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	DeferredRenderer::SubmitDirLight(&dirLight);

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		if (Input::IsMouseButtonPressed(MouseCode::ButtonRight))
		{
			float rayDistance = 50.0f;
			float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			glm::vec3 startPos = context->GetDefaultCamera()->GetPosition();
			glm::mat4 viewProj = context->GetDefaultCamera()->GetViewProjection();

			{
				glm::vec3 pos = Utils::CastRay(startPos, rayDistance, viewProj);

				chunk.Pos = pos;
				chunk.Rot = { 0, 0, 0 };
				chunk.Scale = { 0.5, y, 0.5 };
				chunks.emplace_back(chunk);
			}
		}

		/* 
		   @Calculate physics, process script, etc
		*/


		// FarClip, NearClip, Camera Position, Projection and View Matrix
		BeginSceneInfo info = {};
		info.Update(camera);

		context->UpdateSceneInfo(&info);

		context->BeginFrame(deltaTime);
		{
			uint32_t objects = 0;
			DeferredRenderer::BeginScene(&clearInfo);
			{
				for (const auto& c : chunks)
					DeferredRenderer::SubmitMesh(c.Pos, c.Rot, c.Scale, cube.get(), c.MaterialID);

				objects = DeferredRenderer::GetNumObjects();
			}
			DeferredRenderer::EndScene();

			ImGui::Begin("PBR Sample");
			{
				std::string str = "DeltaTime: " + std::to_string(deltaTime);
				std::string str2 = "Objects: " + std::to_string(objects);

				if (ImGui::DragFloat3("LightDir", glm::value_ptr(dirLight.Direction)))
					DeferredRenderer::SubmitDirLight(&dirLight);

				ImGui::Text(str.c_str());
				ImGui::Text(str2.c_str());
			}
			ImGui::End();
		}
		context->SwapBuffers();

	}
}

void LoadMaterials(std::vector<uint32_t>& materialsIDs)
{
	MaterialLibrary* lib = MaterialLibrary::GetSinglenton();

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
		materialCI.SetTexture(MaterialTexture::Albedro, &textureCI);

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
		materialCI.SetTexture(MaterialTexture::Albedro, &textureCI);

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
		materialCI.SetTexture(MaterialTexture::Albedro, &textureCI);

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
		materialCI.SetTexture(MaterialTexture::Albedro, &textureCI);

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
	DeferredRenderer::UpdateMaterials();
}
