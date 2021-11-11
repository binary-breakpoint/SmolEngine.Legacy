#pragma once
#ifndef OPENGL_IMPL

#include "Graphics/Backends/Vulkan/Vulkan.h"
#include "Graphics/Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	class Mesh;
	struct RaytracingPipelineSceneInfo;
	struct BLAS;

	class VulkanACStructure
	{
	public:
		~VulkanACStructure();

		void                       Free();
		void                       BuildAsBottomLevel(uint32_t vertexStride, VulkanBuffer* transformBuffer, const Ref<Mesh>& mesh);
		void                       BuildAsTopLevel(VulkanBuffer* instancesBuffer, uint32_t primitiveCount, bool& out_update_descriptor);

		VkAccelerationStructureKHR GetHandle();
		VulkanBuffer               GetBuffer() const;
		uint64_t                   GetDeviceAddress() const;


	private:
		uint64_t                    m_DeviceAddress = 0;
		VkAccelerationStructureKHR  m_Handle = nullptr;
		VulkanBuffer                m_Buffer{};
	};
}

#endif