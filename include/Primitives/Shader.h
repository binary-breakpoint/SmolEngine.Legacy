#pragma once
#include "Common/Common.h"

#ifdef FROSTIUM_OPENGL_IMPL
#include "Backends/OpenGL/OpenglShader.h"
#else
#include "Backends/Vulkan/VulkanShader.h"
#endif

#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{

	enum class BufferType : uint16_t
	{
		Uniform,
		Storage
};

	enum class ElementType : uint16_t
	{

	};

	struct Uniform
	{
		//ElementType                      Type;
		size_t                           Size = 0;
		size_t                           Offset = 0;
		std::string                      Name = "";
	};

	struct PushContantData
	{
		uint32_t                         Offset = 0;
		uint32_t                         Size = 0;
		ShaderType                       Stage = ShaderType::Vertex;
	};

	struct ShaderBuffer
	{
		ShaderType                       Stage = ShaderType::Vertex;
		BufferType                       Type = BufferType::Uniform;
		uint32_t                         BindingPoint = 0;
		std::string                      Name = "";
		std::string                      ObjectName = "";
		size_t                           Size = 0;
		std::vector<Uniform>             Uniforms;

	};

	struct UniformResource
	{
		ShaderType                       Stage;
		uint32_t                         Location = 0;
		uint32_t                         BindingPoint = 0;
		uint32_t                         Dimension = 0;
		uint32_t                         ArraySize = 0;
	};

	struct ReflectionData
	{
		PushContantData                                PushConstant{};
		std::unordered_map<uint32_t, UniformResource>  Resources;
		std::unordered_map<uint32_t, ShaderBuffer>     Buffers;

		void Clean()
		{
			Resources.clear();
			Buffers.clear();
			PushConstant = {};
		}
	};

	struct ShaderBufferInfo
	{
		bool   bStatic = false;
		bool   bGlobal = true;
		void* Data = nullptr;
		size_t Size = 0;
    };

	struct GraphicsPipelineShaderCreateInfo
	{
		std::unordered_map<ShaderType, std::string>      FilePaths;
		std::unordered_map<uint32_t, ShaderBufferInfo>  BufferInfos;
	};

#ifdef  FROSTIUM_OPENGL_IMPL
	class Shader : OpenglShader
#else
	class Shader: VulkanShader
#endif
	{
	public:
		void                                Bind() const;
		void                                UnBind() const;
		bool                                Realod();
#ifndef FROSTIUM_OPENGL_IMPL	                				                
		VulkanShader*                       GetVulkanShader() { return dynamic_cast<VulkanShader*>(this); }
#endif
		static void                         Create(GraphicsPipelineShaderCreateInfo* shaderCI, Shader* shader);

	private:				                
		void                                Reflect(const std::vector<uint32_t>& binaryData, ShaderType type);

	private:
		GraphicsPipelineShaderCreateInfo    m_CreateInfo{};
		ReflectionData                      m_ReflectData{};

		friend class GraphicsPipeline;
	};
}