#pragma once

#include "Core/Debug/DebugLog.h"
#include "Graphics/Backends/Vulkan/Vulkan.h"

#include <iomanip>
#include <string>
#include <sstream>

#include <nvidia_aftermath/include/GFSDK_Aftermath.h>
#include <nvidia_aftermath/include/GFSDK_Aftermath_GpuCrashDump.h>
#include <nvidia_aftermath/include/GFSDK_Aftermath_GpuCrashDumpDecoding.h>

namespace std
{
    template<typename T>
    inline std::string to_hex_string(T n)
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2 * sizeof(T)) << std::hex << n;
        return stream.str();
    }

    inline std::string to_string(GFSDK_Aftermath_Result result)
    {
        return std::string("0x") + to_hex_string(static_cast<uint32_t>(result));
    }

    inline std::string to_string(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier)
    {
        return to_hex_string(identifier.id[0]) + "-" + to_hex_string(identifier.id[1]);
    }

    inline std::string to_string(const GFSDK_Aftermath_ShaderHash& hash)
    {
        return to_hex_string(hash.hash);
    }

    inline std::string to_string(const GFSDK_Aftermath_ShaderInstructionsHash& hash)
    {
        return to_hex_string(hash.hash) + "-" + to_hex_string(hash.hash);
    }
} // namespace std

 // Helper for comparing GFSDK_Aftermath_ShaderDebugInfoIdentifier.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& lhs, const GFSDK_Aftermath_ShaderDebugInfoIdentifier& rhs)
{
    if (lhs.id[0] == rhs.id[0])
    {
        return lhs.id[1] < rhs.id[1];
    }
    return lhs.id[0] < rhs.id[0];
}

// Helper for comparing GFSDK_Aftermath_ShaderHash.
inline bool operator<(const GFSDK_Aftermath_ShaderHash& lhs, const GFSDK_Aftermath_ShaderHash& rhs)
{
    return lhs.hash < rhs.hash;
}

// Helper for comparing GFSDK_Aftermath_ShaderInstructionsHash.
inline bool operator<(const GFSDK_Aftermath_ShaderInstructionsHash& lhs, const GFSDK_Aftermath_ShaderInstructionsHash& rhs)
{
    return lhs.hash < rhs.hash;
}

// Helper for comparing GFSDK_Aftermath_ShaderDebugName.
inline bool operator<(const GFSDK_Aftermath_ShaderDebugName& lhs, const GFSDK_Aftermath_ShaderDebugName& rhs)
{
    return strncmp(lhs.name, rhs.name, sizeof(lhs.name)) < 0;
}

//*********************************************************
// Helper for checking Nsight Aftermath failures.
//

inline std::string  AftermathErrorMessage(GFSDK_Aftermath_Result result)
{
    switch (result)
    {
    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
        return "Unsupported driver version - requires a recent NVIDIA R445 display driver or newer.";
    default:
        return "Aftermath Error 0x" + std::to_hex_string(result);
    }
}

// Helper macro for checking Nsight Aftermath results and throwing exception
// in case of a failure.
#ifdef _WIN32
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
    [&]() {                                                                                                 \
        GFSDK_Aftermath_Result _result = FC;                                                                \
        if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
        {                                                                                                   \
            SmolEngine::DebugLog::LogError(AftermathErrorMessage(_result).c_str());                                              \
            MessageBoxA(0, AftermathErrorMessage(_result).c_str(), "Aftermath Error", MB_OK);               \
            exit(1);                                                                                        \
        }                                                                                                   \
    }()
#else
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
    [&]() {                                                                                                 \
        GFSDK_Aftermath_Result _result = FC;                                                                \
        if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
        {                                                                                                   \
            printf("%s\n", AftermathErrorMessage(_result).c_str());                                         \
            fflush(stdout);                                                                                 \
            exit(1);                                                                                        \
        }                                                                                                   \
    }()
#endif