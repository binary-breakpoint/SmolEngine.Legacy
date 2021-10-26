#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanDevice.h"
#include "Backends/Vulkan/VulkanInstance.h"

#include <assert.h>

namespace SmolEngine
{
	VulkanDevice::VulkanDevice()
	{

	}

	VulkanDevice::~VulkanDevice()
	{
		if (m_VkLogicalDevice != VK_NULL_HANDLE)
		{
			//vkDestroyDevice(m_VkLogicalDevice, nullptr);
		}
	}

	bool VulkanDevice::Init(const VulkanInstance* instance)
	{
		if (SetupPhysicalDevice(instance))
		{
			std::stringstream ss;
			ss << "Vulkan Info:\n\n";
			ss << "           Vulkan API Version: {}\n";
			ss << "           Selected Device: {}\n";
			ss << "           Driver Version: {}\n";
			ss << "           Raytracing Enabled: {}\n";
			ss << "           Max push_constant size: {}\n\n";

			DebugLog::LogInfo(ss.str(),
				m_VkDeviceProperties.apiVersion,
				std::string(m_VkDeviceProperties.deviceName),
				m_VkDeviceProperties.driverVersion, m_RayTracingEnabled,
				m_VkDeviceProperties.limits.maxPushConstantsSize);

			return SetupLogicalDevice();
		}

		return false;
	}

	bool VulkanDevice::SetupPhysicalDevice(const VulkanInstance* _instance)
	{
		const VkInstance& instance = _instance->GetInstance();
		m_ExtensionsList = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::vector<const char*> rayTracingEX = m_ExtensionsList;
		// Ray tracing related extensions
		rayTracingEX.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		rayTracingEX.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		// Required by VK_KHR_acceleration_structure
		rayTracingEX.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		rayTracingEX.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		rayTracingEX.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		// Required for VK_KHR_ray_tracing_pipeline
		rayTracingEX.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		// Required by VK_KHR_spirv_1_4
		rayTracingEX.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

		uint32_t devicesCount = 0;
		vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr);
		std::unique_ptr<VkPhysicalDevice[]> devices = std::make_unique<VkPhysicalDevice[]>(devicesCount);
		vkEnumeratePhysicalDevices(instance, &devicesCount, devices.get());

		for (uint32_t i = 0; i < devicesCount; ++i)
		{
			const VkPhysicalDevice& current_device = devices[i];

			if (HasRequiredExtensions(current_device, rayTracingEX)) // Ray Tracing
			{
				m_ExtensionsList = rayTracingEX;
				m_RayTracingEnabled = true;

				SelectDevice(current_device);
				break;
			}

			if (HasRequiredExtensions(current_device, m_ExtensionsList)) // No Ray Tracing
				SelectDevice(current_device);
		}

