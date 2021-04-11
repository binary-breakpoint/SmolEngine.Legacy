#include "stdafx.h"
#include "Vulkan/VulkanText.h"
#ifndef FROSTIUM_OPENGL_IMPL

namespace Frostium
{
	VulkanText::~VulkanText()
	{
		if (m_IndexB)
			delete m_IndexB;

		if (m_VertexB)
			delete m_VertexB;
	}

	void VulkanText::Update(const std::vector<TextVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (m_VertexB && m_IndexB)
		{
			m_VertexB->SetData(vertices.data(), vertices.size() * sizeof(TextVertex));
			m_IndexB->SetData(indices.data(), indices.size());
			return;
		}

		m_VertexB = new VulkanVertexBuffer();
		m_IndexB = new VulkanIndexBuffer();

		m_VertexB->Create(vertices.data(), vertices.size() * sizeof(TextVertex), false);
		m_IndexB->Create(indices.data(), indices.size(), false);
	}
}

#endif