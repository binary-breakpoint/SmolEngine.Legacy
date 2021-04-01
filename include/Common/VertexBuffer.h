#pragma once
#include "Common/Core.h"
#include "Common/BufferLayout.h"

#ifdef FROSTIUM_OPENGL_IMPL
#include "OpenGL/OpenglBuffer.h"
#else
#include "Vulkan/VulkanVertexBuffer.h"
#endif

namespace Frostium
{
	class VertexBuffer
	{
	public:

		VertexBuffer() = default;

		~VertexBuffer() = default;

		///  Binding

		void Bind() const;

		void UnBind() const;

		void Destory();

		/// Data

		void UploadData(const void* data, const uint32_t size, const uint32_t offset = 0);


#ifndef FROSTIUM_OPENGL_IMPL

		void CmdUpdateData(VkCommandBuffer cmdBuffer, const void* data, size_t size, uint32_t offset = 0)
		{
			m_VulkanVertexBuffer.CmdUpdateData(cmdBuffer, data, size, offset);
		}

#endif
		/// Getters

		const BufferLayout& GetLayout() const;

#ifndef FROSTIUM_OPENGL_IMPL

		VulkanVertexBuffer& GetVulkanVertexBuffer() { return m_VulkanVertexBuffer; }
#endif
		/// Setters

		void SetLayout(const BufferLayout& layout);

		/// Factory

		static Ref<VertexBuffer> Create(uint32_t size);

		static Ref<VertexBuffer> Create(void* vertices, uint32_t size);

	private:

#ifdef FROSTIUM_OPENGL_IMPL
		OpenglVertexBuffer m_OpenglVertexBuffer = {};
#else
		VulkanVertexBuffer m_VulkanVertexBuffer = {};
#endif

	};
}