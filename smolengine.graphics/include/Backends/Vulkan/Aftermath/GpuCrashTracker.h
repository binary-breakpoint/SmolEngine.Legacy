#pragma once
#ifdef AFTERMATH
#include <map>
#include <mutex>

#include "Backends/Vulkan/Aftermath/AftermathUtils.h"
#include "Backends/Vulkan/Aftermath/AftermathShaderTracker.h"

namespace SmolEngine
{
    //*********************************************************
    // Implements GPU crash dump tracking using the Nsight
    // Aftermath API.
    //
    class GpuCrashTracker
    {
    public:
        GpuCrashTracker();
        ~GpuCrashTracker();

        // Initialize the GPU crash dump tracker.
        void Initialize();

        AftermathShaderTracker m_tracker{};

    private:

        //*********************************************************
        // Callback handlers for GPU crash dumps and related data.
        //

        // Handler for GPU crash dump callbacks.
        void OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

        // Handler for shader debug information callbacks.
        void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);

        // Handler for GPU crash dump description callbacks.
        void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);

        //*********************************************************
        // Helpers for writing a GPU crash dump and debug information
        // data to files.
        //

        // Helper for writing a GPU crash dump to a file.
        void WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

        // Helper for writing shader debug information to a file
        void WriteShaderDebugInformationToFile(
            GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
            const void* pShaderDebugInfo,
            const uint32_t shaderDebugInfoSize);

        //*********************************************************
        // Helpers for decoding GPU crash dump to JSON.
        //

        // Handler for shader debug info lookup callbacks.
        void OnShaderDebugInfoLookup(
            const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
            PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const;

        // Handler for shader lookup callbacks.
        void OnShaderLookup(
            const GFSDK_Aftermath_ShaderHash& shaderHash,
            PFN_GFSDK_Aftermath_SetData setShaderBinary) const;

        // Handler for shader instructions lookup callbacks.
        void OnShaderInstructionsLookup(
            const GFSDK_Aftermath_ShaderInstructionsHash& shaderInstructionsHash,
            PFN_GFSDK_Aftermath_SetData setShaderBinary) const;

        // Handler for shader source debug info lookup callbacks.
        void OnShaderSourceDebugInfoLookup(
            const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
            PFN_GFSDK_Aftermath_SetData setShaderBinary) const;

        //*********************************************************
        // Static callback wrappers.
        //

        // GPU crash dump callback.
        static void GpuCrashDumpCallback(
            const void* pGpuCrashDump,
            const uint32_t gpuCrashDumpSize,
            void* pUserData);

        // Shader debug information callback.
        static void ShaderDebugInfoCallback(
            const void* pShaderDebugInfo,
            const uint32_t shaderDebugInfoSize,
            void* pUserData);

        // GPU crash dump description callback.
        static void CrashDumpDescriptionCallback(
            PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
            void* pUserData);

        // Shader debug information lookup callback.
        static void ShaderDebugInfoLookupCallback(
            const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
            PFN_GFSDK_Aftermath_SetData setShaderDebugInfo,
            void* pUserData);

        // Shader lookup callback.
        static void ShaderLookupCallback(
            const GFSDK_Aftermath_ShaderHash* pShaderHash,
            PFN_GFSDK_Aftermath_SetData setShaderBinary,
            void* pUserData);

        // Shader instructions lookup callback.
        static void ShaderInstructionsLookupCallback(
            const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash,
            PFN_GFSDK_Aftermath_SetData setShaderBinary,
            void* pUserData);

        // Shader source debug info lookup callback.
        static void ShaderSourceDebugInfoLookupCallback(
            const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
            PFN_GFSDK_Aftermath_SetData setShaderBinary,
            void* pUserData);

        //*********************************************************
        // GPU crash tracker state.
        //

        // Is the GPU crash dump tracker initialized?
        bool m_initialized;

        // For thread-safe access of GPU crash tracker state.
        mutable std::mutex m_mutex;

        // List of Shader Debug Information by ShaderDebugInfoIdentifier.
        std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, std::vector<uint8_t>> m_shaderDebugInfo;
    };
}
#endif