#include "stdafx.h"
#ifdef AFTERMATH
#include "Backends/Vulkan/Aftermath/AftermathShaderTracker.h"

namespace SmolEngine
{
	bool AftermathShaderTracker::FindShaderBinary(const GFSDK_Aftermath_ShaderHash& shaderHash, std::vector<uint8_t>& shader) const
	{
		// Find shader binary data for the shader hash
		auto i_shader = m_shaderBinaries.find(shaderHash);
		if (i_shader == m_shaderBinaries.end())
		{
			// Nothing found.
			return false;
		}

		shader = i_shader->second;
		return true;
	}

	void AftermathShaderTracker::AddShaderBinary(const char* shaderFilePath)
	{
		// Read the shader binary code from the file
		std::vector<uint8_t> data;
		if (!ReadFile(shaderFilePath, data))
		{
			return;
		}

		// Create shader hash for the shader
		const GFSDK_Aftermath_SpirvCode shader{ data.data(), uint32_t(data.size()) };
		GFSDK_Aftermath_ShaderHash shaderHash;
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderHashSpirv(
			GFSDK_Aftermath_Version_API,
			&shader,
			&shaderHash));

		// Store the data for shader mapping when decoding GPU crash dumps.
		// cf. FindShaderBinary()
		m_shaderBinaries[shaderHash].swap(data);
	}

	bool AftermathShaderTracker::ReadFile(const char* filename, std::vector<uint8_t>& data)
	{
		std::ifstream fs(filename, std::ios::in | std::ios::binary);
		if (!fs)
		{
			return false;
		}

		fs.seekg(0, std::ios::end);
		data.resize(fs.tellg());
		fs.seekg(0, std::ios::beg);
		fs.read(reinterpret_cast<char*>(data.data()), data.size());
		fs.close();

		return true;
	}
}
#endif