#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanACStructure.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanVertexBuffer.h"
#include "Backends/Vulkan/VulkanIndexBuffer.h"
#include "Backends/Vulkan/VulkanUtils.h"
#include "Backends/Vulkan/VulkanScratchBuffer.h"
#include "Backends/Vulkan/VulkanCommandBuffer.h"

#include "Pools/MeshPool.h"
#include "Primitives/RaytracingPipeline.h"

namespace SmolEngine
{
	bool VulkanACStructure::BuildBufferEX(size_t size, VmaMemoryUsage memUsage)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		m_Alloc = VulkanAllocator::AllocBuffer(bufferCreateInfo, memUsage, m_Buffer);

		return m_Alloc != nullptr;
	}

	bool VulkanACStructure::BuildBottomLevel(RaytracingPipelineCreateInfo* info)
	{
		// Setup identity transform matrix
		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f
		};

		m_TransformBuffer.CreateBuffer(&transformMatrix, sizeof(VkTransformMatrixKHR),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

		auto& device = VulkanContext::GetDevice();
		auto& [cube, view] = MeshPool::GetCube();

		auto vb = cube->GetVertexBuffer()->Cast<VulkanVertexBuffer>();
		auto ib = cube->GetIndexBuffer()->Cast<VulkanIndexBuffer>();

		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		vertexBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(vb->GetBuffer());

		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
		indexBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(ib->GetBuffer());

		VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
		transformBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(m_TransformBuffer.GetBuffer());

		// Build
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.maxVertex = cube->GetVertexBuffer()->GetVertexCount();
		accelerationStructureGeometry.geometry.triangles.vertexStride = info->VertexInput.Stride;
		accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
		accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
		accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		const uint32_t numTriangles = 1;
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		device.vkGetAccelerationStructureBuildSizesKHR(
			device.GetLogicalDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&numTriangles,
			&accelerationStructureBuildSizesInfo);

		BuildBufferEX(accelerationStructureBuildSizesInfo.accelerationStructureSize);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = m_Buffer;
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		device.vkCreateAccelerationStructureKHR(device.GetLogicalDevice(), &accelerationStructureCreateInfo, nullptr, &m_Handle);

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		VulkanScratchBuffer scratchBuffer{};
		scratchBuffer.Build(accelerationStructureBuildSizesInfo.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = m_Handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;

		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			device.vkCmdBuildAccelerationStructuresKHR(
				cmdStorage.Buffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = m_Handle;
		m_DeviceAddress = device.vkGetAccelerationStructureDeviceAddressKHR(device.GetLogicalDevice(), &accelerationDeviceAddressInfo);

		return true;
	}

	VulkanACStructure::~VulkanACStructure()
	{
		Free();
	}

	void VulkanACStructure::Free()
	{
		if (m_Alloc != nullptr)
		{
			VulkanAllocator::FreeBuffer(m_Buffer, m_Alloc);

			m_Alloc = nullptr;
			m_Buffer = nullptr;
			m_Handle = nullptr;
			m_DeviceAddress = 0;
		}
	}

	VkAccelerationStructureKHR VulkanACStructure::GetHandle() const
	{
		return m_Handle;
	}
}

#endif