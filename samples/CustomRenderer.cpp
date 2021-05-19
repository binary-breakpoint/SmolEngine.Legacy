#include "CustomRenderer.h"

#include <Common/Input.h>
#include <GraphicsContext.h>
#include <MaterialLibrary.h>
#include <Utils/Utils.h>
#include <ImGUI/ImGuiExtension.h>

#include <Vulkan/VulkanPBR.h>

using namespace Frostium;

#define FREE_CAMERA

int main(int argc, char** argv)
{
	CustomRenderer renderer = {};
	renderer.Init();
}

void CustomRenderer::Init()
{
	EditorCameraCreateInfo cameraCI = {};
	m_Camera = new EditorCamera(&cameraCI);

	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Frostium Example";
	}

	GraphicsContextInitInfo info = {};
	info.Flags = {};
	info.pWindowCI = &windoInfo;

	m_Context = new GraphicsContext(&info);
	m_Context->SetEventCallback([&](Event& e)
	{
		if (e.IsType(EventType::WINDOW_CLOSE))
		{
			m_Process = false;
		}

		if (e.IsType(EventType::WINDOW_RESIZE))
		{
			auto resize = e.Cast<WindowResizeEvent>();
			uint32_t width = resize->GetWidth();
			uint32_t height = resize->GetHeight();

			OnResize(width, height);
		}

#ifdef FREE_CAMERA
		m_Camera->OnEvent(e);
#endif
	});

	m_MaterialLibrary = new MaterialLibrary();

	BuildFramebuffers();
	BuildMaterials();
	BuildObjects();
	BuildPipelines();

	// Updates shader buffers
	auto& materials = m_MaterialLibrary->GetMaterials();
	std::vector<Texture*> textures;
	m_MaterialLibrary->GetTextures(textures);

	m_Storage.MRT_Pipeline.SubmitBuffer(3, sizeof(PBRMaterial) * 5, materials.data());
	m_Storage.MRT_Pipeline.UpdateSamplers(textures, 4);

	Render();
}

void CustomRenderer::Render()
{
	glm::vec3 initPos = { 4.495538, 11.454922, -8.130335 };
	float speed = 1.0f;
	m_Camera->SetPosition(initPos);
	m_Camera->SetPitch(0.007f);
	m_Camera->SetYaw(3.15f);

	while (m_Process)
	{
		m_Context->ProcessEvents();
		DeltaTime deltaTime = m_Context->CalculateDeltaTime();

		if (m_Context->IsWindowMinimized())
			continue;

		{
			if (initPos.z < 100)
				initPos.z += (initPos.x * speed) * deltaTime;
			else
				initPos = { 4.495538, 11.454922, -8.130335 };

			m_Camera->SetPosition(initPos);

#ifdef FREE_CAMERA
			m_Camera->OnUpdate(deltaTime);
#endif

			m_CameraUBO.projection = m_Camera->GetProjection();
			m_CameraUBO.view = m_Camera->GetViewMatrix();
			m_CameraUBO.camPos = glm::vec4(m_Camera->GetPosition(), 1.0f);

			bool submit_result = m_Storage.MRT_Pipeline.SubmitBuffer(10, sizeof(CameraUBO), &m_CameraUBO);
			assert(submit_result == true);
		}

		m_Context->BeginFrame(deltaTime);
		{
			m_Storage.MRT_Pipeline.BeginCommandBuffer(true);
			m_Storage.MRT_Pipeline.BeginRenderPass();
			{
				m_Storage.MRT_Pipeline.DrawMeshIndexed(m_Storage.Box_Mesh, 1000);
			}
			m_Storage.MRT_Pipeline.EndRenderPass();


			m_Storage.Comp_Pipeline.BeginCommandBuffer(true);
			m_Storage.Comp_Pipeline.BeginRenderPass();
			{
				m_Storage.Comp_Pipeline.DrawIndexed();
			}
			m_Storage.Comp_Pipeline.EndRenderPass();
		}
		m_Context->SwapBuffers();
	}
}

