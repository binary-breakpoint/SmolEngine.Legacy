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
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"

namespace SmolEngine
{
	void VulkanACStructure::BuildAsBottomLevel(uint32_t vertexStride, VulkanBuffer* transform, const Ref<Mesh>& mesh)
	{
		auto& device = VulkanContext::GetDevice();

		auto vb =  mesh->GetVertexBuffer()->Cast<VulkanVertexBuffer>();
		auto ib =  mesh->GetIndexBuffer()->Cast<VulkanIndexBuffer>();

		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		vertexBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(vb->GetBuffer());

		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
		indexBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(ib->GetBuffer());

		VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
		transformBufferDeviceAddress.deviceAddress = VulkanUtils::GetBufferDeviceAddress(transform->GetBuffer());

		// Build
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.maxVertex = vb->GetVertexCount();
		accelerationStructureGeometry.geometry.triangles.vertexStride = vertexStride;
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

		const uint32_t numTriangles = ib->GetCount() / 3;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		device.vkGetAccelerationStructureBuildSizesKHR(
			device.GetLogicalDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&numTriangles,
			&accelerationStructureBuildSizesInfo);

		m_Buffer.CreateBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = m_Buffer.GetBuffer();
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
	}

	VkAccelerationStructureKHR VulkanACStructure::GetHandle() 
	{
		return m_Handle;
	}

	VulkanBuffer VulkanACStructure::GetBuffer() const
	{
		return m_Buffer;
	}

	uint64_t VulkanACStructure::GetDeviceAddress() const
	{
		return m_DeviceAddress;
	}

	void VulkanACStructure::BuildAsTopLevel(VulkanBuffer* instancesBuffer, RaytracingPipelineSceneInfo* scene, 
		const std::map<Ref<Mesh>, Ref<BLAS>>& bottomLevels, bool& out_update_descriptor)
	{
		out_update_descriptor = false;
		std::vector<VkAccelerationStructureInstanceKHR> instances;
		uint32_t instanceIndex = 0;

		for (auto& [mesh, blas] : bottomLevels)
		{
			for (uint32_t i = 0; i < blas->IntanceCount; ++i)
			{
				VkAccelerationStructureInstanceKHR acceleration_structure_instance{};

				glm::mat3x4 transform = glm::mat3x4(glm::transpose(scene->Transforms[instanceIndex]));
				memcpy(&acceleration_structure_instance.transform, &transform, sizeof(VkTransformMatrixKHR));

				acceleration_structure_instance.instanceCustomIndex = instanceIndex;
				acceleration_structure_instance.mask = 0xFF;
				acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;
				acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

				for (uint32_t j = 0; j < static_cast<uint32_t>(blas->Nodes.size()); ++j)
				{
					acceleration_structure_instance.accelerationStructureReference = blas->Nodes[j]->GetDeviceAddress();
					instances.emplace_back(acceleration_structure_instance);
				}

				instanceIndex++;
			}
		}

		{
			const size_t instancesDataSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
			instancesBuffer->CreateBuffer(instances.data(), instancesDataSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		}

		VkDeviceOrHostAddressConstKHR instance_data_device_address{};
		instance_data_device_address.deviceAddress = VulkanUtils::GetBufferDeviceAddress(instancesBuffer->GetBuffer());

		// The top level acceleration structure contains (bottom level) instance as the input geometry
		VkAccelerationStructureGeometryKHR acceleration_structure_geometry{};
		acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_structure_geometry.geometry.instances = {};
		acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
		acceleration_structure_geometry.geometry.instances.data = instance_data_device_address;

		// Get the size requirements for buffers involved in the acceleration structure build process
		VkAccelerationStructureBuildGeometryInfoKHR acceleration_structure_build_geometry_info{};
		acceleration_structure_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_structure_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_structure_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_structure_build_geometry_info.geometryCount = 1;
		acceleration_structure_build_geometry_info.pGeometries = &acceleration_structure_geometry;

		const uint32_t primitive_count =static_cast<uint32_t>(instances.size());
		auto& device = VulkanContext::GetDevice();

		VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes_info{};
		acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		device.vkGetAccelerationStructureBuildSizesKHR(
			device.GetLogicalDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&acceleration_structure_build_geometry_info,
			&primitive_count,
			&acceleration_structure_build_sizes_info);


		if (m_Buffer.GetSize() == 0)
		{
			m_Buffer.CreateBuffer(acceleration_structure_build_sizes_info.accelerationStructureSize,
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
				VMA_MEMORY_USAGE_GPU_ONLY);
		}
		else if (m_Buffer.GetSize() != acceleration_structure_build_sizes_info.accelerationStructureSize)
		{
			m_Buffer.Destroy();
			m_Buffer.CreateBuffer(acceleration_structure_build_sizes_info.accelerationStructureSize,
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
				VMA_MEMORY_USAGE_GPU_ONLY);

			out_update_descriptor = true;
			device.vkDestroyAccelerationStructureKHR(device.GetLogicalDevice(), m_Handle, nullptr);
		}

		const bool is_update = out_update_descriptor ? false : m_Handle != nullptr;
		if (!is_update)
		{
			VkAccelerationStructureCreateInfoKHR acceleration_structure_create_info{};
			acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			acceleration_structure_create_info.buffer = m_Buffer.GetBuffer();
			acceleration_structure_create_info.size = acceleration_structure_build_sizes_info.accelerationStructureSize;
			acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

			device.vkCreateAccelerationStructureKHR(device.GetLogicalDevice(), &acceleration_structure_create_info, nullptr,
				&m_Handle);
		}

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		VulkanScratchBuffer scratchBuffer{};
		scratchBuffer.Build(acceleration_structure_build_sizes_info.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
		acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		acceleration_build_geometry_info.mode = is_update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		acceleration_build_geometry_info.dstAccelerationStructure = m_Handle;
		acceleration_build_geometry_info.geometryCount = 1;
		acceleration_build_geometry_info.pGeometries = &acceleration_structure_geometry;
		acceleration_build_geometry_info.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

		VkAccelerationStructureBuildRangeInfoKHR acceleration_structure_build_range_info;
		acceleration_structure_build_range_info.primitiveCount = primitive_count;
		acceleration_structure_build_range_info.primitiveOffset = 0;
		acceleration_structure_build_range_info.firstVertex = 0;
		acceleration_structure_build_range_info.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> acceleration_build_structure_range_infos = { &acceleration_structure_build_range_info };

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			device.vkCmdBuildAccelerationStructuresKHR(
				cmdStorage.Buffer,
				1,
				&acceleration_build_geometry_info,
				acceleration_build_structure_range_infos.data());
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		// Get the top acceleration structure's handle, which will be used to set up its descriptor
		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = m_Handle;

		m_DeviceAddress = device.vkGetAccelerationStructureDeviceAddressKHR(device.GetLogicalDevice(), &acceleration_device_address_info);
	}

	VulkanACStructure::~VulkanACStructure()
	{
		Free();
	}

	void VulkanACStructure::Free()
	{
		auto& device = VulkanContext::GetDevice();
		device.vkDestroyAccelerationStructureKHR(device.GetLogicalDevice(), m_Handle, nullptr);

		m_DeviceAddress = 0;
		m_Buffer.Destroy();
	}
}

#endif