#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"

#include <vector>

namespace SmolEngine
{
	class VulkanInstance;

	struct QueueFamilyIndices
	{
		int32_t Graphics = -1;
		int32_t Compute = -1;
		int32_t Transfer = -1;
	};

	enum class QueueFamilyFlags
	{
		Graphics,
		Compute,
		Transfer
	};

	class VulkanDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		bool                                             Init(const VulkanInstance* instance);
		// Getters										 
		uint32_t                                         GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags memFlags) const;
		const VkPhysicalDeviceMemoryProperties*          GetMemoryProperties() const;
		const VkPhysicalDeviceProperties*                GetDeviceProperties() const;
		const VkPhysicalDeviceFeatures*                  GetDeviceFeatures() const;
		const VkSampleCountFlagBits                      GetMSAASamplesCount() const;
		const VkPhysicalDevice                           GetPhysicalDevice() const;
		const VkDevice                                   GetLogicalDevice() const;
		const QueueFamilyIndices&                        GetQueueFamilyIndices() const;
		const VkQueue                                    GetQueue(QueueFamilyFlags flag) const;
		bool                                             GetRaytracingSupport() const;
														 
	private:											 
		bool                                             SetupPhysicalDevice(const VulkanInstance* instance);
		bool                                             SetupLogicalDevice();
		bool                                             HasRequiredExtensions(const VkPhysicalDevice& device, const std::vector<const char*>& extensionsList);
		QueueFamilyIndices                               GetQueueFamilyIndices(int flags);
		void                                             FindMaxUsableSampleCount();
		void                                             SelectDevice(VkPhysicalDevice device);
		void                                             GetFuncPtrs();

	private:

		VkQueue                                          m_GraphicsQueue = nullptr;
		VkQueue                                          m_ComputeQueue = nullptr;
		VkCommandPool                                    m_CommandPool = nullptr;
		VkCommandPool                                    m_ComputeCommandPool = nullptr;
		VkPhysicalDevice                                 m_VkPhysicalDevice = nullptr;
		VkDevice                                         m_VkLogicalDevice = nullptr;
		VkSampleCountFlagBits                            m_MSAASamplesCount = VK_SAMPLE_COUNT_1_BIT;
		bool                                             m_RayTracingEnabled = false;
		VkPhysicalDeviceProperties                       m_VkDeviceProperties = {};
		VkPhysicalDeviceFeatures                         m_VkDeviceFeatures = {};
		VkPhysicalDeviceMemoryProperties                 m_VkMemoryProperties = {};
		QueueFamilyIndices                               m_QueueFamilyIndices = {};
		std::vector<VkQueueFamilyProperties>             m_QueueFamilyProperties;
		std::vector<const char*>                         m_ExtensionsList;

		// Function pointers for ray tracing related stuff
		PFN_vkGetBufferDeviceAddressKHR                  m_vkGetBufferDeviceAddressKHR;
		PFN_vkCreateAccelerationStructureKHR             m_vkCreateAccelerationStructureKHR;
		PFN_vkDestroyAccelerationStructureKHR            m_vkDestroyAccelerationStructureKHR;
		PFN_vkGetAccelerationStructureBuildSizesKHR      m_vkGetAccelerationStructureBuildSizesKHR;
		PFN_vkGetAccelerationStructureDeviceAddressKHR   m_vkGetAccelerationStructureDeviceAddressKHR;
		PFN_vkBuildAccelerationStructuresKHR             m_vkBuildAccelerationStructuresKHR;
		PFN_vkCmdBuildAccelerationStructuresKHR          m_vkCmdBuildAccelerationStructuresKHR;
		PFN_vkCmdTraceRaysKHR                            m_vkCmdTraceRaysKHR;
		PFN_vkGetRayTracingShaderGroupHandlesKHR         m_vkGetRayTracingShaderGroupHandlesKHR;
		PFN_vkCreateRayTracingPipelinesKHR               m_vkCreateRayTracingPipelinesKHR;

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
										                 
		friend class VulkanRendererAPI;
		friend class VulkanCommandPool;
	};
}
#endif