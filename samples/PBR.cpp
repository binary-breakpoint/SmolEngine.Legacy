#include "PBR.h"

#include <GraphicsContext.h>
#include <MaterialLibrary.h>
#include <Renderer.h>

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <Common/Texture.h>
#include <Utils/Utils.h>

#include <imgui/imgui.h>

using namespace Frostium;

struct Chunk
{
	int32_t   MaterialID = 0;
	glm::vec3 Pos = glm::vec3(1.0f);
	glm::vec3 Rot = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
} chunk = {};

struct MaterialTextures
{
	Texture Albedro;
	Texture Normal;
	Texture Metallic;
	Texture Roughness;
	Texture AO;
};

std::vector<MaterialTextures> Materials;

void GenerateMap(std::vector<Chunk>& map, std::vector<int32_t>& materialIDs)
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
		windoInfo.Title = "Frostium Example";
	}

	EditorCameraCreateInfo cameraCI = {};
	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags;
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_MAX_SUPPORTED;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pEditorCameraCI = &cameraCI;
	}

	context = new GraphicsContext(&info);

	ClearInfo clearInfo = {};
	Mesh cube = {};
	Mesh::Create("Assets/cube.glb", &cube);
	bool process = true;

	context->SetEventCallback([&](Event& e) 
	{
		if(e.IsType(EventType::WINDOW_CLOSE))
			process = false;
	});

	Materials.resize(4);
	std::vector<int32_t> materialIDs;
	std::vector<Chunk> chunks;
	LoadMaterials(materialIDs);
	GenerateMap(chunks, materialIDs);

	static glm::vec3 lightDir = glm::vec3(0.0f, 53.0f, 2.0f);

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		if (Input::IsMouseButtonPressed(MouseCode::ButtonRight))
		{
			float rayDistance = 50.0f;
			float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			glm::vec3 startPos = context->GetEditorCamera()->GetPosition();
			glm::mat4 viewProj = context->GetEditorCamera()->GetViewProjection();

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

		context->BeginFrame(deltaTime);
		{
			uint32_t objects = 0;
			Renderer::BeginScene(&clearInfo);
			{
				Renderer::SetShadowLightDirection(lightDir);
				Renderer::SubmitDirectionalLight(lightDir, { 1, 1, 1, 1 });
				for (const auto& c : chunks)
					Renderer::SubmitMesh(c.Pos, c.Rot, c.Scale, &cube, c.MaterialID);

				objects = Renderer::GetNumObjects();
			}
			Renderer::EndScene();

			ImGui::Begin("Debug Window");
			{
				float lastFrameTime = deltaTime.GetTimeSeconds();
				std::string str = "DeltaTime: " + std::to_string(lastFrameTime);
				std::string str2 = "Objects: " + std::to_string(objects);

				ImGui::DragFloat3("LightDir", glm::value_ptr(lightDir));
				ImGui::Text(str.c_str());
				ImGui::Text(str2.c_str());
			}
			ImGui::End();
		}
		context->SwapBuffers();
	}
}

