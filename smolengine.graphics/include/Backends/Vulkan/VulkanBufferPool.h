#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanBuffer.h"

#include <mutex>

namespace SmolEngine
{
	struct BufferObject
	{
		VkDescriptorBufferInfo DesriptorBufferInfo;
		VulkanBuffer VkBuffer = {};
	};

	class VulkanBufferPool
	{
	public:
		void                     Add(size_t size, uint32_t binding, VkBufferUsageFlags usage, VkDescriptorBufferInfo& outDescriptorBufferInfo, bool isStatic = false, void* data = nullptr);
		bool                     IsBindingExist(uint32_t binding);
		VulkanBuffer*            GetBuffer(uint32_t binding);
		static VulkanBufferPool* GetSingleton();

	private:
		static VulkanBufferPool*                           s_Instance;
		std::mutex                                         m_Mutex{};
		std::unordered_map<uint32_t, Ref<BufferObject>>    m_Buffers;
	};
}
#endif