#include "stdafx.h"
#include "Renderer.h"
#include "GraphicsPipeline.h"
#include "MaterialLibrary.h"

#include "Common/Framebuffer.h"
#include "Common/VertexArray.h"
#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"
#include "Common/Shader.h"
#include "Common/Common.h"
#include "Common/Mesh.h"
#include "Common/Texture.h"

#include "Utils/Utils.h"

#ifdef FROSTIUM_OPENGL_IMPL
#include "OpenGL/OpenglShader.h"
#include "OpenGL/OpenglRendererAPI.h"
#else
#include "Vulkan/VulkanPBR.h"
#endif


namespace Frostium
{
	static const size_t                  s_MaxInstances = 1000;
	static const size_t                  s_MaxPackages = 1200;
	static const size_t                  s_MaxDirectionalLights = 1000;
	static const size_t                  s_MaxPointLights = 1000;
	static const size_t                  s_InstanceDataMaxCount = s_MaxPackages * s_MaxInstances;

	struct CommandBuffer
	{
		uint32_t                         InstancesCount = 0;
		uint32_t                         Offset = 0;
		Mesh*                            Mesh = nullptr;
	};									 
										 
	struct InstanceData
	{									 
		glm::mat4                        ModelView = glm::mat4(0.0f);
		glm::vec4                        Params = glm::vec4(0);
	};									 
										 
	struct InstancePackage				 
	{									 
		struct Package					 
		{								 
			int32_t                      MaterialID = 0;
			glm::vec3                    WorldPos = glm::vec3(0.0f);
			glm::vec3                    Rotation = glm::vec3(0.0f);
			glm::vec3                    Scale = glm::vec3(0.0f);
		};								 
										 
		uint32_t                         CurrentIndex = 0;
		Package                          Data[s_MaxInstances];
	};

	struct RendererData
	{
		RendererData(RendererInitInfo* info)
			:m_HDRPath(info->HDR)
		{
			m_Frustum = GraphicsContext::GetSingleton()->GetFrustum();
			m_Packages.reserve(s_MaxPackages);
			m_MapSize = info->sMapSize;
			m_Path = info->resourcesFilePath;
		}

		// States
		bool                             m_IsInitialized = false;
		bool                             m_ShowDebugView = false;
		const bool                       m_HDRPath = false;
		ShadowMapSize                    m_MapSize = ShadowMapSize::SIZE_8;
		// Bindings
		const uint32_t                   m_TexturesBinding = 24;
		const uint32_t                   m_ShaderDataBinding = 25;
		const uint32_t                   m_MaterialsBinding = 26;
		const uint32_t                   m_SceneDataBinding = 27;
		// Instance Data
		uint32_t                         m_Objects = 0;
		uint32_t                         m_InstanceDataIndex = 0;
		uint32_t                         m_UsedMeshesIndex = 0;
		uint32_t                         m_DrawListIndex = 0;
		uint32_t                         m_DirectionalLightIndex = 0;
		uint32_t                         m_PointLightIndex = 0;
		uint32_t                         m_MaxObjects = s_MaxPackages;
		// Pipelines
		GraphicsPipeline                 m_PBRPipeline = {};
		GraphicsPipeline                 m_BloomPipeline = {};
		GraphicsPipeline                 m_CombinationPipeline = {};
		GraphicsPipeline                 m_SkyboxPipeline = {};
		GraphicsPipeline                 m_OmniPipeline = {};
		GraphicsPipeline                 m_DepthPassPipeline = {};
		GraphicsPipeline                 m_DebugPipeline = {};			         
		// Framebuffers
		Framebuffer*                     m_MainFramebuffer = nullptr;
		Framebuffer                      m_PBRFramebuffer = {};
		Framebuffer                      m_BloomFramebuffer = {};
		Framebuffer                      m_DepthFramebuffer = {};
		Framebuffer                      m_OmniFramebuffer = {};
		// Buffers
		Mesh*                            m_UsedMeshes[s_MaxPackages];
		InstanceData                     m_InstancesData[s_InstanceDataMaxCount];
		CommandBuffer                    m_DrawList[s_MaxPackages];
		DirectionalLightBuffer           m_DirectionalLights[s_MaxDirectionalLights];
		PointLightBuffer                 m_PointLights[s_MaxPointLights];
		std::unordered_map<Mesh*,
			InstancePackage>             m_Packages;
		// UBO's & Push Constants
		struct AmbientLighting
		{
			glm::vec4                    DiffuseColor = glm::vec4(1.0f);
			glm::vec4                    SpecularColor = glm::vec4(1.0f);
			glm::vec4                    Ambient = glm::vec4(1.0f);
			glm::vec4                    Params = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f); // x = IBL scale, y = enable IBL;
		};
		struct PushConstant
		{
			glm::mat4                    DepthMVP = glm::mat4(1.0f);