void LoadMaterials(std::vector<int32_t>& materialsIDs)
{
	MaterialLibrary* lib = MaterialLibrary::GetSinglenton();

	std::string albedroPath = "";
	std::string normalPath = "";
	std::string roughnessPath = "";
	std::string aoPath = "";
	std::string metalPath = "";

	MaterialCreateInfo materialCI = {};

	// Wood
	{
		albedroPath = "Assets/materials/wood/WoodFloor041_1K_Color.png"; 
		normalPath = "Assets/materials/wood/WoodFloor041_1K_Normal.png";
		roughnessPath = "Assets/materials/wood/WoodFloor041_1K_Roughness.png";
		aoPath = "Assets/materials/wood/WoodFloor041_1K_AmbientOcclusion.png";

		Texture::Create(albedroPath, &Materials[0].Albedro);
		Texture::Create(normalPath, &Materials[0].Normal);
		Texture::Create(roughnessPath, &Materials[0].Roughness);
		Texture::Create(aoPath, &Materials[0].AO);

		materialCI.SetTexture(MaterialTexture::Albedro, &Materials[0].Albedro, albedroPath);
		materialCI.SetTexture(MaterialTexture::Normal, &Materials[0].Normal, normalPath);
		materialCI.SetTexture(MaterialTexture::Roughness, &Materials[0].Roughness, roughnessPath);
		materialCI.SetTexture(MaterialTexture::AO, &Materials[0].AO, aoPath);
		materialCI.SetMetalness(0.2f);

		int32_t id = lib->Add(&materialCI, "Assets/materials/wood.mat");
		materialsIDs.push_back(id);
	}

	// Stone
	{
		materialCI = {};

		albedroPath = "Assets/materials/stone/Tiles087_1K_Color.png";
		normalPath = "Assets/materials/stone/Tiles087_1K_Normal.png";
		roughnessPath = "Assets/materials/stone/Tiles087_1K_Roughness.png";
		aoPath = "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png";

		Texture::Create(albedroPath, &Materials[1].Albedro);
		Texture::Create(normalPath, &Materials[1].Normal);
		Texture::Create(roughnessPath, &Materials[1].Roughness);
		Texture::Create(aoPath, &Materials[1].AO);

		materialCI.SetTexture(MaterialTexture::Albedro, &Materials[1].Albedro, albedroPath);
		materialCI.SetTexture(MaterialTexture::Normal, &Materials[1].Normal, normalPath);
		materialCI.SetTexture(MaterialTexture::Roughness, &Materials[1].Roughness, roughnessPath);
		materialCI.SetTexture(MaterialTexture::AO, &Materials[1].AO, aoPath);
		materialCI.SetMetalness(1.0f);

		int32_t id = lib->Add(&materialCI, "Assets/materials/stone.mat");
		materialsIDs.push_back(id);
	}

	// Metals 1
	{
		materialCI = {};

		albedroPath = "Assets/materials/metal_1/Metal033_1K_Color.png";
		normalPath = "Assets/materials/metal_1/Metal033_1K_Normal.png";
		roughnessPath = "Assets/materials/metal_1/Metal033_1K_Roughness.png";
		metalPath = "Assets/materials/metal_1/Metal033_1K_Metalness.png";

		Texture::Create(albedroPath, &Materials[2].Albedro);
		Texture::Create(normalPath, &Materials[2].Normal);
		Texture::Create(roughnessPath, &Materials[2].Roughness);
		Texture::Create(metalPath, &Materials[2].Metallic);

		materialCI.SetTexture(MaterialTexture::Albedro, &Materials[2].Albedro, albedroPath);
		materialCI.SetTexture(MaterialTexture::Normal, &Materials[2].Normal, normalPath);
		materialCI.SetTexture(MaterialTexture::Roughness, &Materials[2].Roughness, roughnessPath);
		materialCI.SetTexture(MaterialTexture::Metallic, &Materials[2].Metallic, metalPath);

		int32_t id = lib->Add(&materialCI, "Assets/materials/metal1.mat");
		materialsIDs.push_back(id);
	}

	// Metals 3
	{
		materialCI = {};

		albedroPath = "Assets/materials/metal_2/Metal012_1K_Color.png";
		normalPath = "Assets/materials/metal_2/Metal012_1K_Normal.png";
		roughnessPath = "Assets/materials/metal_2/Metal012_1K_Roughness.png";
		metalPath = "Assets/materials/metal_2/Metal012_1K_Metalness.png";

		Texture::Create(albedroPath, &Materials[3].Albedro);
		Texture::Create(normalPath, &Materials[3].Normal);
		Texture::Create(roughnessPath, &Materials[3].Roughness);
		Texture::Create(metalPath, &Materials[3].Metallic);

		materialCI.SetTexture(MaterialTexture::Albedro, &Materials[3].Albedro, albedroPath);
		materialCI.SetTexture(MaterialTexture::Normal, &Materials[3].Normal, normalPath);
		materialCI.SetTexture(MaterialTexture::Roughness, &Materials[3].Roughness, roughnessPath);
		materialCI.SetTexture(MaterialTexture::Metallic, &Materials[3].Metallic, metalPath);

		int32_t id = lib->Add(&materialCI, "Assets/materials/metal2.mat");
		materialsIDs.push_back(id);
	}

	// Don't forget update materials
	Renderer::UpdateMaterials();
}
