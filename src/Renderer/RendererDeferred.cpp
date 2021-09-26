#include "stdafx.h"
#include "Renderer/RendererDeferred.h"
#include "Tools/MaterialLibrary.h"

#include "Common/Common.h"
#include "Tools/Utils.h"

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

namespace SmolEngine
{
	void RendererDeferred::DrawFrame(ClearInfo* clearInfo, RendererStorage* storage, RendererDrawList* drawList, bool batch_cmd)
	{
		CommandBufferStorage cmdStorage{};
		if (batch_cmd)
			cmdStorage.Buffer = VulkanContext::GetCurrentVkCmdBuffer();
		else
			VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);

		storage->p_Gbuffer.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_Lighting.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_Skybox.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_DepthPass.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_Debug.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_Grid.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_Combination.SetCommandBuffer(cmdStorage.Buffer);
		storage->p_DOF.SetCommandBuffer(cmdStorage.Buffer);

		SubmitInfo submitInfo{};
		submitInfo.pClearInfo = clearInfo;
		submitInfo.pDrawList = drawList;
		submitInfo.pStorage = storage;
		submitInfo.pCmdStorage = &cmdStorage;

		UpdateUniforms(&submitInfo);
		DepthPass(&submitInfo);
		GBufferPass(&submitInfo);

		if (DebugViewPass(&submitInfo)) { return; }

		LightingPass(&submitInfo);
		BloomPass(&submitInfo);
		CompositionPass(&submitInfo);

		if (!batch_cmd)
			VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);
	}

	void RendererStorage::Initilize()
	{
		CreatePBRMaps();
		CreateFramebuffers();
		CreatePipelines();
		OnUpdateMaterials();
	}

	void RendererStorage::SetRenderTarget(Framebuffer* target)
	{
		f_Main = target;
		p_Combination.SetFramebuffers({ f_Main });
		p_Debug.SetFramebuffers({ f_Main });
	}

	void RendererStorage::OnUpdateMaterials()
	{
		std::vector<Texture*> tetxures;
		m_MaterialLibrary.GetTextures(tetxures);
		p_Gbuffer.UpdateSamplers(tetxures, m_TexturesBinding);

		void* data = nullptr;
		uint32_t size = 0;
		m_MaterialLibrary.GetMaterialsPtr(data, size);
		p_Gbuffer.SubmitBuffer(m_MaterialsBinding, size, data);
	}

	void RendererStorage::SetDefaultState()
	{
		m_State = {};
	}

	void RendererStorage::CreatePipelines()
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
		const std::string& path = GraphicsContext::GetSingleton()->GetResourcesPath();

		// Gbuffer
		{
			ShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/Gbuffer.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Gbuffer.frag";

				// SSBO's
				ShaderBufferInfo bufferInfo = {};

				// Vertex
				bufferInfo.Size = sizeof(InstanceData) * max_objects;
				shaderCI.BufferInfos[m_ShaderDataBinding] = bufferInfo;

				bufferInfo.Size = sizeof(PBRMaterial) * max_materials;
				shaderCI.BufferInfos[m_MaterialsBinding] = bufferInfo;

				bufferInfo.Size = sizeof(glm::mat4) * max_anim_joints;
				shaderCI.BufferInfos[m_AnimBinding] = bufferInfo;
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { vertexMain };
				DynamicPipelineCI.PipelineName = "G_Pipeline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.TargetFramebuffers = { &f_GBuffer };
			}

			auto result = p_Gbuffer.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);
		}

		// Lighting
		{
			ShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Lighting.frag";

				ShaderBufferInfo bufferInfo = {};

				// Fragment
				bufferInfo.Size = sizeof(PointLight) * max_lights;
				shaderCI.BufferInfos[m_PointLightBinding] = bufferInfo;

				bufferInfo.Size = sizeof(SpotLight) * max_lights;
				shaderCI.BufferInfos[m_SpotLightBinding] = bufferInfo;

				bufferInfo.Size = sizeof(DirectionalLight);
				shaderCI.BufferInfos[m_DirLightBinding] = bufferInfo;
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.PipelineName = "Lighting_Pipeline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.bDepthTestEnabled = false;
				DynamicPipelineCI.TargetFramebuffers = { &f_Lighting };
			}

			auto result = p_Lighting.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);

			p_Lighting.UpdateSampler(&f_Depth, 1, "Depth_Attachment");
#ifndef FROSTIUM_OPENGL_IMPL
			p_Lighting.UpdateVulkanImageDescriptor(2, m_VulkanPBR->GetIrradianceImageInfo());
			p_Lighting.UpdateVulkanImageDescriptor(3, m_VulkanPBR->GetBRDFLUTImageInfo());
			p_Lighting.UpdateVulkanImageDescriptor(4, m_VulkanPBR->GetPrefilteredCubeImageInfo());
