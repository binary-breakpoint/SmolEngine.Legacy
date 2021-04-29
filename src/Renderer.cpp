#include "stdafx.h"
#include "Renderer.h"
#include "MaterialLibrary.h"

#include "Common/VertexArray.h"
#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"
#include "Common/Shader.h"
#include "Common/Common.h"

#include "Utils/glTFImporter.h"
#include "Utils/Utils.h"

#ifdef FROSTIUM_OPENGL_IMPL
#include "OpenGL/OpenglShader.h"
#include "OpenGL/OpenglRendererAPI.h"
#else
#include "Vulkan/VulkanPBR.h"
#endif

#include "Common/RendererStorage.h"

namespace Frostium
{
	static RendererStorage* s_Data = nullptr;
	void Renderer::Init(RendererStorage* storage)
	{
		if (storage == nullptr)
			std::runtime_error("Renderer: storage is nullptr!");

		s_Data = storage;
		{
			InitPBR();
			InitFramebuffers();
			InitPipelines();
		}
		s_Data->m_IsInitialized = true;
	}

	void Renderer::BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info)
	{
		if (GraphicsContext::GetSingleton()->m_State->UseEditorCamera)
		{
			Camera* camera = GraphicsContext::GetSingleton()->GetDefaultCamera();

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
			InstancePackage& instance = s_Data->m_Packages[mesh];
			CommandBuffer& cmd = s_Data->m_DrawList[s_Data->m_DrawListIndex];

			// Setting draw list command
			cmd.Offset = s_Data->m_InstanceDataIndex;
			cmd.Mesh = mesh;
			cmd.InstancesCount = instance.CurrentIndex;

			for (uint32_t x = 0; x < instance.CurrentIndex; ++x)
			{
				InstancePackage::Package& package = instance.Data[x];
				InstanceData& shaderData = s_Data->m_InstancesData[s_Data->m_InstanceDataIndex];

				Utils::ComposeTransform(*package.WorldPos, *package.Rotation, *package.Scale, shaderData.ModelView);
				shaderData.MaterialIDs = package.MaterialID;

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
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_DirLightBinding, sizeof(DirectionalLightBuffer) * s_Data->m_DirectionalLightIndex, &s_Data->m_DirectionalLights);
				// Calculate Depth
				s_Data->m_MainPushConstant.DepthMVP = CalculateDepthMVP(s_Data->m_ShadowLightDirection);
			}

			if (s_Data->m_PointLightIndex > 0)
			{
				// Updates Point Lights
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_PointLightBinding, sizeof(PointLightBuffer) * s_Data->m_PointLightIndex, &s_Data->m_PointLights);
			}

			// Updates Ambient Lighting
			s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_AmbientLightBinding, s_Data->m_AmbientLightingSize, &s_Data->m_AmbientLighting);
			// Updates model views
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
					s_Data->m_DepthPassPipeline.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
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
				s_Data->m_PBRPipeline.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
			}
		}
		s_Data->m_PBRPipeline.EndRenderPass();

		// Post-Processing: Bloom, Blur
		{
			if (s_Data->m_IsBloomPassActive)
			{
				s_Data->m_BloomPipeline.BeginCommandBuffer(true);
				{
					s_Data->m_BloomPipeline.BeginRenderPass();
					{
						uint32_t dir = 1;
						s_Data->m_BloomPipeline.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &dir);
						s_Data->m_BloomPipeline.DrawIndexed();
					}
					s_Data->m_BloomPipeline.EndRenderPass();
				}
				s_Data->m_BloomPipeline.EndCommandBuffer();
			}

			if (s_Data->m_IsBlurPassActive)
			{
				s_Data->m_BlurPipeline.BeginCommandBuffer(true);
				{
					struct Info
					{
						uint32_t dir = 0;
						float blurScale = 1.0f;
						float blurStrength = 0.5f;
					} data = {};

					s_Data->m_BlurPipeline.BeginRenderPass();
					{
						data.dir = 1;
						s_Data->m_BlurPipeline.SubmitPushConstant(ShaderType::Fragment, sizeof(Info), &data);
						s_Data->m_BlurPipeline.DrawIndexed();
						data.dir = 0;
						s_Data->m_BlurPipeline.SubmitPushConstant(ShaderType::Fragment, sizeof(Info), &data);
						s_Data->m_BlurPipeline.DrawIndexed();
					}
					s_Data->m_BlurPipeline.EndRenderPass();
				}
				s_Data->m_BlurPipeline.EndCommandBuffer();
			}
		}

		// Composition => render to swapchain
		{
			uint32_t state = s_Data->m_IsBloomPassActive + s_Data->m_IsBlurPassActive;
			s_Data->m_CombinationPipeline.BeginRenderPass();
			{
				s_Data->m_CombinationPipeline.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
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
		const glm::vec3& scale, Mesh* mesh, const uint32_t& materialID)
	{
		if (s_Data->m_Frustum->CheckSphere(pos, 3.0f))
			AddMesh(pos, rotation, scale, mesh, materialID);
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

	void Renderer::SetActiveDebugView(bool active)
	{
		s_Data->m_ShowDebugView = active;
	}

	void Renderer::SetActiveBloomPass(bool active)
	{
		s_Data->m_IsBloomPassActive = active;
	}

	void Renderer::SetActiveBlurPass(bool active)
	{
		s_Data->m_IsBlurPassActive = active;
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
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/PBR.frag";

				// Vertex
				shaderCI.StorageBuffersSizes[s_Data->m_ShaderDataBinding] = { sizeof(InstanceData) * s_InstanceDataMaxCount };
				shaderCI.StorageBuffersSizes[s_Data->m_MaterialsBinding] = { sizeof(PBRMaterial) * 1000 };

				// Fragment
				shaderCI.StorageBuffersSizes[s_Data->m_DirLightBinding] = { sizeof(DirectionalLightBuffer) * s_MaxDirectionalLights };
				shaderCI.StorageBuffersSizes[s_Data->m_PointLightBinding] = { sizeof(PointLightBuffer) * s_MaxPointLights };
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
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/Skybox.frag";
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

		// Debug/Combination/Bloom/Blur Pipelines
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
					s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BloomFramebuffer, 1);
					s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BlurFramebuffer, 2);
				}

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

				// Blur
				{
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Vulkan/Blur.frag";
					DynamicPipelineCI.PipelineName = "Blur";
					DynamicPipelineCI.pTargetFramebuffer = &s_Data->m_BlurFramebuffer;

					auto result = s_Data->m_BlurPipeline.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);
					s_Data->m_BlurPipeline.SetVertexBuffers({ vb });
					s_Data->m_BlurPipeline.SetIndexBuffers({ ib });
					s_Data->m_BlurPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0, "color_1");
				}
			}
		}
	}

	void Renderer::InitFramebuffers()
	{
		// Main
		s_Data->m_MainFramebuffer = GraphicsContext::GetSingleton()->GetFramebuffer();

		// PBR
		{
			FramebufferAttachment color_1 = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_1");
			FramebufferAttachment color_2 = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_2");

			FramebufferSpecification framebufferCI = {};
			framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
			framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
			framebufferCI.eMSAASampels = GraphicsContext::GetSingleton()->m_MSAASamples;
			framebufferCI.Attachments = { color_1, color_2 };

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

		// Bloom + Blur
		{
			FramebufferAttachment bloom = FramebufferAttachment(AttachmentFormat::SFloat4_32, false);

			FramebufferSpecification framebufferCI = {};
			framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
			framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
			framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
			framebufferCI.Attachments = { bloom };
			Framebuffer::Create(framebufferCI, &s_Data->m_BloomFramebuffer);
			Framebuffer::Create(framebufferCI, &s_Data->m_BlurFramebuffer);
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

	void Renderer::AddMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh, const uint32_t& materialID)
	{
		if (s_Data->m_Objects >= s_Data->m_MaxObjects)
			StartNewBacth();

		auto& instance = s_Data->m_Packages[mesh];
		if (instance.CurrentIndex >= s_MaxInstances)
			StartNewBacth();

		auto& package = instance.Data[instance.CurrentIndex];

		package.MaterialID = materialID;
		package.WorldPos = const_cast<glm::vec3*>(&pos);
		package.Rotation = const_cast<glm::vec3*>(&rotation);
		package.Scale = const_cast<glm::vec3*>(&scale);
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

		for (auto& sub : mesh->m_Meshes)
			AddMesh(pos, rotation, scale, &sub, sub.m_MaterialID);
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

		if (s_Data->m_IsBloomPassActive)
		{
			s_Data->m_BloomFramebuffer.OnResize(width, height);
			s_Data->m_BloomPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0);
			s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BloomFramebuffer, 1);
		}

		if (s_Data->m_IsBlurPassActive)
		{
			s_Data->m_BlurFramebuffer.OnResize(width, height);
			s_Data->m_BlurPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0);
			s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BlurFramebuffer, 2);
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

	void Renderer::Shutdown()
	{
		if (s_Data != nullptr)
			s_Data->m_IsInitialized = false;
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