#include "stdafx.h"
#include "Panels/MaterialPanel.h"
#include "ImGuiExtension.h"
#include "Editor-Tools/Tools.h"
#include "Primitives/GraphicsPipeline.h"
#include "Renderer/PBRLoader.h"
#include "GraphicsCore.h"
#include "Tools/Utils.h"

#include <imgui/imgui.h>

namespace SmolEngine
{
	MaterialPanel::MaterialPanel()
	{
		m_TexturesLoader = TexturesLoader::Get();
		m_MaterialCI = new PBRCreateInfo();
		m_Data = new PreviewRenderingData();
		s_Instance = this;
		InitPreviewRenderer();
	}

	MaterialPanel::~MaterialPanel()
	{

	}

	void MaterialPanel::OpenExisting(const std::string& path)
	{
		std::filesystem::path p(path);
		m_CurrentName = p.filename().stem().u8string();
		m_CurrentFilePath = path;

		m_MaterialCI->Load(path);
		RenderImage();
	}

	void MaterialPanel::OpenNew(const std::string& path)
	{
		std::filesystem::path p(path);
		m_CurrentName = p.filename().stem().u8string();
		m_CurrentFilePath = path;

		RenderImage();
		Save();
	}

	void MaterialPanel::Close()
	{
		m_CurrentFilePath = "";
	}

	void MaterialPanel::Update()
	{
		ImGui::NewLine();
		ImGui::BeginChild("MaterialViewerWIndow");

		{
			ImGui::SetCursorPosX(12);
			ImGui::TextWrapped("Preview: %s", m_CurrentName.c_str());
			ImGui::Separator();
			ImGui::SetCursorPosX(12);
			ImGui::Image(m_Data->Framebuffer->GetImGuiTextureID(), { 440, 280 });
			if (ImGui::IsItemHovered())
			{
				float my_tex_h = 280;
				float my_tex_w = 440;
				ImVec2 pos = ImGui::GetCursorScreenPos();
				auto& io = ImGui::GetIO();
				ImGui::BeginTooltip();
				float region_sz = 32.0f;
				float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
				float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
				float zoom = 5.0f;
				ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
				ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
				ImGui::Image(m_Data->Framebuffer->GetImGuiTextureID(), ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1);
				ImGui::EndTooltip();
			}
		}

		if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::NewLine();

			if (DrawTextureInfo(m_MaterialCI->AlbedroTex, "Albedro", PBRTexture::Albedo))
				ApplyChanges();

			if (DrawTextureInfo(m_MaterialCI->NormalTex, "Normal", PBRTexture::Normal))
				ApplyChanges();

			if (DrawTextureInfo(m_MaterialCI->MetallnessTex, "Metalness", PBRTexture::Metallic))
				ApplyChanges();

			if (DrawTextureInfo(m_MaterialCI->RoughnessTex, "Roughness", PBRTexture::Roughness))
				ApplyChanges();

			if (DrawTextureInfo(m_MaterialCI->AOTex, "AO", PBRTexture::AO))
				ApplyChanges();

			if (DrawTextureInfo(m_MaterialCI->EmissiveTex, "Emissive", PBRTexture::Emissive))
				ApplyChanges();

			ImGui::NewLine();
		}
		if (ImGui::CollapsingHeader("Attributes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::NewLine();

			if (ImGui::Extensions::InputFloat("Roughness", m_MaterialCI->Roughness))
				ApplyChanges();
			if (ImGui::Extensions::InputFloat("Metalness", m_MaterialCI->Metallness))
				ApplyChanges();
			if (ImGui::Extensions::InputFloat("Emission Strength", m_MaterialCI->EmissionStrength))
				ApplyChanges();
			if (ImGui::Extensions::ColorInput3("Albedro", m_MaterialCI->Albedo))
				ApplyChanges();

			ImGui::NewLine();
		}

		ImGui::EndChild();
	}

	void MaterialPanel::Save()
	{
		if (!m_CurrentFilePath.empty())
		{
			std::string path = m_CurrentFilePath;
			m_MaterialCI->Save(path);
		}
	}

	std::string MaterialPanel::GetCurrentPath() const
	{
		return m_CurrentFilePath;
	}

	MaterialPanel* MaterialPanel::GetSingleton()
	{
		return s_Instance;
	}

	void MaterialPanel::ApplyChanges()
	{
		RenderImage();
		Save();
	}