			uint32_t                     DataOffset = 0;
			uint32_t                     DirectionalLights = 0;
			uint32_t                     PointLights = 0;
		};
		struct DebugView
		{
			uint32_t                     ShowOmniCube = 0;
			uint32_t                     ShowMRT = 0;		                 
			uint32_t                     MRTattachmentIndex = 0;
		};
		struct ShadowMatrix
		{
			glm::mat4 shadowTransforms[6];
		};

		std::string                      m_Path = "";
		float                            m_NearClip = 1.0f;
		float                            m_FarClip = 1000.0f;
		glm::vec3                        m_ShadowLightDirection = glm::vec3(0.0f, 0.0f, 0.0f);
		Frustum*                         m_Frustum = nullptr;
		DebugView                        m_DebugView = {};
		SceneData                        m_SceneData = {};
		PushConstant                     m_MainPushConstant = {};
		ShadowMatrix                     m_ShadowMatrix = {};
		AmbientLighting                  m_AmbientLighting = {};
		// Sizes
		const size_t                     m_DebugViewSize = sizeof(DebugView);
		const size_t                     m_SceneDataSize = sizeof(SceneData);
		const size_t                     m_PushConstantSize = sizeof(PushConstant);
		const size_t                     m_AmbientLightingSize = sizeof(AmbientLighting);

	};

	static RendererData* s_Data = nullptr;

	void Renderer::Init(RendererInitInfo* info)
	{
		s_Data = new RendererData(info);

		InitPBR();
		InitFramebuffers();
		InitPipelines();

		s_Data->m_IsInitialized = true;
	}

	void Renderer::Shutdown()
	{
		if (s_Data != nullptr)
		{
			s_Data->m_IsInitialized = false;
			delete s_Data;
		}
	}

	void Renderer::BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info)
	{
		if (GraphicsContext::GetSingleton()->m_State->UseEditorCamera)
		{
			EditorCamera* camera = GraphicsContext::GetSingleton()->GetEditorCamera();

			s_Data->m_SceneData.View = camera->GetViewMatrix();
			s_Data->m_SceneData.Projection = camera->GetProjection();
			s_Data->m_SceneData.CamPos = glm::vec4(camera->GetPosition(), 1.0f);
			s_Data->m_SceneData.SkyBoxMatrix = glm::mat4(glm::mat3(camera->GetViewMatrix()));
			s_Data->m_NearClip = camera->GetNearClip();
			s_Data->m_FarClip = camera->GetFarClip();
		}

		if (info)
		{
			s_Data->m_SceneData.View = info->view;
			s_Data->m_SceneData.Projection = info->proj;
			s_Data->m_SceneData.CamPos = glm::vec4(info->pos, 1);
			s_Data->m_SceneData.SkyBoxMatrix = glm::mat4(glm::mat3(info->view));
			s_Data->m_NearClip = info->nearClip;
			s_Data->m_FarClip = info->farClip;
		}

		s_Data->m_Frustum->Update(s_Data->m_SceneData.Projection * s_Data->m_SceneData.View);

		s_Data->m_PBRPipeline.BeginCommandBuffer(true);
		s_Data->m_CombinationPipeline.BeginCommandBuffer(true);
		s_Data->m_SkyboxPipeline.BeginCommandBuffer(true);

		if (clearInfo->bClear)
		{
			s_Data->m_CombinationPipeline.BeginRenderPass();
			s_Data->m_CombinationPipeline.ClearColors(clearInfo->color);
			s_Data->m_CombinationPipeline.EndRenderPass();
		}

		Reset();
	}

	void Renderer::EndScene()
	{
		Flush();
		s_Data->m_PBRPipeline.EndCommandBuffer();
		s_Data->m_SkyboxPipeline.EndCommandBuffer();
		s_Data->m_CombinationPipeline.EndCommandBuffer();
	}

	void Renderer::Flush()
	{
		for (uint32_t i = 0; i < s_Data->m_UsedMeshesIndex; ++i)
		{
			// Getting values
			Mesh* mesh = s_Data->m_UsedMeshes[i];
			auto& instance = s_Data->m_Packages[mesh];
			auto& cmd = s_Data->m_DrawList[s_Data->m_DrawListIndex];

			// Setting draw list command
			cmd.Offset = s_Data->m_InstanceDataIndex;
			cmd.Mesh = mesh;
			cmd.InstancesCount = instance.CurrentIndex;

			for (uint32_t x = 0; x < instance.CurrentIndex; ++x)
			{
				auto& package = instance.Data[x];
				auto& shaderData = s_Data->m_InstancesData[s_Data->m_InstanceDataIndex];

				shaderData.Params.x = static_cast<float>(package.MaterialID);
				Utils::ComposeTransform(package.WorldPos, package.Rotation, package.Scale, true, shaderData.ModelView);
				s_Data->m_InstanceDataIndex++;
			}

			instance.CurrentIndex = 0;
			s_Data->m_DrawListIndex++;
		}

		// Updates UBOs and SSBOs 
		{
			// Updates scene data
			s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_SceneDataBinding, s_Data->m_SceneDataSize, &s_Data->m_SceneData);

			if (s_Data->m_DirectionalLightIndex > 0)
			{
				// Updates Directional Lights
				s_Data->m_PBRPipeline.SubmitBuffer(28, sizeof(DirectionalLightBuffer) * s_Data->m_DirectionalLightIndex, &s_Data->m_DirectionalLights);
				// Calculate Depth
				s_Data->m_MainPushConstant.DepthMVP = CalculateDepthMVP(s_Data->m_ShadowLightDirection);
			}

			if (s_Data->m_PointLightIndex > 0)
			{
				// Updates Point Lights
				s_Data->m_PBRPipeline.SubmitBuffer(29, sizeof(PointLightBuffer) * s_Data->m_PointLightIndex, &s_Data->m_PointLights);
			}

			// Updates Ambient Lighting
			s_Data->m_PBRPipeline.SubmitBuffer(30, s_Data->m_AmbientLightingSize, &s_Data->m_AmbientLighting);

			// Updates model views and material indexes
			s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_ShaderDataBinding, sizeof(InstanceData) * s_Data->m_InstanceDataIndex, &s_Data->m_InstancesData);
		}

		// Depth Pass
		if (s_Data->m_DirectionalLightIndex > 0)
		{
			s_Data->m_DepthPassPipeline.BeginCommandBuffer(true);
#ifndef FROSTIUM_OPENGL_IMPL
			VkCommandBuffer cmdBuffer = s_Data->m_DepthPassPipeline.GetVkCommandBuffer();
#endif
			s_Data->m_DepthPassPipeline.BeginRenderPass();
			{
#ifndef FROSTIUM_OPENGL_IMPL
				// Set depth bias (aka "Polygon offset")
				// Required to avoid shadow mapping artifacts
				vkCmdSetDepthBias(cmdBuffer, 1.25f, 0.0f, 1.75f);
#endif

				struct PushConstant
				{
					glm::mat4 depthMVP;
					uint32_t offset;

				} static pc;

				pc.depthMVP = s_Data->m_MainPushConstant.DepthMVP;

				for (uint32_t i = 0; i < s_Data->m_DrawListIndex; ++i)
				{
					auto& cmd = s_Data->m_DrawList[i];
					pc.offset = cmd.Offset;

					s_Data->m_DepthPassPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &pc);
					s_Data->m_DepthPassPipeline.DrawMesh(cmd.Mesh, cmd.InstancesCount);
				}
			}
			s_Data->m_DepthPassPipeline.EndRenderPass();

		}

		// Geometry + SkyBox + Ligting (temp)
		s_Data->m_PBRPipeline.BeginRenderPass();
		{
			s_Data->m_SkyboxPipeline.Draw(36);

			for (uint32_t i = 0; i < s_Data->m_DrawListIndex; ++i)
			{
				auto& cmd = s_Data->m_DrawList[i];

				s_Data->m_MainPushConstant.DataOffset = cmd.Offset;
				s_Data->m_MainPushConstant.DirectionalLights = s_Data->m_DirectionalLightIndex;
				s_Data->m_MainPushConstant.PointLights = s_Data->m_PointLightIndex;

				s_Data->m_PBRPipeline.SubmitPushConstant(ShaderType::Vertex, s_Data->m_PushConstantSize, &s_Data->m_MainPushConstant);
				s_Data->m_PBRPipeline.DrawMesh(cmd.Mesh, cmd.InstancesCount);
			}
		}
		s_Data->m_PBRPipeline.EndRenderPass();

		// Post-Processing: Bloom (vertical)
		{
			if (s_Data->m_HDRPath)
			{
				s_Data->m_BloomPipeline.BeginCommandBuffer(true);
				s_Data->m_BloomPipeline.BeginRenderPass();
				s_Data->m_BloomPipeline.DrawIndexed();
				s_Data->m_BloomPipeline.EndRenderPass();
				s_Data->m_BloomPipeline.EndCommandBuffer();
			}
		}

		// Composition => render to swapchain
		{
			uint32_t hdr = s_Data->m_HDRPath;
			s_Data->m_CombinationPipeline.BeginRenderPass();
			{
				s_Data->m_CombinationPipeline.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &hdr);
				s_Data->m_CombinationPipeline.DrawIndexed();
			}
			s_Data->m_CombinationPipeline.EndRenderPass();
		}
	}

	void Renderer::StartNewBacth()
	{
		Flush();
		Reset();
	}

	void Renderer::SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation,
		const glm::vec3& scale, Mesh* mesh, int32_t materialID)
	{
		if (s_Data->m_Frustum->CheckSphere(pos, 3.0f))
		{
			if (s_Data->m_Objects >= s_Data->m_MaxObjects)
				StartNewBacth();

			auto& instance = s_Data->m_Packages[mesh];
			if (instance.CurrentIndex >= s_MaxInstances)
				StartNewBacth();

			auto& package = instance.Data[instance.CurrentIndex];

			package.MaterialID = materialID;
			package.WorldPos = pos;
			package.Rotation = rotation;
			package.Scale = scale;
			instance.CurrentIndex++;

			bool found = false;
			for (uint32_t i = 0; i < s_Data->m_UsedMeshesIndex; ++i)
			{
				if (s_Data->m_UsedMeshes[i] == mesh)
				{
					found = true;
					break;
				}
			}

			if (found == false)
			{
				s_Data->m_UsedMeshes[s_Data->m_UsedMeshesIndex] = mesh;
				s_Data->m_UsedMeshesIndex++;
			}
			s_Data->m_Objects++;

			for (auto& sub : mesh->GetSubMeshes())
				SubmitMesh(pos, rotation, scale, sub);
		}
	}

	void Renderer::SubmitDirectionalLight(const glm::vec3& dir, const glm::vec4& color)
	{
		uint32_t index = s_Data->m_DirectionalLightIndex;
		if (index >= s_MaxDirectionalLights)
			return; // temp;

		s_Data->m_DirectionalLights[index].Color = color;
		s_Data->m_DirectionalLights[index].Position = glm::normalize(glm::vec4(dir, 1.0f));
		s_Data->m_DirectionalLightIndex++;
	}

	void Renderer::SubmitPointLight(const glm::vec3& pos, const glm::vec4& color, float constant, float linear, float exp)
	{
		uint32_t index = s_Data->m_PointLightIndex;
		if (index >= s_MaxPointLights)
			return; // temp;

		s_Data->m_PointLights[index].Color = color;
		s_Data->m_PointLights[index].Position = glm::normalize(glm::vec4(pos, 1.0f));
		s_Data->m_PointLights[index].Params.x = constant;
		s_Data->m_PointLights[index].Params.y = linear;
		s_Data->m_PointLights[index].Params.z = exp;
		s_Data->m_PointLightIndex++;
	}

	void Renderer::SetDebugViewParams(DebugViewInfo& info)
	{
		s_Data->m_DebugView.ShowOmniCube = info.bShowOmniCube;
		s_Data->m_DebugView.ShowMRT = info.bShowMRT;
		s_Data->m_DebugView.MRTattachmentIndex = info.mrtAttachmentIndex;
	}

	void Renderer::SetAmbientLighting(const glm::vec3& diffuseColor, glm::vec3& specularColor, float IBLscale, bool enableIBL, glm::vec3& ambient)
	{
		s_Data->m_AmbientLighting.DiffuseColor = glm::vec4(diffuseColor, 0.0f);
		s_Data->m_AmbientLighting.SpecularColor = glm::vec4(specularColor, 0.0f);
		s_Data->m_AmbientLighting.Ambient = glm::vec4(ambient, 0.0f);
		s_Data->m_AmbientLighting.Params.x = IBLscale;
		s_Data->m_AmbientLighting.Params.y = enableIBL ? 1.0f : 0.0f;
	}

	void Renderer::SetShadowLightDirection(const glm::vec3& dir)
	{
		s_Data->m_ShadowLightDirection = dir;
	}

	void Renderer::SetAmbientMixer(float value)
	{
		s_Data->m_SceneData.Params.z = value;
	}

	void Renderer::SetGamma(float value)
	{
		s_Data->m_SceneData.Params.y = value;
	}

	void Renderer::SetExposure(float value)
	{
		s_Data->m_SceneData.Params.x = value;
	}

	void Renderer::SetActiveDebugView(bool value)
	{
		s_Data->m_ShowDebugView = value;
	}

	void Renderer::InitPBR()
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		VulkanPBR::Init(GraphicsContext::s_Instance->m_ResourcesFolderPath + "Skyboxes/uffizi_cube.ktx", TextureFormat::R16G16B16A16_SFLOAT);
#endif
	}

	void Renderer::InitPipelines()
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

		// PBR
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Vulkan/PBR.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_HDRPath ? s_Data->m_Path + "Shaders/Vulkan/PBR_HDR.frag" : s_Data->m_Path + "Shaders/Vulkan/PBR.frag";

				shaderCI.StorageBuffersSizes[25] = { sizeof(InstanceData) * s_InstanceDataMaxCount };
				shaderCI.StorageBuffersSizes[26] = { sizeof(Material) * 1000 };
				shaderCI.StorageBuffersSizes[28] = { sizeof(DirectionalLightBuffer) * s_MaxDirectionalLights };
				shaderCI.StorageBuffersSizes[29] = { sizeof(PointLightBuffer) * s_MaxPointLights };
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { vertexMain };
				DynamicPipelineCI.PipelineName = "PBR_Pipeline";
				DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
				DynamicPipelineCI.pTargetFramebuffer = &s_Data->m_PBRFramebuffer;
			}

			auto result = s_Data->m_PBRPipeline.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);

			s_Data->m_PBRPipeline.SubmitBuffer(30, s_Data->m_AmbientLightingSize, &s_Data->m_AmbientLighting);
			s_Data->m_PBRPipeline.UpdateSampler(&s_Data->m_DepthFramebuffer, 1, "Depth_Attachment");