		assert(m_VkPhysicalDevice != VK_NULL_HANDLE);
		return m_VkPhysicalDevice != VK_NULL_HANDLE;
	}

	bool VulkanDevice::SetupLogicalDevice()
	{
		const float priority = 0.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		{
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// If compute family index differs, we need an additional queue create info for the compute queue
		if (m_QueueFamilyIndices.Compute != m_QueueFamilyIndices.Graphics)
		{
			queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.Compute;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		if ((m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Graphics) && (m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Compute))
		{
			queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.Transfer;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Enable features required for ray tracing using feature chaining via pNext	
		VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
		enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddresFeatures;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

		VkDeviceCreateInfo deviceInfo = {};
		{
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			deviceInfo.ppEnabledExtensionNames = m_ExtensionsList.data();
			deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_ExtensionsList.size());
			deviceInfo.pEnabledFeatures = &m_VkDeviceFeatures;

			if (m_RayTracingEnabled)
			{
				deviceInfo.pNext = &enabledAccelerationStructureFeatures;
			}
		}

		VkResult result = vkCreateDevice(m_VkPhysicalDevice, &deviceInfo, nullptr, &m_VkLogicalDevice);

		vkGetDeviceQueue(m_VkLogicalDevice, m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_VkLogicalDevice, m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);

		FindMaxUsableSampleCount();
		GetFuncPtrs();

		assert(result == VK_SUCCESS);
		return result == VK_SUCCESS;
	}

	bool VulkanDevice::HasRequiredExtensions(const VkPhysicalDevice& device, const std::vector<const char*>& extensionsList)
	{
		uint32_t extCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
		std::unique_ptr<VkExtensionProperties[]> extensions = std::make_unique<VkExtensionProperties[]>(extCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, extensions.get());
		for (const auto& name : extensionsList)
		{
			bool ext_found = false;

			for (uint32_t i = 0; i < extCount; ++i)
			{
				// Note:
				// The return value from strcmp is 0 if the two strings are equal

				if (strcmp(extensions[i].extensionName, name) == 0)
				{
					ext_found = true;
					break;
				}
			}

			if (ext_found == false)
			{
				return false;
			}
		}

		return true;
	}

	void VulkanDevice::FindMaxUsableSampleCount()
	{
		VkSampleCountFlagBits sampleCount;
		VkSampleCountFlags counts = m_VkDeviceProperties.limits.framebufferColorSampleCounts & m_VkDeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { sampleCount = VK_SAMPLE_COUNT_64_BIT; }
		else if (counts & VK_SAMPLE_COUNT_32_BIT) { sampleCount = VK_SAMPLE_COUNT_32_BIT; }
		else if (counts & VK_SAMPLE_COUNT_16_BIT) { sampleCount = VK_SAMPLE_COUNT_16_BIT; }
		else if (counts & VK_SAMPLE_COUNT_8_BIT) { sampleCount = VK_SAMPLE_COUNT_8_BIT; }
		else if (counts & VK_SAMPLE_COUNT_4_BIT) { sampleCount = VK_SAMPLE_COUNT_4_BIT; }
		else if (counts & VK_SAMPLE_COUNT_2_BIT) { sampleCount = VK_SAMPLE_COUNT_2_BIT; }
		else { sampleCount = VK_SAMPLE_COUNT_1_BIT; }

		m_VkDeviceFeatures.sampleRateShading = VK_TRUE;
		m_MSAASamplesCount = sampleCount;
	}

	void VulkanDevice::SelectDevice(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		assert(queueFamilyCount > 0);

		m_QueueFamilyProperties.resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, m_QueueFamilyProperties.data());

		VkPhysicalDeviceProperties2 deviceProperties2{};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		// Get ray tracing pipeline properties
		if (m_RayTracingEnabled)
		{
			rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
			deviceProperties2.pNext = &rayTracingPipelineProperties;
		}
		vkGetPhysicalDeviceProperties2(device, &deviceProperties2);

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		// Get acceleration structure properties
		if (m_RayTracingEnabled)
		{
			accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
			deviceFeatures2.pNext = &accelerationStructureFeatures;
		}
		vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

		VkPhysicalDeviceMemoryProperties memoryProperties = {};
		vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

		if (deviceProperties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;

			m_QueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);
			m_VkDeviceProperties = deviceProperties2.properties;
			m_VkDeviceFeatures = deviceFeatures2.features;
			m_VkMemoryProperties = memoryProperties;
			m_VkDeviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
			m_VkPhysicalDevice = device;
		}
	}

	void VulkanDevice::GetFuncPtrs()
	{
		if (m_RayTracingEnabled)
		{
			// Get the function pointers required for ray tracing
			m_vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkGetBufferDeviceAddressKHR"));
			m_vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
			m_vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkBuildAccelerationStructuresKHR"));
			m_vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkCreateAccelerationStructureKHR"));
			m_vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkDestroyAccelerationStructureKHR"));
			m_vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
			m_vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
			m_vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkCmdTraceRaysKHR"));
			m_vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
			m_vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_VkLogicalDevice, "vkCreateRayTracingPipelinesKHR"));
		}
	}

	QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices(int flags)
	{
		QueueFamilyIndices indices;

		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (flags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indices.Compute = i;
					break;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		if (flags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
			{
				auto& queueFamilyProperties = m_QueueFamilyProperties[i];
				if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					indices.Transfer = i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
		{
			if ((flags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
					indices.Transfer = i;
			}

			if ((flags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					indices.Compute = i;
			}

			if (flags & VK_QUEUE_GRAPHICS_BIT)
			{
				if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.Graphics = i;
			}
		}

		return indices;
	}

	const VkPhysicalDeviceMemoryProperties* VulkanDevice::GetMemoryProperties() const
	{
		return &m_VkMemoryProperties;
	}

	const VkPhysicalDeviceProperties* VulkanDevice::GetDeviceProperties() const
	{
		return &m_VkDeviceProperties;
	}

	const VkPhysicalDeviceFeatures* VulkanDevice::GetDeviceFeatures() const
	{
		return &m_VkDeviceFeatures;
	}

	const VkSampleCountFlagBits VulkanDevice::GetMSAASamplesCount() const
	{
		return m_MSAASamplesCount;
	}

	const VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
	{
		return m_VkPhysicalDevice;
	}

	const VkDevice VulkanDevice::GetLogicalDevice() const
	{
		return m_VkLogicalDevice;
	}

	const QueueFamilyIndices& VulkanDevice::GetQueueFamilyIndices() const
	{
		return m_QueueFamilyIndices;
	}

	uint32_t VulkanDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags memFlags) const
	{
		for (uint32_t i = 0; i < m_VkMemoryProperties.memoryTypeCount; ++i)
		{
			if ((typeBits & 1) == 1)
			{
				if ((m_VkMemoryProperties.memoryTypes[i].propertyFlags & memFlags) == memFlags)
				{
					return i;
				}
			}

			typeBits >>= 1;
		}

		DebugLog::LogError("Could not find a suitable memory type!");
		abort();
	}

	const VkQueue VulkanDevice::GetQueue(QueueFamilyFlags flag) const
	{
		return flag == QueueFamilyFlags::Compute ? m_ComputeQueue : m_GraphicsQueue;
	}

	bool VulkanDevice::GetRaytracingSupport() const
	{
		return m_RayTracingEnabled;
	}
}
#endif