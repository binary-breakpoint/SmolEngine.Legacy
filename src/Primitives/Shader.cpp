#include "stdafx.h"
#include "Primitives/Shader.h"

#include "Common/DebugLog.h"
#include "Tools/Utils.h"

VKBP_DISABLE_WARNINGS()
#include <glslang/include/glslang/Public/ShaderLang.h>
#include <glslang/include/StandAlone/ResourceLimits.h>
#include <glslang/include/SPIRV/GlslangToSpv.h>
#include <glslang/include/SPIRV/GLSL.std.450.h>
#include <glslang/include/glslang/Include/ShHandle.h>
#include <glslang/include/glslang/OSDependent/osinclude.h>
VKBP_ENABLE_WARNINGS()

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglShader.h"
#else
#include "Backends/Vulkan/VulkanShader.h"
#endif

namespace SmolEngine
{
	EShLanguage GetShaderType(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::Vertex:      return EShLangVertex;
		case ShaderType::Fragment:    return EShLangFragment;
		case ShaderType::Geometry:    return EShLangGeometry;
		case ShaderType::Compute:     return EShLangCompute;
		case ShaderType::RayGen:      return EShLangRayGen;
		case ShaderType::RayAnyHit:   return EShLangAnyHit;
		case ShaderType::RayCloseHit: return EShLangClosestHit;
		case ShaderType::RayMiss:     return EShLangMiss;
		}

