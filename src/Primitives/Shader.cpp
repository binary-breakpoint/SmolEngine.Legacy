#include "stdafx.h"
#include "Primitives/Shader.h"

#include "Common/SLog.h"
#include "Utils/Utils.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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

	void CompileSPIRV(const std::string& path, std::vector<uint32_t>& binaries, ShaderType type)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
#ifdef FROSTIUM_OPENGL_IMPL
#else
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
#endif
#ifndef FROSTIUM_DEBUG
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif
		// Load source
		std::string src = "";
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
			const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(src, GetShaderType(type), path.c_str(), options);
			if (result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				NATIVE_ERROR(result.GetErrorMessage());
				assert(false);
			}

			binaries = std::vector<uint32_t>(result.cbegin(), result.cend());
		}

		// Save
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

	void Shader::Create(GraphicsPipelineShaderCreateInfo* shaderCI, Shader* obj)
	{
		if (obj == nullptr)
			return;

		obj->m_ReflectData.Clean();
		std::unordered_map<ShaderType, std::vector<uint32_t>> binaryData;
		for (auto& [type, path] : shaderCI->FilePaths)
		{
			if (path.empty())
				continue;

			std::string cachedPath = Utils::GetCachedPath(path, CachedPathType::Shader);
			auto& binaries = binaryData[type];
			if (Utils::IsPathValid(cachedPath))
			{
				LoadSPIRV(cachedPath, binaries);
				obj->Reflect(binaries, type); // TODO:: serialize
				continue;
			}

			CompileSPIRV(path, binaryData[type], type);
			obj->Reflect(binaries, type);
		}

		obj->m_CreateInfo = *shaderCI;
#ifdef FROSTIUM_OPENGL_IMPL
#else
		obj->m_VulkanShader.Init(binaryData, &obj->m_ReflectData, &obj->m_CreateInfo);
#endif

	}

	void Shader::Bind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglShader.Bind();
#endif
	}

	void Shader::UnBind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglShader.UnBind();
#endif
	}

	bool Shader::Realod()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false;
#else
		Create(&m_CreateInfo, this);
		return true;
#endif
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

		for (const auto& res : resources.sampled_images)
		{
			auto& type = compiler.get_type(res.base_type_id);
			uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

			auto& it = m_ReflectData.Resources.find(binding);
			if (it != m_ReflectData.Resources.end())
			{
				it->second.Stage |= shaderType;
			}
			else
			{
				UniformResource resBuffer = {};
				{
					resBuffer.BindingPoint = compiler.get_decoration(res.id, spv::DecorationBinding);
					resBuffer.Location = compiler.get_decoration(res.id, spv::DecorationLocation);
					resBuffer.Dimension = compiler.get_type(res.type_id).image.dim;
					resBuffer.Stage = shaderType;
					resBuffer.ArraySize = compiler.get_type(res.type_id).array[0];
				}

				m_ReflectData.Resources[resBuffer.BindingPoint] = std::move(resBuffer);
			}
		}
	}

	uint32_t Shader::GetProgramID()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglShader.GetProgramID();
#else
		return 0;
#endif
	}

	const std::string Shader::GetName()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglShader.GetName();
#else
		return std::string("None");
#endif
	}
}