void CustomRenderer::OnResize(uint32_t width, uint32_t height)
{
	// Resize
	m_Storage.MRT_Framebufer.OnResize(width, height);

	// Updates descriptors
	m_Storage.Comp_Pipeline.UpdateSampler(&m_Storage.MRT_Framebufer, 0, "albedro");
	m_Storage.Comp_Pipeline.UpdateSampler(&m_Storage.MRT_Framebufer, 1, "position");
	m_Storage.Comp_Pipeline.UpdateSampler(&m_Storage.MRT_Framebufer, 2, "normals");
}

void CustomRenderer::BuildPipelines()
{
	// MRT
	{
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

		GraphicsPipelineShaderCreateInfo shaderCI = {};
		{
			shaderCI.FilePaths[ShaderType::Vertex] = "Assets/shaders/Gbuffer.vert";
			shaderCI.FilePaths[ShaderType::Fragment] = "Assets/shaders/Gbuffer.frag";

			uint32_t bindingPoint = 1;
			ShaderBufferInfo bufferInfo = {};
			bufferInfo.bGlobal = false;
			bufferInfo.bStatic = true;
			bufferInfo.Data = m_Instances.data();
			bufferInfo.Size = sizeof(InstanceUBO) * 1000;

			shaderCI.BufferInfos[bindingPoint] = bufferInfo;
		};

		GraphicsPipelineCreateInfo DynamicPipelineCI = {};
		{
			DynamicPipelineCI.VertexInputInfos = { vertexMain };
			DynamicPipelineCI.PipelineName = "MRT_Pipeline";
			DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
			DynamicPipelineCI.pTargetFramebuffer = &m_Storage.MRT_Framebufer;
		}

		auto result = m_Storage.MRT_Pipeline.Create(&DynamicPipelineCI);
		assert(result == PipelineCreateResult::SUCCESS);

		m_Instances.clear();
	}

	// Combination
	{
		GraphicsPipelineShaderCreateInfo shaderCI = {};
		{
			shaderCI.FilePaths[ShaderType::Vertex] = "../resources/Shaders/GenVertex.vert";
			shaderCI.FilePaths[ShaderType::Fragment] = "Assets/shaders/Combination.frag";
		};

		float quadVertices[] = {
			// positions   // texCoords
			-1.0f, -1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 1.0f,
			 1.0f,  1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f,  0.0f, 0.0f
		};
		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };

		struct FullSreenData
		{
			glm::vec2 pos;
			glm::vec2 uv;
		};

		BufferLayout FullSreenlayout =
		{
			{ DataTypes::Float2, "aPos" },
			{ DataTypes::Float2, "aUV" },
		};

		Ref<VertexBuffer> vb = std::make_shared<VertexBuffer>();
		Ref<IndexBuffer> ib = std::make_shared<IndexBuffer>();
		VertexBuffer::Create(vb.get(), quadVertices, sizeof(quadVertices), true);
		IndexBuffer::Create(ib.get(), squareIndices, 6, true);

		GraphicsPipelineCreateInfo DynamicPipelineCI = {};
		DynamicPipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(FullSreenData), FullSreenlayout) };
		DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
		DynamicPipelineCI.pTargetFramebuffer = m_Context->GetFramebuffer();
		DynamicPipelineCI.eCullMode = CullMode::None;

		auto result = m_Storage.Comp_Pipeline.Create(&DynamicPipelineCI);
		assert(result == PipelineCreateResult::SUCCESS);

		m_Storage.Comp_Pipeline.UpdateSampler(&m_Storage.MRT_Framebufer, 0, "albedro");
		m_Storage.Comp_Pipeline.UpdateSampler(&m_Storage.MRT_Framebufer, 1, "position");
		m_Storage.Comp_Pipeline.UpdateSampler(&m_Storage.MRT_Framebufer, 2, "normals");

		m_Storage.Comp_Pipeline.UpdateVulkanImageDescriptor(3, VulkanPBR::GetIrradianceImageInfo());
		m_Storage.Comp_Pipeline.UpdateVulkanImageDescriptor(4, VulkanPBR::GetBRDFLUTImageInfo());
		m_Storage.Comp_Pipeline.UpdateVulkanImageDescriptor(5, VulkanPBR::GetPrefilteredCubeImageInfo());

		m_Storage.Comp_Pipeline.SetVertexBuffers({ vb });
		m_Storage.Comp_Pipeline.SetIndexBuffers({ ib });
	}
}

