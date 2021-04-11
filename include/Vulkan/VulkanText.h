#pragma once
#ifndef FROSTIUM_OPENGL_IMPL
#include "Common/Core.h"
#include "Common/Common.h"
#include "Vulkan/Vulkan.h"
#include "Vulkan/VulkanVertexBuffer.h"
#include "Vulkan/VulkanIndexBuffer.h"

namespace Frostium
{
	class VulkanText
	{
	public:

		VulkanText() = default;
		~VulkanText();

		void Update(const std::vector<TextVertex>& vertices, const std::vector<uint32_t>& indices);

	private:

		VulkanVertexBuffer* m_VertexB = nullptr;
		VulkanIndexBuffer*  m_IndexB = nullptr;
	};
}

#endif