#endif
			p_Lighting.UpdateSampler(&f_GBuffer, 5, "albedro");
			p_Lighting.UpdateSampler(&f_GBuffer, 6, "position");
			p_Lighting.UpdateSampler(&f_GBuffer, 7, "normals");
			p_Lighting.UpdateSampler(&f_GBuffer, 8, "materials");
		}

		JobsSystemInstance::BeginSubmition();
		{
			// Grid
			JobsSystemInstance::Schedule([&]()
				{
					Utils::ComposeTransform(glm::vec3(0), glm::vec3(0), { 100, 1, 100 }, m_GridModel);
					Mesh::Create(path + "Models/plane_v2.gltf", &m_PlaneMesh);

					GraphicsPipelineCreateInfo pipelineCI = {};
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/Grid.vert";
						shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Grid.frag";
					};

					pipelineCI.PipelineName = "Grid";
					pipelineCI.eCullMode = CullMode::None;
					pipelineCI.VertexInputInfos = { vertexMain };
					pipelineCI.bDepthTestEnabled = false;
					pipelineCI.bDepthWriteEnabled = false;
					pipelineCI.TargetFramebuffers = { &f_GBuffer };
					pipelineCI.ShaderCreateInfo = shaderCI;
					assert(p_Grid.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
				});

			// Skybox
			JobsSystemInstance::Schedule([&]()
				{
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/Skybox.vert";
						shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Skybox.frag";
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
						DynamicPipelineCI.TargetFramebuffers = { &f_GBuffer };
					}

					auto result = p_Skybox.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);
#ifndef FROSTIUM_OPENGL_IMPL
					auto map = m_EnvironmentMap->GetCubeMap();
					p_Skybox.UpdateVulkanImageDescriptor(1,
						map->GetTexture()->GetVulkanTexture()->GetVkDescriptorImageInfo());
#endif
					Ref<VertexBuffer> skyBoxFB = std::make_shared<VertexBuffer>();
					VertexBuffer::Create(skyBoxFB.get(), skyboxVertices, sizeof(skyboxVertices));
					p_Skybox.SetVertexBuffers({ skyBoxFB });
				});


			// Depth Pass
			JobsSystemInstance::Schedule([&]()
				{
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/DepthPass.vert";
						shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/DepthPass.frag";
					};

					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					{
						DynamicPipelineCI.VertexInputInfos = { vertexMain };
						DynamicPipelineCI.PipelineName = "DepthPass_Pipeline";
						DynamicPipelineCI.ShaderCreateInfo = shaderCI;
						DynamicPipelineCI.TargetFramebuffers = { &f_Depth };
						DynamicPipelineCI.bDepthBiasEnabled = true;
						DynamicPipelineCI.StageCount = 1;

						auto result = p_DepthPass.Create(&DynamicPipelineCI);
						assert(result == PipelineCreateResult::SUCCESS);
					}
				});

			// DOF
			//JobsSystemInstance::Schedule([&]()
			//	{
			//		GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			//		DynamicPipelineCI.eCullMode = CullMode::None;
			//		DynamicPipelineCI.TargetFramebuffers = { &f_DOF };
			//
			//		ShaderCreateInfo shaderCI = {};
			//		shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
			//		shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/DOF.frag";
			//		DynamicPipelineCI.ShaderCreateInfo = shaderCI;
			//		DynamicPipelineCI.PipelineName = "DOF";
			//
			//		auto result = p_DOF.Create(&DynamicPipelineCI);
			//		assert(result == PipelineCreateResult::SUCCESS);
			//
			//		p_DOF.UpdateSampler(&f_Lighting, 0);
			//		p_DOF.UpdateSampler(&f_GBuffer, 1, "position");
			//	});

			// Combination
			JobsSystemInstance::Schedule([&]()
				{
					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					DynamicPipelineCI.eCullMode = CullMode::None;
					DynamicPipelineCI.TargetFramebuffers = { f_Main };

					ShaderCreateInfo shaderCI = {};
					shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
					shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Combination.frag";
					DynamicPipelineCI.ShaderCreateInfo = shaderCI;
					DynamicPipelineCI.PipelineName = "Combination";

					auto result = p_Combination.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);

					p_Combination.UpdateSampler(&f_Lighting, 0);
				});

			// Debug
			JobsSystemInstance::Schedule([&]()
				{
					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					DynamicPipelineCI.eCullMode = CullMode::None;
					DynamicPipelineCI.TargetFramebuffers = { f_Main };

					ShaderCreateInfo shaderCI = {};
					shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
					shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/DebugView.frag";
					DynamicPipelineCI.ShaderCreateInfo = shaderCI;
					DynamicPipelineCI.PipelineName = "Debug";

					auto result = p_Debug.Create(&DynamicPipelineCI);
					assert(result == PipelineCreateResult::SUCCESS);

					p_Debug.UpdateSampler(&f_Depth, 1, "Depth_Attachment");
					p_Debug.UpdateSampler(&f_GBuffer, 5, "albedro");
					p_Debug.UpdateSampler(&f_GBuffer, 6, "position");
					p_Debug.UpdateSampler(&f_GBuffer, 7, "normals");
					p_Debug.UpdateSampler(&f_GBuffer, 8, "materials");
				});

			//Bloom
			JobsSystemInstance::Schedule([&]()
				{
					ComputePipelineCreateInfo compCI = {};
					compCI.ShaderPath = path + "Shaders/Bloom.comp";
					compCI.DescriptorCount = 24;
					p_Bloom.Create(&compCI);

					auto& spec = f_Main->GetSpecification();
					uint32_t workgroupSize = 4;
					uint32_t viewportWidth = spec.Width / 2;
					uint32_t viewportHeight = spec.Height / 2;

					viewportWidth += (workgroupSize - (viewportWidth % workgroupSize));
					viewportHeight += (workgroupSize - (viewportHeight % workgroupSize));

					TextureCreateInfo TexCI{};
					TexCI.eFormat = TextureFormat::R32G32B32A32_SFLOAT;
					TexCI.eAddressMode = AddressMode::CLAMP_TO_EDGE;
					TexCI.bIsStorage = true;
					TexCI.bAnisotropyEnable = false;

					m_BloomTex.resize(3);
					for(auto& tex: m_BloomTex)
						tex.GetVulkanTexture()->GenTexture(viewportWidth, viewportHeight, &TexCI);
				});

		}

		JobsSystemInstance::EndSubmition();
	}

	void RendererStorage::CreateFramebuffers()
	{
		// Main
		f_Main = GraphicsContext::GetSingleton()->GetFramebuffer();

		JobsSystemInstance::BeginSubmition();
		{
			// Gbuffer
			JobsSystemInstance::Schedule([&]()
				{
					const bool ClearOp = true;
					FramebufferAttachment albedro = FramebufferAttachment(AttachmentFormat::Color, ClearOp, "albedro");
					FramebufferAttachment position = FramebufferAttachment(AttachmentFormat::SFloat4_32, ClearOp, "position");
					FramebufferAttachment normals = FramebufferAttachment(AttachmentFormat::SFloat4_16, ClearOp, "normals");
					FramebufferAttachment materials = FramebufferAttachment(AttachmentFormat::SFloat4_32, ClearOp, "materials");

					FramebufferSpecification framebufferCI = {};
					framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
					framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { albedro, position, normals, materials };

					Framebuffer::Create(framebufferCI, &f_GBuffer);

				});

			// Lighting
			JobsSystemInstance::Schedule([&]()
				{
					FramebufferAttachment color = FramebufferAttachment(AttachmentFormat::SFloat4_16, true, "color_1");

					FramebufferSpecification framebufferCI = {};
					framebufferCI.eFiltering = ImageFilter::LINEAR;
					framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
					framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { color };

					Framebuffer::Create(framebufferCI, &f_Lighting);

				});

			// DOF
			//JobsSystemInstance::Schedule([&]()
			//	{
			//		FramebufferAttachment color = FramebufferAttachment(AttachmentFormat::Color, true, "color_1");
			//
			//		FramebufferSpecification framebufferCI = {};
			//		framebufferCI.Width = GraphicsContext::GetSingleton()->GetWindowData()->Width;
			//		framebufferCI.Height = GraphicsContext::GetSingleton()->GetWindowData()->Height;
			//		framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
			//		framebufferCI.Attachments = { color };
			//
			//		Framebuffer::Create(framebufferCI, &f_DOF);
			//	});

			// Depth
			JobsSystemInstance::Schedule([&]()
				{
					FramebufferSpecification framebufferCI = {};
					uint32_t size = 8192;
					switch (m_MapSize)
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

					Framebuffer::Create(framebufferCI, &f_Depth);

				});
		}
		JobsSystemInstance::EndSubmition();
	}

	void RendererStorage::CreatePBRMaps()
	{
		m_VulkanPBR = new VulkanPBR();
		m_EnvironmentMap = std::make_shared<EnvironmentMap>();
		auto& config = m_EnvironmentMap->GetDynamicSkyProperties();
		config.SunPosition = glm::vec4(1, -11, 0, 0);
		config.NumCirrusCloudsIterations = 0;

		m_EnvironmentMap->Initialize();
		m_EnvironmentMap->GenerateDynamic();

		auto map = m_EnvironmentMap->GetCubeMap();
		m_VulkanPBR->GeneratePBRCubeMaps(map);
	}

	void RendererDrawList::BuildDrawList()
	{
		JobsSystemInstance::BeginSubmition();
		{
			if (m_UsedMeshes.size() > m_DrawList.size()) { m_DrawList.resize(m_UsedMeshes.size()); }

			uint32_t cmdCount = static_cast<uint32_t>(m_UsedMeshes.size());
			for (uint32_t i = 0; i < cmdCount; ++i)
			{
				// Getting values
				Mesh* mesh = m_UsedMeshes[i];
				InstancePackage& instance = m_Packages[mesh];
				CommandBuffer& cmd = m_DrawList[i];

				// Setting draw list command
				cmd.Offset = m_InstanceDataIndex;
				cmd.Mesh = mesh;
				cmd.InstancesCount = instance.CurrentIndex;

				for (uint32_t x = 0; x < instance.CurrentIndex; ++x)
				{
					InstancePackage::Package& package = instance.Packages[x];
					InstanceData& instanceUBO = m_InstancesData[m_InstanceDataIndex];

					const bool is_animated = package.AnimController != nullptr;
					uint32_t   anim_offset = m_LastAnimationOffset;

					// Animations
					if (is_animated)
					{
						if (mesh->IsRootNode())
						{
							package.AnimController->Update();
							package.AnimController->CopyJoints(m_AnimationJoints, m_LastAnimationOffset);
							m_RootOffsets[mesh] = anim_offset;
						}
						else
						{
							auto& it = m_RootOffsets.find(mesh->m_Root);
							if (it != m_RootOffsets.end())
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

					m_InstanceDataIndex++;
				}

				instance.CurrentIndex = 0;
			}
		}
		JobsSystemInstance::EndSubmition();
	}

	void RendererDrawList::ResetDrawList()
	{
		m_Objects = 0;
		m_InstanceDataIndex = 0;
		m_PointLightIndex = 0;
		m_SpotLightIndex = 0;
		m_LastAnimationOffset = 0;
		m_UsedMeshes.clear();
		m_RootOffsets.clear();
	}

	Frustum& RendererDrawList::GetFrustum()
	{
		return m_Frustum;
	}

	void RendererStorage::UpdateUniforms(RendererDrawList* drawList, Framebuffer* target)
	{
		JobsSystemInstance::BeginSubmition();
		{
			// Updates Scene Data
			JobsSystemInstance::Schedule([&]()
				{
					p_Gbuffer.SubmitBuffer(m_SceneDataBinding, sizeof(SceneViewProjection), drawList->m_SceneInfo);
				});

			// Updates Lighting State
			JobsSystemInstance::Schedule([&]()
				{
					p_Lighting.SubmitBuffer(m_LightingStateBinding, sizeof(IBLProperties), &m_State.IBL);
				});

			// Updates Bloom State
			JobsSystemInstance::Schedule([&]()
				{
					p_Lighting.SubmitBuffer(m_BloomStateBinding, sizeof(BloomProperties), &m_State.Bloom);
				});

			// Updates FXAA State
			JobsSystemInstance::Schedule([&]()
				{
					float width = 1.0f / static_cast<float>(target->GetSpecification().Width);
					float height = 1.0f / static_cast<float>(target->GetSpecification().Height);
					m_State.FXAA.InverseScreenSize = glm::vec2(width, height);
					p_Combination.SubmitBuffer(m_FXAAStateBinding, sizeof(FXAAProperties), &m_State.FXAA);
				});

			// Updates Directional Lights
			JobsSystemInstance::Schedule([&]()
				{
					p_Lighting.SubmitBuffer(m_DirLightBinding, sizeof(DirectionalLight), &drawList->m_DirLight);
				});

			// Updates Point Lights
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_PointLightIndex > 0)
					{
						p_Lighting.SubmitBuffer(m_PointLightBinding,
							sizeof(PointLight) * drawList->m_PointLightIndex, drawList->m_PointLights.data());
					}
				});

			// Updates Spot Lights
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_SpotLightIndex > 0)
					{
						p_Lighting.SubmitBuffer(m_SpotLightBinding,
							sizeof(SpotLight) * drawList->m_SpotLightIndex, drawList->m_SpotLights.data());
					}
				});

			// Updates Animation joints
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_LastAnimationOffset > 0)
					{
						p_Gbuffer.SubmitBuffer(m_AnimBinding,
							sizeof(glm::mat4) * drawList->m_LastAnimationOffset, drawList->m_AnimationJoints.data());
					}
				});

			// Updates Batch Data
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_InstanceDataIndex > 0)
					{
						p_Gbuffer.SubmitBuffer(m_ShaderDataBinding,
							sizeof(InstanceData) * drawList->m_InstanceDataIndex, drawList->m_InstancesData.data());
					}
				});

		}
		JobsSystemInstance::EndSubmition();
	}

	void RendererStorage::SetDynamicSkybox(DynamicSkyProperties& properties, const glm::mat4& proj, bool regeneratePBRmaps)
	{
		auto& ref = m_EnvironmentMap->GetDynamicSkyProperties();
		ref = properties;

		if (regeneratePBRmaps)
		{
			m_EnvironmentMap->GenerateDynamic(proj);

			CubeMap* cubeMap = m_EnvironmentMap->GetCubeMap();
			m_VulkanPBR->GeneratePBRCubeMaps(cubeMap);

			// Update Descriptors
			p_Lighting.UpdateVulkanImageDescriptor(2, m_VulkanPBR->GetIrradianceImageInfo());
			p_Lighting.UpdateVulkanImageDescriptor(3, m_VulkanPBR->GetBRDFLUTImageInfo());
			p_Lighting.UpdateVulkanImageDescriptor(4, m_VulkanPBR->GetPrefilteredCubeImageInfo());
			p_Skybox.UpdateVulkanImageDescriptor(1, cubeMap->GetTexture()->GetVulkanTexture()->GetVkDescriptorImageInfo());
		}
		else
			m_EnvironmentMap->UpdateDescriptors();
	}

	void RendererStorage::SetStaticSkybox(CubeMap* cube)
	{
		m_EnvironmentMap->GenerateStatic(cube);
		CubeMap* cubeMap = m_EnvironmentMap->GetCubeMap();
		m_VulkanPBR->GeneratePBRCubeMaps(cubeMap);

		// Update Descriptors
		p_Lighting.UpdateVulkanImageDescriptor(2, m_VulkanPBR->GetIrradianceImageInfo());
		p_Lighting.UpdateVulkanImageDescriptor(3, m_VulkanPBR->GetBRDFLUTImageInfo());
		p_Lighting.UpdateVulkanImageDescriptor(4, m_VulkanPBR->GetPrefilteredCubeImageInfo());
		p_Skybox.UpdateVulkanImageDescriptor(1, cubeMap->GetTexture()->GetVulkanTexture()->GetVkDescriptorImageInfo());
	}

	RendererStateEX& RendererStorage::GetState()
	{
		return m_State;
	}

	MaterialLibrary& RendererStorage::GetMaterialLibrary()
	{
		return m_MaterialLibrary;
	}

	void RendererStorage::OnResize(uint32_t width, uint32_t height)
	{
		f_GBuffer.OnResize(width, height);
		f_Lighting.OnResize(width, height);

		{
			// Lighting pipeline
			p_Lighting.UpdateSampler(&f_GBuffer, 5, "albedro");
			p_Lighting.UpdateSampler(&f_GBuffer, 6, "position");
			p_Lighting.UpdateSampler(&f_GBuffer, 7, "normals");
			p_Lighting.UpdateSampler(&f_GBuffer, 8, "materials");
			// Debug view pipeline
			p_Debug.UpdateSampler(&f_Depth, 1, "Depth_Attachment");
			p_Debug.UpdateSampler(&f_GBuffer, 5, "albedro");
			p_Debug.UpdateSampler(&f_GBuffer, 6, "position");
			p_Debug.UpdateSampler(&f_GBuffer, 7, "normals");
			p_Debug.UpdateSampler(&f_GBuffer, 8, "materials");

			// Combination
			p_Combination.UpdateSampler(&f_Lighting, 0);

			//// FOV
			// f_DOF.OnResize(width, height);
			//p_DOF.UpdateSampler(&f_Lighting, 0);
			//p_DOF.UpdateSampler(&f_GBuffer, 1, "position");

			// Bloom Textures
			{
				uint32_t viewportWidth = width / 2;
				uint32_t viewportHeight = height / 2;

				viewportWidth += (m_BloomComputeWorkgroupSize - (viewportWidth % m_BloomComputeWorkgroupSize));
				viewportHeight += (m_BloomComputeWorkgroupSize - (viewportHeight % m_BloomComputeWorkgroupSize));

				TextureCreateInfo TexCI{};
				TexCI.eFormat = TextureFormat::R32G32B32A32_SFLOAT;
				TexCI.eAddressMode = AddressMode::CLAMP_TO_EDGE;
				TexCI.bIsStorage = true;
				TexCI.bAnisotropyEnable = false;

				m_BloomTex.clear();
				m_BloomTex.resize(3);
				for (auto& tex : m_BloomTex)
				{
					auto vkTex = tex.GetVulkanTexture();
					vkTex->GenTexture(viewportWidth, viewportHeight, &TexCI);
				}
			}
		}
	}

	RendererDrawList::RendererDrawList()
	{
		m_AnimationJoints.resize(max_anim_joints);
	}

	void RendererDrawList::SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh, const uint32_t& material_id, bool submit_childs, AnimationController* anim_controller)
	{
		if (!m_Frustum.CheckSphere(pos) || m_Objects >= max_objects)
			return;

		auto& instance = m_Packages[mesh];
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

		m_Objects++;
		instance.CurrentIndex++;
		bool found = std::find(m_UsedMeshes.begin(), m_UsedMeshes.end(), mesh) != m_UsedMeshes.end();
		if (found == false) { m_UsedMeshes.emplace_back(mesh); }

		if (submit_childs)
		{
			for (auto& sub : mesh->m_Childs)
				SubmitMesh(pos, rotation, scale, &sub, material_id, submit_childs, anim_controller);
		}
	}

	void RendererDrawList::SubmitDirLight(DirectionalLight* light)
	{
		m_DirLight = *light;

		if(m_DirLight.IsActive && m_DirLight.IsCastShadows)
			CalculateDepthMVP();
	}

	void RendererDrawList::SubmitPointLight(PointLight* light)
	{
		uint32_t index = m_PointLightIndex;
		if (index >= max_lights)
			return;

		m_PointLights[index] = *light;
		m_PointLightIndex++;
	}

	void RendererDrawList::SubmitSpotLight(SpotLight* light)
	{
		uint32_t index = m_SpotLightIndex;
		if (index >= max_lights)
			return;

		m_SpotLights[index] = *light;
		m_SpotLightIndex++;
	}

	void RendererDrawList::CalculateDepthMVP()
	{
		// Keep depth range as small as possible
		// for better shadow map precision
		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(m_DirLight.lightFOV, 1.0f, m_DirLight.zNear, m_DirLight.zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(m_DirLight.Direction), glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0f);

		m_DepthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	}

	void RendererDrawList::SetDefaultState()
	{
		ResetDrawList();
		m_DirLight = {};
	}

	void RendererDrawList::BeginSubmit(SceneViewProjection* viewProj)
	{
		ResetDrawList();
		CalculateFrustum(viewProj);
	}

	void RendererDrawList::EndSubmit()
	{
		BuildDrawList();
	}

	void RendererDrawList::CalculateFrustum(SceneViewProjection* sceneViewProj)
	{
		m_SceneInfo = sceneViewProj;
		m_Frustum.Update(m_SceneInfo->Projection * m_SceneInfo->View);
	}

	void RendererDeferred::GBufferPass(SubmitInfo* info)
	{
		auto drawList = info->pDrawList;
		auto storage = info->pStorage;

		storage->p_Gbuffer.BeginRenderPass();
		{
			// SkyBox
			if (storage->m_State.bDrawSkyBox)
			{
				uint32_t state = storage->m_EnvironmentMap->IsDynamic();
				storage->p_Skybox.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
				storage->p_Skybox.Draw(36);
			}

			// Grid
			if (storage->m_State.bDrawGrid)
			{
				storage->p_Grid.SubmitPushConstant(ShaderType::Vertex, sizeof(glm::mat4), &storage->m_GridModel);
				storage->p_Grid.DrawMeshIndexed(&storage->m_PlaneMesh);
			}

			uint32_t cmdCount = static_cast<uint32_t>(drawList->m_UsedMeshes.size());
			for (uint32_t i = 0; i < cmdCount; ++i)
			{
				auto& cmd = drawList->m_DrawList[i];

				storage->p_Gbuffer.SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &cmd.Offset);
				storage->p_Gbuffer.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
			}
		}
		storage->p_Gbuffer.EndRenderPass();
	}

	void RendererDeferred::LightingPass(SubmitInfo* info)
	{
		auto drawList = info->pDrawList;
		auto storage = info->pStorage;

		storage->p_Lighting.BeginRenderPass();
		{
			struct push_constant
			{
				glm::mat4 lightSpace{};
				uint32_t numPointLights;
				uint32_t numSpotLights;
			};

			push_constant light_pc;
			light_pc.numPointLights = drawList->m_PointLightIndex;
			light_pc.numSpotLights = drawList->m_SpotLightIndex;
			light_pc.lightSpace = info->pDrawList->m_DepthMVP;
			storage->p_Lighting.SubmitPushConstant(ShaderType::Fragment, sizeof(push_constant), &light_pc);
			storage->p_Lighting.Draw(3);
		}
		storage->p_Lighting.EndRenderPass();
	}

	void RendererDeferred::DepthPass(SubmitInfo* info)
	{
		struct PushConstant
		{
			glm::mat4 DepthMVP = glm::mat4(1.0f);
			uint32_t  DataOffset = 0;
		};

		auto drawList = info->pDrawList;
		auto storage = info->pStorage;

		PushConstant  pushConstant{};
		uint32_t      cmdCount = static_cast<uint32_t>(drawList->m_UsedMeshes.size());

		if (drawList->m_DirLight.IsActive && drawList->m_DirLight.IsCastShadows)
		{
#ifndef FROSTIUM_OPENGL_IMPL
			VkCommandBuffer cmdBuffer = storage->p_DepthPass.GetVkCommandBuffer();
#endif
			storage->p_DepthPass.BeginRenderPass();
			{
#ifndef FROSTIUM_OPENGL_IMPL
				// Set depth bias (aka "Polygon offset")
				// Required to avoid shadow mapping artifacts
				vkCmdSetDepthBias(cmdBuffer, 1.25f, 0.0f, 1.75f);
#endif
				pushConstant.DepthMVP = drawList->m_DepthMVP;
				for (uint32_t i = 0; i < cmdCount; ++i)
				{
					auto& cmd = drawList->m_DrawList[i];
					pushConstant.DataOffset = cmd.Offset;

					storage->p_DepthPass.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &pushConstant);
					storage->p_DepthPass.DrawMeshIndexed(cmd.Mesh, cmd.InstancesCount);
				}
			}
			storage->p_DepthPass.EndRenderPass();
		}
	}

	// Credits: https://www.youtube.com/watch?v=tI70-HIc5ro
	void RendererDeferred::BloomPass(SubmitInfo* info)
	{
		if (info->pStorage->m_State.Bloom.Enabled == false)
			return;

		uint32_t descriptorIndex = 0;
		auto storage = info->pStorage;
		auto& bloomTex = storage->m_BloomTex;
		auto& settings = storage->m_State.Bloom;

		auto TEXTURE_STORAGE_SET = [&](Texture* tex, uint32_t mip = 0, uint32_t binding = 0) 
		{
			auto descriptor = tex->GetVulkanTexture()->GetMipImageView(mip);
			storage->p_Bloom.GeDescriptor(descriptorIndex).UpdateImageResource(binding, descriptor, binding == 0 ? true : false);
		};

		auto TEXTURE_SET_SCENE = [&](Texture* tex)
		{
			storage->p_Bloom.GeDescriptor(descriptorIndex).Update2DSamplers({ tex }, 1);
		};

		const auto TEXTURE_BLOOM_SET_FROM_SECENE = [&](uint32_t binding = 2)
		{
			const auto& descriptor = storage->f_Lighting.GetVulkanFramebuffer().GetAttachment(0)->ImageInfo;
			storage->p_Bloom.GeDescriptor(descriptorIndex).UpdateImageResource(binding, descriptor);
		};

		struct BloomComputePushConstants
		{
			glm::vec4 Params;
			float LOD = 0.0f;
			int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
		} bloomComputePushConstants;
		bloomComputePushConstants.Params = { settings.Threshold, settings.Threshold - settings.Knee, settings.Knee * 2.0f, 0.25f / settings.Knee };
		bloomComputePushConstants.Mode = 0;

		uint32_t workGroupsX = 0, workGroupsY = 0;

		storage->p_Bloom.BeginCompute(info->pCmdStorage);
		{
			// Prefilter
			{
				TEXTURE_STORAGE_SET(&bloomTex[0]);
				TEXTURE_BLOOM_SET_FROM_SECENE(1);

				workGroupsX = bloomTex[0].GetInfo().Width / storage->m_BloomComputeWorkgroupSize;
				workGroupsY = bloomTex[0].GetInfo().Height / storage->m_BloomComputeWorkgroupSize;

				storage->p_Bloom.SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom.Dispatch(workGroupsX, workGroupsY, 1, descriptorIndex);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[0].GetVulkanTexture()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[0].GetVulkanTexture()->GetMips(), 0, 1 };
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);
			}

			// Downsample
			bloomComputePushConstants.Mode = 1;
			uint32_t mips = storage->m_BloomTex[0].GetVulkanTexture()->GetMips() - 2;
			for (uint32_t i = 1; i < mips; i++)
			{
				descriptorIndex++;
				auto [mipWidth, mipHeight] = bloomTex[0].GetVulkanTexture()->GetMipSize(i);

				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)storage->m_BloomComputeWorkgroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)storage->m_BloomComputeWorkgroupSize);

				TEXTURE_STORAGE_SET(&bloomTex[1], i);
				TEXTURE_SET_SCENE(&bloomTex[0]);
				TEXTURE_BLOOM_SET_FROM_SECENE();

				bloomComputePushConstants.LOD = i - 1.0f;
				storage->p_Bloom.SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom.Dispatch(workGroupsX, workGroupsY, 1, descriptorIndex);

				{
					VkImageMemoryBarrier imageMemoryBarrier = {};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.image = bloomTex[1].GetVulkanTexture()->GetVkImage();
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[1].GetVulkanTexture()->GetMips(), 0, 1 };
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(
						info->pCmdStorage->Buffer,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						0,
						0, nullptr,
						0, nullptr,
						1, &imageMemoryBarrier);
				}

				descriptorIndex++;

				TEXTURE_STORAGE_SET(&bloomTex[0], i);
				TEXTURE_SET_SCENE(&bloomTex[1]);
				TEXTURE_BLOOM_SET_FROM_SECENE();

				bloomComputePushConstants.LOD = (float)i;
				storage->p_Bloom.SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom.Dispatch(workGroupsX, workGroupsY, 1, descriptorIndex);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[0].GetVulkanTexture()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[0].GetVulkanTexture()->GetMips(), 0, 1 };
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);
			}


			// Upsample first
			descriptorIndex++;
			bloomComputePushConstants.Mode = 2;
			{
				workGroupsX *= 2;
				workGroupsY *= 2;

				TEXTURE_STORAGE_SET(&bloomTex[2], mips - 2);
				TEXTURE_SET_SCENE(&bloomTex[0]);
				TEXTURE_BLOOM_SET_FROM_SECENE();

				bloomComputePushConstants.LOD--;
				storage->p_Bloom.SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);

				auto [mipWidth, mipHeight] = bloomTex[2].GetVulkanTexture()->GetMipSize(mips - 2);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)storage->m_BloomComputeWorkgroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)storage->m_BloomComputeWorkgroupSize);

				storage->p_Bloom.Dispatch(workGroupsX, workGroupsY, 1, descriptorIndex);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[2].GetVulkanTexture()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,   bloomTex[2].GetVulkanTexture()->GetMips(), 0, 1 };
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);
			}

			// Upsample
			descriptorIndex++;
			bloomComputePushConstants.Mode = 3;
			for (int32_t mip = mips - 3; mip >= 0; mip--)
			{
				auto [mipWidth, mipHeight] = bloomTex[2].GetVulkanTexture()->GetMipSize(mip);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)storage->m_BloomComputeWorkgroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)storage->m_BloomComputeWorkgroupSize);

				TEXTURE_STORAGE_SET(&bloomTex[2], mip);
				TEXTURE_SET_SCENE(&bloomTex[0]);
				storage->p_Bloom.GeDescriptor(descriptorIndex).Update2DSamplers({ &bloomTex[2] }, 2, false);

				bloomComputePushConstants.LOD = (float)mip;
				storage->p_Bloom.SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom.Dispatch(workGroupsX, workGroupsY, 1, descriptorIndex);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[2].GetVulkanTexture()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[2].GetVulkanTexture()->GetMips(), 0, 1 };
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);

				descriptorIndex++;
			}
		}
	}

	bool RendererDeferred::DebugViewPass(SubmitInfo* info)
	{
		auto storage = info->pStorage;

		// Debug View
		if (storage->m_State.eDebugView != DebugViewFlags::None)
		{
			storage->p_Debug.BeginRenderPass();
			{
				uint32_t state = (uint32_t)storage->m_State.eDebugView;
				storage->p_Debug.SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
				storage->p_Debug.Draw(3);
			}
			storage->p_Debug.EndRenderPass();
			return true;
		}

		return false;
	}

	void RendererDeferred::CompositionPass(SubmitInfo* info)
	{
		struct push_constant
		{
			uint32_t state = 0;
			uint32_t enabledFXAA = 0;
			uint32_t enabledMask = 0;
		};

		auto storage = info->pStorage;
		push_constant push_constant{};
		push_constant.enabledMask = false;
		push_constant.enabledFXAA = storage->m_State.FXAA.Enabled;
		if (storage->m_State.Bloom.Enabled)
		{
			push_constant.state = 1;
			storage->p_Combination.UpdateSampler(&storage->m_BloomTex[2], 1);
		}

		if (info->pClearInfo->bClear)
		{
			storage->p_Combination.BeginRenderPass();
			storage->p_Combination.ClearColors(info->pClearInfo->color);
			storage->p_Combination.EndRenderPass();
		}

		storage->p_Combination.BeginRenderPass();
		{

			storage->p_Combination.SubmitPushConstant(ShaderType::Fragment, sizeof(push_constant), &push_constant);
			storage->p_Combination.Draw(3);
		}
		storage->p_Combination.EndRenderPass();
	}

	void RendererDeferred::UpdateUniforms(SubmitInfo* info)
	{
		info->pStorage->UpdateUniforms(info->pDrawList, info->pStorage->f_Main);
	}
}