#include "stdafx.h"
#include "Renderer/DeferredRenderer.h"
#include "Tools/MaterialLibrary.h"

#include "Common/Common.h"
#include "Utils/Utils.h"

#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/Shader.h"

#include "Import/glTFImporter.h"
#include "Animation/AnimationController.h"

#include "Multithreading/JobsSystemInstance.h"

#ifdef FROSTIUM_OPENGL_IMPL
#include "Backends/OpenGL/OpenglShader.h"
#include "Backends/OpenGL/OpenglRendererAPI.h"
#else
#include "Backends/Vulkan/VulkanPBR.h"
#endif


#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	static RendererStorage* s_Data = nullptr;

	void DeferredRenderer::Init(RendererStorage* storage)
	{
		if (storage == nullptr)
		{
			throw std::runtime_error("Renderer: storage is nullptr!");
			abort();
		}

		s_Data = storage;
		{
			InitPBR();
			InitFramebuffers();
			InitPipelines();
		}

		s_Data->m_SceneData = &GraphicsContext::GetSingleton()->m_SceneData;
		s_Data->m_IsInitialized = true;
	}

	void DeferredRenderer::BeginScene(const ClearInfo* clearInfo)
	{
		bool mainCmd = true;
		s_Data->p_Gbuffer.BeginCommandBuffer(mainCmd);
		s_Data->p_Lighting.BeginCommandBuffer(mainCmd);
		s_Data->p_Combination.BeginCommandBuffer(mainCmd);
		s_Data->p_Skybox.BeginCommandBuffer(mainCmd);
		s_Data->p_DepthPass.BeginCommandBuffer(mainCmd);
		s_Data->p_Bloom.BeginCommandBuffer(mainCmd);
		s_Data->p_Debug.BeginCommandBuffer(mainCmd);
		s_Data->p_Grid.BeginCommandBuffer(mainCmd);

		s_Data->p_Combination.BeginRenderPass();
		s_Data->p_Combination.ClearColors(clearInfo->color);
		s_Data->p_Combination.EndRenderPass();
		Reset();
	}

	void DeferredRenderer::EndScene()
	{
		Flush();
	}

	void DeferredRenderer::DrawOffscreen(Framebuffer* fb, BeginSceneInfo* info, RendererStateEX* state)
	{
		SceneData oldScene = *s_Data->m_SceneData;
		RendererStateEX oldState = s_Data->m_State;
		CommandBufferStorage cmdStorage = {};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			s_Data->p_Combination.SetFramebuffers({ fb });
			// Update
			{
				s_Data->p_Gbuffer.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_Lighting.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_Combination.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_Skybox.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_DepthPass.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_Bloom.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_Debug.SetCommandBuffer(cmdStorage.Buffer);
				s_Data->p_Grid.SetCommandBuffer(cmdStorage.Buffer);

				s_Data->m_State = *state;
				s_Data->m_SceneData->View = info->View;
				s_Data->m_SceneData->Projection = info->Proj;
				s_Data->m_SceneData->CamPos = glm::vec4(info->Pos, 1);
				s_Data->m_SceneData->SkyBoxMatrix = glm::mat4(glm::mat3(info->View));
				s_Data->m_SceneData->NearClip = info->NearClip;
				s_Data->m_SceneData->FarClip = info->FarClip;
				s_Data->m_Frustum->Update(s_Data->m_SceneData->Projection * s_Data->m_SceneData->View);
			}

			UpdateUniforms();
			Render();
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		// Reset
		bool mainCmd = true;
		s_Data->p_Gbuffer.BeginCommandBuffer(mainCmd);
		s_Data->p_Lighting.BeginCommandBuffer(mainCmd);
		s_Data->p_Combination.BeginCommandBuffer(mainCmd);
		s_Data->p_Skybox.BeginCommandBuffer(mainCmd);
		s_Data->p_DepthPass.BeginCommandBuffer(mainCmd);
		s_Data->p_Bloom.BeginCommandBuffer(mainCmd);
		s_Data->p_Debug.BeginCommandBuffer(mainCmd);
		s_Data->p_Grid.BeginCommandBuffer(mainCmd);
		s_Data->p_Combination.SetFramebuffers({ GraphicsContext::GetSingleton()->GetFramebuffer() });

		*s_Data->m_SceneData = oldScene;
		s_Data->m_State = oldState;

		s_Data->m_Frustum->Update(s_Data->m_SceneData->Projection * s_Data->m_SceneData->View);
		UpdateUniforms();
	}

	void DeferredRenderer::Flush()
	{
		BuildDrawList();
		UpdateUniforms();
		Render();
	}

	void DeferredRenderer::Render()
	{
		uint32_t cmdCount = static_cast<uint32_t>(s_Data->m_UsedMeshes.size());

		// Depth Pass
		if (s_Data->m_DirLight.IsActive && s_Data->m_DirLight.IsCastShadows)
		{
#ifndef FROSTIUM_OPENGL_IMPL
			VkCommandBuffer cmdBuffer = s_Data->p_DepthPass.GetVkCommandBuffer();
#endif
			s_Data->p_DepthPass.BeginRenderPass();
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
				for (uint32_t i = 0; i < cmdCount; ++i)
				{
					auto& cmd = s_Data->m_DrawList[i];
					pc.offset = cmd.Offset;

					s_Data->p_DepthPass.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &pc);
					s_Data->p_DepthPass.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
				}
			}
			s_Data->p_DepthPass.EndRenderPass();
		}

		// G-Buffer Pass
		s_Data->p_Gbuffer.BeginRenderPass();
		{
			// SkyBox
			if (s_Data->m_State.bDrawSkyBox)
			{
				uint32_t state = s_Data->m_EnvironmentMap->IsDynamic();
				s_Data->p_Skybox.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
				s_Data->p_Skybox.Draw(36);
			}

			// Grid
			if (s_Data->m_State.bDrawGrid)
			{
				s_Data->p_Grid.SubmitPushConstant(ShaderType::Vertex, sizeof(glm::mat4), &s_Data->m_GridModel);
				s_Data->p_Grid.DrawMeshIndexed(&s_Data->m_PlaneMesh);
			}

			for (uint32_t i = 0; i < cmdCount; ++i)
			{
				auto& cmd = s_Data->m_DrawList[i];

				s_Data->m_MainPushConstant.DataOffset = cmd.Offset;
				s_Data->p_Gbuffer.SubmitPushConstant(ShaderType::Vertex, s_Data->m_PushConstantSize, &s_Data->m_MainPushConstant);
				s_Data->p_Gbuffer.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
			}
		}
		s_Data->p_Gbuffer.EndRenderPass();

		// Debug View
		if (s_Data->m_State.eDebugView != DebugViewFlags::None)
		{
			s_Data->p_Debug.BeginRenderPass();
			{
				uint32_t state = (uint32_t)s_Data->m_State.eDebugView;
				s_Data->p_Debug.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
				s_Data->p_Debug.Draw(3);
			}
			s_Data->p_Debug.EndRenderPass();

			return;
		}

		// Lighting Pass
		{
			s_Data->p_Lighting.BeginRenderPass();
			{
				struct push_constant
				{
					uint32_t numPointLights;
					uint32_t numSpotLights;
				} pc;

				pc.numPointLights = s_Data->m_PointLightIndex;
				pc.numSpotLights = s_Data->m_SpotLightIndex;
				s_Data->p_Lighting.SubmitPushConstant(ShaderType::Fragment, sizeof(push_constant), &pc);
				s_Data->p_Lighting.Draw(3);
			}
			s_Data->p_Lighting.EndRenderPass();
		}

		// Post-Processing: FXAA, Bloom
		{
			// Bloom
			{
				if (s_Data->m_State.bBloom)
				{
					s_Data->p_Bloom.BeginRenderPass();
					{
						s_Data->p_Bloom.Draw(3);
					}
					s_Data->p_Bloom.EndRenderPass();
				}
			}

			// FXAA + Composition
			{
				if (s_Data->m_DirtMask.Mask != nullptr)
				{
					s_Data->p_Combination.UpdateSampler(s_Data->m_DirtMask.Mask, 2);
				}

				s_Data->p_Combination.BeginRenderPass();
				{
					struct push_constant
					{
						uint32_t state;
						uint32_t enabledFXAA;
						uint32_t enabledMask;
						float maskIntensity;
						float maskBaseIntensity;
					} pc;

					// temp
					pc.state = s_Data->m_State.bBloom ? 1 : 0;
					pc.enabledMask = s_Data->m_DirtMask.Mask != nullptr;
					pc.enabledFXAA = s_Data->m_State.bFXAA;
					pc.maskIntensity = s_Data->m_DirtMask.Intensity;
					pc.maskBaseIntensity = s_Data->m_DirtMask.BaseIntensity;

					s_Data->p_Combination.SubmitPushConstant(ShaderType::Fragment, sizeof(push_constant), &pc);
					s_Data->p_Combination.Draw(3);
				}
				s_Data->p_Combination.EndRenderPass();
			}
		}
	}

	void DeferredRenderer::BuildDrawList()
	{
		JobsSystemInstance::BeginSubmition();
		{
			if (s_Data->m_UsedMeshes.size() > s_Data->m_DrawList.size())
			{
				s_Data->m_DrawList.resize(s_Data->m_UsedMeshes.size());
			}

			uint32_t cmdCount = static_cast<uint32_t>(s_Data->m_UsedMeshes.size());
			for (uint32_t i = 0; i < cmdCount; ++i)
			{
				// Getting values
				Mesh* mesh = s_Data->m_UsedMeshes[i];
				InstancePackage& instance = s_Data->m_Packages[mesh];
				CommandBuffer& cmd = s_Data->m_DrawList[i];

				// Setting draw list command
				cmd.Offset = s_Data->m_InstanceDataIndex;
				cmd.Mesh = mesh;
				cmd.InstancesCount = instance.CurrentIndex;

				for (uint32_t x = 0; x < instance.CurrentIndex; ++x)
				{
					InstancePackage::Package& package = instance.Packages[x];
					InstanceData& instanceUBO = s_Data->m_InstancesData[s_Data->m_InstanceDataIndex];

					const bool is_animated = package.AnimController != nullptr;
					uint32_t   anim_offset = s_Data->m_LastAnimationOffset;

					// Animations
					if (is_animated)
					{
						if (mesh->IsRootNode())
						{
							package.AnimController->Update();
							const auto& joints = *package.AnimController->GetJoints();
							for (uint32_t x = 0; x < static_cast<uint32_t>(joints.size()); ++x)
							{
								s_Data->m_AnimationJoints[s_Data->m_LastAnimationOffset] = joints[x];
								s_Data->m_LastAnimationOffset++;
							}

							s_Data->m_RootOffsets[mesh] = anim_offset;
						}
						else
						{
							auto& it = s_Data->m_RootOffsets.find(mesh->m_Root);
							if (it != s_Data->m_RootOffsets.end())
								anim_offset = it->second;
						}
					}

					// Transform
					{
						JobsSystemInstance::Schedule([is_animated, anim_offset, &package, &instanceUBO]()
						{
							Utils::ComposeTransform(*package.WorldPos, *package.Rotation, *package.Scale, instanceUBO.ModelView);

							instanceUBO.MaterialID = package.MaterialID;
							instanceUBO.IsAnimated = is_animated;
							instanceUBO.AnimOffset = anim_offset;
							instanceUBO.EntityID = 0; // temp
						});
					}

					s_Data->m_InstanceDataIndex++;
				}

				instance.CurrentIndex = 0;
			}
		}
		JobsSystemInstance::EndSubmition();
	}

	void DeferredRenderer::UpdateUniforms()
	{
		JobsSystemInstance::BeginSubmition();
		{
			// Updates Scene Data
			JobsSystemInstance::Schedule([]()
			{
				s_Data->p_Gbuffer.SubmitBuffer(s_Data->m_SceneDataBinding, sizeof(SceneData), s_Data->m_SceneData);
			});

			// Updates Lighting State
			JobsSystemInstance::Schedule([]()
			{
				s_Data->p_Lighting.SubmitBuffer(s_Data->m_LightingStateBinding, sizeof(IBLProperties), &s_Data->m_State.IBL);
			});

			// Updates FXAA State
			JobsSystemInstance::Schedule([]()
			{
				float width = 1.0f / static_cast<float>(s_Data->f_Main->GetSpecification().Width);
				float height = 1.0f / static_cast<float>(s_Data->f_Main->GetSpecification().Height);
				s_Data->m_State.FXAA.InverseScreenSize = glm::vec2(width, height);
				s_Data->p_Combination.SubmitBuffer(s_Data->m_FXAAStateBinding, sizeof(FXAAProperties), &s_Data->m_State.FXAA);
			});

			// Updates Bloom State
			JobsSystemInstance::Schedule([]()
			{
				s_Data->p_Lighting.SubmitBuffer(s_Data->m_BloomStateBinding, sizeof(BloomProperties), &s_Data->m_State.Bloom);
			});

			// Updates Directional Lights
			JobsSystemInstance::Schedule([]()
			{
				s_Data->p_Lighting.SubmitBuffer(s_Data->m_DirLightBinding, sizeof(DirectionalLight), &s_Data->m_DirLight);
				if (s_Data->m_DirLight.IsCastShadows)
				{
					// Calculate Depth
					CalculateDepthMVP(s_Data->m_MainPushConstant.DepthMVP);
				}
			});

			// Updates Point Lights
			JobsSystemInstance::Schedule([]()
			{
				if (s_Data->m_PointLightIndex > 0)
				{
					s_Data->p_Lighting.SubmitBuffer(s_Data->m_PointLightBinding,
						sizeof(PointLight) * s_Data->m_PointLightIndex, s_Data->m_PointLights.data());
				}
			});

			// Updates Spot Lights
			JobsSystemInstance::Schedule([]()
			{
				if (s_Data->m_SpotLightIndex > 0)
				{
					s_Data->p_Lighting.SubmitBuffer(s_Data->m_SpotLightBinding,
						sizeof(SpotLight) * s_Data->m_SpotLightIndex, s_Data->m_SpotLights.data());
				}
			});

			// Updates Animation joints
			JobsSystemInstance::Schedule([]()
			{
				if (s_Data->m_LastAnimationOffset > 0)
				{
					s_Data->p_Gbuffer.SubmitBuffer(s_Data->m_AnimBinding,
						sizeof(glm::mat4) * s_Data->m_LastAnimationOffset, s_Data->m_AnimationJoints.data());
				}
			});

			// Updates Batch Data
			JobsSystemInstance::Schedule([]()
			{
				if (s_Data->m_InstanceDataIndex > 0)
				{
					s_Data->p_Gbuffer.SubmitBuffer(s_Data->m_ShaderDataBinding,
						sizeof(InstanceData) * s_Data->m_InstanceDataIndex, s_Data->m_InstancesData.data());
				}
			});

		}
		JobsSystemInstance::EndSubmition();
	}

	void DeferredRenderer::StartNewBacth()
	{
		Flush();
		Reset();
	}

	void DeferredRenderer::SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh,
		const uint32_t& material_id, bool submit_childs, AnimationController* anim_controller)
	{
		if (!s_Data->m_Frustum->CheckSphere(pos, s_Data->m_State.FrustumRadius))
			return;

		if (s_Data->m_Objects >= max_objects)
			StartNewBacth();

		auto& instance = s_Data->m_Packages[mesh];
		InstancePackage::Package* package = nullptr;

		if (instance.CurrentIndex == instance.Packages.size())
		{
			instance.Packages.emplace_back(InstancePackage::Package());
			package = &instance.Packages.back();
		}
		else { package = &instance.Packages[instance.CurrentIndex]; }

		package->MaterialID = material_id;
		package->WorldPos = const_cast<glm::vec3*>(&pos);
		package->Rotation = const_cast<glm::vec3*>(&rotation);
		package->Scale = const_cast<glm::vec3*>(&scale);
		package->AnimController = anim_controller;

		s_Data->m_Objects++;
		instance.CurrentIndex++;
		bool found = std::find(s_Data->m_UsedMeshes.begin(), s_Data->m_UsedMeshes.end(), mesh) != s_Data->m_UsedMeshes.end();
		if (found == false) { s_Data->m_UsedMeshes.emplace_back(mesh); }

		if (submit_childs)
		{
			for (auto& sub : mesh->m_Childs)
				SubmitMesh(pos, rotation, scale, &sub, material_id, submit_childs, anim_controller);
		}
	}

	void DeferredRenderer::SubmitDirLight(DirectionalLight* light)
	{
		assert(light != nullptr);
		s_Data->m_DirLight = *light;
	}

	void DeferredRenderer::SubmitPointLight(PointLight* light)
	{
		uint32_t index = s_Data->m_PointLightIndex;
		if (index >= max_lights || light == nullptr)
			return;

		s_Data->m_PointLights[index] = *light;
		s_Data->m_PointLightIndex++;
	}

	void DeferredRenderer::SubmitSpotLight(SpotLight* light)
	{
		uint32_t index = s_Data->m_SpotLightIndex;
		if (index >= max_lights || light == nullptr)
			return;

		s_Data->m_SpotLights[index] = *light;
		s_Data->m_SpotLightIndex++;
	}

	void DeferredRenderer::SetDirtMask(Texture* mask, float intensity, float baseIntensity)
	{
		s_Data->m_DirtMask.Mask = mask;
		s_Data->m_DirtMask.Intensity = intensity;
		s_Data->m_DirtMask.BaseIntensity = baseIntensity;
	}

	void DeferredRenderer::SetEnvironmentCube(CubeMap* cube)
	{
		s_Data->m_EnvironmentMap->GenerateStatic(cube);
		auto map = s_Data->m_EnvironmentMap->GetCubeMap();
		s_Data->m_VulkanPBR->GeneratePBRCubeMaps(map);

		// Update Descriptors
		s_Data->p_Lighting.UpdateVulkanImageDescriptor(2, s_Data->m_VulkanPBR->GetIrradianceImageInfo());
		s_Data->p_Lighting.UpdateVulkanImageDescriptor(3, s_Data->m_VulkanPBR->GetBRDFLUTImageInfo());
		s_Data->p_Lighting.UpdateVulkanImageDescriptor(4, s_Data->m_VulkanPBR->GetPrefilteredCubeImageInfo());
		s_Data->p_Skybox.UpdateVulkanImageDescriptor(1, map->GetTexture()->GetVulkanTexture()->GetVkDescriptorImageInfo());
	}

	void DeferredRenderer::SetDynamicSkyboxProperties(DynamicSkyProperties& properties, bool regeneratePBRmaps)
	{
		auto& ref = s_Data->m_EnvironmentMap->GetDynamicSkyProperties();
		ref = properties;

		if (regeneratePBRmaps)
		{
			auto& sceneData = GraphicsContext::GetSingleton()->m_SceneData;

			s_Data->m_EnvironmentMap->GenerateDynamic(sceneData.Projection);
			auto map = s_Data->m_EnvironmentMap->GetCubeMap();
			s_Data->m_VulkanPBR->GeneratePBRCubeMaps(map);

			// Update Descriptors
			s_Data->p_Lighting.UpdateVulkanImageDescriptor(2, s_Data->m_VulkanPBR->GetIrradianceImageInfo());
			s_Data->p_Lighting.UpdateVulkanImageDescriptor(3, s_Data->m_VulkanPBR->GetBRDFLUTImageInfo());
			s_Data->p_Lighting.UpdateVulkanImageDescriptor(4, s_Data->m_VulkanPBR->GetPrefilteredCubeImageInfo());
			s_Data->p_Skybox.UpdateVulkanImageDescriptor(1, map->GetTexture()->GetVulkanTexture()->GetVkDescriptorImageInfo());
		}
		else
			s_Data->m_EnvironmentMap->UpdateDescriptors();
	}

	void DeferredRenderer::SetDebugView(DebugViewFlags flag)
	{
		s_Data->m_State.eDebugView = flag;
	}

	void DeferredRenderer::SetGridActive(bool active)
	{
		s_Data->m_State.bDrawGrid = active;
	}

	void DeferredRenderer::SetBloomActive(bool active)
	{
		s_Data->m_State.bBloom = active;
	}

	void DeferredRenderer::SetFxaaActive(bool active)
	{
		s_Data->m_State.bFXAA = active;
	}

	void DeferredRenderer::SetIblActive(bool active)
	{
		s_Data->m_State.bIBL = active;
		s_Data->m_State.IBL.UseIBL = active;
	}

	void DeferredRenderer::SetFrustumRadius(float radius)
	{
		s_Data->m_State.FrustumRadius = radius;
	}

	void DeferredRenderer::SetIBLProperties(IBLProperties& properties)
	{
		s_Data->m_State.IBL = properties;
	}

	void DeferredRenderer::SetBloomProperties(BloomProperties& properties)
	{
		s_Data->m_State.Bloom = properties;
	}

	void DeferredRenderer::SetFxaaProperties(FXAAProperties& properties)
	{
		s_Data->m_State.FXAA = properties;
	}

	void DeferredRenderer::InitPBR()
	{
		s_Data->m_VulkanPBR = new VulkanPBR();
		s_Data->m_EnvironmentMap = std::make_shared<EnvironmentMap>();
		auto& config = s_Data->m_EnvironmentMap->GetDynamicSkyProperties();
		config.SunPosition = glm::vec4(1, -11, 0, 0);
		config.NumCirrusCloudsIterations = 0;

		s_Data->m_EnvironmentMap->Initialize();
		s_Data->m_EnvironmentMap->GenerateDynamic();

		auto map = s_Data->m_EnvironmentMap->GetCubeMap();
		s_Data->m_VulkanPBR->GeneratePBRCubeMaps(map);
	}

	void DeferredRenderer::InitPipelines()
	{
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

		// Gbuffer
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Gbuffer.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Gbuffer.frag";

				// SSBO's
				ShaderBufferInfo bufferInfo = {};

				// Vertex
				bufferInfo.Size = sizeof(InstanceData) * max_objects;
				shaderCI.BufferInfos[s_Data->m_ShaderDataBinding] = bufferInfo;

				bufferInfo.Size = sizeof(PBRMaterial) * max_materials;
				shaderCI.BufferInfos[s_Data->m_MaterialsBinding] = bufferInfo;

				bufferInfo.Size = sizeof(glm::mat4) * max_anim_joints;
				shaderCI.BufferInfos[s_Data->m_AnimBinding] = bufferInfo;
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { vertexMain };
				DynamicPipelineCI.PipelineName = "G_Pipeline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.TargetFramebuffers = { &s_Data->f_GBuffer };
			}

			auto result = s_Data->p_Gbuffer.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);
		}

		// Lighting
		{
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/GenTriangle.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Lighting.frag";

				ShaderBufferInfo bufferInfo = {};

				// Fragment
				bufferInfo.Size = sizeof(PointLight) * max_lights;
				shaderCI.BufferInfos[s_Data->m_PointLightBinding] = bufferInfo;

				bufferInfo.Size = sizeof(SpotLight) * max_lights;
				shaderCI.BufferInfos[s_Data->m_SpotLightBinding] = bufferInfo;

				bufferInfo.Size = sizeof(DirectionalLight);
				shaderCI.BufferInfos[s_Data->m_DirLightBinding] = bufferInfo;
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.PipelineName = "Lighting_Pipeline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.bDepthTestEnabled = false;
				DynamicPipelineCI.TargetFramebuffers = { &s_Data->f_Lighting };
			}

			auto result = s_Data->p_Lighting.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);

			s_Data->p_Lighting.UpdateSampler(&s_Data->f_Depth, 1, "Depth_Attachment");
