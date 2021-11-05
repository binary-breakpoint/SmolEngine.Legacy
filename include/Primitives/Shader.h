#pragma once
#include "Common/Memory.h"
#include "Common/Flags.h"
#include "Primitives/PrimitiveBase.h"

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
		size_t               Size = 0;
		size_t               Offset = 0;
		std::string          Name = "";
	};

	struct PushContantData
	{
		uint32_t             Offset = 0;
		uint32_t             Size = 0;
		ShaderType           Stage = ShaderType::Vertex;
	};

	struct ACStructure
	{
		ShaderType           Stage = ShaderType::RayGen;
		uint32_t             ArraySize = 0;
		std::string          Name = "";
	};

	struct ShaderBuffer
	{
		ShaderType           Stage = ShaderType::Vertex;
		BufferType           Type = BufferType::Uniform;
		uint32_t             BindingPoint = 0;
		std::string          Name = "";
		std::string          ObjectName = "";
		size_t               Size = 0;
		std::vector<Uniform> Uniforms;

	};

	struct SamplerBuffer
	{
		ShaderType           Stage;
		uint32_t             Location = 0;
		uint32_t             BindingPoint = 0;
		uint32_t             Dimension = 0;
		uint32_t             ArraySize = 0;
	};

	struct ReflectionData
	{
		PushContantData                      PushConstant{};
		std::map<uint32_t, SamplerBuffer>    ImageSamplers;
		std::map<uint32_t, ACStructure>      ACStructures;
		std::map<uint32_t, SamplerBuffer>    StorageImages;
		std::map<uint32_t, ShaderBuffer>     Buffers;

		void Clean()
		{
			ImageSamplers.clear();
			StorageImages.clear();
			Buffers.clear();
			ACStructures.clear();

			PushConstant = {};
		}
	};

	struct ShaderBufferInfo
	{
		bool   bStatic = false;
		bool   bGlobal = true;
		void*  Data = nullptr;
		size_t Size = 0;
    };

	struct ShaderCreateInfo
	{
		std::map<ShaderType, std::string> Stages;
		std::map<uint32_t, ShaderBufferInfo> Buffers;
	};

	class Shader: public PrimitiveBase
	{
	public:
		virtual ~Shader() = default;

		virtual bool           Build(ShaderCreateInfo* info) = 0;
		virtual bool           Realod() = 0;
		const ReflectionData&  GetReflection() const;
		uint32_t               GetACBindingPoint() const;
		ShaderCreateInfo&      GetCreateInfo();
		static Ref<Shader>     Create();

	protected:
		bool                   BuildBase(ShaderCreateInfo* info);
		void                   Reflect(const std::vector<uint32_t>& binaryData, ShaderType type);

	protected:
		bool                   m_RTPipeline = false;
		uint32_t               m_ACBindingPoint = 0;
		ShaderCreateInfo       m_CreateInfo{};
		ReflectionData         m_ReflectData{};

		std::map<ShaderType, 
		std::vector<uint32_t>> m_Binary;
	};
}