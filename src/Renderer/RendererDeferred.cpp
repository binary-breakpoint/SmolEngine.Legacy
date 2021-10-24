#include "stdafx.h"
#include "Renderer/RendererDeferred.h"
#include "Multithreading/JobsSystemInstance.h"
#include "Animation/AnimationController.h"
#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/Shader.h"
#include "Import/glTFImporter.h"
#include "Pools/MaterialPool.h"
#include "Tools/Utils.h"

#include "Materials/MaterialPBR.h"
#include "Materials/PBRFactory.h"

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglShader.h"
#include "Backends/OpenGL/OpenglRendererAPI.h"
#else
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanPBRLoader.h"
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanTexture.h"
#include "Backends/Vulkan/VulkanComputePipeline.h"
#include "Backends/Vulkan/VulkanFramebuffer.h"
#endif

#include <glm/gtx/transform.hpp>

namespace SmolEngine
{
	struct SubmitInfo
	{
		ClearInfo* pClearInfo = nullptr;
		RendererStorage* pStorage = nullptr;
		RendererDrawList* pDrawList = nullptr;
		CommandBufferStorage* pCmdStorage = nullptr;
	};

	void RendererDeferred::DrawFrame(ClearInfo* clearInfo, bool batch_cmd)
	{
		CommandBufferStorage cmdStorage{};
		if (batch_cmd) { cmdStorage.Buffer = VulkanContext::GetCurrentVkCmdBuffer(); }
		else { VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage); }

		SubmitInfo submitInfo{};
		submitInfo.pClearInfo = clearInfo;
		submitInfo.pDrawList = RendererDrawList::GetSingleton();
		submitInfo.pStorage = RendererStorage::GetSingleton();
		submitInfo.pCmdStorage = &cmdStorage;

		submitInfo.pStorage->m_DefaultMaterial->GetPipeline()->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_Lighting->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_Skybox->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_DepthPass->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_Debug->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_Grid->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_Combination->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_Voxelization->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_VoxelView->SetCommandBuffer(cmdStorage.Buffer);
		submitInfo.pStorage->p_GI->SetCommandBuffer(cmdStorage.Buffer);

		//submitInfo.pStorage->p_DOF->Cast<VulkanPipeline>()->SetCommandBuffer(cmdStorage.Buffer);

		UpdateUniforms(&submitInfo);
		DepthPass(&submitInfo);


		VoxelizationPass(&submitInfo);
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
		{
			auto& vct = m_VCTParams;
			vct.VoxelFlags = Texture::Create();
			vct.Radiance = Texture::Create();

			vct.VoxelAlbedo = Texture::Create();
			vct.VoxelNormal = Texture::Create();
			vct.VoxelMaterials = Texture::Create();

			vct.AlbedoBuffer = Texture::Create();
			vct.NormalBuffer = Texture::Create();
			vct.MaterialsBuffer = Texture::Create();

			TextureCreateInfo texCI{};
			texCI.Width = vct.VolumeDimension;
			texCI.Height = vct.VolumeDimension;
			texCI.Depth = vct.VolumeDimension;
			texCI.Mips = 1;

			texCI.eAddressMode = AddressMode::CLAMP_TO_EDGE;
			texCI.eFormat = TextureFormat::R8G8B8A8_UNORM;
			
			vct.VoxelAlbedo->LoadAs3D(&texCI);
			vct.VoxelNormal->LoadAs3D(&texCI);
			vct.VoxelMaterials->LoadAs3D(&texCI);

			vct.Radiance->LoadAs3D(&texCI);

			texCI.eFormat = TextureFormat::R8_UNORM;
			vct.VoxelFlags->LoadAs3D(&texCI);

			texCI.eFormat = TextureFormat::R32_UINT;
			vct.AlbedoBuffer->LoadAs3D(&texCI);
			vct.NormalBuffer->LoadAs3D(&texCI);
			vct.MaterialsBuffer->LoadAs3D(&texCI);


			texCI.eFormat = TextureFormat::R8G8B8A8_UNORM;
			texCI.Width = vct.VolumeDimension / 2;
			texCI.Height = vct.VolumeDimension / 2;
			texCI.Depth = vct.VolumeDimension / 2;
			texCI.Mips = 0;

			// mip mapping textures per face
			vct.VoxelTexMipmap.resize(6);
			for (uint32_t i = 0; i < 6; ++i)
			{
				vct.VoxelTexMipmap[i] = Texture::Create();
				vct.VoxelTexMipmap[i]->LoadAs3D(&texCI);
			}
		}


		CreatePBRMaps();
		CreateFramebuffers();
		CreatePipelines();

