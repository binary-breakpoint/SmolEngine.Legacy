#pragma once
#include "GraphicsContext.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanPipeline.h"
#endif

#include "Common/Common.h"

#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/Shader.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class Framebuffer;
	class CubeMap;
	class Mesh;
	class Texture;

	enum class BlendFactor : uint16_t
	{
		NONE = 0,
		ONE,
		ZERO,
		SRC_ALPHA,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		CONSTANT_COLOR,
		ONE_MINUS_CONSTANT_COLOR,
		CONSTANT_ALPHA,
		ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
		SRC1_COLOR,
		ONE_MINUS_SRC1_COLOR,
		SRC1_ALPHA,
		ONE_MINUS_SRC1_ALPHA,
	};

	enum class BlendOp : uint16_t
	{
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX
	};

	enum class DrawMode : uint16_t
	{
		Triangle,
		Line,
		Fan,
		Triangle_Strip,
	};

	enum class PolygonMode : uint16_t
	{
		Fill,
		Line,
		Point
	};

	enum class CullMode : uint16_t
	{
		None,
		Back,
		Front
	};

	enum class PipelineCreateResult : uint16_t
	{
		SUCCESS,
		ERROR_INVALID_CREATE_INFO,
		ERROR_PIPELINE_NOT_INVALIDATED,
		ERROR_PIPELINE_NOT_CREATED,
		ERROR_SHADER_NOT_RELOADED
	};

	class Framebuffer;
	struct GraphicsPipelineCreateInfo
	{
		BlendFactor                          eSrcColorBlendFactor = BlendFactor::NONE;
		BlendFactor                          eDstColorBlendFactor = BlendFactor::NONE;
		BlendFactor                          eSrcAlphaBlendFactor = BlendFactor::NONE;
		BlendFactor                          eDstAlphaBlendFactor = BlendFactor::NONE;
		BlendOp                              eColorBlendOp = BlendOp::ADD;
		BlendOp                              eAlphaBlendOp = BlendOp::ADD;
		CullMode                             eCullMode = CullMode::Back;
		PolygonMode                          ePolygonMode = PolygonMode::Fill;

		bool                                 bDepthTestEnabled = true;
		bool                                 bDepthWriteEnabled = true;
		bool                                 bDepthBiasEnabled = false;
		float                                MinDepth = 0.0f;
		float                                MaxDepth = 1.0f;
		int32_t                              DescriptorSets = 1;
		int32_t                              StageCount = -1;

		std::string                          PipelineName = "";
		GraphicsPipelineShaderCreateInfo     ShaderCreateInfo = {};
		std::vector<DrawMode>                PipelineDrawModes = { DrawMode::Triangle };
		std::vector<VertexInputInfo>         VertexInputInfos;
		std::vector<Framebuffer*>            TargetFramebuffers;
	};

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline() = default;
		~GraphicsPipeline();

		PipelineCreateResult Create(GraphicsPipelineCreateInfo* pipelineInfo);
		PipelineCreateResult Reload();

		void ClearColors(const glm::vec4& clearColors = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
		void BeginRenderPass(bool flip = false);
		void EndRenderPass();
		void BeginCommandBuffer(bool isMainCmdBufferInUse = false);
		void EndCommandBuffer();
		void ResetStates();
		void Destroy();

		void DrawIndexed(uint32_t vbIndex = 0, uint32_t ibIndex = 0);
		void DrawIndexed(VertexBuffer* vb, IndexBuffer* ib);
		void Draw(VertexBuffer* vb, uint32_t vertextCount);
		void Draw(uint32_t vertextCount, uint32_t vertexBufferIndex = 0);
		void DrawMeshIndexed(Mesh* mesh, uint32_t instances = 1);
		void DrawMesh(Mesh* mesh, uint32_t instances = 1);

		bool SubmitBuffer(uint32_t bindingPoint, size_t size, const void* data, uint32_t offset = 0);
		void SubmitPushConstant(ShaderType shaderStage, size_t size, const void* data);
		void SetFramebuffers(const std::vector<Framebuffer*>& fb);
		void SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vb);
		void SetIndexBuffers(const std::vector<Ref<IndexBuffer>>& ib);
#ifndef FROSTIUM_OPENGL_IMPL
		bool UpdateVulkanImageDescriptor(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo);
#endif
		bool UpdateSamplers(const std::vector<Texture*>& textures, uint32_t bindingPoint);
		bool UpdateSampler(Texture* tetxure, uint32_t bindingPoint);
		bool UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex = 0);
		bool UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, const std::string& attachmentName);
		bool UpdateCubeMap(Texture* cubeMap, uint32_t bindingPoint);

		void SetDescriptorIndex(uint32_t value);
		void SetDrawMode(DrawMode mode);
		void SetFramebufferIndex(uint32_t index);
		void SetFramebufferAttachmentIndex(uint32_t index);

#ifndef FROSTIUM_OPENGL_IMPL
		const VkPipeline& GetVkPipeline(DrawMode mode);
		const VulkanShader* GetVulkanShader();
		VkCommandBuffer GetVkCommandBuffer() const;
		void SetCommandBuffer(VkCommandBuffer cmdBuffer);
#else
		void BindOpenGLShader();
#endif
	private:
		bool IsPipelineCreateInfoValid(const GraphicsPipelineCreateInfo* pipelineInfo);

	private:

#ifndef FROSTIUM_OPENGL_IMPL
		CommandBufferStorage              m_CmdStorage{};
		VulkanPipeline                    m_VulkanPipeline = {};
		VkCommandBuffer                   m_CommandBuffer = nullptr;
		bool                              m_IsMainCmdBufferInUse = false;
#endif
		Ref<VertexArray>                  m_VextexArray = nullptr;
		Ref<Shader>                       m_Shader = nullptr;
		GraphicsContext*                  m_GraphicsContext = nullptr;
		DrawMode                          m_DrawMode = DrawMode::Triangle;
		uint32_t                          m_DescriptorIndex = 0;
		uint32_t                          m_FBIndex = 0;
		uint32_t                          m_FBattachmentIndex = 0;
		GraphicsPipelineCreateInfo        m_PiplineCreateInfo;
		std::vector<Ref<VertexBuffer>>    m_VertexBuffers;
		std::vector<Ref<IndexBuffer>>     m_IndexBuffers;
	};
}