#ifndef FROSTIUM_OPENGL_IMPL
			s_Data->m_PBRPipeline.UpdateVulkanImageDescriptor(2, VulkanPBR::GetIrradianceImageInfo());
			s_Data->m_PBRPipeline.UpdateVulkanImageDescriptor(3, VulkanPBR::GetBRDFLUTImageInfo());
			s_Data->m_PBRPipeline.UpdateVulkanImageDescriptor(4, VulkanPBR::GetPrefilteredCubeImageInfo());
#endif
		}

		// Sky Box
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Vulkan/Skybox.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_HDRPath ? s_Data->m_Path + "Shaders/Vulkan/Skybox_HDR.frag" : s_Data->m_Path + "Shaders/Vulkan/Skybox.frag";
			};

			struct SkyBoxData
			{
				glm::vec3 pos;
			};

			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" }
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(SkyBoxData), layout) };
				DynamicPipelineCI.PipelineName = "Skybox_Pipiline";
				DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
				DynamicPipelineCI.bDepthTestEnabled = false;
				DynamicPipelineCI.bDepthWriteEnabled = false;
				DynamicPipelineCI.pTargetFramebuffer = &s_Data->m_PBRFramebuffer;
			}

			auto result = s_Data->m_SkyboxPipeline.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);
#ifndef FROSTIUM_OPENGL_IMPL
			s_Data->m_SkyboxPipeline.UpdateVulkanImageDescriptor(1, VulkanPBR::GetSkyBox().GetVkDescriptorImageInfo());
