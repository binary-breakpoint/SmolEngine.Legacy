#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanDevice.h"
#include "Backends/Vulkan/VulkanInstance.h"

#include <assert.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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

			bool setup_result = SetupLogicalDevice();
			if (setup_result)
			{
				vkGetDeviceQueue(m_VkLogicalDevice, m_DeviceQueueFamilyIndex, 0, &m_Queue);
				FindMaxUsableSampleCount();
				GetFuncPtrs();
			}
			return setup_result;
		}

		return false;
	}

	bool VulkanDevice::SetupPhysicalDevice(const VulkanInstance* _instance)
	{
		const VkInstance& instance = _instance->GetInstance();
		m_ExtensionsList = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::vector<const char*> rayTracingEX = m_ExtensionsList;
		// Ray tracing related extensions required by this sample
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

			if (HasRequiredExtensions(current_device, rayTracingEX)) // RayTracing
			{
				SelectDevice(current_device);
				m_ExtensionsList = rayTracingEX;
				m_RayTracingEnabled = true;
				break;
			}

			if (HasRequiredExtensions(current_device, m_ExtensionsList))
				SelectDevice(current_device);
		}

		assert(m_VkPhysicalDevice != VK_NULL_HANDLE);
		return m_VkPhysicalDevice != VK_NULL_HANDLE;
	}


	bool VulkanDevice::SetupLogicalDevice()
	{
		static const float priority = 1.0f;

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		{
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_DeviceQueueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;
		}

		VkDeviceCreateInfo deviceInfo = {};
		{
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceInfo.pQueueCreateInfos = &queueCreateInfo;
			deviceInfo.queueCreateInfoCount = 1;
			deviceInfo.pEnabledFeatures = &m_VkDeviceFeatures;
			deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_ExtensionsList.size());
			deviceInfo.ppEnabledExtensionNames = m_ExtensionsList.data();
		}

		VkResult result = vkCreateDevice(m_VkPhysicalDevice, &deviceInfo, nullptr, &m_VkLogicalDevice);
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

	bool VulkanDevice::GetFamilyQueue(const VkPhysicalDevice& device, VkQueueFlags flags, uint32_t& outQueueIndex)
	{
		uint32_t queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
		std::unique_ptr<VkQueueFamilyProperties[]> families = std::make_unique<VkQueueFamilyProperties[]>(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, families.get());
		for (uint32_t i = 0; i < queueCount; ++i)
		{
			if (families[i].queueCount > 0)
			{
				if ((families[i].queueFlags & flags) == flags)
				{
					outQueueIndex = i;

					return true;
				}
			}
		}

		return false;
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
		uint32_t familyIndex = 0;
		if (GetFamilyQueue(device, VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT, familyIndex))
		{
			VkPhysicalDeviceProperties2 deviceProperties2{};
			deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(device, &deviceProperties2);
			VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
			deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);
			VkPhysicalDeviceMemoryProperties memoryProperties = {};
			vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

			if (m_VkPhysicalDevice == VK_NULL_HANDLE || deviceProperties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_VkDeviceProperties = deviceProperties2.properties;
				m_VkDeviceFeatures = deviceFeatures2.features;
				m_VkMemoryProperties = memoryProperties;

				m_VkDeviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

				m_VkPhysicalDevice = device;
				m_DeviceQueueFamilyIndex = familyIndex;
			}
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

	uint32_t VulkanDevice::GetQueueFamilyIndex() const
	{
		return m_DeviceQueueFamilyIndex;
	}

	const VkQueue VulkanDevice::GetQueue() const
	{
		return m_Queue;
	}

	bool VulkanDevice::GetRaytracingSupport() const
	{
		return m_RayTracingEnabled;
	}
}
#endif