#ifndef FROSTIUM_OPENGL_IMPL
			s_Data->p_Lighting.UpdateVulkanImageDescriptor(2, s_Data->m_VulkanPBR->GetIrradianceImageInfo());
			s_Data->p_Lighting.UpdateVulkanImageDescriptor(3, s_Data->m_VulkanPBR->GetBRDFLUTImageInfo());
			s_Data->p_Lighting.UpdateVulkanImageDescriptor(4, s_Data->m_VulkanPBR->GetPrefilteredCubeImageInfo());
#endif
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 5, "albedro");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 6, "position");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 7, "normals");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 8, "materials");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 9, "shadow_coord");
		}

		JobsSystemInstance::BeginSubmition();
		{
			// Grid
			JobsSystemInstance::Schedule([&]()
			{
				Utils::ComposeTransform(glm::vec3(0), glm::vec3(0), { 100, 1, 100 }, s_Data->m_GridModel);
				Mesh::Create(s_Data->m_Path + "Models/plane_v2.gltf", &s_Data->m_PlaneMesh);

				GraphicsPipelineCreateInfo pipelineCI = {};
				GraphicsPipelineShaderCreateInfo shaderCI = {};
				{
					shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/Grid.vert";
					shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Grid.frag";
				};

				pipelineCI.PipelineName = "Grid";
				pipelineCI.eCullMode = CullMode::None;
				pipelineCI.VertexInputInfos = { vertexMain };
				pipelineCI.bDepthTestEnabled = false;
				pipelineCI.bDepthWriteEnabled = false;
				pipelineCI.TargetFramebuffers = { &s_Data->f_GBuffer };
				pipelineCI.ShaderCreateInfo = shaderCI;
				assert(s_Data->p_Grid.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
			});

			// Skybox
			JobsSystemInstance::Schedule([&]()
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
					DynamicPipelineCI.ShaderCreateInfo = shaderCI;
					DynamicPipelineCI.bDepthTestEnabled = false;
					DynamicPipelineCI.bDepthWriteEnabled = false;
					DynamicPipelineCI.TargetFramebuffers = { &s_Data->f_GBuffer };
				}

				auto result = s_Data->p_Skybox.Create(&DynamicPipelineCI);
				assert(result == PipelineCreateResult::SUCCESS);
#ifndef FROSTIUM_OPENGL_IMPL
				auto map = s_Data->m_EnvironmentMap->GetCubeMap();
				s_Data->p_Skybox.UpdateVulkanImageDescriptor(1,
					map->GetTexture()->GetVulkanTexture()->GetVkDescriptorImageInfo());
#endif
				Ref<VertexBuffer> skyBoxFB = std::make_shared<VertexBuffer>();
				VertexBuffer::Create(skyBoxFB.get(), skyboxVertices, sizeof(skyboxVertices));
				s_Data->p_Skybox.SetVertexBuffers({ skyBoxFB });
			});


			// Depth Pass
			JobsSystemInstance::Schedule([&]()
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
					DynamicPipelineCI.ShaderCreateInfo = shaderCI;
					DynamicPipelineCI.TargetFramebuffers = { &s_Data->f_Depth };
					DynamicPipelineCI.bDepthBiasEnabled = true;
					DynamicPipelineCI.StageCount = 1;

					auto result = s_Data->p_DepthPass.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);
				}
			});

			// Combination
			JobsSystemInstance::Schedule([&]()
			{
				GraphicsPipelineCreateInfo DynamicPipelineCI = {};
				DynamicPipelineCI.eCullMode = CullMode::None;
				DynamicPipelineCI.TargetFramebuffers = { s_Data->f_Main };

				GraphicsPipelineShaderCreateInfo shaderCI = {};
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/GenTriangle.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Combination.frag";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.PipelineName = "Combination";

				auto result = s_Data->p_Combination.Create(&DynamicPipelineCI);
				assert(result == PipelineCreateResult::SUCCESS);

				s_Data->p_Combination.UpdateSampler(&s_Data->f_Lighting, 0);
				s_Data->p_Combination.UpdateSampler(&s_Data->f_Bloom, 1);
			});

			// Debug
			JobsSystemInstance::Schedule([&]()
			{
				GraphicsPipelineCreateInfo DynamicPipelineCI = {};
				DynamicPipelineCI.eCullMode = CullMode::None;
				DynamicPipelineCI.TargetFramebuffers = { s_Data->f_Main };

				GraphicsPipelineShaderCreateInfo shaderCI = {};
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/GenTriangle.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/DebugView.frag";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.PipelineName = "Debug";

				auto result = s_Data->p_Debug.Create(&DynamicPipelineCI);
				assert(result == PipelineCreateResult::SUCCESS);

				s_Data->p_Debug.UpdateSampler(&s_Data->f_Depth, 1, "Depth_Attachment");
				s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 5, "albedro");
				s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 6, "position");
				s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 7, "normals");
				s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 8, "materials");
				s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 9, "shadow_coord");
			});


			// Bloom
			JobsSystemInstance::Schedule([&]()
			{
				GraphicsPipelineCreateInfo DynamicPipelineCI = {};
				DynamicPipelineCI.eCullMode = CullMode::None;

				GraphicsPipelineShaderCreateInfo shaderCI = {};
				shaderCI.FilePaths[ShaderType::Vertex] = s_Data->m_Path + "Shaders/GenTriangle.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = s_Data->m_Path + "Shaders/Bloom.frag";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.eColorBlendOp = BlendOp::ADD;
				DynamicPipelineCI.eSrcColorBlendFactor = BlendFactor::ONE;
				DynamicPipelineCI.eDstColorBlendFactor = BlendFactor::ONE;
				DynamicPipelineCI.eAlphaBlendOp = BlendOp::ADD;
				DynamicPipelineCI.eSrcAlphaBlendFactor = BlendFactor::SRC_ALPHA;
				DynamicPipelineCI.eDstAlphaBlendFactor = BlendFactor::DST_ALPHA;

				DynamicPipelineCI.TargetFramebuffers = { &s_Data->f_Bloom };
				DynamicPipelineCI.PipelineName = "Bloom";
				auto result = s_Data->p_Bloom.Create(&DynamicPipelineCI);
				assert(result == PipelineCreateResult::SUCCESS);
				s_Data->p_Bloom.UpdateSampler(&s_Data->f_Lighting, 0, "color_2");
			});

		}

		JobsSystemInstance::EndSubmition();
	}

	void DeferredRenderer::InitFramebuffers()
	{
		// Main
		s_Data->f_Main = GraphicsContext::GetSingleton()->GetFramebuffer();

		JobsSystemInstance::BeginSubmition();
		{
			JobsSystemInstance::Schedule([&]()
			{
				// Gbuffer
				{
					const bool ClearOp = true;
					FramebufferAttachment albedro = FramebufferAttachment(AttachmentFormat::Color, ClearOp, "albedro");
					FramebufferAttachment position = FramebufferAttachment(AttachmentFormat::SFloat4_16, ClearOp, "position");
					FramebufferAttachment normals = FramebufferAttachment(AttachmentFormat::SFloat4_16, ClearOp, "normals");
					FramebufferAttachment materials = FramebufferAttachment(AttachmentFormat::SFloat4_32, ClearOp, "materials");
					FramebufferAttachment shadow_coord = FramebufferAttachment(AttachmentFormat::SFloat4_32, ClearOp, "shadow_coord");

					FramebufferSpecification framebufferCI = {};
					framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
					framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { albedro, position, normals, materials, shadow_coord };

					Framebuffer::Create(framebufferCI, &s_Data->f_GBuffer);
				}

			});

			JobsSystemInstance::Schedule([&]()
			{
				// Lighting
				{
					FramebufferAttachment color = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_1");
					FramebufferAttachment bloom = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_2");

					FramebufferSpecification framebufferCI = {};
					framebufferCI.eFiltering = ImageFilter::LINEAR;
					framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
					framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { color, bloom };

					Framebuffer::Create(framebufferCI, &s_Data->f_Lighting);
				}

			});

			JobsSystemInstance::Schedule([&]()
			{
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

					Framebuffer::Create(framebufferCI, &s_Data->f_Depth);
				}

			});

			// Bloom
			JobsSystemInstance::Schedule([&]()
			{
				FramebufferAttachment color = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "color_1");

				FramebufferSpecification framebufferCI = {};
				framebufferCI.eFiltering = ImageFilter::LINEAR;
				framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
				framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
				framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
				framebufferCI.Attachments = { color };

				Framebuffer::Create(framebufferCI, &s_Data->f_Bloom);
			});
		}
		JobsSystemInstance::EndSubmition();
	}

	void DeferredRenderer::CalculateDepthMVP(glm::mat4& out_mvp)
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

	bool DeferredRenderer::UpdateMaterials()
	{
		if (!s_Data->m_IsInitialized)
			return false;

		std::vector<Texture*> tetxures;
		MaterialLibrary::GetSinglenton()->GetTextures(tetxures);
		s_Data->p_Gbuffer.UpdateSamplers(tetxures, s_Data->m_TexturesBinding);

		void* data = nullptr;
		uint32_t size = 0;
		MaterialLibrary::GetSinglenton()->GetMaterialsPtr(data, size);
		s_Data->p_Gbuffer.SubmitBuffer(s_Data->m_MaterialsBinding, size, data);
		return true;
	}

	bool DeferredRenderer::ResetStates()
	{
		s_Data->m_DirLight = {};
		s_Data->m_State = {};

		Reset();
		return true;
	}

	void DeferredRenderer::OnResize(uint32_t width, uint32_t height)
	{
		s_Data->f_Bloom.OnResize(width, height);
		s_Data->f_GBuffer.OnResize(width, height);
		s_Data->f_Lighting.OnResize(width, height);

		{
			// Lighting pipeline
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 5, "albedro");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 6, "position");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 7, "normals");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 8, "materials");
			s_Data->p_Lighting.UpdateSampler(&s_Data->f_GBuffer, 9, "shadow_coord");
			// Debug view pipeline
			s_Data->p_Debug.UpdateSampler(&s_Data->f_Depth, 1, "Depth_Attachment");
			s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 5, "albedro");
			s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 6, "position");
			s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 7, "normals");
			s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 8, "materials");
			s_Data->p_Debug.UpdateSampler(&s_Data->f_GBuffer, 9, "shadow_coord");

			// Combination
			s_Data->p_Combination.UpdateSampler(&s_Data->f_Lighting, 0);
			s_Data->p_Combination.UpdateSampler(&s_Data->f_Bloom, 1);
			// Bloon
			s_Data->p_Bloom.UpdateSampler(&s_Data->f_Lighting, 0, "color_2");
		}

	}

	void DeferredRenderer::Reset()
	{
		s_Data->m_Objects = 0;
		s_Data->m_InstanceDataIndex = 0;
		s_Data->m_PointLightIndex = 0;
		s_Data->m_SpotLightIndex = 0;
		s_Data->m_LastAnimationOffset = 0;
		s_Data->m_UsedMeshes.clear();
		s_Data->m_RootOffsets.clear();
	}

	void DeferredRenderer::Shutdown()
	{
		if (s_Data != nullptr)
			s_Data->m_IsInitialized = false;
	}

	RendererStateEX& DeferredRenderer::GetState()
	{
		return s_Data->m_State;
	}

	Framebuffer* DeferredRenderer::GetFramebuffer()
	{
		return s_Data->f_Main;
	}

	uint32_t DeferredRenderer::GetNumObjects()
	{
		return s_Data->m_Objects;
	}
}