#endif
			float skyboxVertices[] = {
				// positions          
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};

			Ref<VertexBuffer> skyBoxFB = std::make_shared<VertexBuffer>();
			VertexBuffer::Create(skyBoxFB.get(), skyboxVertices, sizeof(skyboxVertices));
			s_Data->m_SkyboxPipeline.SetVertexBuffers({ skyBoxFB });
		}

		// Depth Pass
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Vulkan/DepthPass.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/DepthPass.frag";
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { vertexMain };
				DynamicPipelineCI.PipelineName = "DepthPass_Pipeline";
				DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
				DynamicPipelineCI.pTargetFramebuffer = &s_Data->m_DepthFramebuffer;
				DynamicPipelineCI.bDepthBiasEnabled = true;
				DynamicPipelineCI.StageCount = 1;

				auto result = s_Data->m_DepthPassPipeline.Create(&DynamicPipelineCI);
				assert(result == PipelineCreateResult::SUCCESS);
			}
		}

		// Debug/Combination/Bloom Pipelines
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Vulkan/GenVertex.vert";
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
			VertexBuffer::Create(vb.get(), quadVertices, sizeof(quadVertices));
			IndexBuffer::Create(ib.get(), squareIndices, 6);

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(FullSreenData), FullSreenlayout) };
				DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
				DynamicPipelineCI.pTargetFramebuffer = s_Data->m_MainFramebuffer;
				DynamicPipelineCI.eCullMode = CullMode::None;

				// Debug
				{
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/DebugView.frag";
					DynamicPipelineCI.PipelineName = "Debug";

					auto result = s_Data->m_DebugPipeline.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);
					s_Data->m_DebugPipeline.UpdateSampler(&s_Data->m_DepthFramebuffer, 1, "Depth_Attachment");
					s_Data->m_DebugPipeline.SetVertexBuffers({ vb });
					s_Data->m_DebugPipeline.SetIndexBuffers({ ib });
				}

				// Combination
				{
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/Combination.frag";
					DynamicPipelineCI.PipelineName = "Combination";

					auto result = s_Data->m_CombinationPipeline.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);
					s_Data->m_CombinationPipeline.SetVertexBuffers({ vb });
					s_Data->m_CombinationPipeline.SetIndexBuffers({ ib });
					s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer,  0);
					if (s_Data->m_HDRPath)
						s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BloomFramebuffer, 1);
				}

				if (s_Data->m_HDRPath)
				{
					// Bloom
					{
						shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/Bloom.frag";
						DynamicPipelineCI.PipelineName = "Bloom";
						DynamicPipelineCI.pTargetFramebuffer = &s_Data->m_BloomFramebuffer;

						auto result = s_Data->m_BloomPipeline.Create(&DynamicPipelineCI);
						assert(result == PipelineCreateResult::SUCCESS);
						s_Data->m_BloomPipeline.SetVertexBuffers({ vb });
						s_Data->m_BloomPipeline.SetIndexBuffers({ ib });
						s_Data->m_BloomPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0, "color_1");
					}
				}

			}
		}

