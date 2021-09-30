#pragma once
#include "Common/Common.h"
#include "Common/Flags.h"

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglShader.h"
#else
#include "Backends/Vulkan/VulkanShader.h"
#endif

#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

namespace SmolEngine
{

	enum class BufferType : uint16_t
	{
		Uniform,
		Storage
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

	struct SamplerBuffer
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
		std::unordered_map<uint32_t, SamplerBuffer>    ImageSamplers;
		std::unordered_map<uint32_t, SamplerBuffer>    StorageImages;
		std::unordered_map<uint32_t, ShaderBuffer>     Buffers;

		void Clean()
		{
			ImageSamplers.clear();
			StorageImages.clear();
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

	struct ShaderCreateInfo
	{
		std::unordered_map<ShaderType, std::string>      FilePaths;
		std::unordered_map<uint32_t, ShaderBufferInfo>   BufferInfos;
	};

#ifdef  OPENGL_IMPL
	class Shader : OpenglShader
#else
	class Shader: VulkanShader
#endif
	{
	public:
		void                 Bind() const;
		void                 UnBind() const;
		bool                 Realod();
#ifndef OPENGL_IMPL  				                
		VulkanShader*        GetVulkanShader() { return dynamic_cast<VulkanShader*>(this); }
#endif
		static void          Create(ShaderCreateInfo* shaderCI, Shader* shader);

		template<typename T>
		T* Cast() { return dynamic_cast<T*>(this); }

	private:				 
		void                 Reflect(const std::vector<uint32_t>& binaryData, ShaderType type);

	private:
		ShaderCreateInfo     m_CreateInfo{};
		ReflectionData       m_ReflectData{};

		friend class GraphicsPipeline;
	};
}