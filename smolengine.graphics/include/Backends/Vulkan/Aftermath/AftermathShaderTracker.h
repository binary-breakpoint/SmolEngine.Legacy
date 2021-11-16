#pragma once
#ifdef AFTERMATH
#include "Backends/Vulkan/Aftermath/AftermathUtils.h"

#include <vector>
#include <map>
#include <mutex>

namespace SmolEngine
{
    class AftermathShaderTracker
    {
    public:
        // Find a shader bytecode binary by shader hash.
        bool FindShaderBinary(const GFSDK_Aftermath_ShaderHash& shaderHash, std::vector<uint8_t>& shader) const;
        void AddShaderBinary(const char* shaderFilePath);

    private:
        static bool ReadFile(const char* filename, std::vector<uint8_t>& data);

        // List of shader binaries by ShaderHash.
        std::map<GFSDK_Aftermath_ShaderHash, std::vector<uint8_t>> m_shaderBinaries;
    };
}
#endif