void CustomRenderer::BuildFramebuffers()
{
	// MRT
	{
		const bool ClearOp = true;
		FramebufferAttachment albedro = FramebufferAttachment(AttachmentFormat::Color, ClearOp, "albedro");
		FramebufferAttachment position = FramebufferAttachment(AttachmentFormat::SFloat4_16, ClearOp, "position");
		FramebufferAttachment normals = FramebufferAttachment(AttachmentFormat::SFloat4_16, ClearOp, "normals");

		FramebufferSpecification framebufferCI = {};
		framebufferCI.Width = m_Context->GetWindowData()->Width;
		framebufferCI.Height = m_Context->GetWindowData()->Height;
		framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
		framebufferCI.Attachments = { albedro, position, normals };

		Framebuffer::Create(framebufferCI, &m_Storage.MRT_Framebufer);
	}

	// PBR
	{
		VulkanPBR::Init("../resources/Skyboxes/uffizi_cube.ktx", TextureFormat::R16G16B16A16_SFLOAT);
	}
}

void CustomRenderer::BuildObjects()
{
	m_Instances.resize(1000);
	m_Storage.Box_Mesh = m_Context->GetBoxMesh();

	uint32_t index = 0;
	for (uint32_t x = 0; x < 10; x += 1)
	{
		for (uint32_t z = 0; z < 100; z += 1)
		{
			float scale_y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));
			float matID = rand() % m_MaterialsIDs.size();

			glm::vec4 pos = { x, 0, z, 0 };
			glm::vec4 scale = { 0.5, scale_y, 0.5, matID };

			m_Instances[index].pos = pos;
			m_Instances[index].scale = scale;
			index++;
		}
	}
}

void CustomRenderer::BuildMaterials()
{
	MaterialLibrary* lib = m_MaterialLibrary;
	MaterialCreateInfo materialCI = {};

	std::string path = "Assets/materials/";

	{
		uint32_t id = lib->Add(&materialCI);
	}

	{
		materialCI.SetTexture(MaterialTexture::Albedro, path + "steel/CorrugatedSteel005_2K_Color.png");
		materialCI.SetTexture(MaterialTexture::Normal, path + "steel/CorrugatedSteel005_2K_Normal.png");
		materialCI.SetTexture(MaterialTexture::Roughness, path + "steel/CorrugatedSteel005_2K_Roughness.png");
		materialCI.SetTexture(MaterialTexture::Metallic, path + "steel/CorrugatedSteel005_2K_Metalness.png");

		uint32_t id = lib->Add(&materialCI);
		m_MaterialsIDs.push_back(id);
	}


	{
		materialCI.SetTexture(MaterialTexture::Albedro, path + "metal_1/Metal033_1K_Color.png");
		materialCI.SetTexture(MaterialTexture::Normal, path + "metal_1/Metal033_1K_Normal.png");
		materialCI.SetTexture(MaterialTexture::Roughness, path + "metal_1/Metal033_1K_Roughness.png");
		materialCI.SetTexture(MaterialTexture::Metallic, path + "metal_1/Metal033_1K_Metalness.png");

		uint32_t id = lib->Add(&materialCI);
		m_MaterialsIDs.push_back(id);
	}

	{
		materialCI.SetTexture(MaterialTexture::Albedro, path + "metal_2/Metal012_1K_Color.png");
		materialCI.SetTexture(MaterialTexture::Normal, path + "metal_2/Metal012_1K_Normal.png");
		materialCI.SetTexture(MaterialTexture::Roughness, path + "metal_2/Metal012_1K_Roughness.png");
		materialCI.SetTexture(MaterialTexture::Metallic, path + "metal_2/Metal012_1K_Metalness.png");

		uint32_t id = lib->Add(&materialCI);
		m_MaterialsIDs.push_back(id);
	}

	{
		materialCI.SetTexture(MaterialTexture::Albedro, path + "plane/Metal021_2K_Color.png");
		materialCI.SetTexture(MaterialTexture::Normal, path + "plane/Metal021_2K_Normal.png");
		materialCI.SetTexture(MaterialTexture::Roughness, path + "plane/Metal021_2K_Roughness.png");
		materialCI.SetTexture(MaterialTexture::Metallic, path + "plane/Metal021_2K_Metalness.png");

		uint32_t id = lib->Add(&materialCI);
		m_MaterialsIDs.push_back(id);
	}
}
