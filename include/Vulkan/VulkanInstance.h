#pragma once
#include "Vulkan/Vulkan.h"

namespace Frostium
{
	class VulkanInstance
	{
	public:

		VulkanInstance();

		~VulkanInstance();

		/// Init
		
		void Init();

		/// Getters

		const VkInstance GetInstance() const;

	private:

		bool CreateAppInfo();

		bool CreateInstance(const VkApplicationInfo& info);

	private:

		VkInstance m_VKInstance = VK_NULL_HANDLE;

		friend class VulkanRendererAPI;
	};
}
