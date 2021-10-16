#include "stdafx.h"
#include "Primitives/Shader.h"

#include "Common/DebugLog.h"
#include "Tools/Utils.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglShader.h"
#else
#include "Backends/Vulkan/VulkanShader.h"
#endif

namespace SmolEngine
{
	shaderc_shader_kind GetShaderType(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::Vertex:      return shaderc_shader_kind::shaderc_vertex_shader;
		case ShaderType::Fragment:    return shaderc_shader_kind::shaderc_fragment_shader;
		case ShaderType::Geometry:    return shaderc_shader_kind::shaderc_geometry_shader;
		case ShaderType::Compute:     return shaderc_shader_kind::shaderc_compute_shader;
		}

		return shaderc_shader_kind::shaderc_vertex_shader;
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
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
#ifdef OPENGL_IMPL
#else
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
#endif
#ifndef SMOLENGINE_DEBUG
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif
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
			const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(src, GetShaderType(type), " ", options);
			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				DebugLog::LogError(result.GetErrorMessage().c_str());
				assert(false);
			}

			binaries = std::vector<uint32_t>(result.cbegin(), result.cend());
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
			if (str.empty())
				return;

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

		auto processImage = [&](std::unordered_map<uint32_t, SamplerBuffer>& map, const spirv_cross::Resource& res)
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