	void MaterialPanel::RenderImage()
	{

		Ref<Texture>& whiteTex = TexturePool::GetWhiteTexture();
		Ref<PBRLoader> pbrLoader = RendererStorage::GetPBRLoader();
		m_Data->Pipeline->UpdateTexture(whiteTex, 5);

		m_Data->Pipeline->UpdateVkDescriptor(2, pbrLoader->GetIrradianceDesriptor());
		m_Data->Pipeline->UpdateVkDescriptor(3, pbrLoader->GetBRDFLUTDesriptor());
		m_Data->Pipeline->UpdateVkDescriptor(4, pbrLoader->GetPrefilteredCubeDesriptor());


		m_Data->Pipeline->UpdateTexture(m_PBRHandle->m_Albedo != nullptr ? m_PBRHandle->m_Albedo 
			: TexturePool::GetWhiteTexture(), 5);

		m_Data->Pipeline->UpdateTexture(m_PBRHandle->m_Normal != nullptr ? m_PBRHandle->m_Normal
			: TexturePool::GetWhiteTexture(), 6);

		m_Data->Pipeline->UpdateTexture(m_PBRHandle->m_Roughness != nullptr ? m_PBRHandle->m_Roughness
			: TexturePool::GetWhiteTexture(), 7);

		m_Data->Pipeline->UpdateTexture(m_PBRHandle->m_Metallness != nullptr ? m_PBRHandle->m_Metallness
			: TexturePool::GetWhiteTexture(), 8);

		m_Data->Pipeline->UpdateTexture(m_PBRHandle->m_AO != nullptr ? m_PBRHandle->m_AO
			: TexturePool::GetWhiteTexture(), 9);

		m_Data->Pipeline->UpdateBuffer(m_BindingPoint, sizeof(PBRUniform), &m_PBRHandle->m_Uniform);
		m_Data->Pipeline->BeginCommandBuffer();
		m_Data->Pipeline->BeginRenderPass();
		{
			Ref<Mesh> mesh = nullptr;
			switch (m_GeometryType)
			{
			case 0: mesh = MeshPool::GetCube().first; break; 
			case 1: mesh = MeshPool::GetSphere().first; break;
			case 2: mesh = MeshPool::GetTorus().first; break;
			}

			struct pc
			{
				glm::mat4 viewProj;
				glm::vec3 camPos;
			} puch_constant;

			puch_constant.viewProj = m_Data->Camera->GetProjection() * m_Data->Camera->GetViewMatrix();
			puch_constant.camPos = m_Data->Camera->GetPosition();

			m_Data->Pipeline->SubmitPushConstant(ShaderType::Vertex, sizeof(pc), &puch_constant);
			m_Data->Pipeline->DrawMeshIndexed(mesh);
		}
		m_Data->Pipeline->EndRenderPass();
		m_Data->Pipeline->EndCommandBuffer();
	}

	void MaterialPanel::InitPreviewRenderer()
	{
		// Editor Camera
		{
			EditorCameraCreateInfo info{};
			info.FOV = 35;
			info.WorldPos = glm::vec3(0, 0, -2.0);
			m_Data->Camera = std::make_shared<EditorCamera>(&info);
		}

		// Framebuffer
		{
			Ref<Framebuffer> fb = Framebuffer::Create();

			FramebufferSpecification framebufferCI = {};
			framebufferCI.Width = 1024;
			framebufferCI.Height = 1024;
			framebufferCI.bResizable = false;
			framebufferCI.bUsedByImGui = true;
			framebufferCI.bAutoSync = false;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color, true) };
			framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;

			fb->Build(&framebufferCI);

