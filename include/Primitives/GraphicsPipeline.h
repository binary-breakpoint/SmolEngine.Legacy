#pragma once
#include "GraphicsContext.h"
#include "Common/Memory.h"
#include "Common/Vertex.h"

#include "Primitives/PrimitiveBase.h"
#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/Shader.h"

namespace SmolEngine
{
	class Framebuffer;
	class CubeMap;
	class Mesh;
	class Texture;
	class GraphicsContext;

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

	struct GraphicsPipelineCreateInfo
	{
		BlendFactor                   eSrcColorBlendFactor = BlendFactor::NONE;
		BlendFactor                   eDstColorBlendFactor = BlendFactor::NONE;
		BlendFactor                   eSrcAlphaBlendFactor = BlendFactor::NONE;
		BlendFactor                   eDstAlphaBlendFactor = BlendFactor::NONE;
		BlendOp                       eColorBlendOp = BlendOp::ADD;
		BlendOp                       eAlphaBlendOp = BlendOp::ADD;
		CullMode                      eCullMode = CullMode::Back;
		PolygonMode                   ePolygonMode = PolygonMode::Fill;
									  
		bool                          bDepthTestEnabled = true;
		bool                          bDepthWriteEnabled = true;
		bool                          bDepthBiasEnabled = false;
		bool                          bPrimitiveRestartEnable = false;
		float                         MinDepth = 0.0f;
		float                         MaxDepth = 1.0f;
		int32_t                       DescriptorSets = 1;
		int32_t                       StageCount = -1;
									  
		std::string                   PipelineName = "";
		ShaderCreateInfo              ShaderCreateInfo = {};
		std::vector<DrawMode>         PipelineDrawModes = { DrawMode::Triangle };
		std::vector<VertexInputInfo>  VertexInputInfos;
		std::vector<Ref<Framebuffer>> TargetFramebuffers;
	};

	class GraphicsPipeline: public PrimitiveBase
	{
	public:
		GraphicsPipeline() = default;
		virtual ~GraphicsPipeline() = default;
							              
		virtual bool                      Build(GraphicsPipelineCreateInfo* info) = 0;
		virtual void                      ClearColors(const glm::vec4& color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) = 0;
		virtual void                      BeginRenderPass(bool flip = false) = 0;
		virtual void                      BeginCommandBuffer(bool batchCmd = false) = 0;
		virtual void                      EndCommandBuffer() = 0;
		virtual void                      EndRenderPass() = 0;
		virtual void                      Reload() {};
					                      
		virtual void                      DrawIndexed(uint32_t vbIndex = 0, uint32_t ibIndex = 0) = 0;
		virtual void                      DrawIndexed(Ref<VertexBuffer>& vb, Ref<IndexBuffer>& ib) = 0;
		virtual void                      Draw(Ref<VertexBuffer>& vb, uint32_t vertextCount) = 0;
		virtual void                      Draw(uint32_t vertextCount, uint32_t vbIndex = 0) = 0;
		virtual void                      DrawMeshIndexed(Ref<Mesh>& mesh, uint32_t instances = 1) = 0;
		virtual void                      DrawMesh(Ref<Mesh>& mesh, uint32_t instances = 1) = 0;
					                      
		virtual bool                      SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) = 0;
		virtual void                      SubmitPushConstant(ShaderType stage, size_t size, const void* data) {};
					                      
		virtual bool                      UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t binding, bool storageImage = false) = 0;
		virtual bool                      UpdateSampler(Ref<Texture>& texture, uint32_t binding, bool storageImage = false) = 0;
		virtual bool                      UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t binding, uint32_t attachmentIndex = 0) = 0;
		virtual bool                      UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t binding, const std::string& attachmentName) = 0;
		virtual bool                      UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t binding) = 0;
					                      
		virtual void                      BindPipeline() {};
		virtual void                      BindDescriptors() {};
		virtual void                      BindIndexBuffer(uint32_t index = 0) {};
		virtual void                      BindVertexBuffer(uint32_t index = 0) {};

		static Ref<GraphicsPipeline>      Create();
		void                              Reset();
		void                              SetDescriptorIndex(uint32_t value);
		void                              SetDrawMode(DrawMode mode);
		void                              SetFramebufferIndex(uint32_t index);
		void                              SetFramebufferAttachmentIndex(uint32_t index);
		void                              SetFramebuffers(const std::vector<Ref<Framebuffer>>& fb);
		void                              SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vb);
		void                              SetIndexBuffers(const std::vector<Ref<IndexBuffer>>& ib);

	private:
		bool                              BuildBase(GraphicsPipelineCreateInfo* pipelineInfo);
		bool                              IsPipelineCreateInfoValid(const GraphicsPipelineCreateInfo* pipelineInfo);

	private:
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

		friend class VulkanPipeline;
	};
}