		m_PBRFactory = std::make_shared<PBRFactory>();
		m_PBRFactory->AddDefaultMaterial();
		m_PBRFactory->UpdateMaterials();
	}

	void RendererStorage::SetRenderTarget(Ref<Framebuffer>& target)
	{
		s_Instance->f_Main = target;
		s_Instance->p_Combination->SetFramebuffers({ s_Instance->f_Main });
		s_Instance->p_Debug->SetFramebuffers({ s_Instance->f_Main });
	}

	void RendererStorage::SetDefaultState()
	{
		s_Instance->m_State = {};
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

		// Default Material
		m_DefaultMaterial = MaterialPBR::Create();

		// Lighting
		{
			ShaderCreateInfo shaderCI = {};
			{
				shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
				shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Lighting.frag";

				ShaderBufferInfo bufferInfo = {};

				// Fragment
				bufferInfo.Size = sizeof(PointLight) * max_lights;
				shaderCI.Buffers[m_PointLightBinding] = bufferInfo;

				bufferInfo.Size = sizeof(SpotLight) * max_lights;
				shaderCI.Buffers[m_SpotLightBinding] = bufferInfo;

				bufferInfo.Size = sizeof(DirectionalLight);
				shaderCI.Buffers[m_DirLightBinding] = bufferInfo;
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.PipelineName = "Lighting_Pipeline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.bDepthTestEnabled = false;
				DynamicPipelineCI.TargetFramebuffers = { f_Lighting };
			}

			p_Lighting = GraphicsPipeline::Create();
			auto result = p_Lighting->Build(&DynamicPipelineCI);
			assert(result == true);

			p_Lighting->UpdateSampler(f_Depth, 1, "Depth_Attachment");
			s_Instance->p_Lighting->UpdateImageDescriptor(2, s_Instance->m_PBRLoader->GetIrradianceDesriptor());
			s_Instance->p_Lighting->UpdateImageDescriptor(3, s_Instance->m_PBRLoader->GetBRDFLUTDesriptor());
			s_Instance->p_Lighting->UpdateImageDescriptor(4, s_Instance->m_PBRLoader->GetPrefilteredCubeDesriptor());

			p_Lighting->UpdateSampler(f_GBuffer, 5, "albedro");
			p_Lighting->UpdateSampler(f_GBuffer, 6, "position");
			p_Lighting->UpdateSampler(f_GBuffer, 7, "normals");
			p_Lighting->UpdateSampler(f_GBuffer, 8, "materials");
		}

		// GI
		{
			ShaderCreateInfo shaderCI = {};
			{
				shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
				shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/GI.frag";

				shaderCI.Buffers[304].bGlobal = false;
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.PipelineName = "Lighting_Pipeline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				//DynamicPipelineCI.bDepthTestEnabled = false;
				DynamicPipelineCI.TargetFramebuffers = { f_Lighting };
			}

			p_GI = GraphicsPipeline::Create();
			auto result = p_GI->Build(&DynamicPipelineCI);
			assert(result == true);

			p_GI->UpdateSampler(f_GBuffer, 0, "albedro");
			p_GI->UpdateSampler(f_GBuffer, 1, "position");
			p_GI->UpdateSampler(f_GBuffer, 2, "normals");
			p_GI->UpdateSampler(f_GBuffer, 3, "materials");
			p_GI->UpdateSampler(f_Depth, 4, "Depth_Attachment");

			p_GI->UpdateSampler(m_VCTParams.VoxelNormal, 5);
			p_GI->UpdateSampler(m_VCTParams.Radiance, 6);
			p_GI->UpdateSamplers(m_VCTParams.VoxelTexMipmap, 7);
		}

		JobsSystemInstance::BeginSubmition();
		{
			// Grid
			JobsSystemInstance::Schedule([&]()
				{
					Utils::ComposeTransform(glm::vec3(0), glm::vec3(0), { 100, 1, 100 }, m_GridModel);

					m_GridMesh = Mesh::Create();
					m_GridMesh->LoadFromFile(path + "Models/plane_v2.gltf");

					GraphicsPipelineCreateInfo pipelineCI = {};
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/Grid.vert";
						shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Grid.frag";
					};

					pipelineCI.PipelineName = "Grid";
					pipelineCI.eCullMode = CullMode::None;
					pipelineCI.VertexInputInfos = { vertexMain };
					pipelineCI.bDepthTestEnabled = false;
					pipelineCI.bDepthWriteEnabled = false;
					pipelineCI.TargetFramebuffers = { f_GBuffer };
					pipelineCI.ShaderCreateInfo = shaderCI;

					p_Grid = GraphicsPipeline::Create();
					auto result = p_Grid->Build(&pipelineCI);
					assert(result == true);
				});

			// Skybox
			JobsSystemInstance::Schedule([&]()
				{
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/Skybox.vert";
						shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Skybox.frag";
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
						DynamicPipelineCI.TargetFramebuffers = { f_GBuffer };
					}

					p_Skybox = GraphicsPipeline::Create();
					auto result = p_Skybox->Build(&DynamicPipelineCI);
					assert(result == true);

					auto map = m_EnvironmentMap->GetCubeMap();
					p_Skybox->UpdateSampler(map, 1);

					Ref<VertexBuffer> skyBoxFB = VertexBuffer::Create();
					skyBoxFB->BuildFromMemory(skyboxVertices, sizeof(skyboxVertices));
					p_Skybox->SetVertexBuffers({ skyBoxFB });
				});


			// Depth Pass
			JobsSystemInstance::Schedule([&]()
				{
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/DepthPass.vert";
						shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/DepthPass.frag";
					};

					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					{
						DynamicPipelineCI.VertexInputInfos = { vertexMain };
						DynamicPipelineCI.PipelineName = "DepthPass_Pipeline";
						DynamicPipelineCI.ShaderCreateInfo = shaderCI;
						DynamicPipelineCI.TargetFramebuffers = { f_Depth };
						DynamicPipelineCI.bDepthBiasEnabled = true;
						DynamicPipelineCI.StageCount = 1;

						p_DepthPass = GraphicsPipeline::Create();
						auto result = p_DepthPass->Build(&DynamicPipelineCI);
						assert(result == true);
					}
				});

			// Draw Voxels

			JobsSystemInstance::Schedule([&]()
				{
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/DrawVoxels.vert";
						shaderCI.Stages[ShaderType::Geometry] = path + "Shaders/DrawVoxels.geom";
						shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/DrawVoxels.frag";

						shaderCI.Buffers[202].bGlobal = false;
					};

					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					{
						DynamicPipelineCI.PipelineName = "DrawVoxels";
						DynamicPipelineCI.ShaderCreateInfo = shaderCI;
						DynamicPipelineCI.TargetFramebuffers = { f_Main };
						DynamicPipelineCI.eCullMode = CullMode::None;
						DynamicPipelineCI.bDepthTestEnabled = false;
						DynamicPipelineCI.MaxDepth = 1.0f;

						p_VoxelView = GraphicsPipeline::Create();
						auto result = p_VoxelView->Build(&DynamicPipelineCI);
						assert(result == true);
					}
				});


			// Inject Propagation

			JobsSystemInstance::Schedule([&]()
				{
					ComputePipelineCreateInfo compCI = {};
					compCI.ShaderPath = path + "Shaders/InjectPropagation.comp";

					p_InjectPropagation = ComputePipeline::Create();
					auto result = p_InjectPropagation->Build(&compCI);
					assert(result == true);

					p_InjectPropagation->UpdateSampler(m_VCTParams.Radiance, 0, true);
					p_InjectPropagation->UpdateSampler(m_VCTParams.VoxelAlbedo, 1);
					p_InjectPropagation->UpdateSampler(m_VCTParams.VoxelNormal, 2);
					p_InjectPropagation->UpdateSamplers(m_VCTParams.VoxelTexMipmap, 3);

				});

			// Mipmap Base

			JobsSystemInstance::Schedule([&]()
				{
					ComputePipelineCreateInfo compCI = {};
					compCI.ShaderPath = path + "Shaders/AnisoMipmapBase.comp";

					p_MipmapBase = ComputePipeline::Create();
					auto result = p_MipmapBase->Build(&compCI);
					assert(result == true);

					p_MipmapBase->UpdateSamplers(m_VCTParams.VoxelTexMipmap, 0, true);
					p_MipmapBase->UpdateSampler(m_VCTParams.Radiance, 5);

				});


			// Mipmap Volume

			JobsSystemInstance::Schedule([&]()
				{
					ComputePipelineCreateInfo compCI = {};
					compCI.ShaderPath = path + "Shaders/AnisoMipmapVolume.comp";

					p_MipmapVolume = ComputePipeline::Create();
					auto result = p_MipmapVolume->Build(&compCI);
					assert(result == true);

					p_MipmapVolume->UpdateSamplers(m_VCTParams.VoxelTexMipmap, 0, true);
					p_MipmapVolume->UpdateSamplers(m_VCTParams.VoxelTexMipmap, 5);

				});


			// Inject Radiance Pass

			JobsSystemInstance::Schedule([&]()
			{
				ComputePipelineCreateInfo compCI = {};
				compCI.ShaderPath = path + "Shaders/InjectRadiance.comp";

				compCI.ShaderBufferInfos[205].bGlobal = false;
				compCI.ShaderBufferInfos[206].bGlobal = false;

				p_InjectRadiance = ComputePipeline::Create();
				auto result = p_InjectRadiance->Build(&compCI);
				assert(result == true);

				auto& vct = m_VCTParams;

				p_InjectRadiance->UpdateSampler(vct.VoxelAlbedo, 0);
				p_InjectRadiance->UpdateSampler(vct.VoxelNormal, 1, true);
				p_InjectRadiance->UpdateSampler(vct.Radiance, 2, true);
				p_InjectRadiance->UpdateSampler(vct.VoxelMaterials, 3, true);
				p_InjectRadiance->UpdateSampler(f_Depth, 4, "Depth_Attachment");

			});

			// Voxelization Pass
			JobsSystemInstance::Schedule([&]()
				{
					ShaderCreateInfo shaderCI = {};
					{
						shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/Voxelization.vert";
						shaderCI.Stages[ShaderType::Geometry] = path + "Shaders/Voxelization.geom";
						shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Voxelization.frag";

						shaderCI.Buffers[202].bGlobal = false;
					};

					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					{
						DynamicPipelineCI.VertexInputInfos = { vertexMain };
						DynamicPipelineCI.PipelineName = "Voxelization_Pipeline";
						DynamicPipelineCI.ShaderCreateInfo = shaderCI;
						DynamicPipelineCI.TargetFramebuffers = { f_Voxelization };
						DynamicPipelineCI.bDepthTestEnabled = false;
						DynamicPipelineCI.eCullMode = CullMode::None;

						p_Voxelization = GraphicsPipeline::Create();
						auto result = p_Voxelization->Build(&DynamicPipelineCI);
						assert(result == true);
					}

					auto& vct = m_VCTParams;

					p_Voxelization->UpdateSampler(vct.VoxelAlbedo, 3, true);
					p_Voxelization->UpdateSampler(vct.VoxelNormal, 4, true);
					p_Voxelization->UpdateSampler(vct.VoxelMaterials, 5, true);
					p_Voxelization->UpdateSampler(vct.VoxelFlags, 6, true);

					p_Voxelization->UpdateSampler(vct.AlbedoBuffer, 0, true);
					p_Voxelization->UpdateSampler(vct.NormalBuffer, 1, true);
					p_Voxelization->UpdateSampler(vct.MaterialsBuffer, 2, true);
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
					shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
					shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Combination.frag";
					DynamicPipelineCI.ShaderCreateInfo = shaderCI;
					DynamicPipelineCI.PipelineName = "Combination";

					p_Combination = GraphicsPipeline::Create();
					auto result = p_Combination->Build(&DynamicPipelineCI);
					assert(result == true);

					p_Combination->UpdateSampler(f_Lighting, 0);
				});

			// Debug
			JobsSystemInstance::Schedule([&]()
				{
					GraphicsPipelineCreateInfo DynamicPipelineCI = {};
					DynamicPipelineCI.eCullMode = CullMode::None;
					DynamicPipelineCI.TargetFramebuffers = { f_Main };

					ShaderCreateInfo shaderCI = {};
					shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/GenTriangle.vert";
					shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/DebugView.frag";
					DynamicPipelineCI.ShaderCreateInfo = shaderCI;
					DynamicPipelineCI.PipelineName = "Debug";

					p_Debug = GraphicsPipeline::Create();
					auto result = p_Debug->Build(&DynamicPipelineCI);
					assert(result == true);

					p_Debug->UpdateSampler(f_Depth, 1, "Depth_Attachment");
					p_Debug->UpdateSampler(f_GBuffer, 5, "albedro");
					p_Debug->UpdateSampler(f_GBuffer, 6, "position");
					p_Debug->UpdateSampler(f_GBuffer, 7, "normals");
					p_Debug->UpdateSampler(f_GBuffer, 8, "materials");
				});

			//Bloom
			JobsSystemInstance::Schedule([&]()
				{
					ComputePipelineCreateInfo compCI = {};
					compCI.ShaderPath = path + "Shaders/Bloom.comp";
					compCI.DescriptorCount = 24;

					p_Bloom = ComputePipeline::Create();
					auto result = p_Bloom->Build(&compCI);
					assert(result == true);

					auto& spec = f_Main->GetSpecification();
					uint32_t workgroupSize = 4;
					uint32_t viewportWidth = spec.Width / 2;
					uint32_t viewportHeight = spec.Height / 2;

					viewportWidth += (workgroupSize - (viewportWidth % workgroupSize));
					viewportHeight += (workgroupSize - (viewportHeight % workgroupSize));

					TextureCreateInfo texCI{};
					texCI.eFormat = TextureFormat::R32G32B32A32_SFLOAT;
					texCI.eAddressMode = AddressMode::CLAMP_TO_EDGE;
					texCI.bAnisotropyEnable = false;
					texCI.Width = viewportWidth;
					texCI.Height = viewportHeight;

					m_BloomTex.resize(3);

					for (auto& tex : m_BloomTex)
					{
						tex = Texture::Create();
						tex->LoadAsStorage(&texCI);
					}
				});

		}

		JobsSystemInstance::EndSubmition();
	}

	void RendererStorage::CreateFramebuffers()
	{
		// Main
		f_Main = GraphicsContext::GetSingleton()->GetMainFramebuffer();

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
					WindowData* winData = GraphicsContext::GetSingleton()->GetWindow()->GetWindowData();

					FramebufferSpecification framebufferCI = {};
					framebufferCI.Width = winData->Width;
					framebufferCI.Height = winData->Height;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { albedro, position, normals, materials };

					f_GBuffer = Framebuffer::Create();
					f_GBuffer->Build(&framebufferCI);

				});

			// Voxelization
			JobsSystemInstance::Schedule([&]()
				{

					FramebufferAttachment color = FramebufferAttachment(AttachmentFormat::SFloat4_16, true, "color_1");
					FramebufferSpecification framebufferCI = {};

					framebufferCI.eFiltering = ImageFilter::LINEAR;
					framebufferCI.Width = m_VCTParams.VolumeDimension;
					framebufferCI.Height = m_VCTParams.VolumeDimension;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { color };

					f_Voxelization = Framebuffer::Create();
					f_Voxelization->Build(&framebufferCI);

				});


			// Lighting
			JobsSystemInstance::Schedule([&]()
				{
					WindowData* winData = GraphicsContext::GetSingleton()->GetWindow()->GetWindowData();
					FramebufferAttachment color = FramebufferAttachment(AttachmentFormat::SFloat4_16, true, "color_1");
					FramebufferSpecification framebufferCI = {};
					framebufferCI.eFiltering = ImageFilter::LINEAR;
					framebufferCI.Width = winData->Width;
					framebufferCI.Height = winData->Height;
					framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
					framebufferCI.Attachments = { color };

					f_Lighting = Framebuffer::Create();
					f_Lighting->Build(&framebufferCI);

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

					f_Depth = Framebuffer::Create();
					f_Depth->Build(&framebufferCI);

				});
		}
		JobsSystemInstance::EndSubmition();
	}

	void RendererStorage::CreatePBRMaps()
	{
		m_PBRLoader = PBRLoader::Create();
		m_EnvironmentMap = std::make_shared<EnvironmentMap>();
		auto& config = m_EnvironmentMap->GetDynamicSkyProperties();
		config.SunPosition = glm::vec4(1, -11, 0, 0);
		config.NumCirrusCloudsIterations = 0;

		m_EnvironmentMap->Initialize();
		m_EnvironmentMap->GenerateDynamic();

		auto map = m_EnvironmentMap->GetCubeMap();
		m_PBRLoader->GeneratePBRCubeMaps(map);
	}

	void RendererDrawList::BuildDrawList()
	{
		JobsSystemInstance::BeginSubmition();
		{
			for (auto& [material, package] : s_Instance->m_Packages)
			{
				for (auto& [mesh, instance] : package.Instances)
				{
					auto& cmd = s_Instance->m_DrawList[s_Instance->m_InstanceIndex];
					cmd.Mesh = mesh;

					for (uint32_t i = 0; i < instance.Index; i++)
					{
						auto& object = instance.Objects[i];

						auto& cmdPackage = cmd.Packages[material];
						bool is_animated = object.AnimController != nullptr;
						uint32_t anim_offset = s_Instance->m_LastAnimationOffset;
						InstanceData& instanceUBO = s_Instance->m_InstancesData[s_Instance->m_Objects];

						// Setting draw list command
						cmdPackage.Offset = s_Instance->m_InstanceIndex;
						cmdPackage.Instances = instance.Index;

						// Animations
						if (is_animated)
						{
							if (mesh->IsRootNode())
							{
								object.AnimController->Update();
								object.AnimController->CopyJoints(s_Instance->m_AnimationJoints, s_Instance->m_LastAnimationOffset);
								s_Instance->m_RootOffsets[mesh] = anim_offset;
							}
							else
							{
								auto& it = s_Instance->m_RootOffsets.find(mesh->m_Root);
								if (it != s_Instance->m_RootOffsets.end())
									anim_offset = it->second;
							}
						}

						// Transform
						{
							JobsSystemInstance::Schedule([is_animated, anim_offset, &object, &instanceUBO]()
								{
									Utils::ComposeTransform(*object.WorldPos, *object.Rotation, *object.Scale, instanceUBO.ModelView);

									instanceUBO.MaterialID = object.PBRHandle != nullptr ? object.PBRHandle->GetID() : 0;
									instanceUBO.IsAnimated = is_animated;
									instanceUBO.AnimOffset = anim_offset;
									instanceUBO.EntityID = 0; // temp

									object.Reset();
								});
						}

						s_Instance->m_Objects++;
					}

					instance.Index = 0;
					s_Instance->m_InstanceIndex++;
				}
			}
		}
		JobsSystemInstance::EndSubmition();
	}

	Frustum& RendererDrawList::GetFrustum()
	{
		return s_Instance->m_Frustum;
	}

	void RendererStorage::UpdateUniforms(RendererDrawList* drawList, Ref<Framebuffer>& target)
	{
		JobsSystemInstance::BeginSubmition();
		{
			// Updates Scene Data
			JobsSystemInstance::Schedule([&]()
				{
					m_DefaultMaterial->SubmitBuffer(m_SceneDataBinding, sizeof(SceneViewProjection), drawList->m_SceneInfo);
				});

			// Updates Lighting State
			JobsSystemInstance::Schedule([&]()
				{
					p_Lighting->SubmitBuffer(m_LightingStateBinding, sizeof(IBLProperties), &m_State.IBL);
				});

			// Updates Bloom State
			JobsSystemInstance::Schedule([&]()
				{
					p_Lighting->SubmitBuffer(m_BloomStateBinding, sizeof(BloomProperties), &m_State.Bloom);
				});

			// Updates FXAA State
			JobsSystemInstance::Schedule([&]()
				{
					float width = 1.0f / static_cast<float>(target->GetSpecification().Width);
					float height = 1.0f / static_cast<float>(target->GetSpecification().Height);
					m_State.FXAA.InverseScreenSize = glm::vec2(width, height);
					p_Combination->SubmitBuffer(m_FXAAStateBinding, sizeof(FXAAProperties), &m_State.FXAA);
				});

			// Updates Directional Lights
			JobsSystemInstance::Schedule([&]()
				{
					p_Lighting->SubmitBuffer(m_DirLightBinding, sizeof(DirectionalLight), &drawList->m_DirLight);
				});

			// Updates Point Lights
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_PointLightIndex > 0)
					{
						p_Lighting->SubmitBuffer(m_PointLightBinding,
							sizeof(PointLight) * drawList->m_PointLightIndex, drawList->m_PointLights.data());
					}
				});

			// Updates Spot Lights
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_SpotLightIndex > 0)
					{
						p_Lighting->SubmitBuffer(m_SpotLightBinding,
							sizeof(SpotLight) * drawList->m_SpotLightIndex, drawList->m_SpotLights.data());
					}
				});

			// Updates Animation joints
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_LastAnimationOffset > 0)
					{
						m_DefaultMaterial->SubmitBuffer(m_AnimBinding,
							sizeof(glm::mat4) * drawList->m_LastAnimationOffset, drawList->m_AnimationJoints.data());
					}
				});

			// Updates Batch Data
			JobsSystemInstance::Schedule([&]()
				{
					if (drawList->m_Objects > 0)
					{
						m_DefaultMaterial->SubmitBuffer(m_ShaderDataBinding,
							sizeof(InstanceData) * drawList->m_Objects, drawList->m_InstancesData.data());
					}
				});

		}
		JobsSystemInstance::EndSubmition();
	}

	RendererStorage::RendererStorage()
	{
		s_Instance = this;
	}

	RendererStorage::~RendererStorage()
	{
		s_Instance = nullptr;
	}

	void RendererStorage::SetDynamicSkybox(DynamicSkyProperties& properties, const glm::mat4& proj, bool regeneratePBRmaps)
	{
		auto& ref = s_Instance->m_EnvironmentMap->GetDynamicSkyProperties();
		ref = properties;

		if (regeneratePBRmaps)
		{
			s_Instance->m_EnvironmentMap->GenerateDynamic(proj);

			auto cubeMap = s_Instance->m_EnvironmentMap->GetCubeMap();
			s_Instance->m_PBRLoader->GeneratePBRCubeMaps(cubeMap);

			// Update Descriptors
			s_Instance->p_Lighting->UpdateImageDescriptor(2, s_Instance->m_PBRLoader->GetIrradianceDesriptor());
			s_Instance->p_Lighting->UpdateImageDescriptor(3, s_Instance->m_PBRLoader->GetBRDFLUTDesriptor());
			s_Instance->p_Lighting->UpdateImageDescriptor(4, s_Instance->m_PBRLoader->GetPrefilteredCubeDesriptor());
			s_Instance->p_Skybox->UpdateSampler(cubeMap, 1);
		}
		else
		{
			s_Instance->m_EnvironmentMap->UpdateDescriptors();
		}
	}

	void RendererStorage::SetStaticSkybox(Ref<Texture>& skybox)
	{
		s_Instance->m_EnvironmentMap->GenerateStatic(skybox);
		auto cubeMap = s_Instance->m_EnvironmentMap->GetCubeMap();
		s_Instance->m_PBRLoader->GeneratePBRCubeMaps(cubeMap);

		// Update Descriptors
		s_Instance->p_Lighting->UpdateImageDescriptor(2, s_Instance->m_PBRLoader->GetIrradianceDesriptor());
		s_Instance->p_Lighting->UpdateImageDescriptor(3, s_Instance->m_PBRLoader->GetBRDFLUTDesriptor());
		s_Instance->p_Lighting->UpdateImageDescriptor(4, s_Instance->m_PBRLoader->GetPrefilteredCubeDesriptor());
		s_Instance->p_Skybox->UpdateSampler(cubeMap, 1);
	}

	RendererStateEX& RendererStorage::GetState()
	{
		return s_Instance->m_State;
	}

	Ref<MaterialPBR> RendererStorage::GetDefaultMaterial()
	{
		return s_Instance->m_DefaultMaterial;
	}

	Ref<PBRLoader> RendererStorage::GetPBRLoader()
	{
		return s_Instance->m_PBRLoader;
	}

	void RendererStorage::OnResize(uint32_t width, uint32_t height)
	{
		f_GBuffer->OnResize(width, height);
		f_Lighting->OnResize(width, height);

		{
			// Lighting pipeline
			p_Lighting->UpdateSampler(f_GBuffer, 5, "albedro");
			p_Lighting->UpdateSampler(f_GBuffer, 6, "position");
			p_Lighting->UpdateSampler(f_GBuffer, 7, "normals");
			p_Lighting->UpdateSampler(f_GBuffer, 8, "materials");
			// Debug view pipeline
			p_Debug->UpdateSampler(f_Depth, 1, "Depth_Attachment");
			p_Debug->UpdateSampler(f_GBuffer, 5, "albedro");
			p_Debug->UpdateSampler(f_GBuffer, 6, "position");
			p_Debug->UpdateSampler(f_GBuffer, 7, "normals");
			p_Debug->UpdateSampler(f_GBuffer, 8, "materials");

			p_GI->UpdateSampler(f_GBuffer, 0, "albedro");
			p_GI->UpdateSampler(f_GBuffer, 1, "position");
			p_GI->UpdateSampler(f_GBuffer, 2, "normals");
			p_GI->UpdateSampler(f_GBuffer, 3, "materials");
			p_GI->UpdateSampler(f_Depth, 4, "Depth_Attachment");

			p_GI->UpdateSampler(m_VCTParams.VoxelNormal, 5);
			p_GI->UpdateSampler(m_VCTParams.Radiance, 6);
			p_GI->UpdateSamplers(m_VCTParams.VoxelTexMipmap, 7);

			// Combination
			p_Combination->UpdateSampler(f_Lighting, 0);

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

				TextureCreateInfo texCI{};
				texCI.eFormat = TextureFormat::R32G32B32A32_SFLOAT;
				texCI.eAddressMode = AddressMode::CLAMP_TO_EDGE;
				texCI.bAnisotropyEnable = false;
				texCI.Width = viewportWidth;
				texCI.Height = viewportHeight;

				m_BloomTex.clear();
				m_BloomTex.resize(3);

				for (auto& tex : m_BloomTex)
				{
					tex = Texture::Create();
					tex->LoadAsStorage(&texCI);
				}
			}
		}
	}

	RendererDrawList::RendererDrawList()
	{
		m_AnimationJoints.resize(max_anim_joints);
		m_DrawList.resize(max_objects);
		s_Instance = this;
	}

	RendererDrawList::~RendererDrawList()
	{
		s_Instance = nullptr;
	}

	void RendererDrawList::SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, const Ref<Mesh>& mesh, const Ref<MeshView>& view)
	{
		//!s_Instance->m_Frustum.CheckSphere(pos) ||
		if (s_Instance->m_InstanceIndex >= max_objects)
		{
			return;
		}

		Material3D* material = view->GetMaterial(mesh->GetNodeIndex()).get();
		material = material == nullptr ? RendererStorage::GetDefaultMaterial().get() : material;

		auto& sMaterial = s_Instance->m_Packages[material];
		auto& instance = sMaterial.Instances[mesh];

		ObjectData* data = nullptr;
		if (instance.Index == instance.Objects.size()) { data = &instance.Objects.emplace_back(ObjectData()); }
		else { data = &instance.Objects[instance.Index]; }

		data->WorldPos = const_cast<glm::vec3*>(&pos);
		data->Rotation = const_cast<glm::vec3*>(&rotation);
		data->Scale = const_cast<glm::vec3*>(&scale);
		data->PBRHandle = view->GetPBRHandle(mesh->GetNodeIndex()).get();
		data->AnimController = view->GetAnimationController().get();

		instance.Index++;

		s_Instance->m_SceneAABB.MaxPoint(mesh->m_SceneAABB.MaxPoint());
		s_Instance->m_SceneAABB.MinPoint(mesh->m_SceneAABB.MinPoint());

		for (auto& sub : mesh->m_Childs)
		{
			SubmitMesh(pos, rotation, scale, sub, view);
		}
	}

	void RendererDrawList::SubmitDirLight(DirectionalLight* light)
	{
		s_Instance->m_DirLight = *light;

		if(s_Instance->m_DirLight.IsActive && s_Instance->m_DirLight.IsCastShadows)
			CalculateDepthMVP();
	}

	void RendererDrawList::SubmitPointLight(PointLight* light)
	{
		uint32_t index = s_Instance->m_PointLightIndex;
		if (index >= max_lights)
			return;

		s_Instance->m_PointLights[index] = *light;
		s_Instance->m_PointLightIndex++;
	}

	void RendererDrawList::SubmitSpotLight(SpotLight* light)
	{
		uint32_t index = s_Instance->m_SpotLightIndex;
		if (index >= max_lights)
			return;

		s_Instance->m_SpotLights[index] = *light;
		s_Instance->m_SpotLightIndex++;
	}

	void RendererDrawList::CalculateDepthMVP()
	{
		// Keep depth range as small as possible
		// for better shadow map precision
		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(s_Instance->m_DirLight.lightFOV, 1.0f, s_Instance->m_DirLight.zNear, s_Instance->m_DirLight.zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(s_Instance->m_DirLight.Direction), glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0f);

		s_Instance->m_DepthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	}

	void RendererDrawList::SetDefaultState()
	{
		ClearDrawList();

		s_Instance->m_DirLight = {};
	}

	void RendererDrawList::ClearDrawList()
	{
		s_Instance->m_Objects = 0;
		s_Instance->m_InstanceIndex = 0;
		s_Instance->m_PointLightIndex = 0;
		s_Instance->m_SpotLightIndex = 0;
		s_Instance->m_LastAnimationOffset = 0;
		s_Instance->m_RootOffsets.clear();
	}

	void RendererDrawList::ClearCache()
	{
		ClearDrawList();

		s_Instance->m_Packages.clear();
	}

	void RendererDrawList::BeginSubmit(SceneViewProjection* viewProj)
	{
		ClearDrawList();
		CalculateFrustum(viewProj);
	}

	void RendererDrawList::EndSubmit()
	{
		BuildDrawList();
	}

	void RendererDrawList::CalculateFrustum(SceneViewProjection* sceneViewProj)
	{
		s_Instance->m_SceneInfo = sceneViewProj;
		s_Instance->m_Frustum.Update(s_Instance->m_SceneInfo->Projection * s_Instance->m_SceneInfo->View);
	}

	void RendererDeferred::GBufferPass(SubmitInfo* info)
	{
		auto drawList = info->pDrawList;
		auto storage = info->pStorage;

		storage->m_DefaultMaterial->GetPipeline()->BeginRenderPass();
		{
			// SkyBox
			if (storage->m_State.bDrawSkyBox)
			{
				uint32_t state = storage->m_EnvironmentMap->IsDynamic();
				storage->p_Skybox->SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
				storage->p_Skybox->Draw(36);
			}

			// Grid
			if (storage->m_State.bDrawGrid)
			{
				storage->p_Grid->SubmitPushConstant(ShaderType::Vertex, sizeof(glm::mat4), &storage->m_GridModel);
				storage->p_Grid->DrawMeshIndexed(storage->m_GridMesh);
			}

			for (uint32_t i = 0; i < drawList->m_InstanceIndex; ++i)
			{
				auto& cmd = drawList->m_DrawList[i];

				for (auto& [material, package] : cmd.Packages)
				{
					material->SetCommandBuffer(storage->m_DefaultMaterial->GetCommandBuffer());
					material->OnPushConstant(package.Offset);
					material->OnDrawCommand(cmd.Mesh, &package);

					package.Reset();
				}
			}
		}
		storage->m_DefaultMaterial->GetPipeline()->EndRenderPass();
	}

	void RendererDeferred::LightingPass(SubmitInfo* info)
	{
		auto drawList = info->pDrawList;
		auto storage = info->pStorage;

		if (false)
		{
			storage->p_Lighting->BeginRenderPass();
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
				storage->p_Lighting->SubmitPushConstant(ShaderType::Fragment, sizeof(push_constant), &light_pc);
				storage->p_Lighting->Draw(3);
			}
			storage->p_Lighting->EndRenderPass();
		}

		auto& vct = storage->m_VCTParams;

		struct GIBuffer
		{
			struct Attenuation
			{
				float constant = 1.0f;
				float linear = 0.2f;
				float quadratic = 0.080f;
				float pad = 0.0f;
			};

			struct Light {

				glm::vec3 ambient = glm::vec3(1);;
				float angleInnerCone = 25.0f;
				glm::vec3 diffuse = glm::vec3(1);;
				float angleOuterCone = 35.0f;
				glm::vec3 specular = glm::vec3(1);
				float pad1 = 0.0f;
				glm::vec3 position = glm::vec3(0);
				float pad2 = 1.0f;
				glm::vec3 direction = glm::vec3(105.0f, 53.0f, 102.0f);
				uint32_t shadowingMethod = 2;
				Attenuation attenuation{};
			};

			glm::mat4 lightViewProjection;
			glm::vec3 cameraPosition;
			float lightBleedingReduction = 0.0f;
			glm::vec2 exponents = glm::vec2(40.0f, 5.0f);
			float voxelScale;
			int volumeDimension;
			glm::vec3 worldMaxPoint;
			float pad1;
			glm::vec3 worldMinPoint;
			float pad2;
			Light directionalLight{};

		} static giBuffer{};


		giBuffer.lightViewProjection = drawList->m_DepthMVP;
		giBuffer.cameraPosition = drawList->m_SceneInfo->CamPos;
		giBuffer.worldMinPoint = drawList->m_SceneAABB.MinPoint();
		giBuffer.worldMaxPoint = drawList->m_SceneAABB.MaxPoint();
		giBuffer.voxelScale = 1.0f / vct.VolumeGridSize;
		giBuffer.volumeDimension = vct.VolumeDimension;

		storage->p_GI->SubmitBuffer(304, sizeof(GIBuffer), &giBuffer);

		storage->p_GI->BeginRenderPass();
		{
			storage->p_GI->Draw(3);
		}
		storage->p_GI->EndRenderPass();
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

		if (drawList->m_DirLight.IsActive && drawList->m_DirLight.IsCastShadows)
		{
#ifndef OPENGL_IMPL
			VkCommandBuffer cmdBuffer = (VkCommandBuffer)storage->p_DepthPass->GetCommandBuffer();
#endif
			storage->p_DepthPass->BeginRenderPass();
			{
#ifndef OPENGL_IMPL
				// Set depth bias (aka "Polygon offset")
				// Required to avoid shadow mapping artifacts
				vkCmdSetDepthBias(cmdBuffer, 1.25f, 0.0f, 1.75f);
#endif
				pushConstant.DepthMVP = drawList->m_DepthMVP;

				for (uint32_t i = 0; i < drawList->m_InstanceIndex; ++i)
				{
					auto& cmd = drawList->m_DrawList[i];
					for (auto& [material, package] : cmd.Packages)
					{
						pushConstant.DataOffset = package.Offset;

						storage->p_DepthPass->SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &pushConstant);
						storage->p_DepthPass->DrawMeshIndexed(cmd.Mesh, package.Instances);
					}
				}
			}
			storage->p_DepthPass->EndRenderPass();
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

		auto TEXTURE_STORAGE_SET = [&](Ref<Texture>& tex, uint32_t mip = 0, uint32_t binding = 0)
		{
			auto descriptor = tex->Cast<VulkanTexture>()->GetMipImageView(mip);
			storage->p_Bloom->Cast<VulkanComputePipeline>()->GeDescriptor(descriptorIndex).UpdateImageResource(binding, descriptor, binding == 0 ? true : false);
		};

		auto TEXTURE_SET_SCENE = [&](Ref<Texture>& tex)
		{
			storage->p_Bloom->Cast<VulkanComputePipeline>()->GeDescriptor(descriptorIndex).Update2DSamplers({ tex }, 1);
		};

		const auto TEXTURE_BLOOM_SET_FROM_SECENE = [&](uint32_t binding = 2)
		{
			const auto& descriptor = storage->f_Lighting->Cast<VulkanFramebuffer>()->GetAttachment(0)->ImageInfo;
			storage->p_Bloom->Cast<VulkanComputePipeline>()->GeDescriptor(descriptorIndex).UpdateImageResource(binding, descriptor);
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

		storage->p_Bloom->BeginCompute(info->pCmdStorage);
		{
			// Prefilter
			{
				TEXTURE_STORAGE_SET(bloomTex[0]);
				TEXTURE_BLOOM_SET_FROM_SECENE(1);

				workGroupsX = bloomTex[0]->GetInfo().Width / storage->m_BloomComputeWorkgroupSize;
				workGroupsY = bloomTex[0]->GetInfo().Height / storage->m_BloomComputeWorkgroupSize;

				storage->p_Bloom->SetDescriptorIndex(descriptorIndex);
				storage->p_Bloom->SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom->Dispatch(workGroupsX, workGroupsY, 1);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[0]->Cast<VulkanTexture>()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[0]->GetMips(), 0, 1 };
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
			uint32_t mips = storage->m_BloomTex[0]->GetMips() - 2;
			for (uint32_t i = 1; i < mips; i++)
			{
				descriptorIndex++;
				auto [mipWidth, mipHeight] = bloomTex[0]->GetMipSize(i);

				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)storage->m_BloomComputeWorkgroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)storage->m_BloomComputeWorkgroupSize);

				TEXTURE_STORAGE_SET(bloomTex[1], i);
				TEXTURE_SET_SCENE(bloomTex[0]);
				TEXTURE_BLOOM_SET_FROM_SECENE();

				bloomComputePushConstants.LOD = i - 1.0f;
				storage->p_Bloom->SetDescriptorIndex(descriptorIndex);
				storage->p_Bloom->SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom->Dispatch(workGroupsX, workGroupsY, 1);

				{
					VkImageMemoryBarrier imageMemoryBarrier = {};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageMemoryBarrier.image = bloomTex[1]->Cast<VulkanTexture>()->GetVkImage();
					imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[1]->GetMips(), 0, 1 };
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

				TEXTURE_STORAGE_SET(bloomTex[0], i);
				TEXTURE_SET_SCENE(bloomTex[1]);
				TEXTURE_BLOOM_SET_FROM_SECENE();

				bloomComputePushConstants.LOD = (float)i;
				storage->p_Bloom->SetDescriptorIndex(descriptorIndex);
				storage->p_Bloom->SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom->Dispatch(workGroupsX, workGroupsY, 1);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[0]->Cast<VulkanTexture>()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[0]->GetMips(), 0, 1 };
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

				TEXTURE_STORAGE_SET(bloomTex[2], mips - 2);
				TEXTURE_SET_SCENE(bloomTex[0]);
				TEXTURE_BLOOM_SET_FROM_SECENE();

				bloomComputePushConstants.LOD--;
				storage->p_Bloom->SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);

				auto [mipWidth, mipHeight] = bloomTex[2]->GetMipSize(mips - 2);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)storage->m_BloomComputeWorkgroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)storage->m_BloomComputeWorkgroupSize);

				storage->p_Bloom->SetDescriptorIndex(descriptorIndex);
				storage->p_Bloom->Dispatch(workGroupsX, workGroupsY, 1);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[2]->Cast<VulkanTexture>()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,   bloomTex[2]->GetMips(), 0, 1 };
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
				auto [mipWidth, mipHeight] = bloomTex[2]->GetMipSize(mip);
				workGroupsX = (uint32_t)glm::ceil((float)mipWidth / (float)storage->m_BloomComputeWorkgroupSize);
				workGroupsY = (uint32_t)glm::ceil((float)mipHeight / (float)storage->m_BloomComputeWorkgroupSize);

				TEXTURE_STORAGE_SET(bloomTex[2], mip);
				TEXTURE_SET_SCENE(bloomTex[0]);
				storage->p_Bloom->Cast<VulkanComputePipeline>()->GeDescriptor(descriptorIndex).Update2DSamplers({ bloomTex[2] }, 2, false);

				bloomComputePushConstants.LOD = (float)mip;
				storage->p_Bloom->SetDescriptorIndex(descriptorIndex);
				storage->p_Bloom->SubmitPushConstant(sizeof(bloomComputePushConstants), &bloomComputePushConstants);
				storage->p_Bloom->Dispatch(workGroupsX, workGroupsY, 1);

				VkImageMemoryBarrier imageMemoryBarrier = {};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = bloomTex[2]->Cast<VulkanTexture>()->GetVkImage();
				imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, bloomTex[2]->GetMips(), 0, 1 };
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

		storage->p_Bloom->SetDescriptorIndex(0);
	}

	void RendererDeferred::VoxelizationPass(SubmitInfo* info) // TEMP
	{
		auto drawList = info->pDrawList;
		auto storage = info->pStorage;
		auto& vct = storage->m_VCTParams;

		static bool voxilize = false;
		auto axisSize = drawList->m_SceneAABB.Extent() * 2.0f;
		auto center = drawList->m_SceneAABB.Center();
		auto minPoint = drawList->m_SceneAABB.MinPoint();
		auto maxPoint = drawList->m_SceneAABB.MaxPoint();

		if (!voxilize)
		{
			voxilize = true;

			{
				vct.VoxelCount = vct.VolumeDimension * vct.VolumeDimension * vct.VolumeDimension;

				vct.VolumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
				vct.VoxelSize = vct.VolumeGridSize / vct.VolumeDimension;

				auto halfSize = vct.VolumeGridSize / 2.0f;

				// projection matrices
				auto projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, vct.VolumeGridSize);

				// view matrices
				vct.ubo.viewProjections[0] = lookAt(center + glm::vec3(halfSize, 0.0f, 0.0f), center, glm::vec3(0.0f, 1.0f, 0.0f));
				vct.ubo.viewProjections[1] = lookAt(center + glm::vec3(0.0f, halfSize, 0.0f), center, glm::vec3(0.0f, 0.0f, -1.0f));
				vct.ubo.viewProjections[2] = lookAt(center + glm::vec3(0.0f, 0.0f, halfSize), center, glm::vec3(0.0f, 1.0f, 0.0f));

				for (uint32_t i = 0; i < 3; i++)
				{
					auto& matrix = vct.ubo.viewProjections[i];

					matrix = projection * matrix;
					vct.ubo.viewProjectionsI[i++] = inverse(matrix);
				}

				vct.ubo.volumeDimension = vct.VolumeDimension;
				vct.ubo.voxelScale = 1.0f / vct.VolumeGridSize;
				vct.ubo.worldMinPoint = minPoint;
				vct.ubo.flagStaticVoxels = 2;

				storage->p_Voxelization->SubmitBuffer(202, sizeof(RendererStorage::VoxelConeTracing::UBO), &vct.ubo);
			}

			storage->p_Voxelization->BeginRenderPass();
			{
				for (uint32_t i = 0; i < drawList->m_InstanceIndex; ++i)
				{
					auto& cmd = drawList->m_DrawList[i];

					for (auto& [material, package] : cmd.Packages)
					{
						storage->p_Voxelization->SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &package.Offset);
						storage->p_Voxelization->DrawMeshIndexed(cmd.Mesh, package.Instances);
					}
				}
			}
			storage->p_Voxelization->EndRenderPass();

			{
				struct ProcessData
				{
					glm::vec2 exponents = glm::vec2(40.0f, 5.0f);
					float lightBleedingReduction = 0.0f;
					float pad1;

					glm::mat4 lightViewProjection;

					float voxelSize;
					float voxelScale;
					int volumeDimension;
					uint32_t storeVisibility = 0;

					glm::vec3 worldMinPoint;
					float traceShadowHit;

					uint32_t normalWeightedLambert;
					glm::vec3 pad2;

				} static process_data{};

				process_data.lightViewProjection = drawList->m_DepthMVP;
				process_data.voxelSize = storage->m_VCTParams.VoxelSize;
				process_data.voxelScale = 1.0f / storage->m_VCTParams.VolumeGridSize;
				process_data.volumeDimension = storage->m_VCTParams.VolumeDimension;
				process_data.worldMinPoint = minPoint;
				process_data.traceShadowHit = 0.5f;
				process_data.normalWeightedLambert = 1;

				storage->p_InjectRadiance->SubmitBuffer(205, sizeof(RendererStorage::VoxelConeTracing::LightData), &storage->m_VCTParams.dirLight);
				storage->p_InjectRadiance->SubmitBuffer(206, sizeof(ProcessData), &process_data);

				auto workGroups = static_cast<unsigned>(glm::ceil(storage->m_VCTParams.VolumeDimension / 8.0f));

				storage->p_InjectRadiance->BeginCompute(info->pCmdStorage);
				storage->p_InjectRadiance->Dispatch(workGroups, workGroups, workGroups);

				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					0, nullptr);
			}

			constexpr auto mip_map_base = [](RendererStorage* storage, SubmitInfo* info, RendererStorage::VoxelConeTracing& vct)
			{
				int halfDimension = vct.VolumeDimension / 2;
				auto workGroups = static_cast<unsigned int>(ceil(halfDimension / 8));

				storage->p_MipmapBase->BeginCompute(info->pCmdStorage);
				storage->p_MipmapBase->SubmitPushConstant(sizeof(int), &halfDimension);
				storage->p_MipmapBase->Dispatch(workGroups, workGroups, workGroups);

				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					0, nullptr);
			};

			constexpr auto mip_map_volume = [](RendererStorage* storage, SubmitInfo* info, RendererStorage::VoxelConeTracing& vct)
			{
				auto mipDimension = vct.VolumeDimension / 4;
				auto mipLevel = 0;

				while (mipDimension >= 1)
				{
					struct push_constant
					{
						glm::vec3 mipDimension;
						int mipLevel;
					} pc;

					pc.mipDimension = glm::vec3(mipDimension, mipDimension, mipDimension);
					pc.mipLevel = mipLevel;

					auto workGroups = static_cast<unsigned>(glm::ceil(mipDimension / 8.0f));

					storage->p_MipmapVolume->BeginCompute(info->pCmdStorage);
					storage->p_MipmapVolume->SubmitPushConstant(sizeof(push_constant), &pc);
					storage->p_MipmapVolume->Dispatch(workGroups, workGroups, workGroups);

					vkCmdPipelineBarrier(
						info->pCmdStorage->Buffer,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						0,
						0, nullptr,
						0, nullptr,
						0, nullptr);

					mipLevel++;
					mipDimension /= 2;
				}
			};

			mip_map_base(storage, info, vct);
			mip_map_volume(storage, info, vct);

			// Inject Propagation
			{
				struct push_constant
				{
					float maxTracingDistanceGlobal = 1.0f;
					int volumeDimension;
					uint32_t checkBoundaries =  0;

				} pc{};

				pc.volumeDimension = vct.VolumeDimension;

				// inject at level 0 of textur
				auto workGroups = static_cast<unsigned>(glm::ceil(vct.VolumeDimension / 8.0f));

				storage->p_InjectPropagation->BeginCompute(info->pCmdStorage);
				storage->p_InjectPropagation->SubmitPushConstant(sizeof(push_constant), &pc);
				storage->p_InjectPropagation->Dispatch(workGroups, workGroups, workGroups);

				vkCmdPipelineBarrier(
					info->pCmdStorage->Buffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					0, nullptr);

				mip_map_base(storage, info, vct);
				mip_map_volume(storage, info, vct);
			}

		}

		return;

		// Draw Voxels
		{
			struct DrawVoxels
			{
				uint32_t volumeDimension;
				float voxelSize;
				glm::vec2 pad1;

				glm::vec4 colorChannels = glm::vec4(1.0f);
				glm::vec4 frustumPlanes[6];

				glm::vec3 worldMinPoint;
				float pad2;

				glm::mat4 modelViewProjection;

			} static drawVoxels{};

			drawVoxels.voxelSize = storage->m_VCTParams.VoxelSize;
			drawVoxels.worldMinPoint = minPoint;

			uint32_t  drawMipLevel = 0;
			uint32_t  vDimension = static_cast<unsigned>(storage->m_VCTParams.VolumeDimension / pow(2.0f, drawMipLevel));
			float vSize = storage->m_VCTParams.VolumeGridSize / vDimension;
			auto model = translate(drawVoxels.worldMinPoint) * scale(glm::vec3(vSize));

			drawVoxels.volumeDimension = vDimension;
			drawVoxels.modelViewProjection = (drawList->m_SceneInfo->Projection * drawList->m_SceneInfo->View) * model;

			{
				auto& frustum = drawList->GetFrustum();

				uint32_t index = 0;
				for (auto& plane : frustum.GetPlanes())
				{
					drawVoxels.frustumPlanes[index] = plane;
					index++;
				}
			}

			storage->p_VoxelView->UpdateSampler(storage->m_VCTParams.Radiance, 0, true);
			storage->p_VoxelView->SubmitBuffer(202, sizeof(DrawVoxels), &drawVoxels);

			storage->p_VoxelView->BeginRenderPass();
			storage->p_VoxelView->Draw(storage->m_VCTParams.VoxelCount);
			storage->p_VoxelView->EndRenderPass();
		}

	}

	bool RendererDeferred::DebugViewPass(SubmitInfo* info)
	{
		auto storage = info->pStorage;

		// Debug View
		if (storage->m_State.eDebugView != DebugViewFlags::None)
		{
			storage->p_Debug->BeginRenderPass();
			{
				uint32_t state = (uint32_t)storage->m_State.eDebugView;
				storage->p_Debug->SubmitPushConstant(ShaderType::Fragment, sizeof(uint32_t), &state);
				storage->p_Debug->Draw(3);
			}
			storage->p_Debug->EndRenderPass();
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
			storage->p_Combination->UpdateSampler(storage->m_BloomTex[2], 1);
		}

		if (info->pClearInfo->bClear)
		{
			storage->p_Combination->BeginRenderPass();
			storage->p_Combination->ClearColors(info->pClearInfo->color);
			storage->p_Combination->EndRenderPass();
		}

		storage->p_Combination->BeginRenderPass();
		{

			storage->p_Combination->SubmitPushConstant(ShaderType::Fragment, sizeof(push_constant), &push_constant);
			storage->p_Combination->Draw(3);
		}
		storage->p_Combination->EndRenderPass();
	}

	void RendererDeferred::UpdateUniforms(SubmitInfo* info)
	{
		info->pStorage->UpdateUniforms(info->pDrawList, info->pStorage->f_Main);
	}
}