			m_Data->Framebuffer = fb;
		}

		// Pipeline
		{
			Ref<GraphicsPipeline> pipeline = GraphicsPipeline::Create();

			BufferLayout mainLayout =
			{
				{ DataTypes::Float3, "aPos" },
				{ DataTypes::Float3, "aNormal" },
				{ DataTypes::Float3, "aTangent" },
				{ DataTypes::Float2, "aUV" },
				{ DataTypes::Int4,   "aBoneIDs"},
				{ DataTypes::Float4, "aWeight"}
			};

			VertexInputInfo vertexMain(sizeof(PBRVertex), mainLayout);

			ShaderCreateInfo shaderCI = {};
			{
				shaderCI.Stages[ShaderType::Vertex] = "assets/shaders/PBR_Preview.vert";
				shaderCI.Stages[ShaderType::Fragment] = "assets/shaders/PBR_Preview.frag";
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { vertexMain };
				DynamicPipelineCI.PipelineName = "PBR_Preview";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.TargetFramebuffers = { m_Data->Framebuffer };
			}

			pipeline->Build(&DynamicPipelineCI);

			m_Data->Pipeline = pipeline;
		}
	}

	bool MaterialPanel::DrawTextureInfo(TextureCreateInfo& texInfo, const std::string& title, PBRTexture type)
	{
		bool used = false;
		std::string id = title + std::string("draw_info");
		ImGui::PushID(id.c_str());

		float posX = 12.0f;
		ImGui::SetCursorPosX(posX);

		Ref<Texture> icon = nullptr;
		switch (type)
		{
		case PBRTexture::Albedo: icon = m_PBRHandle->m_Albedo; break;
		case PBRTexture::Normal: icon = m_PBRHandle->m_Normal; break;
		case PBRTexture::Roughness: icon = m_PBRHandle->m_Roughness; break;
		case PBRTexture::Metallic: icon = m_PBRHandle->m_Metallness; break;
		case PBRTexture::Emissive: icon = m_PBRHandle->m_Emissive; break;
		case PBRTexture::AO: icon = m_PBRHandle->m_AO; break;
		}

		if (icon != nullptr)
		{
			ImGui::Image(icon->GetImGuiTexture(), { 60, 60 });
			if (ImGui::IsItemHovered())
			{
				float my_tex_h = 60;
				float my_tex_w = 60;
				ImVec2 pos = ImGui::GetCursorScreenPos();
				auto& io = ImGui::GetIO();
				ImGui::BeginTooltip();
				float region_sz = 32.0f;
				float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
				float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
				float zoom = 5.0f;
				ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
				ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
				ImGui::Image(icon->GetImGuiTexture(), ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1);
				ImGui::EndTooltip();
			}
		}
	

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
			{
				std::string& path = *(std::string*)payload->Data;
				if (Tools::FileExtensionCheck(path, ".s_image"))
				{
					texInfo.Load(path);

					if (icon == nullptr)
					{
						used = true;
						texInfo.bImGUIHandle = true;

						switch (type)
						{
						case PBRTexture::Albedo: 
						{
							m_PBRHandle->m_Albedo = Texture::Create();
							m_PBRHandle->m_Albedo->LoadFromFile(&texInfo);
							break;
						};
						case PBRTexture::Normal: 
						{
							m_PBRHandle->m_Normal = Texture::Create();
							m_PBRHandle->m_Normal->LoadFromFile(&texInfo);
							break;
						};
						case PBRTexture::Roughness: 
						{
							m_PBRHandle->m_Roughness = Texture::Create();
							m_PBRHandle->m_Roughness->LoadFromFile(&texInfo);
							break;
						}
						case PBRTexture::Metallic: 
						{
							m_PBRHandle->m_Metallness = Texture::Create();
							m_PBRHandle->m_Metallness->LoadFromFile(&texInfo);
							break;
						}
						case PBRTexture::Emissive: 
						{
							m_PBRHandle->m_Emissive = Texture::Create();
							m_PBRHandle->m_Emissive->LoadFromFile(&texInfo);
							break;
						}
						case PBRTexture::AO: 
						{
							m_PBRHandle->m_AO = Texture::Create();
							m_PBRHandle->m_AO->LoadFromFile(&texInfo);
							break;
						}
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			texInfo = {};

			switch (type)
			{
			case PBRTexture::Albedo:    m_PBRHandle->m_Albedo = nullptr; break;
			case PBRTexture::Normal:    m_PBRHandle->m_Normal = nullptr; break;
			case PBRTexture::Roughness: m_PBRHandle->m_Roughness = nullptr; break;
			case PBRTexture::Metallic:  m_PBRHandle->m_Metallness = nullptr; break;
			case PBRTexture::Emissive:  m_PBRHandle->m_Emissive = nullptr; break;
			case PBRTexture::AO:        m_PBRHandle->m_AO = nullptr; break;
			}

			used = true; 
		}

		ImGui::SameLine();
		if (texInfo.FilePath.empty())
		{
			std::string name = title + ": None";
			ImGui::TextUnformatted(name.c_str());
		}
		else
		{
			std::filesystem::path p(texInfo.FilePath);
			std::string name = title + ": " + p.filename().filename().u8string();
			ImGui::TextUnformatted(name.c_str());
		}

		ImGui::PopID();
		return used;
	}
}