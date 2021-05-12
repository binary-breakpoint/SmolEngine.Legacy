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
#include "Utils/glTFImporter.h"

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

		s_Data->m_SceneData = &GraphicsContext::GetSingleton()->m_SceneData;
		s_Data->m_IsInitialized = true;
	}

	void Renderer::BeginScene(const ClearInfo* clearInfo)
	{
		s_Data->m_PBRPipeline.BeginCommandBuffer(true);
		s_Data->m_CombinationPipeline.BeginCommandBuffer(true);
		s_Data->m_SkyboxPipeline.BeginCommandBuffer(true);

		s_Data->m_CombinationPipeline.BeginRenderPass();
		s_Data->m_CombinationPipeline.ClearColors(clearInfo->color);
		s_Data->m_CombinationPipeline.EndRenderPass();
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
				AnimationProperties* props = nullptr;
				uint32_t animStartOffset = 0;
				bool animActive = false;

				for (uint32_t y = 0; y < mesh->GetAnimationsCount(); ++y)
				{
					props = mesh->GetAnimationProperties(y);
					if (props->IsActive())
					{
						animActive = true;
						break;
					}
				}

				const bool animated = mesh->IsAnimated();
				if (animated && mesh->m_ImportedData)
				{
					ImportedDataGlTF* imported = mesh->m_ImportedData;
					glTFAnimation* activeAnim = &imported->Animations[imported->ActiveAnimation];
					if (mesh->IsRootNode())
					{
						animStartOffset = s_Data->m_LastAnimationOffset;
						s_Data->m_RootOffsets[mesh] = animStartOffset;

						if (animActive)
							mesh->UpdateAnimations();

						uint32_t jointCount = static_cast<uint32_t>(props->Joints.size());
						for (uint32_t x = 0; x < jointCount; ++x)
						{
							s_Data->m_AnimationJoints[s_Data->m_LastAnimationOffset] = props->Joints[x];
							s_Data->m_LastAnimationOffset++;
						}
					}

					if (animated && !mesh->IsRootNode())
					{
						auto& it = s_Data->m_RootOffsets.find(mesh->m_Root);
						if (it != s_Data->m_RootOffsets.end())
							animStartOffset = it->second;
					}
				}

				Utils::ComposeTransform(*package.WorldPos, *package.Rotation, *package.Scale, shaderData.ModelView);
				shaderData.MaterialID = package.MaterialID;
				shaderData.IsAnimated = animated;
				shaderData.AnimOffset = animStartOffset;
				shaderData.EntityID = 0; // temp

				s_Data->m_InstanceDataIndex++;
			}

			instance.CurrentIndex = 0;
			s_Data->m_DrawListIndex++;
		}

		// Updates UBOs and SSBOs 
		{
			// Updates scene data
			s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_SceneDataBinding, sizeof(SceneData), s_Data->m_SceneData);
			// Updates scene state
			{
				s_Data->m_SceneState.NumPointsLights = s_Data->m_PointLightIndex;
				s_Data->m_SceneState.NumSpotLights = s_Data->m_SpotLightIndex;
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_SceneStateBinding, sizeof(SceneState), &s_Data->m_SceneState);
			}

			// Updates Directional Lights
			if (s_Data->m_DirLight.IsActive)
			{
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_DirLightBinding, sizeof(DirectionalLight), &s_Data->m_DirLight);
				if (s_Data->m_DirLight.IsCastShadows)
				{
					// Calculate Depth
					CalculateDepthMVP(s_Data->m_MainPushConstant.DepthMVP);
				}
			}

			// Updates Point Lights
			if (s_Data->m_PointLightIndex > 0)
			{
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_PointLightBinding, sizeof(PointLight) * s_Data->m_PointLightIndex, s_Data->m_PointLights.data());
			}

			// Updates Spot Lights
			if (s_Data->m_SpotLightIndex > 0)
			{
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_SpotLightBinding, sizeof(SpotLight) * s_Data->m_SpotLightIndex, s_Data->m_SpotLights.data());
			}

			// Updates Animation joints
			if (s_Data->m_LastAnimationOffset > 0)
			{
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_AnimBinding, sizeof(glm::mat4) * s_Data->m_LastAnimationOffset, s_Data->m_AnimationJoints.data());
			}

			// Updates Batch Data
			if (s_Data->m_InstanceDataIndex > 0)
			{
				s_Data->m_PBRPipeline.SubmitBuffer(s_Data->m_ShaderDataBinding, sizeof(InstanceData) * s_Data->m_InstanceDataIndex, s_Data->m_InstancesData.data());
			}
		}

		// Depth Pass
		if (s_Data->m_DirLight.IsActive && s_Data->m_DirLight.IsCastShadows)
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
			// SkyBox
			if (s_Data->m_State.bDrawSkyBox)
			{
				s_Data->m_SkyboxPipeline.Draw(36);
			}

			if (s_Data->m_State.bDrawGrid)
			{
				s_Data->m_GridPipeline.BeginCommandBuffer(true);
				s_Data->m_GridPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(glm::mat4), &s_Data->m_GridModel);
				s_Data->m_GridPipeline.DrawMeshIndexed(&s_Data->m_PlaneMesh);
			}

			for (uint32_t i = 0; i < s_Data->m_DrawListIndex; ++i)
			{
				auto& cmd = s_Data->m_DrawList[i];

				s_Data->m_MainPushConstant.DataOffset = cmd.Offset;
				s_Data->m_PBRPipeline.SubmitPushConstant(ShaderType::Vertex, s_Data->m_PushConstantSize, &s_Data->m_MainPushConstant);
				s_Data->m_PBRPipeline.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
			}
		}
		s_Data->m_PBRPipeline.EndRenderPass();

		// Post-Processing: Bloom, Blur
		{
			if (s_Data->m_State.bBloomPass)
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

			if (s_Data->m_State.bBlurPass)
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
			uint32_t state = s_Data->m_State.bBloomPass + s_Data->m_State.bBlurPass;
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
		if (s_Data->m_Frustum->CheckSphere(pos, 10.0f) && mesh != nullptr)
		{
			AddMesh(pos, rotation, scale, mesh, materialID == 0 ? mesh->m_MaterialID: materialID);
		}
	}

	void Renderer::SubmitDirLight(DirectionalLight* light)
	{
		assert(light != nullptr);
		s_Data->m_DirLight = *light;
	}

	void Renderer::SubmitPointLight(PointLight* light)
	{
		uint32_t index = s_Data->m_PointLightIndex;
		if (index >= s_MaxLights || light == nullptr)
			return;

		s_Data->m_PointLights[index] = *light;
		s_Data->m_PointLightIndex++;
	}

	void Renderer::SubmitSpotLight(SpotLight* light)
	{
		uint32_t index = s_Data->m_SpotLightIndex;
		if (index >= s_MaxLights || light == nullptr)
			return;

		s_Data->m_SpotLights[index] = *light;
		s_Data->m_SpotLightIndex++;
	}

	void Renderer::SetRenderingState(RenderingState* state)
	{
		assert(state != nullptr);
		s_Data->m_State = *state;
	}

	void Renderer::SetSceneState(SceneState* state)
	{
		assert(state != nullptr);
		s_Data->m_SceneState = *state;
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
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/PBR.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/PBR.frag";

				// Vertex
				shaderCI.StorageBuffersSizes[s_Data->m_ShaderDataBinding] = { sizeof(InstanceData) * s_InstanceDataMaxCount };
				shaderCI.StorageBuffersSizes[s_Data->m_MaterialsBinding] = { sizeof(PBRMaterial) * 1000 };
				shaderCI.StorageBuffersSizes[s_Data->m_AnimBinding] = { sizeof(glm::mat4) * s_MaxAnimationJoints};

				// Fragment
				shaderCI.StorageBuffersSizes[s_Data->m_PointLightBinding] = { sizeof(PointLight) * s_MaxLights };
				shaderCI.StorageBuffersSizes[s_Data->m_SpotLightBinding] = { sizeof(SpotLight) * s_MaxLights };
				shaderCI.StorageBuffersSizes[s_Data->m_DirLightBinding] = { sizeof(DirectionalLight) };
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

			s_Data->m_PBRPipeline.UpdateSampler(&s_Data->m_DepthFramebuffer, 1, "Depth_Attachment");
#ifndef FROSTIUM_OPENGL_IMPL
			s_Data->m_PBRPipeline.UpdateVulkanImageDescriptor(2, VulkanPBR::GetIrradianceImageInfo());
			s_Data->m_PBRPipeline.UpdateVulkanImageDescriptor(3, VulkanPBR::GetBRDFLUTImageInfo());
			s_Data->m_PBRPipeline.UpdateVulkanImageDescriptor(4, VulkanPBR::GetPrefilteredCubeImageInfo());
#endif
		}

		// Grid
		{
			Utils::ComposeTransform(glm::vec3(0), glm::vec3(0), { 100, 1, 100 }, s_Data->m_GridModel);
			Mesh::Create(GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Models/plane_v2.gltf", &s_Data->m_PlaneMesh);

			// Grid
			{
				BufferLayout PBRlayout =
				{
					{ DataTypes::Float3, "aPos" },
					{ DataTypes::Float3, "aNormal" },
					{ DataTypes::Float3, "aTangent" },
					{ DataTypes::Float2, "aUV" },
					{ DataTypes::Int4,   "aBoneIDs"},
					{ DataTypes::Float4, "aWeight"}
				};

				GraphicsPipelineCreateInfo pipelineCI = {};
				GraphicsPipelineShaderCreateInfo shaderCI = {};
				{
					shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/Grid.vert";
					shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/Grid.frag";
				};

				pipelineCI.PipelineName = "Grid";
				pipelineCI.eCullMode = CullMode::None;
				pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(PBRVertex), PBRlayout) };
				pipelineCI.pTargetFramebuffer = &s_Data->m_PBRFramebuffer;
				pipelineCI.pShaderCreateInfo = &shaderCI;

				assert(s_Data->m_GridPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
			}
		}

		// Sky Box
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Skybox.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Skybox.frag";
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
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/DepthPass.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/DepthPass.frag";
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
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/GenVertex.vert";
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
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/DebugView.frag";
					DynamicPipelineCI.PipelineName = "Debug";

					auto result = s_Data->m_DebugPipeline.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);
					s_Data->m_DebugPipeline.UpdateSampler(&s_Data->m_DepthFramebuffer, 1, "Depth_Attachment");
					s_Data->m_DebugPipeline.SetVertexBuffers({ vb });
					s_Data->m_DebugPipeline.SetIndexBuffers({ ib });
				}

				// Combination
				{
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Combination.frag";
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
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Bloom.frag";
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
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Blur.frag";
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

	void Renderer::CalculateDepthMVP(glm::mat4& out_mvp)
	{
		// Keep depth range as small as possible
		// for better shadow map precision
		auto& data = s_Data->m_DirLight;

		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(data.lightFOV, 1.0f, data.zNear, data.zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(data.Direction), glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0f);

		out_mvp =  depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
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

		for (auto& sub : mesh->m_Childs)
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

		if (s_Data->m_State.bBloomPass)
		{
			s_Data->m_BloomFramebuffer.OnResize(width, height);
			s_Data->m_BloomPipeline.UpdateSampler(&s_Data->m_PBRFramebuffer, 0);
			s_Data->m_CombinationPipeline.UpdateSampler(&s_Data->m_BloomFramebuffer, 1);
		}

		if (s_Data->m_State.bBlurPass)
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
		s_Data->m_PointLightIndex = 0;
		s_Data->m_SpotLightIndex = 0;
		s_Data->m_DrawListIndex = 0;
		s_Data->m_UsedMeshesIndex = 0;
		s_Data->m_LastAnimationOffset = 0;
		s_Data->m_RootOffsets.clear();
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