		return EShLangVertex;
	}

	void LoadSPIRV(const std::string& path, std::vector<uint32_t>& binaries)
	{
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (in.is_open())
		{
			in.seekg(0, std::ios::end);
			auto size = in.tellg();
			in.seekg(0, std::ios::beg);

			binaries.resize(size / sizeof(uint32_t));
			in.read((char*)binaries.data(), size);
		}
	}

	void CompileSPIRV(const std::string& str, std::vector<uint32_t>& binaries, ShaderType type, bool isSource)
	{
		// Initialize glslang library.
		glslang::InitializeProcess();

		std::string path = str;
		std::string src = str;

		if (!isSource)
		{
			std::ifstream file(path);
			std::stringstream buffer;
			if (!file)
			{
				throw std::runtime_error("Could not load file " + path);
				abort();
			}

			buffer << file.rdbuf();
			src = buffer.str();
			file.close();
		}

		// Compile
		{
			const char* file_name_list[1] = { "" };
			const char* shader_source = reinterpret_cast<const char*>(src.data());

			EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
			EShLanguage language = GetShaderType(type);

			glslang::TShader shader(language);

			shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
			shader.setStringsWithLengthsAndNames(&shader_source, nullptr, file_name_list, 1);
			shader.setEntryPoint("main");
			shader.setSourceEntryPoint("main");

			std::string log = "";
			if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
			{
				log = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
				abort(); // temp
			}

			// Add shader to new program object.
			glslang::TProgram program;
			program.addShader(&shader);

			// Link program.
			if (!program.link(messages))
			{
				DebugLog::LogInfo(std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog()));
			}

			glslang::TIntermediate* intermediate = program.getIntermediate(language);
			if (!intermediate)
			{
				DebugLog::LogError("Failed to get shared intermediate code.\n");
				abort(); // temp
			}

			spv::SpvBuildLogger logger;
			glslang::GlslangToSpv(*intermediate, binaries, &logger);
			std::string error = logger.getAllMessages();
			if (!error.empty())
			{
				DebugLog::LogError(error);
				abort(); // temp
			}

			// Shutdown glslang library.
			glslang::FinalizeProcess();
		}

		// Save
		if (!isSource)
		{
			std::string cachedPath = Utils::GetCachedPath(path, CachedPathType::Shader);
			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open())
			{
				out.write((char*)binaries.data(), binaries.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		}
	}

	const ReflectionData& Shader::GetReflection() const
	{
		return m_ReflectData;
	}

	uint32_t Shader::GetACBindingPoint() const
	{
		return m_ACBindingPoint;
	}

	ShaderCreateInfo& Shader::GetCreateInfo()
	{
		return m_CreateInfo;
	}

	Ref<Shader> Shader::Create()
	{
		Ref<Shader> shader = nullptr;
#ifdef OPENGL_IMPL
#else
		shader = std::make_shared<VulkanShader>();
#endif
		return shader;
	}

	bool Shader::BuildBase(ShaderCreateInfo* info)
	{
		m_ReflectData.Clean();
		m_Binary.clear();

		const auto loadFn = [this](ShaderType type, const std::string& str, bool isSource)
		{
			if (str.empty()){ return; }
			if (type == ShaderType::RayGen) { m_RTPipeline = true; }

			auto& binaries = m_Binary[type];

			if (!isSource)
			{
				std::string cachedPath = Utils::GetCachedPath(str, CachedPathType::Shader);
				if (Utils::IsPathValid(cachedPath))
				{
					LoadSPIRV(cachedPath, binaries);
					Reflect(binaries, type); // TODO:: serialize

					return;
				}
			}

			CompileSPIRV(str, m_Binary[type], type, isSource);
			Reflect(binaries, type);
		};

		for (auto& [type, str] : info->Stages)
		{
			loadFn(type, str, info->IsSource);
		}

		m_CreateInfo = *info;
		return true;
	}

	void Shader::Reflect(const std::vector<uint32_t>& binaryData, ShaderType shaderType)
	{
		spirv_cross::Compiler compiler(binaryData);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		for (const auto& res : resources.uniform_buffers)
		{
			auto& type = compiler.get_type(res.base_type_id);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			auto& it = m_ReflectData.Buffers.find(binding);
			if (it != m_ReflectData.Buffers.end())
			{
				it->second.Stage |= shaderType;
			}
			else
			{
				uint32_t bufferElements = static_cast<uint32_t>(type.member_types.size());

				ShaderBuffer buffer = {};
				{
					buffer.Type = BufferType::Uniform;
					buffer.Name = res.name;
					buffer.ObjectName = "UBO";
					buffer.BindingPoint = compiler.get_decoration(res.id, spv::DecorationBinding);
					buffer.Size = compiler.get_declared_struct_size(type);
					buffer.Stage = shaderType;

					buffer.Uniforms.reserve(bufferElements);
				}

				for (uint32_t i = 0; i < bufferElements; ++i)
				{
					Uniform uniform = {};
					{
						uniform.Name = compiler.get_member_name(type.self, i);
						//uniform.Type = compiler.get_type(type.member_types[i]);
						uniform.Size = compiler.get_declared_struct_member_size(type, i);
						uniform.Offset = compiler.type_struct_member_offset(type, i);
					}

					buffer.Uniforms.push_back(uniform);
				}

				m_ReflectData.Buffers[buffer.BindingPoint] = std::move(buffer);
			}
		}

		for (const auto& res : resources.acceleration_structures)
		{
			auto& type = compiler.get_type(res.base_type_id);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			if (m_ReflectData.ACStructures.find(binding) == m_ReflectData.ACStructures.end())
			{
				ACStructure acStructure{};
				acStructure.ArraySize = compiler.get_type(res.type_id).array[0];
				acStructure.Name = res.name;

				m_ReflectData.ACStructures[binding] = std::move(acStructure);
				m_ACBindingPoint = binding;
			}
		}

		for (const auto& res : resources.storage_buffers)
		{
			auto& type = compiler.get_type(res.base_type_id);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			auto& it = m_ReflectData.Buffers.find(binding);
			if (it != m_ReflectData.Buffers.end())
			{
				it->second.Stage |= shaderType;
			}
			else
			{
				uint32_t bufferElements = static_cast<uint32_t>(type.member_types.size());
				ShaderBuffer buffer = {};
				{
					buffer.Type = BufferType::Storage;
					buffer.Name = res.name;
					buffer.ObjectName = "SSBO";
					buffer.BindingPoint = binding;
					buffer.Size = compiler.get_declared_struct_size(type);
					buffer.Stage = shaderType;

					buffer.Uniforms.reserve(bufferElements);
				}

				for (uint32_t i = 0; i < bufferElements; ++i)
				{
					Uniform uniform = {};
					{
						uniform.Name = compiler.get_member_name(type.self, i);
						//uniform.Type = compiler.get_type(type.member_types[i]);
						uniform.Size = compiler.get_declared_struct_member_size(type, i);
						uniform.Offset = compiler.type_struct_member_offset(type, i);
					}

					buffer.Uniforms.push_back(uniform);
				}

				m_ReflectData.Buffers[buffer.BindingPoint] = std::move(buffer);
			}
		}

		for (const auto& res : resources.push_constant_buffers)
		{
			auto& type = compiler.get_type(res.base_type_id);
			PushContantData pc = {};
			{
				pc.Offset = 0;
				pc.Size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));
				pc.Stage = shaderType;
			}

			m_ReflectData.PushConstant = pc;
		}

		auto processImage = [&](std::map<uint32_t, SamplerBuffer>& map, const spirv_cross::Resource& res)
		{
			auto& type = compiler.get_type(res.base_type_id);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			auto& it = map.find(binding);
			if (it != map.end()) { it->second.Stage |= shaderType; }
			else
			{
				SamplerBuffer resBuffer = {};
				{
					resBuffer.BindingPoint = compiler.get_decoration(res.id, spv::DecorationBinding);
					resBuffer.Location = compiler.get_decoration(res.id, spv::DecorationLocation);
					resBuffer.Dimension = compiler.get_type(res.type_id).image.dim;
					resBuffer.Stage = shaderType;
					resBuffer.ArraySize = compiler.get_type(res.type_id).array[0];
				}

				map[resBuffer.BindingPoint] = std::move(resBuffer);
			}
		};

		for (const auto& res : resources.sampled_images) { processImage(m_ReflectData.ImageSamplers, res); }
		for (const auto& res : resources.storage_images) { processImage(m_ReflectData.StorageImages, res); }
	}
}