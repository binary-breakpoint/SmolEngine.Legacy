#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanBufferPool.h"

namespace SmolEngine
{
	VulkanBufferPool* VulkanBufferPool::s_Instance = new VulkanBufferPool();

	void VulkanBufferPool::Add(size_t size, uint32_t binding, VkBufferUsageFlags usage, VkDescriptorBufferInfo& outDescriptorBufferInfo, bool isStatic, void* data)
	{
		const auto& it = m_Buffers.find(binding);
		if (it == m_Buffers.end())
		{
			Ref<BufferObject> object = std::make_shared<BufferObject>();

			if (isStatic)
			{
				usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				object->VkBuffer.CreateStaticBuffer(data, size, usage);
			}
			else
			{
				object->VkBuffer.CreateBuffer(size, usage);
			}

			object->DesriptorBufferInfo.buffer = object->VkBuffer.GetBuffer();
			object->DesriptorBufferInfo.offset = 0;
			object->DesriptorBufferInfo.range = size;

			{
				m_Mutex.lock();
				m_Buffers[binding] = object;
				m_Mutex.unlock();
			}

			outDescriptorBufferInfo = object->DesriptorBufferInfo;
			return;
		}

#ifdef SMOLENGINE_DEBUG
		DebugLog::LogInfo("UBO/SSBO with binding {} is reused", binding);
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