#if 0
		// Omni Pass
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/OmniShadow.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/OmniShadow.frag";
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { vertexMain };
				DynamicPipelineCI.PipelineName = "OmniPass_Pipeline";
				DynamicPipelineCI.pShaderCreateInfo = &shaderCI;
				DynamicPipelineCI.pTargetFramebuffer = &s_Data->m_OmniFramebuffer;

				auto result = s_Data->m_OmniPipeline.Create(&DynamicPipelineCI);
				assert(result == PipelineCreateResult::SUCCESS);
			}
		}
#endif
	}

	void Renderer::InitFramebuffers()
	{
		// Main
		s_Data->m_MainFramebuffer = GraphicsContext::GetSingleton()->GetFramebuffer();

		// PBR
		{
			FramebufferSpecification framebufferCI = {};
			framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
			framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
			framebufferCI.eMSAASampels = GraphicsContext::GetSingleton()->m_MSAASamples;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color, true, "color_1") };
			if (s_Data->m_HDRPath)
			{
				FramebufferAttachment color_1 = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_1");
				FramebufferAttachment color_2 = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_2");
				framebufferCI.Attachments = { color_1, color_2 };
			}

			Framebuffer::Create(framebufferCI, &s_Data->m_PBRFramebuffer);
		}

		// Depth
		{
			FramebufferSpecification framebufferCI = {};
			uint32_t size = 8192;
			switch (s_Data->m_MapSize)
			{
			case ShadowMapSize::SIZE_2: size = 2048; break;
			case ShadowMapSize::SIZE_4: size = 4096; break;
			case ShadowMapSize::SIZE_8: size = 8192; break;
			case ShadowMapSize::SIZE_16: size = 16384; break;
			}
			framebufferCI.Width = size;
			framebufferCI.Height = size;
			framebufferCI.bResizable = false;
			framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
			framebufferCI.eSpecialisation = FramebufferSpecialisation::ShadowMap;

			Framebuffer::Create(framebufferCI, &s_Data->m_DepthFramebuffer);
		}

		if (s_Data->m_HDRPath)
		{
			// Bloom
			{
				FramebufferAttachment bloom = FramebufferAttachment(AttachmentFormat::SFloat4_32, false);

				FramebufferSpecification framebufferCI = {};
				framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
				framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
				framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
				framebufferCI.Attachments = { bloom };
				Framebuffer::Create(framebufferCI, &s_Data->m_BloomFramebuffer);
			}
		}
	}

	glm::mat4 Renderer::CalculateDepthMVP(const glm::vec3& lightPos)
	{
		// Keep depth range as small as possible
		// for better shadow map precision
		float zNear = 1.0f;
		float zFar = 500.0f;
		float lightFOV = 45.0f;

		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(lightFOV, 1.0f, zNear, zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0f);

		return depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	}

	bool Renderer::UpdateMaterials()
	{
		if (!s_Data->m_IsInitialized)
			return false;

		std::vector<Texture*> tetxures;
		MaterialLibrary::GetSinglenton()->GetTextures(tetxures);
		s_Data->m_PBRPipeline.UpdateSamplers(tetxures, s_Data->m_TexturesBinding);

		void* data = nullptr;
		uint32_t size = 0;
		MaterialLibrary::GetSinglenton()->GetMaterialsPtr(data, size);
		s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_MaterialsBinding, size, data);

		return true;
	}

	void Renderer::OnResize(uint32_t width, uint32_t height)
	{
		s_Data->m_PBRFramebuffer.OnResize(width, height);
		s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0);

		if (s_Data->m_HDRPath)
		{
			s_Data->m_BloomFramebuffer.OnResize(width, height);
			s_Data->m_BloomPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0);
			s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BloomFramebuffer, 1);
		}
	}

	void Renderer::Reset()
	{
		s_Data->m_Objects = 0;
		s_Data->m_InstanceDataIndex = 0;
		s_Data->m_DirectionalLightIndex = 0;
		s_Data->m_PointLightIndex = 0;
		s_Data->m_DrawListIndex = 0;
		s_Data->m_UsedMeshesIndex = 0;
	}

	Framebuffer* Renderer::GetFramebuffer()
	{
		return s_Data->m_MainFramebuffer;
	}

	uint32_t Renderer::GetNumObjects()
	{
		return s_Data->m_Objects;
	}

}