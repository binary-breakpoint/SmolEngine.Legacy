#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanBufferPool.h"
#include "Common/SLog.h"

namespace Frostium
{
	VulkanBufferPool* VulkanBufferPool::s_Instance = new VulkanBufferPool();

	void VulkanBufferPool::Add(size_t size, uint32_t binding, VkMemoryPropertyFlags mem, VkBufferUsageFlags usage, 
		VkDescriptorBufferInfo& outDescriptorBufferInfo)
	{
		const auto& it = m_Buffers.find(binding);
		if (it == m_Buffers.end())
		{
			Ref<BufferObject> object = std::make_shared<BufferObject>();
			object->VkBuffer.CreateBuffer(size, mem, usage);
			object->DesriptorBufferInfo.buffer = object->VkBuffer.GetBuffer();
			object->DesriptorBufferInfo.offset = 0;
			object->DesriptorBufferInfo.range = size;

#ifdef FROSTIUM_SMOLENGINE_IMPL
			{
				const std::lock_guard<std::mutex> lock(m_Mutex);

				m_Buffers[binding] = object;
			}
#else
			m_Buffers[binding] = object;
#endif

			outDescriptorBufferInfo = object->DesriptorBufferInfo;
			return;
		}

#ifdef FROSTIUM_DEBUG
		NATIVE_INFO("UBO/SSBO with binding {} is reused", binding);
#endif
		outDescriptorBufferInfo = it->second->DesriptorBufferInfo;
	}

	bool VulkanBufferPool::IsBindingExist(uint32_t binding)
	{
		return m_Buffers.find(binding) != m_Buffers.end();
	}

	VulkanBuffer* VulkanBufferPool::GetBuffer(uint32_t binding)
	{
		const auto& it = m_Buffers.find(binding);
		if (it == m_Buffers.end())
			return nullptr;

		return &it->second->VkBuffer;
	}

	VulkanBufferPool* VulkanBufferPool::GetSingleton()
	{
		return s_Instance;
	}
}
#endif